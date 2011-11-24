/**
 * This file is distributed under the University of Illinois Open Source
 * License. See the COPYING file in the root directory for details.
 *
 * Copyright (C) 2010, 2011 Saarland University
 */

/**
 * The functions implemented here correspond to the functions described in
 * chapter 5.6 of the OpenCL 1.1 specification.
 */

#include "wfvocl.h"

/*
creates a program object for a context, and loads the source code specified by the text strings in
the strings array into the program object. The devices associated with the program object are the
devices associated with context.
*/
// -> read strings and store as .cl representation
WFVOPENCL_DLLEXPORT CL_API_ENTRY cl_program CL_API_CALL
clCreateProgramWithSource(cl_context        context,
                          cl_uint           count,
                          const char **     strings,
                          const size_t *    lengths,
                          cl_int *          errcode_ret)
{
	WFVOPENCL_DEBUG ( outs() << "ENTERED clCreateProgramWithSource!\n"; );
	*errcode_ret = CL_SUCCESS;
	_cl_program* p = new _cl_program();
	p->dispatch = &static_dispatch;
	p->context = context;

	// create temp filename
	char* tmpFilename = (char*)malloc(L_tmpnam * sizeof(char));
	tmpnam(tmpFilename);
	p->fileName = tmpFilename;

	// write to temp file
	std::ofstream of(tmpFilename);
	if (!of.good()) {
		*errcode_ret = CL_OUT_OF_RESOURCES;
		return NULL;
	}
	of << *strings;
	of.close();

	return p;
}

// -> read binary and store as .cl representation
WFVOPENCL_DLLEXPORT CL_API_ENTRY cl_program CL_API_CALL
clCreateProgramWithBinary(cl_context                     context,
                          cl_uint                        num_devices,
                          const cl_device_id *           device_list,
                          const size_t *                 lengths,
                          const unsigned char **         binaries,
                          cl_int *                       binary_status,
                          cl_int *                       errcode_ret)
{
	WFVOPENCL_DEBUG ( outs() << "ENTERED clCreateProgramWithBinary!\n"; );
	assert (false && "NOT IMPLEMENTED!");
	return NULL;
}

WFVOPENCL_DLLEXPORT CL_API_ENTRY cl_int CL_API_CALL
clRetainProgram(cl_program program)
{
	WFVOPENCL_DEBUG ( outs() << "ENTERED clRetainProgram!\n"; );
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

WFVOPENCL_DLLEXPORT CL_API_ENTRY cl_int CL_API_CALL
clReleaseProgram(cl_program program)
{
	WFVOPENCL_DEBUG ( outs() << "ENTERED clReleaseProgram!\n"; );
#ifdef WFVOPENCL_ENABLE_JIT_PROFILING
	int success = iJIT_NotifyEvent(iJVM_EVENT_TYPE_SHUTDOWN, NULL);
	if (success != 1) {
		errs() << "ERROR: termination of profiling failed!\n";
	}
#endif
	_cl_program* ptr = (_cl_program*)program;
	delete ptr->targetData;
	delete ptr->module;
	delete ptr;
	return CL_SUCCESS;
}

/*
builds (compiles & links) a program executable from the program source or binary for all the
devices or a specific device(s) in the OpenCL context associated with program. OpenCL allows
program executables to be built using the source or the binary. clBuildProgram must be called
for program created using either clCreateProgramWithSource or
clCreateProgramWithBinary to build the program executable for one or more devices
associated with program.
*/
// -> build LLVM module from .cl representation (from createProgramWithSource/Binary)
// -> invoke clc
// -> invoke llvm-as
// -> store module in _cl_program object
WFVOPENCL_DLLEXPORT CL_API_ENTRY cl_int CL_API_CALL
clBuildProgram(cl_program           program,
               cl_uint              num_devices,
               const cl_device_id * device_list,
               const char *         options,
               void (CL_CALLBACK *  pfn_notify)(cl_program /* program */, void * /* user_data */),
               void *               user_data)
{
	WFVOPENCL_DEBUG ( outs() << "ENTERED clBuildProgram!\n"; );
	if (!program) return CL_INVALID_PROGRAM;
	if (!device_list && num_devices > 0) return CL_INVALID_VALUE;
	if (device_list && num_devices == 0) return CL_INVALID_VALUE;
	if (user_data && !pfn_notify) return CL_INVALID_VALUE;

	// create filename for clc output
	char clcOutPath[L_tmpnam];
	tmpnam(clcOutPath);

	// compile using clc
	std::stringstream clcCmd;
	clcCmd << "clc -o " << clcOutPath << " --msse2 " << program->fileName;
	system(clcCmd.str().c_str());

	// assemble and load module
	llvm::SMDiagnostic asmErr;
	llvm::LLVMContext& context = llvm::getGlobalContext();
	llvm::Module* mod = llvm::ParseAssemblyFile(clcOutPath, asmErr, context);

	// remove temp outputs
	remove(clcOutPath);
	remove(program->fileName);

	// check if module has been loaded
	if (!mod) return CL_BUILD_PROGRAM_FAILURE;
	WFVOPENCL_DEBUG( WFVOpenCL::writeModuleToFile(mod, "debug_kernel_orig_orig_targetdata.mod.ll"); );

	// TODO: do this here or only after packetization?
	mod->setDataLayout(WFVOPENCL_LLVM_DATA_LAYOUT_64);
	// we have to reset the target triple (LLVM does not know amd-opencl)
	//mod->setTargetTriple("");
#if defined _WIN32
	mod->setTargetTriple("x86_64-pc-win32");
#elif defined __APPLE__
	mod->setTargetTriple("x86_64-apple-darwin10.0.0");
#elif defined __linux
	mod->setTargetTriple("x86_64-unknown-linux-gnu");
#else
#	error "unknown platform found, can not assign correct target triple!");
#endif
	program->targetData = new TargetData(mod);

	program->module = mod;
	return CL_SUCCESS;
}

WFVOPENCL_DLLEXPORT CL_API_ENTRY cl_int CL_API_CALL
clUnloadCompiler(void)
{
	WFVOPENCL_DEBUG ( outs() << "ENTERED clUnloadCompiler!\n"; );
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

WFVOPENCL_DLLEXPORT CL_API_ENTRY cl_int CL_API_CALL
clGetProgramInfo(cl_program         program,
                 cl_program_info    param_name,
                 size_t             param_value_size,
                 void *             param_value,
                 size_t *           param_value_size_ret)
{
	WFVOPENCL_DEBUG ( outs() << "ENTERED clGetProgramInfo!\n"; );
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

WFVOPENCL_DLLEXPORT CL_API_ENTRY cl_int CL_API_CALL
clGetProgramBuildInfo(cl_program            program,
                      cl_device_id          device,
                      cl_program_build_info param_name,
                      size_t                param_value_size,
                      void *                param_value,
                      size_t *              param_value_size_ret)
{
	WFVOPENCL_DEBUG ( outs() << "ENTERED clGetProgramBuildInfo!\n"; );
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}
