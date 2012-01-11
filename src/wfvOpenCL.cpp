/**
 * @file   packetizedOpenCL.cpp
 * @date   14.04.2010
 * @author Ralf Karrenberg
 *
 * This file is distributed under the University of Illinois Open Source
 * License. See the COPYING file in the root directory for details.
 *
 * Copyright (C) 2010, 2011 Saarland University
 *
 */

#include <cstddef>

#include <sstream>

#include <xmmintrin.h>

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

#include "passes/continuationGenerator.h"
#include "consts.h"
#include "debug.h"
#include "llvmTools.hpp"

//----------------------------------------------------------------------------//
// Tools
//----------------------------------------------------------------------------//

#include "cast.h"

template<typename T> void* void_cast(T* p) {
	return ptr_cast<void*>(p);
}

// helper: extract ith element of a __m128
inline float& get(const __m128& v, const unsigned idx) {
    return ((float*)&v)[idx];
}
inline unsigned& get(const __m128i& v, const unsigned idx) {
    return ((unsigned*)&v)[idx];
}
inline void printV(const __m128& v) {
	outs() << get(v, 0) << " " << get(v, 1) << " " << get(v, 2) << " " << get(v, 3);
}
inline void printV(const __m128i& v) {
	outs() << get(v, 0) << " " << get(v, 1) << " " << get(v, 2) << " " << get(v, 3);
}

//----------------------------------------------------------------------------//


