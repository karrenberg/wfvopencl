/* ============================================================

Copyright (c) 2009 Advanced Micro Devices, Inc.  All rights reserved.
 
Redistribution and use of this material is permitted under the following 
conditions:
 
Redistributions must retain the above copyright notice and all terms of this 
license.
 
In no event shall anyone redistributing or accessing or using this material 
commence or participate in any arbitration or legal action relating to this 
material against Advanced Micro Devices, Inc. or any copyright holders or 
contributors. The foregoing shall survive any expiration or termination of 
this license or any agreement or access or use related to this material. 

ANY BREACH OF ANY TERM OF THIS LICENSE SHALL RESULT IN THE IMMEDIATE REVOCATION 
OF ALL RIGHTS TO REDISTRIBUTE, ACCESS OR USE THIS MATERIAL.

THIS MATERIAL IS PROVIDED BY ADVANCED MICRO DEVICES, INC. AND ANY COPYRIGHT 
HOLDERS AND CONTRIBUTORS "AS IS" IN ITS CURRENT CONDITION AND WITHOUT ANY 
REPRESENTATIONS, GUARANTEE, OR WARRANTY OF ANY KIND OR IN ANY WAY RELATED TO 
SUPPORT, INDEMNITY, ERROR FREE OR UNINTERRUPTED OPERA TION, OR THAT IT IS FREE 
FROM DEFECTS OR VIRUSES.  ALL OBLIGATIONS ARE HEREBY DISCLAIMED - WHETHER 
EXPRESS, IMPLIED, OR STATUTORY - INCLUDING, BUT NOT LIMITED TO, ANY IMPLIED 
WARRANTIES OF TITLE, MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, 
ACCURACY, COMPLETENESS, OPERABILITY, QUALITY OF SERVICE, OR NON-INFRINGEMENT. 
IN NO EVENT SHALL ADVANCED MICRO DEVICES, INC. OR ANY COPYRIGHT HOLDERS OR 
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, PUNITIVE,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, REVENUE, DATA, OR PROFITS; OR 
BUSINESS INTERRUPTION) HOWEVER CAUSED OR BASED ON ANY THEORY OF LIABILITY 
ARISING IN ANY WAY RELATED TO THIS MATERIAL, EVEN IF ADVISED OF THE POSSIBILITY 
OF SUCH DAMAGE. THE ENTIRE AND AGGREGATE LIABILITY OF ADVANCED MICRO DEVICES, 
INC. AND ANY COPYRIGHT HOLDERS AND CONTRIBUTORS SHALL NOT EXCEED TEN DOLLARS 
(US $10.00). ANYONE REDISTRIBUTING OR ACCESSING OR USING THIS MATERIAL ACCEPTS 
THIS ALLOCATION OF RISK AND AGREES TO RELEASE ADVANCED MICRO DEVICES, INC. AND 
ANY COPYRIGHT HOLDERS AND CONTRIBUTORS FROM ANY AND ALL LIABILITIES, 
OBLIGATIONS, CLAIMS, OR DEMANDS IN EXCESS OF TEN DOLLARS (US $10.00). THE 
FOREGOING ARE ESSENTIAL TERMS OF THIS LICENSE AND, IF ANY OF THESE TERMS ARE 
CONSTRUED AS UNENFORCEABLE, FAIL IN ESSENTIAL PURPOSE, OR BECOME VOID OR 
DETRIMENTAL TO ADVANCED MICRO DEVICES, INC. OR ANY COPYRIGHT HOLDERS OR 
CONTRIBUTORS FOR ANY REASON, THEN ALL RIGHTS TO REDISTRIBUTE, ACCESS OR USE 
THIS MATERIAL SHALL TERMINATE IMMEDIATELY. MOREOVER, THE FOREGOING SHALL 
SURVIVE ANY EXPIRATION OR TERMINATION OF THIS LICENSE OR ANY AGREEMENT OR 
ACCESS OR USE RELATED TO THIS MATERIAL.

NOTICE IS HEREBY PROVIDED, AND BY REDISTRIBUTING OR ACCESSING OR USING THIS 
MATERIAL SUCH NOTICE IS ACKNOWLEDGED, THAT THIS MATERIAL MAY BE SUBJECT TO 
RESTRICTIONS UNDER THE LAWS AND REGULATIONS OF THE UNITED STATES OR OTHER 
COUNTRIES, WHICH INCLUDE BUT ARE NOT LIMITED TO, U.S. EXPORT CONTROL LAWS SUCH 
AS THE EXPORT ADMINISTRATION REGULATIONS AND NATIONAL SECURITY CONTROLS AS 
DEFINED THEREUNDER, AS WELL AS STATE DEPARTMENT CONTROLS UNDER THE U.S. 
MUNITIONS LIST. THIS MATERIAL MAY NOT BE USED, RELEASED, TRANSFERRED, IMPORTED,
EXPORTED AND/OR RE-EXPORTED IN ANY MANNER PROHIBITED UNDER ANY APPLICABLE LAWS, 
INCLUDING U.S. EXPORT CONTROL LAWS REGARDING SPECIFICALLY DESIGNATED PERSONS, 
COUNTRIES AND NATIONALS OF COUNTRIES SUBJECT TO NATIONAL SECURITY CONTROLS. 
MOREOVER, THE FOREGOING SHALL SURVIVE ANY EXPIRATION OR TERMINATION OF ANY 
LICENSE OR AGREEMENT OR ACCESS OR USE RELATED TO THIS MATERIAL.

NOTICE REGARDING THE U.S. GOVERNMENT AND DOD AGENCIES: This material is 
provided with "RESTRICTED RIGHTS" and/or "LIMITED RIGHTS" as applicable to 
computer software and technical data, respectively. Use, duplication, 
distribution or disclosure by the U.S. Government and/or DOD agencies is 
subject to the full extent of restrictions in all applicable regulations, 
including those found at FAR52.227 and DFARS252.227 et seq. and any successor 
regulations thereof. Use of this material by the U.S. Government and/or DOD 
agencies is acknowledgment of the proprietary rights of any copyright holders 
and contributors, including those of Advanced Micro Devices, Inc., as well as 
the provisions of FAR52.227-14 through 23 regarding privately developed and/or 
commercial computer software.

This license forms the entire agreement regarding the subject matter hereof and 
supersedes all proposals and prior discussions and writings between the parties 
with respect thereto. This license does not affect any ownership, rights, title,
or interest in, or relating to, this material. No terms of this license can be 
modified or waived, and no breach of this license can be excused, unless done 
so in a writing signed by all affected parties. Each term of this license is 
separately enforceable. If any term of this license is determined to be or 
becomes unenforceable or illegal, such term shall be reformed to the minimum 
extent necessary in order for this license to remain in effect in accordance 
with its terms as modified by such reformation. This license shall be governed 
by and construed in accordance with the laws of the State of Texas without 
regard to rules on conflicts of law of any state or jurisdiction or the United 
Nations Convention on the International Sale of Goods. All disputes arising out 
of this license shall be subject to the jurisdiction of the federal and state 
courts in Austin, Texas, and all defenses are hereby waived concerning personal 
jurisdiction and venue of these courts.

============================================================ */


