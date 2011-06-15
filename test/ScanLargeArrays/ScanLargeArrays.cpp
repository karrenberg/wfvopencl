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


#include "ScanLargeArrays.hpp"


int 
ScanLargeArrays::setupScanLargeArrays()
{
    /* input buffer size */
    cl_uint sizeBytes = length * sizeof(cl_float);

    /* allocate memory for input arrray */
    input = (cl_float*)malloc(sizeBytes);

    /* error check */
    if(input == NULL)    
    { 
        sampleCommon->error("Failed to allocate host memory. (input)");
        return SDK_FAILURE;
    }

    /* random initialisation of input */
    sampleCommon->fillRandom<cl_float>(input, length, 1, 0, 255);

    /* allocate memory for output buffer */
    output = (cl_float*)malloc(sizeBytes);

    /* error check */
    if(output == NULL)    
    { 
        sampleCommon->error("Failed to allocate host memory. (output)");
        return SDK_FAILURE;
    }


    /* if verification is enabled */
    if(verify)
    {
        /* allocate memory for verification output array */
        verificationOutput = (cl_float*)malloc(sizeBytes);

        if(verificationOutput == NULL)   
        { 
            sampleCommon->error("Failed to allocate host memory. (verify)");
            return SDK_FAILURE;
        }

        memset(verificationOutput, 0, sizeBytes);
    }
    /* Unless quiet mode has been enabled, print the INPUT array. */
    if(!quiet) 
    {
        sampleCommon->printArray<cl_float>("Input", 
                                           input, 
                                           length, 
                                           1);
    }

    return SDK_SUCCESS;
}

