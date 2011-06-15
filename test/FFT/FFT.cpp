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


#include "FFT.hpp"
#include <malloc.h>


int FFT::setupFFT()
{
    cl_uint inputSizeBytes;

    /* allocate and init memory used by host */
    inputSizeBytes = length * sizeof(cl_float);

#if defined (_WIN32)
    input_i = (cl_float*)_aligned_malloc(inputSizeBytes, 16);
    input_r = (cl_float*)_aligned_malloc(inputSizeBytes, 16);
    output_i = (cl_float*)_aligned_malloc(inputSizeBytes, 16);
    output_r = (cl_float*)_aligned_malloc(inputSizeBytes, 16);
#else
    input_i = (cl_float*)memalign(16, inputSizeBytes);
    input_r = (cl_float*)memalign(16, inputSizeBytes);
    output_i = (cl_float*)memalign(16, inputSizeBytes);
    output_r = (cl_float*)memalign(16, inputSizeBytes);
#endif
    if(input_i == NULL)
    { 
        sampleCommon->error("Failed to allocate host memory. (input_i)");
        return SDK_FAILURE;
    }
    if(input_r == NULL)
    { 
        sampleCommon->error("Failed to allocate host memory. (input_r)");
        return SDK_FAILURE;
    }
    if(output_i == NULL)
    { 
        sampleCommon->error("Failed to allocate host memory. (output_i)");
        return SDK_FAILURE;
    }
    if(output_r == NULL)
    { 
        sampleCommon->error("Failed to allocate host memory. (output_r)");
        return SDK_FAILURE;
    }


    /* 
     * random initialisation of input 
     */
    sampleCommon->fillRandom<cl_float >(input_r, length, 1, 0, 255);
    sampleCommon->fillRandom<cl_float >(input_i, length, 1, 0, 0);

    memcpy(output_i, input_i, inputSizeBytes);
    memcpy(output_r, input_r, inputSizeBytes);

    /* 
     * Unless quiet mode has been enabled, print the INPUT array.
     * No more than 256 values are printed because it clutters the screen
     * and it is not practical to manually compare a large set of numbers
     */
    if(!quiet) 
    {
        sampleCommon->printArray<cl_float >("Original Input Real", 
                                            input_r, 
                                            10, 
                                            1);
        sampleCommon->printArray<cl_float >("Original Input Img", 
                                            input_i, 
                                            10, 
                                            1);
    }

    return SDK_SUCCESS;
}

int 
FFT::genBinaryImage()
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
    kernelPath.append("FFT_Kernels.cl");
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
FFT::setupCL(void)
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

    context = clCreateContextFromType(cps,
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

    /* Get Device specific Information */
    status = clGetDeviceInfo(
            devices[deviceId],
            CL_DEVICE_MAX_WORK_GROUP_SIZE,
            sizeof(size_t),
            (void *)&maxWorkGroupSize,
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
                (void *)&maxDimensions,
                NULL);

    if(!sampleCommon->checkVal(
                status,
                CL_SUCCESS, 
                "clGetDeviceInfo CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS failed."))
        return SDK_FAILURE;


    maxWorkItemSizes = (size_t *)malloc(maxDimensions*sizeof(size_t));
    
    status = clGetDeviceInfo(
                devices[deviceId],
                CL_DEVICE_MAX_WORK_ITEM_SIZES,
                sizeof(size_t)*maxDimensions,
                (void *)maxWorkItemSizes,
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
                        "clGetDeviceInfo CL_DEVICE_LOCAL_MEM_SIZES failed."))
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


    /* Create input real buffer */
    buffer_r = clCreateBuffer(context, 
                              CL_MEM_READ_WRITE,
                              sizeof(cl_float) * length,
                              0, 
                              &status);

    if(!sampleCommon->checkVal(status,
                               CL_SUCCESS,
                               "clCreateBuffer failed. (buffer_r)"))
        return SDK_FAILURE;

    /* Write input real data to input_r buffer */
    status = clEnqueueWriteBuffer(commandQueue,
                                  buffer_r,
                                  1,
                                  0,
                                  sizeof(cl_float)*length,
                                  input_r,
                                  0,
                                  0,
                                  0);
    if(!sampleCommon->checkVal(
                status,
                CL_SUCCESS,
                "clEnqueueWriteBuffer failed. (buffer_r)"))
        return SDK_FAILURE;

    /* Create input imaginary buffer */
    buffer_i = clCreateBuffer(
            context, 
            CL_MEM_READ_WRITE,
            sizeof(cl_float)*length,
            0, 
            &status);
    if(!sampleCommon->checkVal(
                status,
                CL_SUCCESS,
                "clCreateBuffer failed. (buffer_i)"))
        return SDK_FAILURE;

    /* Write input imaginary data to input_r buffer */
    status = clEnqueueWriteBuffer(commandQueue,
                                  buffer_i,
                                  1,
                                  0,
                                  sizeof(cl_float)*length,
                                  input_i,
                                  0,
                                  0,
                                  0);
    if(!sampleCommon->checkVal(
                status,
                CL_SUCCESS,
                "clEnqueueWriteBuffer failed. (buffer_i)"))
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

	kernelPath.append("FFT_Kernels.cl");
	if(!kernelFile.open(kernelPath.c_str()))
	{
		std::cout << "Failed to load kernel file : " << kernelPath << std::endl;
		return SDK_FAILURE;
	}

	const char * source = platformIsPacketizedOpenCL ?
		"FFT_Kernels.bc" :
		kernelFile.source().c_str();

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
    kernel = clCreateKernel(program, "kfft", &status);
    if(!sampleCommon->checkVal(
                status,
                CL_SUCCESS,
                "clCreateKernel failed."))
        return SDK_FAILURE;

    return SDK_SUCCESS;
}


