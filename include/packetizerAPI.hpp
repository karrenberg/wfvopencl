/**
 * @file   packetizerAPI.hpp
 * @date   10.12.2008
 * @author Ralf Karrenberg
 *
 *
 * Copyright (C) 2008, 2009, 2010, 2011 Saarland University
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
#ifndef _PACKETIZERAPI_HPP
#define	_PACKETIZERAPI_HPP

#include <string>

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
}

// WFV forward declaration
class WholeFunctionVectorizer;


namespace Packetizer {

PACKETIZER_API class Packetizer {
public:
	PACKETIZER_API Packetizer(llvm::Module& module, const unsigned simdWidth, const unsigned packetizationSize, const bool use_sse41, const bool use_avx, const bool verbose);
	PACKETIZER_API ~Packetizer();

	PACKETIZER_API void addFunction(const std::string& functionName, const std::string& newName);
	PACKETIZER_API void addNativeFunction(const std::string& scalarFunctionName, const int maskPosition, llvm::Function* packetFunction, const bool forcePacketization);
	PACKETIZER_API void run();
private:
	WholeFunctionVectorizer* wfv; // TODO: hide?
};


} //namespace Packetizer


#endif	/* _PACKETIZERAPI_HPP */
