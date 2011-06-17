// Copyright (c) 2009-2011 Intel Corporation
// All rights reserved.
// 
// WARRANTY DISCLAIMER
// 
// THESE MATERIALS ARE PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL INTEL OR ITS
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
// OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THESE
// MATERIALS, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// 
// Intel Corporation is the author of the Materials, and requests that all
// problem reports or change requests be submitted to it directly
//
// DotProduct.cpp : Defines the entry point for the console application.
//
// This sampple demonstrates how a kernel can be written that operates on individual data elements
// of a memory object which is very similar to what is allowed in GLSL / HLSL. In this example
// we show a function that computes a dot product of two float4 arrays and writes result to a float
// array.
// The OpenCL kernel source is:
//
//__kernel void dot_product (__global const float4 *a, __global const float4 *b, __global float *c)
//{
//	int gid = get_global_id(0);
//	c[gid] = dot(a[gid], b[gid]);
//}
//
//The following code describes the OpenCL runtime calls the application must make to create
//appropriate memory objects, create program object and load the program source for the kernel
//described above, build the program executable, create the kernel object, load the appropriate
//argument values and execute the dot product kernel on the CPU device.

#include "stdafx.h"
#include "CL\cl.h"
#include "utils.h"


#define BUFFER_LENGTH	1024


// release OpenCL memory objects points by memobjs. the number of the objects to be released
// defined by the n argument
void delete_memobjs(cl_mem *memobjs, int n)
{
    int i;
    for (i=0; i<n; i++)
    {
        clReleaseMemObject(memobjs[i]);
    }
}