int 
FFT::runCLKernels(void)
{
    cl_int   status;

    /* Kernel Does an inplace FFT */
    status = clEnqueueWriteBuffer(commandQueue,
                                  buffer_r,
                                  1,
                                  0,
                                  sizeof(cl_float)*length,
                                  input_r,
                                  0,
                                  0,
                                  0);
    if(!sampleCommon->checkVal(
                status,
                CL_SUCCESS,
                "clEnqueueWriteBuffer failed. (buffer_r)"))
        return SDK_FAILURE;

    /* Write input imaginary data to input_r buffer */
    status = clEnqueueWriteBuffer(commandQueue,
                                  buffer_i,
                                  1,
                                  0,
                                  sizeof(cl_float)*length,
                                  input_i,
                                  0,
                                  0,
                                  0);
    if(!sampleCommon->checkVal(
                status,
                CL_SUCCESS,
                "clEnqueueWriteBuffer failed. (buffer_i)"))
        return SDK_FAILURE;

    cl_event events[3];

    size_t globalThreads[1];
    size_t localThreads[1];

    globalThreads[0] = 64;
    localThreads[0]  = 64;

    /* Get work-group size supported on this particular kernel */
    status = clGetKernelWorkGroupInfo(kernel, 
                                      devices[deviceId],
                                      CL_KERNEL_WORK_GROUP_SIZE,
                                      sizeof(kernelWorkGroupSize),
                                      &kernelWorkGroupSize,
                                      0);
    if(!sampleCommon->checkVal(
                status,
                CL_SUCCESS,
                "clGetKernelWorkGroupInfo failed."))
        return SDK_FAILURE;

    if(kernelWorkGroupSize < localThreads[0])
    {
        if(!quiet)
        {
            std::cout << "Error : Out of resources!\n";
            std::cout << "Work-group size required by kernel : 64\n";
            std::cout << "Maximum size supported on GPU for this kernel : "
                      << kernelWorkGroupSize << std::endl;
        }
        return SDK_EXPECTED_FAILURE;
    }

    /* 1st argument*/
    status = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&buffer_r);
    if(!sampleCommon->checkVal(
                status,
                CL_SUCCESS,
                "clSetKernelArg failed. (buffer_r)"))
        return SDK_FAILURE;

    /* 2nd argument*/
    status = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&buffer_i);
    if(!sampleCommon->checkVal(
                status,
                CL_SUCCESS,
                "clSetKernelArg failed. (buffer_i)"))
        return SDK_FAILURE;

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

    /* Read real output data */
    status = clEnqueueReadBuffer(
             commandQueue,
             buffer_i,
             CL_TRUE,
             0,
             length * sizeof(cl_float),
             output_i,
             0,
             NULL,
             &events[1]);
    /* Read imaginary output data */
    status = clEnqueueReadBuffer(
             commandQueue,
             buffer_r,
             CL_TRUE,
             0,
             length * sizeof(cl_float),
             output_r,
             0,
             NULL,
             &events[2]);

    /* Wait for the read buffer to finish execution */
    status = clWaitForEvents(1, &events[1]);
    status |= clWaitForEvents(1, &events[2]);
    if(!sampleCommon->checkVal(
            status,
            CL_SUCCESS,
            "clWaitForEvents failed."))
        return SDK_FAILURE;

    status = clReleaseEvent(events[0]);
    if(!sampleCommon->checkVal(
            status,
            CL_SUCCESS,
            "clReleaseEvent failed. (events[0])"))
        return SDK_FAILURE;

    status = clReleaseEvent(events[1]);
    if(!sampleCommon->checkVal(
            status,
            CL_SUCCESS,
            "clReleaseEvent failed. (events[1])"))
        return SDK_FAILURE;

    status = clReleaseEvent(events[2]);
    if(!sampleCommon->checkVal(
            status,
            CL_SUCCESS,
            "clReleaseEvent failed. (events[2])"))
        return SDK_FAILURE;

    return SDK_SUCCESS;
}

