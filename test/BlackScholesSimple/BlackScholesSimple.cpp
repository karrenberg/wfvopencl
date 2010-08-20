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


#include "BlackScholesSimple.hpp"

#include <math.h>
#include <malloc.h>


/**
 *  Constants 
 */

#define S_LOWER_LIMIT 10.0f
#define S_UPPER_LIMIT 100.0f
#define K_LOWER_LIMIT 10.0f
#define K_UPPER_LIMIT 100.0f
#define T_LOWER_LIMIT 1.0f
#define T_UPPER_LIMIT 10.0f
#define R_LOWER_LIMIT 0.01f
#define R_UPPER_LIMIT 0.05f
#define SIGMA_LOWER_LIMIT 0.01f
#define SIGMA_UPPER_LIMIT 0.10f

float 
BlackScholes::phi(float X)
{
    float y, absX, t;

    // the coeffs
    const float c1 =  0.319381530f;
    const float c2 = -0.356563782f;
    const float c3 =  1.781477937f;
    const float c4 = -1.821255978f;
    const float c5 =  1.330274429f;

    const float oneBySqrt2pi = 0.398942280f;

    absX = fabs(X);
    t = 1.0f / (1.0f + 0.2316419f * absX);

    y = 1.0f - oneBySqrt2pi * exp(-X * X / 2.0f) *
        t * (c1 +
                t * (c2 +
                    t * (c3 +
                        t * (c4 + t * c5))));

    return (X < 0) ? (1.0f - y) : y;
}

void 
BlackScholes::blackScholesCPU()
{
    int y;
    for (y = 0; y < width * height; ++y)
    {
        float d1, d2;
        float sigmaSqrtT;
        float KexpMinusRT;
        float s = S_LOWER_LIMIT * randArray[y] + S_UPPER_LIMIT * (1.0f - randArray[y]);
        float k = K_LOWER_LIMIT * randArray[y] + K_UPPER_LIMIT * (1.0f - randArray[y]);
        float t = T_LOWER_LIMIT * randArray[y] + T_UPPER_LIMIT * (1.0f - randArray[y]);
        float r = R_LOWER_LIMIT * randArray[y] + R_UPPER_LIMIT * (1.0f - randArray[y]);
        float sigma = SIGMA_LOWER_LIMIT * randArray[y] + SIGMA_UPPER_LIMIT * (1.0f - randArray[y]);

        sigmaSqrtT = sigma * sqrt(t);

        d1 = (log(s / k) + (r + sigma * sigma / 2.0f) * t) / sigmaSqrtT;
        d2 = d1 - sigmaSqrtT;

        KexpMinusRT = k * exp(-r * t);
        hostCallPrice[y] = s * phi(d1) - KexpMinusRT * phi(d2);
        hostPutPrice[y]  = KexpMinusRT * phi(-d2) - s * phi(-d1);
    }
}



