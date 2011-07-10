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


#include "MatrixMulImage.hpp"

int 
MatrixMulImage::setupMatrixMulImage()
{
    /* allocate and init memory used by host  input0[width0][height0]*/
    cl_uint inputSizeBytes0 = width0 * height0 * sizeof(cl_float);

    input0 = (cl_float *) malloc(inputSizeBytes0);
    if(input0==NULL)    
    { 
        sampleCommon->error("Failed to allocate host memory. (input0)");
        return SDK_FAILURE;
    }

    /* allocate and init memory used by host input1[width1][height1]*/
    cl_uint inputSizeBytes1 = width1 * height1 * sizeof(cl_float);

    input1 = (cl_float *) malloc(inputSizeBytes1);
    if(input1==NULL)    
    { 
        sampleCommon->error("Failed to allocate host memory. (input1)");
        return SDK_FAILURE;
    }
    
    /* random initialisation of input */
    sampleCommon->fillRandom<cl_float>(input0, width0, height0, 0, 10);
    sampleCommon->fillRandom<cl_float>(input1, width1, height1, 0, 10);

    /* allocate memory for output[width1][height0] */
    cl_uint outputSizeBytes = height0 * width1 * sizeof(cl_float);

    output = (cl_float *) malloc(outputSizeBytes);
    if(output==NULL)    
    { 
        sampleCommon->error("Failed to allocate host memory. (output)");
        return SDK_FAILURE;
    }

    /* allocate memory for output[width1][height0] of reference implemenation*/
    if(verify)
    {
        verificationOutput = (cl_float *) malloc(outputSizeBytes);
        if(verificationOutput==NULL)    { 
            sampleCommon->error("Failed to allocate host memory. (verificationOutput)");
                return SDK_FAILURE;
        }
        memset(verificationOutput, 0, outputSizeBytes);
    }
    /* 
     * Unless quiet mode has been enabled, print the INPUT arrays
     */
    if(!quiet) 
    {
        sampleCommon->printArray<cl_float>(
            "Input0", 
            input0, 
            width0, 
            1);
        sampleCommon->printArray<cl_float>(
            "Input1", 
            input1, 
            width1, 
            1);

    }

    return SDK_SUCCESS;
}

int 
MatrixMulImage::genBinaryImage()
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
    kernelPath.append("MatrixMulImage_Kernels.cl");
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
MatrixMulImage::setupCL(void)
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

    /* Check for image support */
    status = clGetDeviceInfo(devices[deviceId],
                             CL_DEVICE_IMAGE_SUPPORT,
                             sizeof(cl_bool),
                             &imageSupport,
                             0);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS, 
        "clGetDeviceInfo failed."))
        return SDK_FAILURE;

    if(!imageSupport)
    {
        sampleCommon->expectedError("Images are not supported on this device!");
        return SDK_EXPECTED_FAILURE;
    }


    {
        /* The block is to move the declaration of prop closer to its use */
        cl_command_queue_properties prop = 0;
        prop |= CL_QUEUE_PROFILING_ENABLE;

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

    cl_image_format imageFormat;
    imageFormat.image_channel_data_type = CL_FLOAT;
    imageFormat.image_channel_order = CL_RGBA;

    /* Create image for matrix A */
    inputBuffer0 = clCreateImage2D(context,
                                   CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
                                   &imageFormat,
                                   width0 / 4,
                                   height0,
                                   0,
                                   input0,
                                   &status);
    if(!sampleCommon->checkVal(
            status,
            CL_SUCCESS,
            "clCreateImage2D failed. (inputBuffer0)"))
        return SDK_FAILURE;


    /* Create image for matrix B */
    inputBuffer1 = clCreateImage2D(context,
                                   CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
                                   &imageFormat,
                                   width1 / 4,
                                   height1,
                                   0,
                                   input1,
                                   &status);
    if(!sampleCommon->checkVal(
            status,
            CL_SUCCESS,
            "clCreateImage2D failed. (inputBuffer1)"))
        return SDK_FAILURE;

    /* Create image for matrix C */
    outputBuffer = clCreateImage2D(context,
                                   CL_MEM_WRITE_ONLY,
                                   &imageFormat,
                                   width1 / 4,
                                   height0,
                                   0,
                                   0,
                                   &status);
    if(!sampleCommon->checkVal(
            status,
            CL_SUCCESS,
            "clCreateImage2D failed. (outputBuffer)"))
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

		kernelPath.append("MatrixMulImage_Kernels.cl");
		if(!kernelFile.open(kernelPath.c_str()))
		{
			std::cout << "Failed to load kernel file : " << kernelPath << std::endl;
			return SDK_FAILURE;
		}

		const char * source = platformIsPacketizedOpenCL ?
			"MatrixMulImage_Kernels.bc" :
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
            char *buildLog = NULL;
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

    kernel = clCreateKernel(program, "mmmKernel3", &status);
    if(!sampleCommon->checkVal(
            status,
            CL_SUCCESS,
            "clCreateKernel failed."))
        return SDK_FAILURE;

    return SDK_SUCCESS;
}


