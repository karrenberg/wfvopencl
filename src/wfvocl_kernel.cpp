/**
 * This file is distributed under the University of Illinois Open Source
 * License. See the COPYING file in the root directory for details.
 *
 * Copyright (C) 2010, 2011 Saarland University
 */

/**
 * The functions implemented here correspond to the functions described in
 * chapter 5.7 and 5.8 of the OpenCL 1.1 specification.
 */

#include "cast.h"
#include "wfvocl.h"

/**
 * Helper for clEnqueueNDRangeKernel
 */
inline cl_int executeRangeKernel1D(cl_kernel kernel, const size_t global_work_size, const size_t local_work_size) {
    WFVOPENCL_DEBUG( outs() << "  global_work_size: " << global_work_size << "\n"; );
    WFVOPENCL_DEBUG( outs() << "  local_work_size: " << local_work_size << "\n"; );
    if (global_work_size % local_work_size != 0) return CL_INVALID_WORK_GROUP_SIZE;
    //if (global_work_size[0] > pow(2, sizeof(size_t)) /* oder so :P */) return CL_OUT_OF_RESOURCES;

typedef void (*kernelFnPtr)(
            const void*,
            const cl_uint,
            const cl_uint*,
            const cl_uint*,
            const cl_int*);
    kernelFnPtr typedPtr = ptr_cast<kernelFnPtr>(kernel->get_compiled_function());

    const void* argument_struct = kernel->get_argument_struct();

    // In general it should be faster to use global_size instead of simd_width
    // In any case, changing the local work size can introduce arbitrary problems
    // except for the case where it is 1.

#ifndef WFVOPENCL_NO_WFV
    assert (global_work_size >= WFVOPENCL_SIMD_WIDTH);
    assert (local_work_size == 1 || local_work_size >= WFVOPENCL_SIMD_WIDTH);
    assert (global_work_size % WFVOPENCL_SIMD_WIDTH == 0);
    assert (local_work_size == 1 || local_work_size % WFVOPENCL_SIMD_WIDTH == 0);
#endif

    // unfortunately we have to convert to 32bit values because we work with 32bit internally
    // TODO: in the 1D case we can optimize because only the first value is loaded (automatic truncation)
    const cl_uint modified_global_work_size = (cl_uint)global_work_size;

#ifdef WFVOPENCL_NO_WFV
    const cl_uint modified_local_work_size = (cl_uint)local_work_size;
#else
    if (local_work_size != 1 && local_work_size < WFVOPENCL_SIMD_WIDTH) {
        errs() << "\nERROR: group size of dimension " << kernel->get_best_simd_dim() << " is smaller than the SIMD width!\n\n";
        exit(-1);
    }
    WFVOPENCL_DEBUG(
        if (local_work_size == 1) {
            errs() << "\nWARNING: group size of dimension " << kernel->get_best_simd_dim() << " is 1, will be increased to multiple of SIMD width!\n\n";
        }
    );

#   ifdef WFVOPENCL_USE_OPENMP
    // If the local work size is set to 1, we should be safe to set it to some arbitrary
    // value unless the application does weird things.
    // TODO: Test if kernel calls get_group_id or get_group_size, in which case we must not change anything!
    // If not, the natural choice is to set the work size in a way that we end up with
    // exactly as many iterations of the outermost loop as we have cores for multi-threading.
    // Using larger amounts of iterations can severely degrade performance (e.g. FloydWarshall, Mandelbrot)
    const cl_uint modified_local_work_size = local_work_size == 1 ?
        modified_global_work_size/WFVOPENCL_NUM_CORES : (cl_uint)local_work_size;
#   else
    const cl_uint modified_local_work_size = local_work_size == 1 ?
        modified_global_work_size : (cl_uint)local_work_size;
#   endif

#endif

    //
    // execute the kernel
    //
    const cl_uint num_iterations = modified_global_work_size / modified_local_work_size; // = total # threads per block
    WFVOPENCL_DEBUG( outs() << "  modified_global_work_size: " << modified_global_work_size << "\n"; );
    WFVOPENCL_DEBUG( outs() << "  modified_local_work_size: " << modified_local_work_size << "\n"; );
    WFVOPENCL_DEBUG( outs() << "\nexecuting kernel (#iterations: " << num_iterations << ")...\n"; );

    assert (num_iterations > 0 && "should give error message before executeRangeKernel!");

#ifdef WFVOPENCL_USE_OPENMP
    // allocate local memory for each thread to prevent data races
    // TODO: move somewhere else? should all be "static" information!
    const cl_uint numArgs = kernel->get_num_args();
    void* argstructs[WFVOPENCL_MAX_NUM_THREADS];
    void** localdata[WFVOPENCL_MAX_NUM_THREADS]; // too much, but easier to access

    const cl_uint argStrSize = kernel->get_argument_struct_size();
    for (cl_uint j=0; j<WFVOPENCL_MAX_NUM_THREADS; ++j) {
        localdata[j] = (void**)malloc(numArgs*sizeof(void*));
        for (cl_uint i=0; i<numArgs; ++i) {
            if (kernel->arg_is_local(i)) {
                const size_t argSize = kernel->arg_get_size(i);
                // allocate memory for this local pointer (store pointer to be able free later)
                localdata[j][i] = malloc(argSize);
                // store in kernel (overwrite in each thread-iteration)
                void* ldata = kernel->arg_get_data(i);
                *((void**)ldata) = localdata[j][i];
            }
        }
        // now copy entire argument struct with updated local pointers
        argstructs[j] = malloc(argStrSize);
        memcpy(argstructs[j], argument_struct, argStrSize);
    }
#endif

    cl_int i;

#ifdef WFVOPENCL_USE_OPENMP
    omp_set_num_threads(WFVOPENCL_MAX_NUM_THREADS);
#   pragma omp parallel for shared(argument_struct, kernel) private(i)
#endif
    for (i=0; i<num_iterations; ++i) {
        WFVOPENCL_DEBUG_RUNTIME( outs() << "\niteration " << i << " (= group id)\n"; );
        WFVOPENCL_DEBUG_RUNTIME( verifyModule(*kernel->get_program()->module); );
        WFVOPENCL_DEBUG_RUNTIME( outs() << "  verification before execution successful!\n"; );

#ifdef WFVOPENCL_USE_OPENMP
        // fetch this thread's argument struct
        const cl_uint tid = omp_get_thread_num();
        void* newargstr = argstructs[tid];
        // TODO: Adding a local copy seems to help OpenMP-based implementation?!
        //       Otherwise, TestBarrier2 on Windows crashed (regardless of wrong results).
        //const cl_uint mg = modified_global_work_size;
        //const cl_uint ml = modified_local_work_size;
        //typedPtr(newargstr, 1U, &mg, &ml, &i);
        typedPtr(newargstr, 1U, &modified_global_work_size, &modified_local_work_size, &i);
#else
        typedPtr(argument_struct, 1U, &modified_global_work_size, &modified_local_work_size, &i);
#endif


        WFVOPENCL_DEBUG_RUNTIME( outs() << "iteration " << i << " finished!\n"; );
        WFVOPENCL_DEBUG_RUNTIME( verifyModule(*kernel->get_program()->module); );
        WFVOPENCL_DEBUG_RUNTIME( outs() << "  verification after execution successful!\n"; );
    }

#ifdef WFVOPENCL_USE_OPENMP
    // clean up memory allocated for local data and each thread's argument struct
    for (cl_uint i=0; i<numArgs; ++i) {
        if (kernel->arg_is_local(i)) {
            for (cl_uint j=0; j<WFVOPENCL_MAX_NUM_THREADS; ++j) {
                free(localdata[j][i]);
            }
        }
    }
    for (cl_uint j=0; j<WFVOPENCL_MAX_NUM_THREADS; ++j) {
        free(argstructs[j]);
    }
#endif

    WFVOPENCL_DEBUG( outs() << "execution of kernel finished!\n"; );

    return CL_SUCCESS;
}
inline cl_int executeRangeKernel2D(cl_kernel kernel, const size_t* global_work_size, const size_t* local_work_size) {
    WFVOPENCL_DEBUG( outs() << "  global_work_sizes: " << global_work_size[0] << ", " << global_work_size[1] << "\n"; );
    WFVOPENCL_DEBUG( outs() << "  local_work_sizes: " << local_work_size[0] << ", " << local_work_size[1] << "\n"; );
    if (global_work_size[0] % local_work_size[0] != 0) return CL_INVALID_WORK_GROUP_SIZE;
    if (global_work_size[1] % local_work_size[1] != 0) return CL_INVALID_WORK_GROUP_SIZE;
    //if (global_work_size[0] > pow(2, sizeof(size_t)) /* oder so :P */) return CL_OUT_OF_RESOURCES;

    typedef void (*kernelFnPtr)(
            const void*,
            const cl_uint,
            const cl_uint*,
            const cl_uint*,
            const cl_int*);
    kernelFnPtr typedPtr = ptr_cast<kernelFnPtr>(kernel->get_compiled_function());

    const void* argument_struct = kernel->get_argument_struct();

    // unfortunately we have to convert to 32bit values because we work with 32bit internally
    const cl_uint modified_global_work_size[2] = { (cl_uint)global_work_size[0], (cl_uint)global_work_size[1] };
    const cl_uint modified_local_work_size[2] = { (cl_uint)local_work_size[0], (cl_uint)local_work_size[1] };

#ifndef WFVOPENCL_NO_WFV
    const cl_uint simd_dim = kernel->get_best_simd_dim();

    assert (global_work_size[simd_dim] >= WFVOPENCL_SIMD_WIDTH);
    assert (local_work_size[simd_dim] == 1 || local_work_size[simd_dim] >= WFVOPENCL_SIMD_WIDTH);
    assert (global_work_size[simd_dim] % WFVOPENCL_SIMD_WIDTH == 0);
    assert (local_work_size[simd_dim] == 1 || local_work_size[simd_dim] % WFVOPENCL_SIMD_WIDTH == 0);
#endif

    // TODO: insert warnings as in 1D case if sizes do not match simd width etc.

    //
    // execute the kernel
    //
    const cl_uint num_iterations_0 = modified_global_work_size[0] / modified_local_work_size[0]; // = total # threads per block in dim 0
    const cl_uint num_iterations_1 = modified_global_work_size[1] / modified_local_work_size[1]; // = total # threads per block in dim 1
    WFVOPENCL_DEBUG( outs() << "  modified_global_work_sizes: " << modified_global_work_size[0] << " / " << modified_global_work_size[1] << "\n"; );
    WFVOPENCL_DEBUG( outs() << "  modified_local_work_sizes: " << modified_local_work_size[0] << " / " << modified_local_work_size[1] << "\n"; );
    WFVOPENCL_DEBUG( outs() << "executing kernel (#iterations: " << num_iterations_0 * num_iterations_1 << ")...\n"; );

    assert (num_iterations_0 > 0 && num_iterations_1 > 0 && "should give error message before executeRangeKernel!");

#ifdef WFVOPENCL_USE_OPENMP
    // allocate local memory for each thread to prevent data races
    // TODO: move somewhere else? should all be "static" information!
    const cl_uint numArgs = kernel->get_num_args();
    void* argstructs[WFVOPENCL_MAX_NUM_THREADS];
    void** localdata[WFVOPENCL_MAX_NUM_THREADS]; // too much, but easier to access

    const cl_uint argStrSize = kernel->get_argument_struct_size();
    for (cl_uint j=0; j<WFVOPENCL_MAX_NUM_THREADS; ++j) {
        localdata[j] = (void**)malloc(numArgs*sizeof(void*));
        for (cl_uint i=0; i<numArgs; ++i) {
            if (kernel->arg_is_local(i)) {
                const size_t argSize = kernel->arg_get_size(i);
                // allocate memory for this local pointer (store pointer to be able free later)
                localdata[j][i] = malloc(argSize);
                // store in kernel (overwrite in each thread-iteration)
                void* ldata = kernel->arg_get_data(i);
                *((void**)ldata) = localdata[j][i];
            }
        }
        // now copy entire argument struct with updated local pointers
        argstructs[j] = malloc(argStrSize);
        memcpy(argstructs[j], argument_struct, argStrSize);
    }
#endif

    cl_int i, j;

#ifdef WFVOPENCL_USE_OPENMP
    omp_set_num_threads(WFVOPENCL_MAX_NUM_THREADS);
#   ifdef _WIN32
#      pragma omp parallel for shared(argument_struct) private(i, j) // VS2010 only supports OpenMP 2.5
#   else
#      pragma omp parallel for shared(argument_struct) private(i, j) collapse(2) // collapse requires OpenMP 3.0
#   endif
#endif
    for (i=0; i<num_iterations_0; ++i) {
        for (j=0; j<num_iterations_1; ++j) {
            WFVOPENCL_DEBUG_RUNTIME( outs() << "\niteration " << i << "/"  << j << " (= group ids)\n"; );
            WFVOPENCL_DEBUG_RUNTIME( verifyModule(*kernel->get_program()->module); );

            const cl_int group_id[2] = { i, j };

#ifdef WFVOPENCL_USE_OPENMP
            // fetch this thread's argument struct
            const cl_uint tid = omp_get_thread_num();
            void* newargstr = argstructs[tid];
            // TODO: Adding a local copy seems to help OpenMP-based implementation?!
            //       Otherwise, TestBarrier2 on Windows crashed (regardless of wrong results).
            //const cl_uint mg = modified_global_work_size;
            //const cl_uint ml = modified_local_work_size;
            //typedPtr(newargstr, 1U, &mg, &ml, &i);
            typedPtr(newargstr,
                2U,
                modified_global_work_size,
                modified_local_work_size,
                group_id
            );
#else
            typedPtr(
                argument_struct,
                2U, // get_work_dim
                modified_global_work_size,
                modified_local_work_size,
                group_id
            );
#endif

            WFVOPENCL_DEBUG_RUNTIME( outs() << "iteration " << i << "/" << j << " finished!\n"; );
            WFVOPENCL_DEBUG_RUNTIME( verifyModule(*kernel->get_program()->module); );
        }
    }

#ifdef WFVOPENCL_USE_OPENMP
    // clean up memory allocated for local data and each thread's argument struct
    for (cl_uint i=0; i<numArgs; ++i) {
        if (kernel->arg_is_local(i)) {
            for (cl_uint j=0; j<WFVOPENCL_MAX_NUM_THREADS; ++j) {
                free(localdata[j][i]);
            }
        }
    }
    for (cl_uint j=0; j<WFVOPENCL_MAX_NUM_THREADS; ++j) {
        free(argstructs[j]);
    }
#endif

    WFVOPENCL_DEBUG( outs() << "execution of kernel finished!\n"; );

    return CL_SUCCESS;
}
inline cl_int executeRangeKernel3D(cl_kernel kernel, const size_t* global_work_size, const size_t* local_work_size) {
    assert (false && "NOT IMPLEMENTED!");
    outs() << "Support for kernels with #dimensions > 2 not fully implemented yet!\n";
    return CL_INVALID_WORK_DIMENSION;

    WFVOPENCL_DEBUG( outs() << "  global_work_sizes: " << global_work_size[0] << ", " << global_work_size[1] << ", " << global_work_size[2] << "\n"; );
    WFVOPENCL_DEBUG( outs() << "  local_work_sizes: " << local_work_size[0] << ", " << local_work_size[1] << ", " << local_work_size[2] << "\n"; );
    if (global_work_size[0] % local_work_size[0] != 0) return CL_INVALID_WORK_GROUP_SIZE;
    if (global_work_size[1] % local_work_size[1] != 0) return CL_INVALID_WORK_GROUP_SIZE;
    if (global_work_size[2] % local_work_size[2] != 0) return CL_INVALID_WORK_GROUP_SIZE;
    //if (global_work_size[0] > pow(2, sizeof(size_t)) /* oder so :P */) return CL_OUT_OF_RESOURCES;

    typedef void (*kernelFnPtr)(
            const void*,
            const cl_uint,
            const cl_uint*,
            const cl_uint*,
            const cl_int*);
    kernelFnPtr typedPtr = ptr_cast<kernelFnPtr>(kernel->get_compiled_function());

    const void* argument_struct = kernel->get_argument_struct();

    // unfortunately we have to convert to 32bit values because we work with 32bit internally
    const cl_uint modified_global_work_size[3] = { (cl_uint)global_work_size[0], (cl_uint)global_work_size[1], (cl_uint)global_work_size[2] };
    const cl_uint modified_local_work_size[3] = { (cl_uint)local_work_size[0], (cl_uint)local_work_size[1],(cl_uint)local_work_size[2] };

    //
    // execute the kernel
    //
    const cl_uint num_iterations_0 = modified_global_work_size[0] / modified_local_work_size[0]; // = total # threads per block in dim 0
    const cl_uint num_iterations_1 = modified_global_work_size[1] / modified_local_work_size[1]; // = total # threads per block in dim 1
    const cl_uint num_iterations_2 = modified_global_work_size[2] / modified_local_work_size[2]; // = total # threads per block in dim 2
    WFVOPENCL_DEBUG( outs() << "executing kernel (#iterations: " << num_iterations_0 * num_iterations_1 * num_iterations_2 << ")...\n"; );

    assert (num_iterations_0 > 0 && num_iterations_1 > 0 && num_iterations_2 && "should give error message before executeRangeKernel!");

#ifdef WFVOPENCL_USE_OPENMP
    // allocate local memory for each thread to prevent data races
    // TODO: move somewhere else? should all be "static" information!
    const cl_uint numArgs = kernel->get_num_args();
    void* argstructs[WFVOPENCL_MAX_NUM_THREADS];
    void** localdata[WFVOPENCL_MAX_NUM_THREADS]; // too much, but easier to access

    const cl_uint argStrSize = kernel->get_argument_struct_size();
    for (cl_uint j=0; j<WFVOPENCL_MAX_NUM_THREADS; ++j) {
        localdata[j] = (void**)malloc(numArgs*sizeof(void*));
        for (cl_uint i=0; i<numArgs; ++i) {
            if (kernel->arg_is_local(i)) {
                const size_t argSize = kernel->arg_get_size(i);
                // allocate memory for this local pointer (store pointer to be able free later)
                localdata[j][i] = malloc(argSize);
                // store in kernel (overwrite in each thread-iteration)
                void* ldata = kernel->arg_get_data(i);
                *((void**)ldata) = localdata[j][i];
            }
        }
        // now copy entire argument struct with updated local pointers
        argstructs[j] = malloc(argStrSize);
        memcpy(argstructs[j], argument_struct, argStrSize);
    }
#endif

    cl_int i, j, k;

#ifdef WFVOPENCL_USE_OPENMP
    omp_set_num_threads(WFVOPENCL_MAX_NUM_THREADS);
#   ifdef _WIN32
#       pragma omp parallel for shared(argument_struct) private(i, j, k) // VS2010 only supports OpenMP 2.5
#   else
#       pragma omp parallel for shared(argument_struct) private(i, j, k) collapse(3) // collapse requires OpenMP 3.0
#   endif
#endif
    for (i=0; i<num_iterations_0; ++i) {
        for (j=0; j<num_iterations_1; ++j) {
            for (k=0; k<num_iterations_2; ++k) {
                WFVOPENCL_DEBUG_RUNTIME( outs() << "\niteration " << i << "/"  << j << "/" << k << " (= group ids)\n"; );
                WFVOPENCL_DEBUG_RUNTIME( verifyModule(*kernel->get_program()->module); );

                const cl_int group_id[3] = { i, j, k };

                typedPtr(
                    argument_struct,
                    3U, // get_work_dim
                    modified_global_work_size,
                    modified_local_work_size,
                    group_id
                );

                WFVOPENCL_DEBUG_RUNTIME( outs() << "iteration " << i << "/" << j << "/" << k << " finished!\n"; );
                WFVOPENCL_DEBUG_RUNTIME( verifyModule(*kernel->get_program()->module); );
            }
        }
    }

#ifdef WFVOPENCL_USE_OPENMP
    // clean up memory allocated for local data and each thread's argument struct
    for (cl_uint i=0; i<numArgs; ++i) {
        if (kernel->arg_is_local(i)) {
            for (cl_uint j=0; j<WFVOPENCL_MAX_NUM_THREADS; ++j) {
                free(localdata[j][i]);
            }
        }
    }
    for (cl_uint j=0; j<WFVOPENCL_MAX_NUM_THREADS; ++j) {
        free(argstructs[j]);
    }
#endif

    WFVOPENCL_DEBUG( outs() << "execution of kernel finished!\n"; );

    return CL_SUCCESS;
}
inline cl_int executeRangeKernelND(cl_kernel kernel, const cl_uint num_dimensions, const size_t* global_work_sizes, const size_t* local_work_sizes) {
    errs() << "ERROR: clEnqueueNDRangeKernels with work_dim > 3 currently not supported!\n";
    assert (false && "NOT IMPLEMENTED!");
    return CL_INVALID_PROGRAM_EXECUTABLE; // just return something != CL_SUCCESS :P
}

