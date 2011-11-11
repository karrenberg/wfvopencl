/**
 * This file is distributed under the University of Illinois Open Source
 * License. See the COPYING file in the root directory for details.
 *
 * Copyright (C) 2010, 2011 Saarland University
 */

/**
 * The functions implemented here correspond to the functions described in
 * chapter 5.4 of the OpenCL 1.1 specification.
 */

#include "wfvocl.h"

WFVOPENCL_DLLEXPORT CL_API_ENTRY cl_int CL_API_CALL
clRetainMemObject(cl_mem memobj)
{
	WFVOPENCL_DEBUG ( outs() << "ENTERED clRetainMemObject!\n"; );
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

WFVOPENCL_DLLEXPORT CL_API_ENTRY cl_int CL_API_CALL
clReleaseMemObject(cl_mem memobj)
{
	WFVOPENCL_DEBUG ( outs() << "ENTERED clReleaseMemObject!\n"; );
	_cl_mem* ptr = (_cl_mem*)memobj;
	delete ptr;
	return CL_SUCCESS;
}

WFVOPENCL_DLLEXPORT CL_API_ENTRY cl_int CL_API_CALL
clSetMemObjectDestructorCallback(cl_mem memobj,
								 void (CL_CALLBACK * pfn_notify)( cl_mem /* memobj */, void* /*user_data*/),
								 void * user_data)
{
	WFVOPENCL_DEBUG ( outs() << "ENTERED clSetMemObjectDestructorCallback!\n"; );
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

WFVOPENCL_DLLEXPORT CL_API_ENTRY cl_int CL_API_CALL
clEnqueueUnmapMemObject(cl_command_queue command_queue,
                        cl_mem           memobj,
                        void *           mapped_ptr,
                        cl_uint          num_events_in_wait_list,
                        const cl_event *  event_wait_list,
                        cl_event *        event)
{
	WFVOPENCL_DEBUG ( outs() << "ENTERED clEnqueueUnmapMemObject!\n"; );
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

WFVOPENCL_DLLEXPORT CL_API_ENTRY cl_int CL_API_CALL
clGetMemObjectInfo(cl_mem           memobj,
                   cl_mem_info      param_name,
                   size_t           param_value_size,
                   void *           param_value,
                   size_t *         param_value_size_ret)
{
	WFVOPENCL_DEBUG ( outs() << "ENTERED clGetMemObjectInfo!\n"; );
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}