int 
MatrixMulImage::runCLKernels(void)
{
    cl_int   status;
    cl_event events[2];

    /* 
     * Kernel runs over complete output matrix with blocks of blockSize x blockSize 
     * running concurrently
     */
    size_t globalThreads[2]= {width1 / 4, height0 / 8};
    size_t localThreads[2] = {blockSize, blockSize};

    status =  clGetKernelWorkGroupInfo(
                    kernel,
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

    availableLocalMemory = totalLocalMemory - usedLocalMemory;

    neededLocalMemory    = 2*blockSize*blockSize*sizeof(cl_float); 

    if(neededLocalMemory > availableLocalMemory)
    {
        std::cout << "Unsupported: Insufficient local memory on device." << std::endl;
        return SDK_SUCCESS;
    }


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

    if((cl_uint)(localThreads[0]*localThreads[1]) > kernelWorkGroupSize)
    {
       if(kernelWorkGroupSize >= 64)
        {
            blockSize = 8; 
            localThreads[0] = blockSize;
            localThreads[1] = blockSize;
        }
        else if(kernelWorkGroupSize >= 32)
        {
            blockSize = 4; 
            localThreads[0] = blockSize;
            localThreads[1] = blockSize;
        }
        else
        {
            std::cout << "Out of Resources!" << std::endl;
            std::cout << "Group Size specified : " << localThreads[0] * localThreads[1] << std::endl;
            std::cout << "Max Group Size supported on the kernel : " 
                      << kernelWorkGroupSize<<std::endl;
            return SDK_FAILURE;
        }
    }

    if(localThreads[0] > maxWorkItemSizes[0] ||
       localThreads[1] > maxWorkItemSizes[1] ||
       localThreads[0]*localThreads[1] > maxWorkGroupSize)
    {
        std::cout << "Unsupported: Device does not support requested number of work items." << std::endl;
        return SDK_FAILURE;
    }

    /*** Set appropriate arguments to the kernel ***/
    
    status = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&inputBuffer0);
    if(!sampleCommon->checkVal(
            status,
            CL_SUCCESS,
            "clSetKernelArg failed. (outputBuffer)"))
        return SDK_FAILURE;

    status = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&inputBuffer1);
    if(!sampleCommon->checkVal(
            status,
            CL_SUCCESS,
            "clSetKernelArg failed. (inputBuffer0)"))
        return SDK_FAILURE;

    status = clSetKernelArg(kernel, 2, sizeof(cl_mem), (void *)&outputBuffer);
    if(!sampleCommon->checkVal(
            status,
            CL_SUCCESS,
            "clSetKernelArg failed. (inputBuffer1)"))
        return SDK_FAILURE;

    status = clSetKernelArg(kernel, 3, sizeof(cl_int),(void*)&width0);
    if(!sampleCommon->checkVal(
            status,
            CL_SUCCESS,
            "clSetKernelArg failed. (width0)"))
        return SDK_FAILURE;

    status = clSetKernelArg(kernel, 4, sizeof(cl_int), &width1);
    if(!sampleCommon->checkVal(
            status,
            CL_SUCCESS,
            "clSetKernelArg failed. (width1)"))
        return SDK_FAILURE;

    /*Enqueue a kernel run call */
    status = clEnqueueNDRangeKernel(
                 commandQueue,
                 kernel,
                 2,
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


    /* Calculate performance */
    cl_ulong startTime;
    cl_ulong endTime;
    
    /* Get kernel profiling info */
    status = clGetEventProfilingInfo(events[0],
                                     CL_PROFILING_COMMAND_START,
                                     sizeof(cl_ulong),
                                     &startTime,
                                     0);
    if(!sampleCommon->checkVal(
            status,
            CL_SUCCESS,
            "clGetEventProfilingInfo failed.(startTime)"))
        return SDK_FAILURE;


    status = clGetEventProfilingInfo(events[0],
                                     CL_PROFILING_COMMAND_END,
                                     sizeof(cl_ulong),
                                     &endTime,
                                     0);

    if(!sampleCommon->checkVal(
            status,
            CL_SUCCESS,
            "clGetEventProfilingInfo failed.(endTime)"))
        return SDK_FAILURE;

    /* Print performance numbers */
    double sec = 1e-9 * (endTime - startTime);
    std::cout << "KernelTime (ms) : " << sec * 1000 << std::endl;

    double flops = 2 * width0 * width1;
    double perf = (flops / sec) * height0 * 1e-9;
    
    std::cout << "GFlops achieved : " << perf << std::endl << std::endl;

    size_t origin[] = {0, 0, 0};
    size_t region[] = {width1 / 4, height0, 1};
    status = clEnqueueReadImage(commandQueue,
                                outputBuffer,
                                1,
                                origin,
                                region,
                                0,
                                0,
                                output,
                                0,
                                0,
                                0);
    if(!sampleCommon->checkVal(
    		status,
    		CL_SUCCESS,
    		"outputBuffer failed."))
    	return SDK_FAILURE;


    return SDK_SUCCESS;
}

