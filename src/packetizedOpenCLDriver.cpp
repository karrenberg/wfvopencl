/**
 * @file   packetizedOpenCLDriver.cpp
 * @date   14.04.2010
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
#include <assert.h>
#include <sstream>  // std::stringstream
#include <string.h> // memcpy

#include <xmmintrin.h> // test output etc.

#ifdef __APPLE__
#include <OpenCL/cl_platform.h>
#include <OpenCL/cl.h>
#else
#include <CL/cl_platform.h> // e.g. for CL_API_ENTRY
#include <CL/cl.h>          // e.g. for cl_platform_id
#endif

#include "Packetizer/api.h" // packetizer
#include "llvmTools.hpp" // all LLVM functionality

#include "livenessAnalyzer.h"
#include "continuationGenerator.h"

//----------------------------------------------------------------------------//
// Configuration
//----------------------------------------------------------------------------//
#define PACKETIZED_OPENCL_DRIVER_VERSION_STRING "0.1" // <major_number>.<minor_number>

#define PACKETIZED_OPENCL_DRIVER_EXTENSIONS "cl_khr_icd cl_amd_fp64 cl_khr_global_int32_base_atomics cl_khr_global_int32_extended_atomics cl_khr_local_int32_base_atomics cl_khr_local_int32_extended_atomics cl_khr_int64_base_atomics cl_khr_int64_extended_atomics cl_khr_byte_addressable_store cl_khr_gl_sharing cl_ext_device_fission cl_amd_device_attribute_query cl_amd_printf"
#define PACKETIZED_OPENCL_DRIVER_LLVM_DATA_LAYOUT_64 "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-f80:128:128"
#define PACKETIZED_OPENCL_DRIVER_MAX_WORK_GROUP_SIZE 8192
#define PACKETIZED_OPENCL_DRIVER_MAX_NUM_DIMENSIONS 3
#define PACKETIZED_OPENCL_DRIVER_SIMD_WIDTH 4
#ifdef PACKETIZED_OPENCL_DRIVER_USE_OPENMP
	#define PACKETIZED_OPENCL_DRIVER_NUM_CORES 4 // TODO: determine somehow, omp_get_num_threads() is dynamic (=1 here)
#else
	#define PACKETIZED_OPENCL_DRIVER_NUM_CORES 1
#endif
#define PACKETIZED_OPENCL_DRIVER_MAX_NUM_THREADS PACKETIZED_OPENCL_DRIVER_NUM_CORES



// these are assumed to be set by build script
//#define PACKETIZED_OPENCL_DRIVER_NO_PACKETIZATION
//#define PACKETIZED_OPENCL_DRIVER_USE_OPENMP
//#define PACKETIZED_OPENCL_DRIVER_SPLIT_EVERYTHING
//#define NDEBUG
//----------------------------------------------------------------------------//


#ifdef PACKETIZED_OPENCL_DRIVER_USE_OPENMP
#include <omp.h>
#endif

#ifdef DEBUG
#define PACKETIZED_OPENCL_DRIVER_DEBUG(x) do { x } while (false)
#else
#define PACKETIZED_OPENCL_DRIVER_DEBUG(x) ((void)0)
#endif

#ifdef DEBUG_RUNTIME
#define PACKETIZED_OPENCL_DRIVER_DEBUG_RUNTIME(x) do { x } while (false)
#else
#define PACKETIZED_OPENCL_DRIVER_DEBUG_RUNTIME(x) ((void)0)
#endif

#ifdef NDEBUG // force debug output disabled
#undef PACKETIZED_OPENCL_DRIVER_DEBUG
#define PACKETIZED_OPENCL_DRIVER_DEBUG(x) ((void)0)
#define PACKETIZED_OPENCL_DRIVER_DEBUG_RUNTIME(x) ((void)0)
#endif


//----------------------------------------------------------------------------//
// Tools
//----------------------------------------------------------------------------//

template<typename T, typename U> T ptr_cast(U* p) {
	return reinterpret_cast<T>(reinterpret_cast<size_t>(p));
}

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


#define CL_CONSTANT 0x3 // does not exist in specification 1.0
#define CL_PRIVATE 0x4 // does not exist in specification 1.0

///////////////////////////////////////////////////////////////////////////
//                 OpenCL Runtime Implementation                         //
///////////////////////////////////////////////////////////////////////////

#if 0
	cl_uint dimensions;


	// packetized implementation
	//
	cl_uint simdDimension;

	inline __m128i get_global_id_SIMD(cl_uint D) {
		assert (D < dimensions);
#ifdef PACKETIZED_OPENCL_DRIVER_USE_OPENMP
		const unsigned thread = omp_get_thread_num();
		assert (thread < PACKETIZED_OPENCL_DRIVER_MAX_NUM_THREADS);
		const size_t simd_id = currentGlobal[thread][D];
#else
		const size_t simd_id = currentGlobal[D];
#endif
		const unsigned id0 = simd_id * 4;
		const unsigned id1 = id0 + 1;
		const unsigned id2 = id0 + 2;
		const unsigned id3 = id0 + 3;
		return _mm_set_epi32(id3, id2, id1, id0);
	}
	inline __m128i get_local_id_SIMD(cl_uint D) {
		assert (D < dimensions);
#ifdef PACKETIZED_OPENCL_DRIVER_USE_OPENMP
		const unsigned thread = omp_get_thread_num();
		assert (thread < PACKETIZED_OPENCL_DRIVER_MAX_NUM_THREADS);
		return currentLocal[thread][D];
#else
		return currentLocal[D];
#endif
	}

	inline void setCurrentLocal(cl_uint D, __m128i id) {
		assert (D < dimensions);
		assert (((unsigned*)&id)[0] < get_local_size(D));
		assert (((unsigned*)&id)[1] < get_local_size(D));
		assert (((unsigned*)&id)[2] < get_local_size(D));
		assert (((unsigned*)&id)[3] < get_local_size(D));
#ifdef PACKETIZED_OPENCL_DRIVER_USE_OPENMP
		const unsigned thread = omp_get_thread_num();
		assert (thread < PACKETIZED_OPENCL_DRIVER_MAX_NUM_THREADS);
		currentLocal[thread][D] = id;
#else
		currentLocal[D] = id;
#endif
	}

	// called automatically by initializeOpenCL
	inline cl_uint initializeThreads(const size_t* gThreads, const size_t* lThreads) {
		
		// set up global/local thread numbers
		for (cl_uint i=0; i<dimensions; ++i) {
			PACKETIZED_OPENCL_DRIVER_DEBUG(
				if (lThreads[i] > gThreads[i]) {
					errs() << "WARNING: local work size is larger than global work size for dimension " << i << "!\n";
				}
			);

			const size_t globalThreadsDimI = gThreads[i];
			const size_t localThreadsDimI = lThreads[i] < PACKETIZED_OPENCL_DRIVER_SIMD_WIDTH ? PACKETIZED_OPENCL_DRIVER_SIMD_WIDTH : lThreads[i];

			globalThreads[i] = globalThreadsDimI;
			localThreads[i] = localThreadsDimI;
		}

		// safety checks
		PACKETIZED_OPENCL_DRIVER_DEBUG(
			size_t globalThreadNum = 0;
			size_t localThreadNum = 0;
			bool* alignedGlobalDims = new bool[dimensions]();
			bool* alignedLocalDims = new bool[dimensions]();

			bool error = false;
			for (cl_uint i=0; i<dimensions; ++i) {
				const size_t globalThreadsDimI = gThreads[i];
				const size_t localThreadsDimI = lThreads[i] < PACKETIZED_OPENCL_DRIVER_SIMD_WIDTH ? PACKETIZED_OPENCL_DRIVER_SIMD_WIDTH : lThreads[i];

				globalThreadNum += globalThreadsDimI;
				alignedGlobalDims[i] = (globalThreadsDimI % PACKETIZED_OPENCL_DRIVER_SIMD_WIDTH == 0);

				localThreadNum += localThreadsDimI;
				alignedLocalDims[i] = (localThreadsDimI % PACKETIZED_OPENCL_DRIVER_SIMD_WIDTH == 0);

				if (lThreads[i] > PACKETIZED_OPENCL_DRIVER_SIMD_WIDTH) errs() << "WARNING: local work size (" << lThreads[i] << ") is larger than " << PACKETIZED_OPENCL_DRIVER_SIMD_WIDTH << "!\n";
				if (lThreads[i] < PACKETIZED_OPENCL_DRIVER_SIMD_WIDTH) {
					// TODO: fall back to scalar mode instead!
					errs() << "WARNING: local work size enlarged from " << lThreads[i] << " to " << PACKETIZED_OPENCL_DRIVER_SIMD_WIDTH << "!\n";
				}


				if (i == simdDimension && !alignedGlobalDims[i]) {
					errs() << "ERROR: size of chosen SIMD dimension " << i
							<< " is globally not dividable by " << PACKETIZED_OPENCL_DRIVER_SIMD_WIDTH
							<< " (global dimension)!\n";
					error = true;
				}
				if (i == simdDimension && !alignedLocalDims[i]) {
					errs() << "ERROR: size of chosen SIMD dimension " << i
							<< " is locally not dividable by " << PACKETIZED_OPENCL_DRIVER_SIMD_WIDTH
							<< " (work-group dimension)!\n";
					error = true;
				}
				if (globalThreadsDimI % localThreadsDimI != 0) {
					errs() << "ERROR: size of global dimension " << i
							<< " not dividable by local dimension ("
							<< globalThreadsDimI << " / " << localThreadsDimI
							<< ")!\n";
					error = true;
				}
			}

			if (globalThreadNum % PACKETIZED_OPENCL_DRIVER_SIMD_WIDTH != 0) {
				errs() << "ERROR: global number of threads is not dividable by "
						<< PACKETIZED_OPENCL_DRIVER_SIMD_WIDTH << "!\n";
				error = true;
			}
			if (localThreadNum % PACKETIZED_OPENCL_DRIVER_SIMD_WIDTH != 0) {
				errs() << "ERROR: number of threads in a group is not dividable by "
						<< PACKETIZED_OPENCL_DRIVER_SIMD_WIDTH << "!\n";
				error = true;
			}

			delete [] alignedGlobalDims;
			delete [] alignedLocalDims;

			if (error) return CL_INVALID_GLOBAL_WORK_SIZE;
		);

		return CL_SUCCESS;
	}
	// simdDim ranges from 0 to num_dims-1 !
	cl_uint initializeOpenCL(const cl_uint num_dims, const cl_uint simdDim, const size_t* gThreads, const size_t* lThreads) {
		// print some information
		PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "\nAutomatic Packetization enabled!\n"; );

		dimensions = num_dims;
		simdDimension = simdDim;

		globalThreads = new size_t[num_dims]();
		localThreads = new size_t[num_dims]();

#ifdef PACKETIZED_OPENCL_DRIVER_USE_OPENMP
		PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "OpenMP enabled!\n"; );
		currentGlobal = new size_t*[PACKETIZED_OPENCL_DRIVER_MAX_NUM_THREADS]();
		currentLocal = new __m128i*[PACKETIZED_OPENCL_DRIVER_MAX_NUM_THREADS]();
		currentGroup = new size_t*[PACKETIZED_OPENCL_DRIVER_MAX_NUM_THREADS]();

		for (cl_uint i=0; i<PACKETIZED_OPENCL_DRIVER_MAX_NUM_THREADS; ++i) {
			currentGlobal[i] = new size_t[num_dims]();
			currentLocal[i] = new __m128i[num_dims]();
			currentGroup[i] = new size_t[num_dims]();

			for (cl_uint j=0; j<num_dims; ++j) {
				if (j == simdDimension) {
					currentGlobal[i][j] = 0;
					currentLocal[i][j] = _mm_set_epi32(0, 1, 2, 3);
				} else {
					currentGlobal[i][j] = 0;
					currentLocal[i][j] = _mm_set_epi32(0, 0, 0, 0);
				}
				currentGroup[i][j] = 0;
			}
		}
#else
		PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "OpenMP disabled!\n"; );
		currentGlobal = new size_t[num_dims]();
		currentLocal = new __m128i[num_dims]();
		currentGroup = new size_t[num_dims]();

		for (cl_uint i=0; i<num_dims; ++i) {
			if (i == simdDimension) {
				currentGlobal[i] = 0;
				currentLocal[i] = _mm_set_epi32(3, 2, 1, 0);
			} else {
				currentGlobal[i] = 0;
				currentLocal[i] = _mm_set_epi32(0, 0, 0, 0);
			}
			currentGroup[i] = 0;
		}
#endif

		return initializeThreads(gThreads, lThreads);
	}
#endif


// common functionality, with or without packetization/openmp
namespace PacketizedOpenCLDriver {

#ifndef PACKETIZED_OPENCL_DRIVER_NO_PACKETIZATION
		bool packetizeKernelFunction(
			const std::string& kernelName,
			const std::string& targetKernelName,
			llvm::Module* mod,
			const cl_uint packetizationSize,
			const bool use_sse41,
			const bool verbose)
	{
		if (!PacketizedOpenCLDriver::getFunction(kernelName, mod)) {
			errs() << "ERROR: source function '" << kernelName
					<< "' not found in module!\n";
			return false;
		}
		if (!PacketizedOpenCLDriver::getFunction(targetKernelName, mod)) {
			errs() << "ERROR: target function '" << targetKernelName
					<< "' not found in module!\n";
			return false;
		}

		Packetizer::Packetizer* packetizer = Packetizer::getPacketizer(use_sse41, verbose);
		Packetizer::addFunctionToPacketizer(
				packetizer,
				kernelName,
				targetKernelName,
				packetizationSize);

		// replace using this scheme:
		// get_global_id       -> non-simd-dim, considered uniform, leave untouched
		// get_global_id_split -> non-linear access, considered varying, replace by specially named packet function for later fixing
		// get_global_id_SIMD  -> linear access, considered varying, replace by itself to load vector from single index
		// see generateOpenCLFunctions() for more details
		Packetizer::addNativeFunctionToPacketizer(
				packetizer,
				"get_global_id_split",
				-1,
				PacketizedOpenCLDriver::getFunction("get_global_id_split_SIMD", mod),
				true); // packetization is mandatory
		Packetizer::addNativeFunctionToPacketizer(
				packetizer,
				"get_global_id_SIMD",
				-1,
				PacketizedOpenCLDriver::getFunction("get_global_id_SIMD", mod),
				true); // packetization is mandatory

		Packetizer::addNativeFunctionToPacketizer(
				packetizer,
				"get_local_id_split",
				-1,
				PacketizedOpenCLDriver::getFunction("get_local_id_split_SIMD", mod),
				true); // packetization is mandatory
		Packetizer::addNativeFunctionToPacketizer(
				packetizer,
				"get_local_id_SIMD",
				-1,
				PacketizedOpenCLDriver::getFunction("get_local_id_SIMD", mod),
				true); // packetization is mandatory

		Packetizer::runPacketizer(packetizer, mod);

		if (!PacketizedOpenCLDriver::getFunction(targetKernelName, mod)) {
			errs() << "ERROR: packetized target function not found in module!\n";
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
			default                : str = "%x"; break;
			case Type::IntegerTyID : str = "%d"; break;
			case Type::FloatTyID   : str = "%f"; break;
			case Type::PointerTyID : str = "%x"; break;
			case Type::VectorTyID  : {
				switch (value->getType()->getContainedType(0)->getTypeID()) {
					default                : str = "%x %x %x %x"; break;
					case Type::IntegerTyID : str = "%d %d %d %d"; break;
					case Type::FloatTyID   : str = "%f %f %f %f"; break;
				}
				break;
			}
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


	void replaceCallbacksByArgAccess(Function* f, Value* arg, Function* source) {
		if (!f) return;
		assert (arg && source);

		PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "\nreplaceCallbacksByArgAccess(" << f->getNameStr() << ", " << *arg << ", " << source->getName() << ")\n"; );

		const bool isArrayArg = isa<ArrayType>(arg->getType());
		const bool isPointerArg = isa<PointerType>(arg->getType());

		for (Function::use_iterator U=f->use_begin(), UE=f->use_end(); U!=UE; ) {
			if (!isa<CallInst>(U)) continue;
			CallInst* call = cast<CallInst>(U++);
			PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "replacing use: " << *call << "\n"; );

			if (call->getParent()->getParent() != source) {
				PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "  is in different function: " << call->getParent()->getParent()->getNameStr() << "\n"; );
				continue; // ignore uses in other functions
			}

			// if arg type is an array, check second operand of call (= supplied parameter)
			// and generate appropriate ExtractValueInst
			if (isArrayArg) {
				PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "  array arg found!\n"; );
				const Value* dimVal = call->getOperand(1);
				assert (isa<ConstantInt>(dimVal));
				const ConstantInt* dimConst = cast<ConstantInt>(dimVal);
				const uint64_t* dimension = dimConst->getValue().getRawData();
				ExtractValueInst* ev = ExtractValueInst::Create(arg, *dimension, "", call);
				PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "  new extract: " << *ev << "\n"; );
				
				// if the result is a 64bit integer value, truncate to 32bit -> more other problems :/
				//if (ev->getType() == f->getReturnType()) arg = ev;
				//else arg = TruncInst::CreateTruncOrBitCast(ev, f->getReturnType(), "", call);
				//outs() << "  new extract/cast: " << *arg << "\n";

				assert (f->getReturnType() == ev->getType());
				call->replaceAllUsesWith(ev);
				call->eraseFromParent();
			} else if (isPointerArg) {
				PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "  pointer arg found!\n"; );
				Value* dimVal = call->getOperand(1);
				PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "  dimVal: " << *dimVal << "\n"; );
				PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "  arg: " << *arg << "\n"; );
				GetElementPtrInst* gep = GetElementPtrInst::Create(arg, dimVal, "", call);
				LoadInst* load = new LoadInst(gep, "", false, 16, call);
				PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "  new gep: " << *gep << "\n"; );
				PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "  new load: " << *load << "\n"; );

				assert (f->getReturnType() == load->getType());
				call->replaceAllUsesWith(load);
				call->eraseFromParent();
			} else {
				PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "  normal arg found!\n"; );
				PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "  arg: " << *arg << "\n"; );
				assert (f->getReturnType() == arg->getType());
				call->replaceAllUsesWith(arg);
				call->eraseFromParent();
			}

		} // for each use

	}

	inline llvm::Function* generateKernelWrapper(
			const std::string& wrapper_name,
			llvm::Function* f,
			llvm::Module* mod,
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
		llvm::Function* wrapper = PacketizedOpenCLDriver::generateFunctionWrapperWithParams(wrapper_name, f, mod, additionalParams, inlineCall);
		if (!wrapper) return  NULL;

		// set argument names and attributes
		Function::arg_iterator arg = wrapper->arg_begin();
		++arg; arg->setName("get_work_dim");
		++arg; arg->setName("get_global_size");
		++arg; arg->setName("get_local_size");
		++arg; arg->setName("get_group_id");

		return wrapper;
	}

	inline CallInst* getWrappedKernelCall(Function* wrapper, Function* kernel) {
		for (Function::use_iterator U=kernel->use_begin(), UE=kernel->use_end(); U!=UE; ++U) {
			if (!isa<CallInst>(U)) continue;
			CallInst* call = cast<CallInst>(U);
			if (call->getParent()->getParent() == wrapper) return call;
		}
		assert (false && "could not find call to kernel - inlined already?");
		return NULL;
	}

	inline void fixFunctionNames(Module* mod) {
		assert (mod);
		// fix __sqrt_f32
		if (PacketizedOpenCLDriver::getFunction("__sqrt_f32", mod)) {

			// create llvm.sqrt.f32 intrinsic
			const llvm::Type* floatType = PacketizedOpenCLDriver::getTypeFromString(mod, "f");
			std::vector<const llvm::Type*> params;
			params.push_back(floatType);
			PacketizedOpenCLDriver::createExternalFunction("llvm.sqrt.f32", floatType, params, mod);
			assert (PacketizedOpenCLDriver::getFunction("llvm.sqrt.f32", mod));

			PacketizedOpenCLDriver::replaceAllUsesWith(PacketizedOpenCLDriver::getFunction("__sqrt_f32", mod), PacketizedOpenCLDriver::getFunction("llvm.sqrt.f32", mod));
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
					const Value* dimVal = call->getOperand(1);
					assert (isa<ConstantInt>(dimVal));
					const ConstantInt* dimConst = cast<ConstantInt>(dimVal);
					const uint64_t dimension = *dimConst->getValue().getRawData() +1; // uses count from 0, max_dim from 1
					
					assert (dimension <= PACKETIZED_OPENCL_DRIVER_MAX_NUM_DIMENSIONS);
					if (dimension > max_dim) max_dim = dimension;
				}
			}
		}
		PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "\nnumber of dimensions used in kernel: " << max_dim << "\n"; );
		return max_dim;
	}

	// generate computation of "flattened" local id
	// this is required to access the correct live value struct of each thread
	// (all dimension's threads of the block are stored flattened in memory)
	// if iterating 'for all dim0 { for all dim1 { for all dim2 { ... } } }' :
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
		
		PACKETIZED_OPENCL_DRIVER_DEBUG_RUNTIME( insertPrintf("\ncontinuation ", ConstantInt::get(newCall->getContext(), APInt(32, continuation_id)), true, callBB->getFirstNonPHI()); );
		
		// adjust GEP-instructions to point to current localID's live value struct,
		// e.g. GEP liveValueUnion, i32 0, i32 elementindex
		// ---> GEP liveValueUnion, i32 local_id_flat, i32 elementindex
		// TODO: move this to a new function
		Value* liveValueStruct = newCall->getOperand(newCall->getNumOperands()-1); // live value union is last parameter to call
		PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "live value struct: " << *liveValueStruct << "\n"; );

		// now get the bitcast-use of the union in this same block
		BitCastInst* liveValueStructBc = NULL;
		for (Value::use_iterator U=liveValueStruct->use_begin(), UE=liveValueStruct->use_end(); U!=UE; ++U) {
			if (!isa<BitCastInst>(U)) continue;
			BitCastInst* bc = cast<BitCastInst>(U);
			if (bc->getParent() != callBB) continue;
			
			liveValueStructBc = bc;
			break; // there is exactly one use of interest
		}
		assert (liveValueStructBc);
		
		// Uses of this bitcast are the GEPs for the load operations that extract the live values.
		// Replace the first index of each GEP (=0 for pointer-step-through in the standard continuation case)
		// by the correct local index.
		for (BitCastInst::use_iterator U=liveValueStructBc->use_begin(), UE=liveValueStructBc->use_end(); U!=UE; ) {
			if (!isa<GetElementPtrInst>(U)) { ++U; continue; }
			GetElementPtrInst* gep = cast<GetElementPtrInst>(U++);
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
			
			PACKETIZED_OPENCL_DRIVER_DEBUG_RUNTIME(
				assert (newGEP->getNumUses() == 1);
				Value* gepUse = newGEP->use_back();
				insertPrintf("live value loaded: ", gepUse, true, newCall);
			);
		}
	}
	void adjustLiveValueStoreGEPs(Function* continuation, const unsigned num_dimensions, LLVMContext& context) {
		assert (continuation);
		PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "\nadjustLiveValueStoreGEPs(" << continuation->getNameStr() << ")\n"; );
		// get the live value union (= last parameter of function)
		Value* liveValueStruct = --(continuation->arg_end());
		PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "live value struct: " << *liveValueStruct << "\n"; );
		if (liveValueStruct->use_empty()) {
			PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "  has no uses -> no adjustment necessary!\n"; );
			return;
		}

		assert (!liveValueStruct->use_empty());

		// load local_ids and local_sizes for the next computation
		Argument* arg_local_id_array = ++continuation->arg_begin(); // 2nd argument
		Function::arg_iterator tmpA = continuation->arg_begin();
#ifdef PACKETIZED_OPENCL_DRIVER_NO_PACKETIZATION
		std::advance(tmpA, 5); // 5th argument
		Argument* arg_local_size_array = tmpA;
#else
		std::advance(tmpA, 7); // 7th argument
		Argument* arg_local_size_array = tmpA;
#endif

		Instruction** local_ids = new Instruction*[num_dimensions]();
		Instruction** local_sizes = new Instruction*[num_dimensions]();


		// the only use of this argument we are interested in is a bitcast to
		// the next continuation's live value struct type in the same block as
		// the call to the barrier
		// TODO: we simply transform *all* uses because it would be more work to
		// make all necessary information (barrier-call) available here.
		for (Value::use_iterator U=liveValueStruct->use_begin(), UE=liveValueStruct->use_end(); U!=UE; ++U) {
			assert (isa<BitCastInst>(U));
			BitCastInst* liveValueStructBc = cast<BitCastInst>(U);

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
				if (!isa<GetElementPtrInst>(U)) { ++U; continue; }
				GetElementPtrInst* gep = cast<GetElementPtrInst>(U++);
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

				PACKETIZED_OPENCL_DRIVER_DEBUG_RUNTIME(
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
			PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "\nmapping callbacks to arguments in continuation '" << continuation->getNameStr() << "'...\n"; );

			// correct order is important! (has to match parameter list of continuation)
			llvm::Function::arg_iterator arg = continuation->arg_begin();
#ifdef PACKETIZED_OPENCL_DRIVER_NO_PACKETIZATION
			PacketizedOpenCLDriver::replaceCallbacksByArgAccess(module->getFunction("get_global_id"),      cast<Value>(arg++), continuation);
			PacketizedOpenCLDriver::replaceCallbacksByArgAccess(module->getFunction("get_local_id"),       cast<Value>(arg++), continuation);
			PacketizedOpenCLDriver::replaceCallbacksByArgAccess(module->getFunction("get_num_groups"),     cast<Value>(arg++), continuation);
			PacketizedOpenCLDriver::replaceCallbacksByArgAccess(module->getFunction("get_work_dim"),       cast<Value>(arg++), continuation);
			PacketizedOpenCLDriver::replaceCallbacksByArgAccess(module->getFunction("get_global_size"),    cast<Value>(arg++), continuation);
			PacketizedOpenCLDriver::replaceCallbacksByArgAccess(module->getFunction("get_local_size"),     cast<Value>(arg++), continuation);
			PacketizedOpenCLDriver::replaceCallbacksByArgAccess(module->getFunction("get_group_id"),       cast<Value>(arg++), continuation);
#else
			// NOTE: _split functions can remain instead of being replaced by _SIMD function if only used on uniform path!
			PacketizedOpenCLDriver::replaceCallbacksByArgAccess(module->getFunction("get_global_id"),      cast<Value>(arg), continuation);
			PacketizedOpenCLDriver::replaceCallbacksByArgAccess(module->getFunction("get_global_id_SIMD"), cast<Value>(arg++), continuation);
			PacketizedOpenCLDriver::replaceCallbacksByArgAccess(module->getFunction("get_local_id"),       cast<Value>(arg), continuation);
			PacketizedOpenCLDriver::replaceCallbacksByArgAccess(module->getFunction("get_local_id_SIMD"),  cast<Value>(arg++), continuation);
			PacketizedOpenCLDriver::replaceCallbacksByArgAccess(module->getFunction("get_global_id_split_SIMD"), cast<Value>(arg++), continuation);
			PacketizedOpenCLDriver::replaceCallbacksByArgAccess(module->getFunction("get_local_id_split_SIMD"),  cast<Value>(arg++), continuation);
			PacketizedOpenCLDriver::replaceCallbacksByArgAccess(module->getFunction("get_num_groups"),     cast<Value>(arg++), continuation);
			PacketizedOpenCLDriver::replaceCallbacksByArgAccess(module->getFunction("get_work_dim"),       cast<Value>(arg++), continuation);
			PacketizedOpenCLDriver::replaceCallbacksByArgAccess(module->getFunction("get_global_size"),    cast<Value>(arg++), continuation);
			PacketizedOpenCLDriver::replaceCallbacksByArgAccess(module->getFunction("get_local_size"),     cast<Value>(arg++), continuation);
			PacketizedOpenCLDriver::replaceCallbacksByArgAccess(module->getFunction("get_group_id"),       cast<Value>(arg++), continuation);
#endif
		}

		PacketizedOpenCLDriver::fixFunctionNames(module);

		return;
	}
	inline void createGroupConstantSpecialParamLoads(
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
		for (unsigned i=0; i<num_dimensions; ++i) {
			Value* dimIdx = ConstantInt::get(context, APInt(32, i));
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
			// we need to make sure that num_groups always returns at least 1
			// TODO: can't we rely on global_sizes being dividable by local_sizes at this point?
			Instruction* div = BinaryOperator::Create(Instruction::UDiv, global_sizes[i], local_sizes[i], "", insertBefore);
			ICmpInst* cmp = new ICmpInst(insertBefore, ICmpInst::ICMP_EQ, div, ConstantInt::get(context, APInt(32, 0)), "");
			num_groupss[i] = SelectInst::Create(cmp, ConstantInt::get(context, APInt(32, 1)), div, sstr4.str(), insertBefore);

			PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "  global_sizes[" << i << "]: " << *(global_sizes[i]) << "\n"; );
			PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "  local_sizes[" << i << "] : " << *(local_sizes[i]) << "\n"; );
			PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "  group_ids[" << i << "]   : " << *(group_ids[i]) << "\n"; );
			PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "  num_groups[" << i << "]  : " << *(num_groupss[i]) << "\n"; );

			// store num_groups into array
			gep = GetElementPtrInst::Create(arg_num_groups_array, dimIdx, "", insertBefore);
			new StoreInst(num_groupss[i], gep, false, 16, insertBefore);
			//InsertValueInst::Create(arg_num_groups_array, num_groupss[i], i, "", insertBefore); // TODO: maybe later...

			PACKETIZED_OPENCL_DRIVER_DEBUG_RUNTIME(
				insertPrintf("i = ", dimIdx, true, insertBefore);
				insertPrintf("work_dim: ", arg_work_dim, true, insertBefore);
				insertPrintf("global_sizes[i]: ", global_sizes[i], true, insertBefore);
				insertPrintf("local_sizes[i]: ", local_sizes[i], true, insertBefore);
				insertPrintf("group_ids[i]: ", group_ids[i], true, insertBefore);
				insertPrintf("num_groups[i]: ", num_groupss[i], true, insertBefore);
			);
		}
	}
	inline void generateLoopsAroundCall(
			CallInst* call,
			const unsigned num_dimensions,
			const int simd_dim,
			Instruction** local_sizes,
			Instruction** group_ids,
			Value* arg_global_id_array,
			Value* arg_local_id_array,
			LLVMContext& context,
			Instruction** global_ids,
			Instruction** local_ids,
			Value** arg_global_id_split_simd=NULL,
			Value** arg_local_id_split_simd=NULL
	) {
		assert (call && local_sizes && group_ids && global_ids && local_ids);
#ifndef PACKETIZED_OPENCL_DRIVER_NO_PACKETIZATION
		assert (arg_global_id_split_simd && arg_local_id_split_simd);
#endif
		
		Function* f = call->getParent()->getParent();
		Instruction* insertBefore = call;

		for (int i=num_dimensions-1; i>=0; --i) {
			Value* local_size = local_sizes[i];
			Value* group_id = group_ids[i];

			// split parent before first instruction (all liveValueUnion-extraction code has to be inside loop)
			BasicBlock* headerBB = call->getParent(); // first iteration = tmpHeaderBB

			assert (headerBB->getUniquePredecessor());
			BasicBlock* entryBB = headerBB->getUniquePredecessor(); // tmpEntryBB -> header of current innermost loop
			BasicBlock* exitBB  = *succ_begin(headerBB); // tmpExitBB -> latch of current innermost loop

			BasicBlock* loopBB   = headerBB->splitBasicBlock(headerBB->begin(), headerBB->getNameStr()+".loop");
			BasicBlock* latchBB  = BasicBlock::Create(context, headerBB->getNameStr()+".loop.end", f, loopBB);

			// Block headerBB
			const Type* counterType = Type::getInt32Ty(context);
			Argument* fwdref = new Argument(counterType);
			PHINode* loopCounterPhi = PHINode::Create(counterType, "", headerBB->getFirstNonPHI());
			loopCounterPhi->reserveOperandSpace(2);
			loopCounterPhi->addIncoming(fwdref, latchBB);
			loopCounterPhi->addIncoming(Constant::getNullValue(counterType), entryBB);

			Instruction* local_id = loopCounterPhi;

#ifndef PACKETIZED_OPENCL_DRIVER_NO_PACKETIZATION
			ConstantInt* simdWidthVal = ConstantInt::get(context, APInt(32, PACKETIZED_OPENCL_DRIVER_SIMD_WIDTH));

			// reconstruct local ids of scalar instances and put into vector for scattered access
			const Type* local_id_type = NULL;
			if (i == simd_dim) local_id_type = VectorType::get(counterType, PACKETIZED_OPENCL_DRIVER_SIMD_WIDTH);
			else local_id_type = counterType;

			Instruction* local_id_split_SIMD = NULL;
			if (i == simd_dim) {
				Value* nextElem = BinaryOperator::Create(Instruction::Mul, loopCounterPhi, simdWidthVal, "", call); // name = local_id_simd_0
				local_id_split_SIMD = InsertElementInst::Create(UndefValue::get(local_id_type), nextElem, Constant::getNullValue(counterType), "", call);
				std::vector<Constant*> consts;
				consts.push_back(ConstantInt::get(context, APInt(32, 0)));
				for (unsigned j=1; j<PACKETIZED_OPENCL_DRIVER_SIMD_WIDTH; ++j) {
					local_id_split_SIMD = InsertElementInst::Create(local_id_split_SIMD, nextElem, ConstantInt::get(context, APInt(32, j)), "", call);
					consts.push_back(ConstantInt::get(context, APInt(32, j)));
				}
				// use vector-add with <0, 1, 2, 3> instead of 4 scalar additions
				Constant* addVec = ConstantVector::get(VectorType::get(counterType, PACKETIZED_OPENCL_DRIVER_SIMD_WIDTH), consts);
				local_id_split_SIMD = BinaryOperator::Create(Instruction::Add, local_id_split_SIMD, addVec, "", call);
				local_id_split_SIMD->setName("get_local_id_split_SIMD");
			}

			// compute number of iterations for the simd dim (= local_size / 4)
			//Value* local_size_orig = local_size;
			if (i == simd_dim) local_size = BinaryOperator::Create(Instruction::UDiv, local_size, simdWidthVal, "local_size_SIMD", call);
#endif
			
			// Block loopBB
			// holds live value extraction and continuation-call and, in case of packetization and the simd_dim, the local_size_simd computation
			loopBB->getTerminator()->eraseFromParent();
			BranchInst::Create(latchBB, loopBB);

			// Block latchBB
			BinaryOperator* loopCounterInc = BinaryOperator::Create(Instruction::Add, loopCounterPhi, ConstantInt::get(context, APInt(32, 1)), "inc", latchBB);
			ICmpInst* exitcond1 = new ICmpInst(*latchBB, ICmpInst::ICMP_EQ, loopCounterInc, local_size, "exitcond");
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
			
#ifdef PACKETIZED_OPENCL_DRIVER_NO_PACKETIZATION
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
			
			PACKETIZED_OPENCL_DRIVER_DEBUG_RUNTIME(
				insertPrintf("global_id[i]: ", global_ids[i], true, call);
				insertPrintf("local_id[i]: ", local_ids[i], true, call);
			);
#else
			// compute global id for consecutive access
			Instruction* global_id = BinaryOperator::Create(Instruction::Mul, group_id, local_size, "", call);
			global_id = BinaryOperator::Create(Instruction::Add, global_id, local_id, "", call);

			// save special parameters global_id, local_id to arrays
			GetElementPtrInst* gep = GetElementPtrInst::Create(arg_global_id_array, ConstantInt::get(context, APInt(32, i)), "", insertBefore);
			new StoreInst(global_id, gep, false, 16, call);
			gep = GetElementPtrInst::Create(arg_local_id_array, ConstantInt::get(context, APInt(32, i)), "", insertBefore);
			new StoreInst(local_id, gep, false, 16, call);

			// reconstruct global ids of scalar instances and put into vector for scattered access
			Instruction* global_id_split_SIMD = NULL;
			if (i == simd_dim) {
				// global_id_split_SIMD = global_id * 4 + <0, 1, 2, 3>
				Value* nextElem = BinaryOperator::Create(Instruction::Mul, global_id, ConstantInt::get(context, APInt(32, PACKETIZED_OPENCL_DRIVER_SIMD_WIDTH)), "", call);
				global_id_split_SIMD = InsertElementInst::Create(UndefValue::get(local_id_type), nextElem, Constant::getNullValue(counterType), "", call);
				std::vector<Constant*> consts;
				consts.push_back(ConstantInt::get(context, APInt(32, 0)));
				// replicate
				for (unsigned j=1; j<PACKETIZED_OPENCL_DRIVER_SIMD_WIDTH; ++j) {
					global_id_split_SIMD = InsertElementInst::Create(global_id_split_SIMD, nextElem, ConstantInt::get(context, APInt(32, j)), "", call);
					consts.push_back(ConstantInt::get(context, APInt(32, j)));
				}
				// use vector-add with <0, 1, 2, 3> instead of 4 scalar additions
				Constant* addVec = ConstantVector::get(VectorType::get(counterType, PACKETIZED_OPENCL_DRIVER_SIMD_WIDTH), consts);
				global_id_split_SIMD = BinaryOperator::Create(Instruction::Add, global_id_split_SIMD, addVec, "", call);
				global_id_split_SIMD->setName("get_global_id_split_SIMD");
			}

			// set names and store values
			if (i == simd_dim) {
				global_id->setName("get_global_id_SIMD");
				local_id->setName("get_local_id_SIMD");

				// global/local split simd ids are supplied as separate arguments (-> do not store to id arrays / can not because of vector type)
				*arg_global_id_split_simd = global_id_split_SIMD;
				*arg_local_id_split_simd = local_id_split_SIMD;

				global_ids[i] = global_id;
				local_ids[i] = local_id;

				PACKETIZED_OPENCL_DRIVER_DEBUG_RUNTIME(
					insertPrintf("global_id[i]: ", global_ids[i], true, call);
					insertPrintf("local_id[i]: ", local_ids[i], true, call);
					insertPrintf("global_id_split_simd: ", *arg_global_id_split_simd, true, call);
					insertPrintf("local_id_split_simd: ", *arg_local_id_split_simd, true, call);
				);
			} else {
				std::stringstream sstr2;
				sstr2 << "global_id_" << i;
				global_id->setName(sstr2.str());

				std::stringstream sstr;
				sstr << "local_id_" << i;
				local_id->setName(sstr.str());

				global_ids[i] = global_id; // not required for anything else but being supplied as parameter
				local_ids[i] = local_id;
				
				PACKETIZED_OPENCL_DRIVER_DEBUG_RUNTIME(
					insertPrintf("global_id[i]: ", global_ids[i], true, call);
					insertPrintf("local_id[i]: ", local_ids[i], true, call);
				);
			}
#endif

		}
	}

	void generateBlockSizeLoopsForWrapper(Function* f, CallInst* call, const unsigned num_dimensions, const int simd_dim, LLVMContext& context, Module* module) {
		assert (f && call);
		assert (f == call->getParent()->getParent());
		assert (num_dimensions <= PACKETIZED_OPENCL_DRIVER_MAX_NUM_DIMENSIONS);
		PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "\ngenerating loop(s) over group size(s) in function '"
				<< f->getNameStr() << "' around call to '" << call->getCalledFunction()->getNameStr() << "'...\n\n"; );

		//PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << *f << "\n"; );

		Instruction* insertBefore = call;

		Function::arg_iterator A = f->arg_begin(); // arg_struct
		Value* arg_work_dim = ++A;
		Value* arg_global_size_array = ++A;
		Value* arg_local_size_array = ++A;
		Value* arg_group_id_array = ++A;

		PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "  work_dim arg   : " << *arg_work_dim << "\n"; );
		PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "  global_size arg: " << *arg_global_size_array << "\n"; );
		PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "  local_size arg : " << *arg_local_size_array << "\n"; );
		PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "  group_id arg   : " << *arg_group_id_array << "\n"; );

		// allocate array of size 'num_dimensions' for special parameter num_groups
		//Value* arg_num_groups_array = UndefValue::get(ArrayType::get(Type::getInt32Ty(context), num_dimensions)); // TODO: maybe later...
		AllocaInst* arg_num_groups_array = new AllocaInst(Type::getInt32Ty(context), ConstantInt::get(context,  APInt(32, num_dimensions)), "num_groups_array", insertBefore);

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
		AllocaInst* arg_global_id_array = new AllocaInst(num_groupss[0]->getType(), numDimVal, "global_id_array", insertBefore);
		AllocaInst* arg_local_id_array  = new AllocaInst(num_groupss[0]->getType(), numDimVal, "local_id_array", insertBefore);

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
		// TODO: move this loop to own function?
#ifdef PACKETIZED_OPENCL_DRIVER_NO_PACKETIZATION
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
#else
		Value* arg_global_id_split_simd = NULL;
		Value* arg_local_id_split_simd = NULL;
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
				local_ids,
				&arg_global_id_split_simd,
				&arg_local_id_split_simd);
#endif

		PACKETIZED_OPENCL_DRIVER_DEBUG( PacketizedOpenCLDriver::writeFunctionToFile(f, "debug_block_wrapper_noinline.ll"); );

		// inline all calls inside wrapper
		PacketizedOpenCLDriver::inlineFunctionCalls(f);

		PACKETIZED_OPENCL_DRIVER_DEBUG( PacketizedOpenCLDriver::writeFunctionToFile(f, "debug_block_wrapper_inline.ll"); );

		// replace functions by parameter accesses (has to be done AFTER inlining!
		// start with second argument (first is void* of argument_struct)
		llvm::Function::arg_iterator arg = f->arg_begin();
		PacketizedOpenCLDriver::replaceCallbacksByArgAccess(module->getFunction("get_work_dim"),       cast<Value>(++arg), f);
		PacketizedOpenCLDriver::replaceCallbacksByArgAccess(module->getFunction("get_global_size"),    cast<Value>(++arg), f);
		PacketizedOpenCLDriver::replaceCallbacksByArgAccess(module->getFunction("get_local_size"),     cast<Value>(++arg), f);
		PacketizedOpenCLDriver::replaceCallbacksByArgAccess(module->getFunction("get_group_id"),       cast<Value>(++arg), f);

		// remap calls to parameters that are generated inside loop(s)
		// NOTE: _split functions can remain instead of being replaced by _SIMD function if only used on uniform path!
		PacketizedOpenCLDriver::replaceCallbacksByArgAccess(module->getFunction("get_num_groups"),     arg_num_groups_array, f);
		PacketizedOpenCLDriver::replaceCallbacksByArgAccess(module->getFunction("get_global_id"),      arg_global_id_array, f);
		PacketizedOpenCLDriver::replaceCallbacksByArgAccess(module->getFunction("get_local_id"),       arg_local_id_array, f);
		#ifndef PACKETIZED_OPENCL_DRIVER_NO_PACKETIZATION
		PacketizedOpenCLDriver::replaceCallbacksByArgAccess(module->getFunction("get_global_id_SIMD"), arg_global_id_array, f);
		PacketizedOpenCLDriver::replaceCallbacksByArgAccess(module->getFunction("get_local_id_SIMD"),  arg_local_id_array, f);
		PacketizedOpenCLDriver::replaceCallbacksByArgAccess(module->getFunction("get_global_id_split_SIMD"), arg_global_id_split_simd, f);
		PacketizedOpenCLDriver::replaceCallbacksByArgAccess(module->getFunction("get_local_id_split_SIMD"),  arg_local_id_split_simd, f);
		#endif

		PacketizedOpenCLDriver::fixFunctionNames(module);

		PACKETIZED_OPENCL_DRIVER_DEBUG( PacketizedOpenCLDriver::writeFunctionToFile(f, "debug_block_wrapper_final.ll"); );


		PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "\n" << *f << "\n"; );
		PACKETIZED_OPENCL_DRIVER_DEBUG( verifyFunction(*f); );

		delete [] global_sizes;
		delete [] local_sizes;
		delete [] group_ids;
		delete [] num_groupss;
		delete [] global_ids;
		delete [] local_ids;
	}
	// NOTE: This function relies on the switch-wrapper function (the one calling
	//       the continuations) being untouched (no optimization/inlining) after
	//       its generation!
	void generateBlockSizeLoopsForContinuations(const unsigned num_dimensions, const int simd_dim, LLVMContext& context, Function* f, ContinuationGenerator::ContinuationVecType& continuations) {
		assert (f);
		assert (num_dimensions <= PACKETIZED_OPENCL_DRIVER_MAX_NUM_DIMENSIONS);
		PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "\ngenerating loops over group size(s) around continuations...\n\n"; );
		typedef ContinuationGenerator::ContinuationVecType ContVecType;

		Instruction* insertBefore = f->begin()->getFirstNonPHI();

		Function::arg_iterator A = f->arg_begin(); // arg_struct
		Value* arg_work_dim = ++A;
		Value* arg_global_size_array = ++A;
		Value* arg_local_size_array = ++A;
		Value* arg_group_id_array = ++A;

		PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "  work_dim arg   : " << *arg_work_dim << "\n"; );
		PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "  global_size arg: " << *arg_global_size_array << "\n"; );
		PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "  local_size arg : " << *arg_local_size_array << "\n"; );
		PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "  group_id arg   : " << *arg_group_id_array << "\n"; );

		// allocate array of size 'num_dimensions' for special parameter num_groups
		Value* numDimVal = ConstantInt::get(context,  APInt(32, num_dimensions));
		//Value* arg_num_groups_array = UndefValue::get(ArrayType::get(Type::getInt32Ty(context), num_dimensions)); // TODO: maybe later...
		AllocaInst* arg_num_groups_array = new AllocaInst(Type::getInt32Ty(context), numDimVal, "num_groups_array", insertBefore);

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
		AllocaInst* arg_global_id_array = new AllocaInst(num_groupss[0]->getType(), numDimVal, "global_id_array", insertBefore);
		AllocaInst* arg_local_id_array  = new AllocaInst(num_groupss[0]->getType(), numDimVal, "local_id_array", insertBefore);

		unsigned continuation_id = 0;
		for (ContVecType::iterator it=continuations.begin(), E=continuations.end(); it!=E; ++it, ++continuation_id) {
			Function* continuation = *it;
			assert (continuation);
			PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "\n  generating loop(s) for continuation " << continuation_id << ": '" << continuation->getNameStr() << "'...\n"; );
			PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "    has " << continuation->getNumUses() << " uses!\n"; );
			
			//PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << *continuation << "\n"; );

			assert (!continuation->use_empty());

			for (Function::use_iterator U=continuation->use_begin(), UE=continuation->use_end(); U!=UE; ++U) {
				assert (isa<CallInst>(U));
				CallInst* call = cast<CallInst>(U);
				if (call->getParent()->getParent() != f) continue; // ignore all uses in different functions

				PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "    generating loop(s) around call: " << *call << "\n"; );

#ifdef PACKETIZED_OPENCL_DRIVER_NO_PACKETIZATION
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
#else
				Value* arg_global_id_split_simd = NULL;
				Value* arg_local_id_split_simd = NULL;
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
						local_ids,
						&arg_global_id_split_simd,
						&arg_local_id_split_simd);
#endif

				// replace undef arguments to function call by special parameters
				std::vector<Value*> params;
				params.push_back(arg_global_id_array);
				params.push_back(arg_local_id_array);
#ifndef PACKETIZED_OPENCL_DRIVER_NO_PACKETIZATION
				params.push_back(arg_global_id_split_simd);
				params.push_back(arg_local_id_split_simd);
#endif
				params.push_back(arg_num_groups_array);
				params.push_back(arg_work_dim);
				params.push_back(arg_global_size_array);
				params.push_back(arg_local_size_array);
				params.push_back(arg_group_id_array);

				PACKETIZED_OPENCL_DRIVER_DEBUG(
					outs() << "\n    params for new call:\n";
					outs() << "     * " << *arg_global_id_array << "\n";
					outs() << "     * " << *arg_local_id_array << "\n";
				);
#ifndef PACKETIZED_OPENCL_DRIVER_NO_PACKETIZATION
				PACKETIZED_OPENCL_DRIVER_DEBUG(
					outs() << "     * " << *arg_global_id_split_simd << "\n";
					outs() << "     * " << *arg_local_id_split_simd << "\n";
				);
#endif
				PACKETIZED_OPENCL_DRIVER_DEBUG(
					outs() << "     * " << *arg_num_groups_array << "\n";
					outs() << "     * " << *arg_work_dim << "\n";
					outs() << "     * " << *arg_global_size_array << "\n";
					outs() << "     * " << *arg_local_size_array << "\n";
					outs() << "     * " << *arg_group_id_array << "\n";
				);

				// add normal parameters and live value struct param
				//(= start at last special param idx +1 for callee)
				for (unsigned i=params.size()+1; i<call->getNumOperands(); ++i) {
					Value* opV = call->getOperand(i);
					params.push_back(opV);
					PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "     * " << *opV << "\n"; );
				}
				CallInst* newCall = CallInst::Create(call->getCalledFunction(), params.begin(), params.end(), "", call);
				call->replaceAllUsesWith(newCall);
				call->eraseFromParent();

				PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "\n    new call: " << *newCall << "\n\n"; );
				PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "\n" << *continuation << "\n"; );

				// adjust GEP-instructions to point to current localID's live value struct,
				// e.g. GEP liveValueUnion, i32 0, i32 elementindex
				// ---> GEP liveValueUnion, i32 local_id_flat, i32 elementindex
				adjustLiveValueLoadGEPs(newCall, continuation_id, num_dimensions, local_ids, local_sizes);

				// Now do the exact same thing inside the continuation:
				// Replace the GEPs that are used for storing the live values
				// of the next continuation.
				adjustLiveValueStoreGEPs(continuation, num_dimensions, context);

				PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "\n" << *continuation << "\n"; );
				PACKETIZED_OPENCL_DRIVER_DEBUG( verifyFunction(*continuation); );

				break; // there is exactly one use of the continuation of interest
			}
		} // for each continuation

		// adjust alloca of liveValueUnion (reserve sizeof(union)*blocksize[0]*blocksize[1]*... )
		assert (continuations.back() && continuations.back()->use_back());
		assert (isa<CallInst>(continuations.back()->use_back()));
		CallInst* someContinuationCall = cast<CallInst>(continuations.back()->use_back());
		assert (someContinuationCall->getOperand(someContinuationCall->getNumOperands()-1));
		Value* liveValueUnion = someContinuationCall->getOperand(someContinuationCall->getNumOperands()-1);
		PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "liveValueUnion: " << *liveValueUnion << "\n"; );

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

		PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "\n" << *f << "\n"; );
		PACKETIZED_OPENCL_DRIVER_DEBUG( verifyFunction(*f); );

		delete [] global_sizes;
		delete [] local_sizes;
		delete [] group_ids;
		delete [] num_groupss;
		delete [] global_ids; // not required for anything else but being supplied as parameter
		delete [] local_ids;
	}

#ifdef PACKETIZED_OPENCL_DRIVER_NO_PACKETIZATION
	inline Function* createKernelSequential(Function* f, const std::string& kernel_name, const unsigned num_dimensions, Module* module, TargetData* targetData, LLVMContext& context, cl_int* errcode_ret) {
		// eliminate barriers
		FunctionPassManager FPM(module);
		LivenessAnalyzer* LA = new LivenessAnalyzer(true);
		ContinuationGenerator* CG = new ContinuationGenerator(true);
		// set "special" parameter types that are generated for each continuation
		// order is important!
		CG->addSpecialParam(Type::getInt32PtrTy(context, 0), "get_global_id");   // generated inside switch (group_id * loc_size + loc_id)
		CG->addSpecialParam(Type::getInt32PtrTy(context, 0), "get_local_id");    // generated inside switch (loop induction variables)
		CG->addSpecialParam(Type::getInt32PtrTy(context, 0), "get_num_groups");  // generated inside switch (glob_size / loc_size)
		CG->addSpecialParam(Type::getInt32Ty(context),       "get_work_dim");    // supplied from outside
		CG->addSpecialParam(Type::getInt32PtrTy(context, 0), "get_global_size"); // supplied from outside
		CG->addSpecialParam(Type::getInt32PtrTy(context, 0), "get_local_size");  // supplied from outside
		CG->addSpecialParam(Type::getInt32PtrTy(context, 0), "get_group_id");    // supplied from outside
		FPM.add(LA);
		FPM.add(CG);
		FPM.run(*f);

		Function* barrierFreeFunction = CG->getBarrierFreeFunction();

		llvm::Function* f_wrapper = NULL;

		if (barrierFreeFunction) {

			PACKETIZED_OPENCL_DRIVER_DEBUG( verifyFunction(*barrierFreeFunction); );
			PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << *barrierFreeFunction << "\n"; );

			f->replaceAllUsesWith(barrierFreeFunction);
			barrierFreeFunction->takeName(f);
			f->setName(barrierFreeFunction->getNameStr()+"_orig");

			PACKETIZED_OPENCL_DRIVER_DEBUG( PacketizedOpenCLDriver::writeModuleToFile(module, "debug_nobarriers.mod.ll"); );

			ContinuationGenerator::ContinuationVecType continuations;
			CG->getContinuations(continuations);

			PACKETIZED_OPENCL_DRIVER_DEBUG(
				outs() << "continuations:\n";
				for (SmallVector<Function*, 4>::iterator it=continuations.begin(), E=continuations.end(); it!=E; ++it) {
					Function* continuation = *it;
					outs() << " * " << continuation->getNameStr() << "\n";
				}
				outs() << "\n";
			);

			// generate wrapper for kernel (= all kernels have same signature)
			//
			std::stringstream strs2;
			strs2 << kernel_name << "_wrapper";
			const std::string wrapper_name = strs2.str();

			PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "generating kernel wrapper... "; );
			const bool inlineCall = true; // inline call immediately
			PacketizedOpenCLDriver::generateKernelWrapper(wrapper_name, barrierFreeFunction, module, inlineCall);
			PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "done.\n"; );

			f_wrapper = PacketizedOpenCLDriver::getFunction(wrapper_name, module);
			if (!f_wrapper) {
				errs() << "ERROR: could not find wrapper function in kernel module!\n";
				*errcode_ret = CL_INVALID_PROGRAM_EXECUTABLE; //sth like that :p
				return NULL;
			}

			// - callbacks inside continuations have to be replaced by argument accesses
			PacketizedOpenCLDriver::mapCallbacksToContinuationArguments(num_dimensions, context, module, continuations);

			// - generate loops
			// - generate code for 3 generated special parameters in each loop
			// - map "special" arguments of calls to each continuation correctly (either to wrapper-param or to generated value inside loop)
			// - make liveValueUnion an array of unions (size: blocksize[0]*blocksize[1]*blocksize[2]*...)
			const int simd_dim = -1;
			PacketizedOpenCLDriver::generateBlockSizeLoopsForContinuations(num_dimensions, simd_dim, context, f_wrapper, continuations);

		} else {

			// no barrier inside function

			// generate wrapper for kernel (= all kernels have same signature)
			//
			std::stringstream strs2;
			strs2 << kernel_name << "_wrapper";
			const std::string wrapper_name = strs2.str();

			PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "  generating kernel wrapper... "; );
			const bool inlineCall = false; // don't inline call immediately (needed for generating loop(s))
			PacketizedOpenCLDriver::generateKernelWrapper(wrapper_name, f, module, inlineCall);
			PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "done.\n"; );

			f_wrapper = PacketizedOpenCLDriver::getFunction(wrapper_name, module);
			if (!f_wrapper) {
				errs() << "ERROR: could not find wrapper function in kernel module!\n";
				*errcode_ret = CL_INVALID_PROGRAM_EXECUTABLE; //sth like that :p
				return NULL;
			}

			// generate loop(s) over blocksize(s) (BEFORE inlining!)
			CallInst* kernelCall = getWrappedKernelCall(f_wrapper, f);
			const int simdDim = -1;
			generateBlockSizeLoopsForWrapper(f_wrapper, kernelCall, num_dimensions, simdDim, context, module);
		}

		assert (f_wrapper);

		// optimize wrapper with inlined kernel
		PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "optimizing wrapper... "; );
		PacketizedOpenCLDriver::inlineFunctionCalls(f_wrapper, targetData);
		PacketizedOpenCLDriver::optimizeFunction(f_wrapper);
		PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "done.\n" << *f_wrapper << "\n"; );
		PACKETIZED_OPENCL_DRIVER_DEBUG( PacketizedOpenCLDriver::verifyModule(module); );
		PACKETIZED_OPENCL_DRIVER_DEBUG( PacketizedOpenCLDriver::writeFunctionToFile(f_wrapper, "debug_kernel_final.ll"); );
		PACKETIZED_OPENCL_DRIVER_DEBUG( PacketizedOpenCLDriver::writeModuleToFile(module, "debug_kernel_final.mod.ll"); );

		return f_wrapper;
	}
#else
	inline Function* createKernelPacketized(Function* f, const std::string& kernel_name, const unsigned num_dimensions, const unsigned simd_dim, Module* module, TargetData* targetData, LLVMContext& context, cl_int* errcode_ret, Function** f_SIMD_ret) {
		// PACKETIZATION ENABLED
		// USE AUTO-GENERATED PACKET WRAPPER
		//
		// save SIMD function for argument checking (uniform vs. varying)
		PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "  generating OpenCL-specific functions etc... "; );

		// generate all necessary function declarations
		std::stringstream strs;
		strs << kernel_name << "_SIMD";
		const std::string kernel_simd_name = strs.str();
		llvm::Function* f_SIMD = PacketizedOpenCLDriver::generatePacketPrototypeFromOpenCLKernel(f, kernel_simd_name, module, PACKETIZED_OPENCL_DRIVER_SIMD_WIDTH);

		PacketizedOpenCLDriver::generateOpenCLFunctions(module, PACKETIZED_OPENCL_DRIVER_SIMD_WIDTH);

		llvm::Function* gid = PacketizedOpenCLDriver::getFunction("get_global_id", module);
		llvm::Function* gid_split = PacketizedOpenCLDriver::getFunction("get_global_id_split", module);
		llvm::Function* gid_simd = PacketizedOpenCLDriver::getFunction("get_global_id_SIMD", module);
		llvm::Function* lid = PacketizedOpenCLDriver::getFunction("get_local_id", module);
		llvm::Function* lid_split = PacketizedOpenCLDriver::getFunction("get_local_id_split", module);
		llvm::Function* lid_simd = PacketizedOpenCLDriver::getFunction("get_local_id_SIMD", module);
		assert (gid && "could not find function 'get_global_id' in module!");
		assert (gid_split && "could not find function 'get_global_id_split' in module!");
		assert (gid_simd && "could not find function 'get_global_id_simd' in module!");
		assert (lid && "could not find function 'get_local_id' in module!");
		assert (lid_split && "could not find function 'get_local_id_split' in module!");
		assert (lid_simd && "could not find function 'get_local_id_simd' in module!");

		// Analyze all calls to get_global_id with simdDim as supplied dimension.
		// Optimize cases where index is used directly or only with linear
		// combinations applied (replace callee with get_global_id_SIMD).
		// Everywhere this is not possible, force splitting of the packets
		// (scattered-gather/scattered-store, replace calls with get_global_id_split).
		// Calls to get_global_id with other dimensions remain unchanged (uses
		// on packetized paths will be replicated);
		PacketizedOpenCLDriver::setupIndexUsages(f, gid, gid_simd, gid_split, simd_dim);
		PacketizedOpenCLDriver::setupIndexUsages(f, lid, lid_simd, lid_split, simd_dim);
		PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << *f << "\n"; );

		PACKETIZED_OPENCL_DRIVER_DEBUG( PacketizedOpenCLDriver::verifyModule(module); );
		PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "done.\n"; );

		// packetize scalar function into SIMD function
		PACKETIZED_OPENCL_DRIVER_DEBUG( PacketizedOpenCLDriver::writeFunctionToFile(f, "debug_kernel_pre_packetization.ll"); );
		PacketizedOpenCLDriver::packetizeKernelFunction(f->getNameStr(), kernel_simd_name, module, PACKETIZED_OPENCL_DRIVER_SIMD_WIDTH, true, false);
		f_SIMD = PacketizedOpenCLDriver::getFunction(kernel_simd_name, module); //pointer not valid anymore!
		PACKETIZED_OPENCL_DRIVER_DEBUG( PacketizedOpenCLDriver::verifyModule(module); );
		PACKETIZED_OPENCL_DRIVER_DEBUG( PacketizedOpenCLDriver::writeFunctionToFile(f_SIMD, "debug_kernel_packetized.ll"); );

		PACKETIZED_OPENCL_DRIVER_DEBUG( PacketizedOpenCLDriver::writeModuleToFile(f_SIMD->getParent(), "debug_f_simd.mod.ll"); );
		std::set<GetElementPtrInst*> fixedGEPs;
		PacketizedOpenCLDriver::fixUniformPacketizedArrayAccesses(f_SIMD, PacketizedOpenCLDriver::getFunction("get_global_id_split_SIMD", module), PACKETIZED_OPENCL_DRIVER_SIMD_WIDTH, fixedGEPs);
		PacketizedOpenCLDriver::fixUniformPacketizedArrayAccesses(f_SIMD, PacketizedOpenCLDriver::getFunction("get_local_id_split_SIMD", module), PACKETIZED_OPENCL_DRIVER_SIMD_WIDTH, fixedGEPs);
		PACKETIZED_OPENCL_DRIVER_DEBUG( PacketizedOpenCLDriver::verifyModule(module); );
		PACKETIZED_OPENCL_DRIVER_DEBUG( PacketizedOpenCLDriver::writeModuleToFile(f_SIMD->getParent(), "debug_f_simd2.mod.ll"); );

		// eliminate barriers
		FunctionPassManager FPM(module);
		LivenessAnalyzer* LA = new LivenessAnalyzer(true);
		ContinuationGenerator* CG = new ContinuationGenerator(true);
		// set "special" parameter types that are generated for each continuation
		// order is important (has to match mapCallbacksToContinuationArguments())!
		CG->addSpecialParam(Type::getInt32PtrTy(context, 0), "get_global_id");   // generated inside switch (group_id * loc_size + loc_id)
		CG->addSpecialParam(Type::getInt32PtrTy(context, 0), "get_local_id");    // generated inside switch (loop induction variables)
		CG->addSpecialParam(VectorType::get(Type::getInt32Ty(context), PACKETIZED_OPENCL_DRIVER_SIMD_WIDTH),      "get_global_id_split_SIMD");   // generated inside switch (reconstructed global ids of scalar instances)
		CG->addSpecialParam(VectorType::get(Type::getInt32Ty(context), PACKETIZED_OPENCL_DRIVER_SIMD_WIDTH),       "get_local_id_split_SIMD");    // generated inside switch (reconstructed local ids of scalar instances)
		CG->addSpecialParam(Type::getInt32PtrTy(context, 0), "get_num_groups");  // generated inside switch (glob_size / loc_size)
		CG->addSpecialParam(Type::getInt32Ty(context),       "get_work_dim");    // supplied from outside
		CG->addSpecialParam(Type::getInt32PtrTy(context, 0), "get_global_size"); // supplied from outside
		CG->addSpecialParam(Type::getInt32PtrTy(context, 0), "get_local_size");  // supplied from outside
		CG->addSpecialParam(Type::getInt32PtrTy(context, 0), "get_group_id");    // supplied from outside
		FPM.add(LA);
		FPM.add(CG);
		FPM.run(*f_SIMD);

		Function* barrierFreeFunction = CG->getBarrierFreeFunction();

		llvm::Function* f_wrapper = NULL;

		if (barrierFreeFunction) {
			// NOTE: We must not optimize or inline anything yet,
			// the wrapper is required as generated for loop generation!

			PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << *barrierFreeFunction << "\n"; );
			PACKETIZED_OPENCL_DRIVER_DEBUG( verifyFunction(*barrierFreeFunction); );

			f_SIMD->replaceAllUsesWith(barrierFreeFunction);
			barrierFreeFunction->takeName(f_SIMD);
			f_SIMD->setName(barrierFreeFunction->getNameStr()+"_orig");

			f_SIMD = barrierFreeFunction;

			PACKETIZED_OPENCL_DRIVER_DEBUG( PacketizedOpenCLDriver::writeModuleToFile(module, "debug_barrier_wrapper.mod.ll"); );
			
			ContinuationGenerator::ContinuationVecType continuations;
			CG->getContinuations(continuations);

			PACKETIZED_OPENCL_DRIVER_DEBUG(
				outs() << "continuations:\n";
				for (SmallVector<Function*, 4>::iterator it=continuations.begin(), E=continuations.end(); it!=E; ++it) {
					Function* continuation = *it;
					outs() << " * " << continuation->getNameStr() << "\n";
				}
				outs() << "\n";
			);


			strs << "_wrapper";
			const std::string wrapper_name = strs.str();

			PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "  generating kernel wrapper... "; );
			const bool inlineCall = true; // inline call immediately (and only this call)
			f_wrapper = PacketizedOpenCLDriver::generateKernelWrapper(wrapper_name, f_SIMD, module, inlineCall);
			if (!f_wrapper) {
				errs() << "FAILED!\nERROR: wrapper generation for kernel module failed!\n";
				*errcode_ret = CL_INVALID_PROGRAM_EXECUTABLE; //sth like that :p
				return NULL;
			}
			PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "done.\n"; );
			PACKETIZED_OPENCL_DRIVER_DEBUG( PacketizedOpenCLDriver::writeFunctionToFile(f_wrapper, "debug_wrapper.ll"); );
			PACKETIZED_OPENCL_DRIVER_DEBUG( PacketizedOpenCLDriver::verifyModule(module); );

			//PACKETIZED_OPENCL_DRIVER_DEBUG( PacketizedOpenCLDriver::writeModuleToFile(f_wrapper->getParent(), "debug_f_simd.mod.ll"); );
			//std::set<GetElementPtrInst*> fixedGEPs;
			//PacketizedOpenCLDriver::fixUniformPacketizedArrayAccesses(f_SIMD, PacketizedOpenCLDriver::getFunction("get_global_id_split_SIMD", module), PACKETIZED_OPENCL_DRIVER_SIMD_WIDTH, fixedGEPs);
			//PacketizedOpenCLDriver::fixUniformPacketizedArrayAccesses(f_SIMD, PacketizedOpenCLDriver::getFunction("get_local_id_split_SIMD", module), PACKETIZED_OPENCL_DRIVER_SIMD_WIDTH, fixedGEPs);
			//PACKETIZED_OPENCL_DRIVER_DEBUG( PacketizedOpenCLDriver::verifyModule(module); );
			//PACKETIZED_OPENCL_DRIVER_DEBUG( PacketizedOpenCLDriver::writeModuleToFile(f_wrapper->getParent(), "debug_f_simd2.mod.ll"); );

			// - callbacks inside continuations have to be replaced by argument accesses
			PacketizedOpenCLDriver::mapCallbacksToContinuationArguments(num_dimensions, context, module, continuations);

			// - generate loops
			// - generate code for 3 generated special parameters in each loop
			// - map "special" arguments of calls to each continuation correctly (either to wrapper-param or to generated value inside loop)
			// - make liveValueUnion an array of unions (size: blocksize[0]*blocksize[1]*blocksize[2]*...)
			PacketizedOpenCLDriver::generateBlockSizeLoopsForContinuations(num_dimensions, simd_dim, context, f_wrapper, continuations);

//			PACKETIZED_OPENCL_DRIVER_DEBUG_RUNTIME(
//				Function* cont = continuations[1];
//				for (Function::iterator BB=cont->begin(), BBE=cont->end(); BB!=BBE; ++BB) {
//					if (!isa<ReturnInst>(BB->getTerminator())) continue;
//					for (BasicBlock::iterator I=BB->begin(), IE=BB->end(); I!=IE; ++I) {
//						if (isa<StoreInst>(I)) {
//							insertPrintf("!! stored return value: ", I->getOperand(0), true, I);
//							insertPrintf("!!   address: ", I->getOperand(1), true, I);
//						}
//					}
//					break;
//				}
//			);

		} else {

			// no barrier inside function

			// Generate wrapper for kernel (= all kernels have same signature)
			// Make sure the call to the original kernel is inlined after this!
			//
			std::stringstream strs2;
			strs2 << kernel_name << "_wrapper";
			const std::string wrapper_name = strs2.str();

			PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "  generating kernel wrapper... "; );
			const bool inlineCall = false; // don't inline call immediately (needed for generating loop(s))
			f_wrapper = PacketizedOpenCLDriver::generateKernelWrapper(wrapper_name, f_SIMD, module, inlineCall);
			if (!f_wrapper) {
				errs() << "FAILED!\nERROR: wrapper generation for kernel module failed!\n";
				*errcode_ret = CL_INVALID_PROGRAM_EXECUTABLE; //sth like that :p
				return NULL;
			}
			PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "done.\n"; );
			PACKETIZED_OPENCL_DRIVER_DEBUG( PacketizedOpenCLDriver::writeFunctionToFile(f_wrapper, "debug_arg_wrapper.ll"); );
			PACKETIZED_OPENCL_DRIVER_DEBUG( PacketizedOpenCLDriver::verifyModule(module); );

			//std::set<GetElementPtrInst*> fixedGEPs;
			//PacketizedOpenCLDriver::fixUniformPacketizedArrayAccesses(f_SIMD, PacketizedOpenCLDriver::getFunction("get_global_id_split_SIMD", module), PACKETIZED_OPENCL_DRIVER_SIMD_WIDTH, fixedGEPs);
			//PacketizedOpenCLDriver::fixUniformPacketizedArrayAccesses(f_SIMD, PacketizedOpenCLDriver::getFunction("get_local_id_split_SIMD", module), PACKETIZED_OPENCL_DRIVER_SIMD_WIDTH, fixedGEPs);
			//PACKETIZED_OPENCL_DRIVER_DEBUG( PacketizedOpenCLDriver::verifyModule(module); );

			// generate loop(s) over blocksize(s) (BEFORE inlining!)
			CallInst* kernelCall = getWrappedKernelCall(f_wrapper, f_SIMD);
			generateBlockSizeLoopsForWrapper(f_wrapper, kernelCall, num_dimensions, simd_dim, context, module);
		}

		assert (f_wrapper);

		// optimize wrapper with inlined kernel
		PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "optimizing wrapper... "; );
		//PacketizedOpenCLDriver::inlineFunctionCalls(f_wrapper, targetData);
		//PacketizedOpenCLDriver::optimizeFunction(f_wrapper);
		PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "done.\n" << *f_wrapper << "\n"; );
		PACKETIZED_OPENCL_DRIVER_DEBUG( PacketizedOpenCLDriver::verifyModule(module); );
		PACKETIZED_OPENCL_DRIVER_DEBUG( PacketizedOpenCLDriver::writeFunctionToFile(f_wrapper, "debug_kernel_wrapped_final.ll"); );
		PACKETIZED_OPENCL_DRIVER_DEBUG( PacketizedOpenCLDriver::writeModuleToFile(module, "debug_kernel_wrapped_final.mod.ll"); );

		*f_SIMD_ret = f_SIMD;
		return f_wrapper;
	}
#endif

	inline cl_uint convertLLVMAddressSpace(cl_uint llvm_address_space) {
		switch (llvm_address_space) {
			case 0 : return CL_PRIVATE;
			case 1 : return CL_GLOBAL;
			case 3 : return CL_LOCAL;
			default : return llvm_address_space;
		}
	}
	inline std::string getAddressSpaceString(cl_uint cl_address_space) {
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
	inline unsigned long long getDeviceMaxMemAllocSize() {
		return 0x3B9ACA00; // 1 GB
	}

}


///////////////////////////////////////////////////////////////////////////
//             Packetized OpenCL Internal Data Structures                //
///////////////////////////////////////////////////////////////////////////

struct _cl_platform_id {};
struct _cl_device_id {};

/*
An OpenCL context is created with one or more devices. Contexts
are used by the OpenCL runtime for managing objects such as command-queues,
memory, program and kernel objects and for executing kernels on one or more
devices specified in the context.
*/
struct _cl_context {};

