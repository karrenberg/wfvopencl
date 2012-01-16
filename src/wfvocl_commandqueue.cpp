/**
 * This file is distributed under the University of Illinois Open Source
 * License. See the COPYING file in the root directory for details.
 *
 * Copyright (C) 2010, 2011 Saarland University
 */

/**
 * The functions implemented here correspond to the functions described in
 * chapter 5.1 of the OpenCL 1.1 specification.
 */

#include "wfvocl.h"

/*
creates a command-queue on a specific device.
*/
// -> ??
WFVOPENCL_DLLEXPORT CL_API_ENTRY cl_command_queue CL_API_CALL
clCreateCommandQueue(cl_context                     context,
                     cl_device_id                   device,
                     cl_command_queue_properties    properties,
                     cl_int *                       errcode_ret)
{
	WFVOPENCL_DEBUG ( outs() << "ENTERED clCreateCommandQueue!\n"; );
	errcode_ret = CL_SUCCESS;
	_cl_command_queue* cq = new _cl_command_queue();
	cq->dispatch = &static_dispatch;
	cq->context = context;
	return cq;
}

WFVOPENCL_DLLEXPORT CL_API_ENTRY cl_int CL_API_CALL
clRetainCommandQueue(cl_command_queue command_queue)
{
	WFVOPENCL_DEBUG ( outs() << "ENTERED clRetainCommandQueue!\n"; );
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

WFVOPENCL_DLLEXPORT CL_API_ENTRY cl_int CL_API_CALL
clReleaseCommandQueue(cl_command_queue command_queue)
{
	WFVOPENCL_DEBUG ( outs() << "ENTERED clReleaseCommandQueue!\n"; );
	_cl_command_queue* ptr = (_cl_command_queue*)command_queue;
	delete ptr;
	return CL_SUCCESS;
}

WFVOPENCL_DLLEXPORT CL_API_ENTRY cl_int CL_API_CALL
clGetCommandQueueInfo(cl_command_queue      command_queue,
                      cl_command_queue_info param_name,
                      size_t                param_value_size,
                      void *                param_value,
                      size_t *              param_value_size_ret)
{
	WFVOPENCL_DEBUG ( outs() << "ENTERED clGetCommandQueueInfo!\n"; );
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

//#ifdef CL_USE_DEPRECATED_OPENCL_1_0_APIS
WFVOPENCL_DLLEXPORT CL_API_ENTRY cl_int CL_API_CALL
clSetCommandQueueProperty(cl_command_queue             command_queue,
                          cl_command_queue_properties  properties,
                          cl_bool                      enable,
                          cl_command_queue_properties* old_properties)
{
	WFVOPENCL_DEBUG ( outs() << "ENTERED clSetCommandQueueProperty!\n"; );
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}
//#endif /* CL_USE_DEPRECATED_OPENCL_1_0_APIS */