#include "BinomialOption.hpp"
#include <cmath>
#include <malloc.h>

int
BinomialOption::setupBinomialOption()
{

    // Make numSamples multiple of 4
    numSamples = (numSamples / 4) * 4;

#if defined (_WIN32)
    randArray = (cl_float*)_aligned_malloc(numSamples * sizeof(cl_float), 16);
    //randArray = (cl_float*)_aligned_malloc(numSamples * sizeof(cl_float4), 16);
#else
    randArray = (cl_float*)memalign(16, numSamples * sizeof(cl_float));
    //randArray = (cl_float*)memalign(16, numSamples * sizeof(cl_float4));
#endif

    if(randArray == NULL)
    { 
        sampleCommon->error("Failed to allocate host memory. (randArray)");
        return SDK_FAILURE;
    }
    
    //for(int i = 0; i < numSamples * 4; i++)
    for(int i = 0; i < numSamples; i++)
    {
        randArray[i] = (float)rand() / (float)RAND_MAX;
    }

#if defined (_WIN32)
    output = (cl_float*)_aligned_malloc(numSamples * sizeof(cl_float), 16);
    //output = (cl_float*)_aligned_malloc(numSamples * sizeof(cl_float4), 16);
#else
    output = (cl_float*)memalign(16, numSamples * sizeof(cl_float));
    //output = (cl_float*)memalign(16, numSamples * sizeof(cl_float4));
#endif

    if(output == NULL)
    {
        sampleCommon->error("Failed to allocate host memory. (output)");
        return SDK_FAILURE;
    }
    memset(output, 0, numSamples * sizeof(cl_float));
    //memset(output, 0, numSamples * sizeof(cl_float4));

    return SDK_SUCCESS;
}

