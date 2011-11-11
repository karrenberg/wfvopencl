/**
 * This file is distributed under the University of Illinois Open Source
 * License. See the COPYING file in the root directory for details.
 *
 * Copyright (C) 2010, 2011 Saarland University
 */

/**
 * The functions implemented here correspond to the functions described in
 * chapter 5.5 of the OpenCL 1.1 specification.
 */

#include "wfvocl.h"

WFVOPENCL_DLLEXPORT CL_API_ENTRY cl_sampler CL_API_CALL
clCreateSampler(cl_context          context,
                cl_bool             normalized_coords,
                cl_addressing_mode  addressing_mode,
                cl_filter_mode      filter_mode,
                cl_int *            errcode_ret)
{
	WFVOPENCL_DEBUG ( outs() << "ENTERED clCreateSampler!\n"; );
	assert (false && "NOT IMPLEMENTED!");
	return NULL;
}

WFVOPENCL_DLLEXPORT CL_API_ENTRY cl_int CL_API_CALL
clRetainSampler(cl_sampler sampler)
{
	WFVOPENCL_DEBUG ( outs() << "ENTERED clRetainSampler!\n"; );
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

WFVOPENCL_DLLEXPORT CL_API_ENTRY cl_int CL_API_CALL
clReleaseSampler(cl_sampler sampler)
{
	WFVOPENCL_DEBUG ( outs() << "ENTERED clReleaseSampler!\n"; );
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

WFVOPENCL_DLLEXPORT CL_API_ENTRY cl_int CL_API_CALL
clGetSamplerInfo(cl_sampler         sampler,
                 cl_sampler_info    param_name,
                 size_t             param_value_size,
                 void *             param_value,
                 size_t *           param_value_size_ret)
{
	WFVOPENCL_DEBUG ( outs() << "ENTERED clGetSamplerInfo!\n"; );
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

