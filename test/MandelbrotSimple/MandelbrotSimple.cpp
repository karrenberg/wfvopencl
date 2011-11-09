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


#include "MandelbrotSimple.hpp"

#ifndef min
int min(int a1, int a2)
{
    return ((a1 < a2) ? a1 : a2);
}
#endif

int 
MandelbrotSimple::setupMandelbrotSimple()
{
    cl_uint sizeBytes;

    /* allocate and init memory used by host */
    sizeBytes = width * height * sizeof(cl_uint);
    output = (cl_uint *) malloc(sizeBytes);
    if(output==NULL)    
    { 
        sampleCommon->error("Failed to allocate host memory. (output)");
        return SDK_FAILURE;
    }

    if(verify)
    {
        verificationOutput = (cl_uint *)malloc(sizeBytes);
        if(verificationOutput==NULL)    
        { 
            sampleCommon->error("Failed to allocate host memory. (verificationOutput)");
            return SDK_FAILURE;
        }

    }
    return SDK_SUCCESS;
}

int 
MandelbrotSimple::genBinaryImage()
{
    cl_int status = CL_SUCCESS;

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

        char platformName[100];
        for (unsigned i = 0; i < numPlatforms; ++i) 
        {
            status = clGetPlatformInfo(platforms[i],
                                       CL_PLATFORM_VENDOR,
                                       sizeof(platformName),
                                       platformName,
                                       NULL);

            if(!sampleCommon->checkVal(status,
                                       CL_SUCCESS,
                                       "clGetPlatformInfo failed."))
            {
                return SDK_FAILURE;
            }

            platform = platforms[i];
            if (!strcmp(platformName, "Advanced Micro Devices, Inc.")) 
            {
                break;
            }
        }
        std::cout << "Platform found : " << platformName << "\n";
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
    cl_context_properties cps[5] = 
    {
        CL_CONTEXT_PLATFORM, 
        (cl_context_properties)platform, 
        CL_CONTEXT_OFFLINE_DEVICES_AMD,
        (cl_context_properties)1,
        0
    };

    context = clCreateContextFromType(cps,
                                      CL_DEVICE_TYPE_ALL,
                                      NULL,
                                      NULL,
                                      &status);

    if(!sampleCommon->checkVal(status,
                               CL_SUCCESS,
                               "clCreateContextFromType failed."))
    {
        return SDK_FAILURE;
    }

    /* create a CL program using the kernel source */
    streamsdk::SDKFile kernelFile;
    std::string kernelPath = sampleCommon->getPath();
    kernelPath.append("MandelbrotSimple_Kernels.cl");
    if(!kernelFile.open(kernelPath.c_str()))
    {
        std::cout << "Failed to load kernel file : " << kernelPath << std::endl;
        return SDK_FAILURE;
    }
    const char * source = kernelFile.source().c_str();
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
    
    std::string flagsStr = std::string("");

    // Get additional options
    if(isComplierFlagsSpecified())
    {
        streamsdk::SDKFile flagsFile;
        std::string flagsPath = sampleCommon->getPath();
        flagsPath.append(flags.c_str());
        if(!flagsFile.open(flagsPath.c_str()))
        {
            std::cout << "Failed to load flags file: " << flagsPath << std::endl;
            return SDK_FAILURE;
        }
        flagsFile.replaceNewlineWithSpaces();
        const char * flags = flagsFile.source().c_str();
        flagsStr.append(flags);
    }

    if(flagsStr.size() != 0)
        std::cout << "Build Options are : " << flagsStr.c_str() << std::endl;


    /* create a cl program executable for all the devices specified */
    status = clBuildProgram(program,
                            0,
                            NULL,
                            flagsStr.c_str(),
                            NULL,
                            NULL);

    sampleCommon->checkVal(status,
                        CL_SUCCESS,
                        "clBuildProgram failed.");

    size_t numDevices;
    status = clGetProgramInfo(program, 
                           CL_PROGRAM_NUM_DEVICES,
                           sizeof(numDevices),
                           &numDevices,
                           NULL );
    if(!sampleCommon->checkVal(status,
                               CL_SUCCESS,
                               "clGetProgramInfo(CL_PROGRAM_NUM_DEVICES) failed."))
    {
        return SDK_FAILURE;
    }

    std::cout << "Number of devices found : " << numDevices << "\n\n";
    devices = (cl_device_id *)malloc( sizeof(cl_device_id) * numDevices );
    if(devices == NULL)
    {
        sampleCommon->error("Failed to allocate host memory.(devices)");
        return SDK_FAILURE;
    }
    /* grab the handles to all of the devices in the program. */
    status = clGetProgramInfo(program, 
                              CL_PROGRAM_DEVICES, 
                              sizeof(cl_device_id) * numDevices,
                              devices,
                              NULL );
    if(!sampleCommon->checkVal(status,
                               CL_SUCCESS,
                               "clGetProgramInfo(CL_PROGRAM_DEVICES) failed."))
    {
        return SDK_FAILURE;
    }


    /* figure out the sizes of each of the binaries. */
    size_t *binarySizes = (size_t*)malloc( sizeof(size_t) * numDevices );
    if(devices == NULL)
    {
        sampleCommon->error("Failed to allocate host memory.(binarySizes)");
        return SDK_FAILURE;
    }
    
    status = clGetProgramInfo(program, 
                              CL_PROGRAM_BINARY_SIZES,
                              sizeof(size_t) * numDevices, 
                              binarySizes, NULL);
    if(!sampleCommon->checkVal(status,
                               CL_SUCCESS,
                               "clGetProgramInfo(CL_PROGRAM_BINARY_SIZES) failed."))
    {
        return SDK_FAILURE;
    }

    size_t i = 0;
    /* copy over all of the generated binaries. */
    char **binaries = (char **)malloc( sizeof(char *) * numDevices );
    if(binaries == NULL)
    {
        sampleCommon->error("Failed to allocate host memory.(binaries)");
        return SDK_FAILURE;
    }

    for(i = 0; i < numDevices; i++)
    {
        if(binarySizes[i] != 0)
        {
            binaries[i] = (char *)malloc( sizeof(char) * binarySizes[i]);
            if(binaries[i] == NULL)
            {
                sampleCommon->error("Failed to allocate host memory.(binaries[i])");
                return SDK_FAILURE;
            }
        }
        else
        {
            binaries[i] = NULL;
        }
    }
    status = clGetProgramInfo(program, 
                              CL_PROGRAM_BINARIES,
                              sizeof(char *) * numDevices, 
                              binaries, 
                              NULL);
    if(!sampleCommon->checkVal(status,
                               CL_SUCCESS,
                               "clGetProgramInfo(CL_PROGRAM_BINARIES) failed."))
    {
        return SDK_FAILURE;
    }

    /* dump out each binary into its own separate file. */
    for(i = 0; i < numDevices; i++)
    {
        char fileName[100];
        sprintf(fileName, "%s.%d", dumpBinary.c_str(), (int)i);
        if(binarySizes[i] != 0)
        {
            char deviceName[1024];
            status = clGetDeviceInfo(devices[i], 
                                     CL_DEVICE_NAME, 
                                     sizeof(deviceName),
                                     deviceName, 
                                     NULL);
            if(!sampleCommon->checkVal(status,
                                       CL_SUCCESS,
                                       "clGetDeviceInfo(CL_DEVICE_NAME) failed."))
            {
                return SDK_FAILURE;
            }

            printf( "%s binary kernel: %s\n", deviceName, fileName);
            streamsdk::SDKFile BinaryFile;
            if(!BinaryFile.writeBinaryToFile(fileName, 
                                             binaries[i], 
                                             binarySizes[i]))
            {
                std::cout << "Failed to load kernel file : " << fileName << std::endl;
                return SDK_FAILURE;
            }
        }
        else
        {
            printf("Skipping %s since there is no binary data to write!\n",
                    fileName);
        }
    }

    // Release all resouces and memory
    for(i = 0; i < numDevices; i++)
    {
        if(binaries[i] != NULL)
        {
            free(binaries[i]);
            binaries[i] = NULL;
        }
    }

    if(binaries != NULL)
    {
        free(binaries);
        binaries = NULL;
    }

    if(binarySizes != NULL)
    {
        free(binarySizes);
        binarySizes = NULL;
    }

    if(devices != NULL)
    {
        free(devices);
        devices = NULL;
    }

    status = clReleaseProgram(program);
    if(!sampleCommon->checkVal(status,
                               CL_SUCCESS,
                               "clReleaseProgram failed."))
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

int
MandelbrotSimple::setupCL(void)
{
    cl_int status = 0;
    size_t deviceListSize;

    if(deviceType.compare("cpu") == 0)
    {
        dType = CL_DEVICE_TYPE_CPU;
    }
    else if(deviceType.compare("all") == 0)
    {
        dType = CL_DEVICE_TYPE_ALL;
    }
    else //deviceType = "gpu" 
    {
        dType = CL_DEVICE_TYPE_GPU;
        if(isThereGPU() == false)
        {
            std::cout << "GPU not found. Falling back to CPU device" << std::endl;
            dType = CL_DEVICE_TYPE_CPU;
        }
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
        if(isPlatformEnabled())
        {
            platform = platforms[platformId];
        }
        else
        {
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
        }
        delete[] platforms;
    }

    if(NULL == platform)
    {
        sampleCommon->error("NULL platform found so Exiting Application.");
        return SDK_FAILURE;
    }

    // Display available devices.
    if(!sampleCommon->displayDevices(platform, dType))
    {
        sampleCommon->error("sampleCommon::displayDevices() failed");
        return SDK_FAILURE;
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

    numDevices = (cl_uint)(deviceListSize/sizeof(cl_device_id));
    numDevices = min(MAX_DEVICES, numDevices);

    if(numDevices != 1 && isLoadBinaryEnabled())
    {
        sampleCommon->expectedError("--load option is not supported if devices are more one");
        return SDK_EXPECTED_FAILURE;
    }

    if(numDevices == 3)
    {
        if(!quiet)
        {
            std::cout << "Number of devices must be even,"
                 << "\nChanging number of devices from three to two\n";
        }
        numDevices = 2;
    }

    for (cl_uint i = 0; i < numDevices; i++)
    {
        /* The block is to move the declaration of prop closer to its use */
        cl_command_queue_properties prop = 0;
        if(timing)
            prop |= CL_QUEUE_PROFILING_ENABLE;

        commandQueue[i] = clCreateCommandQueue(
                context, 
                devices[i], 
                prop, 
                &status);
        if(!sampleCommon->checkVal(
                    status,
                    0,
                    "clCreateCommandQueue failed."))
            return SDK_FAILURE;

        outputBuffer[i] = clCreateBuffer(
                context, 
                CL_MEM_WRITE_ONLY | CL_MEM_USE_HOST_PTR,
                (sizeof(cl_float) * width * height) / numDevices,
                output + ((width * height) / numDevices) * i, 
                &status);
        if(!sampleCommon->checkVal(
                    status,
                    CL_SUCCESS,
                    "clCreateBuffer failed. (outputBuffer)"))
            return SDK_FAILURE;
    }

    /* create a CL program using the kernel source */
    streamsdk::SDKFile kernelFile;

    std::string kernelPath = sampleCommon->getPath();
    if(isLoadBinaryEnabled())
    {
        kernelPath.append(loadBinary.c_str());

        if(!kernelFile.readBinaryFromFile(kernelPath.c_str()))
        {
            std::cout << "Failed to load kernel file : " << kernelPath << std::endl;
            return SDK_FAILURE;
        }

        const char * binary = kernelFile.source().c_str();
        size_t binarySize = kernelFile.source().size();
        program = clCreateProgramWithBinary(context,
                                            1,
                                            &devices[deviceId], 
                                            (const size_t *)&binarySize,
                                            (const unsigned char**)&binary,
                                            NULL,
                                            &status);
    }
    else
	{
		// special case for packetized OpenCL (can not yet compile .cl directly)
		char vName[100];
		status = clGetPlatformInfo(platform,
				CL_PLATFORM_VENDOR,
				sizeof(vName),
				vName,
				NULL);
		const bool platformIsPacketizedOpenCL = !strcmp(vName, "Ralf Karrenberg, Saarland University");
		if (!strcmp(vName, "Intel(R) Corporation")) {
			vendorName = "intel";
		} else if (!strcmp(vName, "Advanced Micro Devices, Inc.")) {
			vendorName = "amd";
		} else if (platformIsPacketizedOpenCL) {
			vendorName = "pkt";
		} else {
			printf("ERROR: vendor not recognized: %s\n", vName);
		}

		kernelPath.append("MandelbrotSimple_Kernels.cl");
		if(!kernelFile.open(kernelPath.c_str()))
		{
			std::cout << "Failed to load kernel file : " << kernelPath << std::endl;
			return SDK_FAILURE;
		}

		const char * source = kernelFile.source().c_str();

		size_t sourceSize[] = { strlen(source) };
		program = clCreateProgramWithSource(context,
				1,
				&source,
				sourceSize,
				&status);
	}
    if(!sampleCommon->checkVal(
            status,
            CL_SUCCESS,
            "clCreateProgramWithSource failed."))
        return SDK_FAILURE;
    std::string flagsStr = std::string("");

    // Get additional options
    if(isComplierFlagsSpecified())
    {
        streamsdk::SDKFile flagsFile;
        std::string flagsPath = sampleCommon->getPath();
        flagsPath.append(flags.c_str());
        if(!flagsFile.open(flagsPath.c_str()))
        {
            std::cout << "Failed to load flags file: " << flagsPath << std::endl;
            return SDK_FAILURE;
        }
        flagsFile.replaceNewlineWithSpaces();
        const char * flags = flagsFile.source().c_str();
        flagsStr.append(flags);
    }

    if(flagsStr.size() != 0)
        std::cout << "Build Options are : " << flagsStr.c_str() << std::endl;

    

    /* create a cl program executable for all the devices specified */
    status = clBuildProgram(program, 
                            numDevices, 
                            devices, 
                            flagsStr.c_str(), 
                            NULL, 
                            NULL);

    if(status != CL_SUCCESS)
    {
        if(status == CL_BUILD_PROGRAM_FAILURE)
        {
            for (cl_uint i = 0; i < numDevices; i++)
            {
                cl_int logStatus;
                char * buildLog = NULL;
                size_t buildLogSize = 0;
                logStatus = clGetProgramBuildInfo (program, 
                    devices[deviceId], 
                    CL_PROGRAM_BUILD_LOG, 
                    buildLogSize, 
                    buildLog, 
                    &buildLogSize);
                if(!sampleCommon->checkVal(
                    logStatus,
                    CL_SUCCESS,
                    "clGetProgramBuildInfo failed."))
                    return SDK_FAILURE;

                buildLog = (char*)malloc(buildLogSize);
                if(buildLog == NULL)
                {
                    sampleCommon->error("Failed to allocate host memory."
                        "(buildLog)");
                    return SDK_FAILURE;
                }
                memset(buildLog, 0, buildLogSize);

                logStatus = clGetProgramBuildInfo (program, 
                    devices[deviceId], 
                    CL_PROGRAM_BUILD_LOG, 
                    buildLogSize, 
                    buildLog, 
                    NULL);
                if(!sampleCommon->checkVal(
                    logStatus,
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
        }

        if(!sampleCommon->checkVal(
            status,
            CL_SUCCESS,
            "clBuildProgram failed."))
            return SDK_FAILURE;
    }

    for (cl_uint i = 0; i < numDevices; i++)
    {
        
       
        /* get a kernel object handle for a kernel with the given name */
        kernel_vector[i] = clCreateKernel(program, "mandelbrot", &status);
        if(!sampleCommon->checkVal(
                    status,
                    CL_SUCCESS,
                    "clCreateKernel failed."))
            return SDK_FAILURE;
    }

    return SDK_SUCCESS;
}


int 
MandelbrotSimple::runCLKernels(void)
{
    cl_int   status;
    cl_event events[MAX_DEVICES];
	cl_kernel kernel;

    size_t globalThreads[1];
    size_t localThreads[1];

	benched = 0;
    globalThreads[0] = width*height / numDevices;
    localThreads[0]  = 1;

	for (cl_uint i = 0; i < numDevices; i++)
	{
		kernel = kernel_vector[i];

		/* Check group size against kernelWorkGroupSize */
		status = clGetKernelWorkGroupInfo(kernel,
										  devices[i],
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
			localThreads[0] = kernelWorkGroupSize;

		/*** Set appropriate arguments to the kernel ***/
		status = clSetKernelArg(
				kernel, 
				0, 
				sizeof(cl_mem),
				(void *)&outputBuffer[i]);
		if(!sampleCommon->checkVal(
					status,
					CL_SUCCESS,
					"clSetKernelArg failed. (inputBuffer)"))
			return SDK_FAILURE;

		status = clSetKernelArg(
				kernel, 
				1, 
				sizeof(cl_float), 
				(void *)&scale);
		if(!sampleCommon->checkVal(
					status,
					CL_SUCCESS,
					"clSetKernelArg failed. (scale)"))
			return SDK_FAILURE;

		status = clSetKernelArg(
				kernel, 
				2, 
				sizeof(cl_uint), 
				(void *)&maxIterations);
		if(!sampleCommon->checkVal(
					status,
                CL_SUCCESS,
                "clSetKernelArg failed. (maxIterations)"))
        return SDK_FAILURE;

		/* width - i.e number of elements in the array */
		status = clSetKernelArg(
				kernel, 
				3, 
				sizeof(cl_int), 
				(void *)&width);
		if(!sampleCommon->checkVal(
					status,
					CL_SUCCESS,
					"clSetKernelArg failed. (width)"))
			return SDK_FAILURE;

		/* 
		 * Enqueue a kernel run call.
		 */
		status = clEnqueueNDRangeKernel(
				commandQueue[i],
				kernel,
				1,
				NULL,
				globalThreads,
				localThreads,
				0,
				NULL,
				&events[i]);

		if(!sampleCommon->checkVal(
					status,
					CL_SUCCESS,
					"clEnqueueNDRangeKernel failed."))
			return SDK_FAILURE;
	}
	/* flush the queues to get things started */
	for (cl_uint i = 0; i < numDevices; i++)
	{
        clFlush(commandQueue[i]);
    }


    /* wait for the kernel call to finish execution */
    for (cl_uint i = 0; i < numDevices; i++)
    {
        // Wait in reverse order
        status = clWaitForEvents(1, &events[numDevices-i-1]);
        if(!sampleCommon->checkVal(
                    status,
                    CL_SUCCESS,
                    "clWaitForEvents failed."))
            return SDK_FAILURE;
    }

    if (timing && bench)
    {
        cl_ulong start;
        cl_ulong stop;
        status = clGetEventProfilingInfo(events[0],
                                         CL_PROFILING_COMMAND_SUBMIT,
                                         sizeof(cl_ulong),
                                         &start,
                                         NULL);
        if(!sampleCommon->checkVal(
                    status,
                    CL_SUCCESS,
                    "clGetEventProfilingInfo failed."))
            return SDK_FAILURE;

        status = clGetEventProfilingInfo(events[0],
                                         CL_PROFILING_COMMAND_END,
                                         sizeof(cl_ulong),
                                         &stop,
                                         NULL);
        if(!sampleCommon->checkVal(
                    status,
                    CL_SUCCESS,
                    "clGetEventProfilingInfo failed."))
            return SDK_FAILURE;

        time = (cl_double)(stop - start)*(cl_double)(1e-09);
    }
    for (cl_uint i = 0; i < numDevices; i++)
    {
        /* Enqueue readBuffer*/
        status = clEnqueueReadBuffer(
                    commandQueue[i],
                    outputBuffer[i],
                    CL_TRUE,
                    0,
                    (width * height * sizeof(cl_float)) / numDevices,
                    output + (width * height / numDevices) * i,
                    0,
                    NULL,
                    &events[0]);
        if(!sampleCommon->checkVal(
            status,
            CL_SUCCESS,
            "clEnqueueReadBuffer failed."))
        return SDK_FAILURE;

        /* Wait for the read buffer to finish execution */
        status = clWaitForEvents(1, &events[0]);
        if(!sampleCommon->checkVal(
            status,
            CL_SUCCESS,
            "clWaitForEvents failed."))
        return SDK_FAILURE;
    }

    if (timing && bench)
    {
        cl_ulong totalIterations = 0;
        for (int i = 0; i < (width * height); i++)
        {
            totalIterations += output[i];
        }
        cl_double flops = 7.0*totalIterations;
        printf("%lf MFLOPs\n", flops*(double)(1e-6)/time);
        printf("%lf MFLOPs according to CPU\n", flops*(double)(1e-6)/totalKernelTime);
        bench = 0;
        benched = 1;
    }
    clReleaseEvent(events[0]);
    return SDK_SUCCESS;
}

/**
* MandelbrotSimple fractal generated with CPU reference implementation
*/
void 
MandelbrotSimple::mandelbrotCPUReference(cl_uint * verificationOutput,
                                   cl_float  mscale, 
                                   cl_uint maxIter, 
                                   cl_int w)

{
    for(cl_int j=0; j < w ; ++j)
        for(cl_int i=0; i < w; ++i)
        {
            cl_float x0 = ((i*mscale) - (mscale/2*w))/w;
            cl_float y0 = ((j*mscale) - (mscale/2*w))/w;

            cl_float x = x0;
            cl_float y = y0;

            cl_float x2 = x*x;
            cl_float y2 = y*y;

            cl_float scaleSquare= mscale*mscale;

            cl_uint iter=0;
            for(iter=0; (x2+y2 <= scaleSquare) && (iter < maxIter); ++iter)
            {
                y = 2 * x*y + y0;
                x = x2 - y2   + x0;

                x2 = x*x;
                y2 = y*y;
            }
            verificationOutput[j*w + i] = 255*iter/maxIter;
        }
}

int MandelbrotSimple::initialize()
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
    image_size->_description = "size of the mandelbrot image";
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

    scale_param->_sVersion = "s";
    scale_param->_lVersion = "scale";
    scale_param->_description = "Scaling factor to generate the mandelbrot fractal";
    scale_param->_type = streamsdk::CA_ARG_INT;
    scale_param->_value = &scale_int;
    sampleArgs->AddOption(scale_param);
    delete scale_param;

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

int MandelbrotSimple::setup()
{

    if(!sampleCommon->isPowerOf2(width))
        width = sampleCommon->roundToPowerOf2(width);

    height = width;

    scale  = scale_int * 1.0f;

    if(setupMandelbrotSimple()!=SDK_SUCCESS)
        return SDK_FAILURE;

    int timer = sampleCommon->createTimer();
    sampleCommon->resetTimer(timer);
    sampleCommon->startTimer(timer);

    int returnVal = setupCL();
    if(returnVal != SDK_SUCCESS)
        return (returnVal == SDK_EXPECTED_FAILURE)? SDK_EXPECTED_FAILURE : SDK_FAILURE;

    sampleCommon->stopTimer(timer);

    setupTime = (cl_double)sampleCommon->readTimer(timer);

    return SDK_SUCCESS;
}


int MandelbrotSimple::run()
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
    totalKernelTime = (double)(sampleCommon->readTimer(timer)) / iterations;

    if(!quiet) {
        sampleCommon->printArray<cl_uint>("Output", output, width, 1);
    }

    return SDK_SUCCESS;
}

int MandelbrotSimple::verifyResults()
{
    if(verify)
    {
        /* reference implementation
         * it overwrites the input array with the output
         */
        int refTimer = sampleCommon->createTimer();
        sampleCommon->resetTimer(refTimer);
        sampleCommon->startTimer(refTimer);
        mandelbrotCPUReference(verificationOutput, scale, maxIterations, width);
        sampleCommon->stopTimer(refTimer);
        referenceKernelTime = sampleCommon->readTimer(refTimer);
		
		if (!quiet) {
			std::cout << "Reference Kernel Time: " << referenceKernelTime << "\n";
			std::cout << "Total Kernel Time    : " << totalKernelTime << "\n";
		}

        // compare the results and see if they match 
        if(memcmp(output, verificationOutput, width*height*sizeof(cl_uint)) == 0)
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

void MandelbrotSimple::printStats()
{
    std::string strArray[4] = {"Width", "Height", "Time(sec)", "kernelTime(sec)"};
    std::string stats[4];

    totalTime = setupTime + totalKernelTime;

    stats[0] = sampleCommon->toString(width, std::dec);
    stats[1] = sampleCommon->toString(height, std::dec);
    stats[2] = sampleCommon->toString(totalTime, std::dec);
    stats[3] = sampleCommon->toString(totalKernelTime, std::dec);

    this->SDKSample::printStats(strArray, stats, 4);
    this->SDKSample::logStats(totalKernelTime, "MandelbrotSimple", vendorName);
}

int MandelbrotSimple::cleanup()
{
    /* Releases OpenCL resources (Context, Memory etc.) */
    cl_int status;

    status = clReleaseProgram(program);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clReleaseProgram failed."))
        return SDK_FAILURE;

    for (cl_uint i = 0; i < numDevices; i++)
    {
        status = clReleaseKernel(kernel_vector[i]);
        if(!sampleCommon->checkVal(
            status,
            CL_SUCCESS,
            "clReleaseKernel failed."))
            return SDK_FAILURE;

        status = clReleaseMemObject(outputBuffer[i]);
        if(!sampleCommon->checkVal(
                    status,
                    CL_SUCCESS,
                    "clReleaseMemObject failed."))
            return SDK_FAILURE;

        status = clReleaseCommandQueue(commandQueue[i]);
        if(!sampleCommon->checkVal(
            status,
            CL_SUCCESS,
            "clReleaseCommandQueue failed."))
            return SDK_FAILURE;
    }

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

cl_uint MandelbrotSimple::getWidth(void)
{
    return width;
}

cl_uint MandelbrotSimple::getHeight(void)
{
    return height;
}


cl_uint * MandelbrotSimple::getPixels(void)
{
    return output;
}

cl_bool MandelbrotSimple::showWindow(void)
{
    return !quiet;
}