int 
BlackScholes::setupBlackScholes()
{
    int i = 0;

    /* Calculate width and height from samples */
    if(samples < 16)
    {
        width = 2;
        height = 2;
    }
    else
    {
        samples = samples / 4;
        unsigned int tempVar1 = (unsigned int)sqrt((double)samples);
        width = tempVar1 - (tempVar1 % blockSizeX);
        height = width;
    }


#if defined (_WIN32)
    randArray = (cl_float*)_aligned_malloc(width * height * sizeof(cl_float), 16);
#else
    randArray = (cl_float*)memalign(16, width * height * sizeof(cl_float));
#endif

    if(randArray == NULL)
    {
        sampleCommon->error("Failed to allocate host memory. (randArray)");
        return SDK_FAILURE;
    }
    for(i = 0; i < width * height; i++)
    {
        randArray[i] = (float)rand() / (float)RAND_MAX;
    }

    deviceCallPrice = (cl_float*)malloc(width * height * sizeof(cl_float));
    if(deviceCallPrice == NULL)
    { 
        sampleCommon->error("Failed to allocate host memory. (deviceCallPrice)");
        return SDK_FAILURE;
    }
    memset(deviceCallPrice, 0, width * height * sizeof(cl_float));

    devicePutPrice = (cl_float*)malloc(width * height * sizeof(cl_float));
    if(devicePutPrice == NULL)
    { 
        sampleCommon->error("Failed to allocate host memory. (devicePutPrice)");
        return SDK_FAILURE;
    }
    memset(devicePutPrice, 0, width * height * sizeof(cl_float));

    hostCallPrice = (cl_float*)malloc(width * height * sizeof(cl_float));
    if(hostCallPrice == NULL)
    {
        sampleCommon->error("Failed to allocate host memory. (hostCallPrice)");
        return SDK_FAILURE;
    }
    memset(hostCallPrice, 0, width * height * sizeof(cl_float));

    hostPutPrice = (cl_float*)malloc(width * height * sizeof(cl_float));
    if(hostPutPrice == NULL)
    { 
        sampleCommon->error("Failed to allocate host memory. (hostPutPrice)");
        return SDK_FAILURE;
    }
    memset(hostPutPrice, 0, width * height * sizeof(cl_float));

    return SDK_SUCCESS;
}