int
BinomialOption::setupCL()
{
    cl_int status = CL_SUCCESS;
    size_t deviceListSize;
    
    cl_device_type dType;
    
    if(deviceType.compare("cpu") == 0)
    {
        dType = CL_DEVICE_TYPE_CPU;
    }
    else //deviceType = "gpu" 
    {
        dType = CL_DEVICE_TYPE_GPU;
    }

    /*
     * Have a look at the available platforms and pick either
     * the AMD one if available or a reasonable default.
     */

    cl_uint numPlatforms;
    cl_platform_id platform = NULL;
    status = clGetPlatformIDs(0, NULL, &numPlatforms);
    if(!sampleCommon->checkVal(status,
                               CL_SUCCESS,
                               "clGetPlatformIDs failed."))
    {
        return SDK_FAILURE;
    }
    if (0 < numPlatforms) 
    {
        cl_platform_id* platforms = new cl_platform_id[numPlatforms];
        status = clGetPlatformIDs(numPlatforms, platforms, NULL);
        if(!sampleCommon->checkVal(status,
                                   CL_SUCCESS,
                                   "clGetPlatformIDs failed."))
        {
            return SDK_FAILURE;
        }
        for (unsigned i = 0; i < numPlatforms; ++i) 
        {
            char pbuf[100];
            status = clGetPlatformInfo(platforms[i],
                                       CL_PLATFORM_VENDOR,
                                       sizeof(pbuf),
                                       pbuf,
                                       NULL);

            if(!sampleCommon->checkVal(status,
                                       CL_SUCCESS,
                                       "clGetPlatformInfo failed."))
            {
                return SDK_FAILURE;
            }

            platform = platforms[i];
            if (!strcmp(pbuf, "Advanced Micro Devices, Inc.")) 
            {
                break;
            }
        }
        delete[] platforms;
    }

    if(NULL == platform)
    {
        sampleCommon->error("NULL platform found so Exiting Application.");
        return SDK_FAILURE;
    }

    /*
     * If we could find our platform, use it. Otherwise use just available platform.
     */
    cl_context_properties cps[3] = 
    {
        CL_CONTEXT_PLATFORM, 
        (cl_context_properties)platform, 
        0
    };

    context = clCreateContextFromType(cps,
                                      dType,
                                      NULL,
                                      NULL,
                                      &status);

    if(!sampleCommon->checkVal(status,
                               CL_SUCCESS,
                               "clCreateContextFromType failed."))
    {
        return SDK_FAILURE;
    }

    /* First, get the size of device list data */
    status = clGetContextInfo(context, 
                              CL_CONTEXT_DEVICES, 
                              0, 
                              NULL, 
                              &deviceListSize);
    if(!sampleCommon->checkVal(status, 
                               CL_SUCCESS,
                               "clGetContextInfo failed."))
    {
        return SDK_FAILURE;
    }

    /* Now allocate memory for device list based on the size we got earlier */
    devices = (cl_device_id *)malloc(deviceListSize);
    if(devices == NULL) 
    {
		sampleCommon->error("Failed to allocate memory (devices).");
		return SDK_FAILURE;
	}

    /* Now, get the device list data */
    status = clGetContextInfo(context, 
                              CL_CONTEXT_DEVICES,
                              deviceListSize,
                              devices,
                              NULL);
    if(!sampleCommon->checkVal(status,
                               CL_SUCCESS, 
                               "clGetContextInfo failed."))
    {
        return SDK_FAILURE;
    }

    /* Create command queue */
    commandQueue = clCreateCommandQueue(context,
                                        devices[0],
                                        0,
                                        &status);

    if(!sampleCommon->checkVal(status,
                               CL_SUCCESS,
                               "clCreateCommandQueue failed."))
    {
        return SDK_FAILURE;
    }

    /* Get Device specific Information */
    status = clGetDeviceInfo(devices[0],
                             CL_DEVICE_MAX_WORK_GROUP_SIZE,
                             sizeof(size_t),
                             (void*)&maxWorkGroupSize,
                             NULL);

    if(!sampleCommon->checkVal(status,
                               CL_SUCCESS, 
                               "clGetDeviceInfo"
                               "CL_DEVICE_MAX_WORK_GROUP_SIZE failed."))
        return SDK_FAILURE;


    status = clGetDeviceInfo(devices[0],
                             CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS,
                             sizeof(cl_uint),
                             (void*)&maxDimensions,
                             NULL);

    if(!sampleCommon->checkVal(status,
                               CL_SUCCESS, 
                               "clGetDeviceInfo"
                               "CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS failed."))
    {
        return SDK_FAILURE;
    }


    maxWorkItemSizes = (size_t*)malloc(maxDimensions * sizeof(size_t));

    status = clGetDeviceInfo(devices[0],
                             CL_DEVICE_MAX_WORK_ITEM_SIZES,
                             sizeof(size_t) * maxDimensions,
                             (void*)maxWorkItemSizes,
                             NULL);

    if(!sampleCommon->checkVal(status,
                               CL_SUCCESS, 
                               "clGetDeviceInfo"
                               "CL_DEVICE_MAX_WORK_ITEM_SIZES failed."))
    {
        return SDK_FAILURE;
    }


    status = clGetDeviceInfo(devices[0],
                             CL_DEVICE_LOCAL_MEM_SIZE,
                             sizeof(cl_ulong),
                             (void*)&totalLocalMemory,
                             NULL);

    if(!sampleCommon->checkVal(status,
                               CL_SUCCESS, 
                               "clGetDeviceInfo"
                               "CL_DEVICE_LOCAL_MEM_SIZE failed."))
    {
        return SDK_FAILURE;
    }

    /**
     * Create and initialize memory objects
     */

    /* Create memory object for stock price */
    randBuffer = clCreateBuffer(context,
                                CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                numSamples * sizeof(cl_float),
                                //numSamples * sizeof(cl_float4),
                                randArray,
                                &status);
    if(!sampleCommon->checkVal(status,
                               CL_SUCCESS,
                               "clCreateBuffer failed. (randBuffer)"))
    {
        return SDK_FAILURE;
    }

    /* Create memory object for output array */
    outBuffer = clCreateBuffer(context,
                               CL_MEM_WRITE_ONLY | CL_MEM_USE_HOST_PTR,
                               numSamples * sizeof(cl_float),
                               //numSamples * sizeof(cl_float4),
                               output,
                               &status);
    if(!sampleCommon->checkVal(status,
                               CL_SUCCESS,
                               "clCreateBuffer failed. (outBuffer)"))
    {
        return SDK_FAILURE;
    }

    /* create a CL program using the kernel source */
#ifdef TESTBENCH_BUILD_FROM_CL
    streamsdk::SDKFile kernelFile;
    std::string kernelPath = sampleCommon->getPath();
    kernelPath.append("BinomialOption_Kernels.cl");
    if(!kernelFile.open(kernelPath.c_str()))
    {
        std::cout << "Failed to load kernel file : " << kernelPath << std::endl;
        return SDK_FAILURE;
    }
    const char* source = kernelFile.source().c_str();
#else
    const char *source = "BinomialOption_Kernels.bc";
#endif
    size_t sourceSize[] = {strlen(source)};

    program = clCreateProgramWithSource(context,
                                        1,
                                        &source,
                                        sourceSize,
                                        &status);
    if(!sampleCommon->checkVal(status,
                               CL_SUCCESS,
                               "clCreateProgramWithSource failed."))
    {
        return SDK_FAILURE;
    }

    /* create a cl program executable for all the devices specified */
    status = clBuildProgram(program,
                            1,
                            devices,
                            NULL,
                            NULL,
                            NULL);
    if(status != CL_SUCCESS)
    {
        if(status == CL_BUILD_PROGRAM_FAILURE)
        {
            cl_int logStatus;
            char * buildLog = NULL;
            size_t buildLogSize = 0;
            logStatus = clGetProgramBuildInfo(program,
                                              devices[0],
                                              CL_PROGRAM_BUILD_LOG,
                                              buildLogSize,
                                              buildLog,
                                              &buildLogSize);
            if(!sampleCommon->checkVal(logStatus,
                                       CL_SUCCESS,
                                       "clGetProgramBuildInfo failed."))
            {
                  return SDK_FAILURE;
            }
            
            buildLog = (char*)malloc(buildLogSize);
            if(buildLog == NULL)
            {
                sampleCommon->error("Failed to allocate host memory. (buildLog)");
                return SDK_FAILURE;
            }
            memset(buildLog, 0, buildLogSize);

            logStatus = clGetProgramBuildInfo(program, 
                                              devices[0], 
                                              CL_PROGRAM_BUILD_LOG, 
                                              buildLogSize, 
                                              buildLog, 
                                              NULL);
            if(!sampleCommon->checkVal(logStatus,
                                       CL_SUCCESS,
                                       "clGetProgramBuildInfo failed."))
            {
                  free(buildLog);
                  return SDK_FAILURE;
            }

            std::cout << " \n\t\t\tBUILD LOG\n";
            std::cout << " ************************************************\n";
            std::cout << buildLog << std::endl;
            std::cout << " ************************************************\n";
            free(buildLog);
        }

          if(!sampleCommon->checkVal(status,
                                     CL_SUCCESS,
                                     "clBuildProgram failed."))
          {
                return SDK_FAILURE;
          }
    }
    /* get a kernel object handle for a kernel with the given name */
    kernel = clCreateKernel(program,
                            "binomial_options",
                            &status);
    if(!sampleCommon->checkVal(status,
                               CL_SUCCESS,
                               "clCreateKernel failed."))
    {
        return SDK_FAILURE;
    }

    /* Get kernel work group size */
    status = clGetKernelWorkGroupInfo(kernel,
                                      devices[0],
                                      CL_KERNEL_WORK_GROUP_SIZE,
                                      sizeof(size_t),
                                      &kernelWorkGroupSize,
                                      0);
    if(!sampleCommon->checkVal(status,
                               CL_SUCCESS, 
                               "clGetKernelWorkGroupInfo failed."))
    {
        return SDK_FAILURE;
    }

    /* If group-size is gerater than maximum supported on kernel */
    if((size_t)(numSteps + 1) > kernelWorkGroupSize)
    {
        if(!quiet)
        {
            std::cout << "Out of Resources!" << std::endl;
            std::cout << "Group Size specified : " << (numSteps + 1) << std::endl;
            std::cout << "Max Group Size supported on the kernel : " 
                      << kernelWorkGroupSize << std::endl;
            std::cout << "Using appropiate group-size." << std::endl;
            std::cout << "-------------------------------------------" << std::endl;
        }
        numSteps = (cl_int)kernelWorkGroupSize - 2;
    }

    return SDK_SUCCESS;
}


