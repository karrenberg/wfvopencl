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


#include "RadixSort.hpp"

#include <math.h>

void 
RadixSort::hostRadixSort()
{
    cl_uint *histogram = (cl_uint*)malloc(RADICES * sizeof(cl_uint));
    if(histogram == NULL)
    {
        sampleCommon->error("Failed to allocate host memory. (histogram)");
        return;
    }

    cl_uint *tempData = (cl_uint*)malloc(elementCount * sizeof(cl_uint));
    if(tempData == NULL)
    {
        sampleCommon->error("Failed to allocate host memory. (tempData)");
        return;
    }

    if(histogram != NULL && tempData != NULL)
    {
        memcpy(tempData, unsortedData, elementCount * sizeof(cl_uint));

        for(int bits = 0; bits < sizeof(cl_uint) * RADIX ; bits += RADIX)
        {

            /* Initialize histogram bucket to zeros */
            memset(histogram, 0, RADICES * sizeof(cl_uint));

            /* Calculate 256 histogram for all elements*/
            for(int i = 0; i < elementCount; ++i)
            {
                cl_uint element = tempData[i];
                cl_uint value = (element >> bits) & RADIX_MASK;
                histogram[value]++;
            }

            /* Prescan the histogram bucket */
            cl_uint sum = 0;
            for(int i = 0; i < RADICES; ++i)
            {
                cl_uint val = histogram[i];
                histogram[i] = sum;
                sum += val;
            }

            /* Rearrange  the elements based on prescaned histogram */
            for(int i = 0; i < elementCount; ++i)
            {
                cl_uint element = tempData[i];
                cl_uint value = (element >> bits) & RADIX_MASK;
                cl_uint index = histogram[value];
                hSortedData[index] = tempData[i];
                histogram[value] = index + 1;
            }

            /* Copy to tempData for further use */
            if(bits != RADIX * 3)
                memcpy(tempData, hSortedData, elementCount * sizeof(cl_uint));
        }
    }

    free(tempData);
    free(histogram);
}

int
RadixSort::setupRadixSort()
{
    int i = 0;

    groupSize = GROUP_SIZE;
    elementCount = sampleCommon->roundToPowerOf2<cl_uint>(elementCount);

    /* element count must be multiple of GROUP_SIZE * RADICES */
    int mulFactor = groupSize * RADICES;

    if(elementCount < mulFactor)
        elementCount = mulFactor;
    else
        elementCount = (elementCount / mulFactor) * mulFactor;

    numGroups = elementCount / mulFactor;

    /* Allocate and init memory used by host */
    unsortedData = (cl_uint*)malloc(elementCount * sizeof(cl_uint));
    if(unsortedData == NULL)
    { 
        sampleCommon->error("Failed to allocate host memory. (unsortedData)");
        return SDK_FAILURE;
    }

    for(i = 0; i < elementCount; i++)
    {
        unsortedData[i] = (cl_uint)rand();
    }

    dSortedData = (cl_uint*)malloc(elementCount * sizeof(cl_uint));
    if(dSortedData == NULL)
    { 
        sampleCommon->error("Failed to allocate host memory. (dSortedData)");
        return SDK_FAILURE;
    }
    memset(dSortedData, 0, elementCount * sizeof(cl_uint));

    hSortedData = (cl_uint*)malloc(elementCount * sizeof(cl_uint));
    if(hSortedData == NULL)
    { 
        sampleCommon->error("Failed to allocate host memory. (hSortedData)");
        return SDK_FAILURE;
    }
    memset(hSortedData, 0, elementCount * sizeof(cl_uint));

    size_t tempSize = numGroups * groupSize * RADICES * sizeof(cl_uint);
	histogramBins = (cl_uint*)malloc(tempSize);
    if(histogramBins == NULL)
    { 
        sampleCommon->error("Failed to allocate host memory. (histogramBins)");
        return SDK_FAILURE;
    }
    memset(histogramBins, 0, tempSize);

    return SDK_SUCCESS;
}

