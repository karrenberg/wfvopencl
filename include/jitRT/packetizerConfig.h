/**
 * @file   packetizerConfig.h
 * @date   13.10.2009
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
#ifndef _PACKETIZERCONFIG_H
#define	_PACKETIZERCONFIG_H

// variables defined by corresponding compiler
// gcc     = __GNUC__
// icc     = __INTEL_COMPILER
// msvc    = _MSC_VER
// llvm    = __llvm__
// borland = __BORLANDC__

// variables defined by corresponding operating system
// windows (general) = _WIN32
// windows 64bit     = _WIN64
// linux             = __linux
// mac os            = __APPLE__ & __MACH__ (icc & gcc)

// http://predef.sourceforge.net/


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
// packetizer ifdefs
//----------------------------------------------------------------------------//

// use LLVM 2.4
// (default: deactivated)

//#define LLVM_2_4


// don't use any of LLVM's optimizations
// (default: deactivated)

//#define DO_NOT_OPTIMIZE
#ifdef DO_NOT_OPTIMIZE
	#ifndef _MSC_VER
        #warning "WARNING: scalar LLVM optimizations disabled!"
    #else
        //#pragma WARNING ( scalar LLVM optimizations disabled! )
	#endif
#endif


// prevent replacing of unsupported constructs (e.g. types i64, double,
// instructions sext/zext/...)
// (default: deactivated)

//#define PACKETIZER_DO_NOT_REPLACE_UNSUPPORTED_CONSTRUCTS //executes transformFunction() which removes all zext/sext/i64/double
#ifdef PACKETIZER_DO_NOT_REPLACE_UNSUPPORTED_CONSTRUCTS
    #ifndef _MSC_VER
        #warning "WARNING: packetizer will not attempt to replace unsupported constructs (e.g. sext/zext/fpext, values of types i64/double, etc.)!"
    #else
        //#pragma WARNING ( packetizer will not attempt to replace unsupported constructs (e.g. sext/zext/fpext, values of types i64/double, etc.)! )
    #endif
#endif


// uses custom block allocation inside packetized function
// NOTE: not fully implemented
// (default: deactivated)

//#define USE_CUSTOM_SHADER_ALLOC 
#ifdef USE_CUSTOM_SHADER_ALLOC
    #ifndef _MSC_VER
        #warning "WARNING: Custom shader memory allocation activated!"
    #else
        //#pragma WARNING ( Custom shader memory allocation activated! )
    #endif
#endif


// always execute calls to rendering api (trace, getlightcontribution etc.) scalar
// (default: deactivated)

//#define NO_PACKETIZED_API_CALLS //FIXME: does not really work anymore :P
#ifdef NO_PACKETIZED_API_CALLS
    #ifndef _MSC_VER
        #warning "WARNING: API calls are always executed 4x scalar instead of packetized!"
    #else
        //#pragma WARNING ( API calls are always executed 4x scalar instead of packetized! )
    #endif
#endif


// always execute calls (sin, log, floor, trace, etc.)
// NOTE: this has some issues with API calls (llvm assertion "Calling a function with a bad signature" in packetizeCalls())
// (default: deactivated)

//#define NO_PACKETIZED_CALLS
#ifdef NO_PACKETIZED_CALLS
    #ifndef _MSC_VER
        #warning "WARNING: *all* calls are always executed 4x scalar instead of packetized!"
    #else
        //#pragma WARNING ( *all* calls are always executed 4x scalar instead of packetized! )
    #endif
#endif


// allow irreducible control-flow
// if deactivated, packetizer exits with an appropriate error message if an irreducible
// loop is found
// NOTE: not fully implemented
// (default: deactivated)

//#define SUPPORT_IRREDUCIBLE_LOOPS


// use coherent mask branching
// NOTE: this can result both in speedups and slow-downs, it is very much depending on the function's CFG
// (default: deactivated)

//#define USE_COHERENT_MASK_BRANCHING


// use 'old' phi canonicalization
// randomly joins incoming edges
// deactivated joins edge pairs depending on the dominator relation of the
// nearest common dominators of all possible pairs
// (default: deactivated)

//#define USE_OLD_PHI_CANONICALIZATION
#if defined USE_COHERENT_MASK_BRANCHING && defined USE_OLD_PHI_CANONICALIZATION
    #undef USE_OLD_PHI_CANONICALIZATION
    #ifndef _MSC_VER
        #warning "WARNING: disabled usage of old phi canonicalization (not supported in combination with uniform mask branching)!"
    #else
        //#pragma WARNING ( disabled usage of old phi canonicalization (not supported in combination with uniform mask branching)! )
    #endif
#endif


// switch conditional store technique:
// split up store instructions into scalar stores and build if-cascades
// instead of using the default load-blend-store scheme (which should be faster)
// (default: deactivated)

//#define USE_CASCADING_SCALAR_STORE
#if defined USE_CASCADING_SCALAR_STORE
	#ifndef _MSC_VER
        #warning "WARNING: using if-cascade scalar store style for conditional stores!"
    #else
        //#pragma WARNING ( using if-cascade scalar store style for conditional stores! )
    #endif
#endif



// make use of custom powf SSE implementation
// NOTE: some shaders (e.g. c++ phong and whitted) produce artifacts
// (default: deactivated)

//#define USE_NATIVE_POW_PS
#if defined USE_NATIVE_POW_PS
	#ifndef _MSC_VER
        #warning "WARNING: using native SSE pow implementation, might produce artifacts!"
    #else
        //#pragma WARNING ( using native SSE pow implementation, might produce artifacts! )
    #endif
#endif

//----------------------------------------------------------------------------//
// debug flags
//----------------------------------------------------------------------------//

// DEBUG flag has to be set by compiler
#if defined(DEBUG) || defined(_DEBUG)
    #define DEBUG_PACKETIZER
#endif

//if NDEBUG is defined, always ignore debug information
#ifdef NDEBUG
    #undef DEBUG_PACKETIZER
#endif


//----------------------------------------------------------------------------//
// misc settings
//----------------------------------------------------------------------------//

// suppress "controlling expression is constant" warning of icc
#ifdef __INTEL_COMPILER
    #pragma warning( disable : 279 )
#endif



#endif // _PACKETIZERCONFIG_H