int 
BinomialOption::runCLKernels()
{
    cl_int status;

     /*
     * This algorithm reduces each group of work-items to a single value
     * on OpenCL device
     */

    /* Set appropriate arguments to the kernel */

    /* number of steps */
    status = clSetKernelArg(kernel, 0, sizeof(int), (void*)&numSteps);
    if(!sampleCommon->checkVal(status,
                               CL_SUCCESS, 
                               "clSetKernelArg failed. (numSteps)"))
    {
        return SDK_FAILURE;
    }

    /* randBuffer */
    status = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void*)&randBuffer);
    if(!sampleCommon->checkVal(status,
                               CL_SUCCESS, 
                               "clSetKernelArg failed. (randBuffer)"))
    {
        return SDK_FAILURE;
    }

    /* outBuffer */
    status = clSetKernelArg(kernel, 2, sizeof(cl_mem), (void*)&outBuffer);
    if(!sampleCommon->checkVal(status,
                               CL_SUCCESS, 
                               "clSetKernelArg failed. (outBuffer)"))
    {
        return SDK_FAILURE;
    }

    /* local memory callA */
    status = clSetKernelArg(kernel,
                            3,
                            (numSteps + 1) * sizeof(cl_float),
                            //(numSteps + 1) * sizeof(cl_float4),
                            NULL);
    if(!sampleCommon->checkVal(status,
                               CL_SUCCESS, 
                               "clSetKernelArg failed. (callA)"))
    {
        return SDK_FAILURE;
    }

    /* local memory callB */
    status = clSetKernelArg(kernel,
                            4,
                            numSteps * sizeof(cl_float),
                            //numSteps * sizeof(cl_float4),
                            NULL);
    if(!sampleCommon->checkVal(status,
                               CL_SUCCESS, 
                               "clSetKernelArg failed. (callB)"))
    {
        return SDK_FAILURE;
    }

    /**
     * Enqueue a kernel run call.
     */
    size_t globalThreads[] = {numSamples * (numSteps + 1)};
    size_t localThreads[] = {numSteps + 1};

    if(localThreads[0] > maxWorkItemSizes[0] || localThreads[0] > maxWorkGroupSize)
    {
        std::cout << "Unsupported: Device does not support"
            "requested number of work items.";
        return SDK_FAILURE;
    }

    status = clGetKernelWorkGroupInfo(kernel,
                                      devices[0],
                                      CL_KERNEL_LOCAL_MEM_SIZE,
                                      sizeof(cl_ulong),
                                      &usedLocalMemory,
                                      NULL);
    if(!sampleCommon->checkVal(status,
                               CL_SUCCESS, 
                               "clGetKernelWorkGroupInfo"
                               "CL_KERNEL_LOCAL_MEM_SIZE failed."))
    {
        return SDK_FAILURE;
    }

    if(usedLocalMemory > totalLocalMemory)
    {
        std::cout << "Unsupported: Insufficient local memory on device."
            << std::endl;
        return SDK_FAILURE;
    }
    

    status = clEnqueueNDRangeKernel(commandQueue,
                                    kernel,
                                    1,
                                    NULL,
                                    globalThreads,
                                    localThreads,
                                    0,
                                    NULL,
                                    NULL);
    if(!sampleCommon->checkVal(status, 
                               CL_SUCCESS, 
                               "clEnqueueNDRangeKernel failed."))
    {
        return SDK_FAILURE;
    }

    status = clFinish(commandQueue);
    if(!sampleCommon->checkVal(status,
                               CL_SUCCESS, 
                               "clFinish failed."))
    {
        return SDK_FAILURE;
    }
   
    cl_event events[1]; 
    /* Enqueue readBuffer*/
    status = clEnqueueReadBuffer(commandQueue,
                                 outBuffer,
                                 CL_TRUE,
                                 0,
                                 numSamples * sizeof(cl_float),
                                 //numSamples * sizeof(cl_float4),
                                 output,
                                 0,
                                 NULL,
                                 &events[0]);
    if(!sampleCommon->checkVal(status,
                               CL_SUCCESS,
                               "clEnqueueReadBuffer failed."))
    {
        return SDK_FAILURE;
    }
    
    /* Wait for the read buffer to finish execution */
    status = clWaitForEvents(1, &events[0]);
    if(!sampleCommon->checkVal(status,
                               CL_SUCCESS,
                               "clWaitForEvents failed."))
    {
        return SDK_FAILURE;
    }

    clReleaseEvent(events[0]);

    return SDK_SUCCESS;
}