/**
 * from chapter 5.7
 */

// -> compile bitcode of function from .bc file to native code
// -> store void* in _cl_kernel object
WFVOPENCL_DLLEXPORT CL_API_ENTRY cl_kernel CL_API_CALL
clCreateKernel(cl_program      program,
               const char *    kernel_name,
               cl_int *        errcode_ret)
{
    WFVOPENCL_DEBUG ( outs() << "ENTERED clCreateKernel!\n"; );
    if (!program) { *errcode_ret = CL_INVALID_PROGRAM; return NULL; }
    if (!program->module) {
        if (errcode_ret != NULL) {
            *errcode_ret = CL_INVALID_PROGRAM_EXECUTABLE;
        }
        return NULL;
    }
    WFVOPENCL_DEBUG( outs() << "\nclCreateKernel(" << program->module->getModuleIdentifier() << ", " << kernel_name << ")\n"; );

    // does the returned error code mean we should compile before??
    llvm::Module* module = program->module;

    if (!kernel_name) { *errcode_ret = CL_INVALID_VALUE; return NULL; }

    std::stringstream strs;
    strs << "__OpenCL_" << kernel_name << "_kernel";
    const std::string new_kernel_name = strs.str();

    WFVOPENCL_DEBUG( outs() << "new kernel name: " << new_kernel_name << "\n"; );

    llvm::Function* f = WFVOpenCL::getFunction(new_kernel_name, module);
    if (!f) { *errcode_ret = CL_INVALID_KERNEL_NAME; return NULL; }

    WFVOPENCL_DEBUG( WFVOpenCL::writeModuleToFile(module, "debug_kernel_orig_noopt.mod.ll"); );

    // before doing anything, replace function names generated by clc
    WFVOpenCL::fixFunctionNames(module);

    // optimize kernel // TODO: not necessary if we optimize wrapper afterwards
    WFVOpenCL::inlineFunctionCalls(f, program->targetData);
    // Optimize
    // This is essential, we have to get rid of allocas etc.
    // Unfortunately, for packetization enabled, loop rotate has to be disabled (otherwise, Mandelbrot breaks).
#ifdef WFVOPENCL_NO_WFV
    WFVOpenCL::optimizeFunction(f); // enable all optimizations
#else
    WFVOpenCL::optimizeFunction(f, false, true); // enable LICM, disable loop rotate
#endif

    WFVOPENCL_DEBUG( WFVOpenCL::writeFunctionToFile(f, "debug_kernel_orig.ll"); );
    WFVOPENCL_DEBUG( WFVOpenCL::writeModuleToFile(module, "debug_kernel_orig.mod.ll"); );

    LLVMContext& context = module->getContext();

    // determine number of dimensions required by kernel
    const unsigned num_dimensions = WFVOpenCL::determineNumDimensionsUsed(f);


#ifdef WFVOPENCL_NO_WFV

    const int simd_dim = -1;
    llvm::Function* f_wrapper = WFVOpenCL::createKernel(f, kernel_name, num_dimensions, simd_dim, module, program->targetData, context, errcode_ret, NULL);
    if (!f_wrapper) {
        errs() << "ERROR: kernel generation failed!\n";
        return NULL;
    }

    _cl_kernel* kernel = new _cl_kernel(program->context, program, f, f_wrapper);
    kernel->set_num_dimensions(num_dimensions);

#else

    // determine best dimension for packetization
    const int simd_dim = WFVOpenCL::getBestSimdDim(f, num_dimensions);

    llvm::Function* f_SIMD = NULL;
    llvm::Function* f_wrapper = WFVOpenCL::createKernel(f, kernel_name, num_dimensions, simd_dim, module, program->targetData, context, errcode_ret, &f_SIMD);
    if (!f_wrapper) {
        errs() << "ERROR: kernel generation failed!\n";
        return NULL;
    }
    _cl_kernel* kernel;
    if (f_SIMD) {
        // vectorization was successful
        kernel = new _cl_kernel(program->context, program, f, f_wrapper, f_SIMD);
        kernel->set_num_dimensions(num_dimensions);
        kernel->set_best_simd_dim(simd_dim);
    }
    else {
        // vectorization did not work
        kernel = new _cl_kernel(program->context, program, f, f_wrapper);
        kernel->set_num_dimensions(num_dimensions);
    }
    assert(kernel);

#endif

    if (!kernel->get_compiled_function()) { *errcode_ret = CL_INVALID_PROGRAM_EXECUTABLE; return NULL; }

    *errcode_ret = CL_SUCCESS;
    return kernel;
}

