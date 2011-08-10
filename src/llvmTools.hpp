/**
 * @file   llvmTools.h
 * @date   27.05.2010
 * @author Ralf Karrenberg
 *
 *
 * Copyright (C) 2010 Saarland University
 *
 * This file is part of packetizedOpenCL.
 *
 * packetizedOpenCL is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * packetizedOpenCL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with packetizedOpenCL.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#ifndef _LLVMTOOLS_H
#define _LLVMTOOLS_H

#include <cassert>
#include <set>

#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/system_error.h" // error_code

#include "llvm/Module.h"
#include "llvm/Target/TargetData.h"

#include "llvm/Bitcode/ReaderWriter.h" // createModuleFromFile
#include "llvm/Support/MemoryBuffer.h" // createModuleFromFile

#include "llvm/ExecutionEngine/JITMemoryManager.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/JIT.h" //required to prevent "JIT has not been linked in" errors

#include "llvm/Target/TargetSelect.h" // InitializeNativeTarget (getExecutionEngine)

#include "llvm/Analysis/Verifier.h" // AbortProcessAction (verifyModule)

#include "llvm/PassManager.h"
#include "llvm/LinkAllPasses.h" // createXYZPass
#include "llvm/Support/StandardPasses.h" // createXYZPass
//#include "llvm/Transforms/IPO.h" //FunctionInliningPass
#include "llvm/Transforms/Utils/Cloning.h" //InlineFunction

#include "llvm/Support/Timer.h"

#include "llvm/Linker.h"


// for generateFunctionWrapper
#include "llvm/Support/IRBuilder.h"
#include "llvm/Support/TypeBuilder.h"


#include "packetizerAPI.hpp"

#ifdef DEBUG
#	define PACKETIZED_OPENCL_DEBUG(x) do { x } while (false)
#	define PACKETIZED_OPENCL_DEBUG_VISIBLE(x) x
#else
#	define PACKETIZED_OPENCL_DEBUG(x) ((void)0)
#	define PACKETIZED_OPENCL_DEBUG_VISIBLE(x)
#endif

#ifdef NDEBUG // force debug output disabled
#	undef PACKETIZED_OPENCL_DEBUG
#	define PACKETIZED_OPENCL_DEBUG(x) ((void)0)
#	undef PACKETIZED_OPENCL_DEBUG_VISIBLE
#	define PACKETIZED_OPENCL_DEBUG_VISIBLE(x)
#endif

#define PACKETIZED_OPENCL_SIMD_WIDTH 4

using namespace llvm;

namespace PacketizedOpenCL {

	// small helper function
	inline bool constantEqualsInt(const Constant* c, const uint64_t intValue) {
		assert (c);
		assert (isa<ConstantInt>(c));
		const ConstantInt* constIntOp = cast<ConstantInt>(c);
		const uint64_t constVal = *(constIntOp->getValue().getRawData());
		return (constVal == intValue);
	}
	inline bool constantIsDividableBySIMDWidth(const Constant* c) {
		assert (c);
		assert (isa<ConstantInt>(c));
		const ConstantInt* constIntOp = cast<ConstantInt>(c);
		const uint64_t constVal = *(constIntOp->getValue().getRawData());
		return (constVal % PACKETIZED_OPENCL_SIMD_WIDTH == 0);
	}

	void addNativeFunctions(Function* kernel, const cl_uint simdDim, Packetizer::Packetizer& packetizer) {

		for (Function::iterator BB=kernel->begin(), BBE=kernel->end();
				BB!=BBE; ++BB)
		{
			for (BasicBlock::iterator I=BB->begin(), IE=BB->end(); I!=IE; ++I)
			{
				if (!isa<CallInst>(I)) continue;

				CallInst* call = cast<CallInst>(I);
				Function* callee = call->getCalledFunction();

				if (std::strstr(callee->getNameStr().c_str(), "get_global_id") ||
					std::strstr(callee->getNameStr().c_str(), "get_local_id"))
				{
					// get dimension
					assert (call->getNumArgOperands() == 1);
					assert (isa<ConstantInt>(call->getArgOperand(0)));
					Constant* dimC = cast<Constant>(call->getArgOperand(0));

					if (constantEqualsInt(dimC, simdDim)) {
						// VARYING / INDEX_CONSECUTIVE / ALIGN_TRUE
						packetizer.addValueInfo(call, false, true, true);
					} else {
						// UNIFORM / INDEX_SAME / ALIGN_FALSE
						packetizer.addValueInfo(call, true, false, false);
					}

					continue;
				}

				// All other OpenCL-runtime-calls:
				// UNIFORM / INDEX_SAME / ALIGN_FALSE
				if (std::strstr(callee->getNameStr().c_str(), "get_group_id") ||
					std::strstr(callee->getNameStr().c_str(), "get_global_size") ||
					std::strstr(callee->getNameStr().c_str(), "get_local_size"))
				{
					packetizer.addValueInfo(call, true, false, false);
				}
			}
		}

	}

#if 0
	// Generate a new function that only receives a void* argument.
	// This void* is interpreted as a struct which holds exactly the values that
	// would be passed as parameters to 'f'.
	// The Wrapper extracts the values and calls f.
	// NOTE: Return values are ignored, as the wrapper should *always* have the
	//       exact same signature.
	// NOTE: currently not used!
	Function* generateFunctionWrapper(const std::string& wrapper_name, Function* f, Module* mod) {
		assert (f && mod);
		assert (f->getParent());

		if (f->getParent() != mod) {
			errs() << "WARNING: generateFunctionWrapper(): module '"
					<< mod->getModuleIdentifier()
					<< "' is not the parent of function '"
					<< f->getNameStr() << "' (parent: '"
					<< f->getParent()->getModuleIdentifier() << "')!\n";
		}

		// first make sure there is no function with that name in mod
		// TODO: Implement logic from LLVM tutorial that checks for matching extern
		//       declaration without body and, in this case, goes on.
		if (mod->getFunction(wrapper_name)) {
			errs() << "ERROR: generateFunctionWrapper(): Function with name '"
					<< wrapper_name << "' already exists in module '"
					<< mod->getModuleIdentifier() << "'!\n";
			return NULL;
		}

		// warn if f has a return value
		if (!f->getReturnType()->isVoidTy()) {
			errs() << "WARNING: generateFunctionWrapper(): target function '"
					<< f->getNameStr() << "' must not have a return type (ignored)!\n";
		}

		LLVMContext& context = mod->getContext();

		IRBuilder<> builder(context);

		// determine all arguments of f
		std::vector<const Argument*> oldArgs;
		std::vector<const Type*> oldArgTypes;
		for (Function::const_arg_iterator A=f->arg_begin(), AE=f->arg_end(); A!=AE; ++A) {
			oldArgs.push_back(A);
			oldArgTypes.push_back(A->getType());
		}

		// create a struct type with a member for each argument
		const StructType* argStructType = StructType::get(context, oldArgTypes, false);


		// create function
		//const FunctionType* fType = TypeBuilder<void(void*), true>::get(context);
		std::vector<const Type*> params;
		params.push_back(PointerType::getUnqual(argStructType));
		const FunctionType* fType = FunctionType::get(Type::getVoidTy(context), params, false);
		Function* wrapper = Function::Create(fType, Function::ExternalLinkage, wrapper_name, mod);

		// set name of argument
		Argument* arg_str = wrapper->arg_begin();
		arg_str->setName("arg_struct");

		// create entry block
		BasicBlock* entryBB = BasicBlock::Create(context, "entry", wrapper);
		builder.SetInsertPoint(entryBB);

		// create extractions of arguments out of the struct
		SmallVector<Value*, 8> extractedArgs;
		for (unsigned i=0, e=oldArgTypes.size(); i<e; ++i) {
			// create GEP
			std::vector<Value*> indices;
			indices.push_back(Constant::getNullValue(Type::getInt32Ty(context))); // step through pointer
			indices.push_back(ConstantInt::get(context, APInt(32, i))); // index of argument
			Value* gep = builder.CreateGEP(arg_str, indices.begin(), indices.end(), "");

			// create load
			LoadInst* load = builder.CreateLoad(gep, false, "");

			// store as argument for call to f
			extractedArgs.push_back(load);
		}

		// create the call to f
		CallInst* call = builder.CreateCall(f, extractedArgs.begin(), extractedArgs.end(), "");
		call->addAttribute(1, Attribute::NoCapture);
		call->addAttribute(1, Attribute::NoAlias);

		// the function returns void
		builder.CreateRetVoid();


		//wrapper->addAttribute(0, Attribute::NoUnwind); // function does not unwind stack -> why is there an index required ???
		wrapper->setDoesNotCapture(1, true); // arg ptr does not capture
		wrapper->setDoesNotAlias(1, true);   // arg ptr does not alias

		//outs() << *argStructType << "\n";
		//outs() << *wrapper << "\n";

		//verifyFunction(*wrapper, NULL);

		return wrapper;
	}
#endif

	Function* generateFunctionWrapperWithParams(const std::string& wrapper_name, Function* f, Module* mod, std::vector<const Type*>& additionalParams, const bool inlineCall) {
		assert (f && mod);
		assert (f->getParent());

		if (f->getParent() != mod) {
			errs() << "WARNING: generateFunctionWrapper(): module '"
					<< mod->getModuleIdentifier()
					<< "' is not the parent of function '"
					<< f->getNameStr() << "' (parent: '"
					<< f->getParent()->getModuleIdentifier() << "')!\n";
		}

		// first make sure there is no function with that name in mod
		// TODO: Implement logic from LLVM tutorial that checks for matching extern
		//       declaration without body and, in this case, goes on.
		if (mod->getFunction(wrapper_name)) {
			errs() << "ERROR: generateFunctionWrapper(): Function with name '"
					<< wrapper_name << "' already exists in module '"
					<< mod->getModuleIdentifier() << "'!\n";
			return NULL;
		}

		// warn if f has a return value
		if (!f->getReturnType()->isVoidTy()) {
			errs() << "WARNING: generateFunctionWrapper(): target function '"
					<< f->getNameStr() << "' must not have a return type (ignored)!\n";
		}

		LLVMContext& context = mod->getContext();

		IRBuilder<> builder(context);

		// determine all arguments of f
		std::vector<const Argument*> oldArgs;
		std::vector<const Type*> oldArgTypes;
		for (Function::const_arg_iterator A=f->arg_begin(), AE=f->arg_end(); A!=AE; ++A) {
			oldArgs.push_back(A);
			oldArgTypes.push_back(A->getType());
		}

		// create a struct type with a member for each argument
		const StructType* argStructType = StructType::get(context, oldArgTypes, false);


		// create function
		//const FunctionType* fType = TypeBuilder<void(void*), true>::get(context);
		std::vector<const Type*> params;
		params.push_back(PointerType::getUnqual(argStructType));

		for (std::vector<const Type*>::const_iterator it=additionalParams.begin(), E=additionalParams.end(); it!=E; ++it) {
			params.push_back(*it);
		}

		const FunctionType* fType = FunctionType::get(Type::getVoidTy(context), params, false);
		Function* wrapper = Function::Create(fType, Function::ExternalLinkage, wrapper_name, mod);

		// set name of argument
		Argument* arg_str = wrapper->arg_begin();
		arg_str->setName("arg_struct");

		// create entry block
		BasicBlock* entryBB = BasicBlock::Create(context, "entry", wrapper);
		builder.SetInsertPoint(entryBB);

		// create extractions of arguments out of the struct
		SmallVector<Value*, 8> extractedArgs;
		for (unsigned i=0, e=oldArgTypes.size(); i<e; ++i) {
			// create GEP
			std::vector<Value*> indices;
			indices.push_back(Constant::getNullValue(Type::getInt32Ty(context))); // step through pointer
			indices.push_back(ConstantInt::get(context, APInt(32, i))); // index of argument
			Value* gep = builder.CreateGEP(arg_str, indices.begin(), indices.end(), "");

			// create load
			LoadInst* load = builder.CreateLoad(gep, false, "");

			// store as argument for call to f
			extractedArgs.push_back(load);
		}

		// create the call to f
		CallInst* call = builder.CreateCall(f, extractedArgs.begin(), extractedArgs.end(), "");

		// the function returns void
		builder.CreateRetVoid();


		//wrapper->addAttribute(0, Attribute::NoUnwind); // function does not unwind stack -> why is there an index required ???
		wrapper->setDoesNotCapture(1, true); // arg ptr does not capture
		wrapper->setDoesNotAlias(1, true);   // arg ptr does not alias

		// inline call if required
		if (inlineCall) {
			InlineFunctionInfo IFI(NULL, new TargetData(mod));
			const bool success = InlineFunction(call, IFI);
			if (!success) {
				errs() << "WARNING: could not inline function call inside wrapper: " << *call << "\n";
			}
			assert (success);
		}

		//verifyFunction(*wrapper, NULL);

		return wrapper;
	}

	void generateOpenCLFunctions(Module* mod) {
		assert (mod);
		LLVMContext& context = mod->getContext();

		std::vector<const Type*> params;
		params.push_back(Type::getInt32Ty(context));

		// generate 'unsigned get_global_id(unsigned)' if not already there
		// represents uniform accesses to global id (e.g. of other dimensions than simd_dim)
		// is required to be broadcasted (loading from this as index otherwise would be equivalent to get_global_id_SIMD)
		if (!mod->getFunction("get_global_id")) {
			const FunctionType* fTypeG0 = FunctionType::get(Type::getInt32Ty(context), params, false);
			Function::Create(fTypeG0, Function::ExternalLinkage, "get_global_id", mod);
		}

		// generate 'unsigned get_local_id(unsigned)' if not already there
		// represents uniform accesses to local id (e.g. of other dimensions than simd_dim)
		// is required to be broadcasted (loading from this as index otherwise would be equivalent to get_global_id_SIMD)
		if (!mod->getFunction("get_local_id")) {
			const FunctionType* fTypeL0 = FunctionType::get(Type::getInt32Ty(context), params, false);
			Function::Create(fTypeL0, Function::ExternalLinkage, "get_local_id", mod);
		}

#ifdef PACKETIZED_OPENCL_OLD_PACKETIZER
		// generate 'unsigned get_global_id_split(unsigned)'
		// is replaced by get_global_id_split_SIMD during packetization
		const FunctionType* fTypeG1 = FunctionType::get(Type::getInt32Ty(context), params, false);
		Function::Create(fTypeG1, Function::ExternalLinkage, "get_global_id_split", mod);
		// generate '__m128i get_global_id_split_SIMD(unsigned)'
		// returns a vector to force splitting during packetization
		const FunctionType* fTypeG2 = FunctionType::get(VectorType::get(Type::getInt32Ty(context), PACKETIZED_OPENCL_SIMD_WIDTH), params, false);
		Function::Create(fTypeG2, Function::ExternalLinkage, "get_global_id_split_SIMD", mod);
		// generate 'unsigned get_global_id_SIMD(unsigned)'
		// does not return a vector because the simd value is loaded from the
		// begin-index of the four consecutive values
		const FunctionType* fTypeG3 = FunctionType::get(Type::getInt32Ty(context), params, false);
		Function::Create(fTypeG3, Function::ExternalLinkage, "get_global_id_SIMD", mod);

		// generate 'unsigned get_local_id_split(unsigned)'
		// is replaced by get_local_id_split_SIMD during packetization
		const FunctionType* fTypeL1 = FunctionType::get(Type::getInt32Ty(context), params, false);
		Function::Create(fTypeL1, Function::ExternalLinkage, "get_local_id_split", mod);
		// generate '__m128i get_local_id_split_SIMD(unsigned)'
		// returns a vector to force splitting during packetization
		const FunctionType* fTypeL2 = FunctionType::get(VectorType::get(Type::getInt32Ty(context), PACKETIZED_OPENCL_SIMD_WIDTH), params, false);
		Function::Create(fTypeL2, Function::ExternalLinkage, "get_local_id_split_SIMD", mod);
		// generate 'unsigned get_local_id_SIMD(unsigned)'
		// does not return a vector because the simd value is loaded from the
		// begin-index of the four consecutive values
		const FunctionType* fTypeL3 = FunctionType::get(Type::getInt32Ty(context), params, false);
		Function::Create(fTypeL3, Function::ExternalLinkage, "get_local_id_SIMD", mod);
#endif

		// generate 'void barrier(unsigned, unsigned)' if not already there
		if (!mod->getFunction("barrier")) {
			params.push_back(Type::getInt32Ty(context)); // receives 2 integers
			const FunctionType* fTypeB = FunctionType::get(Type::getInt32Ty(context), params, false);
			Function::Create(fTypeB, Function::ExternalLinkage, "barrier", mod);
		}
	}


	// taken from sdr2bc
	const Type * getPrimitiveType(Module *mod, char t) {
		switch (t)
		{
			case 'v': return Type::getVoidTy(mod->getContext());
			case 'b': return Type::getInt1Ty(mod->getContext()); //bool
			case 'c': return Type::getInt8Ty(mod->getContext()); //char
			case 'i': return Type::getInt32Ty(mod->getContext());
			case 'f': return Type::getFloatTy(mod->getContext());
			case 'd': return Type::getDoubleTy(mod->getContext());
			case 's':
			case 'x':
					  return PointerType::getUnqual(Type::getInt8Ty(mod->getContext()));
		}
		return 0;
	}
	const Type * getTypeFromString(Module *mod, const std::string &typeString) {
		std::vector<const Type *> typeList;

		int num = 0;
		for (unsigned int i = 0; i < typeString.length(); i++)
		{
			char ch = typeString[i];
			if (ch >= '0' && ch <= '9')
			{
				int digit = static_cast<int>(ch)-48;
				num = num*10 + digit;
			}
			else
			{
				const Type *ptype = getPrimitiveType(mod, ch);
				assert(ptype && "Unknown type character");
				if (num == 0 || num == 1)
					typeList.push_back(ptype);
				else
					typeList.push_back(ArrayType::get(ptype, num));
				num = 0;
			}
		}
		if (typeList.size() == 1)
		{
			return typeList[0];
		}
		else if (typeList.size() > 1)
		{
			return StructType::get(mod->getContext(), typeList);
		}
		return 0;
	}

	Function* createExternalFunction(const std::string& name, const FunctionType* fType, Module* mod) {
		assert (mod);
		return Function::Create(fType, Function::ExternalLinkage, name, mod);
	}
	Function* createExternalFunction(const std::string& name, const Type* returnType, std::vector<const Type*>& paramTypes, Module* mod) {
		assert (mod);
		const FunctionType* fType = FunctionType::get(returnType, paramTypes, false);
		return Function::Create(fType, Function::ExternalLinkage, name, mod);
	}

}



// to be removed :)
namespace PacketizedOpenCL {

	Function* getFunction(const std::string& name, Module* module) {
		assert (module);
		return module->getFunction(name);
	}


	Module* createModuleFromFile(const std::string & fileName) {

		std::string errorMessage;

		//create memory buffer for file

		OwningPtr<MemoryBuffer> fileBuffer;
		error_code e = MemoryBuffer::getFile(fileName.c_str(), fileBuffer);

		if (e) {
			errs() << "Error reading file '"
				<< fileName << "': " << e.message() << "\n";
			return NULL;
		}

		if (!fileBuffer) {
			errs() << "Error reading file '" << fileName << "'.\n";
			return NULL;
		}

		if (fileBuffer->getBufferSize() & 3) {
			errs() << "Error: Bitcode stream should be "
				<< "a multiple of 4 bytes in length\n";
			return NULL;
		}

		//parse file

		Module* mod = ParseBitcodeFile(fileBuffer.get(), getGlobalContext(), &errorMessage);

		if (errorMessage != "") {
			errs() << "Error reading bitcode file: " << errorMessage << "\n";
			return NULL;
		}

		if (!mod) {
			errs() << "Error reading bitcode file.\n";
			return NULL;
		}

		return mod;
	}


	void writeModuleToFile(const Module * M, const std::string & fileName) {
		assert (M);
		std::string errorMessage = "";
		raw_fd_ostream file(fileName.c_str(), errorMessage);
		M->print(file, NULL);
		file.close();
		if (errorMessage != "") {
			errs() << "ERROR: printing module to file failed: " << errorMessage << "\n";
		}
	}
	void writeFunctionToFile(const Function * F, const std::string & fileName) {
		assert (F);
		std::string errorMessage = "";
		raw_fd_ostream file(fileName.c_str(), errorMessage);
		F->print(file, NULL);
		file.close();
		if (errorMessage != "") {
			errs() << "ERROR: printing function to file failed: " << errorMessage << "\n";
		}
	}
	

	//declare as static to prevent creation of several JITs
	static ExecutionEngine *globalExecEngine = 0;

	ExecutionEngine* createExecutionEngine(Module* mod) {

		if (globalExecEngine == 0)
		{
			//we first have to initialize the native target for code generation
			const bool initFailed = InitializeNativeTarget();

			if (initFailed) {
				errs() << "ERROR: could not initialize native target (required for "
					<< "LLVM execution engine)\n";
				return NULL;
			}

			std::string errorMessage = "";

			EngineBuilder eb = EngineBuilder(mod);
			eb.setEngineKind(EngineKind::JIT);
			eb.setErrorStr(&errorMessage);
			eb.setJITMemoryManager(JITMemoryManager::CreateDefaultMemManager());
			eb.setOptLevel(CodeGenOpt::Aggressive);
			eb.setAllocateGVsWithCode(false);
			eb.setCodeModel(CodeModel::Default);
			//eb.setMArch("x86-64");
			//eb.setMCPU("corei7");

			globalExecEngine = eb.create();

			if (errorMessage != "") {
				errs() << "ERROR: could not create execution engine for module "
					<< mod->getModuleIdentifier() << ": " << errorMessage << "\n";
				return NULL;
			}

			if (!globalExecEngine) {
				errs() << "ERROR: could not create execution engine for module "
					<< mod->getModuleIdentifier() << "!\n";
				return NULL;
			}
		}

		return globalExecEngine;
	}

	ExecutionEngine * getExecutionEngine(Module* mod) {
		return createExecutionEngine(mod);
	}

	void* getPointerToFunction(Module * mod, Function * func)
	{
		ExecutionEngine * execEngine = createExecutionEngine(mod);
		if (!execEngine) exit(-1);

		return execEngine->getPointerToFunction(func);
	}

	void* getPointerToFunction(Module* mod, std::string functionName) {

		ExecutionEngine *execEngine = createExecutionEngine(mod);
		if (!execEngine) exit(-1);

		Function* f = mod->getFunction(functionName);
		if (!f) {
			errs() << "Error: Function '" << functionName << "' not found in module "
				<< mod->getModuleIdentifier() << "!\n";
			exit(-1);
		}

		return execEngine->getPointerToFunction(f);
	}

	void* getPointerToFunction(ExecutionEngine * engine, Function * func) {
		assert (engine && func);
		return engine->getPointerToFunction(func);
	}

	unsigned getPrimitiveSizeInBits(const Type* type) {
		assert (type);
		return type->getPrimitiveSizeInBits();
	}
	bool isPointerType(const Type* type) {
		assert (type);
		return type->getTypeID() == Type::PointerTyID;
	}
	const Type* getContainedType(const Type* type, const unsigned index) {
		assert (type);
		assert (type->getContainedType(index));
		return type->getContainedType(index);
	}

	TargetData* getTargetData(Module* mod) {
		assert (mod);
		return new TargetData(mod);
	}
	void setTargetData(Module* mod, const std::string& dataLayout, const std::string& targetTriple) {
		mod->setDataLayout(dataLayout);
		mod->setTargetTriple(targetTriple);
	}
	const std::string& getTargetTriple(const Module* mod) {
		assert (mod);
		return mod->getTargetTriple();
	}

	void setDataLayout(Module* mod, const std::string& dataLayout) {
		mod->setDataLayout(dataLayout);
	}
	uint64_t getTypeSizeInBits(const TargetData* targetData, const Type* type) {
		assert (targetData && type);
		return targetData->getTypeSizeInBits(type);
	}


	void runStaticConstructorsDestructors(Module* mod, ExecutionEngine* engine, const bool isDtors) {
		assert (mod && engine);
		engine->runStaticConstructorsDestructors(mod, isDtors);
	}

	unsigned getNumArgs(const Function* f) {
		assert (f);
		return f->arg_size();
	}

	const Type* getArgumentType(const Function* f, const unsigned arg_index) {
		assert (f);
		assert (arg_index < f->arg_size());
		Function::const_arg_iterator A = f->arg_begin();
		for (unsigned i=0; i<arg_index; ++i) ++A; //is there a better way? :P
		return A->getType();
	}

	unsigned getAddressSpace(const Type* type) {
		if (!isa<PointerType>(type)) return 0;
		const PointerType* pType = cast<PointerType>(type);
		return pType->getAddressSpace();
	}



	void inlineFunctionCalls(Function* f, TargetData* targetData=NULL) {
		//PACKETIZED_OPENCL_DEBUG( outs() << "  inlining function calls... "; );
		bool functionChanged = true;
		while (functionChanged) {
			functionChanged = false;
			for (Function::iterator BB=f->begin(); BB!=f->end(); ++BB) {
				bool blockChanged = false;
				for (BasicBlock::iterator I=BB->begin(); !blockChanged && I!=BB->end();) {
					if (!isa<CallInst>(I)) { ++I; continue; }
					CallInst* call = cast<CallInst>(I++);
					Function* callee = call->getCalledFunction();
					if (!callee) {
						//errs() << "ERROR: could not inline function call: " << *call;
						continue;
					}
					if (callee->getAttributes().hasAttrSomewhere(Attribute::NoInline)) {
						//PACKETIZED_OPENCL_DEBUG( outs() << "    function '" << callee->getNameStr() << "' has attribute 'no inline', ignored call!\n"; );
						continue;
					}

					const std::string calleeName = callee->getNameStr(); //possibly deleted by InlineFunction()

					InlineFunctionInfo IFI(NULL, targetData);
					blockChanged = InlineFunction(call, IFI);
					//PACKETIZED_OPENCL_DEBUG(
						//if (blockChanged) outs() << "  inlined call to function '" << calleeName << "'\n";
						//else errs() << "  inlining of call to function '" << calleeName << "' FAILED!\n";
					//);
					functionChanged |= blockChanged;
				}
			}
		}
		//PACKETIZED_OPENCL_DEBUG( outs() << "done.\n"; );
	}


#if 0
	/// adopted from: llvm-2.5/tools/llvm-ld
	void optimizeFunction(Function* f) {
		assert (f);
		assert (f->getParent());
		Module* mod = f->getParent();
		TargetData* targetData = new TargetData(mod);

		//PACKETIZED_OPENCL_DEBUG( outs() << "optimizing function '" << f->getNameStr() << "'...\n"; );


		//PassManager Passes;
		FunctionPassManager Passes(mod);
		Passes.add(targetData);
//		createStandardFunctionPasses(&Passes, 3);
		Passes.add(createScalarReplAggregatesPass(2048)); // Break up allocas, override default threshold of maximum struct size of 128 bytes
		Passes.add(createInstructionCombiningPass());
		Passes.add(createJumpThreadingPass());        // Thread jumps.
		Passes.add(createReassociatePass());
		Passes.add(createInstructionCombiningPass());
		Passes.add(createCFGSimplificationPass());
		Passes.add(createAggressiveDCEPass()); //fpm.add(createDeadCodeEliminationPass());
		Passes.add(createScalarReplAggregatesPass(2048)); // Break up allocas, override default threshold of maximum struct size of 128 bytes
		Passes.add(createLICMPass());                 // Hoist loop invariants   //Pass
		Passes.add(createGVNPass());                  // Remove redundancies
		Passes.add(createMemCpyOptPass());            // Remove dead memcpy's
		Passes.add(createDeadStoreEliminationPass()); // Nuke dead stores
		Passes.add(createInstructionCombiningPass());
		Passes.add(createJumpThreadingPass());        // Thread jumps.
		Passes.add(createPromoteMemoryToRegisterPass()); // Cleanup jumpthread.
		Passes.add(createCFGSimplificationPass());

		//custom
		Passes.add(createSCCPPass()); //fpm.add(createConstantPropagationPass());
		Passes.add(createTailCallEliminationPass());
		Passes.add(createAggressiveDCEPass());

		// custom, added after llvm 2.7
		// -> wrong results in OpenCL-Mandelbrot!!!
		// -> without: bad optimization of OpenCL-SimpleConvolution
		/*Passes.add(createLoopRotatePass());            // Rotate Loop
		  Passes.add(createLICMPass());                  // Hoist loop invariants
		  Passes.add(createLoopUnswitchPass(false));
		  Passes.add(createInstructionCombiningPass());
		  Passes.add(createIndVarSimplifyPass());        // Canonicalize indvars
		  Passes.add(createLoopDeletionPass());          // Delete dead loops
		  Passes.add(createLoopUnrollPass());          // Unroll small loops
		 */
		//clean up again
		Passes.add(createInstructionCombiningPass());
		Passes.add(createReassociatePass());
		Passes.add(createGVNPass());
		Passes.add(createCFGSimplificationPass());
		Passes.add(createAggressiveDCEPass());

		PACKETIZED_OPENCL_DEBUG( Passes.add(createVerifierPass()); );

		//PACKETIZED_OPENCL_DEBUG_VISIBLE( llvm::TimerGroup tg("llvmOptimization"); )
		//PACKETIZED_OPENCL_DEBUG_VISIBLE( llvm::Timer t("llvmOptimization", tg); )
		//PACKETIZED_OPENCL_DEBUG( t.startTimer(); );

		Passes.doInitialization();
		Passes.run(*f);
		Passes.doFinalization();

		//PACKETIZED_OPENCL_DEBUG( t.stopTimer(); );
		//PACKETIZED_OPENCL_DEBUG( outs() << "done.\ndone.\n"; );
		//PACKETIZED_OPENCL_DEBUG( tg.print(outs()); );
	}
