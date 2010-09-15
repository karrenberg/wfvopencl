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


#include "SHA1.hpp"

int 
SHA1::setupSHA1()
{
    cl_uint sizeBytes;

    /* allocate and init memory used by host */
    sizeBytes = width * height * sizeof(cl_int);
    output = (cl_int *) malloc(sizeBytes);
    if(output==NULL)    
    { 
        sampleCommon->error("Failed to allocate host memory. (output)");
        return SDK_FAILURE;
    }

    if(verify)
    {
        verificationOutput = (cl_int *)malloc(sizeBytes);
        if(verificationOutput==NULL)    
        { 
            sampleCommon->error("Failed to allocate host memory. (verificationOutput)");
            return SDK_FAILURE;
        }

    }
    return SDK_SUCCESS;
}

int
SHA1::setupCL(void)
{
    cl_int status = 0;
    size_t deviceListSize;

    cl_device_type dType;
    
    //if(deviceType.compare("cpu") == 0)
    //{
        dType = CL_DEVICE_TYPE_CPU;
    //}
    //else //deviceType = "gpu"
    //{
        //dType = CL_DEVICE_TYPE_GPU;
    //}
    
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

    /*
     * If we could find our platform, use it. Otherwise pass a NULL and get whatever the
     * implementation thinks we should be using.
     */

    cl_context_properties cps[3] = 
    {
        CL_CONTEXT_PLATFORM, 
        (cl_context_properties)platform, 
        0
    };
    /* Use NULL for backward compatibility */
    cl_context_properties* cprops = (NULL == platform) ? NULL : cps;

    context = clCreateContextFromType(
                  cprops,
                  dType,
                  NULL,
                  NULL,
                  &status);

    if(!sampleCommon->checkVal(status, 
                CL_SUCCESS,
                "clCreateContextFromType failed."))
        return SDK_FAILURE;

    /* First, get the size of device list data */
    status = clGetContextInfo(
            context, 
            CL_CONTEXT_DEVICES, 
            0, 
            NULL, 
            &deviceListSize);
    if(!sampleCommon->checkVal(
                status, 
                CL_SUCCESS,
                "clGetContextInfo failed."))
        return SDK_FAILURE;

    /* Now allocate memory for device list based on the size we got earlier */
    devices = (cl_device_id *)malloc(deviceListSize);
    if(devices==NULL) {
        sampleCommon->error("Failed to allocate memory (devices).");
        return SDK_FAILURE;
    }

    /* Now, get the device list data */
    status = clGetContextInfo(
            context, 
            CL_CONTEXT_DEVICES, 
            deviceListSize, 
            devices, 
            NULL);
    if(!sampleCommon->checkVal(
                status,
                CL_SUCCESS, 
                "clGetGetContextInfo failed."))
        return SDK_FAILURE;

    {
        /* The block is to move the declaration of prop closer to its use */
        cl_command_queue_properties prop = 0;
        if(timing)
            prop |= CL_QUEUE_PROFILING_ENABLE;

        commandQueue = clCreateCommandQueue(
                context, 
                devices[0], 
                prop, 
                &status);
        if(!sampleCommon->checkVal(
                    status,
                    0,
                    "clCreateCommandQueue failed."))
            return SDK_FAILURE;
    }

    outputBuffer = clCreateBuffer(
            context, 
            CL_MEM_WRITE_ONLY | CL_MEM_USE_HOST_PTR,
            sizeof(cl_float) * width * height,
            output, 
            &status);
    if(!sampleCommon->checkVal(
                status,
                CL_SUCCESS,
                "clCreateBuffer failed. (outputBuffer)"))
        return SDK_FAILURE;

    /* create a CL program using the kernel source */
    //streamsdk::SDKFile kernelFile;
    //std::string kernelPath = sampleCommon->getPath();
    //kernelPath.append("SHA1_Kernels.cl");
    //kernelFile.open(kernelPath.c_str());
    const char * source = "SHA1_Kernels.bc"; //kernelFile.source().c_str();
    size_t sourceSize[] = { strlen(source) };
    program = clCreateProgramWithSource(
        context,
        1,
        &source,
        sourceSize,
        &status);
    if(!sampleCommon->checkVal(
            status,
            CL_SUCCESS,
            "clCreateProgramWithSource failed."))
        return SDK_FAILURE;

    /* create a cl program executable for all the devices specified */
    status = clBuildProgram(program, 1, devices, NULL, NULL, NULL);
    if(!sampleCommon->checkVal(
                status,
                CL_SUCCESS,
                "clBuildProgram failed."))
        return SDK_FAILURE;

    /* get a kernel object handle for a kernel with the given name */
    kernel = clCreateKernel(program, "sha1", &status);
    if(!sampleCommon->checkVal(
                status,
                CL_SUCCESS,
                "clCreateKernel failed."))
        return SDK_FAILURE;

    return SDK_SUCCESS;
}