/*
   This computes an in-place complex-to-complex FFT 
   x and y are the real and imaginary arrays of 2^m points.
   dir =  1 gives forward transform
   dir = -1 gives reverse transform 
*/
void fftCPU(short int dir,long m,cl_float *x,cl_float *y)
{
   long n,i,i1,j,k,i2,l,l1,l2;
   double c1,c2,tx,ty,t1,t2,u1,u2,z;

   /* Calculate the number of points */
   n = 1;
   for (i=0;i<m;i++) 
      n *= 2;

   /* Do the bit reversal */
   i2 = n >> 1;
   j = 0;
   for (i=0;i<n-1;i++) 
   {
      if (i < j) 
      {
         tx = x[i];
         ty = y[i];
         x[i] = x[j];
         y[i] = y[j];
         x[j] = (cl_float)tx;
         y[j] = (cl_float)ty;
      }
      k = i2;
      while (k <= j)
      {
         j -= k;
         k >>= 1;
      }
      j += k;
   }

   /* Compute the FFT */
   c1 = -1.0; 
   c2 = 0.0;
   l2 = 1;
   for (l=0;l<m;l++) 
   {
      l1 = l2;
      l2 <<= 1;
      u1 = 1.0; 
      u2 = 0.0;
      for (j=0;j<l1;j++)
      {
         for (i=j;i<n;i+=l2) 
         {
            i1 = i + l1;
            t1 = u1 * x[i1] - u2 * y[i1];
            t2 = u1 * y[i1] + u2 * x[i1];
            x[i1] = (cl_float)(x[i] - t1); 
            y[i1] = (cl_float)(y[i] - t2);
            x[i] += (cl_float)t1;
            y[i] += (cl_float)t2;
         }
         z =  u1 * c1 - u2 * c2;
         u2 = u1 * c2 + u2 * c1;
         u1 = z;
      }
      c2 = sqrt((1.0 - c1) / 2.0);
      if (dir == 1) 
         c2 = -c2;
      c1 = sqrt((1.0 + c1) / 2.0);
   }

   /* Scaling for forward transform */
   /*if (dir == 1) {
      for (i=0;i<n;i++) {
         x[i] /= n;
         y[i] /= n;
      }
   }*/
   
}

/**
 * Reference CPU implementation of FFT Convolution 
 * for performance comparison
 */
void 
FFT::fftCPUReference(cl_float *referenceReal,
                     cl_float *referenceImaginary,
                     cl_float *input_r,
                     cl_float *input_i,
                     cl_uint  w)
{
    /* Copy data from input to reference buffers */
    memcpy(referenceReal, input_r, w * sizeof(cl_float));
    memcpy(referenceImaginary, input_i, w * sizeof(cl_float));
    /* Compute reference FFT */
    fftCPU(1, 10, referenceReal, referenceImaginary);
}