/*
 * Reduces the input array (in place)
 * length specifies the length of the array
 */
int 
BinomialOption::binomialOptionCPUReference()
{
    refOutput = (float*)malloc(numSamples * sizeof(cl_float4));
    if(refOutput == NULL)
	{ 
		sampleCommon->error("Failed to allocate host memory. (refOutput)");
		return SDK_FAILURE;
	}

    float* stepsArray = (float*)malloc((numSteps + 1) * sizeof(cl_float4));
    if(stepsArray == NULL)
    {
        sampleCommon->error("Failed to allocate host memory. (stepsArray)");
        return SDK_FAILURE;
    }

    /* Iterate for all samples */
    for(int bid = 0; bid < numSamples; ++bid)
    {
        float s[4];
        float x[4];
        float vsdt[4];
        float puByr[4];
        float pdByr[4];
        float optionYears[4];

        float inRand[4];


        for(int i = 0; i < 4; ++i)
        {
            inRand[i] = randArray[bid + i];
            s[i] = (1.0f - inRand[i]) * 5.0f + inRand[i] * 30.f;
            x[i] = (1.0f - inRand[i]) * 1.0f + inRand[i] * 100.f;
            optionYears[i] = (1.0f - inRand[i]) * 0.25f + inRand[i] * 10.f; 
            float dt = optionYears[i] * (1.0f / (float)numSteps);
            vsdt[i] = VOLATILITY * sqrtf(dt);
            float rdt = RISKFREE * dt;
            float r = expf(rdt);
            float rInv = 1.0f / r;
            float u = expf(vsdt[i]);
            float d = 1.0f / u;
            float pu = (r - d)/(u - d);
            float pd = 1.0f - pu;
            puByr[i] = pu * rInv;
            pdByr[i] = pd * rInv;
        }
        // Compute values at expiration date:
        // Call option value at period end is v(t) = s(t) - x
        // If s(t) is greater than x, or zero otherwise...
        // The computation is similar for put options...
        for(int j = 0; j <= numSteps; j++)
        {
            for(int i = 0; i < 4; ++i)
            {
                float profit = s[i] * expf(vsdt[i] * (2.0f * j - numSteps)) - x[i];
                stepsArray[j * 4 + i] = profit > 0.0f ? profit : 0.0f;
            }
        }

        //walk backwards up on the binomial tree of depth numSteps
        //Reduce the price step by step
        for(int j = numSteps; j > 0; --j)
        {
            for(int k = 0; k <= j - 1; ++k)
            {
                for(int i = 0; i < 4; ++i)
                {
                    stepsArray[k * 4 + i] = pdByr[i] * stepsArray[(k + 1) * 4 + i] + puByr[i] * stepsArray[k * 4 + i];
                }
            }   
        }

        //Copy the root to result
        refOutput[bid] = stepsArray[0];
    }
    
    free(stepsArray);

    return SDK_SUCCESS;
}

