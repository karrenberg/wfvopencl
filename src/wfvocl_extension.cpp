/**
 * This file is distributed under the University of Illinois Open Source
 * License. See the COPYING file in the root directory for details.
 *
 * Copyright (C) 2010, 2011 Saarland University
 */

/**
 * The functions implemented here correspond to the functions described in
 * chapter 9.2 of the OpenCL 1.1 specification.
 */

#include "wfvocl.h"

/* Extension function access
 *
 * Returns the extension function address for the given function name,
 * or NULL if a valid function can not be found.  The client must
 * check to make sure the address is not NULL, before using or
 * calling the returned function address.
 */
WFVOPENCL_DLLEXPORT CL_API_ENTRY void * CL_API_CALL clGetExtensionFunctionAddress(const char * func_name)
{
	WFVOPENCL_DEBUG ( outs() << "ENTERED clGetExtensionFunctionAddress!\n"; );
	WFVOPENCL_DEBUG ( outs() << "  func_name: " << func_name << "\n"; );
	// This is for identification by the ICD mechanism
	if (!strcmp(func_name, "clIcdGetPlatformIDsKHR")) {
		return (void*)clIcdGetPlatformIDsKHR;
	}

	return (void*)clIcdGetPlatformIDsKHR;


	// If we add any additional extensions, we have to insert something here
	// that queries the func_name for our suffix and returns the appropriate
	// function.

	//return NULL;
}