/*
OpenCL objects such as memory, program and kernel objects are created using a
context.
Operations on these objects are performed using a command-queue. The
command-queue can be used to queue a set of operations (referred to as commands)
in order. Having multiple command-queues allows applications to queue multiple
independent commands without requiring synchronization. Note that this should
work as long as these objects are not being shared. Sharing of objects across
multiple command-queues will require the application to perform appropriate
synchronization. This is described in Appendix A.
*/
struct _cl_command_queue {
	_cl_context* context;
};

/*
Memory objects are categorized into two types: buffer objects, and image
objects. A buffer object stores a one-dimensional collection of elements whereas
an image object is used to store a two- or three- dimensional texture,
frame-buffer or image.
Elements of a buffer object can be a scalar data type (such as an int, float),
vector data type, or a user-defined structure. An image object is used to
represent a buffer that can be used as a texture or a frame-buffer. The elements
of an image object are selected from a list of predefined image formats. The
minimum number of elements in a memory object is one.
*/
struct _cl_mem {
private:
	_cl_context* context;
	size_t size; //entire size in bytes
	void* data;
	const bool canRead;
	const bool canWrite;
public:
	_cl_mem(_cl_context* ctx, size_t bytes, void* values, bool can_read, bool can_write)
			: context(ctx), size(bytes), data(values), canRead(can_read), canWrite(can_write) {}
	