int 
SHA1::runCLKernels(void)
{
    cl_int   status;
    cl_event events[2];

    size_t globalThreads[1];
    size_t localThreads[1];

    globalThreads[0] = len;
    localThreads[0]  = 1;

    /* Check group size against kernelWorkGroupSize */
    status = clGetKernelWorkGroupInfo(kernel,
                                      devices[0],
                                      CL_KERNEL_WORK_GROUP_SIZE,
                                      sizeof(size_t),
                                      &kernelWorkGroupSize,
                                      0);
    if(!sampleCommon->checkVal(
                        status,
                        CL_SUCCESS, 
                        "clGetKernelWorkGroupInfo failed."))
    {
        return SDK_FAILURE;
    }

    if((cl_uint)(localThreads[0]) > kernelWorkGroupSize)
    {
        std::cout<<"Out of Resources!" << std::endl;
        std::cout<<"Group Size specified : "<<localThreads[0]<<std::endl;
        std::cout<<"Max Group Size supported on the kernel : " 
            <<kernelWorkGroupSize<<std::endl;
        return SDK_FAILURE;
    }

    /*** Set appropriate arguments to the kernel ***/
    status = clSetKernelArg(
                    kernel, 
                    0, 
                    sizeof(cl_mem),
                   (void *)&H_array);
    if(!sampleCommon->checkVal(
                status,
                CL_SUCCESS,
                "clSetKernelArg failed. (inputBuffer)"))
        return SDK_FAILURE;

	status = clSetKernelArg(
                    kernel,
                    1,
                    sizeof(cl_mem),
                   (void *)&W_array);
    if(!sampleCommon->checkVal(
                status,
                CL_SUCCESS,
                "clSetKernelArg failed. (inputBuffer)"))
        return SDK_FAILURE;

	status = clSetKernelArg(
                    kernel,
                    2,
                    sizeof(cl_mem),
                   (void *)&work_items);
    if(!sampleCommon->checkVal(
                status,
                CL_SUCCESS,
                "clSetKernelArg failed. (inputBuffer)"))
        return SDK_FAILURE;

    /* 
     * Enqueue a kernel run call.
     */
    status = clEnqueueNDRangeKernel(
            commandQueue,
            kernel,
            1,
            NULL,
            globalThreads,
            localThreads,
            0,
            NULL,
            &events[0]);

    if(!sampleCommon->checkVal(
                status,
                CL_SUCCESS,
                "clEnqueueNDRangeKernel failed."))
        return SDK_FAILURE;


    /* wait for the kernel call to finish execution */
    status = clWaitForEvents(1, &events[0]);
    if(!sampleCommon->checkVal(
                status,
                CL_SUCCESS,
                "clWaitForEvents failed."))
        return SDK_FAILURE;
    
    /* Enqueue readBuffer*/
    status = clEnqueueReadBuffer(
                commandQueue,
                outputBuffer,
                CL_TRUE,
                0,
                width * height * sizeof(cl_float),
                output,
                0,
                NULL,
                &events[1]);
    if(!sampleCommon->checkVal(
    		status,
    		CL_SUCCESS,
    		"clEnqueueReadBuffer failed."))
    	return SDK_FAILURE;
    
    /* Wait for the read buffer to finish execution */
    status = clWaitForEvents(1, &events[1]);
    if(!sampleCommon->checkVal(
    		status,
    		CL_SUCCESS,
    		"clWaitForEvents failed."))
    	return SDK_FAILURE;
    
    clReleaseEvent(events[1]);
    return SDK_SUCCESS;
}

/**
* SHA1 fractal generated with CPU reference implementation
*/
void 
SHA1::sha1CPUReference(cl_uint *W, cl_uint *H, cl_uint *workItems)
{
	verificationOutput[0] = 255;
}