int 
RadixSort::genBinaryImage()
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
    kernelPath.append("RadixSort_Kernels.cl");
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
RadixSort::setupCL(void)
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

    /* Check whether the device supports byte-addressable 
    * load/stores : required for RadixSort */
    char deviceExtensions[2048];

    /* Get list of extensions supported by the device */
    status = clGetDeviceInfo(devices[deviceId], 
        CL_DEVICE_EXTENSIONS, 
        sizeof(deviceExtensions), 
        deviceExtensions,
        0);

    /* Check particular extension */
    if(!strstr(deviceExtensions, "cl_khr_byte_addressable_store"))
    {
        byteRWSupport = false;
        sampleCommon->expectedError("Device does not support cl_khr_byte_addressable_store extension!");
        return SDK_SUCCESS;
    }

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

    /* Input buffer */
    unsortedDataBuf = clCreateBuffer(
        context,
        CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
        sizeof(cl_uint) * elementCount,
        unsortedData,
        &status);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clCreateBuffer failed. (unsortedDataBuf)"))
        return SDK_FAILURE;

    /* Output for histogram kernel */
    histogramBinsBuf = clCreateBuffer(
        context, 
        CL_MEM_WRITE_ONLY,
        numGroups * groupSize * RADICES * sizeof(cl_uint),
        NULL, 
        &status);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clCreateBuffer failed. (histogramBinsBuf)"))
        return SDK_FAILURE;

    /* Input for permute kernel */
    scanedHistogramBinsBuf = clCreateBuffer(
        context, 
        CL_MEM_WRITE_ONLY,
        numGroups * groupSize * RADICES * sizeof(cl_uint),
        NULL, 
        &status);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clCreateBuffer failed. (scanedHistogramBinsBuf)"))
        return SDK_FAILURE;

    /* Final output */
    sortedDataBuf = clCreateBuffer(
        context, 
        CL_MEM_WRITE_ONLY | CL_MEM_ALLOC_HOST_PTR,
        elementCount * sizeof(cl_uint),
        NULL,
        &status);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clCreateBuffer failed. (sortedDataBuf)"))
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

	kernelPath.append("RadixSort_Kernels.cl");
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
    histogramKernel = clCreateKernel(program, "histogram", &status);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clCreateKernel failed."))
        return SDK_FAILURE;

    size_t histogramGroupSize;
    /* Check group size against group size returned by kernel */
    status = clGetKernelWorkGroupInfo(histogramKernel,
        devices[deviceId],
        CL_KERNEL_WORK_GROUP_SIZE,
        sizeof(size_t),
        &histogramGroupSize,
        0);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS, 
        "clGetKernelWorkGroupInfo  failed."))
    {
        return SDK_FAILURE;
    }

    permuteKernel = clCreateKernel(program, "permute", &status);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clCreateKernel failed."))
        return SDK_FAILURE;

    size_t permuteGroupSize;
    /* Check group size against group size returned by kernel */
    status = clGetKernelWorkGroupInfo(permuteKernel,
        devices[deviceId],
        CL_KERNEL_WORK_GROUP_SIZE,
        sizeof(size_t),
        &permuteGroupSize,
        0);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS, 
        "clGetKernelWorkGroupInfo  failed."))
    {
        return SDK_FAILURE;
    }

    /* Find minimum of all kernel's work-group sizes */
    size_t temp = min(histogramGroupSize, permuteGroupSize);

    /* If groupSize exceeds the minimum */
    if((size_t)groupSize > temp)
    {
        if(!quiet)
        {
            std::cout << "Out of Resources!" << std::endl;
            std::cout << "Group Size specified : " << groupSize << std::endl;
            std::cout << "Max Group Size supported on the kernel : "
                      << temp << std::endl;
            std::cout << "Falling back to " << temp << std::endl;
        }
        groupSize = (cl_int)temp;
    }

    return SDK_SUCCESS;
}