WFVOPENCL_DLLEXPORT CL_API_ENTRY cl_int CL_API_CALL
clCreateKernelsInProgram(cl_program     program,
                         cl_uint        num_kernels,
                         cl_kernel *    kernels,
                         cl_uint *      num_kernels_ret)
{
    WFVOPENCL_DEBUG ( outs() << "ENTERED clCreateKernelsInProgram!\n"; );
    assert (false && "NOT IMPLEMENTED!");
    return CL_SUCCESS;
}

WFVOPENCL_DLLEXPORT CL_API_ENTRY cl_int CL_API_CALL
clRetainKernel(cl_kernel    kernel)
{
    WFVOPENCL_DEBUG ( outs() << "ENTERED clRetainKernel!\n"; );
    assert (false && "NOT IMPLEMENTED!");
    return CL_SUCCESS;
}

WFVOPENCL_DLLEXPORT CL_API_ENTRY cl_int CL_API_CALL
clReleaseKernel(cl_kernel   kernel)
{
    WFVOPENCL_DEBUG ( outs() << "ENTERED clReleaseKernel!\n"; );
    _cl_kernel* ptr = (_cl_kernel*)kernel;
    delete ptr;
    return CL_SUCCESS;
}

WFVOPENCL_DLLEXPORT CL_API_ENTRY cl_int CL_API_CALL
clSetKernelArg(cl_kernel    kernel,
               cl_uint      arg_index,
               size_t       arg_size,
               const void * arg_value)
{
    WFVOPENCL_DEBUG ( outs() << "ENTERED clSetKernelArg!\n"; );
    WFVOPENCL_DEBUG( outs() << "\nclSetKernelArg(" << kernel->function_wrapper->getNameStr() << ", " << arg_index << ", " << arg_size << ")\n"; );
    if (!kernel) return CL_INVALID_KERNEL;
    if (arg_index > kernel->get_num_args()) return CL_INVALID_ARG_INDEX;

    kernel->set_arg_data(arg_index, arg_value, arg_size);
    return CL_SUCCESS;
}