int BinomialOption::initialize()
{
    // Call base class Initialize to get default configuration
    if(!this->SDKSample::initialize())
    {
        return SDK_FAILURE;
    }

    streamsdk::Option* num_samples = new streamsdk::Option;
    if(!num_samples)
    {
        std::cout<<"error. Failed to allocate memory (num_samples)\n";
        return SDK_FAILURE;
    }

    num_samples->_sVersion = "x";
    num_samples->_lVersion = "samples";
    num_samples->_description = "Number of samples to be calculated";
    num_samples->_type = streamsdk::CA_ARG_INT;
    num_samples->_value = &numSamples;

    sampleArgs->AddOption(num_samples);

    delete num_samples;

    streamsdk::Option* num_iterations = new streamsdk::Option;
    if(!num_iterations)
    {
        std::cout << "error. Failed to allocate memory (num_iterations)\n";
        return SDK_FAILURE;
    }

    num_iterations->_sVersion = "i";
    num_iterations->_lVersion = "iterations";
    num_iterations->_description = "Number of iterations for kernel execution";
    num_iterations->_type = streamsdk::CA_ARG_INT;
    num_iterations->_value = &iterations;

    sampleArgs->AddOption(num_iterations);

    delete num_iterations;

    return SDK_SUCCESS;
}