int
BlackScholes::setupCL(void)
{
    cl_int status = 0;
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
    devices = (cl_device_id*)malloc(deviceListSize);
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

    {
        /* The block is to move the declaration of prop closer to its use */
        cl_command_queue_properties prop = 0;

        if(timing)
        {
            prop |= CL_QUEUE_PROFILING_ENABLE;
        }

        commandQueue = clCreateCommandQueue(context, 
                                            devices[0], 
                                            prop, 
                                            &status);
        if(!sampleCommon->checkVal(status,
                                   0,
                                   "clCreateCommandQueue failed."))
        {
            return SDK_FAILURE;
        }
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
    {
        return SDK_FAILURE;
    }


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

    randBuf = clCreateBuffer(context, 
                             CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
                             sizeof(cl_float) * width  * height,
                             randArray, 
                             &status);
    if(!sampleCommon->checkVal(status,
                               CL_SUCCESS,
                               "clCreateBuffer failed. (randBuf)"))
    {
        return SDK_FAILURE;
    }

    callPriceBuf = clCreateBuffer(context, 
                                  CL_MEM_WRITE_ONLY,
                                  sizeof(cl_float) * width * height,
                                  NULL, 
                                  &status);
    if(!sampleCommon->checkVal(status,
                               CL_SUCCESS,
                               "clCreateBuffer failed. (callPriceBuf)"))
    {
        return SDK_FAILURE;
    }

    putPriceBuf = clCreateBuffer(context, 
                                 CL_MEM_WRITE_ONLY,
                                 sizeof(cl_float) * width * height,
                                 NULL, 
                                 &status);
    if(!sampleCommon->checkVal(status,
                               CL_SUCCESS,
                               "clCreateBuffer failed. (putPriceBuf)"))
    {
        return SDK_FAILURE;
    }

    /* create a CL program using the kernel source */
    //streamsdk::SDKFile kernelFile;
    //std::string kernelPath = sampleCommon->getPath();
    //kernelPath.append("BlackScholes_Kernels.cl");
    //if(!kernelFile.open(kernelPath.c_str()))
    //{
        //std::cout << "Failed to load kernel file : " << kernelPath << std::endl;
        //return SDK_FAILURE;
    //}
    //const char* source = kernelFile.source().c_str();
    const char* source = "BlackScholesSimple_Kernels.bc";
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
    status = clBuildProgram(program, 1, devices, NULL, NULL, NULL);
    if(status != CL_SUCCESS)
    {
        if(status == CL_BUILD_PROGRAM_FAILURE)
        {
            cl_int logStatus;
            char *buildLog = NULL;
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
    kernel = clCreateKernel(program, "blackScholes", &status);
    if(!sampleCommon->checkVal(status,
                               CL_SUCCESS,
                               "clCreateKernel failed."))
    {
        return SDK_FAILURE;
    }


    /* Check if blockSize exceeds group-size returned by kernel */
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

    if((blockSizeX * blockSizeY) > kernelWorkGroupSize)
    {
        if(!quiet)
        {
            std::cout << "Out of Resources!" << std::endl;
            std::cout << "Group Size specified : "
                      <<  blockSizeX * blockSizeY << std::endl;
            std::cout << "Max Group Size supported on the kernel : " 
                      << kernelWorkGroupSize << std::endl;
            std::cout << "Falling back to " << kernelWorkGroupSize << std::endl;
            std::cout << "------------------------------------------" << std::endl;
        }
        if(blockSizeX > kernelWorkGroupSize)
        {
            blockSizeX = kernelWorkGroupSize;
            blockSizeY = 1;
        }
    }

    return SDK_SUCCESS;
}


int 
BlackScholes::runCLKernels(void)
{
    cl_int   status;

    //size_t globalThreads[2] = {width, height};
    //size_t localThreads[2] = {blockSizeX, blockSizeY};
    size_t globalThreads[1] = {width * height};
    size_t localThreads[1] = {blockSizeX * blockSizeY};

    if(localThreads[0] > maxWorkItemSizes[0] ||
       //localThreads[1] > maxWorkItemSizes[1] ||
       (size_t)blockSizeX * blockSizeY > maxWorkGroupSize)
    {
        std::cout << "Unsupported: Device does not support" 
            "requested number of work items.";
        free(maxWorkItemSizes);
        return SDK_FAILURE;
    }
    
    /* whether sort is to be in increasing order. CL_TRUE implies increasing */
    status = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&randBuf); 
    if(!sampleCommon->checkVal(status,
                               CL_SUCCESS,
                               "clSetKernelArg failed. (randBuf)"))
    {
        return SDK_FAILURE;
    }

    status = clSetKernelArg(kernel, 1, sizeof(width), (const void *)&width);
    if(!sampleCommon->checkVal(status,
                               CL_SUCCESS,
                               "clSetKernelArg failed. (width)"))
    {
        return SDK_FAILURE;
    }

    status = clSetKernelArg(kernel, 2, sizeof(cl_mem), (void *)&callPriceBuf);
    if(!sampleCommon->checkVal(
            status,
            CL_SUCCESS,
            "clSetKernelArg failed. (callPriceBuf)"))
        return SDK_FAILURE;

    status = clSetKernelArg(kernel, 3, sizeof(cl_mem), (void *)&putPriceBuf);
    if(!sampleCommon->checkVal(status,
                               CL_SUCCESS,
                               "clSetKernelArg failed. (putPriceBuf)"))
    {
        return SDK_FAILURE;
    }

    /* 
     * Enqueue a kernel run call.
     */
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


    /* wait for the kernel call to finish execution */
    status = clFinish(commandQueue);
    if(!sampleCommon->checkVal(status,
                               CL_SUCCESS, 
                               "clFinish failed."))
    {
        return SDK_FAILURE;
    }

    /* Enqueue the results to application pointer*/
    status = clEnqueueReadBuffer(commandQueue, 
                                 callPriceBuf, 
                                 CL_TRUE,
                                 0,
                                 width * height * sizeof(cl_float),
                                 deviceCallPrice,
                                 0,
                                 NULL,
                                 NULL);
    if(!sampleCommon->checkVal(status,
                               CL_SUCCESS,
                               "clEnqueueReadBuffer failed."))
    {
        return SDK_FAILURE;
    }
    
    /* wait for the read buffer to finish execution */
    status = clFinish(commandQueue);
    if(!sampleCommon->checkVal(status,
                               CL_SUCCESS, 
                               "clFinish failed."))
    {
        return SDK_FAILURE;
    }


    /* Enqueue the results to application pointer*/
    status = clEnqueueReadBuffer(commandQueue, 
                                 putPriceBuf, 
                                 CL_TRUE,
                                 0,
                                 width * height * sizeof(cl_float),
                                 devicePutPrice,
                                 0,
                                 NULL,
                                 NULL);
    if(!sampleCommon->checkVal(status,
                               CL_SUCCESS,
                               "clEnqueueReadBuffer failed."))
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

	return SDK_SUCCESS;
}

int 
BlackScholes::initialize()
{
    // Call base class Initialize to get default configuration
    if(!this->SDKSample::initialize())
        return SDK_FAILURE;

    streamsdk::Option* num_samples = new streamsdk::Option;
    if(!num_samples)
    {
        std::cout << "error. Failed to allocate memory (num_samples)\n";
        return SDK_FAILURE;
    }

    num_samples->_sVersion = "x";
    num_samples->_lVersion = "samples";
    num_samples->_description = "Number of samples to be calculated";
    num_samples->_type = streamsdk::CA_ARG_INT;
    num_samples->_value = &samples;

    sampleArgs->AddOption(num_samples);

    num_samples->_sVersion = "i";
    num_samples->_lVersion = "iterations";
    num_samples->_description = "Number of iterations";
    num_samples->_type = streamsdk::CA_ARG_INT;
    num_samples->_value = &iterations;
    
    sampleArgs->AddOption(num_samples);

    delete num_samples;

    return SDK_SUCCESS;
}

int 
BlackScholes::setup()
{
    if(setupBlackScholes() != SDK_SUCCESS)
        return SDK_FAILURE;

    int timer = sampleCommon->createTimer();
    sampleCommon->resetTimer(timer);
    sampleCommon->startTimer(timer);

    if(setupCL() != SDK_SUCCESS)
        return SDK_FAILURE;

    sampleCommon->stopTimer(timer);
    /* Compute setup time */
    setupTime = (double)(sampleCommon->readTimer(timer));

	return SDK_SUCCESS;
}


int
BlackScholes::run()
{
    int timer = sampleCommon->createTimer();
    sampleCommon->resetTimer(timer);
    sampleCommon->startTimer(timer);

    std::cout << "Executing kernel for " << iterations 
        << " iterations" << std::endl;
    std::cout <<"-------------------------------------------" << std::endl;

    for(int i = 0; i < iterations; i++)
    {
        /* Arguments are set and execution call is enqueued on command buffer */
        if(runCLKernels()!=SDK_SUCCESS)
            return SDK_FAILURE;
    }

    sampleCommon->stopTimer(timer);
    /* Compute kernel time */
    kernelTime = (double)(sampleCommon->readTimer(timer)) / iterations;

    if(!quiet)
    {
        sampleCommon->printArray<cl_float>("deviceCallPrice",
                                           deviceCallPrice, 
                                           width, 
                                           1);

        sampleCommon->printArray<cl_float>("devicePutPrice", 
                                           devicePutPrice, 
                                           width, 
                                           1);
    }

    return SDK_SUCCESS;
}

int 
BlackScholes::verifyResults()
{
    if(verify)
    {
        /* reference implementation
         * it overwrites the input array with the output
         */
        blackScholesCPU();

        if(!quiet)
        {
            sampleCommon->printArray<cl_float>("hostCallPrice",
                                               hostCallPrice, 
                                               width, 
                                               1);

            sampleCommon->printArray<cl_float>("hostPutPrice", 
                                               hostPutPrice, 
                                               width, 
                                               1);
        }
        /* compare the call price results and see if they match */
        if(!sampleCommon->compare(hostCallPrice, deviceCallPrice, width * height))
        {
            std::cout << "Failed\n";
            return SDK_FAILURE;
        }
        else
        {
            std::cout << "Passed!\n";
            return SDK_SUCCESS;
        }

        /* compare the results and see if they match */
        if(!sampleCommon->compare(hostPutPrice, devicePutPrice, width * height))
        {
            std::cout << "Failed\n";
            return SDK_FAILURE;
        }
        else
        {
            std::cout << "Passed!\n";
            return SDK_SUCCESS;
        }
    }

    return SDK_SUCCESS;
}

void 
BlackScholes::printStats()
{
    int actualSamples = width * height;
    totalTime = setupTime + kernelTime;

    std::string strArray[4] = 
    {
        "Option Samples", 
        "Time(sec)", 
        "KernelTime(sec)", 
        "Options/sec"
    };

    std::string stats[4];
    stats[0] = sampleCommon->toString(actualSamples, std::dec);
    stats[1] = sampleCommon->toString(totalTime, std::dec);
    stats[2] = sampleCommon->toString(kernelTime, std::dec);
    stats[3] = sampleCommon->toString(actualSamples / totalTime, std::dec);

    this->SDKSample::printStats(strArray, stats, 4);

}


int BlackScholes::cleanup()
{
    /* Releases OpenCL resources (Context, Memory etc.) */
    cl_int status;
    status = clReleaseMemObject(randBuf);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clReleaseMemObject failed."))
        return SDK_FAILURE;

    status = clReleaseMemObject(callPriceBuf);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clReleaseMemObject failed."))
        return SDK_FAILURE;

    status = clReleaseMemObject(putPriceBuf);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clReleaseMemObject failed."))
        return SDK_FAILURE;

    status = clReleaseKernel(kernel);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clReleaseKernel failed."))
        return SDK_FAILURE;

    status = clReleaseProgram(program);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clReleaseProgram failed."))
        return SDK_FAILURE;

    status = clReleaseCommandQueue(commandQueue);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clReleaseCommandQueue failed."))
        return SDK_FAILURE;

    status = clReleaseContext(context);
    if(!sampleCommon->checkVal(
            status,
            CL_SUCCESS,
            "clReleaseContext failed."))
        return SDK_FAILURE;

    /* Release program resources (input memory etc.) */

     if(randArray) 
     {
         #ifdef _WIN32
             _aligned_free(randArray);
         #else
             free(randArray);
         #endif
         randArray = NULL;
     }

    if(deviceCallPrice)
    {
        free(deviceCallPrice);
        deviceCallPrice = NULL;
    }

    if(devicePutPrice) 
    {
        free(devicePutPrice);
        devicePutPrice = NULL;
    }

    if(hostCallPrice)
    {
        free(hostCallPrice);
        hostCallPrice = NULL;
    }

    if(hostPutPrice) 
    {
        free(hostPutPrice);
        hostPutPrice = NULL;
    }

    if(devices)
    {
        free(devices);
        devices = NULL;
    }

     if(maxWorkItemSizes)
     {
         free(maxWorkItemSizes);
         maxWorkItemSizes = NULL;
     }

    return SDK_SUCCESS;
}

int 
main(int argc, char * argv[])
{
    /* Create MonteCalroAsian object */
    BlackScholes clBlackScholes("Black Scholes OpenCL sample");
    
    /* Initialization */
    if(clBlackScholes.initialize()!=SDK_SUCCESS)
        return SDK_FAILURE;

    /* Parse command line options */
    if(!clBlackScholes.parseCommandLine(argc, argv))
        return SDK_FAILURE;

    /* Setup */
    if(clBlackScholes.setup()!=SDK_SUCCESS)
        return SDK_FAILURE;

    /* Run */
    if(clBlackScholes.run()!=SDK_SUCCESS)
        return SDK_FAILURE;

    /* Verifty */
    if(clBlackScholes.verifyResults()!=SDK_SUCCESS)
        return SDK_FAILURE;

    /* Cleanup resources created */
    if(clBlackScholes.cleanup()!=SDK_SUCCESS)
        return SDK_FAILURE;

    /* Print performance statistics */
    clBlackScholes.printStats();

    return SDK_SUCCESS;
}
