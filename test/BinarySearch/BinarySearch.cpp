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


#include "BinarySearch.hpp"
#if !defined __APPLE__
#	include <malloc.h>
#endif

/* 
 * \brief set up program input data 
 */
int BinarySearch::setupBinarySearch()
{
	/* allocate and init memory used by host */
    cl_uint inputSizeBytes = length *  sizeof(cl_uint);
    input = (cl_uint *) malloc(inputSizeBytes);
    if(input==NULL)    
    { 
        sampleCommon->error("Failed to allocate host memory. (input)");
        return SDK_FAILURE;
    }

    cl_uint max = length * 20;
    /* random initialisation of input */
    input[0] = 0;
    for(cl_uint i = 1; i < length; i++)
        input[i] = input[i-1] + (cl_uint) (max * rand()/(float)RAND_MAX);
   
#if defined (_WIN32)
    output = (cl_uint *)_aligned_malloc(sizeof(cl_uint4), 16);
#elif defined (__APPLE__)
    output = (cl_uint *)malloc(sizeof(cl_uint4));
#else
    output = (cl_uint *)memalign(16, sizeof(cl_uint4));
#endif 

    if(output==NULL)    
    { 
        sampleCommon->error("Failed to allocate host memory. (output)");
        return SDK_FAILURE;
    }

    /* 
     * Unless quiet mode has been enabled, print the INPUT array.
     */
    if(!quiet) 
    {
        sampleCommon->printArray<cl_uint>(
            "Sorted Input", 
            input, 
            length, 
            1);
    }

    return SDK_SUCCESS;
}


int 
BinarySearch::genBinaryImage()
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
    kernelPath.append("BinarySearch_Kernels.cl");
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


/*
 * \brief OpenCL related initialisations are done here.
 *        Context, Device list, Command Queue are set up.
 *        Calls are made to set up OpenCL memory buffers that this program uses
 *        and to load the programs into memory and get kernel handles.
 *          Load and build OpenCL program and get kernel handles.
 *        Set up OpenCL memory buffers used by this program.
 */