	inline _cl_context* get_context() const { return context; }
	inline void* get_data() const { return data; }
	inline size_t get_size() const { return size; }
	inline bool isReadOnly() const { return canRead && !canWrite; }
	inline bool isWriteOnly() const { return !canRead && canWrite; }

	inline void copy_data(
			const void* values,
			const size_t bytes,
			const size_t offset=0)
	{
		assert (bytes+offset <= size);
		if (offset == 0) memcpy(data, values, bytes);
		else {
			for (cl_uint i=offset; i<bytes; ++i) {
				((char*)data)[i] = ((const char*)values)[i];
			}
		}
	}
};

/*
A sampler object describes how to sample an image when the image is read in the
kernel. The built-in functions to read from an image in a kernel take a sampler
as an argument. The sampler arguments to the image read function can be sampler
objects created using OpenCL functions and passed as argument values to the
kernel or can be samplers declared inside a kernel. In this section we discuss
how sampler objects are created using OpenCL functions.
*/
struct _cl_sampler {
	_cl_context* context;
};

/*
An OpenCL program consists of a set of kernels that are identified as functions
declared with the __kernel qualifier in the program source. OpenCL programs may
also contain auxiliary functions and constant data that can be used by __kernel
functions. The program executable can be generated online or offline by the
OpenCL compiler for the appropriate target device(s).
A program object encapsulates the following information:
       An associated context.
       A program source or binary.
       The latest successfully built program executable, the list of devices for
       which the program executable is built, the build options used and a build
       log.
       The number of kernel objects currently attached.
*/
struct _cl_program {
	_cl_context* context;
	const char* fileName;
	llvm::Module* module;
	llvm::TargetData* targetData;
};