// main execution routine - perform dot-product on float4 vectors
int exec_dot_product_kernel(const char *program_source, int n, void *srcA, void *srcB, void *dst)
{
    cl_context context;
    cl_command_queue cmd_queue;
    cl_device_id *devices;
    cl_program program;
    cl_kernel kernel;
    cl_mem memobjs[3];
    size_t global_work_size[1];
    size_t local_work_size[1];
    size_t cb;
    cl_uint size_ret = 0;
    cl_int err;

    printf("Trying to run on Intel CPU \n");
    cl_platform_id intel_platform_id = GetIntelOCLPlatform();
    if( intel_platform_id == NULL )
    {
        printf("ERROR: Failed to find Intel OpenCL platform.\n");
        return -1;
    }

    cl_context_properties context_properties[3] = {CL_CONTEXT_PLATFORM, (cl_context_properties)intel_platform_id, NULL };

    // create the OpenCL context on a CPU 
    context = clCreateContextFromType(context_properties, CL_DEVICE_TYPE_CPU, NULL, NULL, NULL);
    if (context == (cl_context)0)
        return -1;

    // get the list of CPU devices associated with context
    err = clGetContextInfo(context, CL_CONTEXT_DEVICES, 0, NULL, &cb);

    devices = (cl_device_id*)malloc(cb);

    clGetContextInfo(context, CL_CONTEXT_DEVICES, cb, devices, NULL);

    // create a command-queue
    cmd_queue = clCreateCommandQueue(context, devices[0], 0, NULL);

    if (cmd_queue == (cl_command_queue)0)
    {
        clReleaseContext(context);
        free(devices);
        return -1;
    }
    free(devices);

    // allocate the buffer memory objects
    memobjs[0] = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(cl_float4) * n, srcA, NULL);

    if (memobjs[0] == (cl_mem)0)
    {
        printf("ERROR: Failed to create Buffer...\n");
        clReleaseCommandQueue(cmd_queue);
        clReleaseContext(context);
        return -1;
    }

    memobjs[1] = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(cl_float4) * n, srcB, NULL);

    if (memobjs[1] == (cl_mem)0)
    {
        printf("ERROR: Failed to create Buffer...\n");
        delete_memobjs(memobjs, 1);
        clReleaseCommandQueue(cmd_queue);
        clReleaseContext(context);
        return -1;
    }

    memobjs[2] = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(cl_float) * n, NULL, NULL);

    if (memobjs[2] == (cl_mem)0)
    {
        printf("ERROR: Failed to create Buffer...\n");
        delete_memobjs(memobjs, 2);
        clReleaseCommandQueue(cmd_queue);
        clReleaseContext(context);
        return -1;
    }

    // create the program
    program = clCreateProgramWithSource(context, 1, (const char**)&program_source, NULL, NULL);
    if (program == (cl_program)0)
    {
        printf("ERROR: Failed to create Program with source...\n");
        delete_memobjs(memobjs, 3);
        clReleaseCommandQueue(cmd_queue);
        clReleaseContext(context);
        return -1;
    }

    // build the program
    err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
    if (err != CL_SUCCESS)
    {
        printf("ERROR: Failed to build program...\n");
        delete_memobjs(memobjs, 3);
        clReleaseProgram(program);
        clReleaseCommandQueue(cmd_queue);
        clReleaseContext(context);
        return -1;
    }

    // create the kernel
    kernel = clCreateKernel(program, "dot_product", NULL);
    if (kernel == (cl_kernel)0)
    {
        printf("ERROR: Failed to create kernel...\n");
        delete_memobjs(memobjs, 3);
        clReleaseProgram(program);
        clReleaseCommandQueue(cmd_queue);
        clReleaseContext(context);
        return -1;
    }

    // set the args values
    err = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *) &memobjs[0]);

    err |= clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *) &memobjs[1]);

    err |= clSetKernelArg(kernel, 2,sizeof(cl_mem), (void *) &memobjs[2]);

    if (err != CL_SUCCESS)
    {
        printf("ERROR: Failed to set kernel arguments...\n");
        delete_memobjs(memobjs, 3);
        clReleaseKernel(kernel);
        clReleaseProgram(program);
        clReleaseCommandQueue(cmd_queue);
        clReleaseContext(context);
        return -1;
    }
    // set work-item dimensions
    global_work_size[0] = n;
    local_work_size[0]= 1;

    // execute kernel
    err = clEnqueueNDRangeKernel(cmd_queue, kernel, 1, NULL, global_work_size, local_work_size, 0, NULL, NULL);
    if (err != CL_SUCCESS)
    {
        printf("ERROR: Failed to enqueue NDRange kernel...\n");
        delete_memobjs(memobjs, 3);
        clReleaseKernel(kernel);
        clReleaseProgram(program);
        clReleaseCommandQueue(cmd_queue);
        clReleaseContext(context);
        return -1;
    }

    // read output image
    err = clEnqueueReadBuffer(cmd_queue, memobjs[2], CL_TRUE, 0, n * sizeof(cl_float), dst, 0, NULL, NULL);
    if (err != CL_SUCCESS)
    {
        printf("ERROR: Failed to enqueue read buffer command...\n");
        delete_memobjs(memobjs, 3);
        clReleaseKernel(kernel);
        clReleaseProgram(program);
        clReleaseCommandQueue(cmd_queue);
        clReleaseContext(context);
        return -1;
    }

    //release kernel, program, and memory objects
    delete_memobjs(memobjs, 3);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(cmd_queue);
    clReleaseContext(context);

    printf("Dot Product Succeeded!\n");

    return 0; // success...
}


int _tmain(int argc, _TCHAR* argv[])
{
    const char *ocl_test_program[] = {\
        "__kernel void dot_product (__global const float4 *a, __global const float4 *b, __global float *c)"\
        "{"\
        "int tid = get_global_id(0);"\
        "c[tid] = dot(a[tid], b[tid]);"\
        "}"
    };


    cl_float * srcA, * srcB, * dst;

    srcA = new cl_float[BUFFER_LENGTH];
    if (NULL == srcA)
    {
        return -1;
    }
    srcB = new cl_float[BUFFER_LENGTH];
    if (NULL == srcB)
    {
        delete[] srcA;
        return -1;
    }
    dst = new cl_float[BUFFER_LENGTH];
    if (NULL == dst)
    {
        delete[] srcA;
        delete[] srcB;
        return -1;
    }

    for(int j = 0; j < BUFFER_LENGTH; j++)
    {
        srcA[j] = (cl_float)j;
        srcB[j] = 1;
        dst[j] = 0;
    }

    int err = exec_dot_product_kernel(ocl_test_program[0], BUFFER_LENGTH/4, srcA, srcB, dst);

    delete[] srcA;
    delete[] srcB;
    delete[] dst;

    return err;
}

