/**
 * @file   llvmTools.h
 * @date   27.05.2010
 * @author Ralf Karrenberg
 *
 * This file is distributed under the University of Illinois Open Source
 * License. See the COPYING file in the root directory for details.
 *
 * Copyright (C) 2010, 2011 Saarland University
 *
 */
#ifndef _LLVMTOOLS_H
#define _LLVMTOOLS_H

#include <cassert>
#include <set>

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>          // e.g. for cl_platform_id
#endif

// TODO clean this up, most includes are probably only needed in llvmTools.cpp

#include "llvm/Assembly/Parser.h"

#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/system_error.h" // error_code
#include "llvm/Support/SourceMgr.h" // SMDiagnostic

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


#ifndef WFVOPENCL_NO_PACKETIZATION
#	include "packetizerAPI.hpp"
#endif

#include "debug.h"

#define WFVOPENCL_SIMD_WIDTH 4

using namespace llvm;

namespace WFVOpenCL {
#ifndef WFVOPENCL_NO_PACKETIZATION
	void addNativeFunctions(Function* kernel, const cl_uint simdDim, Packetizer::Packetizer& packetizer);
#endif
	Function* generateFunctionWrapperWithParams(const std::string& wrapper_name, Function* f, Module* mod, std::vector<const Type*>& additionalParams, const bool inlineCall);
	void generateOpenCLFunctions(Module* mod);
	const Type * getPrimitiveType(Module *mod, char t);
	const Type * getTypeFromString(Module *mod, const std::string &typeString);
	Function* createExternalFunction(const std::string& name, const FunctionType* fType, Module* mod);
	Function* createExternalFunction(const std::string& name, const Type* returnType, std::vector<const Type*>& paramTypes, Module* mod);
}



// to be removed :)
namespace WFVOpenCL {
	Function* getFunction(const std::string& name, Module* module);
	Module* createModuleFromFile(const std::string & fileName);
	void writeModuleToFile(const Module * M, const std::string & fileName);
	void writeFunctionToFile(const Function * F, const std::string & fileName);
	ExecutionEngine* createExecutionEngine(Module* mod);
	ExecutionEngine * getExecutionEngine(Module* mod);
	void* getPointerToFunction(Module * mod, Function * func);
	void* getPointerToFunction(Module* mod, std::string functionName);
	void* getPointerToFunction(ExecutionEngine * engine, Function * func);
	unsigned getPrimitiveSizeInBits(const Type* type);
	bool isPointerType(const Type* type);
	const Type* getContainedType(const Type* type, const unsigned index);
	TargetData* getTargetData(Module* mod);
	void setTargetData(Module* mod, const std::string& dataLayout, const std::string& targetTriple);
	const std::string& getTargetTriple(const Module* mod);
	void setDataLayout(Module* mod, const std::string& dataLayout);
	uint64_t getTypeSizeInBits(const TargetData* targetData, const Type* type);
	void runStaticConstructorsDestructors(Module* mod, ExecutionEngine* engine, const bool isDtors);
	unsigned getNumArgs(const Function* f);
	const Type* getArgumentType(const Function* f, const unsigned arg_index);
	unsigned getAddressSpace(const Type* type);
	void inlineFunctionCalls(Function* f, TargetData* targetData=NULL);
	void optimizeFunction(Function* f, const bool disableLICM=false, const bool disableLoopRotate=false);
	Constant * createPointerConstant(void * thePointer, const Type * pointerType);
	const char * readConstant(Constant *& target,const Type * type, const char * position);
	Constant * createConstant(const Type * type, const char * value);
	Constant * createFunctionPointer(Function * decl, void * ptr);
	void replaceAllUsesWith(Function* oldFn, Function* newFn);
	void replaceAllUsesWith(Function* oldFn, Constant* newVal);
	void replaceAllUsesWith(Value* oldVal, Value* newVal);
	Module* linkInModule(Module* target, Module* source);
}


#endif /* _LLVMTOOLS_H */