int FFT::initialize()
{
   // Call base class Initialize to get default configuration
   if(!this->SDKSample::initialize())
      return SDK_FAILURE;

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

int FFT::setup()
{
   if(setupFFT()!=SDK_SUCCESS)
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


int FFT::run()
{
    /* Arguments are set and execution call is enqueued on command buffer */
    /* create and initialize timers */
    int timer = sampleCommon->createTimer();
    sampleCommon->resetTimer(timer);
    sampleCommon->startTimer(timer);

    std::cout << "Executing kernel for " << iterations 
              << " iterations" << std::endl;
    std::cout << "-------------------------------------------" << std::endl;

    for(int i = 0; i < iterations; i++)
    {
        int rValue = runCLKernels();
        if(rValue != SDK_SUCCESS)
        {
            if(rValue == SDK_EXPECTED_FAILURE)
                return SDK_EXPECTED_FAILURE;
            else
                return SDK_FAILURE;
        }
    }

    sampleCommon->stopTimer(timer);    
    /* Compute kernel time */
    kernelTime = (double)(sampleCommon->readTimer(timer)) / iterations;


    if(!quiet)
    {
        sampleCommon->printArray<cl_float >("Output real", output_r, 10, 1);
        sampleCommon->printArray<cl_float >("Output img", output_i, 10, 1);
    }

    return SDK_SUCCESS;
}

int FFT::verifyResults()
{
    if(verify)
    {
        verificationOutput_i = (cl_float *)malloc(length * sizeof(cl_float));
        verificationOutput_r = (cl_float *)malloc(length * sizeof(cl_float));

        if(!verificationOutput_i || !verificationOutput_r)   
        { 
            sampleCommon->error("Failed to allocate host memory"
                                "(verificationOutput)");
            return SDK_FAILURE;
        }

        /* Compute reference FFT on input */
        fftCPUReference(verificationOutput_r, 
                        verificationOutput_i, 
                        input_r, 
                        input_i, 
                        length);

		if(!quiet)
			sampleCommon->printArray<cl_float >("verification Output img", 
												verificationOutput_i, 
												10, 
												1);

        /* Compare results */
        if(sampleCommon->compare(verificationOutput_i, output_i, length, 1e-4f))
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

void FFT::printStats()
{
    std::string strArray[3] = {"Length", "Time(sec)", "kernelTime(sec)"};
    std::string stats[3];

    totalTime = setupTime + kernelTime;

    stats[0] = sampleCommon->toString(length   , std::dec);
    stats[1] = sampleCommon->toString(totalTime, std::dec);
	stats[2] = sampleCommon->toString(kernelTime, std::dec);
    
    this->SDKSample::printStats(strArray, stats, 3);
}

int FFT::cleanup()
{
    /* Releases OpenCL resources (Context, Memory etc.) */
    cl_int status;

    status = clReleaseMemObject(buffer_i);
   if(!sampleCommon->checkVal(
      status,
      CL_SUCCESS,
      "clReleaseMemObject failed."))
      return SDK_FAILURE;

   status = clReleaseMemObject(buffer_r);
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

    /* release program resources (input memory etc.) */
    if(input_i) 
    {
#if defined (_WIN32)
        _aligned_free(input_i);
#else
        free(input_i);
#endif
    }

    if(input_r) 
    {
#if defined (_WIN32)
        _aligned_free(input_r);
#else
        free(input_r);
#endif
    }

   if(output_i) 
   {
#if defined (_WIN32)
        _aligned_free(output_i);
#else
        free(output_i);
#endif
   }

   if(output_r)
   {
#if defined (_WIN32)
        _aligned_free(output_r);
#else
        free(output_r);
#endif
   }

   if(verificationOutput_i) 
        free(verificationOutput_i);

   if(verificationOutput_r) 
        free(verificationOutput_r);

    if(devices)
        free(devices);

    if(maxWorkItemSizes)
        free(maxWorkItemSizes);

   return SDK_SUCCESS;
}

int 
main(int argc, char * argv[])
{
    FFT clFFT("OpenCL FFT ");

    if(clFFT.initialize()!=SDK_SUCCESS)
        return SDK_FAILURE;

    if(!clFFT.parseCommandLine(argc, argv))
        return SDK_FAILURE;

    if(clFFT.isDumpBinaryEnabled())
    {
        return clFFT.genBinaryImage();
    }
    else
    {
        if(clFFT.setup()!=SDK_SUCCESS)
            return SDK_FAILURE;

        int state = clFFT.run();

        if(state == SDK_FAILURE)
            return SDK_FAILURE;

        else if(state == SDK_SUCCESS)
        {
            if(clFFT.verifyResults() == SDK_FAILURE)
                return SDK_FAILURE;
        }

        if(clFFT.cleanup() != SDK_SUCCESS)
            return SDK_FAILURE;

        clFFT.printStats();
    }

    return SDK_SUCCESS;
}
