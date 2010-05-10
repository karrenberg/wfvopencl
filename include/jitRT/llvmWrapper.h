/**
 * @file   llvmWrapper.h
 * @date   10.12.2008
 * @author Ralf Karrenberg
 *
 *
 * Copyright (C) 2008, 2009, 2010 Saarland University
 *
 * This file is part of jitRT.
 *
 * jitRT is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * jitRT is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with jitRT.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#ifndef _LLVMWRAPPER_H
#define	_LLVMWRAPPER_H

#include <string>
#include <vector>
#include <set>
#include <map>

//----------------------------------------------------------------------------//
// necessary definitions for library compilation
//----------------------------------------------------------------------------//

#if defined(_WIN32)

    #if !defined(JITRT_STATIC_LIBS)
        #define JITRT_DLL_EXPORT __declspec(dllexport)
        #define JITRT_DLL_IMPORT __declspec(dllimport)
    #else
        #define JITRT_DLL_EXPORT
        #define JITRT_DLL_IMPORT
    #endif

#else

    // POSIX, etc.
    #define JITRT_DLL_EXPORT
    #define JITRT_DLL_IMPORT

#endif


// move this if necessary into a separate config file
#if defined(JITRT_LIB)
    #define JITRT_API JITRT_DLL_EXPORT
#else
    #define JITRT_API JITRT_DLL_IMPORT
#endif

//----------------------------------------------------------------------------//
//----------------------------------------------------------------------------//

//forward declarations
namespace llvm {
  class Module;
  class Function;
  class CallSite;
  class Value;
  class GlobalValue;
  class GlobalVariable;
  class Constant;
  class Type;
  class ExecutionEngine;
  class TargetData;
}

//using namespace llvm; //use explicit qualifiers to prevent ambiguous names

namespace jitRT {
    //llvmDebug.cpp
    JITRT_API void viewCFG(const llvm::Module * M);
    JITRT_API void viewCFG(const llvm::Function * F);
    JITRT_API void viewCFGOnly(const llvm::Module * M);
    JITRT_API void listFunctions(const llvm::Module * M);
    JITRT_API void listFunctionsDemangled(const llvm::Module * M);
    JITRT_API void verifyModule(llvm::Module * module);

    //llvmTools.cpp
    JITRT_API const llvm::Type* getType(const llvm::Value* value);
    JITRT_API const llvm::Type* getType(const llvm::GlobalValue* value);
    JITRT_API llvm::GlobalVariable* getGlobalVariable(llvm::Module* mod, const std::string& name);
    JITRT_API llvm::GlobalValue* getNamedGlobalValue(llvm::Module* mod, const std::string& name);
    JITRT_API void setGlobalVariableZero(llvm::Module* mod, const std::string& name);
    JITRT_API llvm::Function* getFunction(const std::string& name, llvm::Module* module);
    JITRT_API std::string getFunctionName(const llvm::Function* f);
    JITRT_API llvm::Module* getModule(llvm::Function* f);
    JITRT_API unsigned computeBitSize(const llvm::Type * type);
    JITRT_API void printModule(const llvm::Module* module);
    JITRT_API void printModules(const int moduleCount, llvm::Module** modules);
    JITRT_API void printFunction(const llvm::Function* f);
    JITRT_API llvm::Module* createModuleFromFile(const std::string & fileName);
    JITRT_API void deleteModule(llvm::Module * mod);
    JITRT_API void writeModuleToFile(llvm::Module * M, const std::string & fileName);
    JITRT_API void writeFunctionToFile(llvm::Function * F, const std::string & fileName);
    JITRT_API void printValue(const llvm::Value* V);
    JITRT_API void printType(const llvm::Type* T);

    JITRT_API unsigned mapGlobalValues(llvm::Module* mod, std::map<std::string, int>& indexMap, std::vector<unsigned>& values);
    JITRT_API void setModuleIdentifier(llvm::Module* mod, const std::string& name);
    JITRT_API void replaceAllUsesWith(llvm::Function* oldFn, llvm::Function* newFn);
    JITRT_API void replaceAllUsesWith(llvm::Function* oldFn, llvm::Constant* newVal);
    JITRT_API void replaceAllUsesWith(llvm::Value* oldVal, llvm::Value* newVal);
    JITRT_API void uncheckedReplaceAllUsesWith(llvm::Function* oldFn, llvm::Function* newFn);

    //llvmLinker.cpp
    JITRT_API llvm::Module* linkInModule(llvm::Module* target, llvm::Module* source);
    JITRT_API llvm::Module* linkInModules(llvm::Module* target, std::vector<llvm::Module*>& sources);
    JITRT_API llvm::Module* link(int moduleCount, llvm::Module** modules);
    JITRT_API llvm::Module* link(llvm::Module* linkedModule, llvm::Module* module);
    JITRT_API llvm::Module* linkEverything(int moduleCount, llvm::Module** modules);

    //llvmInliner.cpp
    JITRT_API typedef llvm::GlobalValue* (*FncGlobalValueHandler)(llvm::GlobalValue*, llvm::Module*);
    JITRT_API typedef std::vector<llvm::Function*> FuncVector;
    JITRT_API void inlineAllCalls(llvm::Module * scope, FncGlobalValueHandler handler);
    JITRT_API void inlineAllCalls(llvm::Module * scopeModule, llvm::Function * scope, FncGlobalValueHandler handler);
    JITRT_API void inlineAllCalls(llvm::Module * scopeModule, llvm::Function * scope, FncGlobalValueHandler handler, std::map<llvm::GlobalValue*, llvm::GlobalValue*> & mappedValues, FuncVector & copiedFunctions);

    JITRT_API void inlineInternal(llvm::Function * scope, const std::string & calledFnc);
    JITRT_API void inlineExcluding(llvm::Function * scope, const std::set<std::string> & dontInline);
    JITRT_API void inlineExcluding(llvm::Function * scope, int count, ...);
    JITRT_API bool inlineFunctionInModule(llvm::Function * F, llvm::Module* mod);
    JITRT_API bool inlineFunctionInModule(const std::string functionName, llvm::Module* mod);
    JITRT_API void inlineCall(llvm::Function * F, const std::string & calledFnc, llvm::Function * callee);
    JITRT_API void inlineCall(llvm::Module * scope, const std::string & calledFnc, llvm::Function * callee);
    JITRT_API void getCallSites(std::vector<llvm::CallSite> & CallSites, llvm::Module * M, const std::string & calledFnc);
    JITRT_API void getCallSites(std::vector<llvm::CallSite> & CallSites, llvm::Function * scope, const std::string & calledFnc);
    JITRT_API llvm::Value* getCalledValue(llvm::CallSite& callSite);
    JITRT_API void setCalledFunction(llvm::CallSite& callSite, llvm::Value* funcPointer);
    JITRT_API void removeCall(llvm::Module * scope, const std::string & callee);

    //llvmSpecialization.cpp
    JITRT_API llvm::Constant * createFunctionPointer(llvm::Function * decl, void * ptr);
    JITRT_API void linkCall(llvm::Function * F, const std::string & calledFunction, llvm::Function * callee);
    JITRT_API void linkCall(llvm::Module * M, const std::string & calledFunction, llvm::Function * callee);
    JITRT_API llvm::GlobalValue * getGlobal(const llvm::Module * M, const std::string & globalName);

    JITRT_API llvm::Constant * createZeroConstant(const llvm::Type * type);
    JITRT_API llvm::Constant * createConstant(const llvm::Type * type, const char * value);
    JITRT_API const char * readConstant(llvm::Constant *& target, const llvm::Type * type, const char * value);
    JITRT_API void specializeGlobalIntInModule(const std::string & variableName, const int value, llvm::Module* mod);
    JITRT_API void specializeGlobalFloatInModule(const std::string & variableName, const float value, llvm::Module* mod);
    JITRT_API void specializeGlobal(const std::string & globalName, llvm::Constant * c, llvm::Module * mod);

	JITRT_API void setFunctionDoesNotAlias(llvm::Function* f);
	JITRT_API void setFunctionCallsDoNotAccessMemory(llvm::Function* f);
	JITRT_API void setAllCallsDoNotAccessMemory(llvm::Function* f);

    //llvmPartialEvaluation.cpp
    JITRT_API void evaluateFunctionInt(llvm::Function* f, std::string argName, int value);
    JITRT_API void evaluateFunctionFloat(llvm::Function* f, std::string argName, float value);
    JITRT_API void evaluateConstantFunctionCalls(std::string functionName, llvm::Module* mod);

    //llvmOptimization.cpp
    JITRT_API void optimizeFunction(llvm::Function* f);
    JITRT_API void optimizeFunction(const std::string& functionName, llvm::Module* mod);
    JITRT_API void optimizeFunctionFast(llvm::Function* f);
    JITRT_API void optimizeModule(llvm::Module* mod);
    JITRT_API void inlineFunctionCalls(llvm::Function* f);

    //llvmExecution
    JITRT_API llvm::ExecutionEngine * getExecutionEngine(llvm::Module* mod);
    JITRT_API llvm::ExecutionEngine* createExecutionEngine(llvm::Module* mod);
    JITRT_API int executeMain(void* mainPtr, int argc, char **argv);
	JITRT_API void executeFunction(llvm::Function* f, llvm::ExecutionEngine* engine);
	JITRT_API void* executeVoidPtrFunction(llvm::Function* f, llvm::ExecutionEngine* engine);
    JITRT_API void* getPointerToFunction(llvm::Module* mod, std::string functionName);
    JITRT_API void* getPointerToFunction(llvm::Module * mod, llvm::Function * func);
    JITRT_API void* getPointerToFunction(llvm::ExecutionEngine * engine, llvm::Function * func);
    JITRT_API void addGlobalMapping(llvm::ExecutionEngine* engine, llvm::GlobalValue* value, void* data);
    JITRT_API void freeMachineCodeForFunction(llvm::Module* mod, llvm::Function* f);
    JITRT_API void* getOrEmitGlobalVariable(llvm::ExecutionEngine* engine, llvm::GlobalVariable* var);
	JITRT_API void* getPointerToGlobal(llvm::ExecutionEngine* engine, const llvm::GlobalValue *GV);
	JITRT_API unsigned getPrimitiveSizeInBits(const llvm::Type* type);
	JITRT_API bool isPointerType(const llvm::Type* type);
	JITRT_API const llvm::Type* getContainedType(const llvm::Type* type, const unsigned index);
	JITRT_API llvm::TargetData* getTargetData(llvm::Module* mod);
	JITRT_API void setTargetData(llvm::Module* mod, const std::string& dataLayout, const std::string& targetTriple);
	JITRT_API const std::string& getTargetTriple(const llvm::Module* mod);
	JITRT_API unsigned getTypeSizeInBits(const llvm::TargetData* targetData, const llvm::Type* type);
	JITRT_API void runStaticConstructorsDestructors(llvm::Module* mod, llvm::ExecutionEngine* engine, const bool isDtors);

	JITRT_API unsigned getNumArgs(const llvm::Function* f);
	JITRT_API const llvm::Type* getArgumentType(const llvm::Function* f, const unsigned arg_index);
	JITRT_API unsigned getAddressSpace(const llvm::Type* type);

	JITRT_API llvm::Function* generateFunctionWrapper(const std::string& wrapper_name, llvm::Function* f, llvm::Module* mod);
	// OpenCL stuff
	JITRT_API void replaceNonContiguousIndexUsage(llvm::Function* f, llvm::Function* oldF, llvm::Function* newF);
	JITRT_API void generateOpenCLFunctions(llvm::Module* mod);
	JITRT_API llvm::Function* generatePacketPrototypeFromOpenCLKernel(const llvm::Function* kernel, const std::string& packetKernelName, llvm::Module* mod);

    //llvmCloning
    JITRT_API llvm::Module * cloneModule(const llvm::Module*);
    JITRT_API llvm::Function * cloneFunction(llvm::Function*, llvm::Module * destModule);
    JITRT_API llvm::GlobalVariable * cloneGlobalVariable(llvm::GlobalVariable * var, llvm::Module * destModule);
    JITRT_API llvm::Function * copyFunction(llvm::Function * func, llvm::Module * destModule);
    JITRT_API llvm::GlobalVariable * copyGlobalVariable(llvm::GlobalVariable * var, llvm::Module * destModule);
    JITRT_API llvm::Function * moveFunction(llvm::Function * func, llvm::Module * destModule);
    JITRT_API llvm::GlobalVariable * moveGlobalVariable(llvm::GlobalVariable * gv, llvm::Module * destModule);

    //Handler from llvmCloning
    JITRT_API llvm::GlobalValue * copyGlobal(llvm::GlobalValue * global, llvm::Module * module);
    JITRT_API llvm::GlobalValue * moveGlobal(llvm::GlobalValue * global, llvm::Module * module);

    //llvmMangling
    JITRT_API llvm::Function * getMangledFunction(const std::string & demangledName, llvm::Module * mod);
    JITRT_API std::string GCCdemangle(std::string & mangledName);
    JITRT_API std::string GCCdemangle(const std::string & mangledName);
    JITRT_API bool GCCdemangle(std::string & outName, const std::string & mangledName);

    //timer
    class Timer;
    JITRT_API Timer* getTimer();
    JITRT_API inline void startTimer(Timer* timer);
    JITRT_API inline void stopTimer(Timer* timer);
    JITRT_API inline double getTime(Timer* time);

    //llvmTypeReplacement
    JITRT_API void replaceTypes(llvm::Module* mod);
    JITRT_API void replaceAllocas(llvm::Function* f, std::map<std::string, const llvm::Type*>& typeMap);
    JITRT_API void replaceDerivAllocas(llvm::Function* f, std::map<std::string, const llvm::Type*>& typeMap);
    JITRT_API void replaceGEPs(llvm::Function* f, std::map<std::string, const llvm::Type*>& typeMap);
    JITRT_API void replaceGetTexturePtrs(llvm::Function* f, std::map<const std::string, const unsigned>& textureMap);
    JITRT_API void replaceBitcasts(llvm::Function* f);

	//languageIntegration
	JITRT_API void resolveThreadedRSLCode(llvm::Module* shader, const llvm::Module* runtime, const llvm::Module* glue, std::map<std::string, const llvm::Type*>& typeMap, std::map<const std::string, const unsigned>& textureMap, const bool use_common_org, const bool use_derivatives, const bool packetize, const bool use_sse41);

} //namespace jitRT

namespace Packetizer {
    class Packetizer;
    JITRT_API Packetizer* getPacketizer(const bool use_sse41, const bool verbose=false);
    JITRT_API void addFunctionToPacketizer(Packetizer* packetizer, const std::string& functionName, const std::string& newName, const unsigned packetizationSize);
    JITRT_API void addNativeFunctionToPacketizer(Packetizer* packetizer, const std::string& scalarFunctionName, const int maskPosition, llvm::Function* packetFunction, const bool forcePacketization);
    JITRT_API void runPacketizer(Packetizer* packetizer, llvm::Module* mod);
    JITRT_API void packetize(const unsigned packetizationSize, const std::string& functionName, const std::string& newName, llvm::Module* mod);
    JITRT_API void packetize(const unsigned packetizationSize, std::map<const std::string, const std::string>& functionNameMap, llvm::Module* mod);
}


#endif	/* _LLVMWRAPPER_H */