/*
 * This is a naive O(N^3) CPU implementatio of matrix multiplication
 */
void 
MatrixMulImage::MatrixMulImageCPUReference(
    cl_float * output,
    cl_float * input0,
    cl_float * input1,
    const cl_uint y,
    const cl_uint x,
    const cl_uint z)
{
    for(cl_uint i=0; i < y; i++)
    {
        for(cl_uint j=0; j < z; j++)
        {
            for(cl_uint k=0; k < x; k++)
            {
                output[i*z + j] += (input0[i*x + k]*input1[k*z + j]);
            }
        }
    }
}

int 
MatrixMulImage::initialize()
{
    /* Call base class Initialize to get default configuration */
    if(!this->SDKSample::initialize())
        return SDK_FAILURE;

    /* add an option for getting blockSize from commandline */
    streamsdk::Option* xParam = new streamsdk::Option;
    if(!xParam)
    {
        sampleCommon->error("Memory Allocation error.\n");
        return SDK_FAILURE;
    }

    xParam->_sVersion = "x";
    xParam->_lVersion = "height0";
    xParam->_description = "height of matrix A";
    xParam->_type     = streamsdk::CA_ARG_INT;
    xParam->_value    = &n;

    sampleArgs->AddOption(xParam);
    delete xParam;

    streamsdk::Option* yParam = new streamsdk::Option;
    if(!yParam)
    {
        sampleCommon->error("Memory Allocation error.\n");
        return SDK_FAILURE;
    }

    yParam->_sVersion = "y";
    yParam->_lVersion = "width0";
    yParam->_description = "width of matrix A and Height of matrix B";
    yParam->_type     = streamsdk::CA_ARG_INT;
    yParam->_value    = &m;

    sampleArgs->AddOption(yParam);
    delete yParam;

    streamsdk::Option* zParam = new streamsdk::Option;
    if(!zParam)
    {
        sampleCommon->error("Memory Allocation error.\n");
        return SDK_FAILURE;
    }

    zParam->_sVersion = "z";
    zParam->_lVersion = "width1";
    zParam->_description = "width of matrix B";
    zParam->_type     = streamsdk::CA_ARG_INT;
    zParam->_value    = &k;

    sampleArgs->AddOption(zParam);
    delete zParam;

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

int 
MatrixMulImage::setup()
{  
    /* Make sure the dimensions are multiples of blockSize */
    const int vectorSize = 4;
    if(n % (blockSize * vectorSize) != 0)
    {
        n = (n / (blockSize * vectorSize) + 1) * (blockSize * vectorSize);
    }

    if(m % blockSize != 0)
    {
        m = (m / blockSize + 1) * blockSize;
    }

    if(k % (blockSize * vectorSize) != 0)
    {
       k = (k / (blockSize * vectorSize) + 1) * (blockSize * vectorSize);
    }

    width0  = m;
    height0 = n;
    
    width1  = k;
    height1 = m;

    if(setupMatrixMulImage()!=SDK_SUCCESS)
        return SDK_FAILURE;

    int timer = sampleCommon->createTimer();
    sampleCommon->resetTimer(timer);
    sampleCommon->startTimer(timer);

    int status = setupCL();
    if(status != SDK_SUCCESS)
        return status;

    sampleCommon->stopTimer(timer);

    setupTime = (cl_double)sampleCommon->readTimer(timer);

    return SDK_SUCCESS;
}


int 
MatrixMulImage::run()
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
        int kernelRun = runCLKernels();
        if(kernelRun != SDK_SUCCESS)
        {
            return kernelRun;
        }
    }

    sampleCommon->stopTimer(timer);
    totalKernelTime = (double)(sampleCommon->readTimer(timer)) / iterations;

    if(!quiet) {
        sampleCommon->printArray<cl_float>("Output", output, width1, 1);
    }

    return SDK_SUCCESS;
}