WFVOPENCL_DLLEXPORT CL_API_ENTRY cl_int CL_API_CALL
clGetKernelInfo(cl_kernel       kernel,
                cl_kernel_info  param_name,
                size_t          param_value_size,
                void *          param_value,
                size_t *        param_value_size_ret)
{
    WFVOPENCL_DEBUG ( outs() << "ENTERED clGetKernelInfo!\n"; );
    assert (false && "NOT IMPLEMENTED!");
    return CL_SUCCESS;
}

WFVOPENCL_DLLEXPORT CL_API_ENTRY cl_int CL_API_CALL
clGetKernelWorkGroupInfo(cl_kernel                  kernel,
                         cl_device_id               device,
                         cl_kernel_work_group_info  param_name,
                         size_t                     param_value_size,
                         void *                     param_value,
                         size_t *                   param_value_size_ret)
{
    WFVOPENCL_DEBUG ( outs() << "ENTERED clGetKernelWorkGroupInfo!\n"; );
    if (!kernel) return CL_INVALID_KERNEL;
    //if (!device) return CL_INVALID_DEVICE;
    switch (param_name) {
        case CL_KERNEL_WORK_GROUP_SIZE:{
            *(size_t*)param_value = WFVOPENCL_MAX_WORK_GROUP_SIZE; //simdWidth * maxNumThreads;
            break; // type conversion slightly hacked (should use param_value_size) ;)
        }
        case CL_KERNEL_COMPILE_WORK_GROUP_SIZE: {
            assert (false && "NOT IMPLEMENTED");
            break;
        }
        case CL_KERNEL_LOCAL_MEM_SIZE: {
            *(cl_ulong*)param_value = 0; // FIXME ?
            break;
        }
        default: return CL_INVALID_VALUE;
    }
    return CL_SUCCESS;
}