struct _cl_kernel_arg {
private:
	// known at kernel creation time
	const size_t element_size; // size of one item in bytes
	const cl_uint address_space;
	const bool uniform;
	void* mem_address; // values are inserted by kernel::set_arg_data()

	// only known after clSetKernelArg
	size_t size; // size of entire argument value

public:
	_cl_kernel_arg(
			const size_t _elem_size,
			const cl_uint _address_space,
			const bool _uniform,
			void* _mem_address,
			const size_t _size=0)
		: element_size(_elem_size),
		address_space(_address_space),
		uniform(_uniform),
		mem_address(_mem_address),
		size(_size)
	{}

	inline void set_size(size_t _size) { size = _size; }

	inline size_t get_size() const { return size; }
	inline size_t get_element_size() const { return element_size; }
	inline cl_uint get_address_space() const { return address_space; }
	inline void* get_mem_address() const { return mem_address; } // must not assert (data) -> can be 0 if non-pointer type (e.g. float)

	inline bool is_uniform() const { return uniform; }
};

/*
A kernel is a function declared in a program. A kernel is identified by the
__kernel qualifier applied to any function in a program. A kernel object
encapsulates the specific __kernel function declared in a program and the
argument values to be used when executing this __kernel function.
*/
struct _cl_kernel {
private:
	_cl_context* context;
	_cl_program* program;
	const void* compiled_function;

	std::vector<_cl_kernel_arg*> args;
	const cl_uint num_args;

	void* argument_struct;
	size_t argument_struct_size;

	cl_uint num_dimensions;
	cl_uint best_simd_dim;

public:
	_cl_kernel(_cl_context* ctx, _cl_program* prog, llvm::Function* f,
			llvm::Function* f_wrapper, llvm::Function* f_SIMD=NULL)
		: context(ctx), program(prog), compiled_function(NULL), num_args(PacketizedOpenCLDriver::getNumArgs(f)),
		argument_struct(NULL), argument_struct_size(0), num_dimensions(0), best_simd_dim(0),
		function(f), function_wrapper(f_wrapper), function_SIMD(f_SIMD)
	{
		PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "  creating kernel object... \n"; );
		assert (ctx && prog && f && f_wrapper);