#else

	/// adopted from: llvm-2.9/include/llvm/Support/StandardPasses.h
	void optimizeFunction(Function* f, const bool disableLICM=false, const bool disableLoopRotate=false) {
		assert (f);
		assert (f->getParent());
		Module* mod = f->getParent();
		TargetData* targetData = new TargetData(mod);

		const unsigned OptimizationLevel = 3;
		const bool OptimizeSize = false;
		const bool UnitAtATime = true;
		const bool UnrollLoops = true;
		const bool SimplifyLibCalls = true;
		const bool HaveExceptions = false;
		Pass* InliningPass = createFunctionInliningPass(275);

		//PassManager Passes;
		FunctionPassManager Passes(mod);
		Passes.add(targetData);
		
		//
		// custom
		//
		Passes.add(createScalarReplAggregatesPass(-1, false));

		//
		// createStandardFunctionPasses
		//
		Passes.add(createCFGSimplificationPass());
		Passes.add(createPromoteMemoryToRegisterPass());
		Passes.add(createInstructionCombiningPass());

		// Add TypeBasedAliasAnalysis before BasicAliasAnalysis so that
		// BasicAliasAnalysis wins if they disagree. This is intended to help
		// support "obvious" type-punning idioms.
		Passes.add(createTypeBasedAliasAnalysisPass());
		Passes.add(createBasicAliasAnalysisPass());

		// Start of function pass.
		// Break up aggregate allocas, using SSAUpdater.
		Passes.add(createScalarReplAggregatesPass(-1, false));
		Passes.add(createEarlyCSEPass());              // Catch trivial redundancies
		if (SimplifyLibCalls)
			Passes.add(createSimplifyLibCallsPass());    // Library Call Optimizations
		Passes.add(createJumpThreadingPass());         // Thread jumps.
		Passes.add(createCorrelatedValuePropagationPass()); // Propagate conditionals
		Passes.add(createCFGSimplificationPass());     // Merge & remove BBs
		Passes.add(createInstructionCombiningPass());  // Combine silly seq's

		Passes.add(createTailCallEliminationPass());   // Eliminate tail calls
		Passes.add(createCFGSimplificationPass());     // Merge & remove BBs
		Passes.add(createReassociatePass());           // Reassociate expressions
		if (!disableLoopRotate) Passes.add(createLoopRotatePass());            // Rotate Loop // makes packetized Mandelbrot fail
		if (!disableLICM) Passes.add(createLICMPass());                  // Hoist loop invariants // makes scalar driver crash after optimization
		Passes.add(createLoopUnswitchPass(OptimizeSize || OptimizationLevel < 3));
		Passes.add(createInstructionCombiningPass());
		Passes.add(createIndVarSimplifyPass());        // Canonicalize indvars
		Passes.add(createLoopIdiomPass());             // Recognize idioms like memset.
		Passes.add(createLoopDeletionPass());          // Delete dead loops
		if (UnrollLoops)
			Passes.add(createLoopUnrollPass());          // Unroll small loops
		Passes.add(createInstructionCombiningPass());  // Clean up after the unroller
		if (OptimizationLevel > 1)
			Passes.add(createGVNPass());                 // Remove redundancies
		Passes.add(createMemCpyOptPass());             // Remove memcpy / form memset
		Passes.add(createSCCPPass());                  // Constant prop with SCCP

		// Run instcombine after redundancy elimination to exploit opportunities
		// opened up by them.
		Passes.add(createInstructionCombiningPass());
		Passes.add(createJumpThreadingPass());         // Thread jumps
		Passes.add(createCorrelatedValuePropagationPass());
		Passes.add(createDeadStoreEliminationPass());  // Delete dead stores
		Passes.add(createAggressiveDCEPass());         // Delete dead instructions
		Passes.add(createCFGSimplificationPass());     // Merge & remove BBs

		PACKETIZED_OPENCL_DEBUG( Passes.add(createVerifierPass()); );

		Passes.doInitialization();
		Passes.run(*f);
		Passes.doFinalization();
	}
