/**
 * This file is distributed under the University of Illinois Open Source
 * License. See the COPYING file in the root directory for details.
 *
 * Copyright (C) 2010, 2011 Saarland University
 */

/**
 * The functions implemented here correspond to the functions described in
 * chapter 5.13 of the OpenCL 1.1 specification.
 */

#include "wfvocl.h"

WFVOPENCL_DLLEXPORT CL_API_ENTRY cl_int CL_API_CALL
clFlush(cl_command_queue command_queue)
{
    WFVOPENCL_DEBUG ( outs() << "ENTERED clFlush!\n"; );
    WFVOPENCL_DEBUG( outs() << "TODO: implement clFlush()\n"; );
    return CL_SUCCESS;
}

WFVOPENCL_DLLEXPORT CL_API_ENTRY cl_int CL_API_CALL
clFinish(cl_command_queue command_queue)
{
    WFVOPENCL_DEBUG ( outs() << "ENTERED clFinish!\n"; );
    if (!command_queue) return CL_INVALID_COMMAND_QUEUE;
    // do nothing :P
    return CL_SUCCESS;
}
