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

#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/system_error.h> // error_code

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
		// represents uniform accesses to global id (e.g. of other dimensions than simd_dim)
		if (!mod->getFunction("get_global_id")) {
			const FunctionType* fTypeG0 = FunctionType::get(Type::getInt32Ty(context), params, false);
			Function::Create(fTypeG0, Function::ExternalLinkage, "get_global_id", mod);
		}
		// generate 'unsigned get_global_id_split(unsigned)'
		// is replaced by get_global_id_split_SIMD during packetization
		const FunctionType* fTypeG1 = FunctionType::get(Type::getInt32Ty(context), params, false);
		Function::Create(fTypeG1, Function::ExternalLinkage, "get_global_id_split", mod);
		// generate '__m128i get_global_id_split_SIMD(unsigned)'
		// returns a vector to force splitting during packetization
		const FunctionType* fTypeG2 = FunctionType::get(VectorType::get(Type::getInt32Ty(context), simdWidth), params, false);
		Function::Create(fTypeG2, Function::ExternalLinkage, "get_global_id_split_SIMD", mod);
		// generate 'unsigned get_global_id_SIMD(unsigned)'
		// does not return a vector because the simd value is loaded from the
		// begin-index of the four consecutive values
		const FunctionType* fTypeG3 = FunctionType::get(Type::getInt32Ty(context), params, false);
		Function::Create(fTypeG3, Function::ExternalLinkage, "get_global_id_SIMD", mod);


		// generate 'unsigned get_local_id(unsigned)' if not already there
		// represents uniform accesses to local id (e.g. of other dimensions than simd_dim)
		if (!mod->getFunction("get_local_id")) {
			const FunctionType* fTypeL0 = FunctionType::get(Type::getInt32Ty(context), params, false);
			Function::Create(fTypeL0, Function::ExternalLinkage, "get_local_id", mod);
		}
		// generate 'unsigned get_local_id_split(unsigned)'
		// is replaced by get_local_id_split_SIMD during packetization
		const FunctionType* fTypeL1 = FunctionType::get(Type::getInt32Ty(context), params, false);
		Function::Create(fTypeL1, Function::ExternalLinkage, "get_local_id_split", mod);
		// generate '__m128i get_local_id_split_SIMD(unsigned)'
		// returns a vector to force splitting during packetization
		const FunctionType* fTypeL2 = FunctionType::get(VectorType::get(Type::getInt32Ty(context), simdWidth), params, false);
		Function::Create(fTypeL2, Function::ExternalLinkage, "get_local_id_split_SIMD", mod);
		// generate 'unsigned get_local_id_SIMD(unsigned)'
		// does not return a vector because the simd value is loaded from the
		// begin-index of the four consecutive values
		const FunctionType* fTypeL3 = FunctionType::get(Type::getInt32Ty(context), params, false);
		Function::Create(fTypeL3, Function::ExternalLinkage, "get_local_id_SIMD", mod);


		// generate 'void barrier(unsigned, unsigned)' if not already there
		if (!mod->getFunction("barrier")) {
			params.push_back(Type::getInt32Ty(context)); // receives 2 ints
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

	Function* createExternalFunction(const std::string& name, const Type* returnType, std::vector<const Type*>& paramTypes, Module* mod) {
		assert (mod);
		const FunctionType* fType = FunctionType::get(returnType, paramTypes, false);
		return Function::Create(fType, Function::ExternalLinkage, name, mod);
	}

	Function* generatePacketPrototypeFromOpenCLKernel(const Function* kernel, const std::string& packetKernelName, Module* mod, const unsigned simdWidth) {
		assert (kernel && mod);
		assert (kernel->getParent() == mod);
		LLVMContext& context = mod->getContext();

		if (Function* fn = mod->getFunction(packetKernelName)) {
			PACKETIZED_OPENCL_DEBUG( errs() << "WARNING: target packet prototype already exists, automatic generation skipped!\n"; );
			fn->setName(packetKernelName);
			return fn;
		}

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
				case 1: // CL_GLOBAL, fall through
				case 3: // CL_LOCAL
				{
					// generate packet type if possible
					const Type* contType = pType->getContainedType(0);
					if (!contType->isIntegerTy() && !contType->isFloatTy()) {
						errs() << "ERROR: bad __global/__local parameter type found: " << *contType << " (can only handle int/float)!\n";
						return NULL;
					}
					TargetData* td = new TargetData(mod);
					if (td->getTypeSizeInBits(contType) > 32) {
						errs() << "ERROR: bad __global/__local parameter type found: " << *contType << " (can not handle data types with precision > 32bit)!\n";
						return NULL;
					}
					//const VectorType* vType = contType->isIntegerTy()
						//? VectorType::get(Type::getInt32Ty(context), simdWidth) // make i32 out of any integer type
						//: VectorType::get(contType, simdWidth);
					const VectorType* vType = VectorType::get(contType, simdWidth);
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

	// We currently do not have a possibility to ask the packetizer what instructions are
	// varying and which are uniform, so we have to rely on the domain-specific knowledge we have.
	// This will be far from optimal, but at least cover the most common cases.
	// The things we can test are the following:
	// - constants
	// - known arguments (localsize, globalsize, groupid, numgroups)
	// - possibly all non-pointer parameters (__local keyword) (TODO: really? if so, implement!)
	//
	// TODO: do we need separate checks for casts, phis, and selects as in functions below?
	// NOTE: The optimization has to be performed BEFORE packetization and callback replacement,
	//       so we still have calls instead of argument accesses!
	bool isNonVaryingMultiplicationTerm(Value* value, const unsigned simdDim, std::set<Value*>& visitedValues) {
		assert (value);

		if (visitedValues.find(value) != visitedValues.end()) return true; // TODO: is this safe?
		visitedValues.insert(value);

		if (isa<Constant>(value)) return true;
		if (isa<Argument>(value)) return false; // TODO: optimize for uniform arguments

		if (!isa<Instruction>(value)) return false; // TODO: actually, this function should never be called with basic blocks or so ^^

		if (CallInst* call = dyn_cast<CallInst>(value)) {
			if (call->getCalledFunction()->getNameStr() == "get_local_size") return true;
			if (call->getCalledFunction()->getNameStr() == "get_global_size") return true;
			if (call->getCalledFunction()->getNameStr() == "get_group_id") return true;
			if (call->getCalledFunction()->getNameStr() == "get_num_groups") return true;
			// calls to local/global id are okay as long as they don't ask for the same dimension (=the simd dimension)
			if (call->getCalledFunction()->getNameStr() == "get_local_id" ||
			    call->getCalledFunction()->getNameStr() == "get_global_id") {
				assert (isa<Constant>(call->getArgOperand(0)));
				return !constantEqualsInt(cast<Constant>(call->getArgOperand(0)), simdDim);
			}

			return false;
		}

		// no constant, no callback -> only uniform if all operands are uniform
		Instruction* inst = cast<Instruction>(value);
		for (Instruction::op_iterator O=inst->op_begin(), OE=inst->op_end(); O!=OE; ++O) {
			if (!isa<Value>(O)) continue;
			Value* opVal = cast<Value>(O);
			if (!isNonVaryingMultiplicationTerm(opVal, simdDim, visitedValues)) return false;
		}

		return true;
	}
	// returns true if any term of a multiplication-chain ( x * y * z * ... ) is
	// the local size argument
	// TODO: implement generic function 'isSafeConsecutiveMultiplicationTerm' that checks for multiple of SIMD width
	bool hasLocalSizeMultiplicationTerm(Value* value) {
		assert (value);
		// if the value is the "local_size"- or "global_size"-argument, the entire multiplication term is safe
		// NOTE: The optimization has to be performed BEFORE packetization and callback replacement,
		//       so we still have calls instead of argument accesses!

		if (CallInst* call = dyn_cast<CallInst>(value)) {
			if (call->getCalledFunction()->getNameStr() == "get_local_size") return true;
			if (call->getCalledFunction()->getNameStr() == "get_global_size") return true;
			return false;
		}


		// if this is a cast, recurse into the casted operand
		// TODO: other casts?
		//if (isa<CastInst>(value)) return hasLocalSizeMultiplicationTerm(cast<UnaryInstruction>(value)->getOperand(0));
		if (isa<BitCastInst>(value)) return hasLocalSizeMultiplicationTerm(cast<BitCastInst>(value)->getOperand(0));

		// if this is a phi, recurse into all incoming operands to ensure safety
		if (PHINode* phi = dyn_cast<PHINode>(value)) {
			for (unsigned i=0, e=phi->getNumIncomingValues(); i<e; ++i) {
				Value* incVal = phi->getIncomingValue(i);
				if (!hasLocalSizeMultiplicationTerm(incVal)) return false;
			}
			return true;
		}

		// same goes for selects (both incoming values have to be safe)
		if (SelectInst* select = dyn_cast<SelectInst>(value)) {
			return hasLocalSizeMultiplicationTerm(select->getTrueValue()) && hasLocalSizeMultiplicationTerm(select->getFalseValue());
		}

		// if it is none of the above and no binary operator, we are screwed anyway
		if (!isa<BinaryOperator>(value)) return false;

		BinaryOperator* binOp = cast<BinaryOperator>(value);
		switch (binOp->getOpcode()) {
			case Instruction::Mul: {
				// Check if the result of the multiplication is a multiple of the SIMD width.
				// Simpler version: check if any operand of the multiplication is the local size.
				Value* op0 = binOp->getOperand(0);
				Value* op1 = binOp->getOperand(1);
				return hasLocalSizeMultiplicationTerm(op0) || hasLocalSizeMultiplicationTerm(op1);
			}
			default: {
				return false;
			}
		}
	}
	// 'use' is required to know where we should place additional operations like the division
	// by the simd width in case of a constant
	bool isLinearModificationCalculation(Value* value, Instruction* use, const unsigned simdDim) {
		assert (value);

		PACKETIZED_OPENCL_DEBUG( outs() << "  testing consecutive modification calculation: " << *value << "\n"; );

		if (isa<Constant>(value)) {
			if (!constantIsDividableBySIMDWidth(cast<Constant>(value))) {
				PACKETIZED_OPENCL_DEBUG( outs() << "    value is a constant NOT dividable by SIMD width (has to be replicated)!\n"; );
				return false;
			}

			BinaryOperator::Create(Instruction::UDiv, value, ConstantInt::get(value->getType(), PACKETIZED_OPENCL_SIMD_WIDTH, false), "", use);
			PACKETIZED_OPENCL_DEBUG( outs() << "    value is a constant dividable by SIMD width (division inserted)!\n"; );
			return true;
		}

		// if the value is an argument, the indexing is consecutive if it is
		// the local or global id (and not the split id which holds 4 values)
		// NOTE: The optimization has to be performed BEFORE packetization and callback replacement,
		//       so we still have calls instead of argument accesses!
		//if (isa<Argument>(value)) {
		//	const std::string name = value->getNameStr();
		//	if (std::strstr(name.c_str(), "get_local_id") != 0) return true;
		//	if (std::strstr(name.c_str(), "get_global_id") != 0) return true;
		//	PACKETIZED_OPENCL_DEBUG( outs() << "    value is unsuited function parameter (neither local nor global ID)!\n"; );
		//	return false;
		//}

		// otherwise, this has to be an instruction
		assert (isa<Instruction>(value));

		// If we find a call, it is only a valid modification if local size is queried.
		// It is especially invalid if local/global id is used again.
		if (CallInst* call = dyn_cast<CallInst>(value)) {
			if (call->getCalledFunction()->getNameStr() == "get_local_size") return true;
			if (call->getCalledFunction()->getNameStr() == "get_global_size") return true;
			PACKETIZED_OPENCL_DEBUG( outs() << "    value is unsuited function parameter (only local size allowed)!\n"; );
			return false;
		}

		// if this is a cast, recurse into the casted operand
		// TODO: what about other casts?
		//if (isa<CastInst>(value)) return isConsecutiveIndex(cast<UnaryInstruction>(value)->getOperand(0));
		if (isa<BitCastInst>(value)) return isLinearModificationCalculation(cast<BitCastInst>(value)->getOperand(0), use, simdDim);

		// same for SExt/ZExt
		if (isa<SExtInst>(value)) return isLinearModificationCalculation(cast<SExtInst>(value)->getOperand(0), use, simdDim);
		if (isa<ZExtInst>(value)) return isLinearModificationCalculation(cast<ZExtInst>(value)->getOperand(0), use, simdDim);

		// if this is a phi, recurse into all incoming operands to ensure safety
		if (PHINode* phi = dyn_cast<PHINode>(value)) {
			for (unsigned i=0, e=phi->getNumIncomingValues(); i<e; ++i) {
				Value* incVal = phi->getIncomingValue(i);
				if (!isLinearModificationCalculation(incVal, phi->getIncomingBlock(i)->getTerminator(), simdDim)) return false;
			}
			return true;
		}

		// same goes for selects (both incoming values have to be safe)
		if (SelectInst* select = dyn_cast<SelectInst>(value)) {
			return isLinearModificationCalculation(select->getTrueValue(), select, simdDim) && isLinearModificationCalculation(select->getFalseValue(), select, simdDim);
		}

		// if it is none of the above and no binary operator, we cannot do anything
		if (!isa<BinaryOperator>(value)) return false;

		// This is the hard part:
		// Analyze the computation-tree below the value and check whether
		// it only consists of linear combinations where each term is a multiple
		// of the SIMD width.
		// We implement a simplified version that only checks if each term is a
		// multiple of the local size.

		BinaryOperator* binOp = cast<BinaryOperator>(value);
		switch (binOp->getOpcode()) {
			case Instruction::Sub: // fallthrough
			case Instruction::Add: {
				// recurse into subterms
				Value* op0 = binOp->getOperand(0);
				Value* op1 = binOp->getOperand(1);
				// check if the usage of local/global id is okay (only one use of one id (local OR global) in all terms)
				// example: arr[local id + 2] = 0+2 / 1+2 / 2+2 / 3+2 = 2 / 3 / 4 / 5 = consecutive
				// example: arr[local id + local id] = 0+0 / 1+1 / 2+2 / 3+3 = 0 / 2 / 4 / 6 = non-consecutive
				// TODO: this recomputes the same info for all terms on each level of recursion -> can be optimized
				//const bool idUsageOkay = termUsesID(op0) ^ termUsesID(op1);
				//return idUsageOkay && isConsecutiveModification(op0, binOp) && isConsecutiveModification(op1, binOp);
				return isLinearModificationCalculation(op0, binOp, simdDim) && isLinearModificationCalculation(op1, binOp, simdDim);
			}
			case Instruction::Mul: {
				// Check if the result of the multiplication is a multiple of the SIMD width.
				// Simplified version (currently implemented): check if any operand of the multiplication is the local size.

				// We also require all terms of the multiplication to be uniform.
				// example: arr[local id + local size * 2] = 0+16*2 / 1+16*2 / 2+16*2 / 3+16*2 = 32 / 33 / 34 / 35 = consecutive
				// example: arr[local id + local size * (2/3/4/5)] = 0+16*2 / 1+16*3 / 2+16*4 / 3+16*5 = 32 / 49 / 66 / 83 = non-consecutive

				std::set<Value*> visitedValues;
				return hasLocalSizeMultiplicationTerm(binOp) && isNonVaryingMultiplicationTerm(binOp, simdDim, visitedValues);
			}
			default: {
				PACKETIZED_OPENCL_DEBUG( outs() << "    found unknown operation in consective modification calculation!\n"; );
				return false;
			}
		}
	}

	// TODO: other safe operations? FPExt? Trunc? FPTrunc?
	// TODO: loops? safe paths?
	// TODO: phi, select
	// TODO: calls?
	bool indexIsOnlyUsedWithLinearModifications(Instruction* I, Instruction* parent, const unsigned simdDim, std::set<GetElementPtrInst*>& dependentGEPs, std::set<Instruction*>& safePathVec, std::set<Instruction*>& visited) {
		PACKETIZED_OPENCL_DEBUG( outs() << "\nindexIsOnlyUsedWithLinearModifications(" << *I << ")\n"; );

		// if this use is a GEP, this path is fine (don't go beyond GEPs)
		if (isa<GetElementPtrInst>(I)) {
			dependentGEPs.insert(cast<GetElementPtrInst>(I));
			return true;
		}

//		// if this is a loop, this path is fine
//		if (visited.find(I) != visited.end()) return true;
//		visited.insert(I);
//
//		// if this use is inside safeInstVec already, we can stop recursion
//		if (safePathVec.find(I) != safePathVec.end()) return true;

		// check for safe operations (rest of path still has to be checked)
		bool operandsAreSafe = false;

		switch (I->getOpcode()) {
			case Instruction::Add : //fallthrough
			case Instruction::Sub : {
				PACKETIZED_OPENCL_DEBUG( outs() << "  add/sub found!\n"; );
				// add/sub is okay if other operand is dividable by simd width (currently: local size if not constant) and uniform

				// get other operand
				Value* op = I->getOperand(0) == parent ? I->getOperand(1) : I->getOperand(0);

				// If operand is constant, we have to divide it by the SIMD width.
				// If it is not dividable, it must not be optimized.
				// example: arr[local id + 8] = 0+8 / 1+8 / 2+8 / 3+8 = 8 / 9 / 10 / 11 = 'SIMD position' 2
				// optimized: arr[local id SIMD + 8] = 0+8 = 'SIMD position' 8 = wrong values
				// optimized&divided: arr[local id SIMD + 8/4] = 0+2 = 'SIMD position' 2 = correct values
				if (isa<Constant>(op)) {
					PACKETIZED_OPENCL_DEBUG( outs() << "    has constant operand!\n"; );
					if (!constantIsDividableBySIMDWidth(cast<Constant>(op))) break;

					BinaryOperator* divOp = BinaryOperator::Create(Instruction::UDiv, op, ConstantInt::get(op->getType(), PACKETIZED_OPENCL_SIMD_WIDTH, false), "", I);
					I->replaceUsesOfWith(op, divOp);
					operandsAreSafe = true;
					break;
				}

				// Otherwise, check if other operand is dividable by simd width (currently: local size if not constant)
				// This function tests an entire add/sub-tree above the operand
				// whether each term is dividable.
				operandsAreSafe = isLinearModificationCalculation(op, I, simdDim);
				break;
			}

			case Instruction::SExt    : // SExt is okay, fallthrough
			case Instruction::ZExt    : // ZExt is okay, fallthrough
			case Instruction::FPToUI  : // FPToUI is okay, fallthrough
			case Instruction::FPToSI  : // FPToSI is okay, fallthrough
			case Instruction::UIToFP  : // UIToFP is okay, fallthrough
			case Instruction::SIToFP  : // SIToFP is okay, fallthrough
			case Instruction::FPExt   : // FPExt is okay, fallthrough
			case Instruction::BitCast : {
				PACKETIZED_OPENCL_DEBUG( outs() << "  cast found!\n"; );
				assert (isa<Instruction>(I->getOperand(0)));
				operandsAreSafe = true; //indexIsOnlyUsedWithLinearModifications(cast<Instruction>(I->getOperand(0)), I, simdDim, safePathVec, visited);
				break;
			}

			//case Instruction::Mul : useIsBad = false; break; // Mul is NOT okay!
			//case Instruction::ICmp    : useIsBad = false; break; // ICmp is NOT okay! (can jump to different targets)
			//case Instruction::FCmp    : useIsBad = false; break; // FCmp is NOT okay! (can jump to different targets)
			//case Instruction::Ret     : useIsBad = false; break; // Ret is NOT okay! (can return different results)
			//case Instruction::Br      : useIsBad = false; break; // Br is NOT okay! (can jump to different targets)
			//case Instruction::Switch  : useIsBad = false; break; // Switch NOT okay! (can jump to different targets)

			// These two cases are a little more complicated:
			// If the local/global id is used in a phi/select, all other operands
			// of the phi/select are allowed to use local/global id again (in
			// contrast to additions).
			// TODO: implement additional function to be called here instead of
			// indexIsOnlyUsedWithLinearModifications that allows usage of ids.
			// alternative: add a flag that enables this feature
			case Instruction::PHI     : {
				PACKETIZED_OPENCL_DEBUG( outs() << "  phi found!\n"; );
				// recurse into all other incoming operands to ensure safety
				PHINode* phi = cast<PHINode>(I);
				bool phiIsSafe = true;
				for (unsigned i=0, e=phi->getNumIncomingValues(); i<e; ++i) {
					Value* incVal = phi->getIncomingValue(i);
					if (incVal == I) continue; // skip current value
					if (isa<Constant>(incVal)) {
						if (constantIsDividableBySIMDWidth(cast<Constant>(incVal))) continue;
						else {
							phiIsSafe = false;
							break;
						}
					} else if (isa<Argument>(incVal)) {
						phiIsSafe = false;
						break;
					} else {
						assert (isa<Instruction>(incVal));
						if (!indexIsOnlyUsedWithLinearModifications(cast<Instruction>(incVal), I, simdDim, dependentGEPs, safePathVec, visited)) {
							phiIsSafe = false;
							break;
						}
					}
				}
				operandsAreSafe = phiIsSafe;
				break;
			}
			case Instruction::Select  : {
				PACKETIZED_OPENCL_DEBUG( outs() << "  select found!\n"; );
				// same goes for selects (second incoming value also has to be safe)
				SelectInst* select = cast<SelectInst>(I);
				Value* otherVal = select->getTrueValue() == I ? select->getFalseValue() : select->getTrueValue();
				if (isa<Constant>(otherVal)) {
					if (constantIsDividableBySIMDWidth(cast<Constant>(otherVal))) operandsAreSafe = true;
					break;
				} else if (isa<Argument>(otherVal)) {
					break; // not safe
				} else {
					assert (isa<Instruction>(otherVal));
					operandsAreSafe = indexIsOnlyUsedWithLinearModifications(cast<Instruction>(otherVal), I, simdDim, dependentGEPs, safePathVec, visited);
					break;
				}
			}
		}

		PACKETIZED_OPENCL_DEBUG(
			if (!operandsAreSafe && I->getNumOperands() == 1) {
				outs() << "\n\n  instruction only has one operand -> should be safe?!\n";
				outs() << "    " << *I << "\n";
				exit(0);
			}
		);

		PACKETIZED_OPENCL_DEBUG(
			if (operandsAreSafe) outs() << "  operands safe for instruction: " << *I << "\n";
			else outs() << "  operands NOT safe for instruction: " << *I << "\n";
		);

		if (!operandsAreSafe) return false;

		// now recurse further into uses
		// (in case of add/sub: make sure all further terms are also dividable)

		for (Instruction::use_iterator U=I->use_begin(), UE=I->use_end(); U!=UE; ++U) {
			if (Instruction* useI = dyn_cast<Instruction>(*U))
				if (!indexIsOnlyUsedWithLinearModifications(useI, I, simdDim, dependentGEPs, safePathVec, visited)) {
					PACKETIZED_OPENCL_DEBUG( outs() << "  uses NOT safe for instruction: " << *I << "\n"; );
					return false;
				}
		}

		PACKETIZED_OPENCL_DEBUG( outs() << "  INSTRUCTION IS SAFE: " << *I << "\n"; );
		return true;
	}


	// NOTE: We assume that the local sizes of dimensions other than simdDim are also dividable by simd width
	bool indexIsMultipleOfOriginalLocalSize(Value* index, Instruction* use, const unsigned simdDim) {
		assert (index);
		PACKETIZED_OPENCL_DEBUG( outs() << "\nindexIsMultipleOfOriginalLocalSize(" << *index << ")\n"; );

		if (isa<Constant>(index)) return false;
		if (isa<Argument>(index)) return false;

		assert (isa<Instruction>(index));
		Instruction* indexInst = cast<Instruction>(index);

		bool instructionIsSafe = false;

		switch (indexInst->getOpcode()) {
			case Instruction::Mul : // fallthrough
			case Instruction::Add : // fallthrough
			case Instruction::Sub : {
				Value* op0 = indexInst->getOperand(0);
				Value* op1 = indexInst->getOperand(1);
				instructionIsSafe = indexIsMultipleOfOriginalLocalSize(op0, indexInst, simdDim) && indexIsMultipleOfOriginalLocalSize(op1, indexInst, simdDim);
				break;
			}
			case Instruction::Call : {
				CallInst* call = cast<CallInst>(indexInst);
				instructionIsSafe = call->getCalledFunction()->getNameStr() == "get_local_size" || 
						call->getCalledFunction()->getNameStr() == "get_global_size";

				// make sure each term that uses local or global size is divided by the SIMD width
				// NOTE: global size does not make it safe, but also has to be fixed
				// NOTE: We must not divide the result if the call is to a dimension other than simdDim
				assert (isa<Constant>(call->getArgOperand(0)));
				const bool callsSimdDim = constantEqualsInt(cast<Constant>(call->getArgOperand(0)), simdDim);
				if ((instructionIsSafe || call->getCalledFunction()->getNameStr() == "get_global_size") && callsSimdDim) {
					BinaryOperator* divIndex = BinaryOperator::Create(Instruction::UDiv, call, ConstantInt::get(call->getType(), PACKETIZED_OPENCL_SIMD_WIDTH, false), "", use);
					use->replaceUsesOfWith(call, divIndex);
				}
				break;
			}

			case Instruction::SExt    :
			case Instruction::FPToUI  :
			case Instruction::FPToSI  :
			case Instruction::UIToFP  :
			case Instruction::SIToFP  :
			case Instruction::FPExt   :
			case Instruction::BitCast : {
				instructionIsSafe = true;
				break;
			}

			case Instruction::PHI : {
				PHINode* phi = cast<PHINode>(indexInst);
				bool phiIsSafe = true;
				for (unsigned i=0, e=phi->getNumIncomingValues(); i<e; ++i) {
					Value* incVal = phi->getIncomingValue(i);
					if (!indexIsMultipleOfOriginalLocalSize(incVal, phi, simdDim)) {
						phiIsSafe = false;
						break;
					}
				}
				instructionIsSafe = phiIsSafe;
				break;
			}
			case Instruction::Select : {
				SelectInst* select = cast<SelectInst>(indexInst);
				instructionIsSafe = indexIsMultipleOfOriginalLocalSize(select->getTrueValue(), select, simdDim) && indexIsMultipleOfOriginalLocalSize(select->getFalseValue(), select, simdDim);
				break;
			}
		}

		if (!instructionIsSafe) return false;

		for (Instruction::op_iterator O=indexInst->op_begin(), OE=indexInst->op_end(); O!=OE; ++O) {
			assert (isa<Value>(O));
			if (indexIsMultipleOfOriginalLocalSize(cast<Value>(O), indexInst, simdDim)) return true;
		}
		return false;
	}
	void fixLocalSizeSIMDUsageInIndexCalculation(GetElementPtrInst* gep, const unsigned simdDim) {
		assert (gep);
		PACKETIZED_OPENCL_DEBUG( outs() << "\nfixLocalSizeSIMDUsageInIndexCalculation(" << *gep << ")\n"; );

		assert (gep->getNumIndices() == 1);

		Value* index = cast<Value>(gep->idx_begin());
		assert (index->getType()->isIntegerTy());

		indexIsMultipleOfOriginalLocalSize(index, gep, simdDim);
	}

	// Replace calls to oldF with index 'simdDim' by splitF in function f
	// wherever the result of the call is not used directly in a load or store (via GEP),
	// e.g. as part of some index arithmetic accessing array[i*4+1]
	// Replace contiguous accesses to oldF with index 'simdDim' by simdFn (which
	// is only a placeholder (with same scalar type as oldFn but different name)
	// for the instruction inserted during packetization).
	// Leave uses with indices other than 'simdDim' untouched.
	// Effectively, this is an optimization of array accesses that load or store
	// contiguous indices.
	void setupIndexUsages(Function* parent, Function* oldFn, Function* simdFn, Function* splitFn, const unsigned simdDim) {
		assert (parent && oldFn && simdFn && splitFn);
		assert (oldFn->getReturnType()->isIntegerTy());
		assert (splitFn->getReturnType()->isIntegerTy());
		// TODO: check if signature matches (we are replacing oldF by splitFn...)

		PACKETIZED_OPENCL_DEBUG( outs() << "\nsetupIndexUsages(" << oldFn->getNameStr() << ")\n"; );

		std::vector<CallInst*> deleteVec;
		std::set<Instruction*> safePathVec; // marks instructions whose use-subtrees are entirely safe

		for (Function::use_iterator U=oldFn->use_begin(), UE=oldFn->use_end(); U!=UE; ++U) {
			if (!isa<CallInst>(*U)) continue;
			CallInst* call = cast<CallInst>(*U);
			if (call->getParent()->getParent() != parent) continue; // ignore uses in other functions

			// calls with other indices than 'simdDim' are not touched (they are
			// uniform for the N threads of 'simdDim'.
			// They will be replicated by packetizer if required.
			assert (isa<ConstantInt>(call->getArgOperand(0)));
			if (!constantEqualsInt(cast<Constant>(call->getArgOperand(0)), simdDim)) continue;

			// generate calls (we might still need the old one) to splitFn and simdFn
			// TODO: where to place the new calls? maybe directly in front of use?
			//       for now, place them at the same spot as the old call
			//CallInst* splitCall = CallInst::Create(splitFn, call->op_begin()+1, call->op_end(), call->getName()+"_split", call);
			std::vector<Value*> args;
			for (CallInst::op_iterator OP=call->op_begin(), OPE=call->op_end()-1; OP!=OPE; ++OP) {
				args.push_back(*OP);
			}
			CallInst* splitCall = CallInst::Create(splitFn, args.begin(), args.end(), call->getCalledFunction()->getName()+"_split", call);
			splitCall->setTailCall(true);

			CallInst* simdCall = CallInst::Create(simdFn, args.begin(), args.end(), call->getCalledFunction()->getName()+"_simd_tmp", call);
			simdCall->setTailCall(true);


			// iterate over uses of the original call
			for (CallInst::use_iterator U2=call->use_begin(), UE2=call->use_end(); U2!=UE2; ) {
				assert (isa<Instruction>(*U2));
				Instruction* useI = cast<Instruction>(*U2++);

#ifndef PACKETIZED_OPENCL_SPLIT_EVERYTHING
				// attempt to analyze path
				// TODO: entirely uniform paths are okay as well
				std::set<Instruction*> visited;
				std::set<GetElementPtrInst*> dependentGEPs;
				if (indexIsOnlyUsedWithLinearModifications(useI, call, simdDim, dependentGEPs, safePathVec, visited)) {
					// If any index calculation uses local_size, we have to divide the corresponding
					// terms by the SIMD width because local_size is the unmodified value.
					// This is safer than dividing local_size by the SIMD width everywhere
					// because kernels might rely on the actual unmodified number of threads
					// in a group.
					for (std::set<GetElementPtrInst*>::iterator it=dependentGEPs.begin(), E=dependentGEPs.end(); it!=E; ++it) {
						fixLocalSizeSIMDUsageInIndexCalculation(*it, simdDim);
					}

					useI->replaceUsesOfWith(call, simdCall);
					PACKETIZED_OPENCL_DEBUG( outs() << "  OPTIMIZED USE: " << *useI << "\n"; );
					continue;
				} else {
					PACKETIZED_OPENCL_DEBUG( outs() << "  COULD NOT OPTIMIZE USE OF CALL: " << *useI << "\n"; );
				}
#endif

				// otherwise, we have a use that (conservatively) has to be split
				// ( = replaced with call to splitFn)
				// -> replace call by new call in this use
				useI->replaceUsesOfWith(call, splitCall);
			}

			if (call->use_empty()) deleteVec.push_back(call); // still iterating over calls, postpone deletion
			if (splitCall->use_empty()) splitCall->eraseFromParent(); // can erase directly
			if (simdCall->use_empty()) simdCall->eraseFromParent(); // can erase directly
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
			if (Instruction* useI = dyn_cast<Instruction>(*U))
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
	void fixUniformPacketizedArrayAccesses(Function* f, Function* splitF, const unsigned simdWidth, std::set<GetElementPtrInst*>& fixedGEPs) {
		assert (f && splitF);

		// first walk the dependency-graph top-down to find GEPs that use (modified) indices
		std::vector<GetElementPtrInst*> gepVec;
		std::set<Instruction*> visited; // set is kept for all uses (must never walk the same path more than once)
		for (Function::use_iterator U=splitF->use_begin(), UE=splitF->use_end(); U!=UE; ++U) {
			if (!isa<CallInst>(*U)) continue;
			CallInst* call = cast<CallInst>(*U);
			if (call->getParent()->getParent() != f) continue; // ignore uses in other functions

			findDependentGEPs(call, gepVec, visited);
		}

		// gepVec now holds all GEP instructions that access result of splitF (=split index-vector)
		if (gepVec.empty()) return;

		std::vector<GetElementPtrInst*> deleteVec;
		for (std::vector<GetElementPtrInst*>::iterator it=gepVec.begin(), E=gepVec.end(); it!=E; ++it) {
			GetElementPtrInst* gep = *it;
			if (fixedGEPs.find(gep) != fixedGEPs.end()) continue;

			assert (gep->getNumIndices() == 2);

			// get first index
			Value* idx = *gep->idx_begin();
			// get instance index (= constant)
			//Value* instance = *(gep->idx_begin()+1);
			
			//outs () << "gep: " << *gep << "\n";
			//outs () << "gep-idx_begin+1: " << **(gep->idx_begin()+1) << "\n";

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
			fixedGEPs.insert(newGEP);
		}
		// delete dead GEPs
		for (std::vector<GetElementPtrInst*>::iterator it=deleteVec.begin(), E=deleteVec.end(); it!=E; ++it) {
			(*it)->eraseFromParent();
		}
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
