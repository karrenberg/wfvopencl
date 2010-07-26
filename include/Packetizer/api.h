/**
 * @file   api.h
 * @date   10.12.2008
 * @author Ralf Karrenberg
 *
 *
 * Copyright (C) 2008, 2009, 2010 Saarland University
 *
 * This file is part of Packetizer.
 *
 * Packetizer is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Packetizer is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Packetizer.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#ifndef _API_H
#define	_API_H

#include <string>
#include <vector>
#include <map>

//----------------------------------------------------------------------------//
// necessary definitions for library compilation
//----------------------------------------------------------------------------//

#if defined(_WIN32)

    #if !defined(PACKETIZER_STATIC_LIBS)
        #define PACKETIZER_DLL_EXPORT __declspec(dllexport)
        #define PACKETIZER_DLL_IMPORT __declspec(dllimport)
    #else
        #define PACKETIZER_DLL_EXPORT
        #define PACKETIZER_DLL_IMPORT
    #endif

#else

    // POSIX, etc.
    #define PACKETIZER_DLL_EXPORT
    #define PACKETIZER_DLL_IMPORT

#endif

#if defined(PACKETIZER_LIB)
    #define PACKETIZER_API PACKETIZER_DLL_EXPORT
#else
    #define PACKETIZER_API PACKETIZER_DLL_IMPORT
#endif

//----------------------------------------------------------------------------//
//----------------------------------------------------------------------------//


// LLVM forward declarations
namespace llvm {
  class Module;
  class Function;
  class ExecutionEngine;
}

namespace Packetizer {

    class Packetizer;

    PACKETIZER_API Packetizer* getPacketizer(const bool use_sse41, const bool verbose=false);
    PACKETIZER_API void addFunctionToPacketizer(Packetizer* packetizer, const std::string& functionName, const std::string& newName, const unsigned packetizationSize);
    PACKETIZER_API void addNativeFunctionToPacketizer(Packetizer* packetizer, const std::string& scalarFunctionName, const int maskPosition, llvm::Function* packetFunction, const bool forcePacketization);
    PACKETIZER_API void runPacketizer(Packetizer* packetizer, llvm::Module* mod);

	// direct packetization with default setup and without support of native functions
    PACKETIZER_API void packetize(const unsigned packetizationSize, const std::string& functionName, const std::string& newName, llvm::Module* mod);
    PACKETIZER_API void packetize(const unsigned packetizationSize, std::map<const std::string, const std::string>& functionNameMap, llvm::Module* mod);


	//
	// Basic LLVM wrapper that provides functionality to use the packetizer
	// without touching anything of LLVM's insides.
	// See 'tools/packetizeFunction.cpp' for an example.
	//

	// create module, search function, etc.
    PACKETIZER_API llvm::Module* createModuleFromFile(const std::string & fileName);
    PACKETIZER_API void deleteModule(llvm::Module* mod);
	PACKETIZER_API void verifyModule(llvm::Module* mod);

	PACKETIZER_API llvm::Function* getFunction(const std::string& name, llvm::Module* mod);
    PACKETIZER_API std::string getFunctionName(const llvm::Function* f);
    PACKETIZER_API llvm::Module* getModule(llvm::Function* f);

	// linking
    PACKETIZER_API llvm::Module* linkInModule(llvm::Module* target, llvm::Module* source);

	// execution
	PACKETIZER_API llvm::ExecutionEngine * getExecutionEngine();
    PACKETIZER_API llvm::ExecutionEngine * createExecutionEngine(llvm::Module* mod);
	PACKETIZER_API bool removeModule(llvm::Module* mod, llvm::ExecutionEngine* engine);
	PACKETIZER_API void* getPointerToFunction(llvm::Function* f, llvm::Module* mod);
    PACKETIZER_API void* getPointerToFunction(llvm::Function* f, llvm::ExecutionEngine* engine);
	PACKETIZER_API void executeFunction(llvm::Function* f, llvm::ExecutionEngine* engine);
	PACKETIZER_API void* executeVoidPtrFunction(llvm::Function* f, llvm::ExecutionEngine* engine);
	PACKETIZER_API int executeMain(void* mainPtr, int argc, char **argv);

	// optimization
    PACKETIZER_API void optimizeFunction(llvm::Function* f);
    PACKETIZER_API void optimizeFunction(const std::string& functionName, llvm::Module* mod);
    PACKETIZER_API void optimizeFunctionFast(llvm::Function* f);
    PACKETIZER_API void optimizeModule(llvm::Module* mod);

    PACKETIZER_API void inlineFunctionCalls(llvm::Function* f); // attempts to inline all calls inside f
	PACKETIZER_API bool inlineFunctionInModule(llvm::Function * F, llvm::Module* mod); // attempts to inline f at all call sites in mod

	// output / debugging
	PACKETIZER_API void printFunction(const llvm::Function* f);
	PACKETIZER_API void printModule(const llvm::Module* module);

    PACKETIZER_API void writeFunctionToFile(llvm::Function * F, const std::string & fileName);
	PACKETIZER_API void writeModuleToFile(llvm::Module * M, const std::string & fileName);

} //namespace Packetizer

#endif	/* _API_H */