#endif


	Constant * createPointerConstant(void * thePointer, const Type * pointerType) {
		//use target data for pointer, this might not work on 64 bit!
		//TargetData td(mod);
		//td.getIntPtrType();
		Constant * constInt = ConstantInt::get(Type::getInt64Ty(getGlobalContext()), (size_t) thePointer);
		Constant * voidPtr  = ConstantExpr::getIntToPtr(constInt, PointerType::getUnqual(Type::getInt32Ty(getGlobalContext())));
		Constant * typedPtr = ConstantExpr::getBitCast(voidPtr, pointerType);

		return typedPtr;
	}
	const char * readConstant(Constant *& target,const Type * type, const char * position) {
		switch (type->getTypeID()) {
			case Type::PointerTyID: {
										const PointerType* ptrType = static_cast<const PointerType*>(type);
										target = createPointerConstant((void*) position, PointerType::getUnqual(ptrType));
										return position + sizeof(void*);
									}

			case Type::FunctionTyID: {
										 const FunctionType * funcType = static_cast<const FunctionType*>(type);
										 target = createPointerConstant((void*) position, PointerType::getUnqual(funcType));
										 return position + sizeof(void*);
									 }

			case Type::IntegerTyID: {
										int value = *((int*) position);
										target = ConstantInt::get(getGlobalContext(), APInt(32, value));
										return position + sizeof(int);
									}

			case Type::FloatTyID: {
									  float value = *((float*) position);
									  APFloat apFloat = APFloat(value);
									  target = ConstantFP::get(getGlobalContext(), apFloat);
									  return position + sizeof(float);
								  }

			case Type::StructTyID: {
									   const StructType * structType = static_cast<const StructType*>(type);
									   std::vector<Constant*> elements;

									   for(StructType::element_iterator it = structType->element_begin();
											   it != structType->element_end();
											   ++it) {
										   Constant * elementTarget = NULL;
										   position = readConstant(elementTarget, *it, position);
										   elements.push_back(elementTarget);
									   }
									   target = ConstantStruct::get(structType, elements);
									   return position;
								   }

			case Type::VectorTyID: {
									   const VectorType * vectorType = static_cast<const VectorType*>(type);
									   const Type * elementType = vectorType->getElementType();
									   std::vector<Constant*> elements;
									   int numElements = vectorType->getNumElements();

									   for(int index = 0; index < numElements; ++index) {
										   Constant * elementTarget = NULL;
										   position = readConstant(elementTarget, elementType, position);
										   elements.push_back(elementTarget);
									   }

									   target = ConstantVector::get(vectorType, elements);
									   return position;
								   }

			case Type::ArrayTyID: {
									  const ArrayType * arrayType = static_cast<const ArrayType*>(type);
									  const Type * elementType = arrayType->getElementType();
									  std::vector<Constant*> elements;
									  uint64_t numElements = arrayType->getNumElements();

									  elements.resize(numElements);
									  for(uint64_t index = 0; index < numElements; ++index) {
										  Constant * elementTarget;
										  position = readConstant(elementTarget, elementType, position);
										  elements.push_back(elementTarget);
									  }

									  target = ConstantArray::get(arrayType, elements);
									  return position;
								  }

			default:
								  errs() << "ERROR: Unsupported Type in createConstant(): " << *type << "\n";
		};
		return NULL;
	}
	Constant * createConstant(const Type * type, const char * value) {
		if (isa<PointerType>(type)) {
			const PointerType * ptrType = cast<PointerType>(type);

			if (ptrType->getElementType()->getTypeID() == Type::FunctionTyID) {
				return createPointerConstant((void*) value, ptrType);
			}

			Constant * innerConstant = NULL;
			//dereference inner type
			const PointerType * pointerType = static_cast<const PointerType*>(type);
			const Type * derefType = pointerType->getElementType();

			readConstant(innerConstant, derefType, value);

			return innerConstant;
		} else {
			Constant * c = NULL;
			readConstant(c, type, value);

			return c;
		}

	}
	Constant * createFunctionPointer(Function * decl, void * ptr) {
		assert (ptr && "calling a NULL pointer will cause a seg fault");
		assert (decl && "invalid declaration given in createFunctionPointer");

		//fetch the type
		const PointerType * callType = cast<PointerType>(decl->getType());
		const FunctionType * calledFuncType = cast<FunctionType>(callType->getElementType());

		//create the constant
		return createConstant(calledFuncType, (const char*) ptr);
	}

	void replaceAllUsesWith(Function* oldFn, Function* newFn) {
		assert (oldFn && newFn);
		oldFn->replaceAllUsesWith(newFn);
	}
	void replaceAllUsesWith(Function* oldFn, Constant* newVal) {
		assert (oldFn && newVal);
		oldFn->replaceAllUsesWith(newVal);
	}
	void replaceAllUsesWith(Value* oldVal, Value* newVal) {
		assert (oldVal && newVal);
		oldVal->replaceAllUsesWith(newVal);
	}



	// link source module into target module
	// FIXME: this function is required to resolve Linker usage in packetizer...
	Module* linkInModule(Module* target, Module* source) {
		assert (source && target);
		outs() << "linking src module '" << source->getModuleIdentifier()
				<< "' into dest module '" << target->getModuleIdentifier()
				<< "'... ";
		std::auto_ptr<llvm::Linker> linker(new llvm::Linker("jitRT Linker", target));
		assert (linker.get() != 0);
		std::string errorMessage;
		if (linker->LinkInModule(source, &errorMessage)) {
			errs() << "Could not link source module into cloned target: " << errorMessage << "\n";
			return NULL;
		}

		return linker->releaseModule();
	}

}


#endif /* _LLVMTOOLS_H */