		// compile wrapper function (to be called in clEnqueueNDRangeKernel())
		// NOTE: be sure that f_SIMD or f are inlined and f_wrapper was optimized to the max :p
		PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "    compiling function '" << f_wrapper->getNameStr() << "'... "; );
		compiled_function = PacketizedOpenCLDriver::getPointerToFunction(prog->module, f_wrapper);
		if (!compiled_function) {
			errs() << "\nERROR: JIT compilation of kernel function failed!\n";
		}
		PACKETIZED_OPENCL_DRIVER_DEBUG( if (compiled_function) outs() << "done.\n"; );

		// get argument information
		PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "    collecting argument information...\n"; );
		//num_args = PacketizedOpenCLDriver::getNumArgs(f);
		assert (num_args > 0); // TODO: don't we allow kernels without arguments? do they make sense?
		args.reserve(num_args);

		// determine size of each argument
		for (cl_uint arg_index=0; arg_index<num_args; ++arg_index) {
			// get type of argument and corresponding size
			const llvm::Type* argType = PacketizedOpenCLDriver::getArgumentType(f, arg_index);
			const size_t arg_size_bytes = PacketizedOpenCLDriver::getTypeSizeInBits(program->targetData, argType) / 8;

			argument_struct_size += arg_size_bytes;
		}

		// allocate memory for argument_struct
		// TODO: do we have to care about type padding?
		argument_struct = malloc(argument_struct_size);
		PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "      size of argument-struct: " << argument_struct_size << " bytes\n"; );
		PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "      address of argument-struct: " << argument_struct << "\n"; );
		PACKETIZED_OPENCL_DRIVER_DEBUG(
			const llvm::Type* argType = PacketizedOpenCLDriver::getArgumentType(f_wrapper, 0);
			outs() << "      LLVM type: " << *argType << "\n";
			const llvm::Type* sType = PacketizedOpenCLDriver::getContainedType(argType, 0); // get size of struct, not of pointer to struct
			outs() << "      LLVM type size: " << PacketizedOpenCLDriver::getTypeSizeInBits(prog->targetData, sType)/8 << "\n";
		);

		// create argument objects
		size_t current_size=0;
		for (cl_uint arg_index=0; arg_index<num_args; ++arg_index) {

			const llvm::Type* argType = PacketizedOpenCLDriver::getArgumentType(f, arg_index);
			const size_t arg_size_bytes = PacketizedOpenCLDriver::getTypeSizeInBits(program->targetData, argType) / 8;
			const cl_uint address_space = PacketizedOpenCLDriver::convertLLVMAddressSpace(PacketizedOpenCLDriver::getAddressSpace(argType));

			// check if argument is uniform
#ifdef PACKETIZED_OPENCL_DRIVER_NO_PACKETIZATION
			const bool arg_uniform = true; // no packetization = no need for uniform/varying
#else
			assert (f_SIMD);
			// save info if argument is uniform or varying
			// TODO: implement in packetizer directly
			// HACK: if types match, they are considered uniform, varying otherwise

			const llvm::Type* argType_SIMD = PacketizedOpenCLDriver::getArgumentType(f_SIMD, arg_index);
			const bool arg_uniform = argType == argType_SIMD;

			// check for sanity
			if (!arg_uniform && (address_space != CL_GLOBAL)) {
				// NOTE: This can not really exist, as the input data for such a value
				//       is always a scalar (the user's host program is not changed)!
				// NOTE: This case would mean there are values that change for each thread in a group
				//       but that is the same for the ith thread of any group.
				errs() << "WARNING: packet function must not use varying, non-pointer agument!\n";
				// TODO: should we do something about this and return some error code? :p
			}
#endif

			// save pointer to address of argument inside argument_struct
			void* arg_struct_addr = ((char*)argument_struct)+current_size;
			current_size += arg_size_bytes;

			PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "      argument " << arg_index << "\n"; );
			PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "        size     : " << arg_size_bytes << " bytes\n"; );
			PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "        address  : " << (void*)arg_struct_addr << "\n"; );
			PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "        addrspace: " << PacketizedOpenCLDriver::getAddressSpaceString(address_space) << "\n"; );

			args[arg_index] = new _cl_kernel_arg(arg_size_bytes, address_space, arg_uniform, arg_struct_addr);
		}

		PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "  kernel object created successfully!\n\n"; );
	}

	~_cl_kernel() { args.clear(); }

	const llvm::Function* function;
	const llvm::Function* function_wrapper;
	const llvm::Function* function_SIMD;

	// Copy 'arg_size' bytes from 'data' into argument_struct at the position
	// of argument at index 'arg_index'.
	// There are some possible issues with the type of data being copied:
	// We simply distinguish different address spaces to know whether the data
	// is actually a _cl_mem object, raw data, or a local pointer - all three
	// cases have to be treated differently:
	// _cl_mem** - CL_GLOBAL  - access the mem object and copy its data
	// raw data  - CL_PRIVATE - copy the data directly
	// local ptr - CL_LOCAL   - copy the pointer
	//
	// OpenCL Specification 1.0 for clSetKernelArg:
	// The argument data pointed to by arg_value is copied and the arg_value
	// pointer can therefore be reused by the application after clSetKernelArg returns.
	//
	// arg_size specifies the size of the argument value. If the argument is a memory object, the size is
	// the size of the buffer or image object type. For arguments declared with the __local qualifier,
	// the size specified will be the size in bytes of the buffer that must be allocated for the __local
	// argument.
	inline cl_uint set_arg_data(const cl_uint arg_index, const void* data, const size_t arg_size) {
		assert (arg_index < num_args);
		assert (args[arg_index] && "kernel object not completely initialized?");

		// store argument size
		args[arg_index]->set_size(arg_size);

		char* arg_pos = (char*)arg_get_data(arg_index); //((char*)argument_struct)+current_size;

		// NOTE: for pointers, we supply &data because we really want to copy the pointer!
		switch (arg_get_address_space(arg_index)) {
			case CL_GLOBAL: {
				assert (arg_size == sizeof(_cl_mem*)); // = sizeof(cl_mem)
				assert (data);
				// data is actually a _cl_mem* given by reference
				const _cl_mem* mem = *(const _cl_mem**)data; 
				const void* datax = mem->get_data();
				// copy the pointer, not what is pointed to
				memcpy(arg_pos, &datax, arg_size);
				break;
			}
			case CL_PRIVATE: {
				assert (data);
				// copy the data itself
				memcpy(arg_pos, data, arg_size);
				break;
			}
			case CL_LOCAL: {
				assert (!data);
				// allocate memory of size 'arg_size' and copy the pointer
				const void* datax = malloc(arg_size);
				memcpy(arg_pos, &datax, sizeof(void*));
				break;
			}
			case CL_CONSTANT: {
				errs() << "ERROR: support for constant memory not implemented yet!\n";
				assert (false && "support for constant memory not implemented yet!");
				return CL_INVALID_VALUE; // sth like that :p
			}
			default: {
				errs() << "ERROR: unknown address space found: " << arg_get_address_space(arg_index) << "\n";
				assert (false && "unknown address space found!");
				return CL_INVALID_VALUE; // sth like that :p
			}
		}

		PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "  data source: " << data << "\n"; );
		PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "  target pointer: " << (void*)arg_pos << "\n"; );

		return CL_SUCCESS;
	}
	inline void set_num_dimensions(const cl_uint num_dim) { num_dimensions = num_dim; }
	inline void set_best_simd_dim(const cl_uint dim) { best_simd_dim = dim; }

	inline _cl_context* get_context() const { return context; }
	inline _cl_program* get_program() const { return program; }
	inline const void* get_compiled_function() const { return compiled_function; }
	inline cl_uint get_num_args() const { return num_args; }
	inline const void* get_argument_struct() const { return argument_struct; }
	inline size_t get_argument_struct_size() const { return argument_struct_size; }
	inline cl_uint get_num_dimensions() const { return num_dimensions; }
	inline cl_uint get_best_simd_dim() const { return best_simd_dim; }

	inline size_t arg_get_element_size(const cl_uint arg_index) const {
		assert (arg_index < num_args);
		assert (args[arg_index] && "kernel object not completely initialized?");
		return args[arg_index]->get_element_size();
	}
	inline cl_uint arg_get_address_space(const cl_uint arg_index) const {
		assert (arg_index < num_args);
		assert (args[arg_index] && "kernel object not completely initialized?");
		return args[arg_index]->get_address_space();
	}
	inline bool arg_is_global(const cl_uint arg_index) const {
		assert (arg_index < num_args);
		assert (args[arg_index] && "kernel object not completely initialized?");
		return args[arg_index]->get_address_space() == CL_GLOBAL;
	}
	inline bool arg_is_local(const cl_uint arg_index) const {
		assert (arg_index < num_args);
		assert (args[arg_index] && "kernel object not completely initialized?");
		return args[arg_index]->get_address_space() == CL_LOCAL;
	}
	inline bool arg_is_private(const cl_uint arg_index) const {
		assert (arg_index < num_args);
		assert (args[arg_index] && "kernel object not completely initialized?");
		return args[arg_index]->get_address_space() == CL_PRIVATE;
	}
	inline bool arg_is_constant(const cl_uint arg_index) const {
		assert (arg_index < num_args);
		assert (args[arg_index] && "kernel object not completely initialized?");
		return args[arg_index]->get_address_space() == CL_CONSTANT;
	}
	inline void* arg_get_data(const cl_uint arg_index) const {
		assert (arg_index < num_args);
		assert (args[arg_index] && "kernel object not completely initialized?");
		return args[arg_index]->get_mem_address();
	}

	inline bool arg_is_uniform(const cl_uint arg_index) const {
		assert (arg_index < num_args);
		assert (args[arg_index] && "kernel object not completely initialized?");
		return args[arg_index]->is_uniform();
	}

};

struct _cl_event {
	_cl_context* context;
};


///////////////////////////////////////////////////////////////////////////
//              Packetized OpenCL Driver Implementation                  //
///////////////////////////////////////////////////////////////////////////

