/**
 * This file is distributed under the University of Illinois Open Source
 * License. See the COPYING file in the root directory for details.
 *
 * Copyright (C) 2010, 2011 Saarland University
 */

/**
 * The functions here implement the cl_khr_icd extension
 */


#include "wfvocl.h"

WFVOPENCL_DLLEXPORT CL_API_ENTRY cl_int CL_API_CALL
clIcdGetPlatformIDsKHR(cl_uint              num_entries,
                           cl_platform_id * platforms,
                           cl_uint *        num_platforms)
{
    WFVOPENCL_DEBUG ( outs() << "ENTERED clIcdGetPlatformIDsKHR!\n"; );
    WFVOPENCL_DEBUG ( outs() << "  num_entries: " << num_entries << "\n"; );
    WFVOPENCL_DEBUG ( outs() << "  platforms: " << (void*)platforms << "\n"; );
    WFVOPENCL_DEBUG ( if (num_platforms) outs() << "  num_platforms: " << *num_platforms << "\n"; );

    if (num_entries == 0 && platforms) return CL_INVALID_VALUE;
    if (!num_platforms && !platforms) return CL_INVALID_VALUE;

    //if (!platforms) return CL_PLATFORM_NOT_FOUND_KHR;

    if (platforms) {
        platforms[0] = &static_platform;
    }
    if (num_platforms) {
        *num_platforms = 1;
    }

    return CL_SUCCESS;
}