int 
MatrixMulImage::verifyResults()
{
    if(verify)
    {
        /* reference implementation */
        int refTimer = sampleCommon->createTimer();
        sampleCommon->resetTimer(refTimer);
        sampleCommon->startTimer(refTimer);
        MatrixMulImageCPUReference(verificationOutput, 
                                              input0, 
                                              input1, 
                                              height0, 
                                              width0,  
                                              width1);
        sampleCommon->stopTimer(refTimer);
        referenceKernelTime = sampleCommon->readTimer(refTimer);

        /* compare the results and see if they match */
        if(sampleCommon->compare(output, verificationOutput, height0*width1))
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

void 
MatrixMulImage::printStats()
{
    std::string strArray[4] = 
    {
        "MatrixA", 
        "MatrixB", 
        "Time(sec)", 
        "KernelTime(sec)"
    };
    std::string stats[4];

    totalTime = setupTime + totalKernelTime;

    stats[0]  = sampleCommon->toString(height0, std::dec)
                +"x"+sampleCommon->toString(width0, std::dec);
    stats[1]  = sampleCommon->toString(height1, std::dec)
                +"x"+sampleCommon->toString(width1, std::dec);
    stats[2]  = sampleCommon->toString(totalTime, std::dec);
	stats[3]  = sampleCommon->toString(totalKernelTime, std::dec);
    
    this->SDKSample::printStats(strArray, stats, 4);
    this->SDKSample::logStats(strArray, stats, 4, "MatrixMulImage.txt");
}

int 
MatrixMulImage::cleanup()
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
 
    status = clReleaseMemObject(inputBuffer0);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clReleaseMemObject failed."))
        return SDK_FAILURE;

    status = clReleaseMemObject(inputBuffer1);
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
    if(input0) 
        free(input0);

    if(input1) 
        free(input1);

    if(output)
        free(output);

    if(verificationOutput) 
        free(verificationOutput);

    /* release device list */
    if(devices)
        free(devices);

    if(maxWorkItemSizes)
        free(maxWorkItemSizes);

    return SDK_SUCCESS;
}

int 
main(int argc, char * argv[])
{
    MatrixMulImage clMatrixMulImage("OpenCL Matrix Multiplication");

    clMatrixMulImage.initialize();

    if(!clMatrixMulImage.parseCommandLine(argc, argv))
        return SDK_FAILURE;

    if(clMatrixMulImage.isDumpBinaryEnabled())
    {
        return clMatrixMulImage.genBinaryImage();
    }
    else
    {
        int status = clMatrixMulImage.setup();
        if(status !=  SDK_SUCCESS)
        {
            if(status == SDK_FAILURE)
                return SDK_FAILURE;
            else
                return SDK_SUCCESS;
        }
        int state = clMatrixMulImage.run();
        if(state == SDK_FAILURE)
          return SDK_FAILURE;
        else
        {
            if(clMatrixMulImage.verifyResults()==SDK_FAILURE)
                return SDK_FAILURE;
        }
        if(clMatrixMulImage.cleanup()==SDK_FAILURE)
          return SDK_FAILURE;
        clMatrixMulImage.printStats();
    }


    return SDK_SUCCESS;
}