int
BinarySearch::setupCL(void)
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
    devices = (cl_device_id *)malloc(deviceListSize);
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
            "clGetGetContextInfo failed."))
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

    inputBuffer = clCreateBuffer(
                      context, 
                      CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
                      sizeof(cl_uint) * length,
                      input, 
                      &status);
    if(!sampleCommon->checkVal(
            status,
            CL_SUCCESS,
            "clCreateBuffer failed. (inputBuffer)"))
        return SDK_FAILURE;

    outputBuffer = clCreateBuffer(
                      context, 
                      CL_MEM_WRITE_ONLY | CL_MEM_USE_HOST_PTR,
                      sizeof(cl_uint4),
                      output, 
                      &status);
    if(!sampleCommon->checkVal(
            status,
            CL_SUCCESS,
            "clCreateBuffer failed. (outputBuffer)"))
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
                                            (const size_t*)&binarySize,
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

		kernelPath.append("BinarySearch_Kernels.cl");
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
    status = clBuildProgram(program, 1, &devices[deviceId], flagsStr.c_str(), NULL, NULL);
    if(status != CL_SUCCESS)
    {
        if(status == CL_BUILD_PROGRAM_FAILURE)
        {
            cl_int logStatus;
            char * buildLog = NULL;
            size_t buildLogSize = 0;
            logStatus = clGetProgramBuildInfo(program,
                                              devices[deviceId],
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
                                              devices[deviceId], 
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
    kernel = clCreateKernel(program, "binarySearch", &status);
    if(!sampleCommon->checkVal(
            status,
            CL_SUCCESS,
            "clCreateKernel failed."))
        return SDK_FAILURE;

    return SDK_SUCCESS;
}

int 
BinarySearch::runCLKernels(void)
{
    cl_int   status;
    cl_event events[2];

    size_t globalThreads[1];
    size_t localThreads[1];

	localThreads[0] = 256;

	numSubdivisions = length / (cl_uint)localThreads[0];
	if(numSubdivisions < localThreads[0])
	{
		numSubdivisions = (cl_uint)localThreads[0];
	}
	globalThreads[0] = numSubdivisions;

	/* Check group size against kernelWorkGroupSize */
    status = clGetKernelWorkGroupInfo(kernel,
                                      devices[deviceId],
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
		if(!quiet)
		{
			std::cout << "Out of Resources!" << std::endl;
			std::cout << "Group Size specified : " << localThreads[0] << std::endl;
			std::cout << "Max Group Size supported on the kernel : " 
				<< kernelWorkGroupSize << std::endl;
			std::cout << "Changing the group size to " 
                << kernelWorkGroupSize << std::endl;
		}

        localThreads[0] = kernelWorkGroupSize;
		numSubdivisions = length / (cl_uint)localThreads[0];
		if(numSubdivisions < localThreads[0])
		{
			numSubdivisions = (cl_uint)localThreads[0];
		}
		globalThreads[0] = numSubdivisions;
    }

	//return SDK_SUCCESS;

    /**
     * Since a plain binary search on the GPU would not achieve much benefit over the GPU
     * we are doing an N'ary search. We split the array into N segments every pass and therefore
     * get log (base N) passes instead of log (base 2) passes.
     *
     * In every pass, only the thread that can potentially have the element we are looking for
     * writes to the output array. For ex: if we are looking to find 4567 in the array and every
     * thread is searching over a segment of 1000 values and the input array is 1, 2, 3, 4,...
     * then the first thread is searching in 1 to 1000, the second one from 1001 to 2000, etc.
     * The first one does not write to the output. The second one doesn't either. The fifth one however is from 
     * 4001 to 5000. So it can potentially have the element 4567 which lies between them.
     *
     * This particular thread writes to the output the lower bound, upper bound and whether the element equals the lower bound element.
     * So, it would be 4001, 5000, 0
     *
     * The next pass would subdivide 4001 to 5000 into smaller segments and continue the same process from there.
     *
     * When a pass returns 1 in the third element, it means the element has been found and we can stop executing the kernel.
     * If the element is not found, then the execution stops after looking at segment of size 1.
     */
    cl_uint globalLowerBound = 0;
    cl_uint globalUpperBound = length - 1;
    cl_uint subdivSize = (globalUpperBound - globalLowerBound + 1)/numSubdivisions;
    cl_uint isElementFound = 0;


    if((input[0] > findMe) || (input[length-1] < findMe))
    {
        output[0] = 0;
        output[1] = length - 1;
        output[2] = 0;

        return SDK_SUCCESS;
    }
    output[3] = 1;

    /*** Set appropruiate arguments to the kernel ***/
    /*
     * First argument of the kernel is the output buffer
     */
    status = clSetKernelArg(
             kernel, 
             0, 
             sizeof(cl_mem), 
            (void *)&outputBuffer);
        if(!sampleCommon->checkVal(
                    status,
                    CL_SUCCESS,
                    "clSetKernelArg 0(OutputBuffer) failed."))
                return SDK_FAILURE;

    /*
     * Second argument is input buffer
     */
    status = clSetKernelArg(
                    kernel, 
                    1, 
                    sizeof(cl_mem), 
                    (void *)&inputBuffer);
        if(!sampleCommon->checkVal(
                    status,
                    CL_SUCCESS,
                    "clSetKernelArg 1(inputBuffer) failed."))
                return SDK_FAILURE;

    /*
     * Third is the element we are looking for
     */
    status = clSetKernelArg(
                    kernel, 
                    2, 
                    sizeof(cl_uint), 
                    (void *)&findMe);
        if(!sampleCommon->checkVal(
                    status,
                    CL_SUCCESS,
                    "clSetKernelArg 2(findMe) failed."))
                return SDK_FAILURE;

    while(subdivSize > 1 && output[3] != 0)
    {
        output[3] = 0;

        /* Enqueue readBuffer*/
        status = clEnqueueWriteBuffer(
                    commandQueue,
                    outputBuffer,
                    CL_TRUE,
                    0,
                    sizeof(cl_uint4),
                    output,
                    0,
                    NULL,
                    &events[1]);
        if(!sampleCommon->checkVal(
        		status,
        		CL_SUCCESS,
        		"clEnqueueWriteBuffer failed."))
        	return SDK_FAILURE;

        /* Wait for the write buffer to finish execution */
        status = clWaitForEvents(1, &events[1]);
        if(!sampleCommon->checkVal(
        		status,
        		CL_SUCCESS,
        		"clWaitForEvents failed."))
        	return SDK_FAILURE;

        clReleaseEvent(events[1]);


        /*
         * Fourth argument is the lower bound for the full segment for this pass.
         * Each thread derives its own lower and upper bound from this.
         */
        status = clSetKernelArg(
                            kernel, 
                            3, 
                            sizeof(cl_uint), 
                            (void *)&globalLowerBound);
        if(!sampleCommon->checkVal(
                    status,
                    CL_SUCCESS,
                    "clSetKernelArg 3(globalLowerBound) failed."))
                return SDK_FAILURE;

        /*
         * Similar to the above, but it is the upper bound
         */
        status = clSetKernelArg(
                            kernel, 
                            4, 
                            sizeof(cl_uint), 
                            (void *)&globalUpperBound);
        if(!sampleCommon->checkVal(
                    status,
                    CL_SUCCESS,
                    "clSetKernelArg 4(globalUpperBound) failed."))
                return SDK_FAILURE;

        /*
         * The size of the subdivision for each thread
         */
        status = clSetKernelArg(
                            kernel, 
                            5, 
                            sizeof(cl_uint), 
                            (void *)&subdivSize);
        if(!sampleCommon->checkVal(
                    status,
                    CL_SUCCESS,
                    "clSetKernelArg 5(sumdivSize) failed."))
                return SDK_FAILURE;

        /*
         * Enqueue a kernel run call
         */
        status = clEnqueueNDRangeKernel(commandQueue,
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
                    sizeof(cl_uint4),
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

        globalLowerBound = output[0];
        globalUpperBound = output[1];
        subdivSize = (globalUpperBound - globalLowerBound + 1)/numSubdivisions;

    }

   
    for(cl_uint i=globalLowerBound; i<= globalUpperBound; i++)
    {
        if(input[i] == findMe)
        {
            output[0] = i;
            output[1] = i+1;
            output[2] = 1;
			return SDK_SUCCESS;
        }
    }

    /* The findMe element is not found from globalLowerBound to globalUpperBound */
    output[2] = 0;
    return SDK_SUCCESS;
}


/**
 * CPU verification for the BinarySearch algorithm
 */
int 
BinarySearch::binarySearchCPUReference() 
{
    cl_uint globalLowerBound = output[0];
    cl_uint globalUpperBound = output[1];
    cl_uint isElementFound = output[2];

    if(isElementFound)
    {
        if(input[globalLowerBound] == findMe)
            return 1;
        else
            return 0;
    }
    else
    {
        for(cl_uint i=0; i< length; i++)
        {
            if(input[i] == findMe)
                return 0;
        }
        return 1;
    }
}

int BinarySearch::initialize()
{
    /*Call base class Initialize to get default configuration*/
    if(!this->SDKSample::initialize())
        return SDK_FAILURE;

    /* Now add customized options */
    streamsdk::Option* array_length = new streamsdk::Option;
    if(!array_length)
    {
        sampleCommon->error("Memory allocation error.\n");
        return SDK_FAILURE;
    }
    
    array_length->_sVersion = "x";
    array_length->_lVersion = "length";
    array_length->_description = "Lenght of the input array";
    array_length->_type = streamsdk::CA_ARG_INT;
    array_length->_value = &length;

    sampleArgs->AddOption(array_length);

    delete array_length;

    streamsdk::Option* find_me = new streamsdk::Option;
    if(!find_me)
    {
        sampleCommon->error("Memory allocation error.\n");
        return SDK_FAILURE;
    }
    find_me->_sVersion = "f";
    find_me->_lVersion = "find";
    find_me->_description = "element to be found";
    find_me->_type = streamsdk::CA_ARG_INT;
    find_me->_value = &findMe;
    sampleArgs->AddOption(find_me);

    delete find_me;

    streamsdk::Option* sub_div = new streamsdk::Option;
    if(!sub_div)
    {
        sampleCommon->error("Memory allocation error.\n");
        return SDK_FAILURE;
    }
    sub_div->_sVersion = "s";
    sub_div->_lVersion = "subdivisions";
    sub_div->_description = "number of subdivisions";
    sub_div->_type = streamsdk::CA_ARG_INT;
    sub_div->_value = &numSubdivisions;
    sampleArgs->AddOption(sub_div);

    delete sub_div;

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

int BinarySearch::setup()
{
    if(!sampleCommon->isPowerOf2(length))
        length = sampleCommon->roundToPowerOf2(length);

    if(setupBinarySearch()!=SDK_SUCCESS)
        return SDK_FAILURE;

    int timer = sampleCommon->createTimer();
    sampleCommon->resetTimer(timer);
    sampleCommon->startTimer(timer); 

    if(setupCL()!=SDK_SUCCESS)
        return SDK_FAILURE;

    setupTime = (cl_double)(sampleCommon->readTimer(timer));

    return SDK_SUCCESS;
}


int BinarySearch::run()
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
        cl_uint globalLowerBound = output[0];
        cl_uint globalUpperBound = output[1];
        cl_uint isElementFound = output[2];

        printf("l = %d, u = %d, isfound = %d, fm = %d\n",
                globalLowerBound, globalUpperBound, isElementFound, findMe);
    }

    return SDK_SUCCESS;
}

int BinarySearch::verifyResults()
{
    if(verify)
    {
        verificationInput = (cl_uint *) malloc(length*sizeof(cl_int));
        if(verificationInput==NULL)    { 
            sampleCommon->error("Failed to allocate host memory. (verificationInput)");
                return SDK_FAILURE;
        }
        memcpy(verificationInput, input, length*sizeof(cl_int));

        /* reference implementation
         * it overwrites the input array with the output
         */
        int refTimer = sampleCommon->createTimer();
        sampleCommon->resetTimer(refTimer);
        sampleCommon->startTimer(refTimer);
        cl_int verified = binarySearchCPUReference();
        sampleCommon->stopTimer(refTimer);
        referenceKernelTime = sampleCommon->readTimer(refTimer);

        /* compare the results and see if they match */
        if(verified)
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

void BinarySearch::printStats()
{
    std::string strArray[3] = {"Length", "Time(sec)", "kernelTime(sec)"};
    std::string stats[3];

    totalTime = setupTime + totalKernelTime;

    stats[0] = sampleCommon->toString(length   , std::dec);
    stats[1] = sampleCommon->toString(totalTime, std::dec);
	stats[2] = sampleCommon->toString(totalKernelTime, std::dec);
    
    this->SDKSample::printStats(strArray, stats, 3);
    this->SDKSample::logStats(totalKernelTime, "BinarySearch", vendorName);
}

int BinarySearch::cleanup()
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
   
    status = clReleaseMemObject(inputBuffer);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clReleaseMemObject failed."))
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
    if(input) 
        free(input);


    if(output)
    {
#if defined (_WIN32)
        _aligned_free(output);
#else
        free(output);
#endif 
    }


    if(devices)
        free(devices);

    if(verificationInput) 
        free(verificationInput);

    return SDK_SUCCESS;
}

int 
main(int argc, char * argv[])
{
    BinarySearch clBinarySearch("OpenCL Binary Search");

    if(clBinarySearch.initialize()!=SDK_SUCCESS)
		return SDK_FAILURE;
    if(!clBinarySearch.parseCommandLine(argc, argv))
        return SDK_FAILURE;

    if(clBinarySearch.isDumpBinaryEnabled())
    {
        return clBinarySearch.genBinaryImage();
    }
    else
    {
        if(clBinarySearch.setup()!=SDK_SUCCESS)
		    return SDK_FAILURE;
        if(clBinarySearch.run()!=SDK_SUCCESS)
		    return SDK_FAILURE;
        if(clBinarySearch.verifyResults()!=SDK_SUCCESS)
		    return SDK_FAILURE;
        if(clBinarySearch.cleanup()!=SDK_SUCCESS)
		    return SDK_FAILURE;
        clBinarySearch.printStats();
    }

    return SDK_SUCCESS;
}