/* Platform API */
CL_API_ENTRY cl_int CL_API_CALL
clGetPlatformIDs(cl_uint          num_entries,
                 cl_platform_id * platforms,
                 cl_uint *        num_platforms) CL_API_SUFFIX__VERSION_1_0
{
	if (!platforms && !num_platforms) return CL_INVALID_VALUE;
	if (platforms && num_entries == 0) return CL_INVALID_VALUE;

	if (platforms) platforms[0] = new _cl_platform_id();
	if (num_platforms) *num_platforms = 1;

	return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clGetPlatformInfo(cl_platform_id   platform,
                  cl_platform_info param_name,
                  size_t           param_value_size,
                  void *           param_value,
                  size_t *         param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
	if (!platform) return CL_INVALID_PLATFORM; //effect implementation defined
	if (param_value && param_value_size < sizeof(char*)) return CL_INVALID_VALUE;

	switch (param_name) {
		case CL_PLATFORM_PROFILE:
			if (param_value) param_value = (void*)"FULL_PROFILE"; //or "EMBEDDED_PROFILE"
			break;
		case CL_PLATFORM_VERSION:
			if (param_value) param_value = (void*)"OpenCL 1.0 PACKETIZED OPENCL DRIVER";
			break;
		case CL_PLATFORM_NAME:
			if (param_value) param_value = (void*)"cpu";
			break;
		case CL_PLATFORM_VENDOR:
			if (param_value) param_value = (void*)"Saarland University";
			break;
		case CL_PLATFORM_EXTENSIONS:
			if (param_value) param_value = (void*)"";
			break;
		default:
			return CL_INVALID_VALUE;
	}

	return CL_SUCCESS;
}

/* Device APIs */
CL_API_ENTRY cl_int CL_API_CALL
clGetDeviceIDs(cl_platform_id   platform,
               cl_device_type   device_type,
               cl_uint          num_entries,
               cl_device_id *   devices,
               cl_uint *        num_devices) CL_API_SUFFIX__VERSION_1_0
{
	if (device_type != CL_DEVICE_TYPE_CPU) {
		errs() << "ERROR: packetized OpenCL driver can not handle devices other than CPU!\n";
		return CL_INVALID_DEVICE_TYPE;
	}
	if (devices && num_entries < 1) return CL_INVALID_VALUE;
	if (!devices && !num_devices) return CL_INVALID_VALUE;
	if (devices) devices = new cl_device_id();
	if (num_devices) num_devices = new cl_uint(1);
	return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clGetDeviceInfo(cl_device_id    device,
                cl_device_info  param_name,
                size_t          param_value_size,
                void *          param_value,
                size_t *        param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
	if (!device) return CL_INVALID_DEVICE;

	// TODO: move into _cl_device_id, this here is the wrong place !!
	switch (param_name) {
		case CL_DEVICE_TYPE: {
			if (param_value_size < sizeof(cl_device_type)) return CL_INVALID_VALUE;
			if (param_value) *(cl_device_type*)param_value = CL_DEVICE_TYPE_CPU;
			if (param_value_size_ret) *param_value_size_ret = sizeof(cl_device_type);
			break;
		}
		case CL_DEVICE_VENDOR_ID: {
			if (param_value_size < sizeof(cl_uint)) return CL_INVALID_VALUE;
			if (param_value) *(cl_uint*)param_value = 0; // should be some "unique device vendor identifier"
			if (param_value_size_ret) *param_value_size_ret = sizeof(cl_uint);
			break;
		}
		case CL_DEVICE_MAX_COMPUTE_UNITS: {
			if (param_value_size < sizeof(cl_uint)) return CL_INVALID_VALUE;

#ifdef PACKETIZED_OPENCL_DRIVER_NO_PACKETIZATION
			if (param_value) *(cl_uint*)param_value = PACKETIZED_OPENCL_DRIVER_NUM_CORES;
#else
	#ifndef PACKETIZED_OPENCL_DRIVER_USE_OPENMP
			if (param_value) *(cl_uint*)param_value = PACKETIZED_OPENCL_DRIVER_SIMD_WIDTH;
	#else
			if (param_value) *(cl_uint*)param_value = PACKETIZED_OPENCL_DRIVER_NUM_CORES*PACKETIZED_OPENCL_DRIVER_SIMD_WIDTH; // ? :P
	#endif
#endif

			if (param_value_size_ret) *param_value_size_ret = sizeof(cl_uint);
			break;
		}
		case CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS: {
			if (param_value_size < sizeof(cl_uint)) return CL_INVALID_VALUE;
			if (param_value) *(cl_uint*)param_value = PACKETIZED_OPENCL_DRIVER_MAX_NUM_DIMENSIONS;
			if (param_value_size_ret) *param_value_size_ret = sizeof(cl_uint);
			break;
		}
		case CL_DEVICE_MAX_WORK_ITEM_SIZES: {
			if (param_value_size < sizeof(size_t)) return CL_INVALID_VALUE;
			if (param_value) {
				for (unsigned i=0; i<PACKETIZED_OPENCL_DRIVER_MAX_NUM_DIMENSIONS; ++i) {
					((size_t*)param_value)[i] = PacketizedOpenCLDriver::getDeviceMaxMemAllocSize(); // TODO: FIXME
				}
			}
			if (param_value_size_ret) *param_value_size_ret = sizeof(size_t)*PACKETIZED_OPENCL_DRIVER_MAX_NUM_DIMENSIONS;
			break;
		}
		case CL_DEVICE_MAX_WORK_GROUP_SIZE: {
			if (param_value_size < sizeof(size_t)) return CL_INVALID_VALUE;
			if (param_value) *(size_t*)param_value = PacketizedOpenCLDriver::getDeviceMaxMemAllocSize(); // FIXME
			if (param_value_size_ret) *param_value_size_ret = sizeof(size_t*);
			break;
		}
		case CL_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR: {
			errs() << "ERROR: param_name '" << param_name << "' not implemented yet!\n";
			assert (false && "NOT IMPLEMENTED!");
			return CL_INVALID_VALUE;
		}
		case CL_DEVICE_PREFERRED_VECTOR_WIDTH_SHORT: {
			errs() << "ERROR: param_name '" << param_name << "' not implemented yet!\n";
			assert (false && "NOT IMPLEMENTED!");
			return CL_INVALID_VALUE;
		}
		case CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT: {
			errs() << "ERROR: param_name '" << param_name << "' not implemented yet!\n";
			assert (false && "NOT IMPLEMENTED!");
			return CL_INVALID_VALUE;
		}
		case CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT: {
			errs() << "ERROR: param_name '" << param_name << "' not implemented yet!\n";
			assert (false && "NOT IMPLEMENTED!");
			return CL_INVALID_VALUE;
		}
		case CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE: {
			errs() << "ERROR: param_name '" << param_name << "' not implemented yet!\n";
			assert (false && "NOT IMPLEMENTED!");
			return CL_INVALID_VALUE;
		}
		case CL_DEVICE_MAX_CLOCK_FREQUENCY: {
			errs() << "ERROR: param_name '" << param_name << "' not implemented yet!\n";
			assert (false && "NOT IMPLEMENTED!");
			return CL_INVALID_VALUE;
		}
		case CL_DEVICE_ADDRESS_BITS: {
			errs() << "ERROR: param_name '" << param_name << "' not implemented yet!\n";
			assert (false && "NOT IMPLEMENTED!");
			return CL_INVALID_VALUE;
		}
		case CL_DEVICE_MAX_MEM_ALLOC_SIZE: {
			errs() << "ERROR: param_name '" << param_name << "' not implemented yet!\n";
			assert (false && "NOT IMPLEMENTED!");
			return CL_INVALID_VALUE;
		}
		case CL_DEVICE_IMAGE_SUPPORT: {
			errs() << "ERROR: param_name '" << param_name << "' not implemented yet!\n";
			assert (false && "NOT IMPLEMENTED!");
			return CL_INVALID_VALUE;
		}
		case CL_DEVICE_MAX_READ_IMAGE_ARGS: {
			errs() << "ERROR: param_name '" << param_name << "' not implemented yet!\n";
			assert (false && "NOT IMPLEMENTED!");
			return CL_INVALID_VALUE;
		}
		case CL_DEVICE_MAX_WRITE_IMAGE_ARGS: {
			errs() << "ERROR: param_name '" << param_name << "' not implemented yet!\n";
			assert (false && "NOT IMPLEMENTED!");
			return CL_INVALID_VALUE;
		}
		case CL_DEVICE_IMAGE2D_MAX_WIDTH: {
			errs() << "ERROR: param_name '" << param_name << "' not implemented yet!\n";
			assert (false && "NOT IMPLEMENTED!");
			return CL_INVALID_VALUE;
		}
		case CL_DEVICE_IMAGE2D_MAX_HEIGHT: {
			errs() << "ERROR: param_name '" << param_name << "' not implemented yet!\n";
			assert (false && "NOT IMPLEMENTED!");
			return CL_INVALID_VALUE;
		}
		case CL_DEVICE_IMAGE3D_MAX_WIDTH: {
			errs() << "ERROR: param_name '" << param_name << "' not implemented yet!\n";
			assert (false && "NOT IMPLEMENTED!");
			return CL_INVALID_VALUE;
		}
		case CL_DEVICE_IMAGE3D_MAX_HEIGHT: {
			errs() << "ERROR: param_name '" << param_name << "' not implemented yet!\n";
			assert (false && "NOT IMPLEMENTED!");
			return CL_INVALID_VALUE;
		}
		case CL_DEVICE_IMAGE3D_MAX_DEPTH: {
			errs() << "ERROR: param_name '" << param_name << "' not implemented yet!\n";
			assert (false && "NOT IMPLEMENTED!");
			return CL_INVALID_VALUE;
		}
		case CL_DEVICE_MAX_SAMPLERS: {
			errs() << "ERROR: param_name '" << param_name << "' not implemented yet!\n";
			assert (false && "NOT IMPLEMENTED!");
			return CL_INVALID_VALUE;
		}
		case CL_DEVICE_MAX_PARAMETER_SIZE: {
			errs() << "ERROR: param_name '" << param_name << "' not implemented yet!\n";
			assert (false && "NOT IMPLEMENTED!");
			return CL_INVALID_VALUE;
		}
		case CL_DEVICE_MEM_BASE_ADDR_ALIGN: {
			errs() << "ERROR: param_name '" << param_name << "' not implemented yet!\n";
			assert (false && "NOT IMPLEMENTED!");
			return CL_INVALID_VALUE;
		}
		case CL_DEVICE_MIN_DATA_TYPE_ALIGN_SIZE: {
			errs() << "ERROR: param_name '" << param_name << "' not implemented yet!\n";
			assert (false && "NOT IMPLEMENTED!");
			return CL_INVALID_VALUE;
		}
		case CL_DEVICE_SINGLE_FP_CONFIG: {
			errs() << "ERROR: param_name '" << param_name << "' not implemented yet!\n";
			assert (false && "NOT IMPLEMENTED!");
			return CL_INVALID_VALUE;
		}
		case CL_DEVICE_GLOBAL_MEM_CACHE_TYPE: {
			errs() << "ERROR: param_name '" << param_name << "' not implemented yet!\n";
			assert (false && "NOT IMPLEMENTED!");
			return CL_INVALID_VALUE;
		}
		case CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE: {
			errs() << "ERROR: param_name '" << param_name << "' not implemented yet!\n";
			assert (false && "NOT IMPLEMENTED!");
			return CL_INVALID_VALUE;
		}
		case CL_DEVICE_GLOBAL_MEM_CACHE_SIZE: {
			errs() << "ERROR: param_name '" << param_name << "' not implemented yet!\n";
			assert (false && "NOT IMPLEMENTED!");
			return CL_INVALID_VALUE;
		}
		case CL_DEVICE_GLOBAL_MEM_SIZE: {
			errs() << "ERROR: param_name '" << param_name << "' not implemented yet!\n";
			assert (false && "NOT IMPLEMENTED!");
			return CL_INVALID_VALUE;
		}
		case CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE: {
			errs() << "ERROR: param_name '" << param_name << "' not implemented yet!\n";
			assert (false && "NOT IMPLEMENTED!");
			return CL_INVALID_VALUE;
		}
		case CL_DEVICE_MAX_CONSTANT_ARGS: {
			errs() << "ERROR: param_name '" << param_name << "' not implemented yet!\n";
			assert (false && "NOT IMPLEMENTED!");
			return CL_INVALID_VALUE;
		}
		case CL_DEVICE_LOCAL_MEM_TYPE: {
			errs() << "ERROR: param_name '" << param_name << "' not implemented yet!\n";
			assert (false && "NOT IMPLEMENTED!");
			return CL_INVALID_VALUE;
		}
		case CL_DEVICE_LOCAL_MEM_SIZE: {
			// local memory == global memory for cpu
			// TODO: make sure size*maxNumThreads(*simdWidth?) is really available on host ;)
			if (param_value_size < sizeof(unsigned long long)) return CL_INVALID_VALUE;
			if (param_value) *(unsigned long long*)param_value = PacketizedOpenCLDriver::getDeviceMaxMemAllocSize(); // FIXME: use own function
			if (param_value_size_ret) *param_value_size_ret = sizeof(unsigned long long);
			break;
		}
		case CL_DEVICE_ERROR_CORRECTION_SUPPORT: {
			errs() << "ERROR: param_name '" << param_name << "' not implemented yet!\n";
			assert (false && "NOT IMPLEMENTED!");
			return CL_INVALID_VALUE;
		}
		case CL_DEVICE_PROFILING_TIMER_RESOLUTION: {
			errs() << "ERROR: param_name '" << param_name << "' not implemented yet!\n";
			assert (false && "NOT IMPLEMENTED!");
			return CL_INVALID_VALUE;
		}
		case CL_DEVICE_ENDIAN_LITTLE: {
			errs() << "ERROR: param_name '" << param_name << "' not implemented yet!\n";
			assert (false && "NOT IMPLEMENTED!");
			return CL_INVALID_VALUE;
		}
		case CL_DEVICE_AVAILABLE: {
			if (param_value_size < sizeof(cl_bool)) return CL_INVALID_VALUE;
			if (param_value) *(cl_bool*)param_value = true; // TODO: check if cpu supports SSE
			if (param_value_size_ret) *param_value_size_ret = sizeof(cl_bool);
			break;
		}
		case CL_DEVICE_COMPILER_AVAILABLE: {
			if (param_value_size < sizeof(cl_bool)) return CL_INVALID_VALUE;
			if (param_value) *(cl_bool*)param_value = true; // TODO: check if clang/llvm is available
			if (param_value_size_ret) *param_value_size_ret = sizeof(cl_bool);
			break;
		}
		case CL_DEVICE_EXECUTION_CAPABILITIES: {
			errs() << "ERROR: param_name '" << param_name << "' not implemented yet!\n";
			assert (false && "NOT IMPLEMENTED!");
			return CL_INVALID_VALUE;
		}
		case CL_DEVICE_QUEUE_PROPERTIES: {
			errs() << "ERROR: param_name '" << param_name << "' not implemented yet!\n";
			assert (false && "NOT IMPLEMENTED!");
			return CL_INVALID_VALUE;
		}
		case CL_DEVICE_PLATFORM: {
			errs() << "ERROR: param_name '" << param_name << "' not implemented yet!\n";
			assert (false && "NOT IMPLEMENTED!");
			return CL_INVALID_VALUE;
		}
		case CL_DEVICE_NAME: {
			if (param_value_size < sizeof(char*)) return CL_INVALID_VALUE;
			if (param_value) *(std::string*)param_value = "sse cpu"; // TODO: should just return "cpu"?
			if (param_value_size_ret) *param_value_size_ret = sizeof(char*);
			break;
		}
		case CL_DEVICE_VENDOR: {
			errs() << "ERROR: param_name '" << param_name << "' not implemented yet!\n";
			assert (false && "NOT IMPLEMENTED!");
			return CL_INVALID_VALUE;
		}
		case CL_DRIVER_VERSION: {
			if (param_value_size < sizeof(char*)) return CL_INVALID_VALUE;
			if (param_value) *(std::string*)param_value = PACKETIZED_OPENCL_DRIVER_VERSION_STRING;
			if (param_value_size_ret) *param_value_size_ret = sizeof(char*);
			break;
		}
		case CL_DEVICE_PROFILE: {
			errs() << "ERROR: param_name '" << param_name << "' not implemented yet!\n";
			assert (false && "NOT IMPLEMENTED!");
			return CL_INVALID_VALUE;
		}
		case CL_DEVICE_VERSION: {
			if (param_value_size < sizeof(char*)) return CL_INVALID_VALUE;
			if (param_value) *(std::string*)param_value = "OpenCL 1.0 Packetized";
			if (param_value_size_ret) *param_value_size_ret = sizeof(char*);
			break;
		}
		case CL_DEVICE_EXTENSIONS: {
			if (param_value_size < sizeof(char*)) return CL_INVALID_VALUE;
			if (param_value) strcpy((char*)param_value, PACKETIZED_OPENCL_DRIVER_EXTENSIONS);
			if (param_value_size_ret) *param_value_size_ret = sizeof(char*);
			break;
		}

		default: {
			errs() << "ERROR: unknown param_name found: " << param_name << "!\n";
			return CL_INVALID_VALUE;
		}
	}

	return CL_SUCCESS;
}

/* Context APIs  */
CL_API_ENTRY cl_context CL_API_CALL
clCreateContext(const cl_context_properties * properties,
                cl_uint                       num_devices,
                const cl_device_id *          devices,
                void (*pfn_notify)(const char *, const void *, size_t, void *),
                void *                        user_data,
                cl_int *                      errcode_ret) CL_API_SUFFIX__VERSION_1_0
{
	*errcode_ret = CL_SUCCESS;
	return new _cl_context();
}

CL_API_ENTRY cl_context CL_API_CALL
clCreateContextFromType(const cl_context_properties * properties,
                        cl_device_type                device_type,
                        void (*pfn_notify)(const char *, const void *, size_t, void *),
                        void *                        user_data,
                        cl_int *                      errcode_ret) CL_API_SUFFIX__VERSION_1_0
{
	if (!pfn_notify && user_data) { *errcode_ret = CL_INVALID_VALUE; return NULL; }

	if (device_type != CL_DEVICE_TYPE_CPU) { *errcode_ret = CL_DEVICE_NOT_AVAILABLE; return NULL; }

	*errcode_ret = CL_SUCCESS;
	return new _cl_context();
}

CL_API_ENTRY cl_int CL_API_CALL
clRetainContext(cl_context context) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clReleaseContext(cl_context context) CL_API_SUFFIX__VERSION_1_0
{
	PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "TODO: implement clReleaseContext()\n"; );
	return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clGetContextInfo(cl_context         context,
                 cl_context_info    param_name,
                 size_t             param_value_size,
                 void *             param_value,
                 size_t *           param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
	PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "TODO: implement clGetContextInfo()\n"; );
	if (param_value_size_ret) *param_value_size_ret = 4;
	return CL_SUCCESS;
}

/* Command Queue APIs */

/*
creates a command-queue on a specific device.
*/
// -> ??
CL_API_ENTRY cl_command_queue CL_API_CALL
clCreateCommandQueue(cl_context                     context,
                     cl_device_id                   device,
                     cl_command_queue_properties    properties,
                     cl_int *                       errcode_ret) CL_API_SUFFIX__VERSION_1_0
{
	errcode_ret = CL_SUCCESS;
	_cl_command_queue* cq = new _cl_command_queue();
	cq->context = context;
	return cq;
}

CL_API_ENTRY cl_int CL_API_CALL
clRetainCommandQueue(cl_command_queue command_queue) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clReleaseCommandQueue(cl_command_queue command_queue) CL_API_SUFFIX__VERSION_1_0
{
	PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "TODO: implement clReleaseCommandQueue()\n"; );
	return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clGetCommandQueueInfo(cl_command_queue      command_queue,
                      cl_command_queue_info param_name,
                      size_t                param_value_size,
                      void *                param_value,
                      size_t *              param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clSetCommandQueueProperty(cl_command_queue              command_queue,
                          cl_command_queue_properties   properties,
                          cl_bool                        enable,
                          cl_command_queue_properties * old_properties) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

/* Memory Object APIs  */

/*
Memory objects are categorized into two types: buffer objects, and image objects. A buffer
object stores a one-dimensional collection of elements whereas an image object is used to store a
two- or three- dimensional texture, frame-buffer or image.
Elements of a buffer object can be a scalar data type (such as an int, float), vector data type, or a
user-defined structure. An image object is used to represent a buffer that can be used as a texture
or a frame-buffer. The elements of an image object are selected from a list of predefined image
formats. The minimum number of elements in a memory object is one.
*/
CL_API_ENTRY cl_mem CL_API_CALL
clCreateBuffer(cl_context   context,
               cl_mem_flags flags,
               size_t       size, //in bytes
               void *       host_ptr,
               cl_int *     errcode_ret) CL_API_SUFFIX__VERSION_1_0
{
	if (!context) { if (errcode_ret) *errcode_ret = CL_INVALID_CONTEXT; return NULL; }
	if (size == 0 || size > PacketizedOpenCLDriver::getDeviceMaxMemAllocSize()) { if (errcode_ret) *errcode_ret = CL_INVALID_BUFFER_SIZE; return NULL; }
	const bool useHostPtr   = flags & CL_MEM_USE_HOST_PTR;
	const bool copyHostPtr  = flags & CL_MEM_COPY_HOST_PTR;
	const bool allocHostPtr = flags & CL_MEM_ALLOC_HOST_PTR;
	if (!host_ptr && (useHostPtr || copyHostPtr)) { if (errcode_ret) *errcode_ret = CL_INVALID_HOST_PTR; return NULL; }
	if (host_ptr && !useHostPtr && !copyHostPtr) { if (errcode_ret) *errcode_ret = CL_INVALID_HOST_PTR; return NULL; }
	if (useHostPtr && allocHostPtr) { if (errcode_ret) *errcode_ret = CL_INVALID_VALUE; return NULL; } // custom
	if (useHostPtr && copyHostPtr) { if (errcode_ret) *errcode_ret = CL_INVALID_VALUE; return NULL; } // custom

	const bool canRead     = (flags & CL_MEM_READ_ONLY) || (flags & CL_MEM_READ_WRITE);
	const bool canWrite    = (flags & CL_MEM_WRITE_ONLY) || (flags & CL_MEM_READ_WRITE);

	PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "clCreateBuffer(" << size << " bytes, " << host_ptr << ")\n"; );
	PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "  canRead     : " << (canRead ? "true" : "false") << "\n"; );
	PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "  canWrite    : " << (canWrite ? "true" : "false") << "\n"; );
	PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "  useHostPtr  : " << (useHostPtr ? "true" : "false") << "\n"; );
	PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "  copyHostPtr : " << (copyHostPtr ? "true" : "false") << "\n"; );
	PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "  allocHostPtr: " << (allocHostPtr ? "true" : "false") << "\n"; );

	void* device_ptr = NULL;

	if (useHostPtr) {
		assert (host_ptr);
		device_ptr = host_ptr;
		PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "    using supplied host ptr: " << device_ptr << "\n"; );
	}

	if (allocHostPtr) {
		device_ptr = malloc(size);
		PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "    new host ptr allocated: " << device_ptr << "\n"; );
		if (!device_ptr) { if (errcode_ret) *errcode_ret = CL_MEM_OBJECT_ALLOCATION_FAILURE; return NULL; }
	}

	if (copyHostPtr) {
		// CL_MEM_COPY_HOST_PTR can be used with
		// CL_MEM_ALLOC_HOST_PTR to initialize the contents of
		// the cl_mem object allocated using host-accessible (e.g.
		// PCIe) memory.
		assert (host_ptr);
		if (!allocHostPtr) {
			device_ptr = malloc(size);
			PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "    new host ptr allocated for copying: " << device_ptr << "\n"; );
			if (!device_ptr) { if (errcode_ret) *errcode_ret = CL_MEM_OBJECT_ALLOCATION_FAILURE; return NULL; }
		}
		// copy data into new_host_ptr
		PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "    copying data of supplied host ptr to new host ptr... "; );
		memcpy(device_ptr, host_ptr, size);
		PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "done.\n"; );
	}

	// if no flag was supplied, allocate memory (host_ptr must be NULL by specification)
	if (!device_ptr) {
		assert (!host_ptr);
		device_ptr = malloc(size);
		PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "    new host ptr allocated (no flag specified): " << device_ptr << "\n"; );
		if (!device_ptr) { if (errcode_ret) *errcode_ret = CL_MEM_OBJECT_ALLOCATION_FAILURE; return NULL; }
	}

	if (errcode_ret) *errcode_ret = CL_SUCCESS;
	return new _cl_mem(context, size, device_ptr, canRead, canWrite);
}

CL_API_ENTRY cl_mem CL_API_CALL
clCreateImage2D(cl_context              context,
                cl_mem_flags            flags,
                const cl_image_format * image_format,
                size_t                  image_width,
                size_t                  image_height,
                size_t                  image_row_pitch,
                void *                  host_ptr,
                cl_int *                errcode_ret) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return NULL;
}

CL_API_ENTRY cl_mem CL_API_CALL
clCreateImage3D(cl_context              context,
                cl_mem_flags            flags,
                const cl_image_format * image_format,
                size_t                  image_width,
                size_t                  image_height,
                size_t                  image_depth,
                size_t                  image_row_pitch,
                size_t                  image_slice_pitch,
                void *                  host_ptr,
                cl_int *                errcode_ret) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return NULL;
}

CL_API_ENTRY cl_int CL_API_CALL
clRetainMemObject(cl_mem memobj) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clReleaseMemObject(cl_mem memobj) CL_API_SUFFIX__VERSION_1_0
{
	PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "TODO: implement clReleaseMemObject()\n"; );
	return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clGetSupportedImageFormats(cl_context           context,
                           cl_mem_flags         flags,
                           cl_mem_object_type   image_type,
                           cl_uint              num_entries,
                           cl_image_format *    image_formats,
                           cl_uint *            num_image_formats) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clGetMemObjectInfo(cl_mem           memobj,
                   cl_mem_info      param_name,
                   size_t           param_value_size,
                   void *           param_value,
                   size_t *         param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clGetImageInfo(cl_mem           image,
               cl_image_info    param_name,
               size_t           param_value_size,
               void *           param_value,
               size_t *         param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

/* Sampler APIs  */
CL_API_ENTRY cl_sampler CL_API_CALL
clCreateSampler(cl_context          context,
                cl_bool             normalized_coords,
                cl_addressing_mode  addressing_mode,
                cl_filter_mode      filter_mode,
                cl_int *            errcode_ret) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return NULL;
}

CL_API_ENTRY cl_int CL_API_CALL
clRetainSampler(cl_sampler sampler) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clReleaseSampler(cl_sampler sampler) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clGetSamplerInfo(cl_sampler         sampler,
                 cl_sampler_info    param_name,
                 size_t             param_value_size,
                 void *             param_value,
                 size_t *           param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

/* Program Object APIs  */

/*
creates a program object for a context, and loads the source code specified by the text strings in
the strings array into the program object. The devices associated with the program object are the
devices associated with context.
*/
// -> read strings and store as .cl representation
CL_API_ENTRY cl_program CL_API_CALL
clCreateProgramWithSource(cl_context        context,
                          cl_uint           count,
                          const char **     strings,
                          const size_t *    lengths,
                          cl_int *          errcode_ret) CL_API_SUFFIX__VERSION_1_0
{
	errcode_ret = CL_SUCCESS;
	_cl_program* p = new _cl_program();
	p->context = context;
	p->fileName = *strings;
	return p;
}

// -> read binary and store as .cl representation
CL_API_ENTRY cl_program CL_API_CALL
clCreateProgramWithBinary(cl_context                     context,
                          cl_uint                        num_devices,
                          const cl_device_id *           device_list,
                          const size_t *                 lengths,
                          const unsigned char **         binaries,
                          cl_int *                       binary_status,
                          cl_int *                       errcode_ret) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return NULL;
}

CL_API_ENTRY cl_int CL_API_CALL
clRetainProgram(cl_program program) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clReleaseProgram(cl_program program) CL_API_SUFFIX__VERSION_1_0
{
	PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "TODO: implement clReleaseProgram()\n"; );
	return CL_SUCCESS;
}

