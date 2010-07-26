/**
 * @file   llvmTools.h
 * @date   27.05.2010
 * @author Ralf Karrenberg
 *
 *
 * Copyright (C) 2010 Saarland University
 *
 * This file is part of packetizedOpenCLDriver.
 *
 * packetizedOpenCLDriver is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * packetizedOpenCLDriver is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with packetizedOpenCLDriver.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#ifndef _LLVMTOOLS_H
#define _LLVMTOOLS_H

#include <cassert>
#include <set>

#include <llvm/Support/raw_ostream.h>

#include <llvm/Module.h>
#include <llvm/Target/TargetData.h>

#include <llvm/Bitcode/ReaderWriter.h> // createModuleFromFile
#include <llvm/Support/MemoryBuffer.h> // createModuleFromFile

#include <llvm/ExecutionEngine/JITMemoryManager.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/JIT.h> //required to prevent "JIT has not been linked in" errors

#include <llvm/Target/TargetSelect.h> // InitializeNativeTarget (getExecutionEngine)

#include <llvm/Analysis/Verifier.h> // AbortProcessAction (verifyModule)

#include <llvm/PassManager.h>
#include <llvm/LinkAllPasses.h> // createXYZPass
#include <llvm/Support/StandardPasses.h> // createXYZPass
//#include <llvm/Transforms/IPO.h> //FunctionInliningPass
#include <llvm/Transforms/Utils/Cloning.h> //InlineFunction

#include <llvm/Support/Timer.h>

#include <llvm/Linker.h>


// for generateFunctionWrapper
#include <llvm/Support/IRBuilder.h>
#include <llvm/Support/TypeBuilder.h>

#ifdef DEBUG
#define PACKETIZED_OPENCL_DRIVER_DEBUG(x) do { x } while (false)
#define PACKETIZED_OPENCL_DRIVER_DEBUG_VISIBLE(x) x
#else
#define PACKETIZED_OPENCL_DRIVER_DEBUG(x) ((void)0)
#define PACKETIZED_OPENCL_DRIVER_DEBUG_VISIBLE(x)
#endif

#ifdef NDEBUG // force debug output disabled
#undef PACKETIZED_OPENCL_DRIVER_DEBUG
#define PACKETIZED_OPENCL_DRIVER_DEBUG(x) ((void)0)
#undef PACKETIZED_OPENCL_DRIVER_DEBUG_VISIBLE
#define PACKETIZED_OPENCL_DRIVER_DEBUG_VISIBLE(x)
#endif

using namespace llvm;

namespace PacketizedOpenCLDriver {

	// Generate a new function that only receives a void* argument.
	// This void* is interpreted as a struct which holds exactly the values that
	// would be passed as parameters to 'f'.
	// The Wrapper extracts the values and calls f.
	// NOTE: Return values are ignored, as the wrapper should *always* have the
	//       exact same signature.
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
		//std::map<std::string, Value*> NamedValues;

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
		//std::map<std::string, Value*> NamedValues;

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
		call->addAttribute(1, Attribute::NoCapture);
		call->addAttribute(1, Attribute::NoAlias);

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

	void generateOpenCLFunctions(Module* mod, const unsigned simdWidth) {
		assert (mod);
		LLVMContext& context = mod->getContext();

		std::vector<const Type*> params;
		params.push_back(Type::getInt32Ty(context));

		// generate 'unsigned get_global_id(unsigned)' if not already there
		if (!mod->getFunction("get_global_id")) {
			const FunctionType* fType0 = FunctionType::get(Type::getInt32Ty(context), params, false);
			Function::Create(fType0, Function::ExternalLinkage, "get_global_id", mod);
		}

		// generate 'unsigned get_local_id(unsigned)' if not already there
		if (!mod->getFunction("get_local_id")) {
			const FunctionType* fType0 = FunctionType::get(Type::getInt32Ty(context), params, false);
			Function::Create(fType0, Function::ExternalLinkage, "get_local_id", mod);
		}

		// generate 'unsigned get_global_id_split(unsigned)'
		const FunctionType* fType1 = FunctionType::get(Type::getInt32Ty(context), params, false);
		Function::Create(fType1, Function::ExternalLinkage, "get_global_id_split", mod);

		// generate 'unsigned get_local_id_split(unsigned)'
		const FunctionType* fType2 = FunctionType::get(Type::getInt32Ty(context), params, false);
		Function::Create(fType2, Function::ExternalLinkage, "get_local_id_split", mod);

		// generate '__m128i get_global_id_SIMD(unsigned)'
		const FunctionType* fType3 = FunctionType::get(VectorType::get(Type::getInt32Ty(context), simdWidth), params, false);
		Function::Create(fType3, Function::ExternalLinkage, "get_global_id_SIMD", mod);

		// generate '__m128i get_local_id_SIMD(unsigned)'
		const FunctionType* fType4 = FunctionType::get(VectorType::get(Type::getInt32Ty(context), simdWidth), params, false);
		Function::Create(fType4, Function::ExternalLinkage, "get_local_id_SIMD", mod);

		// generate 'void barrier(unsigned, unsigned)' if not already there
		if (!mod->getFunction("barrier")) {
			params.push_back(Type::getInt32Ty(context)); // receives 2 ints
			const FunctionType* fType4 = FunctionType::get(Type::getInt32Ty(context), params, false);
			Function::Create(fType4, Function::ExternalLinkage, "barrier", mod);
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

	Function* createExternalFunction(const std::string& name, const Type* returnType, std::vector<const Type*>& paramTypes, Module* mod) {
		assert (mod);
		const FunctionType* fType = FunctionType::get(returnType, paramTypes, false);
		return Function::Create(fType, Function::ExternalLinkage, name, mod);
	}

	Function* generatePacketPrototypeFromOpenCLKernel(const Function* kernel, const std::string& packetKernelName, Module* mod, const unsigned simdWidth) {
		assert (kernel && mod);
		assert (kernel->getParent() == mod);
		LLVMContext& context = mod->getContext();

		std::vector<const Type*> params;

		for (Function::const_arg_iterator A=kernel->arg_begin(), AE=kernel->arg_end(); A!=AE; ++A) {
			const Type* argType = A->getType();
			if (!argType->isPointerTy()) {
				// constant param
				params.push_back(argType);
				continue;
			}

			const PointerType* pType = cast<PointerType>(argType);

			switch (pType->getAddressSpace()) {
				case 0: // CL_PRIVATE
					{
						params.push_back(pType);
						break;
					}
				case 1: // CL_GLOBAL
					{
						// generate packet type if possible
						const Type* contType = pType->getContainedType(0);
						if (!contType->isIntegerTy() && !contType->isFloatTy()) {
							errs() << "ERROR: bad __global parameter type found: " << *contType << " (can only handle int/float)!\n";
							return NULL;
						}
						TargetData* td = new TargetData(mod);
						if (td->getTypeSizeInBits(contType) > 32) {
							errs() << "ERROR: bad __global parameter type found: " << *contType << " (can not handle data types with precision > 32bit)!\n";
							return NULL;
						}
						const VectorType* vType = contType->isIntegerTy()
							? VectorType::get(Type::getInt32Ty(context), simdWidth) //make i32 out of any integer type
							: VectorType::get(contType, simdWidth);
						params.push_back(PointerType::get(vType, pType->getAddressSpace()));

						delete td;
						break;
					}
				default:
					{
						errs() << "ERROR: bad address space for OpenCL found: " << pType->getAddressSpace() << "\n";
						return NULL;
					}
			}

		}

		const FunctionType* fType = FunctionType::get(Type::getVoidTy(context), params, false);
		return Function::Create(fType, Function::ExternalLinkage, packetKernelName, mod);
	}

	// Helper for replaceNonContiguousIndexUsage()
	// Returns true, if all instructions that depend upon I are only constant linear
	// modifications of the index.
	// Recursion stops at GEP instructions.
	// Returns false in all cases where this cannot be verified
	bool indexIsOnlyUsedWithLinearModifications(Instruction* I, Instruction* parent, std::set<Instruction*>& safePathVec, std::set<Instruction*>& visited) {
		// if this use is a GEP, this path is fine (don't go beyond GEPs)
		if (isa<GetElementPtrInst>(I)) return true;

		// if this is a loop, this path is fine
		if (visited.find(I) != visited.end()) return true;
		visited.insert(I);

		// if this use is inside safeInstVec already, we can stop recursion
		if (safePathVec.find(I) != safePathVec.end()) return true;

		// check for safe operations (rest of path still has to be checked)
		// TODO: other cases? ZExt? FPExt? Trunc? FPTrunc?
		bool useIsBad = true;

		switch (I->getOpcode()) {
			case Instruction::Add : //fallthrough
			case Instruction::Sub : {
				// add/sub is okay if other operand is constant
				// TODO: uniform is also okay

				// get other operand
				Value* op = I->getOperand(0) == parent ? I->getOperand(1) : I->getOperand(0);
				// if operand is constant, the use is okay :)
				if (isa<Constant>(op)) useIsBad = false;
				break;
			}

			case Instruction::SExt    : useIsBad = false; break; // SExt is okay
			case Instruction::ICmp    : useIsBad = false; break; // ICmp is okay
			case Instruction::FCmp    : useIsBad = false; break; // FCmp is okay
			case Instruction::FPToUI  : useIsBad = false; break; // FPToUI is okay
			case Instruction::FPToSI  : useIsBad = false; break; // FPToSI is okay
			case Instruction::UIToFP  : useIsBad = false; break; // UIToFP is okay
			case Instruction::SIToFP  : useIsBad = false; break; // SIToFP is okay
			case Instruction::FPExt   : useIsBad = false; break; // FPExt is okay
			case Instruction::BitCast : useIsBad = false; break; // BitCast is okay

			case Instruction::Ret     : useIsBad = false; break; // Ret is okay
			case Instruction::Br      : useIsBad = false; break; // Br is okay
			case Instruction::Switch  : useIsBad = false; break; // Switch is okay

			case Instruction::PHI     : useIsBad = false; break; // PHI is okay
		}

		if (useIsBad) return false;

		// recurse into uses
		for (Instruction::use_iterator U=I->use_begin(), UE=I->use_end(); U!=UE; ++U) {
			if (Instruction* useI = dyn_cast<Instruction>(U))
				if (!indexIsOnlyUsedWithLinearModifications(useI, I, safePathVec, visited)) return false;
		}

		// all uses are okay
		safePathVec.insert(I);
		return true;
	}

	// Replace calls to oldF by newF in function f
	// wherever the result of the call is not used directly in a load or store (via GEP),
	// e.g. as part of some index arithmetic accessing array[i*4+1]
	// Effectively, this is an optimization of array accesses that load or store
	// contiguous indices.
	void replaceNonContiguousIndexUsage(Function* f, Function* oldF, Function* newF) {
		assert (f && oldF && newF);
		assert (oldF->getReturnType()->isIntegerTy());
		assert (newF->getReturnType()->isIntegerTy());
		// TODO: check if signature matches (we are replacing oldF by newF...)

		std::vector<CallInst*> deleteVec;
		std::set<Instruction*> safePathVec; // marks instructions whose use-subtrees are entirely safe

		for (Function::use_iterator U=oldF->use_begin(), UE=oldF->use_end(); U!=UE; ++U) {
			if (!isa<CallInst>(U)) continue;
			CallInst* call = cast<CallInst>(U);
			if (call->getParent()->getParent() != f) continue; // ignore uses in other functions

			// generate new call (we might still need the old one) to newF
			std::vector<Value*> args;
			for (CallInst::op_iterator OP=call->op_begin()+1, OPE=call->op_end(); OP!=OPE; ++OP) {
				args.push_back(*OP);
			}
			// TODO: where to place the new call? maybe directly in front of use?
			//       for now, place it at the same spot as the old call
			CallInst* newCall = CallInst::Create(newF, args.begin(), args.end(), call->getName(), call);
			//CallInst* newCall = CallInst::Create(newF, call->op_begin()+1, call->op_end(), call->getName(), call);
			newCall->setTailCall(true);


			// iterate over uses of the call's result
			for (CallInst::use_iterator U2=call->use_begin(), UE2=call->use_end(); U2!=UE2; ) {
				assert (isa<Instruction>(U2));
				Instruction* useI = cast<Instruction>(U2++);

				// attempt to analyze path
				// TODO: entirely uniform paths are okay as well
				std::set<Instruction*> visited;
				if (indexIsOnlyUsedWithLinearModifications(useI, call, safePathVec, visited)) continue;

				// otherwise, we have a use that (conservatively) has to be split
				// ( = replaced with newF)
				// -> replace call by new call in this use
				useI->replaceUsesOfWith(call, newCall);
			}

			if (call->use_empty()) deleteVec.push_back(call); // still iterating over calls, postpone deletion
			if (newCall->use_empty()) newCall->eraseFromParent(); // can erase directly
		}
		// delete dead calls
		for (std::vector<CallInst*>::iterator it=deleteVec.begin(), E=deleteVec.end(); it!=E; ++it) {
			(*it)->eraseFromParent();
		}
	}

	// helper for fixUniformPacketizedArrayAccesses()
	// Recursively searches all dependency paths top-down from I and collects all
	// GEP instructions.
	void findDependentGEPs(Instruction* I, std::vector<GetElementPtrInst*>& gepVec, std::set<Instruction*>& visited) {
		if (visited.find(I) != visited.end()) return;
		visited.insert(I);

		if (GetElementPtrInst* gep = dyn_cast<GetElementPtrInst>(I)) {
			gepVec.push_back(gep);
			return;
		}

		for (Instruction::use_iterator U=I->use_begin(), UE=I->use_end(); U!=UE; ++U) {
			if (Instruction* useI = dyn_cast<Instruction>(U))
				findDependentGEPs(useI, gepVec, visited);
		}
	}


	// Modify all GEPs in f that use indices returned by splitF (that have to be split).
	// This fixes access to "uniform packetized" arrays, which have packetized
	// type but hold the exact same data as the scalar array (e.g. float[128] ->  __m128[32]).
	// Instead of accessing <i0+0, i1+1, i2+2, i3+3> as in the fully packetized case,
	// these have to access <i0/4 + i0%4, i1/4 + i1%4, i2/4 + i2%4, i3/4 + i3%4>.
	//
	// Example:
	//
	// BEFORE:
	// %array = load ...                                ; <<4 x float>*>
	// %indices = splitF()                              ; <<4 x i32>>
	// %idx0 = extractelement <4 x i32> %indices, i32 0 ; <i32>    // 0
	// %idx1 = extractelement <4 x i32> %indices, i32 1 ; <i32>    // 2
	// %idx2 = extractelement <4 x i32> %indices, i32 2 ; <i32>    // 4
	// %idx3 = extractelement <4 x i32> %indices, i32 3 ; <i32>    // 6
	// %gep0 = GEP %array, %idx0, 0                     ; <float>  // scalar element 0
	// %gep1 = GEP %array, %idx1, 1                     ; <float>  // scalar element 2*4+1=9  instead of 2
	// %gep2 = GEP %array, %idx2, 2                     ; <float>  // scalar element 4*4+2=18 instead of 4
	// %gep3 = GEP %array, %idx3, 3                     ; <float>  // scalar element 6*4+3=27 instead of 6
	//
	// AFTER:
	// %array = load ...                                ; <<4 x float>*>
	// %indices = splitF()                              ; <<4 x i32>>
	// %idx0 = extractelement <4 x i32> %indices, i32 0 ; <i32>    // 0
	// %idx1 = extractelement <4 x i32> %indices, i32 1 ; <i32>    // 2
	// %idx2 = extractelement <4 x i32> %indices, i32 2 ; <i32>    // 4
	// %idx3 = extractelement <4 x i32> %indices, i32 3 ; <i32>    // 6
	// %simdWidth = i32 4                               ; <i32>
	// %idx0.d = udiv <i32> %idx0, %simdWidth            ; <i32>   // 0/4=0
	// %idx1.d = udiv <i32> %idx1, %simdWidth            ; <i32>   // 2/4=0
	// %idx2.d = udiv <i32> %idx2, %simdWidth            ; <i32>   // 4/4=1
	// %idx3.d = udiv <i32> %idx3, %simdWidth            ; <i32>   // 6/4=1
	// %idx0.r = urem <i32> %idx0, %simdWidth            ; <i32>   // 0%4=0
	// %idx1.r = urem <i32> %idx1, %simdWidth            ; <i32>   // 2%4=2
	// %idx2.r = urem <i32> %idx2, %simdWidth            ; <i32>   // 4%4=0
	// %idx3.r = urem <i32> %idx3, %simdWidth            ; <i32>   // 6%4=2
	// %gep0 = GEP %array, %idx0.d, %idx0.r              ; <float> // scalar element 0+0=0
	// %gep1 = GEP %array, %idx1.d, %idx1.r              ; <float> // scalar element 0+2=2
	// %gep2 = GEP %array, %idx2.d, %idx2.r              ; <float> // scalar element 1+0=4
	// %gep3 = GEP %array, %idx3.d, %idx3.r              ; <float> // scalar element 1+2=6
	void fixUniformPacketizedArrayAccesses(Function* f, Function* splitF, const unsigned simdWidth) {
		assert (f && splitF);

		// first walk the dependency-graph top-down to find GEPs that use (modified) indices
		std::vector<GetElementPtrInst*> gepVec;
		std::set<Instruction*> visited; // set is kept for all uses (must never walk the same path more than once)
		for (Function::use_iterator U=splitF->use_begin(), UE=splitF->use_end(); U!=UE; ++U) {
			if (!isa<CallInst>(U)) continue;
			CallInst* call = cast<CallInst>(U);
			if (call->getParent()->getParent() != f) continue; // ignore uses in other functions

			findDependentGEPs(call, gepVec, visited);
		}

		// gepVec now holds all GEP instructions that access result of splitF (=split index-vector)
		if (gepVec.empty()) return;

		std::vector<GetElementPtrInst*> deleteVec;
		for (std::vector<GetElementPtrInst*>::iterator it=gepVec.begin(), E=gepVec.end(); it!=E; ++it) {
			GetElementPtrInst* gep = *it;

			assert (gep->getNumIndices() == 2);

			// get first index
			Value* idx = *gep->idx_begin();
			// get instance index (= constant)
			//Value* instance = *(gep->idx_begin()+1);

			assert (isa<Constant>(*(gep->idx_begin()+1)));
			assert (idx->getType()->isIntegerTy());
			assert ((*(gep->idx_begin()+1))->getType()->isIntegerTy());

			Constant* simdWidthC = ConstantInt::get(idx->getType(), simdWidth, false);

			// generate div/rem instructions
			Instruction* insertBefore = isa<Instruction>(idx) ? cast<Instruction>(idx) : gep;
			Instruction* udiv = BinaryOperator::Create(Instruction::UDiv, idx, simdWidthC, "", insertBefore);
			Instruction* urem = BinaryOperator::Create(Instruction::URem, idx, simdWidthC, "", insertBefore);
			if (insertBefore == idx) cast<Instruction>(idx)->moveBefore(udiv);

			// generate new GEP
			std::vector<Value*> indices;
			indices.push_back(udiv);
			indices.push_back(urem);
			GetElementPtrInst* newGEP = GetElementPtrInst::Create(gep->getPointerOperand(), indices.begin(), indices.end(), "", gep);
			newGEP->takeName(gep);

			// replace uses of old GEP with new one
			gep->replaceAllUsesWith(newGEP);

			// mark old GEP to be deleted
			assert (gep->use_empty());
			deleteVec.push_back(gep);
		}
		// delete dead GEPs
		for (std::vector<GetElementPtrInst*>::iterator it=deleteVec.begin(), E=deleteVec.end(); it!=E; ++it) {
			(*it)->eraseFromParent();
		}
	}

}



// to be removed :)
namespace PacketizedOpenCLDriver {

	Function* getFunction(const std::string& name, Module* module) {
		assert (module);
		return module->getFunction(name);
	}


	Module* createModuleFromFile(const std::string & fileName) {

		std::string errorMessage;

		//create memory buffer for file

		MemoryBuffer* fileBuffer =
			MemoryBuffer::getFile(fileName.c_str(), &errorMessage);

		if (errorMessage != "") {
			errs() << "Error reading file '"
				<< fileName << "': " << errorMessage << "\n";
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

		Module* mod = ParseBitcodeFile(fileBuffer, getGlobalContext(), &errorMessage);

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
	unsigned getTypeSizeInBits(const TargetData* targetData, const Type* type) {
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


	void verifyModule(Module * module)
	{
		assert(module && "module must not be NULL");
		std::string moduleName = module->getModuleIdentifier().c_str();

		//check for references to other modules
		for(Module::iterator func = module->begin(), E = module->end();
				func != E;
				++func)
		{
			for (Function::iterator block = func->begin();
					block != func->end();
					++block) {
				for(BasicBlock::iterator inst = block->begin();
						inst != block->end();
						++inst) {

					//calls
					if (isa<CallInst>(inst)) {
						CallInst * callInst = cast<CallInst>(inst);
						Value * calledValue = callInst->getCalledValue();
						if (!calledValue) {
							//errs() << "ERROR: " << func->getNameStr() << " has call to NULL "; callInst->dump();
						}

						if (isa<Function>(calledValue)) {
							Function * callee = cast<Function>(calledValue);
							Module * calleeMod = callee->getParent();
							std::string calleeModName = calleeMod->getModuleIdentifier();
							std::string calleeName = callee->getNameStr();

							if (callee && (calleeMod != module)) {
								//errs() << "ERROR: In module '" << moduleName << "': Called function '" << calleeName << "' is in module '" << calleeModName << "'.\n";
								inst->print(outs());
								exit(-1);
							}
						}
					}

					//globals
					for(unsigned iOperand = 0; iOperand < inst->getNumOperands(); ++iOperand)
					{
						Value * operand = inst->getOperand(iOperand);

						if (isa<GlobalVariable>(operand)) {
							GlobalVariable * gv = cast<GlobalVariable>(operand);
							if (gv->getParent() != module) {
								std::string parentModuleName = gv->getParent()->getModuleIdentifier().c_str();
								//errs() << "ERROR: In module '" << moduleName << "': referencing global value '" << gv->getNameStr() << "' of module '" << parentModuleName << "'.\n";
								exit(-1);
							}
						}
					}
				}
			}
		}

		//use PacketizedOpenCLDrivers own verifier pass
		std::string errorMessage;
		if (verifyModule(*module, AbortProcessAction, &errorMessage)) {
			errs() << errorMessage;
		}
	}



	void inlineFunctionCalls(Function* f, TargetData* targetData=NULL) {
		//PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "  inlining function calls... "; );
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
						//PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "    function '" << callee->getNameStr() << "' has attribute 'no inline', ignored call!\n"; );
						continue;
					}

					const std::string calleeName = callee->getNameStr(); //possibly deleted by InlineFunction()

					InlineFunctionInfo IFI(NULL, targetData);
					blockChanged = InlineFunction(call, IFI);
					//PACKETIZED_OPENCL_DRIVER_DEBUG(
						//if (blockChanged) outs() << "  inlined call to function '" << calleeName << "'\n";
						//else errs() << "  inlining of call to function '" << calleeName << "' FAILED!\n";
					//);
					functionChanged |= blockChanged;
				}
			}
		}
		//PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "done.\n"; );
	}

#ifdef DO_NOT_OPTIMIZE
	void optimizeModule(Module* mod) {}
	void optimizeFunction(Function* f) {}
	void optimizeFunction(const std::string& functionName, Module* mod) {}
	void optimizeFunctionFast(Function* f) {}
#else
	/// Optimize - Perform link time optimizations. This will run the scalar
	/// optimizations, any loaded plugin-optimization modules, and then the
	/// inter-procedural optimizations if applicable.
	/// adopted from: llvm-2.5/lib/tools/llvm-ld/Optimize.cpp
	/// adopted from: llvm-2.7svn/include/llvm/Support/StandardPasses.h
	void optimizeModule(Module* mod) {
		assert (mod);

		// Instantiate the pass manager to organize the passes.
		PassManager Passes;

		//  // If we're verifying, start off with a verification pass.
		//  if (VerifyEach)
		//    Passes.add(createVerifierPass());

		// Add an appropriate TargetData instance for this module...
		Passes.add(new TargetData(mod));

		//TODO: activate
		//Passes.add(createStripSymbolsPass(false)); //false = do not only strip debug symbols

		//TODO: activate
		//createStandardLTOPasses(&Passes,
		//							true, // run internalize pass
		//							true, // use a function inlining pass
		//							false); // run verifyier after each pass

		//TODO: activate?
		//createStandardModulePasses(...);

		// Propagate constants at call sites into the functions they call.  This
		// opens opportunities for globalopt (and inlining) by substituting function
		// pointers passed as arguments to direct uses of functions.
		//    Passes.add(createIPSCCPPass());

		// Now that we internalized some globals, see if we can hack on them!
		Passes.add(createGlobalOptimizerPass());

		// Linking modules together can lead to duplicated global constants, only
		// keep one copy of each constant...
		Passes.add(createConstantMergePass());

		// Remove unused arguments from functions...
		Passes.add(createDeadArgEliminationPass());

		// Reduce the code after globalopt and ipsccp.  Both can open up significant
		// simplification opportunities, and both can propagate functions through
		// function pointers.  When this happens, we often have to resolve varargs
		// calls, etc, so let instcombine do this.
		Passes.add(createInstructionCombiningPass());

		//Passes.add(createFunctionInliningPass()); // Inline small functions //->assertion "invalid LLVM intrinsic name"

		//Passes.add(createPruneEHPass());            // Remove dead EH info   //->assertion "invalid LLVM intrinsic name"
		Passes.add(createGlobalOptimizerPass());    // Optimize globals again.
		Passes.add(createGlobalDCEPass());          // Remove dead functions

		// If we didn't decide to inline a function, check to see if we can
		// transform it to pass arguments by value instead of by reference.
		Passes.add(createArgumentPromotionPass());

		// The IPO passes may leave cruft around.  Clean up after them.
		Passes.add(createInstructionCombiningPass());
		Passes.add(createJumpThreadingPass());        // Thread jumps.
		Passes.add(createScalarReplAggregatesPass(2048)); // Break up allocas, override default threshold of maximum struct size of 128 bytes

		// Run a few AA driven optimizations here and now, to cleanup the code.
		Passes.add(createGlobalsModRefPass());      // IP alias analysis

		Passes.add(createLICMPass());               // Hoist loop invariants
		Passes.add(createGVNPass());                  // Remove redundancies
		Passes.add(createMemCpyOptPass());          // Remove dead memcpy's
		Passes.add(createDeadStoreEliminationPass()); // Nuke dead stores

		// Cleanup and simplify the code after the scalar optimizations.
		Passes.add(createInstructionCombiningPass());

		//Passes.add(createJumpThreadingPass());        // Thread jumps.
		Passes.add(createPromoteMemoryToRegisterPass()); // Cleanup jumpthread.

		// Delete basic blocks, which optimization passes may have killed...
		Passes.add(createCFGSimplificationPass());

		// Now that we have optimized the program, discard unreachable functions...
		Passes.add(createGlobalDCEPass());

		// The user's passes may leave cruft around. Clean up after them them but
		// only if we haven't got DisableOptimizations set
		//    Passes.add(createInstructionCombiningPass());
		//    Passes.add(createCFGSimplificationPass());
		//    Passes.add(createAggressiveDCEPass());
		//    Passes.add(createGlobalDCEPass());

		// Make sure everything is still good.
		PACKETIZED_OPENCL_DRIVER_DEBUG( Passes.add(createVerifierPass()); );

		// Run our queue of passes all at once now, efficiently.
		Passes.run(*mod);
	}

#if 1
	/// adopted from: llvm-2.5/tools/llvm-ld
	void optimizeFunction(Function* f) {
		assert (f);
		assert (f->getParent());
		Module* mod = f->getParent();
		TargetData* targetData = new TargetData(mod);

		//PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "optimizing function '" << f->getNameStr() << "'...\n"; );

#ifdef DEBUG_LLVM_PASSES
		DebugFlag = true;
#endif

		//PassManager Passes;
		FunctionPassManager Passes(mod);
		Passes.add(targetData);
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

		PACKETIZED_OPENCL_DRIVER_DEBUG( Passes.add(createVerifierPass()); );

		//PACKETIZED_OPENCL_DRIVER_DEBUG_VISIBLE( llvm::TimerGroup tg("llvmOptimization"); )
		//PACKETIZED_OPENCL_DRIVER_DEBUG_VISIBLE( llvm::Timer t("llvmOptimization", tg); )
		//PACKETIZED_OPENCL_DRIVER_DEBUG( t.startTimer(); );

		Passes.doInitialization();
		Passes.run(*f);
		Passes.doFinalization();

#ifdef DEBUG_LLVM_PASSES
		DebugFlag = false;
#endif

		//PACKETIZED_OPENCL_DRIVER_DEBUG( t.stopTimer(); );
		//PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "done.\ndone.\n"; );
		//PACKETIZED_OPENCL_DRIVER_DEBUG( tg.print(outs()); );
	}
#else
	/// adopted from: llvm-2.7/tools/opt
	// breaks Mandelbrot :p
	void optimizeFunction(Function* f) {
		assert (f);
		assert (f->getParent());
		Module* mod = f->getParent();
		TargetData* targetData = new TargetData(mod);

		//PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "optimizing function '" << f->getNameStr() << "'...\n"; );

#ifdef DEBUG_LLVM_PASSES
		DebugFlag = true;
#endif

		const unsigned OptimizationLevel = 3;
		const bool OptimizeSize = false;
		const bool UnitAtATime = true;
		const bool UnrollLoops = true;
		const bool SimplifyLibCalls = true;
		const bool HaveExceptions = true;
		Pass* InliningPass = createFunctionInliningPass(250);

		//PassManager Passes;
		FunctionPassManager Passes(mod);
		Passes.add(targetData);

		//
		// custom
		//
		Passes.add(createScalarReplAggregatesPass(2048)); // Break up allocas, override default threshold of maximum struct size of 128 bytes

		//
		// createStandardFunctionPasses
		//
		Passes.add(createCFGSimplificationPass());
		Passes.add(createPromoteMemoryToRegisterPass());
		Passes.add(createInstructionCombiningPass());

		//
		// createStandardModulePasses
		//

		//Passes.add(createGlobalOptimizerPass());
		//Passes.add(createIPSCCPPass());
		//Passes.add(createDeadArgEliminationPass());
		//Passes.add(createInstructionCombiningPass());  // Clean up after IPCP & DAE
		//Passes.add(createCFGSimplificationPass());     // Clean up after IPCP & DAE

		// Start of CallGraph SCC passes.
		//if (UnitAtATime && HaveExceptions)
		//Passes.add(createPruneEHPass());           // Remove dead EH info
		//if (InliningPass)
		//Passes.add(InliningPass);
		//if (UnitAtATime)
		//Passes.add(createFunctionAttrsPass());       // Set readonly/readnone attrs
		//if (OptimizationLevel > 2)
		//Passes.add(createArgumentPromotionPass());   // Scalarize uninlined fn args

		// Start of function pass.

		Passes.add(createScalarReplAggregatesPass(2048));  // Break up aggregate allocas
		if (SimplifyLibCalls)
			Passes.add(createSimplifyLibCallsPass());    // Library Call Optimizations
		Passes.add(createInstructionCombiningPass());  // Cleanup for scalarrepl.
		Passes.add(createJumpThreadingPass());         // Thread jumps.
		Passes.add(createCFGSimplificationPass());     // Merge & remove BBs
		Passes.add(createInstructionCombiningPass());  // Combine silly seq's

		Passes.add(createTailCallEliminationPass());   // Eliminate tail calls
		Passes.add(createCFGSimplificationPass());     // Merge & remove BBs
		Passes.add(createReassociatePass());           // Reassociate expressions
		Passes.add(createLoopRotatePass());            // Rotate Loop
		Passes.add(createLICMPass());                  // Hoist loop invariants
		Passes.add(createLoopUnswitchPass(OptimizeSize || OptimizationLevel < 3));
		Passes.add(createInstructionCombiningPass());
		Passes.add(createIndVarSimplifyPass());        // Canonicalize indvars
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
		Passes.add(createDeadStoreEliminationPass());  // Delete dead stores
		Passes.add(createAggressiveDCEPass());         // Delete dead instructions
		Passes.add(createCFGSimplificationPass());     // Merge & remove BBs

		if (UnitAtATime) {
			//Passes.add(createStripDeadPrototypesPass()); // Get rid of dead prototypes
			//Passes.add(createDeadTypeEliminationPass()); // Eliminate dead types

			// GlobalOpt already deletes dead functions and globals, at -O3 try a
			// late pass of GlobalDCE.  It is capable of deleting dead cycles.
			//if (OptimizationLevel > 2)
			//Passes.add(createGlobalDCEPass());         // Remove dead fns and globals.

			//if (OptimizationLevel > 1)
			//Passes.add(createConstantMergePass());       // Merge dup global constants
		}

		PACKETIZED_OPENCL_DRIVER_DEBUG( Passes.add(createVerifierPass()); );

		Passes.doInitialization();
		Passes.run(*f);
		Passes.doFinalization();

#ifdef DEBUG_LLVM_PASSES
		DebugFlag = false;
#endif
	}
#endif
	void optimizeFunction(const std::string& functionName, Module* mod) {
		assert (mod);
		Function* f = mod->getFunction(functionName);

		if (!f) {
			errs() << "failed.\nError while optimizing function in module '"
				<< mod->getModuleIdentifier() << "': function '"
				<< functionName << "' not found!\n";
			return;
		}
		optimizeFunction(f);
	}

	void optimizeFunctionFast(Function* f) {
		assert (f);
		assert (f->getParent());
		Module* mod = f->getParent();
		TargetData* targetData = new TargetData(mod);

		//PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "optimizing function '" << f->getNameStr() << "'...\n"; );

		FunctionPassManager Passes(mod);
		Passes.add(targetData);

		Passes.add(createInstructionCombiningPass());
		Passes.add(createReassociatePass());
		Passes.add(createGVNPass());
		Passes.add(createCFGSimplificationPass());
		Passes.add(createAggressiveDCEPass()); //fpm.add(createDeadCodeEliminationPass());

		PACKETIZED_OPENCL_DRIVER_DEBUG( Passes.add(createVerifierPass()); );

		//PACKETIZED_OPENCL_DRIVER_DEBUG_VISIBLE( llvm::TimerGroup tg("llvmOptimization"); )
		//PACKETIZED_OPENCL_DRIVER_DEBUG_VISIBLE( llvm::Timer t("llvmOptimization", tg); )
		//PACKETIZED_OPENCL_DRIVER_DEBUG( t.startTimer(); );

		Passes.doInitialization();
		Passes.run(*f);
		Passes.doFinalization();

		//PACKETIZED_OPENCL_DRIVER_DEBUG( t.stopTimer(); );
		//PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "done.\ndone.\n"; );
		//PACKETIZED_OPENCL_DRIVER_DEBUG( tg.print(outs()); );

		//std::string* errInfo = NULL;
		//mp.releaseModule(errInfo);
		//if (errInfo) errs() << "ERROR: could not release module.\n  " << *errInfo << "\n";
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
									  int numElements = arrayType->getNumElements();

									  elements.resize(numElements);
									  for(int index = 0; index < numElements; ++index) {
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
