/**
 * This file is distributed under the University of Illinois Open Source
 * License. See the COPYING file in the root directory for details.
 *
 * Copyright (C) 2010, 2011 Saarland University
 */

/**
 * The functions implemented here correspond to the functions described in
 * chapter 5.12 of the OpenCL 1.1 specification.
 */

#include "wfvocl.h"

WFVOPENCL_DLLEXPORT CL_API_ENTRY cl_int CL_API_CALL
clGetEventProfilingInfo(cl_event            event,
                        cl_profiling_info   param_name,
                        size_t              param_value_size,
                        void *              param_value,
                        size_t *            param_value_size_ret)
{
    WFVOPENCL_DEBUG ( outs() << "ENTERED clGetEventProfilingInfo!\n"; );
    assert (false && "NOT IMPLEMENTED!");
    return CL_SUCCESS;
}