/*
builds (compiles & links) a program executable from the program source or binary for all the
devices or a specific device(s) in the OpenCL context associated with program. OpenCL allows
program executables to be built using the source or the binary. clBuildProgram must be called
for program created using either clCreateProgramWithSource or
clCreateProgramWithBinary to build the program executable for one or more devices
associated with program.
*/
// -> build LLVM module from .cl representation (from createProgramWithSource/Binary)
// -> invoke clc
// -> invoke llvm-as
// -> store module in _cl_program object
CL_API_ENTRY cl_int CL_API_CALL
clBuildProgram(cl_program           program,
               cl_uint              num_devices,
               const cl_device_id * device_list,
               const char *         options,
               void (*pfn_notify)(cl_program program, void * user_data),
               void *               user_data) CL_API_SUFFIX__VERSION_1_0
{
	if (!program) return CL_INVALID_PROGRAM;
	if (!device_list && num_devices > 0) return CL_INVALID_VALUE;
	if (device_list && num_devices == 0) return CL_INVALID_VALUE;
	if (user_data && !pfn_notify) return CL_INVALID_VALUE;

	// TODO: read .cl representation, invoke clc, invoke llvm-as
	// alternative: link libClang and use it directly from here :)

	//FIXME: hardcoded for testing ;)
	llvm::Module* mod = PacketizedOpenCLDriver::createModuleFromFile(program->fileName);
	if (!mod) return CL_BUILD_PROGRAM_FAILURE;

	// TODO: do this here or only after packetization?
	mod->setDataLayout(PACKETIZED_OPENCL_DRIVER_LLVM_DATA_LAYOUT_64);
	// we have to reset the target triple (LLVM does not know amd-opencl)
	mod->setTargetTriple("");
	program->targetData = new TargetData(mod);

	program->module = mod;
	return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clUnloadCompiler(void) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clGetProgramInfo(cl_program         program,
                 cl_program_info    param_name,
                 size_t             param_value_size,
                 void *             param_value,
                 size_t *           param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clGetProgramBuildInfo(cl_program            program,
                      cl_device_id          device,
                      cl_program_build_info param_name,
                      size_t                param_value_size,
                      void *                param_value,
                      size_t *              param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

/* Kernel Object APIs */

// -> compile bitcode of function from .bc file to native code
// -> store void* in _cl_kernel object
CL_API_ENTRY cl_kernel CL_API_CALL
clCreateKernel(cl_program      program,
               const char *    kernel_name,
               cl_int *        errcode_ret) CL_API_SUFFIX__VERSION_1_0
{
	if (!program) { *errcode_ret = CL_INVALID_PROGRAM; return NULL; }
	if (!program->module) { *errcode_ret = CL_INVALID_PROGRAM_EXECUTABLE; return NULL; }
	PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "\nclCreateKernel(" << program->module->getModuleIdentifier() << ", " << kernel_name << ")\n"; );

	// does the returned error code mean we should compile before??
	llvm::Module* module = program->module;

	if (!kernel_name) { *errcode_ret = CL_INVALID_VALUE; return NULL; }

	std::stringstream strs;
	strs << "__OpenCL_" << kernel_name << "_kernel";
	const std::string new_kernel_name = strs.str();

	llvm::Function* f = PacketizedOpenCLDriver::getFunction(new_kernel_name, module);
	if (!f) { *errcode_ret = CL_INVALID_KERNEL_NAME; return NULL; }

	// optimize kernel // TODO: not necessary if we optimize wrapper afterwards
	PacketizedOpenCLDriver::inlineFunctionCalls(f, program->targetData);
	PacketizedOpenCLDriver::optimizeFunction(f);

	PACKETIZED_OPENCL_DRIVER_DEBUG( PacketizedOpenCLDriver::writeFunctionToFile(f, "debug_kernel_orig.ll"); );

	LLVMContext& context = module->getContext();

	// determine number of dimensions required by kernel
	const unsigned num_dimensions = PacketizedOpenCLDriver::determineNumDimensionsUsed(f);


#ifdef PACKETIZED_OPENCL_DRIVER_NO_PACKETIZATION

	llvm::Function* f_wrapper = PacketizedOpenCLDriver::createKernelSequential(f, kernel_name, num_dimensions, module, program->targetData, context, errcode_ret);
	if (!f_wrapper) {
		errs() << "ERROR: kernel generation failed!\n";
		return NULL;
	}

	_cl_kernel* kernel = new _cl_kernel(program->context, program, f, f_wrapper);
	kernel->set_num_dimensions(num_dimensions);

#else

	// determine best dimension for packetization
	const unsigned simd_dim = PacketizedOpenCLDriver::getBestSimdDim(f, num_dimensions);

	llvm::Function* f_SIMD = NULL;
	llvm::Function* f_wrapper = PacketizedOpenCLDriver::createKernelPacketized(f, kernel_name, num_dimensions, simd_dim, module, program->targetData, context, errcode_ret, &f_SIMD);
	if (!f_wrapper || !f_SIMD) {
		errs() << "ERROR: kernel generation failed!\n";
		return NULL;
	}

	_cl_kernel* kernel = new _cl_kernel(program->context, program, f, f_wrapper, f_SIMD);
	kernel->set_num_dimensions(num_dimensions);
	kernel->set_best_simd_dim(simd_dim);

#endif

	if (!kernel->get_compiled_function()) { *errcode_ret = CL_INVALID_PROGRAM_EXECUTABLE; return NULL; }

	*errcode_ret = CL_SUCCESS;
	return kernel;
}

CL_API_ENTRY cl_int CL_API_CALL
clCreateKernelsInProgram(cl_program     program,
                         cl_uint        num_kernels,
                         cl_kernel *    kernels,
                         cl_uint *      num_kernels_ret) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clRetainKernel(cl_kernel    kernel) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clReleaseKernel(cl_kernel   kernel) CL_API_SUFFIX__VERSION_1_0
{
	PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "TODO: implement clReleaseKernel()\n"; );
	return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clSetKernelArg(cl_kernel    kernel,
               cl_uint      arg_index,
               size_t       arg_size,
               const void * arg_value) CL_API_SUFFIX__VERSION_1_0
{
	PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "\nclSetKernelArg(" << kernel->function_wrapper->getNameStr() << ", " << arg_index << ", " << arg_size << ")\n"; );
	if (!kernel) return CL_INVALID_KERNEL;
	if (arg_index > kernel->get_num_args()) return CL_INVALID_ARG_INDEX;

	kernel->set_arg_data(arg_index, arg_value, arg_size);
	return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clGetKernelInfo(cl_kernel       kernel,
                cl_kernel_info  param_name,
                size_t          param_value_size,
                void *          param_value,
                size_t *        param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clGetKernelWorkGroupInfo(cl_kernel                  kernel,
                         cl_device_id               device,
                         cl_kernel_work_group_info  param_name,
                         size_t                     param_value_size,
                         void *                     param_value,
                         size_t *                   param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
	if (!kernel) return CL_INVALID_KERNEL;
	//if (!device) return CL_INVALID_DEVICE;
	switch (param_name) {
		case CL_KERNEL_WORK_GROUP_SIZE:{
			*(size_t*)param_value = PACKETIZED_OPENCL_DRIVER_MAX_WORK_GROUP_SIZE; //simdWidth * maxNumThreads;
			break; // type conversion slightly hacked (should use param_value_size) ;)
		}
		case CL_KERNEL_COMPILE_WORK_GROUP_SIZE: {
			assert (false && "NOT IMPLEMENTED");
			break;
		}
		case CL_KERNEL_LOCAL_MEM_SIZE: {
			*(cl_ulong*)param_value = 0; // FIXME ?
			break;
		}
		default: return CL_INVALID_VALUE;
	}
	return CL_SUCCESS;
}

/* Event Object APIs  */
CL_API_ENTRY cl_int CL_API_CALL
clWaitForEvents(cl_uint             num_events,
                const cl_event *    event_list) CL_API_SUFFIX__VERSION_1_0
{
	PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "TODO: implement clWaitForEvents()\n"; );
	return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clGetEventInfo(cl_event         event,
               cl_event_info    param_name,
               size_t           param_value_size,
               void *           param_value,
               size_t *         param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clRetainEvent(cl_event event) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clReleaseEvent(cl_event event) CL_API_SUFFIX__VERSION_1_0
{
	PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "TODO: implement clReleaseEvent()\n"; );
	return CL_SUCCESS;
}

/* Profiling APIs  */
CL_API_ENTRY cl_int CL_API_CALL
clGetEventProfilingInfo(cl_event            event,
                        cl_profiling_info   param_name,
                        size_t              param_value_size,
                        void *              param_value,
                        size_t *            param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

/* Flush and Finish APIs */
CL_API_ENTRY cl_int CL_API_CALL
clFlush(cl_command_queue command_queue) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clFinish(cl_command_queue command_queue) CL_API_SUFFIX__VERSION_1_0
{
	if (!command_queue) return CL_INVALID_COMMAND_QUEUE;
	// do nothing :P
	return CL_SUCCESS;
}

/* Enqueued Commands APIs */
CL_API_ENTRY cl_int CL_API_CALL
clEnqueueReadBuffer(cl_command_queue    command_queue,
                    cl_mem              buffer,
                    cl_bool             blocking_read,
                    size_t              offset,
                    size_t              cb,
                    void *              ptr,
                    cl_uint             num_events_in_wait_list,
                    const cl_event *    event_wait_list,
                    cl_event *          event) CL_API_SUFFIX__VERSION_1_0
{
	if (!command_queue) return CL_INVALID_COMMAND_QUEUE;
	if (!buffer) return CL_INVALID_MEM_OBJECT;
	if (!ptr || buffer->get_size() < cb+offset) return CL_INVALID_VALUE;
	if (!event_wait_list && num_events_in_wait_list > 0) return CL_INVALID_EVENT_WAIT_LIST;
	if (event_wait_list && num_events_in_wait_list == 0) return CL_INVALID_EVENT_WAIT_LIST;
	if (command_queue->context != buffer->get_context()) return CL_INVALID_CONTEXT;
    //err = clEnqueueReadBuffer( commands, output, CL_TRUE, 0, sizeof(float) * count, results, 0, NULL, NULL );

	// Write data back into host memory (ptr) from device memory (buffer)
	// In our case, we actually should not have to copy data
	// because we are still on the CPU. However, const void* prevents this.
	// Thus, just copy over each byte.
	// TODO: specification seems to require something different?
	//       storing access patterns to command_queue or sth like that?
	
	void* data = buffer->get_data();
	memcpy(ptr, data, cb);

	return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueWriteBuffer(cl_command_queue   command_queue,
                     cl_mem             buffer,
                     cl_bool            blocking_write,
                     size_t             offset,
                     size_t             cb,
                     const void *       ptr,
                     cl_uint            num_events_in_wait_list,
                     const cl_event *   event_wait_list,
                     cl_event *         event) CL_API_SUFFIX__VERSION_1_0
{
	if (!command_queue) return CL_INVALID_COMMAND_QUEUE;
	if (!buffer) return CL_INVALID_MEM_OBJECT;
	if (!ptr || buffer->get_size() < cb+offset) return CL_INVALID_VALUE;
	if (!event_wait_list && num_events_in_wait_list > 0) return CL_INVALID_EVENT_WAIT_LIST;
	if (event_wait_list && num_events_in_wait_list == 0) return CL_INVALID_EVENT_WAIT_LIST;
	if (command_queue->context != buffer->get_context()) return CL_INVALID_CONTEXT;

	// Write data into 'device memory' (buffer)
	// In our case, we actually should not have to copy data
	// because we are still on the CPU. However, const void* prevents this.
	// Thus, just copy over each byte.
	// TODO: specification seems to require something different?
	//       storing access patterns to command_queue or sth like that?
	buffer->copy_data(ptr, cb, offset); //cb is size in bytes

	return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueCopyBuffer(cl_command_queue    command_queue,
                    cl_mem              src_buffer,
                    cl_mem              dst_buffer,
                    size_t              src_offset,
                    size_t              dst_offset,
                    size_t              cb,
                    cl_uint             num_events_in_wait_list,
                    const cl_event *    event_wait_list,
                    cl_event *          event) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueReadImage(cl_command_queue     command_queue,
                   cl_mem               image,
                   cl_bool              blocking_read,
                   const size_t         origin[3],
                   const size_t         region[3],
                   size_t               row_pitch,
                   size_t               slice_pitch,
                   void *               ptr,
                   cl_uint              num_events_in_wait_list,
                   const cl_event *     event_wait_list,
                   cl_event *           event) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueWriteImage(cl_command_queue    command_queue,
                    cl_mem              image,
                    cl_bool             blocking_write,
                    const size_t        origin[3],
                    const size_t        region[3],
                    size_t              input_row_pitch,
                    size_t              input_slice_pitch,
                    const void *        ptr,
                    cl_uint             num_events_in_wait_list,
                    const cl_event *    event_wait_list,
                    cl_event *          event) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueCopyImage(cl_command_queue     command_queue,
                   cl_mem               src_image,
                   cl_mem               dst_image,
                   const size_t         src_origin[3],
                   const size_t         dst_origin[3],
                   const size_t         region[3],
                   cl_uint              num_events_in_wait_list,
                   const cl_event *     event_wait_list,
                   cl_event *           event) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueCopyImageToBuffer(cl_command_queue command_queue,
                           cl_mem           src_image,
                           cl_mem           dst_buffer,
                           const size_t     src_origin[3],
                           const size_t     region[3],
                           size_t           dst_offset,
                           cl_uint          num_events_in_wait_list,
                           const cl_event * event_wait_list,
                           cl_event *       event) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueCopyBufferToImage(cl_command_queue command_queue,
                           cl_mem           src_buffer,
                           cl_mem           dst_image,
                           size_t           src_offset,
                           const size_t     dst_origin[3],
                           const size_t     region[3],
                           cl_uint          num_events_in_wait_list,
                           const cl_event * event_wait_list,
                           cl_event *       event) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

CL_API_ENTRY void * CL_API_CALL
clEnqueueMapBuffer(cl_command_queue command_queue,
                   cl_mem           buffer,
                   cl_bool          blocking_map,
                   cl_map_flags     map_flags,
                   size_t           offset,
                   size_t           cb,
                   cl_uint          num_events_in_wait_list,
                   const cl_event * event_wait_list,
                   cl_event *       event,
                   cl_int *         errcode_ret) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return NULL;
}

CL_API_ENTRY void * CL_API_CALL
clEnqueueMapImage(cl_command_queue  command_queue,
                  cl_mem            image,
                  cl_bool           blocking_map,
                  cl_map_flags      map_flags,
                  const size_t      origin[3],
                  const size_t      region[3],
                  size_t *          image_row_pitch,
                  size_t *          image_slice_pitch,
                  cl_uint           num_events_in_wait_list,
                  const cl_event *  event_wait_list,
                  cl_event *        event,
                  cl_int *          errcode_ret) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return NULL;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueUnmapMemObject(cl_command_queue command_queue,
                        cl_mem           memobj,
                        void *           mapped_ptr,
                        cl_uint          num_events_in_wait_list,
                        const cl_event *  event_wait_list,
                        cl_event *        event) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}


#if 0
inline cl_int executeRangeKernelNDOLD(cl_kernel kernel, cl_uint num_dimensions, const size_t* global_work_size, const size_t* local_work_size) {

	#ifdef PACKETIZED_OPENCL_DRIVER_USE_OPENMP
	PACKETIZED_OPENCL_DRIVER_DEBUG( errs() << "WARNING: clEnqueueNDRangeKernels with work_dim > 1 currently does not support multithreading - falling back to single-thread mode!\n"; );
	#endif

	typedef void (*kernelFnPtr)(
			const void*,
			const cl_uint,
			const cl_uint*,
			const cl_uint*,
			const cl_uint*,
			const cl_uint*,
			const cl_uint*,
			const cl_uint*);
	kernelFnPtr typedPtr = ptr_cast<kernelFnPtr>(kernel->get_compiled_function());

	const void* argument_struct = kernel->get_argument_struct();

	// TODO: move allocation somewhere else
	size_t* num_groups = new size_t[num_dimensions](); // #groups per dimension
	size_t* cur_global = new size_t[num_dimensions](); // ids per dimension
	size_t* cur_local = new size_t[num_dimensions](); // ids per dimension
	size_t* cur_group = new size_t[num_dimensions](); // ids per dimension

	for (unsigned cur_work_dim=0; cur_work_dim < num_dimensions; ++cur_work_dim) {
		const size_t groupnr = global_work_size[cur_work_dim] / local_work_size[cur_work_dim];
		// if local size is larger than global size, groupnr is 0 but we have one group ;)
		num_groups[cur_work_dim] = groupnr == 0 ? 1 : groupnr;

		cur_global[cur_work_dim] = 0;
		cur_local[cur_work_dim] = 0;
		cur_group[cur_work_dim] = 0;
	}


	bool kernel_finished = false;
	do {

		bool group_finished = false;
		do {
			PACKETIZED_OPENCL_DRIVER_DEBUG(
				outs() << "\nexecuting kernel...\n  global:";
				for (unsigned i=0; i<num_dimensions; ++i) {
					outs() << " " << get_global_id(i);
				}
				outs() << "\n  local:";
				for (unsigned i=0; i<num_dimensions; ++i) {
					outs() << " " << get_local_id(i);
				}
				outs() << "\n  group:";
				for (unsigned i=0; i<num_dimensions; ++i) {
					outs() << " " << get_group_id(i);
				}
				outs() << "\n";
			);

			// execute kernel
			const cl_uint argument_get_work_dim = num_dimensions;

			const cl_uint argument_get_global_size[3] = { global_work_size[0] };
			const cl_uint argument_get_global_id[3] = { cur_global[0] };
			const cl_uint argument_get_local_size[3] = { local_work_size[0] };
			const cl_uint argument_get_num_groups[3] = { num_groups[0] };
			const cl_uint argument_get_group_id[3] = { cur_group[0] };
			const cl_uint argument_get_local_id[3] = { cur_local[0] };

			typedPtr(
					argument_struct,
					argument_get_work_dim,
					argument_get_global_size,
					argument_get_global_id,
					argument_get_local_size,
					argument_get_num_groups,
					argument_get_group_id,
					argument_get_local_id
			);

			PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "kernel execution finished!\n"; );

			for (int cur_work_dim=num_dimensions-1; cur_work_dim >= 0; --cur_work_dim) {
				++cur_local[cur_work_dim];
				++cur_global[cur_work_dim];

				// if local work size is allowed to be larger than global work
				// size, we additionally need to check global id.
				if (cur_local[cur_work_dim] >= local_work_size[cur_work_dim] ||
						cur_global[cur_work_dim] >= global_work_size[cur_work_dim]) {
					if (cur_work_dim == 0) {
						group_finished = true;
						// globals are updated after group update
						break;
					}
					cur_local[cur_work_dim] = 0;

					//cur_global[cur_work_dim] -= local_work_size[cur_work_dim]
					cur_global[cur_work_dim] = cur_group[cur_work_dim] * local_work_size[cur_work_dim];
				} else {
					break;
				}
			}

		} while (!group_finished);



		// update group ids of all dimensions (leave untouched, increment, or reset)
		// This means we perform exactly one increment and at most one reset
		for (int cur_work_dim=num_dimensions-1; cur_work_dim >= 0; --cur_work_dim) {
			// Increment group id of dimension for next iteration
			++cur_group[cur_work_dim];

			if (cur_group[cur_work_dim] >= num_groups[cur_work_dim]) {
				// Dimension is finished: Reset corresponding group index to 0
				// If this is the outermost loop (cur_work_dim = 0), stop iterating.
				// (This means all dimensions have reached their max index.)
				if (cur_work_dim == 0) {
					kernel_finished = true;
					break;
				}
				cur_group[cur_work_dim] = 0;
			} else {
				// Otherwise, this dimension has not reached its maximum,
				// so we stop updating (= leave all outer indices untouched).
				break;
			}
		}

		if (kernel_finished) break;

		// update global ids using info of new group
		for (int cur_work_dim=num_dimensions-1; cur_work_dim >= 0; --cur_work_dim) {
			cur_global[cur_work_dim] = cur_group[cur_work_dim] * local_work_size[cur_work_dim];
		}

	} while (!kernel_finished);

	delete [] num_groups;
	delete [] cur_global;
	delete [] cur_local;
	delete [] cur_group;

	return CL_SUCCESS;
}
inline cl_int executeRangeKernel1DPacketOLD(cl_kernel kernel, const size_t global_work_size, const size_t local_work_size) {
	PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "  global_work_size: " << global_work_size << "\n"; );
	PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "  local_work_size: " << local_work_size << "\n"; );
	if (global_work_size % local_work_size != 0) return CL_INVALID_WORK_GROUP_SIZE;
	//if (global_work_size[0] > pow(2, sizeof(size_t)) /* oder so :P */) return CL_OUT_OF_RESOURCES;

	const size_t groupnr = global_work_size / local_work_size;
	const cl_uint argument_get_global_size = global_work_size;
	const cl_uint argument_get_local_size  = local_work_size;
	const cl_uint argument_get_num_groups  = groupnr == 0 ? 1 : groupnr;
	const __m128i argument_get_local_id_SIMD  = _mm_set_epi32(3, 2, 1, 0);
	typedef void (*kernelFnPtr)(
			const void*,
			const cl_uint,
			const cl_uint*,
			const cl_uint*,
			const cl_uint*,
			const cl_uint*,
			const cl_uint*,
			const __m128i*,
			const __m128i*);
	kernelFnPtr typedPtr = ptr_cast<kernelFnPtr>(kernel->get_compiled_function());

	const void* argument_struct = kernel->get_argument_struct();

	//
	// execute the kernel
	//
	const size_t num_iterations = global_work_size / PACKETIZED_OPENCL_DRIVER_SIMD_WIDTH; //local_work_size; // = #groups
	PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "\nexecuting kernel (#iterations: " << num_iterations << ")...\n"; );
	PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "global_size(" << 0 << "): " << get_global_size(0) << "\n"; );

	unsigned i;
	#ifdef PACKETIZED_OPENCL_DRIVER_USE_OPENMP
	omp_set_num_threads(PACKETIZED_OPENCL_DRIVER_MAX_NUM_THREADS);
	#pragma omp parallel for default(none) private(i) shared(argument_struct, typedPtr)
	#endif
	for (i=0; i<num_iterations; ++i) {
		PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "\niteration " << i << "\n"; );

		const cl_uint argument_get_global_id      = i;
		const cl_uint argument_get_group_id       = i;
		const unsigned id0 = i * 4;
		const __m128i argument_get_global_id_SIMD = _mm_set_epi32(id0 + 3, id0 + 2, id0 + 1, id0);

		typedPtr(
			argument_struct,
			1U,
			&argument_get_global_size,
			&argument_get_global_id,
			&argument_get_local_size,
			&argument_get_num_groups,
			&argument_get_group_id,
			&argument_get_global_id_SIMD,
			&argument_get_local_id_SIMD
		);

		PACKETIZED_OPENCL_DRIVER_DEBUG(
			outs() << "  iteration " << i << " finished!\n";
			PacketizedOpenCLDriver::verifyModule(kernel->get_program()->module);
		);
	}

	PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "execution of kernel finished!\n"; );

	return CL_SUCCESS;
}
#endif