/**
 * 5.8
 */

WFVOPENCL_DLLEXPORT CL_API_ENTRY cl_int CL_API_CALL
clEnqueueNDRangeKernel(cl_command_queue command_queue,
                       cl_kernel        kernel,
                       cl_uint          work_dim,
                       const size_t *   global_work_offset,
                       const size_t *   global_work_size,
                       const size_t *   local_work_size,
                       cl_uint          num_events_in_wait_list,
                       const cl_event * event_wait_list,
                       cl_event *       event)
{
    WFVOPENCL_DEBUG ( outs() << "ENTERED clEnqueueNDRangeKernel!\n"; );
    const unsigned num_dimensions = work_dim; // rename for better understandability ;)
    WFVOPENCL_DEBUG( outs() << "\nclEnqueueNDRangeKernel(" << kernel->function_wrapper->getNameStr() << ")\n"; );
    WFVOPENCL_DEBUG( outs() << "  num_dimensions: " << num_dimensions << "\n"; );
    WFVOPENCL_DEBUG( outs() << "  num_events_in_wait_list: " << num_events_in_wait_list << "\n"; );
    if (!command_queue) return CL_INVALID_COMMAND_QUEUE;
    if (!kernel) return CL_INVALID_KERNEL;
    if (command_queue->context != kernel->get_context()) return CL_INVALID_CONTEXT;
    //if (command_queue->context != event_wait_list->context) return CL_INVALID_CONTEXT;
    if (num_dimensions < 1 || num_dimensions > WFVOPENCL_MAX_NUM_DIMENSIONS) return CL_INVALID_WORK_DIMENSION;
    if (!kernel->get_compiled_function()) return CL_INVALID_PROGRAM_EXECUTABLE; // ?
    if (!global_work_size) return CL_INVALID_GLOBAL_WORK_SIZE;
    if (!local_work_size) return CL_INVALID_WORK_GROUP_SIZE;
    if (global_work_offset) return CL_INVALID_GLOBAL_OFFSET; // see specification p.109
    if (!event_wait_list && num_events_in_wait_list > 0) return CL_INVALID_EVENT_WAIT_LIST;
    if (event_wait_list && num_events_in_wait_list == 0) return CL_INVALID_EVENT_WAIT_LIST;

    if (event) {
        _cl_event* e = new _cl_event();
        e->dispatch = &static_dispatch;
        e->context = ((_cl_kernel*)kernel)->get_context();
        *event = e;
    }

    // compare work_dim and derived dimensions and issue warning/error if not the same
    // (we generate code specific to the number of dimensions actually used)
    // TODO: reject kernel if any of the builtin functions receives variable parameter
    WFVOPENCL_DEBUG(
        if (kernel->get_num_dimensions() != num_dimensions) {
            errs() << "WARNING: number of dimensions used in kernel (" << kernel->get_num_dimensions() <<
                    ") does not match 'work_dim' (" << num_dimensions << ") supplied by clEnqueueNDRangeKernel()!\n";
        }
    );

#ifndef WFVOPENCL_NO_WFV
    WFVOPENCL_DEBUG(
        const size_t simd_dim_work_size = local_work_size[kernel->get_best_simd_dim()];
        outs() << "  best simd dim: " << kernel->get_best_simd_dim() << "\n";
        outs() << "  local_work_size of dim: " << simd_dim_work_size << "\n";
        const bool dividableBySimdWidth = simd_dim_work_size % WFVOPENCL_SIMD_WIDTH == 0;
        if (!dividableBySimdWidth) {
            errs() << "WARNING: group size of simd dimension not dividable by simdWidth\n";
            //return CL_INVALID_WORK_GROUP_SIZE;
        }
    );
#endif

    switch (num_dimensions) {
        case 1: return executeRangeKernel1D(kernel, global_work_size[0], local_work_size[0]);
        case 2: return executeRangeKernel2D(kernel, global_work_size,    local_work_size);
        case 3: return executeRangeKernel3D(kernel, global_work_size,    local_work_size);
        default: return executeRangeKernelND(kernel, num_dimensions, global_work_size, local_work_size);
    }

}

WFVOPENCL_DLLEXPORT CL_API_ENTRY cl_int CL_API_CALL
clEnqueueTask(cl_command_queue  command_queue,
              cl_kernel         kernel,
              cl_uint           num_events_in_wait_list,
              const cl_event *  event_wait_list,
              cl_event *        event)
{
    WFVOPENCL_DEBUG ( outs() << "ENTERED clEnqueueTask!\n"; );
    assert (false && "NOT IMPLEMENTED!");
    return CL_SUCCESS;
}

WFVOPENCL_DLLEXPORT CL_API_ENTRY cl_int CL_API_CALL
clEnqueueNativeKernel(cl_command_queue  command_queue,
                      void (CL_CALLBACK *user_func)(void *),
                      void *            args,
                      size_t            cb_args,
                      cl_uint           num_mem_objects,
                      const cl_mem *    mem_list,
                      const void **     args_mem_loc,
                      cl_uint           num_events_in_wait_list,
                      const cl_event *  event_wait_list,
                      cl_event *        event)
{
    WFVOPENCL_DEBUG ( outs() << "ENTERED clEnqueueNativeKernel!\n"; );
    assert (false && "NOT IMPLEMENTED!");
    return CL_SUCCESS;
}