int BinomialOption::setup()
{
    if(setupBinomialOption() != SDK_SUCCESS)
    {
        return SDK_FAILURE;
    }

    int timer = sampleCommon->createTimer();
    sampleCommon->resetTimer(timer);
    sampleCommon->startTimer(timer);

    if(setupCL() != SDK_SUCCESS)
    {
        return SDK_FAILURE;
    }

    sampleCommon->stopTimer(timer);
    /* Compute setup time */
    setupTime = (double)(sampleCommon->readTimer(timer));

    return SDK_SUCCESS;
}


int BinomialOption::run()
{
    int timer = sampleCommon->createTimer();
    sampleCommon->resetTimer(timer);
    sampleCommon->startTimer(timer);   

    std::cout << "Executing kernel for " << iterations << 
        " iterations" << std::endl;
    std::cout << "-------------------------------------------" << std::endl;

    for(int i = 0; i < iterations; i++)
    {
        /* Arguments are set and execution call is enqueued on command buffer */
        if(runCLKernels()!=SDK_SUCCESS)
        {
            return SDK_FAILURE;
        }
    }

    sampleCommon->stopTimer(timer);
    /* Compute average kernel time */
    kernelTime = (double)(sampleCommon->readTimer(timer)) / iterations;

    if(!quiet)
    {
        sampleCommon->printArray<cl_float>("Output", output, numSamples, 1);
    }
 
    return SDK_SUCCESS;
}