int 
RadixSort::runHistogramKernel(int bits)
{
    cl_int status;
    cl_event events[1];

    size_t globalThreads = elementCount / RADICES;
    size_t localThreads = groupSize;

    if(localThreads > maxWorkItemSizes[0] || 
       localThreads > maxWorkGroupSize)
    {
        std::cout << "Unsupported: Device does not"
            "support requested number of work items.";
        return SDK_FAILURE;
    }

    /* Enqueue write from unSortedData to unSortedDataBuf */
    status = clEnqueueWriteBuffer(commandQueue, 
                                  unsortedDataBuf, 
                                  1,
                                  0, sizeof(cl_uint) * elementCount,
                                  unsortedData, 
                                  0, 
                                  0, 
                                  0);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clEnqueueWriteBuffer failed. (unsortedDataBuf)"))
        return SDK_FAILURE;
    
    /* Setup kernel arguments */
    status = clSetKernelArg(histogramKernel, 
                            0, 
                            sizeof(cl_mem), 
                            (void *)&unsortedDataBuf); 
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clSetKernelArg failed. (unsortedDataBuf)"))
        return SDK_FAILURE;

    status = clSetKernelArg(histogramKernel, 
                            1, 
                            sizeof(cl_mem), 
                            (void *)&histogramBinsBuf); 
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clSetKernelArg failed. (histogramBinsBuf)"))
        return SDK_FAILURE;

    status = clSetKernelArg(histogramKernel, 
                            2, 
                            sizeof(cl_int), 
                            (void *)&bits); 
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clSetKernelArg failed. (bits)"))
        return SDK_FAILURE;

    status = clSetKernelArg(histogramKernel, 
                            3, 
                            (groupSize * RADICES * sizeof(cl_ushort)), 
                            NULL); 
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clSetKernelArg failed. (local memory)"))
        return SDK_FAILURE;

    status = clGetKernelWorkGroupInfo(histogramKernel,
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

    /* 
    * Enqueue a kernel run call.
    */
    status = clEnqueueNDRangeKernel(
        commandQueue,
        histogramKernel,
        1,
        NULL,
        &globalThreads,
        &localThreads,
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

    status = clReleaseEvent(events[0]);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clReleaseEvent failed."))
        return SDK_FAILURE;

    /* Enqueue the results to application pointer*/
    status = clEnqueueReadBuffer(
        commandQueue, 
        histogramBinsBuf, 
        CL_TRUE,
        0,
        numGroups * groupSize * 256 * sizeof(cl_uint),
        histogramBins,
        0,
        NULL,
        0);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clEnqueueReadBuffer failed."))
        return SDK_FAILURE;

    return SDK_SUCCESS;
}



int
RadixSort::runPermuteKernel(int bits)
{
    cl_int status;
    cl_event events[1];

    size_t bufferSize = numGroups * groupSize * RADICES * sizeof(cl_uint);

    /* Write the host updated data to histogramBinsBuf */
    status = clEnqueueWriteBuffer(commandQueue,
                                  scanedHistogramBinsBuf,
                                  1,
                                  0,
                                  bufferSize,
                                  histogramBins,
                                  0, 
                                  0, 
                                  0);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clEnqueueWriteBuffer failed. (scanedHistogramBinsBuf)"))
        return SDK_FAILURE;

    size_t globalThreads = elementCount / RADICES;
    size_t localThreads = groupSize;

    if(localThreads > maxWorkItemSizes[0] || 
       localThreads > maxWorkGroupSize)
    {
        std::cout<<"Unsupported: Device does not"
            "support requested number of work items.";
        return SDK_FAILURE;
    }

    /* Whether sort is to be in increasing order. CL_TRUE implies increasing */
    status = clSetKernelArg(permuteKernel, 
                            0, 
                            sizeof(cl_mem), 
                            (void *)&unsortedDataBuf); 
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clSetKernelArg failed. (unsortedDataBuf)"))
        return SDK_FAILURE;


    status = clSetKernelArg(permuteKernel, 
                            1, 
                            sizeof(cl_mem), 
                            (void *)&scanedHistogramBinsBuf); 
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clSetKernelArg failed. (scanedHistogramBinsBuf)"))
        return SDK_FAILURE;


    status = clSetKernelArg(permuteKernel, 
                            2, 
                            sizeof(cl_int), 
                            (void *)&bits); 
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clSetKernelArg failed. (bits)"))
        return SDK_FAILURE;

    status = clSetKernelArg(permuteKernel, 
                            3, 
                            (groupSize * RADICES * sizeof(cl_ushort)),
                            NULL); 
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clSetKernelArg failed. (local memory)"))
        return SDK_FAILURE;

    status = clSetKernelArg(permuteKernel, 
                            4, 
                            sizeof(cl_mem), 
                            (void *)&sortedDataBuf); 
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clSetKernelArg failed. (sortedDataBuf)"))
        return SDK_FAILURE;

    status = clGetKernelWorkGroupInfo(permuteKernel,
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

    /* 
    * Enqueue a kernel run call.
    */
    status = clEnqueueNDRangeKernel(
        commandQueue,
        permuteKernel,
        1,
        NULL,
        &globalThreads,
        &localThreads,
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

    status = clReleaseEvent(events[0]);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clReleaseEvent failed."))
        return SDK_FAILURE;

    /* Enqueue the results to application pointer*/
    status = clEnqueueReadBuffer(
        commandQueue, 
        sortedDataBuf, 
        CL_TRUE,
        0,
        elementCount * sizeof(cl_uint),
        dSortedData,
        0,
        NULL,
        0);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clEnqueueReadBuffer failed."))
        return SDK_FAILURE;

    return SDK_SUCCESS;
}



int 
RadixSort::runCLKernels(void)
{
    for(int bits = 0; bits < sizeof(cl_uint) * RADIX; bits += RADIX)
    {
        /* Calculate thread-histograms */
        runHistogramKernel(bits);

        /* Scan the histogram  */
        int sum = 0;
        for(int i = 0; i < RADICES; ++i)
        {
            for(int j = 0; j < numGroups; ++j)
            {
                for(int k = 0; k < groupSize; ++k)
                {
                    int index = j * groupSize * RADICES + k * RADICES + i;
                    int value = histogramBins[index];
                    histogramBins[index] = sum;
                    sum += value;
                }
            }
        }

        /* Permute the element to appropriate place */
        runPermuteKernel(bits);

        /* Current output now becomes the next input */
        memcpy(unsortedData, dSortedData, elementCount * sizeof(cl_uint));
    }

    return SDK_SUCCESS;
}

int 
RadixSort::initialize()
{
    // Call base class Initialize to get default configuration
    if(!this->SDKSample::initialize())
        return SDK_FAILURE;

    // Now add customized options
    streamsdk::Option* array_length = new streamsdk::Option;
    if(!array_length)
    {
        sampleCommon->error("Memory allocation error.\n");
        return SDK_FAILURE;
    }

    array_length->_sVersion = "x";
    array_length->_lVersion = "count";
    array_length->_description = "Element count";
    array_length->_type = streamsdk::CA_ARG_INT;
    array_length->_value = &elementCount;
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

int RadixSort::setup()
{
    if(setupRadixSort() != SDK_SUCCESS)
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


int RadixSort::run()
{
    if(!byteRWSupport)
        return SDK_SUCCESS;

    int timer = sampleCommon->createTimer();
    sampleCommon->resetTimer(timer);
    sampleCommon->startTimer(timer);

    std::cout << "Executing kernel for " <<
        iterations<<" iterations" << std::endl;
    std::cout << "-------------------------------------------" << std::endl;

    for(int i = 0; i < iterations; i++)
    {
        /* Arguments are set and execution call is enqueued on command buffer */
        if(runCLKernels() != SDK_SUCCESS)
            return SDK_FAILURE;
    }

    sampleCommon->stopTimer(timer);    
    /* Compute kernel time */
    kernelTime = (double)(sampleCommon->readTimer(timer)) / iterations;

    if(!quiet)
        sampleCommon->printArray<cl_uint>("dSortedData", dSortedData, 512, 1);

    return SDK_SUCCESS;
}

int 
RadixSort::verifyResults()
{
    if(!byteRWSupport)
        return SDK_SUCCESS;

    if(verify)
    {
        /* Rreference implementation on host device
        * Sorted by radix sort
        */
        hostRadixSort();

        /* compare the results and see if they match */
        bool result = true;
        int failedCount = 0;
        for(int i = 0; i < elementCount; ++i)
        {
            if(dSortedData[i] != hSortedData[i])
            {
                result = false;
                failedCount++;
                printf("Element(%d) -  %u = %u \n", 
                       i,
                       dSortedData[i], hSortedData[i]);
            }
        }

        if(result)
        {
            std::cout << "Passed!\n";
            return SDK_SUCCESS;
        }
        else
        {
            printf("\n%d elements Failed\n", failedCount);
            std::cout <<" Failed\n";
            return SDK_FAILURE;
        }
    }

    return SDK_SUCCESS;
}

void
RadixSort::printStats()
{
    if(!byteRWSupport)
        return;

    std::string strArray[3] = {"Elements", "Time(sec)", "kernelTime(sec)"};
    std::string stats[3];

    totalTime = setupTime + kernelTime;
    stats[0]  = sampleCommon->toString(elementCount, std::dec);
    stats[1]  = sampleCommon->toString(totalTime, std::dec);
    stats[2]  = sampleCommon->toString(kernelTime, std::dec);

    this->SDKSample::printStats(strArray, stats, 3);
    this->SDKSample::logStats(kernelTime, "RadixSort", vendorName);
}


int
RadixSort::cleanup()
{
    if(!byteRWSupport)
        return SDK_SUCCESS;

    /* Releases OpenCL resources (Context, Memory etc.) */
    cl_int status;

    status = clReleaseMemObject(unsortedDataBuf);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clReleaseMemObject failed."))
        return SDK_FAILURE;

    status = clReleaseMemObject(sortedDataBuf);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clReleaseMemObject failed."))
        return SDK_FAILURE;

    status = clReleaseMemObject(histogramBinsBuf);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clReleaseMemObject failed."))
        return SDK_FAILURE;


    status = clReleaseKernel(histogramKernel);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clReleaseKernel failed."))
        return SDK_FAILURE;


    status = clReleaseKernel(permuteKernel);
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
    if(unsortedData) 
    {
        free(unsortedData);
        unsortedData = NULL;
    }

    if(dSortedData) 
    {
        free(dSortedData);
        dSortedData = NULL;
    }

    if(hSortedData) 
    {
        free(hSortedData);
        hSortedData = NULL;
    }

    if(histogramBins) 
    {
        free(histogramBins);
        histogramBins = NULL;
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
    RadixSort clRadixSort("RadixSort sample");

    /* Initialization */
    if(clRadixSort.initialize() != SDK_SUCCESS)
        return SDK_FAILURE;

    /* Parse command line options */
    if(!clRadixSort.parseCommandLine(argc, argv))
        return SDK_FAILURE;

    if(clRadixSort.isDumpBinaryEnabled())
    {
        return clRadixSort.genBinaryImage();
    }
    else
    {
        /* Setup */
        if(clRadixSort.setup() != SDK_SUCCESS)
            return SDK_FAILURE;

        /* Run */
        if(clRadixSort.run() != SDK_SUCCESS)
            return SDK_FAILURE;

        /* Verify */
        if(clRadixSort.verifyResults() != SDK_SUCCESS)
            return SDK_FAILURE;

        /* Cleanup resources created */
        if(clRadixSort.cleanup() != SDK_SUCCESS)
            return SDK_FAILURE;

        /* Print performance statistics */
        clRadixSort.printStats();
    }

    return SDK_SUCCESS;
}
