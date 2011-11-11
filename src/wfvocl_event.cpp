/**
 * This file is distributed under the University of Illinois Open Source
 * License. See the COPYING file in the root directory for details.
 *
 * Copyright (C) 2010, 2011 Saarland University
 */

/**
 * The functions implemented here correspond to the functions described in
 * chapter 5.9 of the OpenCL 1.1 specification.
 */

#include "wfvocl.h"

WFVOPENCL_DLLEXPORT CL_API_ENTRY cl_event CL_API_CALL
clCreateUserEvent(cl_context    context,
                  cl_int *      errcode_ret)
{
	WFVOPENCL_DEBUG ( outs() << "ENTERED clCreateUserEvent!\n"; );
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

WFVOPENCL_DLLEXPORT CL_API_ENTRY cl_int CL_API_CALL
clSetUserEventStatus(cl_event   event,
                     cl_int     execution_status)
{
	WFVOPENCL_DEBUG ( outs() << "ENTERED clSetUserEventStatus!\n"; );
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

WFVOPENCL_DLLEXPORT CL_API_ENTRY cl_int CL_API_CALL
clWaitForEvents(cl_uint             num_events,
                const cl_event *    event_list)
{
	WFVOPENCL_DEBUG ( outs() << "ENTERED clWaitForEvents!\n"; );
	WFVOPENCL_DEBUG( outs() << "TODO: implement clWaitForEvents()\n"; );
	return CL_SUCCESS;
}

WFVOPENCL_DLLEXPORT CL_API_ENTRY cl_int CL_API_CALL
clGetEventInfo(cl_event         event,
               cl_event_info    param_name,
               size_t           param_value_size,
               void *           param_value,
               size_t *         param_value_size_ret)
{
	WFVOPENCL_DEBUG ( outs() << "ENTERED clGetEventInfo!\n"; );
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

WFVOPENCL_DLLEXPORT CL_API_ENTRY cl_int CL_API_CALL
clSetEventCallback( cl_event    event,
                    cl_int      command_exec_callback_type,
                    void (CL_CALLBACK * pfn_notify)(cl_event, cl_int, void *),
                    void *      user_data)
{
	WFVOPENCL_DEBUG ( outs() << "ENTERED clSetEventCallback!\n"; );
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

WFVOPENCL_DLLEXPORT CL_API_ENTRY cl_int CL_API_CALL
clRetainEvent(cl_event event)
{
	WFVOPENCL_DEBUG ( outs() << "ENTERED clRetainEvent!\n"; );
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

WFVOPENCL_DLLEXPORT CL_API_ENTRY cl_int CL_API_CALL
clReleaseEvent(cl_event event)
{
	WFVOPENCL_DEBUG ( outs() << "ENTERED clReleaseEvent!\n"; );
	_cl_event* ptr = (_cl_event*)event;
	delete ptr;
	return CL_SUCCESS;
}