int 
ScanLargeArrays::genBinaryImage()
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
    kernelPath.append("ScanLargeArrays_Kernels.cl");
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
ScanLargeArrays::setupCL(void)
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
     * If we could find our platform, use it. Otherwise use just available platform.
     */
    cl_context_properties cps[3] = 
    {
        CL_CONTEXT_PLATFORM, 
        (cl_context_properties)platform, 
        0
    };

    context = clCreateContextFromType(
        cps,
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

    int deviceCount = (int)(deviceListSize / sizeof(cl_device_id));
    if(!sampleCommon->validateDeviceId(deviceId, deviceCount))
    {
        sampleCommon->error("sampleCommon::validateDeviceId() failed");
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
    status = clGetContextInfo(
        context, 
        CL_CONTEXT_DEVICES, 
        deviceListSize, 
        devices, 
        NULL);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS, 
        "clGetContextInfo failed."))
        return SDK_FAILURE;

    {
        /* The block is to move the declaration of prop closer to its use */
        cl_command_queue_properties prop = 0;
        commandQueue = clCreateCommandQueue(
            context, 
            devices[deviceId], 
            prop, 
            &status);
        if(!sampleCommon->checkVal(
            status,
            0,
            "clCreateCommandQueue failed."))
            return SDK_FAILURE;
    }

    /* Get Device specific Information */
    status = clGetDeviceInfo(
        devices[deviceId],
        CL_DEVICE_MAX_WORK_GROUP_SIZE,
        sizeof(size_t),
        (void*)&maxWorkGroupSize,
        NULL);

    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS, 
        "clGetDeviceInfo CL_DEVICE_MAX_WORK_GROUP_SIZE failed."))
        return SDK_FAILURE;


    status = clGetDeviceInfo(
        devices[deviceId],
        CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS,
        sizeof(cl_uint),
        (void*)&maxDimensions,
        NULL);

    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS, 
        "clGetDeviceInfo CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS failed."))
        return SDK_FAILURE;


    maxWorkItemSizes = (size_t*)malloc(maxDimensions * sizeof(size_t));

    status = clGetDeviceInfo(
        devices[deviceId],
        CL_DEVICE_MAX_WORK_ITEM_SIZES,
        sizeof(size_t) * maxDimensions,
        (void*)maxWorkItemSizes,
        NULL);

    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS, 
        "clGetDeviceInfo CL_DEVICE_MAX_WORK_ITEM_SIZES failed."))
        return SDK_FAILURE;


    status = clGetDeviceInfo(
        devices[deviceId],
        CL_DEVICE_LOCAL_MEM_SIZE,
        sizeof(cl_ulong),
        (void *)&totalLocalMemory,
        NULL);

    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS, 
        "clGetDeviceInfo CL_DEVICE_LOCAL_MEM_SIZE failed."))
        return SDK_FAILURE;


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
        if(!sampleCommon->checkVal(status,
                                   CL_SUCCESS,
                                   "clCreateProgramWithBinary failed."))
        {
            return SDK_FAILURE;
        }

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

	kernelPath.append("ScanLargeArrays_Kernels.cl");
	if(!kernelFile.open(kernelPath.c_str()))
	{
		std::cout << "Failed to load kernel file : " << kernelPath << std::endl;
		return SDK_FAILURE;
	}

	const char * source = platformIsPacketizedOpenCL ?
		"ScanLargeArrays_Kernels.bc" :
		kernelFile.source().c_str();

        size_t sourceSize[] = {strlen(source)};
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
                            1, 
                            &devices[deviceId], 
                            flagsStr.c_str(), 
                            NULL, 
                            NULL);
    if(status != CL_SUCCESS)
    {
        if(status == CL_BUILD_PROGRAM_FAILURE)
        {
            cl_int logStatus;
            char *buildLog = NULL;
            size_t buildLogSize = 0;
            logStatus = clGetProgramBuildInfo(program, 
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
                sampleCommon->error("Failed to allocate host memory. (buildLog)");
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

        if(!sampleCommon->checkVal(
            status,
            CL_SUCCESS,
            "clBuildProgram failed."))
            return SDK_FAILURE;
    }

    /* get a kernel object handle for a kernel with the given name */
    bScanKernel = clCreateKernel(program, "ScanLargeArrays", &status);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clCreateKernel failed. (bScanKernel)"))
        return SDK_FAILURE;

    bAddKernel = clCreateKernel(program, "blockAddition", &status);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clCreateKernel failed. (bAddKernel)"))
        return SDK_FAILURE;

    /* get a kernel object handle for a kernel with the given name */
    pScanKernel = clCreateKernel(program, "prefixSum", &status);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clCreateKernel failed. (pScanKernel)"))
        return SDK_FAILURE;

    size_t bScanKernelGroupSize;
    size_t pScanKernelGroupSize;
    size_t bAddKernelGroupSize;

    /* Group size should be same for all kernels */
    status = clGetKernelWorkGroupInfo(bScanKernel,
        devices[deviceId],
        CL_KERNEL_WORK_GROUP_SIZE,
        sizeof(size_t),
        &bScanKernelGroupSize,
        0);

    status = clGetKernelWorkGroupInfo(pScanKernel,
        devices[deviceId],
        CL_KERNEL_WORK_GROUP_SIZE,
        sizeof(size_t),
        &pScanKernelGroupSize,
        0);

    status = clGetKernelWorkGroupInfo(bAddKernel,
        devices[deviceId],
        CL_KERNEL_WORK_GROUP_SIZE,
        sizeof(size_t),
        &bAddKernelGroupSize,
        0);

    /* Find munimum of all kernel's group-sizes */
    size_t temp = min(bScanKernelGroupSize, pScanKernelGroupSize);
    temp = (temp > bAddKernelGroupSize) ? bAddKernelGroupSize : temp;

    if(blockSize > (cl_uint)temp)
    {
        if(!quiet)
        {
            std::cout << "Out of Resources!" << std::endl;
            std::cout << "Group Size specified : " << blockSize << std::endl;
            std::cout << "Max Group Size supported on the kernel : "
                      << temp << std::endl;
            std::cout << "Falling back to " << temp << std::endl;
        }
        blockSize = (cl_uint)temp;
    }

    /* Calculate number of passes required */
    float t = log((float)length) / log((float)blockSize);
    pass = (cl_uint)t;

    // If t is equal to pass
    if(fabs(t - (float)pass) < 1e-7)
    {
        pass--;
    }

    /* Create input buffer on device */
    inputBuffer = clCreateBuffer(
        context, 
        CL_MEM_READ_ONLY,
        sizeof(cl_float) * length,
        0, 
        &status);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clCreateBuffer failed. (inputBuffer)"))
        return SDK_FAILURE;

    /* Allocate output buffers */
    outputBuffer = (cl_mem*)malloc(pass * sizeof(cl_mem));

    for(int i = 0; i < (int)pass; i++)
    {
        int size = (int)(length / pow((float)blockSize,(float)i));
        outputBuffer[i] = clCreateBuffer(
            context, 
            CL_MEM_READ_WRITE,
            sizeof(cl_float) * size,
            0, 
            &status);

        if(!sampleCommon->checkVal(
            status,
            CL_SUCCESS,
            "clCreateBuffer failed. (outputBuffer)"))
            return SDK_FAILURE;
    }

    /* Allocate blockSumBuffers */
    blockSumBuffer = (cl_mem*)malloc(pass * sizeof(cl_mem));

    for(int i = 0; i < (int)pass; i++)
    {
        int size = (int)(length / pow((float)blockSize,(float)(i + 1)));
        blockSumBuffer[i] = clCreateBuffer(
            context, 
            CL_MEM_READ_WRITE,
            sizeof(cl_float) * size,
            0, 
            &status);

        if(!sampleCommon->checkVal(
            status,
            CL_SUCCESS,
            "clCreateBuffer failed. (blockSumBuffer)"))
            return SDK_FAILURE;
    }

    /* Create a tempBuffer on device */
    int tempLength = (int)(length / pow((float)blockSize, (float)pass));

    tempBuffer = clCreateBuffer(context,
        CL_MEM_READ_WRITE,
        sizeof(cl_float) * tempLength,
        0,
        &status);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clCreateBuffer failed. (tempBuffer)"))
        return SDK_FAILURE;

    return SDK_SUCCESS;

}

int 
ScanLargeArrays::bScan(cl_uint len, 
                       cl_mem *inputBuffer, 
                       cl_mem *outputBuffer,
                       cl_mem *blockSumBuffer)
{
    cl_int status;
    cl_event events[1];

    /* set the block size*/ 
    size_t globalThreads[1]= {len / 2};
    size_t localThreads[1] = {blockSize / 2};

    if(localThreads[0] > maxWorkItemSizes[0] ||
       localThreads[0] > maxWorkGroupSize)
    {
        std::cout<<"Unsupported: Device does not"
            "support requested number of work items.";

        return SDK_FAILURE;
    }

    /* Set appropriate arguments to the kernel */
    /* 1st argument to the kernel - outputBuffer */
    status = clSetKernelArg(
        bScanKernel, 
        0, 
        sizeof(cl_mem), 
        (void *)outputBuffer);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clSetKernelArg failed. (outputBuffer)"))
        return SDK_FAILURE;

    /* 2nd argument to the kernel - inputBuffer */
    status = clSetKernelArg(
        bScanKernel, 
        1, 
        sizeof(cl_mem), 
        (void *)inputBuffer);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clSetKernelArg failed. (inputBuffer)"))
        return SDK_FAILURE;

    /* 3rd argument to the kernel - local memory */
    status = clSetKernelArg(
        bScanKernel, 
        2, 
        blockSize * sizeof(cl_float), 
        NULL);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clSetKernelArg failed. (local memory)"))
        return SDK_FAILURE;


    /* 4th argument to the kernel - block_size  */
    status = clSetKernelArg(
        bScanKernel, 
        3, 
        sizeof(cl_int),
        &blockSize);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clSetKernelArg failed. (blockSize)"))
        return SDK_FAILURE;

    /* 5th argument to the kernel - length  */
    status = clSetKernelArg(
        bScanKernel, 
        4, 
        sizeof(cl_int),
        &len);

    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clSetKernelArg failed. (len)"))
        return SDK_FAILURE;

    /* 6th argument to the kernel - sum of blocks  */
    status = clSetKernelArg(
        bScanKernel, 
        5, 
        sizeof(cl_mem),
        blockSumBuffer);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clSetKernelArg failed. (blockSumBuffer)"))
        return SDK_FAILURE;


    status = clGetKernelWorkGroupInfo(bScanKernel,
        devices[deviceId],
        CL_KERNEL_LOCAL_MEM_SIZE,
        sizeof(cl_ulong),
        &usedLocalMemory,
        NULL);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clGetKernelWorkGroupInfo failed.(usedLocalMemory)"))
        return SDK_FAILURE;


    if(usedLocalMemory > totalLocalMemory)
    {
        std::cout << "Unsupported: Insufficient"
            "local memory on device." << std::endl;
        return SDK_FAILURE;
    }

    /* Enqueue a kernel run call.*/
    status = clEnqueueNDRangeKernel(
        commandQueue,
        bScanKernel,
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

    return SDK_SUCCESS;
}

int 
ScanLargeArrays::pScan(cl_uint len,
                       cl_mem *inputBuffer,
                       cl_mem *outputBuffer)
{
    cl_int status;
    cl_event events[2];

    size_t globalThreads[1]= {len/2};
    size_t localThreads[1] = {len/2};

    status =  clGetKernelWorkGroupInfo(
        pScanKernel,
        devices[deviceId],
        CL_KERNEL_LOCAL_MEM_SIZE,
        sizeof(cl_ulong),
        &usedLocalMemory,
        NULL);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clGetKernelWorkGroupInfo failed.(usedLocalMemory)"))
        return SDK_FAILURE; 

    if(usedLocalMemory > totalLocalMemory)
    {
        std::cout << "Unsupported: Insufficient local memory on device." 
            << std::endl;
        return SDK_SUCCESS;
    }

    if(localThreads[0] > maxWorkItemSizes[0] ||
        localThreads[0] > maxWorkGroupSize)
    {
        std::cout << "Unsupported: Device does not support"
            "requested number of work items." << std::endl;
        return SDK_SUCCESS;
    }


    /* Set appropriate arguments to the kernel */
    /* 1st argument to the kernel - outputBuffer */
    status = clSetKernelArg(
        pScanKernel, 
        0, 
        sizeof(cl_mem), 
        (void *)outputBuffer);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clSetKernelArg failed. (outputBuffer)"))
        return SDK_FAILURE;

    /* 2nd argument to the kernel - inputBuffer */
    status = clSetKernelArg(
        pScanKernel, 
        1, 
        sizeof(cl_mem), 
        (void *)inputBuffer);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clSetKernelArg failed. (inputBuffer)"))
        return SDK_FAILURE;

    /* 3rd argument to the kernel - local memory */
    status = clSetKernelArg(
        pScanKernel, 
        2, 
        len * sizeof(cl_float), 
        NULL);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clSetKernelArg failed. (local)"))
        return SDK_FAILURE;

    /* 4th argument to the kernel - length */
    status = clSetKernelArg(
        pScanKernel, 
        3, 
        sizeof(cl_int),
        (void*)&len);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clSetKernelArg failed. (length)"))
        return SDK_FAILURE;

    /* Enqueue a kernel run call.*/
    status = clEnqueueNDRangeKernel(
        commandQueue,
        pScanKernel,
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

    return SDK_SUCCESS;
}



int 
ScanLargeArrays::bAddition(cl_uint len,
                           cl_mem *inputBuffer,
                           cl_mem *outputBuffer)
{
    cl_int   status;
    cl_event events[2];

    /* set the block size*/ 
    size_t globalThreads[1]= {len};
    size_t localThreads[1] = {blockSize};

    if(localThreads[0] > maxWorkItemSizes[0] ||
       localThreads[0] > maxWorkGroupSize)
    {
        std::cout<<"Unsupported: Device does not support"
            "requested number of work items.";

        return SDK_FAILURE;
    }

    /*** Set appropriate arguments to the kernel ***/
    /* 1st argument to the kernel - inputBuffer */
    status = clSetKernelArg(
        bAddKernel, 
        0, 
        sizeof(cl_mem), 
        (void*)inputBuffer);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clSetKernelArg failed. (outputBuffer)"))
        return SDK_FAILURE;

    /* 2nd argument to the kernel - outputBuffer */
    status = clSetKernelArg(
        bAddKernel, 
        1, 
        sizeof(cl_mem), 
        (void *)outputBuffer);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clSetKernelArg failed. (inputBuffer)"))
        return SDK_FAILURE;


    status = clGetKernelWorkGroupInfo(bAddKernel,
        devices[deviceId],
        CL_KERNEL_LOCAL_MEM_SIZE,
        sizeof(cl_ulong),
        &usedLocalMemory,
        NULL);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clGetKernelWorkGroupInfo failed.(usedLocalMemory)"))
        return SDK_FAILURE;


    if(usedLocalMemory > totalLocalMemory)
    {
        std::cout << "Unsupported: Insufficient local memory on device." 
            << std::endl;
        return SDK_FAILURE;
    }

    /* Enqueue a kernel run call.*/
    status = clEnqueueNDRangeKernel(
        commandQueue,
        bAddKernel,
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

    return SDK_SUCCESS;
}


int 
ScanLargeArrays::runCLKernels(void)
{
    cl_int status;
    cl_event events[2];

    /* Enqueue Input Buffer */
    status = clEnqueueWriteBuffer(commandQueue,
        inputBuffer,
        1,
        0,
        length * sizeof(cl_float),
        input,
        0,
        0,
        &events[0]);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clEnqueueWriteBuffer failed. (inputBuffer)"))
        return SDK_FAILURE;

    /* Wait for write to finish */
    clFinish(commandQueue);


    /* Do block-wise sum */
    if(bScan(length, &inputBuffer, &outputBuffer[0], &blockSumBuffer[0]))
        return SDK_FAILURE;

    for(int i = 1; i < (int)pass; i++)
    {
        if(bScan((cl_uint)(length / pow((float)blockSize, (float)i)), 
            &blockSumBuffer[i - 1],
            &outputBuffer[i],
            &blockSumBuffer[i]))
        {
            return SDK_FAILURE;		
        }
    }

    int tempLength = (int)(length / pow((float)blockSize, (float)pass));

    /* Do scan to tempBuffer */
    if(pScan(tempLength, &blockSumBuffer[pass - 1], &tempBuffer))
        return SDK_FAILURE;

    /* Do block-addition on outputBuffers */
    if(bAddition((cl_uint)(length / pow((float)blockSize, (float)(pass - 1))),
        &tempBuffer, &outputBuffer[pass - 1]))
    {
        return SDK_FAILURE;
    }

    for(int i = pass - 1; i > 0; i--)
    {
        if(bAddition((cl_uint)(length / pow((float)blockSize, (float)(i - 1))),
            &outputBuffer[i], &outputBuffer[i - 1]))
        {
            return SDK_FAILURE;
        }
    }

    /* Read the final result */
    status = clEnqueueReadBuffer(commandQueue,
        outputBuffer[0],
        1,
        0,
        length * sizeof(cl_float),
        output,
        0,
        0,
        &events[0]);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clReadBuffer failed. (outputBuffer[0])"))
        return SDK_FAILURE;

    clFinish(commandQueue);
    return SDK_SUCCESS;
}

/*
* Naive implementation of Scan 
*/
void 
ScanLargeArrays::scanLargeArraysCPUReference(
    cl_float * output,
    cl_float * input,
    const cl_uint length)
{
    output[0] = 0;

    for(cl_uint i = 1; i < length; ++i)
    {
        output[i] = input[i-1] + output[i-1];
    }
}

int ScanLargeArrays::initialize()
{
    // Call base class Initialize to get default configuration
    if(!this->SDKSample::initialize())
        return SDK_FAILURE;

    streamsdk::Option* array_length = new streamsdk::Option;
    if(!array_length)
    {
        sampleCommon->error("Memory allocation error.\n");
        return SDK_FAILURE;
    }

    array_length->_sVersion = "x";
    array_length->_lVersion = "length";
    array_length->_description = "Length of the input array";
    array_length->_type = streamsdk::CA_ARG_INT;
    array_length->_value = &length;
    sampleArgs->AddOption(array_length);
    delete array_length;

    streamsdk::Option* iteration_option = new streamsdk::Option;
    if(!iteration_option)
    {
        sampleCommon->error("Memory Allocation error.\n");
        return SDK_FAILURE;
    }
    iteration_option->_sVersion = "i";
    iteration_option->_lVersion = "iterations";
    iteration_option->_description = "Number of iterations to execute kernel";
    iteration_option->_type = streamsdk::CA_ARG_INT;
    iteration_option->_value = &iterations;

    sampleArgs->AddOption(iteration_option);
    delete iteration_option;

    return SDK_SUCCESS;
}

int ScanLargeArrays::setup()
{  
    if(!sampleCommon->isPowerOf2(length))
        length = sampleCommon->roundToPowerOf2(length);

    if(setupScanLargeArrays() != SDK_SUCCESS)
        return SDK_FAILURE;

    /* create and initialize timers */
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


int ScanLargeArrays::run()
{
    /* create and initialize timers */
    int timer = sampleCommon->createTimer();
    sampleCommon->resetTimer(timer);
    sampleCommon->startTimer(timer);

    std::cout << "Executing kernel for " << 
        iterations << " iterations" << std::endl;
    std::cout << "-------------------------------------------" << 
        std::endl;

    for(int i = 0; i < iterations; i++)
    {
        /* Arguments are set and execution call is enqueued on command buffer */
        if(runCLKernels() != SDK_SUCCESS)
            return SDK_FAILURE;
    }

    sampleCommon->stopTimer(timer);    
    /* Compute kernel time */
    kernelTime = (double)(sampleCommon->readTimer(timer)) / iterations;

    if(!quiet) {
        sampleCommon->printArray<cl_float>("Output", output, length, 1);
    }

    return SDK_SUCCESS;
}

int ScanLargeArrays::verifyResults()
{
    if(verify)
    {
        /* reference implementation */
        scanLargeArraysCPUReference(verificationOutput, input, length);

        /* compare the results and see if they match */
        if(sampleCommon->compare(output, 
                                 verificationOutput, 
                                 length, 
                                 (float)0.001))
        {
            std::cout << "Passed!\n";
            return SDK_SUCCESS;
        }
        else
        {
            std::cout << "Failed\n";
            return SDK_FAILURE;
        }
    }
    return SDK_SUCCESS;
}

void ScanLargeArrays::printStats()
{
    std::string strArray[3] = 
    {
        "Elements", 
        "Time(sec)", 
        "kernelTime(sec)"
    };
    std::string stats[3];

    totalTime = setupTime + kernelTime;
    stats[0]  = sampleCommon->toString(length, std::dec);
    stats[1]  = sampleCommon->toString(totalTime, std::dec);
    stats[2]  = sampleCommon->toString(kernelTime, std::dec);

    this->SDKSample::printStats(strArray, stats, 3);
}

int 
ScanLargeArrays::cleanup()
{
    /* Releases OpenCL resources (Context, Memory etc.) */
    cl_int status;

    status = clReleaseKernel(pScanKernel);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clReleaseKernel failed. (pScanKernel)"))
        return SDK_FAILURE;

    status = clReleaseKernel(bScanKernel);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clReleaseKernel failed. (bScanKernel)"))
        return SDK_FAILURE;

    status = clReleaseKernel(bAddKernel);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clReleaseKernel failed. (bAddKernel)"))
        return SDK_FAILURE;

    status = clReleaseProgram(program);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clReleaseProgram failed."))
        return SDK_FAILURE;

    status = clReleaseMemObject(inputBuffer);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clReleaseMemObject failed.(inputBuffer)"))
        return SDK_FAILURE;

    status = clReleaseMemObject(tempBuffer);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clReleaseMemObject failed. (tempBuffer)"))
        return SDK_FAILURE;

    for(int i = 0; i < (int)pass; i++)
    {
        status = clReleaseMemObject(outputBuffer[i]);
        if(!sampleCommon->checkVal(
            status,
            CL_SUCCESS,
            "clReleaseMemObject failed. (outputBuffer)"))
            return SDK_FAILURE;

        status = clReleaseMemObject(blockSumBuffer[i]);
        if(!sampleCommon->checkVal(
            status,
            CL_SUCCESS,
            "clReleaseMemObject failed. (blockSumBuffer)"))
            return SDK_FAILURE;

    }

    if(outputBuffer)
        free(outputBuffer);

    if(blockSumBuffer)
        free(blockSumBuffer);

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
    if(input) 
        free(input);

    if(output)
        free(output);

    if(verificationOutput) 
        free(verificationOutput);

    if(devices)
        free(devices);

    if(maxWorkItemSizes)
        free(maxWorkItemSizes);

    return SDK_SUCCESS;
}

int 
main(int argc, char * argv[])
{
    ScanLargeArrays clScanLargeArrays("OpenCL ScanLargeArrays");

    clScanLargeArrays.initialize();
    if(!clScanLargeArrays.parseCommandLine(argc, argv))
        return SDK_FAILURE;

    if(clScanLargeArrays.isDumpBinaryEnabled())
    {
        return clScanLargeArrays.genBinaryImage();
    }
    else
    {
        if(clScanLargeArrays.setup() != SDK_SUCCESS)
            return SDK_FAILURE;
        if(clScanLargeArrays.run() != SDK_SUCCESS)
            return SDK_FAILURE;
        if(clScanLargeArrays.verifyResults() != SDK_SUCCESS)
            return SDK_FAILURE;
        if(clScanLargeArrays.cleanup() != SDK_SUCCESS)
            return SDK_FAILURE;
        clScanLargeArrays.printStats();
    }

    return SDK_SUCCESS;
}