inline cl_int executeRangeKernel1D(cl_kernel kernel, const size_t global_work_size, const size_t local_work_size) {
	PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "  global_work_size: " << global_work_size << "\n"; );
	PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "  local_work_size: " << local_work_size << "\n"; );
	if (global_work_size % local_work_size != 0) return CL_INVALID_WORK_GROUP_SIZE;
	//if (global_work_size[0] > pow(2, sizeof(size_t)) /* oder so :P */) return CL_OUT_OF_RESOURCES;

	typedef void (*kernelFnPtr)(
			const void*,
			const cl_uint,
			const size_t*,
			const size_t*,
			const cl_uint*);
	kernelFnPtr typedPtr = ptr_cast<kernelFnPtr>(kernel->get_compiled_function());

	const void* argument_struct = kernel->get_argument_struct();

	// In general it should be faster to use global_size instead of simd_width
	// In any case, changing the local work size can introduce arbitrary problems
	// except for the case where it is 1.

	assert (global_work_size >= PACKETIZED_OPENCL_DRIVER_SIMD_WIDTH);

#ifdef PACKETIZED_OPENCL_DRIVER_NO_PACKETIZATION
	const size_t modified_local_work_size = local_work_size;
#else
	#ifdef PACKETIZED_OPENCL_DRIVER_USE_OPENMP
	// TODO: simd width? simd width squared? other factor?
	const size_t modified_local_work_size = local_work_size < PACKETIZED_OPENCL_DRIVER_SIMD_WIDTH ?
		PACKETIZED_OPENCL_DRIVER_SIMD_WIDTH*PACKETIZED_OPENCL_DRIVER_SIMD_WIDTH : local_work_size;
	#else
	const size_t modified_local_work_size = local_work_size < PACKETIZED_OPENCL_DRIVER_SIMD_WIDTH ?
		global_work_size : local_work_size;
	#endif
#endif

	//
	// execute the kernel
	//
	const size_t num_iterations = global_work_size / modified_local_work_size; // = total # threads per block
	PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "executing kernel (#iterations: " << num_iterations << ")...\n"; );

	assert (num_iterations > 0 && "should give error message before executeRangeKernel!");

	unsigned i;
	#ifdef PACKETIZED_OPENCL_DRIVER_USE_OPENMP
	omp_set_num_threads(PACKETIZED_OPENCL_DRIVER_MAX_NUM_THREADS);
	#pragma omp parallel for default(none) private(i) shared(argument_struct, typedPtr)
	#endif
	for (i=0; i<num_iterations; ++i) {
		PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "\niteration " << i << " (= group id)\n"; );
		PACKETIZED_OPENCL_DRIVER_DEBUG( PacketizedOpenCLDriver::verifyModule(kernel->get_program()->module); );

		typedPtr(
			argument_struct,
			1U, // get_work_dim
			&global_work_size,
			&modified_local_work_size,
			&i
		);

		PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "iteration " << i << " finished!\n"; );
		PACKETIZED_OPENCL_DRIVER_DEBUG( PacketizedOpenCLDriver::verifyModule(kernel->get_program()->module); );
	}

	PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "execution of kernel finished!\n"; );

	return CL_SUCCESS;
}
inline cl_int executeRangeKernel2D(cl_kernel kernel, const size_t* global_work_size, const size_t* local_work_size) {
	PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "  global_work_sizes: " << global_work_size[0] << ", " << global_work_size[1] << "\n"; );
	PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "  local_work_sizes: " << local_work_size[0] << ", " << local_work_size[1] << "\n"; );
	if (global_work_size[0] % local_work_size[0] != 0) return CL_INVALID_WORK_GROUP_SIZE;
	if (global_work_size[1] % local_work_size[1] != 0) return CL_INVALID_WORK_GROUP_SIZE;
	//if (global_work_size[0] > pow(2, sizeof(size_t)) /* oder so :P */) return CL_OUT_OF_RESOURCES;

	typedef void (*kernelFnPtr)(
			const void*,
			const cl_uint,
			const size_t*,
			const size_t*,
			const size_t*);
	kernelFnPtr typedPtr = ptr_cast<kernelFnPtr>(kernel->get_compiled_function());

	const void* argument_struct = kernel->get_argument_struct();

	//
	// execute the kernel
	//
	const size_t num_iterations_0 = global_work_size[0] / local_work_size[0]; // = total # threads per block in dim 0
	const size_t num_iterations_1 = global_work_size[1] / local_work_size[1]; // = total # threads per block in dim 1
	PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "executing kernel (#iterations: " << num_iterations_0 * num_iterations_1 << ")...\n"; );

	assert (num_iterations_0 > 0 && num_iterations_1 > 0 && "should give error message before executeRangeKernel!");

	unsigned i, j;
	#ifdef PACKETIZED_OPENCL_DRIVER_USE_OPENMP
	omp_set_num_threads(PACKETIZED_OPENCL_DRIVER_MAX_NUM_THREADS);
	#pragma omp parallel for default(none) private(i, j) shared(argument_struct, typedPtr, global_work_size, local_work_size)
	#endif
	for (i=0; i<num_iterations_0; ++i) {
		for (j=0; j<num_iterations_1; ++j) {
			PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "\niteration " << i << "/"  << j << " (= group ids)\n"; );
			PACKETIZED_OPENCL_DRIVER_DEBUG( PacketizedOpenCLDriver::verifyModule(kernel->get_program()->module); );

			// TODO: unsure whether this really works
			const size_t group_id[2] = { i, j };

			typedPtr(
				argument_struct,
				2U, // get_work_dim
				global_work_size,
				local_work_size,
				group_id
			);

			PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "iteration " << i << "/" << j << " finished!\n"; );
			PACKETIZED_OPENCL_DRIVER_DEBUG( PacketizedOpenCLDriver::verifyModule(kernel->get_program()->module); );
		}
	}

	PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "execution of kernel finished!\n"; );

	return CL_SUCCESS;
}
inline cl_int executeRangeKernel3D(cl_kernel kernel, const size_t* global_work_size, const size_t* local_work_size) {
	PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "  global_work_sizes: " << global_work_size[0] << ", " << global_work_size[1] << ", " << global_work_size[2] << "\n"; );
	PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "  local_work_sizes: " << local_work_size[0] << ", " << local_work_size[1] << ", " << local_work_size[2] << "\n"; );
	if (global_work_size[0] % local_work_size[0] != 0) return CL_INVALID_WORK_GROUP_SIZE;
	if (global_work_size[1] % local_work_size[1] != 0) return CL_INVALID_WORK_GROUP_SIZE;
	if (global_work_size[2] % local_work_size[2] != 0) return CL_INVALID_WORK_GROUP_SIZE;
	//if (global_work_size[0] > pow(2, sizeof(size_t)) /* oder so :P */) return CL_OUT_OF_RESOURCES;

	typedef void (*kernelFnPtr)(
			const void*,
			const cl_uint,
			const size_t*,
			const size_t*,
			const size_t*);
	kernelFnPtr typedPtr = ptr_cast<kernelFnPtr>(kernel->get_compiled_function());

	const void* argument_struct = kernel->get_argument_struct();

	//
	// execute the kernel
	//
	const size_t num_iterations_0 = global_work_size[0] / local_work_size[0]; // = total # threads per block in dim 0
	const size_t num_iterations_1 = global_work_size[1] / local_work_size[1]; // = total # threads per block in dim 1
	const size_t num_iterations_2 = global_work_size[2] / local_work_size[2]; // = total # threads per block in dim 2
	PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "executing kernel (#iterations: " << num_iterations_0 * num_iterations_1 * num_iterations_2 << ")...\n"; );

	assert (num_iterations_0 > 0 && num_iterations_1 > 0 && num_iterations_2 && "should give error message before executeRangeKernel!");

	unsigned i, j, k;
	#ifdef PACKETIZED_OPENCL_DRIVER_USE_OPENMP
	omp_set_num_threads(PACKETIZED_OPENCL_DRIVER_MAX_NUM_THREADS);
	#pragma omp parallel for default(none) private(i, j, k) shared(argument_struct, typedPtr, global_work_size, local_work_size)
	#endif
	for (i=0; i<num_iterations_0; ++i) {
		for (j=0; j<num_iterations_1; ++j) {
			for (k=0; k<num_iterations_2; ++k) {
				PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "\niteration " << i << "/"  << j << "/" << k << " (= group ids)\n"; );
				PACKETIZED_OPENCL_DRIVER_DEBUG( PacketizedOpenCLDriver::verifyModule(kernel->get_program()->module); );

				const size_t group_id[3] = { i, j, k };

				typedPtr(
					argument_struct,
					3U, // get_work_dim
					global_work_size,
					local_work_size,
					group_id
				);

				PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "iteration " << i << "/" << j << "/" << k << " finished!\n"; );
				PACKETIZED_OPENCL_DRIVER_DEBUG( PacketizedOpenCLDriver::verifyModule(kernel->get_program()->module); );
			}
		}
	}

	PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "execution of kernel finished!\n"; );

	return CL_SUCCESS;
}
inline cl_int executeRangeKernelND(cl_kernel kernel, const cl_uint num_dimensions, const size_t* global_work_sizes, const size_t* local_work_sizes) {
	errs() << "ERROR: clEnqueueNDRangeKernels with work_dim > 3 currently not supported!\n";
	assert (false && "NOT IMPLEMENTED!");
	return CL_INVALID_PROGRAM_EXECUTABLE; // just return something != CL_SUCCESS :P
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueNDRangeKernel(cl_command_queue command_queue,
                       cl_kernel        kernel,
                       cl_uint          work_dim,
                       const size_t *   global_work_offset,
                       const size_t *   global_work_size,
                       const size_t *   local_work_size,
                       cl_uint          num_events_in_wait_list,
                       const cl_event * event_wait_list,
                       cl_event *       event) CL_API_SUFFIX__VERSION_1_0
{
	const unsigned num_dimensions = work_dim; // rename for better understandability ;)
	PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "\nclEnqueueNDRangeKernel(" << kernel->function_wrapper->getNameStr() << ")\n"; );
	PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "  num_dimensions: " << num_dimensions << "\n"; );
	PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "  num_events_in_wait_list: " << num_events_in_wait_list << "\n"; );
	if (!command_queue) return CL_INVALID_COMMAND_QUEUE;
	if (!kernel) return CL_INVALID_KERNEL;
	if (command_queue->context != kernel->get_context()) return CL_INVALID_CONTEXT;
	//if (command_queue->context != event_wait_list->context) return CL_INVALID_CONTEXT;
	if (num_dimensions < 1 || num_dimensions > PACKETIZED_OPENCL_DRIVER_MAX_NUM_DIMENSIONS) return CL_INVALID_WORK_DIMENSION;
	if (!kernel->get_compiled_function()) return CL_INVALID_PROGRAM_EXECUTABLE; // ?
	if (!global_work_size) return CL_INVALID_GLOBAL_WORK_SIZE;
	if (!local_work_size) return CL_INVALID_WORK_GROUP_SIZE;
	if (global_work_offset) return CL_INVALID_GLOBAL_OFFSET; // see specification p.109
	if (!event_wait_list && num_events_in_wait_list > 0) return CL_INVALID_EVENT_WAIT_LIST;
	if (event_wait_list && num_events_in_wait_list == 0) return CL_INVALID_EVENT_WAIT_LIST;

	// compare work_dim and derived dimensions and issue warning/error if not the same
	// (we generate code specific to the number of dimensions actually used)
	// TODO: reject kernel if any of the builtin functions receives variable parameter
	PACKETIZED_OPENCL_DRIVER_DEBUG(
		if (kernel->get_num_dimensions() != num_dimensions) {
			errs() << "WARNING: number of dimensions used in kernel (" << kernel->get_num_dimensions() <<
					") does not match 'work_dim' (" << num_dimensions << ") supplied by clEnqueueNDRangeKernel()!\n";
		}
	);

#ifndef PACKETIZED_OPENCL_DRIVER_NO_PACKETIZATION
	PACKETIZED_OPENCL_DRIVER_DEBUG(
		const size_t simd_dim_work_size = local_work_size[kernel->get_best_simd_dim()];
		outs() << "  best simd dim: " << kernel->get_best_simd_dim() << "\n";
		outs() << "  local_work_size of dim: " << simd_dim_work_size << "\n";
		const bool dividableBySimdWidth = simd_dim_work_size % PACKETIZED_OPENCL_DRIVER_SIMD_WIDTH == 0;
		if (!dividableBySimdWidth) {
			errs() << "WARNING: group size of simd dimension not dividable by simdWidth\n";
		}
		//errs() << "         group size of dimension " << kernel->get_best_simd_dim() << " will be set to " << PACKETIZED_OPENCL_DRIVER_SIMD_WIDTH << " or entire global size!\n";
	);
#endif

	switch (num_dimensions) {
		case 1: return executeRangeKernel1D(kernel, global_work_size[0], local_work_size[0]);
		case 2: return executeRangeKernel2D(kernel, global_work_size,    local_work_size);
		case 3: return executeRangeKernel3D(kernel, global_work_size,    local_work_size);
		default: return executeRangeKernelND(kernel, num_dimensions, global_work_size, local_work_size);
	}

}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueTask(cl_command_queue  command_queue,
              cl_kernel         kernel,
              cl_uint           num_events_in_wait_list,
              const cl_event *  event_wait_list,
              cl_event *        event) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueNativeKernel(cl_command_queue  command_queue,
					  void (*user_func)(void *),
                      void *            args,
                      size_t            cb_args,
                      cl_uint           num_mem_objects,
                      const cl_mem *    mem_list,
                      const void **     args_mem_loc,
                      cl_uint           num_events_in_wait_list,
                      const cl_event *  event_wait_list,
                      cl_event *        event) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueMarker(cl_command_queue    command_queue,
                cl_event *          event) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueWaitForEvents(cl_command_queue command_queue,
                       cl_uint          num_events,
                       const cl_event * event_list) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueBarrier(cl_command_queue command_queue) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

/* Extension function access
 *
 * Returns the extension function address for the given function name,
 * or NULL if a valid function can not be found.  The client must
 * check to make sure the address is not NULL, before using or
 * calling the returned function address.
 */
CL_API_ENTRY void * CL_API_CALL clGetExtensionFunctionAddress(const char * func_name) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return NULL;
}

#ifdef __cplusplus
}
#endif



/*
for (i=0; i<num_iterations; ++i) {
			PACKETIZED_OPENCL_DRIVER_DEBUG( outs() << "\niteration " << i << "\n"; );

			// update runtime environment
			setCurrentGlobal(cur_work_dim, i);
			setCurrentGroup(cur_work_dim, i / lws);
			setCurrentLocal(cur_work_dim, i % lws);

			PACKETIZED_OPENCL_DRIVER_DEBUG(
				outs() << "  global id: " << get_global_id(cur_work_dim) << "\n";
				outs() << "  local id: " << get_local_id(cur_work_dim) << "\n";
				outs() << "  group id: " << get_group_id(cur_work_dim) << "\n";

				//hardcoded debug output for simpleTest
				//typedef struct { float* input; float* output; unsigned count; } tt;
				//outs() << "  input-addr : " << ((tt*)kernel->get_argument_struct())->input << "\n";
				//outs() << "  output-addr: " << ((tt*)kernel->get_argument_struct())->output << "\n";
				//outs() << "  input : " << ((tt*)kernel->get_argument_struct())->input[i] << "\n";
				//outs() << "  output: " << ((tt*)kernel->get_argument_struct())->output[i] << "\n";
				//outs() << "  count : " << ((tt*)kernel->get_argument_struct())->count << "\n";

				//hardcoded debug output for DwtHaar1D
				//PacketizedOpenCLDriver::writeFunctionToFile(kernel->function_wrapper, "executed.ll");
				//typedef struct { float* inSignal; float *coefsSignal; float *AverageSignal; float *sharedArray;
				//unsigned tLevels; unsigned signalLength; unsigned levelsDone; unsigned mLevels; } tt;
				//outs() << "  inSignal : " << ((tt*)kernel->get_argument_struct())->inSignal << "\n";
				//outs() << "  coefsSignal : " << ((tt*)kernel->get_argument_struct())->coefsSignal << "\n";
				//outs() << "  AverageSignal : " << ((tt*)kernel->get_argument_struct())->AverageSignal << "\n";
				//outs() << "  sharedArray : " << ((tt*)kernel->get_argument_struct())->sharedArray << "\n";
				////outs() << "  tLevels : " << ((tt*)kernel->get_argument_struct())->tLevels << "\n";
				//outs() << "  signalLength : " << ((tt*)kernel->get_argument_struct())->signalLength << "\n";
				//outs() << "  levelsDone : " << ((tt*)kernel->get_argument_struct())->levelsDone << "\n";
				//outs() << "  mLevels : " << ((tt*)kernel->get_argument_struct())->mLevels << "\n";
				//const float* arr = (float*)((tt*)kernel->get_argument_struct())->inSignal;
				//float before[64];
				//for (unsigned j=0; j<64; ++j) {
				//before[j] = arr[j];
				//}

				//hardcoded debug output for Histogram
				//typedef struct { unsigned* data; unsigned* sharedArray; unsigned* binResult; } tt;
				//outs() << "  data: " << ((tt*)kernel->get_argument_struct())->data << "\n";
				//outs() << "  sharedArray: " << ((tt*)kernel->get_argument_struct())->sharedArray << "\n";
				//outs() << "  binResult: " << ((tt*)kernel->get_argument_struct())->binResult << "\n";
				//const unsigned* arr = (unsigned*)((tt*)kernel->get_argument_struct())->data;
				//outs() << "  data:\n-------\n";
				//for (unsigned j=0; j<8; ++j) {
				//outs() << "    " << arr[j] << "\n";
				//}
				//outs() << "-------\n";
				//const unsigned* arr2 = (unsigned*)((tt*)kernel->get_argument_struct())->sharedArray;
				//outs() << "  sharedArray:\n-------\n";
				//for (unsigned j=0; j<8; ++j) {
				//outs() << "    " << arr2[j] << "\n";
				//}
				//outs() << "-------\n";
				//const unsigned* arr3 = (unsigned*)((tt*)kernel->get_argument_struct())->binResult;
				//outs() << "  binResult:\n-------\n";
				//for (unsigned j=0; j<8; ++j) {
				//outs() << "    " << arr3[j] << "\n";
				//}
				//outs() << "-------\n";

				PacketizedOpenCLDriver::verifyModule(kernel->get_program()->module);
				//outs() << "  verification before execution successful!\n";
			);

			// call kernel
			typedPtr(argument_struct);

			PACKETIZED_OPENCL_DRIVER_DEBUG(
				//hardcoded debug output for SimpleTest
				//typedef struct { float* input; float* output; unsigned count; } tt;
				//outs() << "  result: " << ((tt*)kernel->get_argument_struct())->output[i] << "\n";

				//hardcoded debug output for Histogram
				//typedef struct { unsigned* data; unsigned* sharedArray; unsigned* binResult; } tt;
				//const unsigned* arr = (unsigned*)((tt*)kernel->get_argument_struct())->data;
				//outs() << "  data:\n-------\n";
				//for (unsigned j=0; j<8; ++j) {
				//outs() << "    " << arr[j] << "\n";
				//}
				//outs() << "-------\n";
				//const unsigned* arr2 = (unsigned*)((tt*)kernel->get_argument_struct())->sharedArray;
				//outs() << "  sharedArray:\n-------\n";
				//for (unsigned j=0; j<8; ++j) {
				//outs() << "    " << arr2[j] << "\n";
				//}
				//outs() << "-------\n";
				//const unsigned* arr3 = (unsigned*)((tt*)kernel->get_argument_struct())->binResult;
				//outs() << "  binResult:\n-------\n";
				//for (unsigned j=0; j<8; ++j) {
				//outs() << "    " << arr3[j] << "\n";
				//}
				//outs() << "-------\n";

				outs() << "  iteration " << i << " finished!\n";
				PacketizedOpenCLDriver::verifyModule(kernel->get_program()->module);
				//outs() << "  verification after execution successful!\n";
			);
		}
*/