int BinomialOption::verifyResults()
{
    if(verify)
    {
        /* reference implementation
         * it overwrites the input array with the output
         */
        int result = SDK_SUCCESS;
        result = binomialOptionCPUReference();
        if(result != SDK_SUCCESS)
            return SDK_FAILURE;

        /* compare the results and see if they match */
        if(sampleCommon->compare(output, refOutput, numSamples, 0.001f))
        {
            std::cout << "Passed!\n";
            return SDK_SUCCESS;
        }
        else
        {
            std::cout <<" Failed\n";
            return SDK_FAILURE;
        }
    }
    return SDK_SUCCESS;
}

void BinomialOption::printStats()
{
    std::string strArray[4] = 
    {
        "Option Samples", 
        "Time(sec)", 
        "KernelTime(sec)" , 
        "Options/sec"
    };

    totalTime = setupTime + kernelTime;

    std::string stats[4];
    stats[0] = sampleCommon->toString(numSamples, std::dec);
    stats[1] = sampleCommon->toString(totalTime, std::dec);
	stats[2] = sampleCommon->toString(kernelTime, std::dec);
    stats[3] = sampleCommon->toString(numSamples/totalTime, std::dec);

    this->SDKSample::printStats(strArray, stats, 4);
}

int
BinomialOption::cleanup()
{
    /* Releases OpenCL resources (Context, Memory etc.) */
    cl_int status;

    status = clReleaseKernel(kernel);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clReleaseKernel failed."))
    {
        return SDK_FAILURE;
    }

    status = clReleaseProgram(program);
    if(!sampleCommon->checkVal(status,
                               CL_SUCCESS,
                               "clReleaseProgram failed."))
    {
        return SDK_FAILURE;
    }

    status = clReleaseMemObject(randBuffer);
    if(!sampleCommon->checkVal(status,
                               CL_SUCCESS,
                               "clReleaseMemObject failed."))
    {
        return SDK_FAILURE;
    }

    status = clReleaseMemObject(outBuffer);
    if(!sampleCommon->checkVal(status,
                               CL_SUCCESS,
                               "clReleaseMemObject failed."))
    {
        return SDK_FAILURE;
    }

    status = clReleaseCommandQueue(commandQueue);
    if(!sampleCommon->checkVal(status,
                               CL_SUCCESS,
                               "clReleaseCommandQueue failed."))
    {
        return SDK_FAILURE;
    }

    status = clReleaseContext(context);
    if(!sampleCommon->checkVal(status,
                               CL_SUCCESS,
                               "clReleaseContext failed."))
    {
        return SDK_FAILURE;
    }

    return SDK_SUCCESS;
}

BinomialOption::~BinomialOption()
{
     if(randArray) 
     {
         #ifdef _WIN32
             _aligned_free(randArray);
         #else
             free(randArray);
         #endif
         randArray = NULL;
     }

     if(output) 
     {
         #ifdef _WIN32
             _aligned_free(output);
         #else
             free(output);
         #endif
         output = NULL;
     }

     if(refOutput)
     {
         free(refOutput);
         refOutput = NULL;
     }

     if(maxWorkItemSizes)
     {
         free(maxWorkItemSizes);
         maxWorkItemSizes = NULL;
     }

     if(devices)
     {
         free(devices);
         devices = NULL;
     }
}

int 
main(int argc, char * argv[])
{
    BinomialOption clBinomialOption("OpenCL BinomialOption");

    clBinomialOption.initialize();
    if(!clBinomialOption.parseCommandLine(argc, argv))
        return SDK_FAILURE;
    if(clBinomialOption.setup()!=SDK_SUCCESS)
        return SDK_FAILURE;
    if(clBinomialOption.run()!=SDK_SUCCESS)
        return SDK_FAILURE;
    if(clBinomialOption.verifyResults()!=SDK_SUCCESS)
        return SDK_FAILURE;
    if(clBinomialOption.cleanup()!=SDK_SUCCESS)
        return SDK_FAILURE;
    clBinomialOption.printStats();

	return SDK_SUCCESS;
}



