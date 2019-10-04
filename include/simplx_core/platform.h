/**
 * @file platform.h
 * @brief cross-platform system wrapper
 * @copyright 2019 Scalewatch (www.scalewatch.com). All rights reserved.
 * Please see accompanying LICENSE file for licensing terms.
 */

#pragma once

//---- Debug/Release -----------------------------------------------------------

#if (defined(NDEBUG) && defined(DEBUG))
    // debug flags error
    #error conflicting NDEBUG & DEBUG defines
#else
    #if (!defined(NDEBUG) || defined(DEBUG))
        #ifndef DEBUG
            #define DEBUG
        #endif
        #define SIMPLX_DEBUG      1
        #define SIMPLX_RELEASE    0
    #else
        #ifndef RELEASE
            #define RELEASE
        #endif
        #define SIMPLX_DEBUG      0
        #define SIMPLX_RELEASE    1
    #endif
#endif

//---- C++11 support -----------------------------------------------------------

/*
 * Detect c++11 and set macro
 * cf: http://en.cppreference.com/w/cpp/preprocessor/replace
 */
#if __cplusplus <= 199711L
    #error C++11 is required
#endif

//---- Endianness --------------------------------------------------------------

#if (defined(SIMPLX_LITTLE_ENDIAN) || defined(SIMPLX_BIG_ENDIAN))
    // endianness error
    #error endianness is defined by COMPILER itself, not user-defined compiler flags
#endif

#ifndef SIMPLX_STREAM_SWAP
    // don't swap network serializations by default
    #define SIMPLX_STREAM_SWAP   0
#endif 
    
//---- OS platform -------------------------------------------------------------

#if !defined(SIMPLX_PLATFORM_LINUX) && defined(__linux__)
    #define SIMPLX_PLATFORM_LINUX
#elif !defined(SIMPLX_PLATFORM_APPLE) && defined(__APPLE__)
    #error Apple platform is currently under development!
#elif !defined(SIMPLX_PLATFORM_WINDOWS) && (defined(_WIN32) || defined(__CYGWIN__))
    #error Windows platform is currently under development!
#endif

#if defined(SIMPLX_PLATFORM_LINUX)
    #if defined(__GNUG__)
        #include "simplx_core/internal/linux/platform_gcc.h"
    #else
        #error No supported C++ compiler for LINUX
    #endif
    
    #include "simplx_core/internal/linux/platform_linux.h"
    
#elif defined(SIMPLX_PLATFORM_APPLE)
    #error Apple platform is currently under development!
#elif defined(SIMPLX_PLATFORM_WINDOWS)
    #error Windows platform is currently under development!
#else
    // unsupported platform
    #error Undefined OS level symbol SIMPLX_PLATFORM_xxx (where xxx is LINUX or APPLE or WINDOWS)
#endif

//---- Log ---------------------------------------------------------------------

#if (!defined(SIMPLX_LOG))
    // if wasn't user-defined
    #if (SIMPLX_DEBUG == 1)
        // in debug is on by default
        #define SIMPLX_LOG        1
    #else
        // in Release default is off
        #define SIMPLX_LOG        0
    #endif
#endif

//---- References --------------------------------------------------------------

#if (defined(REF_DEBUG) || defined(DEBUG_REF))
    // warn about deprecated flags
    #pragma message "REF_DEBUG/DEBUG_REF is unknown, did you mean TRACE_REF ?"
#endif

#ifdef TRACE_REF
    #include "simplx_core/traceref.h"
#else
    // dummy when no Enterprise license
    #include "simplx_core/internal/dummy/traceref_dummy.h"
#endif

#if (!defined(SIMPLX_CHECK_CYCLICAL_REFS))
    // if wasn't user-defined
    #if (SIMPLX_DEBUG == 1)
        #define SIMPLX_CHECK_CYCLICAL_REFS    1
    #else
        #define SIMPLX_CHECK_CYCLICAL_REFS    0
    #endif
#endif

//---- Enterprise --------------------------------------------------------------

#ifdef SIMPLX_E2E
    #if (SIMPLX_E2E != 0)
        #undef SIMPLX_E2E
        #define SIMPLX_E2E    1
    #endif
#endif // SIMPLX_E2E


// nada mas