#ifdef __cplusplus
extern "C" {
#endif

///////////////////////////////////////////////////////////////////////////
//                     OpenCL Code Generation                            //
///////////////////////////////////////////////////////////////////////////
namespace WFVOpenCL {

#ifndef WFVOPENCL_NO_WFV
		bool packetizeKernelFunction(
			const std::string& kernelName,
			const std::string& targetKernelName,
			llvm::Module* mod,
			const cl_uint packetizationSize,
			const cl_uint simdDim,
			const bool use_sse41,
			const bool use_avx,
			const bool verbose)
	{
		if (!WFVOpenCL::getFunction(kernelName, mod)) {
			errs() << "ERROR: source function '" << kernelName
					<< "' not found in module!\n";
			return false;
		}
		if (!WFVOpenCL::getFunction(targetKernelName, mod)) {
			errs() << "ERROR: target function '" << targetKernelName
					<< "' not found in module!\n";
			return false;
		}

		Packetizer::Packetizer packetizer(*mod, packetizationSize, packetizationSize, use_sse41, use_avx, verbose);

		packetizer.addFunction(kernelName, targetKernelName);

		WFVOpenCL::addNativeFunctions(WFVOpenCL::getFunction(kernelName, mod), simdDim, packetizer);

		packetizer.run();

		// check if the function has been vectorized
		if (mod->getFunction(targetKernelName)->getBasicBlockList().empty()) {
			return false;
		}

		return true;
	}
#endif

	//------------------------------------------------------------------------//
	// LLVM tools
	//------------------------------------------------------------------------//


	// insert print statement that prints 'value' preceeded by 'DEBUG: `message`'
	// example what can be generated:
	// declare i32 @printf(i8* noalias nocapture, ...) nounwind
	// @.str1 = private constant [19 x i8] c"DEBUG: indexA: %d\0A\00", align 1 ; <[19 x i8]*> [#uses=1]
	// %printf1 = tail call i32 (i8*, ...)* @printf(i8* getelementptr inbounds ([19 x i8]* @.str1, i64 0, i64 0), i32 %call) ; <i32> [#uses=0]
	CallInst* insertPrintf(const std::string& message, Value* value, const bool endLine, Instruction* insertBefore) {
		assert (value && insertBefore);
		assert (insertBefore->getParent());
		Function* f = insertBefore->getParent()->getParent();
		assert (f);
		Module* mod = f->getParent();
		assert (mod);

		Function* func_printf =  mod->getFunction("printf");

		if (!func_printf) {
			PointerType* PointerTy_6 = PointerType::get(IntegerType::get(mod->getContext(), 8), 0);

			std::vector<const Type*>FuncTy_10_args;
			FuncTy_10_args.push_back(PointerTy_6);
			FunctionType* FuncTy_10 = FunctionType::get(
					/*Result=*/IntegerType::get(mod->getContext(), 32),
					/*Params=*/FuncTy_10_args,
					/*isVarArg=*/true);

			func_printf = Function::Create(
					/*Type=*/FuncTy_10,
					/*Linkage=*/GlobalValue::ExternalLinkage,
					/*Name=*/"printf", mod); // (external, no body)
			func_printf->setCallingConv(CallingConv::C);
			AttrListPtr func_printf_PAL;
			{
				SmallVector<AttributeWithIndex, 4> Attrs;
				AttributeWithIndex PAWI;
				PAWI.Index = 1U; PAWI.Attrs = 0  | Attribute::NoAlias | Attribute::NoCapture;
				Attrs.push_back(PAWI);
				PAWI.Index = 4294967295U; PAWI.Attrs = 0  | Attribute::NoUnwind;
				Attrs.push_back(PAWI);
				func_printf_PAL = AttrListPtr::get(Attrs.begin(), Attrs.end());

			}
			func_printf->setAttributes(func_printf_PAL);
		}

		const bool valueIsVector = value->getType()->isVectorTy();
		ArrayType* ArrayTy_0 = ArrayType::get(IntegerType::get(mod->getContext(), 8), message.length()+9+(endLine ? 2 : 1)+(valueIsVector ? 9 : 0));
		GlobalVariable* gvar_array__str = new GlobalVariable(/*Module=*/*mod,
				/*Type=*/ArrayTy_0,
				/*isConstant=*/true,
				/*Linkage=*/GlobalValue::PrivateLinkage,
				/*Initializer=*/0, // has initializer, specified below
				/*Name=*/".str");
		gvar_array__str->setAlignment(1);

		// Constant Definitions
		std::string str = "";
		switch (value->getType()->getTypeID()) {
			case Type::IntegerTyID : str = "%d"; break;
			case Type::FloatTyID   : str = "%f"; break;
			case Type::PointerTyID : str = "%x"; break;
			case Type::VectorTyID  : {
				switch (value->getType()->getContainedType(0)->getTypeID()) {
					case Type::IntegerTyID : str = "%d %d %d %d"; break;
					case Type::FloatTyID   : str = "%f %f %f %f"; break;
					default                : str = "%x %x %x %x"; break;
				}
				break;
			}
			default                : str = "%x"; break;
		}
		std::stringstream sstr;
		sstr << "DEBUG: " << message << str << (endLine ? "\x0A" : "");
		Constant* const_array_11 = ConstantArray::get(mod->getContext(), sstr.str(), true);
		std::vector<Constant*> const_ptr_17_indices;
		ConstantInt* const_int64_18 = ConstantInt::get(mod->getContext(), APInt(64, StringRef("0"), 10));
		const_ptr_17_indices.push_back(const_int64_18);
		const_ptr_17_indices.push_back(const_int64_18);
		Constant* const_ptr_17 = ConstantExpr::getGetElementPtr(gvar_array__str, &const_ptr_17_indices[0], const_ptr_17_indices.size());

		// Global Variable Definitions
		gvar_array__str->setInitializer(const_array_11);


		std::vector<Value*> int32_51_params;
		int32_51_params.push_back(const_ptr_17);
		if (valueIsVector) {
			const unsigned size = cast<VectorType>(value->getType())->getNumElements();
			for (unsigned i=0; i<size; ++i) {
				ExtractElementInst* ei = ExtractElementInst::Create(value, ConstantInt::get(mod->getContext(), APInt(32, i)), "printfElem", insertBefore);
				int32_51_params.push_back(ei);
			}
		} else {
			int32_51_params.push_back(value);
		}
		CallInst* int32_51 = CallInst::Create(func_printf, int32_51_params.begin(), int32_51_params.end(), "", insertBefore);
		return int32_51;
	}


	// We assume that A dominates B, so all paths from A have to lead to B.
	bool barrierBetweenInstructions(BasicBlock* block, Instruction* A, Instruction* B, std::set<BasicBlock*>& visitedBlocks) {
		assert (block && A && B);

		if (visitedBlocks.find(block) != visitedBlocks.end()) return false;
		visitedBlocks.insert(block);

		if (block == A->getParent()) {

			bool foundI = false;
			for (BasicBlock::iterator I=block->begin(), IE=block->end(); I!=IE; ++I) {
				if (!foundI && A != I) continue; // make sure we ignore instructions in front of A
				foundI = true;

				if (B == I) return false;

				if (!isa<CallInst>(I)) continue;
				CallInst* call = cast<CallInst>(I);
				Function* callee = call->getCalledFunction();
				if (callee->getName().equals(WFVOPENCL_FUNCTION_NAME_BARRIER)) return true;
			}

		} else if (block == B->getParent()) {

			for (BasicBlock::iterator I=block->begin(), IE=block->end(); I!=IE; ++I) {
				if (B == I) return false;

				if (!isa<CallInst>(I)) continue;
				CallInst* call = cast<CallInst>(I);
				Function* callee = call->getCalledFunction();
				if (callee->getName().equals(WFVOPENCL_FUNCTION_NAME_BARRIER)) return true;
			}
			assert (false && "SHOULD NEVER HAPPEN!");
			return false;

		} else {

			// This is a block between A and B -> test instructions
			for (BasicBlock::iterator I=block->begin(), IE=block->end(); I!=IE; ++I) {
				if (!isa<CallInst>(I)) continue;
				CallInst* call = cast<CallInst>(I);
				Function* callee = call->getCalledFunction();
				if (callee->getName().equals(WFVOPENCL_FUNCTION_NAME_BARRIER)) return true;
			}

		}

		// Neither B nor barrier found -> recurse into successor blocks.

		typedef GraphTraits<BasicBlock*> BlockTraits;
		for (BlockTraits::ChildIteratorType PI = BlockTraits::child_begin(block),
				PE = BlockTraits::child_end(block); PI != PE; ++PI)
		{
			BasicBlock* succBB = *PI;
			if (barrierBetweenInstructions(succBB, A, B, visitedBlocks)) return true;
		}

		return false;
	}


	// Special case for SExt/ZExt: step through and iterate over their uses again.
	void findStepThroughCallbackUses(Instruction* inst, CallInst* call, std::vector<CallInst*>& calls, std::vector<Instruction*>& uses, std::vector<Instruction*>& targets) {
		assert (inst);
		for (Instruction::use_iterator U=inst->use_begin(), UE=inst->use_end(); U!=UE; ++U) {
			assert (isa<Instruction>(*U));
			Instruction* useI = cast<Instruction>(*U);

			if (isa<SExtInst>(useI) || isa<ZExtInst>(useI)) {
				findStepThroughCallbackUses(useI, call, calls, uses, targets);
			}

			std::set<BasicBlock*> visitedBlocks;
			if (!barrierBetweenInstructions(inst->getParent(), inst, useI, visitedBlocks)) {
				//outs() << "  no barrier between insts:\n";
				//outs() << "    inst: " << *inst << "\n";
				//outs() << "    useI: " << *useI << "\n";
				continue;
			}

			calls.push_back(call);
			targets.push_back(inst);
			uses.push_back(useI);
		}
	}

	// Replace all uses of a callback that do not follow the call directly by
	// an additional call.
	// This reduces the amount of live values we have to store when generating
	// continuations.
	void findCallbackUses(CallInst* call, std::vector<CallInst*>& calls, std::vector<Instruction*>& uses, std::vector<Instruction*>& targets) {
		assert (call);
		for (CallInst::use_iterator U=call->use_begin(), UE=call->use_end(); U!=UE; ++U) {
			assert (isa<Instruction>(*U));
			Instruction* useI = cast<Instruction>(*U);

			if (isa<SExtInst>(useI) || isa<ZExtInst>(useI)) {
				findStepThroughCallbackUses(useI, call, calls, uses, targets);
			}

			std::set<BasicBlock*> visitedBlocks;
			if (!barrierBetweenInstructions(call->getParent(), call, useI, visitedBlocks)) {
				//outs() << "  no barrier between insts:\n";
				//outs() << "    inst: " << *inst << "\n";
				//outs() << "    useI: " << *useI << "\n";
				continue;
			}

			calls.push_back(call);
			targets.push_back(call);
			uses.push_back(useI);
		}
	}

	void replaceCallbackUsesByNewCallsInFunction(Function* callback, Function* parentF) {
		if (!callback) return;
		assert (parentF);

		std::vector<CallInst*> calls; // actually, calls and sext/zext
		std::vector<Instruction*> uses;
		std::vector<Instruction*> targets;
		for (Function::use_iterator U=callback->use_begin(), UE=callback->use_end(); U!=UE; ++U) {
			assert (isa<CallInst>(*U));
			CallInst* call = cast<CallInst>(*U);
			if (call->getParent()->getParent() != parentF) continue;

			findCallbackUses(call, calls, uses, targets);
		}

		for (unsigned i=0; i<calls.size(); ++i) {
			WFVOPENCL_DEBUG( outs() << "replacing callback-use by new call in instruction: " << *uses[i] << "\n"; );
			if (!isa<CallInst>(targets[i])) {
				Instruction* newCall = calls[i]->clone();
				newCall->insertBefore(uses[i]);
				Instruction* newTarget = targets[i]->clone();
				newTarget->insertAfter(newCall);
				newTarget->replaceUsesOfWith(calls[i], newCall);
				uses[i]->replaceUsesOfWith(targets[i], newTarget);
			} else {
				Instruction* newCall = calls[i]->clone();
				newCall->insertBefore(uses[i]);
				uses[i]->replaceUsesOfWith(targets[i], newCall);
			}
		}
	}


	void replaceCallbacksByArgAccess(Function* f, Value* arg, Function* source) {
		if (!f) return;
		assert (arg && source);

		WFVOPENCL_DEBUG( outs() << "\nreplaceCallbacksByArgAccess(" << f->getNameStr() << ", " << *arg << ", " << source->getName() << ")\n"; );

		const bool isArrayArg = isa<ArrayType>(arg->getType());
		const bool isPointerArg = isa<PointerType>(arg->getType());

		for (Function::use_iterator U=f->use_begin(), UE=f->use_end(); U!=UE; ) {
			if (!isa<CallInst>(*U)) continue;
			CallInst* call = cast<CallInst>(*U++);
			WFVOPENCL_DEBUG( outs() << "replacing use: " << *call << "\n"; );

			if (call->getParent()->getParent() != source) {
				WFVOPENCL_DEBUG( outs() << "  is in different function: " << call->getParent()->getParent()->getNameStr() << "\n"; );
				continue; // ignore uses in other functions
			}

			// if arg type is an array, check second operand of call (= supplied parameter)
			// and generate appropriate ExtractValueInst
			if (isArrayArg) {
				WFVOPENCL_DEBUG( outs() << "  array arg found!\n"; );
				const Value* dimVal = call->getArgOperand(0);
				assert (isa<ConstantInt>(dimVal));
				const ConstantInt* dimConst = cast<ConstantInt>(dimVal);
				const uint64_t* dimension = dimConst->getValue().getRawData();
				ExtractValueInst* ev = ExtractValueInst::Create(arg, *dimension, "", call);
				WFVOPENCL_DEBUG( outs() << "  new extract: " << *ev << "\n"; );

				// if the result is a 64bit integer value, truncate to 32bit -> more other problems :/
				//if (ev->getType() == f->getReturnType()) arg = ev;
				//else arg = TruncInst::CreateTruncOrBitCast(ev, f->getReturnType(), "", call);
				//outs() << "  new extract/cast: " << *arg << "\n";

				assert (f->getReturnType() == ev->getType());
				call->replaceAllUsesWith(ev);
				call->eraseFromParent();
			} else if (isPointerArg) {
				WFVOPENCL_DEBUG( outs() << "  pointer arg found!\n"; );
				Value* dimVal = call->getArgOperand(0);
				WFVOPENCL_DEBUG( outs() << "  dimVal: " << *dimVal << "\n"; );
				WFVOPENCL_DEBUG( outs() << "  arg: " << *arg << "\n"; );
				GetElementPtrInst* gep = GetElementPtrInst::Create(arg, dimVal, "", call);
				LoadInst* load = new LoadInst(gep, "", false, 16, call);
				WFVOPENCL_DEBUG( outs() << "  new gep: " << *gep << "\n"; );
				WFVOPENCL_DEBUG( outs() << "  new load: " << *load << "\n"; );

				assert (f->getReturnType() == load->getType());
				call->replaceAllUsesWith(load);
				call->eraseFromParent();
			} else {
				WFVOPENCL_DEBUG( outs() << "  normal arg found!\n"; );
				WFVOPENCL_DEBUG( outs() << "  arg: " << *arg << "\n"; );
				assert (f->getReturnType() == arg->getType());
				call->replaceAllUsesWith(arg);
				call->eraseFromParent();
			}

		} // for each use

	}

	llvm::Function* generateKernelWrapper(
			const std::string& wrapper_name,
			llvm::Function* f,
			llvm::Module* mod,
			TargetData* targetData,
			const bool inlineCall)
	{
		assert (f && mod);

		LLVMContext& context = mod->getContext();

		// collect return types of the callback functions of interest
		std::vector<const llvm::Type*> additionalParams;
		additionalParams.push_back(Type::getInt32Ty(context)); // get_work_dim = cl_uint
		additionalParams.push_back(Type::getInt32PtrTy(context, 0)); // get_global_size = size_t[]
		additionalParams.push_back(Type::getInt32PtrTy(context, 0)); // get_local_size = size_t[]
		additionalParams.push_back(Type::getInt32PtrTy(context, 0)); // get_group_id = size_t[]
		// other callbacks are resolved inside kernel

		// generate wrapper
		llvm::Function* wrapper = WFVOpenCL::generateFunctionWrapperWithParams(wrapper_name, f, mod, additionalParams, inlineCall);
		if (!wrapper) return NULL;

		// set argument names and attributes
		Function::arg_iterator arg = wrapper->arg_begin();
		++arg; arg->setName("get_work_dim");
		++arg; arg->setName("get_global_size");
		++arg; arg->setName("get_local_size");
		++arg; arg->setName("get_group_id");

		return wrapper;
	}

	CallInst* getWrappedKernelCall(Function* wrapper, Function* kernel) {
		for (Function::use_iterator U=kernel->use_begin(), UE=kernel->use_end(); U!=UE; ++U) {
			if (!isa<CallInst>(*U)) continue;
			CallInst* call = cast<CallInst>(*U);
			if (call->getParent()->getParent() == wrapper) return call;
		}
		assert (false && "could not find call to kernel - inlined already?");
		return NULL;
	}

	// TODO: make sure all functions have appropriate attributes (nounwind, readonly/readnone, ...)
	void fixFunctionNames(Module* mod) {
		assert (mod);
		// fix __sqrt_f32
		if (WFVOpenCL::getFunction("__sqrt_f32", mod)) {

			// create llvm.sqrt.f32 intrinsic
			const llvm::Type* floatType = WFVOpenCL::getTypeFromString(mod, "f");
			std::vector<const llvm::Type*> params;
			params.push_back(floatType);
			WFVOpenCL::createExternalFunction("llvm.sqrt.f32", floatType, params, mod);
			assert (WFVOpenCL::getFunction("llvm.sqrt.f32", mod));

			WFVOpenCL::replaceAllUsesWith(WFVOpenCL::getFunction("__sqrt_f32", mod), WFVOpenCL::getFunction("llvm.sqrt.f32", mod));
		}
		// fix __exp_f32
		if (WFVOpenCL::getFunction("__exp_f32", mod)) {

#if 1
			// create llvm.exp.f32 intrinsic
			const llvm::Type* floatType = WFVOpenCL::getTypeFromString(mod, "f");
			std::vector<const llvm::Type*> params;
			params.push_back(floatType);
			WFVOpenCL::createExternalFunction("llvm.exp.f32", floatType, params, mod);
			assert (WFVOpenCL::getFunction("llvm.exp.f32", mod));

			WFVOpenCL::replaceAllUsesWith(WFVOpenCL::getFunction("__exp_f32", mod), WFVOpenCL::getFunction("llvm.exp.f32", mod));
#else
			// TODO: This requires llvm/Intrinsics.h to be included in this file.
			//       Do we really want to capsulate all LLVM stuff into llvmTools.hpp ???
			const Type** types = new const Type*[1]();
			types[0] = Type::getFloatTy(getGlobalContext());
			Function* exp = Intrinsic::getDeclaration(mod, Intrinsic::exp, types, 1);
			mod->getFunction("__exp_f32")->replaceAllUsesWith(exp);
#endif
		}
		// fix __log_f32
		if (WFVOpenCL::getFunction("__log_f32", mod)) {

			// create llvm.log.f32 intrinsic
			const llvm::Type* floatType = WFVOpenCL::getTypeFromString(mod, "f");
			std::vector<const llvm::Type*> params;
			params.push_back(floatType);
			WFVOpenCL::createExternalFunction("llvm.log.f32", floatType, params, mod);
			assert (WFVOpenCL::getFunction("llvm.log.f32", mod));

			WFVOpenCL::replaceAllUsesWith(WFVOpenCL::getFunction("__log_f32", mod), WFVOpenCL::getFunction("llvm.log.f32", mod));
		}
		// fix __log2_f32
		if (WFVOpenCL::getFunction("__log2_f32", mod)) {

			// create llvm.log2.f32 intrinsic
			const llvm::Type* floatType = WFVOpenCL::getTypeFromString(mod, "f");
			std::vector<const llvm::Type*> params;
			params.push_back(floatType);
			WFVOpenCL::createExternalFunction("llvm.log.f32", floatType, params, mod);
			assert (WFVOpenCL::getFunction("llvm.log.f32", mod));

			WFVOpenCL::replaceAllUsesWith(WFVOpenCL::getFunction("__log2_f32", mod), WFVOpenCL::getFunction("llvm.log.f32", mod));
		}
		// fix __fabs_f32
		if (WFVOpenCL::getFunction("__fabs_f32", mod)) {

			// create fabs intrinsic
			const llvm::Type* floatType = WFVOpenCL::getTypeFromString(mod, "f");
			std::vector<const llvm::Type*> params;
			params.push_back(floatType);
			WFVOpenCL::createExternalFunction("fabs", floatType, params, mod);
			assert (WFVOpenCL::getFunction("fabs", mod));

			WFVOpenCL::replaceAllUsesWith(WFVOpenCL::getFunction("__fabs_f32", mod), WFVOpenCL::getFunction("fabs", mod));
		}
		// fix __fmod_f32
		if (Function* fmodFun = WFVOpenCL::getFunction("__fmod_f32", mod)) {
#if 0
			// create llvm.fmod.f32 intrinsic
			const llvm::Type* floatType = WFVOpenCL::getTypeFromString(mod, "f");
			std::vector<const llvm::Type*> params;
			params.push_back(floatType);
			params.push_back(floatType);
			WFVOpenCL::createExternalFunction("fmodf", floatType, params, mod);
			assert (WFVOpenCL::getFunction("fmodf", mod));

			WFVOpenCL::replaceAllUsesWith(fmodFun, WFVOpenCL::getFunction("fmodf", mod));
#else
			for (Function::use_iterator U=fmodFun->use_begin(), UE=fmodFun->use_end(); U!=UE; ) {
				assert (isa<CallInst>(*U));
				CallInst* call = cast<CallInst>(*U++);
				Value* val0 = call->getArgOperand(0);
				Value* val1 = call->getArgOperand(1);
				BinaryOperator* subInst = BinaryOperator::Create(Instruction::FRem, val0, val1, "", call);
				call->replaceAllUsesWith(subInst);
				call->eraseFromParent();
			}
#endif
		}
		// fix __cos_f32
		if (WFVOpenCL::getFunction("__cos_f32", mod)) {

			// create llvm.cos.f32 intrinsic
			const llvm::Type* floatType = WFVOpenCL::getTypeFromString(mod, "f");
			std::vector<const llvm::Type*> params;
			params.push_back(floatType);
			WFVOpenCL::createExternalFunction("llvm.cos.f32", floatType, params, mod);
			assert (WFVOpenCL::getFunction("llvm.cos.f32", mod));

			WFVOpenCL::replaceAllUsesWith(WFVOpenCL::getFunction("__cos_f32", mod), WFVOpenCL::getFunction("llvm.cos.f32", mod));
		}
		// fix __sin_f32
		if (WFVOpenCL::getFunction("__sin_f32", mod)) {

			// create llvm.sin.f32 intrinsic
			const llvm::Type* floatType = WFVOpenCL::getTypeFromString(mod, "f");
			std::vector<const llvm::Type*> params;
			params.push_back(floatType);
			WFVOpenCL::createExternalFunction("llvm.sin.f32", floatType, params, mod);
			assert (WFVOpenCL::getFunction("llvm.sin.f32", mod));

			WFVOpenCL::replaceAllUsesWith(WFVOpenCL::getFunction("__sin_f32", mod), WFVOpenCL::getFunction("llvm.sin.f32", mod));
		}
		// fix __pow_f32
		if (WFVOpenCL::getFunction("__pow_f32", mod)) {

			// create llvm.pow.f32 intrinsic
			const llvm::Type* floatType = WFVOpenCL::getTypeFromString(mod, "f");
			std::vector<const llvm::Type*> params;
			params.push_back(floatType);
			params.push_back(floatType);
			WFVOpenCL::createExternalFunction("powf", floatType, params, mod);
			assert (WFVOpenCL::getFunction("powf", mod));

			WFVOpenCL::replaceAllUsesWith(WFVOpenCL::getFunction("__pow_f32", mod), WFVOpenCL::getFunction("powf", mod));
		}
	}

	// TODO: implement some kind of heuristic
	unsigned getBestSimdDim(Function* f, const unsigned num_dimensions) {
		return 0;
	}
	unsigned determineNumDimensionsUsed(Function* f) {
		unsigned max_dim = 1;
		for (Function::iterator BB=f->begin(), BBE=f->end(); BB!=BBE; ++BB) {
			for (BasicBlock::iterator I=BB->begin(), IE=BB->end(); I!=IE; ++I) {
				if (!isa<CallInst>(I)) continue;
				CallInst* call = cast<CallInst>(I);

				const Function* callee = call->getCalledFunction();
				const StringRef fnName = callee->getName();
				if (fnName.equals("get_global_id") ||
						fnName.equals("get_local_id") ||
						fnName.equals("get_num_groups") ||
						fnName.equals("get_work_dim") ||
						fnName.equals("get_global_size") ||
						fnName.equals("get_local_size") ||
						fnName.equals("get_group_id")) {
					// get dimension
					const Value* dimVal = call->getArgOperand(0);
					assert (isa<ConstantInt>(dimVal));
					const ConstantInt* dimConst = cast<ConstantInt>(dimVal);
					const uint64_t dimension = *dimConst->getValue().getRawData() +1; // uses count from 0, max_dim from 1
					
					assert (dimension <= WFVOPENCL_MAX_NUM_DIMENSIONS);
					if (dimension > max_dim) max_dim = dimension;
				}
			}
		}
		WFVOPENCL_DEBUG( outs() << "\nnumber of dimensions used in kernel: " << max_dim << "\n"; );
		return max_dim;
	}

	// generate computation of "flattened" local id
	// this is required to access the correct live value struct of each local
	// instance (all dimension's instances of the block are stored flattened in
	// memory) if iterating
	// 'for all dim0 { for all dim1 { for all dim2 { ... } } }' :
	// local_flat_id(1d) = loc_id[0]
	// local_flat_id(2d) = loc_id[0] + loc_id[1] * loc_size[0]
	// local_flat_id(3d) = loc_id[0] + loc_id[1] * loc_size[0] + loc_id[2] * (loc_size[1] * loc_size[0])
	Value* generateLocalFlatIndex(const unsigned num_dimensions, Instruction** local_ids, Instruction** local_sizes, Instruction* insertBefore) {
		Value* local_id_flat = local_ids[0];
		for (unsigned i=1; i<num_dimensions; ++i) {
			Value* tmp = local_ids[i];
			for (int j=i-1; j>=0; --j) {
				tmp = BinaryOperator::Create(Instruction::Mul, tmp, local_sizes[j], "", insertBefore);
			}
			local_id_flat = BinaryOperator::Create(Instruction::Add, tmp, local_id_flat, "", insertBefore);
		}
		return local_id_flat;
	}

	void adjustLiveValueLoadGEPs(CallInst* newCall, const unsigned continuation_id, const unsigned num_dimensions, Instruction** local_ids, Instruction** local_sizes) {
		
		// generate computation of "flattened" local id
		BasicBlock* callBB = newCall->getParent();
		Value* local_id_flat = generateLocalFlatIndex(num_dimensions, local_ids, local_sizes, callBB->getFirstNonPHI());
		if (local_id_flat != local_ids[0]) {
			std::stringstream sstr;
			sstr << "local_id_flat_cont_" << continuation_id;
			local_id_flat->setName(sstr.str());
		}
		
		WFVOPENCL_DEBUG_RUNTIME(
			insertPrintf(
				"\ncontinuation ",
				ConstantInt::get(
					newCall->getContext(),
					APInt(32, continuation_id)
				),
				true,
				callBB->getFirstNonPHI()
			);
		);
		
		// adjust GEP-instructions to point to current localID's live value struct,
		// e.g. GEP liveValueUnion, i32 0, i32 elementindex
		// ---> GEP liveValueUnion, i32 local_id_flat, i32 elementindex
		// TODO: move this to a new function
		Value* liveValueStruct = newCall->getArgOperand(newCall->getNumArgOperands()-1); // live value union is last parameter to call
		WFVOPENCL_DEBUG( outs() << "live value struct: " << *liveValueStruct << "\n"; );

		// now get the bitcast-use of the union in this same block
		BitCastInst* liveValueStructBc = NULL;
		for (Value::use_iterator U=liveValueStruct->use_begin(), UE=liveValueStruct->use_end(); U!=UE; ++U) {
			if (!isa<BitCastInst>(*U)) continue;
			BitCastInst* bc = cast<BitCastInst>(*U);
			if (bc->getParent() != callBB) continue;
			
			liveValueStructBc = bc;
			break; // there is exactly one use of interest
		}
		assert (liveValueStructBc);
		
		// Uses of this bitcast are the GEPs for the load operations that extract the live values.
		// Replace the first index of each GEP (=0 for pointer-step-through in the standard continuation case)
		// by the correct local index.
		for (BitCastInst::use_iterator U=liveValueStructBc->use_begin(), UE=liveValueStructBc->use_end(); U!=UE; ) {
			if (!isa<GetElementPtrInst>(*U)) { ++U; continue; }
			GetElementPtrInst* gep = cast<GetElementPtrInst>(*U++);
			std::vector<Value*> params;
			for (GetElementPtrInst::op_iterator O=gep->idx_begin(), OE=gep->idx_end(); O!=OE; ++O) {
				if (O == gep->idx_begin()) {
					// replace first index by correct flat index
					params.push_back(local_id_flat);
				} else {
					// all other indices remain unchanged
					assert (isa<Value>(O));
					params.push_back(cast<Value>(O));
				}
			}
			
			// replace gep
			GetElementPtrInst* newGEP = GetElementPtrInst::Create(gep->getPointerOperand(), params.begin(), params.end(), "", gep);
			gep->replaceAllUsesWith(newGEP);
			gep->eraseFromParent();
			
			WFVOPENCL_DEBUG_RUNTIME(
				assert (newGEP->getNumUses() == 1);
				Value* gepUse = newGEP->use_back();
				insertPrintf("live value loaded: ", gepUse, true, newCall);
			);
		}
	}

	void adjustLiveValueStoreGEPs(Function* continuation, const unsigned num_dimensions, LLVMContext& context) {
		assert (continuation);
		WFVOPENCL_DEBUG( outs() << "\nadjustLiveValueStoreGEPs(" << continuation->getNameStr() << ")\n"; );
		// get the live value union (= last parameter of function)
		Value* liveValueStruct = --(continuation->arg_end());
		WFVOPENCL_DEBUG( outs() << "live value struct: " << *liveValueStruct << "\n"; );
		if (liveValueStruct->use_empty()) {
			WFVOPENCL_DEBUG( outs() << "  has no uses -> no adjustment necessary!\n"; );
			return;
		}

		assert (!liveValueStruct->use_empty());

		// load local_ids and local_sizes for the next computation
		Argument* arg_local_id_array = ++continuation->arg_begin(); // 2nd argument
		Function::arg_iterator tmpA = continuation->arg_begin();
		std::advance(tmpA, 5); // 5th argument
		Argument* arg_local_size_array = tmpA;

		Instruction** local_ids = new Instruction*[num_dimensions]();
		Instruction** local_sizes = new Instruction*[num_dimensions]();


		// the only use of this argument we are interested in is a bitcast to
		// the next continuation's live value struct type in the same block as
		// the call to the barrier
		// TODO: we simply transform *all* uses because it would be more work to
		// make all necessary information (barrier-call) available here.
		for (Value::use_iterator U=liveValueStruct->use_begin(), UE=liveValueStruct->use_end(); U!=UE; ++U) {
			//assert (isa<BitCastInst>(*U));
			if (!isa<BitCastInst>(*U)) {
				// if this is no bitcast, it can only be a store instruction
				// generated by ExtractCodeRegion (which treats the liveValueStruct
				// as a live value itself and stores it)
				assert (isa<StoreInst>(*U));
				StoreInst* st = cast<StoreInst>(*U);
				st->eraseFromParent();
				continue;
			}
			BitCastInst* liveValueStructBc = cast<BitCastInst>(*U);

			for (unsigned i=0; i<num_dimensions; ++i) {
				Value* dimIdx = ConstantInt::get(context, APInt(32, i));

				std::stringstream sstr;
				sstr << "local_id_" << i;
				GetElementPtrInst* gep = GetElementPtrInst::Create(arg_local_id_array, dimIdx, "", liveValueStructBc);
				local_ids[i] = new LoadInst(gep, sstr.str(), false, 16, liveValueStructBc);

				std::stringstream sstr2;
				sstr2 << "local_size_" << i;
				gep = GetElementPtrInst::Create(arg_local_size_array, dimIdx, "", liveValueStructBc);
				local_sizes[i] = new LoadInst(gep, sstr2.str(), false, 16, liveValueStructBc);
			}

			// compute the local "flat" index (computation will be redundant after inlining,
			// but this is easier than introducing another parameter to the function)
			Value* local_id_flat = generateLocalFlatIndex(num_dimensions, local_ids, local_sizes, liveValueStructBc);
			if (local_id_flat != local_ids[0]) local_id_flat->setName("local_id_flat");


			// Uses of this bitcast are the GEPs for the store operations of the live values.
			// Replace the first index of each GEP (=0 for pointer-step-through in the standard continuation case)
			// by the correct local index.
			for (BitCastInst::use_iterator U=liveValueStructBc->use_begin(), UE=liveValueStructBc->use_end(); U!=UE; ) {
				if (!isa<GetElementPtrInst>(*U)) { ++U; continue; }
				GetElementPtrInst* gep = cast<GetElementPtrInst>(*U++);
				assert (liveValueStructBc->getParent()->getParent() == gep->getParent()->getParent());
				std::vector<Value*> params;
				for (GetElementPtrInst::op_iterator O=gep->idx_begin(), OE=gep->idx_end(); O!=OE; ++O) {
					if (O == gep->idx_begin()) {
						// replace first index by correct flat index
						params.push_back(local_id_flat);
					} else {
						// all other indices remain unchanged
						assert (isa<Value>(O));
						params.push_back(cast<Value>(O));
					}
				}

				// replace gep
				GetElementPtrInst* newGEP = GetElementPtrInst::Create(gep->getPointerOperand(), params.begin(), params.end(), "", gep);
				gep->replaceAllUsesWith(newGEP);
				gep->eraseFromParent();

				WFVOPENCL_DEBUG_RUNTIME(
					assert (newGEP->getNumUses() == 1);
					Value* gepUse = newGEP->use_back();
					assert (isa<StoreInst>(gepUse));
					StoreInst* store = cast<StoreInst>(gepUse);
					Value* storedVal = store->getOperand(0);
					insertPrintf("live value stored: ", storedVal, true, store->getParent()->getTerminator());
					//insertPrintf("  address: ", store->getOperand(1), true, store->getParent()->getTerminator());
				);
			}
		}

		delete [] local_ids;
		delete [] local_sizes;
	}

	void mapCallbacksToContinuationArguments(const unsigned num_dimensions, LLVMContext& context, Module* module, ContinuationGenerator::ContinuationVecType& continuations) {
		typedef ContinuationGenerator::ContinuationVecType ContVecType;

		for (ContVecType::iterator it=continuations.begin(), E=continuations.end(); it!=E; ++it) {
			Function* continuation = *it;
			assert (continuation);
			WFVOPENCL_DEBUG( outs() << "\nmapping callbacks to arguments in continuation '" << continuation->getNameStr() << "'...\n"; );

			// correct order is important! (has to match parameter list of continuation)
			llvm::Function::arg_iterator arg = continuation->arg_begin();
			WFVOpenCL::replaceCallbacksByArgAccess(module->getFunction("get_global_id"),      cast<Value>(arg++), continuation);
			WFVOpenCL::replaceCallbacksByArgAccess(module->getFunction("get_local_id"),       cast<Value>(arg++), continuation);
			WFVOpenCL::replaceCallbacksByArgAccess(module->getFunction("get_num_groups"),     cast<Value>(arg++), continuation);
			WFVOpenCL::replaceCallbacksByArgAccess(module->getFunction("get_work_dim"),       cast<Value>(arg++), continuation);
			WFVOpenCL::replaceCallbacksByArgAccess(module->getFunction("get_global_size"),    cast<Value>(arg++), continuation);
			WFVOpenCL::replaceCallbacksByArgAccess(module->getFunction("get_local_size"),     cast<Value>(arg++), continuation);
			WFVOpenCL::replaceCallbacksByArgAccess(module->getFunction("get_group_id"),       cast<Value>(arg++), continuation);
		}

		return;
	}
	void createGroupConstantSpecialParamLoads(
			const unsigned num_dimensions,
			LLVMContext& context,
			Value* arg_work_dim,
			Value* arg_global_size_array,
			Value* arg_local_size_array,
			Value* arg_group_id_array,
			Value* arg_num_groups_array,
			Instruction** global_sizes,
			Instruction** local_sizes,
			Instruction** group_ids,
			Instruction** num_groupss,
			Instruction* insertBefore
	) {
		assert (arg_global_size_array->getType()->isPointerTy());
		const Type* argType = arg_global_size_array->getType()->getContainedType(0);

		for (unsigned i=0; i<num_dimensions; ++i) {
			Value* dimIdx = ConstantInt::get(argType, i, false); //ConstantInt::get(context, APInt(32, i));
			// "0123456789ABCDEF"[i] only works as long as we do not have more than 10 dimensions :P
			// TODO: somehow it doesn't and just screws the names...

			std::stringstream sstr;
			sstr << "global_size_" << i;
			GetElementPtrInst* gep = GetElementPtrInst::Create(arg_global_size_array, dimIdx, "", insertBefore);
			global_sizes[i] = new LoadInst(gep, sstr.str(), false, 16, insertBefore);

			std::stringstream sstr2;
			sstr2 << "local_size_" << i;
			gep = GetElementPtrInst::Create(arg_local_size_array, dimIdx, "", insertBefore);
			local_sizes[i] = new LoadInst(gep, sstr2.str(), false, 16, insertBefore);

			std::stringstream sstr3;
			sstr3 << "group_id_" << i;
			gep = GetElementPtrInst::Create(arg_group_id_array, dimIdx, "", insertBefore);
			group_ids[i] = new LoadInst(gep, sstr3.str(), false, 16, insertBefore);

			std::stringstream sstr4;
			sstr4 << "num_groups_" << i;
#if 1
			// NOTE: We rely on global_sizes being dividable by local_sizes at this point.
			//       Otherwise we would have to to make sure that num_groups always returns at least 1.
			num_groupss[i] = BinaryOperator::Create(Instruction::UDiv, global_sizes[i], local_sizes[i], sstr4.str(), insertBefore);
#else
			Instruction* div = BinaryOperator::Create(Instruction::UDiv, global_sizes[i], local_sizes[i], "", insertBefore);
			ICmpInst* cmp = new ICmpInst(insertBefore, ICmpInst::ICMP_EQ, div, ConstantInt::get(context, APInt(32, 0)), "");
			num_groupss[i] = SelectInst::Create(cmp, ConstantInt::get(context, APInt(32, 1)), div, sstr4.str(), insertBefore);
#endif

			WFVOPENCL_DEBUG( outs() << "  global_sizes[" << i << "]: " << *(global_sizes[i]) << "\n"; );
			WFVOPENCL_DEBUG( outs() << "  local_sizes[" << i << "] : " << *(local_sizes[i]) << "\n"; );
			WFVOPENCL_DEBUG( outs() << "  group_ids[" << i << "]   : " << *(group_ids[i]) << "\n"; );
			WFVOPENCL_DEBUG( outs() << "  num_groups[" << i << "]  : " << *(num_groupss[i]) << "\n"; );

			// store num_groups into array
			gep = GetElementPtrInst::Create(arg_num_groups_array, dimIdx, "", insertBefore);
			new StoreInst(num_groupss[i], gep, false, 16, insertBefore);
			//InsertValueInst::Create(arg_num_groups_array, num_groupss[i], i, "", insertBefore); // TODO: maybe later...

			WFVOPENCL_DEBUG_RUNTIME(
				insertPrintf("i = ", dimIdx, true, insertBefore);
				insertPrintf("work_dim: ", arg_work_dim, true, insertBefore);
				insertPrintf("global_sizes[i]: ", global_sizes[i], true, insertBefore);
				insertPrintf("local_sizes[i]: ", local_sizes[i], true, insertBefore);
				insertPrintf("group_ids[i]: ", group_ids[i], true, insertBefore);
				insertPrintf("num_groups[i]: ", num_groupss[i], true, insertBefore);
			);
		}
	}

	void generateLoopsAroundCall(
			CallInst* call,
			const unsigned num_dimensions,
			const int simd_dim,
			Instruction** local_sizes,
			Instruction** group_ids,
			Value* arg_global_id_array,
			Value* arg_local_id_array,
			LLVMContext& context,
			Instruction** global_ids,
			Instruction** local_ids
	) {
		assert (call && local_sizes && group_ids && global_ids && local_ids);
		
		Function* f = call->getParent()->getParent();
		Instruction* insertBefore = call;

		assert (arg_global_id_array->getType()->isPointerTy());
		const Type* argType = arg_global_id_array->getType()->getContainedType(0);

		for (int i=num_dimensions-1; i>=0; --i) {
			Value* local_size = local_sizes[i];
			Value* group_id = group_ids[i];

			// split parent before first instruction (all liveValueUnion-extraction code has to be inside loop)
			BasicBlock* headerBB = call->getParent(); // first iteration = tmpHeaderBB

			assert (headerBB->getUniquePredecessor());
			BasicBlock* entryBB = headerBB->getUniquePredecessor(); // tmpEntryBB -> header of current innermost loop
			BasicBlock* exitBB  = *succ_begin(headerBB); // tmpExitBB -> latch of current innermost loop

			BasicBlock* loopBB  = headerBB->splitBasicBlock(headerBB->begin(), headerBB->getNameStr()+".loop");
			BasicBlock* latchBB = BasicBlock::Create(context, headerBB->getNameStr()+".loop.end", f, loopBB);
			loopBB->moveBefore(latchBB); // only for more intuitive readability

			// Block headerBB
			std::stringstream sstr;
			sstr << "local_id_" << i;
			const Type* counterType = argType; //Type::getInt32Ty(context);
			Argument* fwdref = new Argument(counterType);
			PHINode* loopCounterPhi = PHINode::Create(counterType, sstr.str(), headerBB->getFirstNonPHI());
			loopCounterPhi->reserveOperandSpace(2);
			loopCounterPhi->addIncoming(Constant::getNullValue(counterType), entryBB);
			loopCounterPhi->addIncoming(fwdref, latchBB);

			Instruction* local_id = loopCounterPhi;

			// Block loopBB
			// holds live value extraction and continuation-call
			loopBB->getTerminator()->eraseFromParent();
			BranchInst::Create(latchBB, loopBB);

			// Block latchBB
#ifdef WFVOPENCL_NO_WFV
			const uint64_t incInt = 1;
#else
			// fall back to increments of 1 if vectorization failed
			const uint64_t incInt =
				(simd_dim == -1 ? 1 :
					(i == simd_dim ? WFVOPENCL_SIMD_WIDTH : 1U));
#endif
			BinaryOperator* loopCounterInc = BinaryOperator::Create(Instruction::Add, loopCounterPhi, ConstantInt::get(counterType, incInt, false), "inc", latchBB);
			ICmpInst* exitcond1 = new ICmpInst(*latchBB, ICmpInst::ICMP_UGE, loopCounterInc, local_size, "exitcond");
			BranchInst::Create(exitBB, headerBB, exitcond1, latchBB);

			// Resolve Forward References
			fwdref->replaceAllUsesWith(loopCounterInc); delete fwdref;

			assert (num_dimensions > 0);
			if (i == (int)num_dimensions-1) {
				// replace uses of loopBB in phis of exitBB with outermost latchBB
				for (BasicBlock::iterator I=exitBB->begin(), IE=exitBB->end(); I!=IE; ++I) {
					if (exitBB->getFirstNonPHI() == I) break;
					PHINode* phi = cast<PHINode>(I);

					Value* val = phi->getIncomingValueForBlock(loopBB);
					phi->removeIncomingValue(loopBB, false);
					phi->addIncoming(val, latchBB);
				}
			}

			// generate special parameter global_id right before call
			
			std::stringstream sstr2;
			sstr2 << "global_id_" << i;
			Instruction* global_id = BinaryOperator::Create(Instruction::Mul, group_id, local_size, "", call);
			global_id = BinaryOperator::Create(Instruction::Add, global_id, local_id, sstr2.str(), call);

			// save special parameters global_id, local_id to arrays
			GetElementPtrInst* gep = GetElementPtrInst::Create(arg_global_id_array, ConstantInt::get(context, APInt(32, i)), "", insertBefore);
			new StoreInst(global_id, gep, false, 16, call);
			gep = GetElementPtrInst::Create(arg_local_id_array, ConstantInt::get(context, APInt(32, i)), "", insertBefore);
			new StoreInst(local_id, gep, false, 16, call);

			global_ids[i] = global_id;
			local_ids[i] = local_id;

			WFVOPENCL_DEBUG_RUNTIME(
				//insertPrintf("global_id[i]: ", global_ids[i], true, call);
				//insertPrintf("local_id[i]: ", local_ids[i], true, call);
			);

		}
	}

	void generateBlockSizeLoopsForWrapper(Function* f, CallInst* call, const unsigned num_dimensions, const int simd_dim, LLVMContext& context, Module* module) {
		assert (f && call);
		assert (f == call->getParent()->getParent());
		assert (num_dimensions <= WFVOPENCL_MAX_NUM_DIMENSIONS);
		WFVOPENCL_DEBUG( outs() << "\ngenerating loop(s) over group size(s) in function '"
				<< f->getNameStr() << "' around call to '" << call->getCalledFunction()->getNameStr() << "'...\n\n"; );

		Instruction* insertBefore = call;

		Function::arg_iterator A = f->arg_begin(); // arg_struct
		Value* arg_work_dim = ++A;
		Value* arg_global_size_array = ++A;
		Value* arg_local_size_array = ++A;
		Value* arg_group_id_array = ++A;

		WFVOPENCL_DEBUG( outs() << "  work_dim arg   : " << *arg_work_dim << "\n"; );
		WFVOPENCL_DEBUG( outs() << "  global_size arg: " << *arg_global_size_array << "\n"; );
		WFVOPENCL_DEBUG( outs() << "  local_size arg : " << *arg_local_size_array << "\n"; );
		WFVOPENCL_DEBUG( outs() << "  group_id arg   : " << *arg_group_id_array << "\n"; );

		// allocate array of size 'num_dimensions' for special parameter num_groups
		assert (arg_global_size_array->getType()->isPointerTy());
		const Type* argType = arg_global_size_array->getType()->getContainedType(0);
		AllocaInst* arg_num_groups_array = new AllocaInst(argType, ConstantInt::get(context,  APInt(32, num_dimensions)), "num_groups_array", insertBefore);

		// load/compute special values for each dimension
		Instruction** global_sizes = new Instruction*[num_dimensions]();
		Instruction** local_sizes = new Instruction*[num_dimensions]();
		Instruction** group_ids = new Instruction*[num_dimensions]();
		Instruction** num_groupss = new Instruction*[num_dimensions]();

		createGroupConstantSpecialParamLoads(
				num_dimensions,
				context,
				arg_work_dim,
				arg_global_size_array,
				arg_local_size_array,
				arg_group_id_array,
				arg_num_groups_array,
				global_sizes,
				local_sizes,
				group_ids,
				num_groupss,
				insertBefore);

		Instruction** global_ids = new Instruction*[num_dimensions](); // not required for anything else but being supplied as parameter
		Instruction** local_ids = new Instruction*[num_dimensions]();

		// allocate array of size 'num_dimensions' for special parameters global_id, local_id
		Value* numDimVal = ConstantInt::get(context,  APInt(32, num_dimensions));
		AllocaInst* arg_global_id_array = new AllocaInst(argType, numDimVal, "global_id_array", insertBefore);
		AllocaInst* arg_local_id_array  = new AllocaInst(argType, numDimVal, "local_id_array", insertBefore);

		assert (f->getBasicBlockList().size() == 1);

		// split parent at call
		BasicBlock* tmpEntryBB = call->getParent();
		BasicBlock* tmpExitBB = BasicBlock::Create(context, "exit", f);
		ReturnInst::Create(context, tmpExitBB);
		assert (isa<ReturnInst>(tmpEntryBB->getTerminator()));
		assert (cast<ReturnInst>(tmpEntryBB->getTerminator())->getReturnValue() == NULL);
		tmpEntryBB->getTerminator()->eraseFromParent();
		BranchInst::Create(tmpExitBB, tmpEntryBB); // create new terminator for entry block

		tmpEntryBB->splitBasicBlock(call, tmpEntryBB->getNameStr()+".header"); // res = tmpHeaderBB

		// now we have three blocks :)

		// generate loop(s)
		// iterate backwards in order to have loops ordered by dimension
		// (highest dimension = innermost loop)
		generateLoopsAroundCall(
				call,
				num_dimensions,
				simd_dim,
				local_sizes,
				group_ids,
				arg_global_id_array,
				arg_local_id_array,
				context,
				global_ids,
				local_ids);

		WFVOPENCL_DEBUG( WFVOpenCL::writeFunctionToFile(f, "debug_block_wrapper_noinline.ll"); );

		// inline all calls inside wrapper
		WFVOpenCL::inlineFunctionCalls(f);

		WFVOPENCL_DEBUG( WFVOpenCL::writeFunctionToFile(f, "debug_block_wrapper_inline.ll"); );

		// replace functions by parameter accesses (has to be done AFTER inlining!
		// start with second argument (first is void* of argument_struct)
		llvm::Function::arg_iterator arg = f->arg_begin();
		WFVOpenCL::replaceCallbacksByArgAccess(module->getFunction("get_work_dim"),       cast<Value>(++arg), f);
		WFVOpenCL::replaceCallbacksByArgAccess(module->getFunction("get_global_size"),    cast<Value>(++arg), f);
		WFVOpenCL::replaceCallbacksByArgAccess(module->getFunction("get_local_size"),     cast<Value>(++arg), f);
		WFVOpenCL::replaceCallbacksByArgAccess(module->getFunction("get_group_id"),       cast<Value>(++arg), f);

		// remap calls to parameters that are generated inside loop(s)
		WFVOpenCL::replaceCallbacksByArgAccess(module->getFunction("get_num_groups"),     arg_num_groups_array, f);
		WFVOpenCL::replaceCallbacksByArgAccess(module->getFunction("get_global_id"),      arg_global_id_array, f);
		WFVOpenCL::replaceCallbacksByArgAccess(module->getFunction("get_local_id"),       arg_local_id_array, f);

		WFVOPENCL_DEBUG( WFVOpenCL::writeFunctionToFile(f, "debug_block_wrapper_final.ll"); );


		WFVOPENCL_DEBUG( outs() << "\n" << *f << "\n"; );
		WFVOPENCL_DEBUG( verifyFunction(*f); );

		delete [] global_sizes;
		delete [] local_sizes;
		delete [] group_ids;
		delete [] num_groupss;
		delete [] global_ids;
		delete [] local_ids;
		WFVOPENCL_DEBUG( outs() << "generateBlockSizeLoopsForWrapper finished!\n"; );
	}

	// NOTE: This function relies on the switch-wrapper function (the one calling
	//       the continuations) being untouched (no optimization/inlining) after
	//       its generation!
	void generateBlockSizeLoopsForContinuations(const unsigned num_dimensions, const int simd_dim, LLVMContext& context, Function* f, ContinuationGenerator::ContinuationVecType& continuations) {
		assert (f);
		assert (num_dimensions <= WFVOPENCL_MAX_NUM_DIMENSIONS);
		WFVOPENCL_DEBUG( outs() << "\ngenerating loops over group size(s) around continuations...\n\n"; );
		typedef ContinuationGenerator::ContinuationVecType ContVecType;

		Instruction* insertBefore = f->begin()->getFirstNonPHI();

		Function::arg_iterator A = f->arg_begin(); // arg_struct
		Value* arg_work_dim = ++A;
		Value* arg_global_size_array = ++A;
		Value* arg_local_size_array = ++A;
		Value* arg_group_id_array = ++A;

		WFVOPENCL_DEBUG( outs() << "  work_dim arg   : " << *arg_work_dim << "\n"; );
		WFVOPENCL_DEBUG( outs() << "  global_size arg: " << *arg_global_size_array << "\n"; );
		WFVOPENCL_DEBUG( outs() << "  local_size arg : " << *arg_local_size_array << "\n"; );
		WFVOPENCL_DEBUG( outs() << "  group_id arg   : " << *arg_group_id_array << "\n"; );

		// allocate array of size 'num_dimensions' for special parameter num_groups
		Value* numDimVal = ConstantInt::get(context,  APInt(32, num_dimensions));
		assert (arg_global_size_array->getType()->isPointerTy());
		const Type* argType = arg_global_size_array->getType()->getContainedType(0);
		AllocaInst* arg_num_groups_array = new AllocaInst(argType, numDimVal, "num_groups_array", insertBefore);

		// load/compute special values for each dimension
		Instruction** global_sizes = new Instruction*[num_dimensions]();
		Instruction** local_sizes = new Instruction*[num_dimensions]();
		Instruction** group_ids = new Instruction*[num_dimensions]();
		Instruction** num_groupss = new Instruction*[num_dimensions]();

		createGroupConstantSpecialParamLoads(
				num_dimensions,
				context,
				arg_work_dim,
				arg_global_size_array,
				arg_local_size_array,
				arg_group_id_array,
				arg_num_groups_array,
				global_sizes,
				local_sizes,
				group_ids,
				num_groupss,
				insertBefore);

		Instruction** global_ids = new Instruction*[num_dimensions](); // not required for anything else but being supplied as parameter
		Instruction** local_ids = new Instruction*[num_dimensions]();

		// allocate array of size 'num_dimensions' for special parameters global_id, local_id
		AllocaInst* arg_global_id_array = new AllocaInst(argType, numDimVal, "global_id_array", insertBefore);
		AllocaInst* arg_local_id_array  = new AllocaInst(argType, numDimVal, "local_id_array", insertBefore);

		unsigned continuation_id = 0;
		for (ContVecType::iterator it=continuations.begin(), E=continuations.end(); it!=E; ++it, ++continuation_id) {
			Function* continuation = *it;
			assert (continuation);
			WFVOPENCL_DEBUG( outs() << "\n  generating loop(s) for continuation " << continuation_id << ": '" << continuation->getNameStr() << "'...\n"; );
			WFVOPENCL_DEBUG( outs() << "    has " << continuation->getNumUses() << " uses!\n"; );
			
			//WFVOPENCL_DEBUG( outs() << *continuation << "\n"; );

			assert (!continuation->use_empty());

			for (Function::use_iterator U=continuation->use_begin(), UE=continuation->use_end(); U!=UE; ++U) {
				assert (isa<CallInst>(*U));
				CallInst* call = cast<CallInst>(*U);
				if (call->getParent()->getParent() != f) continue; // ignore all uses in different functions

				WFVOPENCL_DEBUG( outs() << "    generating loop(s) around call: " << *call << "\n"; );

				generateLoopsAroundCall(
						call,
						num_dimensions,
						simd_dim,
						local_sizes,
						group_ids,
						arg_global_id_array,
						arg_local_id_array,
						context,
						global_ids,
						local_ids);

				// replace undef arguments to function call by special parameters
				std::vector<Value*> params;
				params.push_back(arg_global_id_array);
				params.push_back(arg_local_id_array);
				params.push_back(arg_num_groups_array);
				params.push_back(arg_work_dim);
				params.push_back(arg_global_size_array);
				params.push_back(arg_local_size_array);
				params.push_back(arg_group_id_array);

				WFVOPENCL_DEBUG(
					outs() << "\n    params for new call:\n";
					outs() << "     * " << *arg_global_id_array << "\n";
					outs() << "     * " << *arg_local_id_array << "\n";
					outs() << "     * " << *arg_num_groups_array << "\n";
					outs() << "     * " << *arg_work_dim << "\n";
					outs() << "     * " << *arg_global_size_array << "\n";
					outs() << "     * " << *arg_local_size_array << "\n";
					outs() << "     * " << *arg_group_id_array << "\n";
				);

				// add normal parameters and live value struct param
				//(= start at last special param idx +1 for callee)
				for (unsigned i=params.size(); i<call->getNumArgOperands(); ++i) {
					Value* opV = call->getArgOperand(i);
					params.push_back(opV);
					WFVOPENCL_DEBUG( outs() << "     * " << *opV << "\n"; );
				}
				CallInst* newCall = CallInst::Create(call->getCalledFunction(), params.begin(), params.end(), "", call);
				call->replaceAllUsesWith(newCall);
				call->eraseFromParent();

				WFVOPENCL_DEBUG( outs() << "\n    new call: " << *newCall << "\n\n"; );
				WFVOPENCL_DEBUG( outs() << "\n" << *continuation << "\n"; );

				// adjust GEP-instructions to point to current localID's live value struct,
				// e.g. GEP liveValueUnion, i32 0, i32 elementindex
				// ---> GEP liveValueUnion, i32 local_id_flat, i32 elementindex
				adjustLiveValueLoadGEPs(newCall, continuation_id, num_dimensions, local_ids, local_sizes);

				// Now do the exact same thing inside the continuation:
				// Replace the GEPs that are used for storing the live values
				// of the next continuation.
				adjustLiveValueStoreGEPs(continuation, num_dimensions, context);

				WFVOPENCL_DEBUG( outs() << "\n" << *continuation << "\n"; );
				WFVOPENCL_DEBUG( verifyFunction(*continuation); );

				break; // there is exactly one use of the continuation of interest
			}
		} // for each continuation

		// adjust alloca of liveValueUnion (reserve sizeof(union)*blocksize[0]*blocksize[1]*... )
		assert (continuations.back() && continuations.back()->use_back());
		assert (isa<CallInst>(continuations.back()->use_back()));
		CallInst* someContinuationCall = cast<CallInst>(continuations.back()->use_back());
		assert (someContinuationCall->getArgOperand(someContinuationCall->getNumArgOperands()-1));
		Value* liveValueUnion = someContinuationCall->getArgOperand(someContinuationCall->getNumArgOperands()-1);
		WFVOPENCL_DEBUG( outs() << "liveValueUnion: " << *liveValueUnion << "\n"; );

		assert (isa<AllocaInst>(liveValueUnion));
		AllocaInst* alloca = cast<AllocaInst>(liveValueUnion);
		Value* local_size_flat = local_sizes[0];
		for (unsigned i=1; i<num_dimensions; ++i) {
			local_size_flat = BinaryOperator::Create(Instruction::Mul, local_size_flat, local_sizes[i], "", alloca);
		}
		Value* newSize = BinaryOperator::Create(Instruction::Mul, alloca->getArraySize(), local_size_flat, "arraySize", alloca);
		AllocaInst* newAlloca = new AllocaInst(Type::getInt8Ty(context), newSize, "", alloca);
		alloca->replaceAllUsesWith(newAlloca);
		newAlloca->takeName(alloca);
		alloca->eraseFromParent();

		WFVOPENCL_DEBUG( outs() << "\n" << *f << "\n"; );
		WFVOPENCL_DEBUG( verifyFunction(*f); );

		delete [] global_sizes;
		delete [] local_sizes;
		delete [] group_ids;
		delete [] num_groupss;
		delete [] global_ids; // not required for anything else but being supplied as parameter
		delete [] local_ids;
	}

	Function* createKernel(Function* f, const std::string& kernel_name, const unsigned num_dimensions, int simd_dim, Module* module, TargetData* targetData, LLVMContext& context, cl_int* errcode_ret, Function** f_SIMD_ret) {
		assert (f && module && targetData);
		assert (num_dimensions > 0 && num_dimensions < 4);
		assert (simd_dim < (int)num_dimensions);

#ifdef WFVOPENCL_NO_WFV
		assert (simd_dim == -1); // packetization disabled: only -1 is a valid value
		assert (!f_SIMD_ret);

		std::stringstream strs;
		strs << kernel_name;
#else
		assert (simd_dim >= 0); // packetization enabled: 0, 1, 2 are valid values
		assert (f_SIMD_ret);

		// generate packet prototype
		std::stringstream strs;
		strs << kernel_name << "_SIMD";
		const std::string kernel_simd_name = strs.str();

		llvm::Function* f_SIMD = WFVOpenCL::createExternalFunction(kernel_simd_name, f->getFunctionType(), module);
		if (!f_SIMD) {
			errs() << "ERROR: could not create packet prototype for kernel '" << kernel_simd_name << "'!\n";
			return NULL;
		}

		WFVOPENCL_DEBUG( outs() << *f << "\n"; );

		WFVOPENCL_DEBUG( verifyModule(*module); );
		WFVOPENCL_DEBUG( outs() << "done.\n"; );

		// packetize scalar function into SIMD function
		WFVOPENCL_DEBUG( WFVOpenCL::writeFunctionToFile(f, "debug_kernel_pre_packetization.ll"); );

#ifdef WFVOPENCL_USE_AVX
		const bool use_sse41 = false;
		const bool use_avx = true;
#else
		const bool use_sse41 = true;
		const bool use_avx = false;
#endif
		const bool verbose = false;
		const bool vectorized =
			WFVOpenCL::packetizeKernelFunction(f->getNameStr(),
													 kernel_simd_name,
													 module,
													 WFVOPENCL_SIMD_WIDTH,
													 (cl_uint)simd_dim,
													 use_sse41,
													 use_avx,
													 verbose);

		if (vectorized) {
			f_SIMD = WFVOpenCL::getFunction(kernel_simd_name, module); // old pointer not valid anymore!

			WFVOPENCL_DEBUG( verifyModule(*module); );
			WFVOPENCL_DEBUG( WFVOpenCL::writeFunctionToFile(f_SIMD, "debug_kernel_packetized.ll"); );
			WFVOPENCL_DEBUG( WFVOpenCL::writeModuleToFile(f_SIMD->getParent(), "debug_f_simd.mod.ll"); );
			WFVOPENCL_DEBUG( outs() << *f_SIMD << "\n"; );

			WFVOPENCL_DEBUG_RUNTIME(
				BasicBlock* block = &f_SIMD->getEntryBlock();
				insertPrintf("\nf_SIMD called!", Constant::getNullValue(Type::getInt32Ty(getGlobalContext())), true, block->getFirstNonPHI());
				for (Function::iterator BB=f_SIMD->begin(), BBE=f_SIMD->end(); BB!=BBE; ++BB) {
					for (BasicBlock::iterator I=BB->begin(), IE=BB->end(); I!=IE; ++I) {
						if (CallInst* call = dyn_cast<CallInst>(I)) {
							std::string name = call->getCalledFunction()->getNameStr();
							if (name != "get_global_size" &&
								name != "get_local_size" &&
								name != "get_group_id" &&
								name != "get_global_id" &&
								name != "get_local_id") continue;

							assert (isa<ConstantInt>(call->getOperand(0)));
							ConstantInt* dimIdx = cast<ConstantInt>(call->getOperand(0));
							uint64_t intValue = *dimIdx->getValue().getRawData();
							std::stringstream sstr;
							sstr << name << "(" << intValue << "): ";

							insertPrintf(sstr.str(), call, true, BB->getTerminator());
						}
					}
				}
			);

//			BasicBlock* block = &f_SIMD->getEntryBlock();
//			for (BasicBlock::iterator I=block->begin(), IE=block->end(); I!=IE; ++I) {
//				if (GetElementPtrInst* gep = dyn_cast<GetElementPtrInst>(I)) {
//					insertPrintf("out-index: ", cast<Value>(gep->idx_begin()), true, block->getTerminator());
//					break;
//				}
//			}
//			int count = 0;
//			for (BasicBlock::iterator I=block->begin(), IE=block->end(); I!=IE; ++I) {
//				if (GetElementPtrInst* gep = dyn_cast<GetElementPtrInst>(I)) {
//					if (count == 0) { ++count; continue; }
//					insertPrintf("in-index: ", cast<Value>(gep->idx_begin()), true, block->getTerminator());
//				}
//			}
//			WFVOPENCL_DEBUG( WFVOpenCL::writeFunctionToFile(f_SIMD, "special.ll"); );

			f = f_SIMD;
		}
		else {
			// vectorization failed
			simd_dim = -1;
		}

#endif

		bool hasBarriers = false;
		for (Function::iterator BB=f->begin(), BBE=f->end();
				BB!=BBE && !hasBarriers; ++BB)
		{
			for (BasicBlock::iterator I=BB->begin(), IE=BB->end();
					I!=IE && !hasBarriers; ++I)
			{
				if (!isa<CallInst>(I)) continue;
				CallInst* call = cast<CallInst>(I);
				const Function* callee = call->getCalledFunction();
				if (!callee->getName().equals(WFVOPENCL_FUNCTION_NAME_BARRIER)) continue;
				hasBarriers = true;
			}
		}

		llvm::Function* f_wrapper = NULL;

		if (!hasBarriers) {

			// no barrier inside function

			// Generate wrapper for kernel (= all kernels have same signature)
			// Make sure the call to the original kernel is inlined after this!
			//
			std::stringstream strs2;
			strs2 << kernel_name << "_wrapper";
			const std::string wrapper_name = strs2.str();

			WFVOPENCL_DEBUG( outs() << "  generating kernel wrapper... "; );
			const bool inlineCall = false; // don't inline call immediately (needed for generating loop(s))
			f_wrapper = WFVOpenCL::generateKernelWrapper(wrapper_name, f, module, targetData, inlineCall);
			if (!f_wrapper) {
				errs() << "FAILED!\nERROR: wrapper generation for kernel module failed!\n";
				*errcode_ret = CL_INVALID_PROGRAM_EXECUTABLE; //sth like that :p
				return NULL;
			}
			WFVOPENCL_DEBUG( outs() << "done.\n"; );
			WFVOPENCL_DEBUG( WFVOpenCL::writeFunctionToFile(f_wrapper, "debug_arg_wrapper.ll"); );
			WFVOPENCL_DEBUG( verifyModule(*module); );

			// generate loop(s) over blocksize(s) (BEFORE inlining!)
			CallInst* kernelCall = getWrappedKernelCall(f_wrapper, f);
			generateBlockSizeLoopsForWrapper(f_wrapper, kernelCall, num_dimensions, simd_dim, context, module);

		} else {
			// minimize number of live values before splitting
			replaceCallbackUsesByNewCallsInFunction(module->getFunction("get_global_id"), f);
			replaceCallbackUsesByNewCallsInFunction(module->getFunction("get_local_id"), f);
			replaceCallbackUsesByNewCallsInFunction(module->getFunction("get_num_groups"), f);
			replaceCallbackUsesByNewCallsInFunction(module->getFunction("get_work_dim"), f);
			replaceCallbackUsesByNewCallsInFunction(module->getFunction("get_global_size"), f);
			replaceCallbackUsesByNewCallsInFunction(module->getFunction("get_local_size"), f);
			replaceCallbackUsesByNewCallsInFunction(module->getFunction("get_group_id"), f);

			WFVOPENCL_DEBUG( verifyFunction(*f); );

			// eliminate barriers
			FunctionPassManager FPM(module);

			CallSiteBlockSplitter* CSBS = new CallSiteBlockSplitter(WFVOPENCL_FUNCTION_NAME_BARRIER);
			LivenessAnalyzer* LA = new LivenessAnalyzer(true);
			ContinuationGenerator* CG = new ContinuationGenerator(true);

			// set "special" parameter types that are generated for each continuation
			// order is important (has to match mapCallbacksToContinuationArguments())!
			CG->addSpecialParam(Type::getInt32PtrTy(context, 0), "get_global_id");   // generated inside switch (group_id * loc_size + loc_id)
			CG->addSpecialParam(Type::getInt32PtrTy(context, 0), "get_local_id");    // generated inside switch (loop induction variables)
			CG->addSpecialParam(Type::getInt32PtrTy(context, 0), "get_num_groups");  // generated inside switch (glob_size / loc_size)
			CG->addSpecialParam(Type::getInt32Ty(context),       "get_work_dim");    // supplied from outside
			CG->addSpecialParam(Type::getInt32PtrTy(context, 0), "get_global_size"); // supplied from outside
			CG->addSpecialParam(Type::getInt32PtrTy(context, 0), "get_local_size");  // supplied from outside
			CG->addSpecialParam(Type::getInt32PtrTy(context, 0), "get_group_id");    // supplied from outside

			FPM.add(CSBS);
			FPM.add(LA);
			FPM.add(CG);

			FPM.run(*f);

			Function* barrierFreeFunction = CG->getBarrierFreeFunction();

			// NOTE: We must not optimize or inline anything yet,
			// the wrapper is required as generated for loop generation!

			WFVOPENCL_DEBUG( outs() << *barrierFreeFunction << "\n"; );
			WFVOPENCL_DEBUG( verifyFunction(*barrierFreeFunction); );

			f->replaceAllUsesWith(barrierFreeFunction);
			barrierFreeFunction->takeName(f);
			f->setName(barrierFreeFunction->getNameStr()+"_orig");

			f = barrierFreeFunction;

			WFVOPENCL_DEBUG( WFVOpenCL::writeModuleToFile(module, "debug_barrier_wrapper.mod.ll"); );
			
			ContinuationGenerator::ContinuationVecType continuations;
			CG->getContinuations(continuations);

			WFVOPENCL_DEBUG(
				outs() << "continuations:\n";
				for (SmallVector<Function*, 4>::iterator it=continuations.begin(), E=continuations.end(); it!=E; ++it) {
					Function* continuation = *it;
					outs() << " * " << continuation->getNameStr() << "\n";
				}
				outs() << "\n";
			);


			strs << "_wrapper";
			const std::string wrapper_name = strs.str();

			WFVOPENCL_DEBUG( outs() << "  generating kernel wrapper... "; );
			const bool inlineCall = true; // inline call immediately (and only this call)
			f_wrapper = WFVOpenCL::generateKernelWrapper(wrapper_name, f, module, targetData, inlineCall);
			if (!f_wrapper) {
				errs() << "FAILED!\nERROR: wrapper generation for kernel module failed!\n";
				*errcode_ret = CL_INVALID_PROGRAM_EXECUTABLE; //sth like that :p
				return NULL;
			}
			WFVOPENCL_DEBUG( outs() << "done.\n"; );
			WFVOPENCL_DEBUG( WFVOpenCL::writeFunctionToFile(f_wrapper, "debug_wrapper.ll"); );
			WFVOPENCL_DEBUG( verifyModule(*module); );

			// - callbacks inside continuations have to be replaced by argument accesses
			WFVOpenCL::mapCallbacksToContinuationArguments(num_dimensions, context, module, continuations);

			// - generate loops
			// - generate code for 3 generated special parameters in each loop
			// - map "special" arguments of calls to each continuation correctly (either to wrapper-param or to generated value inside loop)
			// - make liveValueUnion an array of unions (size: blocksize[0]*blocksize[1]*blocksize[2]*...)
			WFVOpenCL::generateBlockSizeLoopsForContinuations(num_dimensions, simd_dim, context, f_wrapper, continuations);

		}

		assert (f_wrapper);

		// optimize wrapper with inlined kernel
		WFVOPENCL_DEBUG( WFVOpenCL::writeFunctionToFile(f_wrapper, "debug_wrapper_beforeopt.ll"); );
		WFVOPENCL_DEBUG( outs() << "optimizing wrapper... "; );
		WFVOpenCL::inlineFunctionCalls(f_wrapper, targetData);
		WFVOpenCL::optimizeFunction(f_wrapper);
		WFVOPENCL_DEBUG( WFVOpenCL::writeFunctionToFile(f_wrapper, "debug_wrapper_afteropt.ll"); );
		
		WFVOPENCL_DEBUG_RUNTIME(
			for (Function::iterator BB=f_wrapper->begin(), BBE=f_wrapper->end(); BB!=BBE; ++BB) {
				for (BasicBlock::iterator I=BB->begin(), IE=BB->end(); I!=IE; ++I) {
					if (!isa<StoreInst>(I)) continue;
					insertPrintf("  stored return value: ", I->getOperand(0), true, BB->getTerminator());
					//insertPrintf("    store-address: ", I->getOperand(1), true, I);
				}
			}
			for (Function::iterator BB=f_wrapper->begin(), BBE=f_wrapper->end(); BB!=BBE; ++BB) {
				for (BasicBlock::iterator I=BB->begin(), IE=BB->end(); I!=IE; ++I) {
					if (!isa<LoadInst>(I)) continue;
					insertPrintf("  loaded value: ", I, true, BB->getTerminator());
					//insertPrintf("    load-address: ", I->getOperand(0), true, I);
				}
			}
		);

		WFVOPENCL_DEBUG_RUNTIME(
			for (Function::iterator BB=f_wrapper->begin(), BBE=f_wrapper->end(); BB!=BBE; ++BB) {
				for (BasicBlock::iterator I=BB->begin(), IE=BB->end(); I!=IE; ++I) {
					if (I->getName().equals("indvar")) {
						insertPrintf("  indvar: ", I, true, BB->getTerminator());
						continue;
					}
					if (I->getName().equals("indvar.next")) {
						insertPrintf("  indvar.next: ", I, true, BB->getTerminator());
						continue;
					}
					if (I->getName().equals("local_id_01")) {
						insertPrintf("  local_id_01: ", I, true, BB->getTerminator());
						continue;
					}
					if (I->getName().equals("global_id_04")) {
						insertPrintf("  global_id_04: ", I, true, BB->getTerminator());
						continue;
					}
					if (I->getName().equals("inc2")) {
						insertPrintf("  inc2: ", I, true, BB->getTerminator());
						continue;
					}
				}
			}
		);

		WFVOPENCL_DEBUG( outs() << "done.\n" << *f_wrapper << "\n"; );
		WFVOPENCL_DEBUG( verifyModule(*module); );
		WFVOPENCL_DEBUG( WFVOpenCL::writeFunctionToFile(f_wrapper, "debug_kernel_wrapped_final.ll"); );
		WFVOPENCL_DEBUG( WFVOpenCL::writeModuleToFile(module, "debug_kernel_wrapped_final.mod.ll"); );


#ifndef WFVOPENCL_NO_WFV
		// if vectorization is enabled, we "return" the SIMD function as well
		if (vectorized) {
			*f_SIMD_ret = f;
		}
#endif
		return f_wrapper;
	}


	cl_uint convertLLVMAddressSpace(cl_uint llvm_address_space) {
		switch (llvm_address_space) {
			case 0 : return CL_PRIVATE;
			case 1 : return CL_GLOBAL;
			case 3 : return CL_LOCAL;
			default : return llvm_address_space;
		}
	}
	std::string getAddressSpaceString(cl_uint cl_address_space) {
		switch (cl_address_space) {
			case CL_GLOBAL: return "CL_GLOBAL";
			case CL_PRIVATE: return "CL_PRIVATE";
			case CL_LOCAL: return "CL_LOCAL";
			case CL_CONSTANT: return "CL_CONSTANT";
			default: return "";
		}
	}



	//------------------------------------------------------------------------//
	// host information
	//------------------------------------------------------------------------//

	// TODO: get real info :p
	unsigned long long getDeviceMaxMemAllocSize() {
		//return 0x3B9ACA00; // 1 GB
		return 0xEE6B2800; // 4 GB
	}

}

#ifdef __cplusplus
}
#endif


