/**
 * This file is distributed under the University of Illinois Open Source
 * License. See the COPYING file in the root directory for details.
 *
 * Copyright (C) 2010, 2011 Saarland University
 */

/**
 * The functions implemented here correspond to the functions described in
 * chapter 5.10 of the OpenCL 1.1 specification.
 */

#include "wfvocl.h"

WFVOPENCL_DLLEXPORT CL_API_ENTRY cl_int CL_API_CALL
clEnqueueMarker(cl_command_queue    command_queue,
                cl_event *          event)
{
	WFVOPENCL_DEBUG ( outs() << "ENTERED clEnqueueMarker!\n"; );
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

WFVOPENCL_DLLEXPORT CL_API_ENTRY cl_int CL_API_CALL
clEnqueueBarrier(cl_command_queue command_queue)
{
	WFVOPENCL_DEBUG ( outs() << "ENTERED clEnqueueBarrier!\n"; );
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

WFVOPENCL_DLLEXPORT CL_API_ENTRY cl_int CL_API_CALL
clEnqueueWaitForEvents(cl_command_queue command_queue,
                       cl_uint          num_events,
                       const cl_event * event_list)
{
	WFVOPENCL_DEBUG ( outs() << "ENTERED clEnqueueWaitForEvents!\n"; );
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}