int SHA1::initialize()
{
    // Call base class Initialize to get default configuration
    if(!this->SDKSample::initialize())
        return SDK_FAILURE;

    streamsdk::Option* image_size = new streamsdk::Option;
    if(!image_size)
    {
        sampleCommon->error("Memory allocation error.\n");
        return SDK_FAILURE;
    }
    
    image_size->_sVersion = "x";
    image_size->_lVersion = "size";
    image_size->_description = "size of the data";
    image_size->_type = streamsdk::CA_ARG_INT;
    image_size->_value = &width;
    sampleArgs->AddOption(image_size);
    delete image_size;

    streamsdk::Option* scale_param = new streamsdk::Option;
    if(!scale_param)
    {
        sampleCommon->error("Memory allocation error.\n");
        return SDK_FAILURE;
    }

    streamsdk::Option* num_iterations = new streamsdk::Option;
    if(!num_iterations)
    {
        sampleCommon->error("Memory allocation error.\n");
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

int SHA1::setup()
{

    if(!sampleCommon->isPowerOf2(width))
        width = sampleCommon->roundToPowerOf2(width);

    height = width;

    scale  = scale_int * 1.0f;

    iterations = 1;

    if(setupSHA1()!=SDK_SUCCESS)
        return SDK_FAILURE;

    int timer = sampleCommon->createTimer();
    sampleCommon->resetTimer(timer);
    sampleCommon->startTimer(timer);

    if(setupCL()!=SDK_SUCCESS)
        return SDK_FAILURE;

    sampleCommon->stopTimer(timer);

    setupTime = (cl_double)sampleCommon->readTimer(timer);

    return SDK_SUCCESS;
}


int SHA1::run()
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
            return SDK_FAILURE;
    }

    sampleCommon->stopTimer(timer);
    totalKernelTime = (double)(sampleCommon->readTimer(timer)) / iterations;

    if(!quiet) {
        sampleCommon->printArray<cl_int>("Output", output, width, 1);
    }

    return SDK_SUCCESS;
}

int SHA1::verifyResults()
{
    if(verify)
    {
        /* reference implementation
         * it overwrites the input array with the output
         */
        int refTimer = sampleCommon->createTimer();
        sampleCommon->resetTimer(refTimer);
        sampleCommon->startTimer(refTimer);
        //sha1CPUReference(W, H, workItems);
        sampleCommon->stopTimer(refTimer);
        referenceKernelTime = sampleCommon->readTimer(refTimer);
		std::cout << "Reference Kernel Time: " << referenceKernelTime << "\n";
		std::cout << "Total Kernel Time    : " << totalKernelTime << "\n";

        // compare the results and see if they match 
        if(memcmp(output, verificationOutput, width*height*sizeof(cl_int)) == 0)
        {
            std::cout<<"Passed!\n";
            return SDK_SUCCESS;
        }
        else
        {
            std::cout<<"Failed\n";
            return SDK_FAILURE;
        }

    }

    return SDK_SUCCESS;
}

void SHA1::printStats()
{
    std::string strArray[3] = {"Width", "Height", "Time(sec)"};
    std::string stats[3];

    totalTime = setupTime + totalKernelTime;

    stats[0] = sampleCommon->toString(width, std::dec);
    stats[1] = sampleCommon->toString(height, std::dec);
    stats[2] = sampleCommon->toString(totalTime, std::dec);

    this->SDKSample::printStats(strArray, stats, 3);
}

int SHA1::cleanup()
{
    /* Releases OpenCL resources (Context, Memory etc.) */
    cl_int status;

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

    status = clReleaseMemObject(outputBuffer);
    if(!sampleCommon->checkVal(
                status,
                CL_SUCCESS,
                "clReleaseMemObject failed."))
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

    /* release program resources (input memory etc.) */
    if(output) 
        free(output);

    if(verificationOutput) 
        free(verificationOutput);

    if(devices)
        free(devices);

    return SDK_SUCCESS;
}


int
main(int argc, char * argv[])
{
    SHA1 clSHA1("OpenCL SHA1");

   if(clSHA1.initialize()!=SDK_SUCCESS)
	   return SDK_FAILURE;
   if(!clSHA1.parseCommandLine(argc, argv))
      return SDK_FAILURE;
   if(clSHA1.setup()!=SDK_SUCCESS)
      return SDK_FAILURE;
   if(clSHA1.run()!=SDK_SUCCESS)
      return SDK_FAILURE;
   if(clSHA1.verifyResults()!=SDK_SUCCESS)
      return SDK_FAILURE;
   if(clSHA1.cleanup()!=SDK_SUCCESS)
      return SDK_FAILURE;
   clSHA1.printStats();

    return SDK_SUCCESS;
}
