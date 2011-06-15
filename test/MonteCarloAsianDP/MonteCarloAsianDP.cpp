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


#include "MonteCarloAsianDP.hpp"

#include <math.h>
#include <malloc.h>


/*
 *  structure for attributes of Monte calro 
 *  simulation
 */

 typedef struct _MonteCalroAttrib
 {
     cl_double4 strikePrice;
     cl_double4 c1;
     cl_double4 c2;
     cl_double4 c3;
     cl_double4 initPrice;
     cl_double4 sigma;
     cl_double4 timeStep;
 }MonteCarloAttrib;


int 
MonteCarloAsianDP::setupMonteCarloAsianDP()
{
    int i = 0;
    const cl_double finalValue = 0.8;
    const cl_double stepValue = finalValue / (cl_double)steps;
    
    /* Allocate and init memory used by host */
    sigma = (cl_double*)malloc(steps * sizeof(cl_double));
    if(sigma == NULL)
    {
        sampleCommon->error("Failed to allocate host memory. (sigma)");
        return SDK_FAILURE;
    }

    sigma[0] = 0.01;
    for(i = 1; i < steps; i++)
    {
        sigma[i] = sigma[i - 1] + stepValue;
    }
    
    price = (cl_double*) malloc(steps * sizeof(cl_double));
    if(price == NULL)
    {
        sampleCommon->error("Failed to allocate host memory. (price)");
        return SDK_FAILURE;
    }
    memset((void*)price, 0, steps * sizeof(cl_double));

    vega = (cl_double*) malloc(steps * sizeof(cl_double));
    if(vega == NULL)
    {
        sampleCommon->error("Failed to allocate host memory. (vega)");
        return SDK_FAILURE;
    }
    memset((void*)vega, 0, steps * sizeof(cl_double));

    refPrice = (cl_double*) malloc(steps * sizeof(cl_double));
    if(refPrice == NULL)
    {
        sampleCommon->error("Failed to allocate host memory. (refPrice)");
        return SDK_FAILURE;
    }
    memset((void*)refPrice, 0, steps * sizeof(cl_double));

    refVega = (cl_double*) malloc(steps * sizeof(cl_double));
    if(refVega == NULL)
    {
        sampleCommon->error("Failed to allocate host memory. (refVega)");
        return SDK_FAILURE;
    }
    memset((void*)refVega, 0, steps * sizeof(cl_double));

    /* Set samples and exercize points */
    noOfSum = 12;
    noOfTraj = 1024;

    width = noOfTraj / 4;
    height = noOfTraj / 2;

#if defined (_WIN32)
    randNum = (cl_uint*)_aligned_malloc(width * height * sizeof(cl_uint4), 16);
#else
    randNum = (cl_uint*)memalign(16, width * height * sizeof(cl_uint4));
#endif

    if(randNum == NULL)
    { 
        sampleCommon->error("Failed to allocate host memory. (randNum)");
        return SDK_FAILURE;
    }

    priceVals = (cl_double*)malloc(width * height * 2 * sizeof(cl_double4));
    if(priceVals == NULL)
    { 
        sampleCommon->error("Failed to allocate host memory. (priceVals)");
        return SDK_FAILURE;
    }
    memset((void*)priceVals, 0, width * height * 2 * sizeof(cl_double4));

    priceDeriv = (cl_double*)malloc(width * height * 2 * sizeof(cl_double4));
    if(priceDeriv == NULL)	
    { 
        sampleCommon->error("Failed to allocate host memory. (priceDeriv)");
        return SDK_FAILURE;
    }
    memset((void*)priceDeriv, 0, width * height * 2 * sizeof(cl_double4));

    /* 
     * Unless quiet mode has been enabled, print the INPUT array.
     * No more than 256 values are printed because it clutters the screen
     * and it is not practical to manually compare a large set of numbers
     */
    if(!quiet) 
    {
        sampleCommon->printArray<cl_double>("sigma values", 
                                           sigma, 
                                           steps, 
                                           1);
    }

    return SDK_SUCCESS;
}

int 
MonteCarloAsianDP::genBinaryImage()
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
    kernelPath.append("MonteCarloAsianDP_Kernels.cl");
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
MonteCarloAsianDP::setupCL(void)
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
        commandQueue = clCreateCommandQueue(context, 
                                            devices[deviceId], 
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
    status = clGetDeviceInfo(devices[deviceId],
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


    status = clGetDeviceInfo(devices[deviceId],
                             CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS,
                             sizeof(cl_uint),
                             (void*)&maxDimensions,
                             NULL);

    if(!sampleCommon->checkVal(status,
                               CL_SUCCESS, 
                               "clGetDeviceInfo "
                               "CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS failed."))
    {
        return SDK_FAILURE;
    }


    maxWorkItemSizes = (size_t*)malloc(maxDimensions * sizeof(size_t));
    
    status = clGetDeviceInfo(devices[deviceId],
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


    char* deviceExtensions = NULL;;
    size_t extStringSize = 0;

    // Get device extensions 
    status = clGetDeviceInfo(devices[deviceId], 
        CL_DEVICE_EXTENSIONS, 
        0, 
        deviceExtensions, 
        &extStringSize);

    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS, 
        "clGetDeviceInfo failed.(CL_DEVICE_EXTENSIONS)"))
        return SDK_FAILURE;
    
    deviceExtensions = new char[extStringSize];
    if(NULL == deviceExtensions)
    {
        sampleCommon->error("Failed to allocate memory(deviceExtensions)");
        return  SDK_FAILURE;
    }

    status = clGetDeviceInfo(devices[deviceId], 
        CL_DEVICE_EXTENSIONS, 
        extStringSize, 
        deviceExtensions, 
        NULL);

    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS, 
        "clGetDeviceInfo failed.(CL_DEVICE_EXTENSIONS)"))
        return SDK_FAILURE;


    std::string buildOptions = std::string("");
    /* Check if cl_khr_fp64 extension is supported */
    if(strstr(deviceExtensions, "cl_khr_fp64"))
    {
        buildOptions.append("-D KHR_DP_EXTENSION");
    }
    else
    {
        /* Check if cl_amd_fp64 extension is supported */
        if(!strstr(deviceExtensions, "cl_amd_fp64"))
        {
            sampleCommon->expectedError("Device does not support cl_amd_fp64 extension!");
            delete deviceExtensions;
            return SDK_EXPECTED_FAILURE;
        }
    }

    delete deviceExtensions;

    // Get device version string
    char deviceVersion[1024];
    status = clGetDeviceInfo(devices[deviceId], 
        CL_DEVICE_VERSION, 
        sizeof(deviceVersion), 
        deviceVersion, 
        NULL);

    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS, 
        "clGetDeviceInfo failed.(CL_DEVICE_VERSION)"))
        return SDK_FAILURE;

    // Exit if SDK version is 2.2 or less
    std::string deviceVersionStr = std::string(deviceVersion);
    size_t vStart = deviceVersionStr.find_last_of("v");
    size_t vEnd = deviceVersionStr.find(" ", vStart);
    std::string vStrVal = deviceVersionStr.substr(vStart + 1, vEnd - vStart - 1);
    if(vStrVal.compare("2.2") <= 0 && dType == CL_DEVICE_TYPE_GPU)
    {
        sampleCommon->expectedError("Few double math functions are not supported in SDK2.2 or less!");
        return SDK_EXPECTED_FAILURE;
    }

    // Exit if OpenCL version is 1.0
    vStart = deviceVersionStr.find(" ", 0);
    vEnd = deviceVersionStr.find(" ", vStart + 1);
    vStrVal = deviceVersionStr.substr(vStart + 1, vEnd - vStart - 1);
    if(vStrVal.compare("1.0") <= 0 && dType == CL_DEVICE_TYPE_GPU)
    {
        sampleCommon->expectedError("Unsupported device!");
        return SDK_EXPECTED_FAILURE;
    }
    
    randBuf = clCreateBuffer(context, 
                             CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
                             sizeof(cl_uint4) * width  * height,
                             randNum, 
                             &status);
    if(!sampleCommon->checkVal(status,
                               CL_SUCCESS,
                               "clCreateBuffer failed. (randBuf)"))
    {
        return SDK_FAILURE;
    }


    priceBuf = clCreateBuffer(context, 
                              CL_MEM_WRITE_ONLY,
                              sizeof(cl_double4) * width * height * 2,
                              NULL, 
                              &status);
    if(!sampleCommon->checkVal(status,
                               CL_SUCCESS,
                               "clCreateBuffer failed. (priceBuf)"))
    {
        return SDK_FAILURE;
    }

    priceDerivBuf = clCreateBuffer(context, 
                                   CL_MEM_WRITE_ONLY,
                                   sizeof(cl_double4) * width * height * 2,
                                   NULL, 
                                   &status);
    if(!sampleCommon->checkVal(status,
                               CL_SUCCESS,
                               "clCreateBuffer failed. (priceDerivBuf)"))
    {
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

	kernelPath.append("MonteCarloAsianDP_Kernels.cl");
	if(!kernelFile.open(kernelPath.c_str()))
	{
		std::cout << "Failed to load kernel file : " << kernelPath << std::endl;
		return SDK_FAILURE;
	}

	const char * source = platformIsPacketizedOpenCL ?
		"MonteCarloAsianDP_Kernels.bc" :
		kernelFile.source().c_str();

        size_t sourceSize[] = {strlen(source)};
        program = clCreateProgramWithSource(context,
                                            1,
                                            &source,
                                            sourceSize,
                                            &status);
    }
    if(!sampleCommon->checkVal(status,
                               CL_SUCCESS,
                               "clCreateProgramWithSource failed."))
    {
        return SDK_FAILURE;
    }

    std::string flagsStr = std::string("");

    //Get build options if any
    flagsStr.append(buildOptions.c_str());

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
            if(!sampleCommon->checkVal(logStatus,
                                       CL_SUCCESS,
                                       "clGetProgramBuildInfo failed."))
            {
                return SDK_FAILURE;
            }
            
            buildLog = (char*)malloc(buildLogSize);
            if(buildLog == NULL)
            {
                sampleCommon->error("Failed to allocate host memory"
                    "(buildLog)");
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
    kernel = clCreateKernel(program, "calPriceVega", &status);
    if(!sampleCommon->checkVal(status,
                               CL_SUCCESS,
                               "clCreateKernel failed."))
    {
        return SDK_FAILURE;
    }

    /* Check group-size against what is returned by kernel */
    status = clGetKernelWorkGroupInfo(kernel,
                                      devices[deviceId],
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
                      << blockSizeX * blockSizeY << std::endl;
            std::cout << "Max Group Size supported on the kernel : " 
                      << kernelWorkGroupSize << std::endl;
            std::cout << "Falling back to " << kernelWorkGroupSize << std::endl;
        }

        /* Three possible cases */
        if(blockSizeX > kernelWorkGroupSize)
        {
            blockSizeX = kernelWorkGroupSize;
            blockSizeY = 1;
        }
    }

    return SDK_SUCCESS;
}


int 
MonteCarloAsianDP::runCLKernels(void)
{
    cl_int status;
    cl_event events[1];

    size_t globalThreads[2] = {width, height};
    size_t localThreads[2] = {blockSizeX, blockSizeY};

    /*
     * Declare attribute structure
     */
    MonteCarloAttrib attributes;

    if(localThreads[0] > maxWorkItemSizes[0] || 
       localThreads[1] > maxWorkItemSizes[1] || 
       (size_t)blockSizeX * blockSizeY > maxWorkGroupSize)
    {
        std::cout << "Unsupported: Device does not support requested"
            ":number of work items.";
        return SDK_FAILURE;
    }

    /* width - i.e number of elements in the array */
    status = clSetKernelArg(kernel, 2, sizeof(cl_uint), (void*)&width);
    if(!sampleCommon->checkVal(status,
                               CL_SUCCESS,
                               "clSetKernelArg failed. (width)"))
    {
        return SDK_FAILURE;
    }

    /* whether sort is to be in increasing order. CL_TRUE implies increasing */
    status = clSetKernelArg(kernel, 3, sizeof(cl_mem), (void*)&randBuf); 
    if(!sampleCommon->checkVal(status, 
                               CL_SUCCESS,
                               "clSetKernelArg failed. (randBuf)"))
    {
        return SDK_FAILURE;
    }

    status = clSetKernelArg(kernel, 4, sizeof(cl_mem), (void*)&priceBuf); 
    if(!sampleCommon->checkVal(status,
                               CL_SUCCESS,
                               "clSetKernelArg failed. (priceBuf)"))
    {
        return SDK_FAILURE;
    }

    status = clSetKernelArg(kernel, 5, sizeof(cl_mem), (void*)&priceDerivBuf); 
    if(!sampleCommon->checkVal(status,
                               CL_SUCCESS,
                               "clSetKernelArg failed. (priceDerivBuf)"))
    {
        return SDK_FAILURE;
    }

    status = clSetKernelArg(kernel, 1, sizeof(cl_int), (void*)&noOfSum);
    if(!sampleCommon->checkVal(status,
                               CL_SUCCESS,
                               "clSetKernelArg failed. (noOfSum)"))
    {
        return SDK_FAILURE;
    }

    double timeStep = maturity / (noOfSum - 1);

    // Initialize random number generator
    srand(1);
    
    for(int k = 0; k < steps; k++)
    {
        for(int j = 0; j < (width * height * 4); j++)
        {
            randNum[j] = (cl_uint)rand();
        }

        double c1 = (interest - 0.5 * sigma[k] * sigma[k]) * timeStep;
        double c2 = sigma[k] * sqrt(timeStep);
        double c3 = (interest + 0.5 * sigma[k] * sigma[k]);
        
        const cl_double4 c1F4 = {c1, c1, c1, c1};
        attributes.c1 = c1F4;

        const cl_double4 c2F4 = {c2, c2, c2, c2};
        attributes.c2 = c2F4;

        const cl_double4 c3F4 = {c3, c3, c3, c3};
        attributes.c3 = c3F4;

        const cl_double4 initPriceF4 = {initPrice, initPrice, initPrice, initPrice};
        attributes.initPrice = initPriceF4;

        const cl_double4 strikePriceF4 = {strikePrice, strikePrice, strikePrice, strikePrice};
        attributes.strikePrice = strikePriceF4;

        const cl_double4 sigmaF4 = {sigma[k], sigma[k], sigma[k], sigma[k]};
        attributes.sigma = sigmaF4;

        const cl_double4 timeStepF4 = {timeStep, timeStep, timeStep, timeStep};
        attributes.timeStep = timeStepF4;


        /* Set appropriate arguments to the kernel */

        /* the input array - also acts as output for this pass (input for next) */
        status = clSetKernelArg(kernel, 0, sizeof(attributes), (void*)&attributes);
        if(!sampleCommon->checkVal(status,
                                   CL_SUCCESS,
                                   "clSetKernelArg failed. (attributes)"))
        {
            return SDK_FAILURE;
        }

        /* 
         * Enqueue a kernel run call.
         */
        status = clEnqueueNDRangeKernel(commandQueue,
                                        kernel,
                                        2,
                                        NULL,
                                        globalThreads,
                                        localThreads,
                                        0,
                                        NULL,
                                        &events[0]);
        
        if(!sampleCommon->checkVal(status,
                                   CL_SUCCESS,
                                   "clEnqueueNDRangeKernel failed."))
        {
            return SDK_FAILURE;
        }


        /* wait for the kernel call to finish execution */
        status = clWaitForEvents(1, &events[0]);
        if(!sampleCommon->checkVal(status,
                                   CL_SUCCESS,
                                   "clWaitForEvents failed."))
        {
            return SDK_FAILURE;
        }
        
        clReleaseEvent(events[0]);

        /* Enqueue the results to application pointer*/
        status = clEnqueueReadBuffer(commandQueue, 
                                     priceBuf, 
                                     CL_TRUE,
                                     0,
                                     width * height * 2 * sizeof(cl_double4),
                                     priceVals,
                                     0,
                                     NULL,
                                     &events[0]);
        if(!sampleCommon->checkVal(status,
                                   CL_SUCCESS,
                                   "clEnqueueReadBuffer failed."))
        {
            return SDK_FAILURE;
        }
        
        /* wait for the read buffer to finish execution */
        status = clWaitForEvents(1, &events[0]);
        if(!sampleCommon->checkVal(status,
                                   CL_SUCCESS,
                                   "clWaitForEvents failed."))
        {
            return SDK_FAILURE;
        }

        clReleaseEvent(events[0]);

        /* Enqueue the results to application pointer*/
        status = clEnqueueReadBuffer(commandQueue, 
                                     priceDerivBuf, 
                                     CL_TRUE,
                                     0,
                                     width * height * 2 * sizeof(cl_double4),
                                     priceDeriv,
                                     0,
                                     NULL,
                                     &events[0]);
        if(!sampleCommon->checkVal(status,
                                   CL_SUCCESS,
                                   "clEnqueueReadBuffer failed."))
        {
            return SDK_FAILURE;
        }

        /* wait for the read buffer to finish execution */
        status = clWaitForEvents(1, &events[0]);
        if(!sampleCommon->checkVal(status,
                                   CL_SUCCESS,
                                   "clWaitForEvents failed."))
        {
            return SDK_FAILURE;
        }

        clReleaseEvent(events[0]);
        
        /* Replace Following "for" loop with reduction kernel */
        for(int i = 0; i < noOfTraj * noOfTraj; i++)
        {
            price[k] += priceVals[i];
            vega[k] += priceDeriv[i];
        }

        price[k] /= (noOfTraj * noOfTraj);
        vega[k] /= (noOfTraj * noOfTraj);

        price[k] = exp(-interest * maturity) * price[k];
        vega[k] = exp(-interest * maturity) * vega[k];
    }
    
    return SDK_SUCCESS;
}

int
MonteCarloAsianDP::initialize()
{
    /* Call base class Initialize to get default configuration */
    if(!this->SDKSample::initialize())
        return SDK_FAILURE;
        
    const int optionsCount = 5;
    streamsdk::Option *optionList = new streamsdk::Option[optionsCount];
    if(!optionList)
    {
        std::cout << "Error. Failed to allocate memory (optionList)\n";
        return SDK_FAILURE;
    }

    optionList[0]._sVersion = "c";
    optionList[0]._lVersion = "steps";
    optionList[0]._description = "Steps of Monte carlo simuation";
    optionList[0]._type = streamsdk::CA_ARG_INT;
    optionList[0]._value = &steps;

    optionList[1]._sVersion = "p";
    optionList[1]._lVersion = "initPrice";
    optionList[1]._description = "Initial price(Default value 50)";
    optionList[1]._type = streamsdk::CA_ARG_STRING;
    optionList[1]._value = &initPrice;

    optionList[2]._sVersion = "s";
    optionList[2]._lVersion = "strikePrice";
    optionList[2]._description = "Strike price (Default value 55)";
    optionList[2]._type = streamsdk::CA_ARG_STRING;
    optionList[2]._value = &strikePrice;

    optionList[3]._sVersion = "r";
    optionList[3]._lVersion = "interest";
    optionList[3]._description = "interest rate (Default value 0.06)";
    optionList[3]._type = streamsdk::CA_ARG_STRING;
    optionList[3]._value = &interest;

    optionList[4]._sVersion = "m";
    optionList[4]._lVersion = "maturity";
    optionList[4]._description = "Maturity (Default value 1)";
    optionList[4]._type = streamsdk::CA_ARG_STRING;
    optionList[4]._value = &maturity;

    
    for(cl_int i = 0; i < optionsCount; ++i)
    {
        sampleArgs->AddOption(&optionList[i]);
    }

    delete[] optionList;

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

int MonteCarloAsianDP::setup()
{
    if(setupMonteCarloAsianDP() != SDK_SUCCESS)
        return SDK_FAILURE;

    /* create and initialize timers */
    int timer = sampleCommon->createTimer();
    sampleCommon->resetTimer(timer);
    sampleCommon->startTimer(timer);

    int returnVal = setupCL();
    if(returnVal != SDK_SUCCESS)
        return returnVal;

    sampleCommon->stopTimer(timer);
    /* Compute setup time */
    setupTime = (double)(sampleCommon->readTimer(timer));

    return SDK_SUCCESS;
}


int MonteCarloAsianDP::run()
{
    /* create and initialize timers */
    int timer = sampleCommon->createTimer();
    sampleCommon->resetTimer(timer);
    sampleCommon->startTimer(timer);

    std::cout<<"Executing kernel for " << 
        iterations << " iterations" << std::endl;
    std::cout<<"-------------------------------------------" << 
        std::endl;

    for(int i = 0; i < iterations; i++)
    {
        /* Arguments are set and execution call is enqueued on command buffer */
        if(runCLKernels() != SDK_SUCCESS)
            return SDK_FAILURE;
    }

    sampleCommon->stopTimer(timer);
    /* Compute average kernel time */
    kernelTime = (double)(sampleCommon->readTimer(timer)) / iterations;


    if(!quiet)
    {
        sampleCommon->printArray<cl_double>("price", price, steps, 1);
        sampleCommon->printArray<cl_double>("vega", vega, steps, 1);
    }

    return SDK_SUCCESS;
}

void
MonteCarloAsianDP::printStats()
{
    std::string strArray[4] = 
    {
        "Steps", 
        "Time(sec)", 
        "kernelTime(sec)", 
        "Samples used /sec"
    };
    std::string stats[4];
        
    totalTime = setupTime + kernelTime;
    stats[0] = sampleCommon->toString(steps, std::dec);
    stats[1] = sampleCommon->toString(totalTime, std::dec);
    stats[2] = sampleCommon->toString(kernelTime, std::dec);
    stats[3] = sampleCommon->toString((noOfTraj * (noOfSum - 1) * steps) / 
                                      totalTime, std::dec);
    
    this->SDKSample::printStats(strArray, stats, 4);
}

void
MonteCarloAsianDP::lshift128(unsigned int* input,
                           unsigned int shift, 
                           unsigned int * output)
{
    unsigned int invshift = 32u - shift;

    output[0] = input[0] << shift;
    output[1] = (input[1] << shift) | (input[0] >> invshift);
    output[2] = (input[2] << shift) | (input[1] >> invshift);
    output[3] = (input[3] << shift) | (input[2] >> invshift);
}

void
MonteCarloAsianDP::rshift128(unsigned int* input, 
                           unsigned int shift, 
                           unsigned int* output)
{
    unsigned int invshift = 32u - shift;
    output[3]= input[3] >> shift;
    output[2] = (input[2] >> shift) | (input[0] >> invshift);
    output[1] = (input[1] >> shift) | (input[1] >> invshift);
    output[0] = (input[0] >> shift) | (input[2] >> invshift);
}

void 
MonteCarloAsianDP::generateRand(unsigned int* seed,
                              double *gaussianRand1,
                              double *gaussianRand2,
                              unsigned int* nextRand)
{

    unsigned int mulFactor = 4;
    unsigned int temp[8][4];
    
    unsigned int state1[4] = {seed[0], seed[1], seed[2], seed[3]};
    unsigned int state2[4] = {0u, 0u, 0u, 0u}; 
    unsigned int state3[4] = {0u, 0u, 0u, 0u}; 
    unsigned int state4[4] = {0u, 0u, 0u, 0u}; 
    unsigned int state5[4] = {0u, 0u, 0u, 0u}; 
    
    unsigned int stateMask = 1812433253u;
    unsigned int thirty = 30u;
    unsigned int mask4[4] = {stateMask, stateMask, stateMask, stateMask};
    unsigned int thirty4[4] = {thirty, thirty, thirty, thirty};
    unsigned int one4[4] = {1u, 1u, 1u, 1u};
    unsigned int two4[4] = {2u, 2u, 2u, 2u};
    unsigned int three4[4] = {3u, 3u, 3u, 3u};
    unsigned int four4[4] = {4u, 4u, 4u, 4u};

    unsigned int r1[4] = {0u, 0u, 0u, 0u}; 
    unsigned int r2[4] = {0u, 0u, 0u, 0u}; 

    unsigned int a[4] = {0u, 0u, 0u, 0u}; 
    unsigned int b[4] = {0u, 0u, 0u, 0u}; 

    unsigned int e[4] = {0u, 0u, 0u, 0u}; 
    unsigned int f[4] = {0u, 0u, 0u, 0u}; 
    
    unsigned int thirteen  = 13u;
    unsigned int fifteen = 15u;
    unsigned int shift = 8u * 3u;

    unsigned int mask11 = 0xfdff37ffu;
    unsigned int mask12 = 0xef7f3f7du;
    unsigned int mask13 = 0xff777b7du;
    unsigned int mask14 = 0x7ff7fb2fu;
    
    const double one = 1.0;
    const double intMax = 4294967296.0;
    const double PI = 3.14159265358979;
    const double two = 2.0;

    double r[4] = {0.0, 0.0, 0.0, 0.0}; 
    double phi[4] = {0.0, 0.0, 0.0, 0.0};

    double temp1[4] = {0.0, 0.0, 0.0, 0.0};
    double temp2[4] = {0.0, 0.0, 0.0, 0.0};
    
    //Initializing states.
    for(int c = 0; c < 4; ++c)
    {
        state2[c] = mask4[c] * (state1[c] ^ (state1[c] >> thirty4[c])) + one4[c];
        state3[c] = mask4[c] * (state2[c] ^ (state2[c] >> thirty4[c])) + two4[c];
        state4[c] = mask4[c] * (state3[c] ^ (state3[c] >> thirty4[c])) + three4[c];
        state5[c] = mask4[c] * (state4[c] ^ (state4[c] >> thirty4[c])) + four4[c];
    }
    
    unsigned int i = 0;
    for(i = 0; i < mulFactor; ++i)
    {
        switch(i)
        {
            case 0:
                for(int c = 0; c < 4; ++c)
                {
                    r1[c] = state4[c];
                    r2[c] = state5[c];
                    a[c] = state1[c];
                    b[c] = state3[c];
                }
                break;
            case 1:
                for(int c = 0; c < 4; ++c)
                {
                    r1[c] = r2[c];
                    r2[c] = temp[0][c];
                    a[c] = state2[c];
                    b[c] = state4[c];
                }
                break;
            case 2:
                for(int c = 0; c < 4; ++c)
                {
                    r1[c] = r2[c];
                    r2[c] = temp[1][c];
                    a[c] = state3[c];
                    b[c] = state5[c];
                }
                break;
            case 3:
                for(int c = 0; c < 4; ++c)
                {
                    r1[c] = r2[c];
                    r2[c] = temp[2][c];
                    a[c] = state4[c];
                    b[c] = state1[c];
                }
                break;
            default:
                break;            
                
        }
        
        lshift128(a, shift, e);
        rshift128(r1, shift, f);
        
        temp[i][0] = a[0] ^ e[0] ^ ((b[0] >> thirteen) & mask11) ^ f[0] ^ (r2[0] << fifteen);
        temp[i][1] = a[1] ^ e[1] ^ ((b[1] >> thirteen) & mask12) ^ f[1] ^ (r2[1] << fifteen);
        temp[i][2] = a[2] ^ e[2] ^ ((b[2] >> thirteen) & mask13) ^ f[2] ^ (r2[2] << fifteen);
        temp[i][3] = a[3] ^ e[3] ^ ((b[3] >> thirteen) & mask14) ^ f[3] ^ (r2[3] << fifteen);
 
    }        

    for(int c = 0; c < 4; ++c)
    {    
        temp1[c] = temp[0][c] * one / intMax;
        temp2[c] = temp[1][c] * one / intMax;
    }

    for(int c = 0; c < 4; ++c)
    { 
        // Applying Box Mullar Transformations.
        r[c] = sqrt((-two) * log(temp1[c]));
        phi[c]  = two * PI * temp2[c];
        gaussianRand1[c] = r[c] * cos(phi[c]);
        gaussianRand2[c] = r[c] * sin(phi[c]);
        
        nextRand[c] = temp[2][c];
    }
}

void 
MonteCarloAsianDP::calOutputs(double strikePrice, 
                            double* meanDeriv1, 
                            double*  meanDeriv2, 
                            double* meanPrice1,
                            double* meanPrice2, 
                            double* pathDeriv1, 
                            double* pathDeriv2, 
                            double* priceVec1, 
                            double* priceVec2)
{
    double temp1[4] = {0.0, 0.0, 0.0, 0.0};
	double temp2[4] = {0.0, 0.0, 0.0, 0.0};
	double temp3[4] = {0.0, 0.0, 0.0, 0.0};
	double temp4[4] = {0.0, 0.0, 0.0, 0.0};
	
    double tempDiff1[4] = {0.0, 0.0, 0.0, 0.0};
	double tempDiff2[4] = {0.0, 0.0, 0.0, 0.0};

    for(int c = 0; c < 4; ++c)
    {
    	tempDiff1[c] = meanPrice1[c] - strikePrice;
	    tempDiff2[c] = meanPrice2[c] - strikePrice;
    }
	if(tempDiff1[0] > 0.0)
	{
		temp1[0] = 1.0;
		temp3[0] = tempDiff1[0];
	}
	if(tempDiff1[1] > 0.0)
	{
		temp1[1] = 1.0;
		temp3[1] = tempDiff1[1];
	}
	if(tempDiff1[2] > 0.0)
	{
		temp1[2] = 1.0;
		temp3[2] = tempDiff1[2];
	}
	if(tempDiff1[3] > 0.0)
	{
		temp1[3] = 1.0;
		temp3[3] = tempDiff1[3];
	}

	if(tempDiff2[0] > 0.0)
	{
		temp2[0] = 1.0;
		temp4[0] = tempDiff2[0];
	}
	if(tempDiff2[1] > 0.0)
	{
		temp2[1] = 1.0;
		temp4[1] = tempDiff2[1];
	}
	if(tempDiff2[2] > 0.0)
	{
		temp2[2] = 1.0;
		temp4[2] = tempDiff2[2];
	}
	if(tempDiff2[3] > 0.0)
	{
		temp2[3] = 1.0;
		temp4[3] = tempDiff2[3];
	}
	
    for(int c = 0; c < 4; ++c)
    {
	    pathDeriv1[c] = meanDeriv1[c] * temp1[c]; 
	    pathDeriv2[c] = meanDeriv2[c] * temp2[c]; 
	    priceVec1[c] = temp3[c]; 
	    priceVec2[c] = temp4[c];	
    }
}

void MonteCarloAsianDP::cpuReferenceImpl()
{
    double timeStep = maturity / (noOfSum - 1);
    
    // Initialize random number generator
    srand(1);
    
    for(int k = 0; k < steps; k++)
    {
        double c1 = (interest - 0.5 * sigma[k] * sigma[k]) * timeStep;
        double c2 = sigma[k] * sqrt(timeStep);
        double c3 = (interest + 0.5 * sigma[k] * sigma[k]);  

        for(int j = 0; j < (width * height); j++)
        {
            unsigned int nextRand[4] = {0u, 0u, 0u, 0u};
            for(int c = 0; c < 4; ++c)
                nextRand[c] = (cl_uint)rand();

            double trajPrice1[4] = {initPrice, initPrice, initPrice, initPrice};
            double sumPrice1[4] = {initPrice, initPrice, initPrice, initPrice};
            double sumDeriv1[4] = {0.0, 0.0, 0.0, 0.0};
            double meanPrice1[4] = {0.0, 0.0, 0.0, 0.0};
            double meanDeriv1[4] = {0.0, 0.0, 0.0, 0.0};
            double price1[4] = {0.0, 0.0, 0.0, 0.0};
            double pathDeriv1[4] = {0.0, 0.0, 0.0, 0.0};

            double trajPrice2[4] = {initPrice, initPrice, initPrice, initPrice};
            double sumPrice2[4] = {initPrice, initPrice, initPrice, initPrice};
            double sumDeriv2[4] = {0.0, 0.0, 0.0, 0.0};
            double meanPrice2[4] = {0.0, 0.0, 0.0, 0.0};
            double meanDeriv2[4] = {0.0, 0.0, 0.0, 0.0};
            double price2[4] = {0.0, 0.0, 0.0, 0.0};
            double pathDeriv2[4] = {0.0, 0.0, 0.0, 0.0};

            //Run the Monte Carlo simulation a total of Num_Sum - 1 times
            for(int i = 1; i < noOfSum; i++)
            {
                unsigned int tempRand[4] =  {0u, 0u, 0u, 0u};
                for(int c = 0; c < 4; ++c)
                    tempRand[c] = nextRand[c];

                double gaussian1[4] = {0.0, 0.0, 0.0, 0.0};
                double gaussian2[4] = {0.0, 0.0, 0.0, 0.0};
                generateRand(tempRand, gaussian1, gaussian2, nextRand);
                
                //Calculate the trajectory price and sum price for all trajectories
                for(int c = 0; c < 4; ++c)
                {
                    trajPrice1[c] = trajPrice1[c] * exp(c1 + c2 * gaussian1[c]);
                    trajPrice2[c] = trajPrice2[c] * exp(c1 + c2 * gaussian2[c]);
                    
                    sumPrice1[c] = sumPrice1[c] + trajPrice1[c];
                    sumPrice2[c] = sumPrice2[c] + trajPrice2[c];
                    
                    double temp = c3 * timeStep * i;
                    
                    // Calculate the derivative price for all trajectories
                    sumDeriv1[c] = sumDeriv1[c] + trajPrice1[c] 
                                * ((log(trajPrice1[c] / initPrice) - temp) / sigma[k]);
    
                    sumDeriv2[c] = sumDeriv2[c] + trajPrice2[c] 
                                * ((log(trajPrice2[c] / initPrice) - temp) / sigma[k]);			            
                }
                            
        }
    
            //Calculate the average price and “average derivative” of each simulated path
            for(int c = 0; c < 4; ++c)
            {
                meanPrice1[c] = sumPrice1[c] / noOfSum;
                meanPrice2[c] = sumPrice2[c] / noOfSum;
                meanDeriv1[c] = sumDeriv1[c] / noOfSum;
                meanDeriv2[c] = sumDeriv2[c] / noOfSum;
            }

            calOutputs(strikePrice, meanDeriv1, meanDeriv2, meanPrice1, meanPrice2, 
                        pathDeriv1, pathDeriv2, price1, price2);

            for(int c = 0; c < 4; ++c)
            {
                priceVals[j * 8 + c] = price1[c];
                priceVals[j * 8 + 1 * 4 + c] = price2[c];
                priceDeriv[j * 8 + c] = pathDeriv1[c]; 
                priceDeriv[j * 8 + 1 * 4 + c] = pathDeriv2[c];
            }
        }
       
        /* Replace Following "for" loop with reduction kernel */
        for(int i = 0; i < noOfTraj * noOfTraj; i++)
        {
            refPrice[k] += priceVals[i];
            refVega[k] += priceDeriv[i];
        }

        refPrice[k] /= (noOfTraj * noOfTraj);
        refVega[k] /= (noOfTraj * noOfTraj);

        refPrice[k] = exp(-interest * maturity) * refPrice[k];
        refVega[k] = exp(-interest * maturity) * refVega[k];
    }
}

int MonteCarloAsianDP::verifyResults()
{
    if(verify)
    {
        /* reference implementation
         * it overwrites the input array with the output
         */
        cpuReferenceImpl();

        /* compare the results and see if they match */
        for(int i = 0; i < steps; ++i)
        {
            if(fabs(price[i] - refPrice[i]) > 0.2)
            {
                std::cout << "Failed\n";
                return SDK_FAILURE;
            }
            if(fabs(vega[i] - refVega[i]) > 0.2)
            {
                std::cout << "Failed\n";
                return SDK_FAILURE;
            }
        }
        std::cout << "Passed!\n";
    }

    return SDK_SUCCESS;
}

int MonteCarloAsianDP::cleanup()
{
    /* Releases OpenCL resources (Context, Memory etc.) */
    cl_int status;

    status = clReleaseMemObject(priceBuf);
    if(!sampleCommon->checkVal(status,
                               CL_SUCCESS,
                               "clReleaseMemObject failed."))
    {
        return SDK_FAILURE;
    }

    status = clReleaseMemObject(priceDerivBuf);
    if(!sampleCommon->checkVal(status,
                               CL_SUCCESS,
                               "clReleaseMemObject failed."))
    {
        return SDK_FAILURE;
    }

    status = clReleaseMemObject(randBuf);
    if(!sampleCommon->checkVal(status,
                               CL_SUCCESS,
                               "clReleaseMemObject failed."))
    {
        return SDK_FAILURE;
    }

    status = clReleaseKernel(kernel);
    if(!sampleCommon->checkVal(status,
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

    /* Release program resources (input memory etc.) */
    if(sigma)
    {
        free(sigma);
        sigma = NULL;
    }

    if(price) 
    {
        free(price);
        price = NULL;
    }

    if(vega) 
    {
        free(vega);
        vega = NULL;
    }

    if(refPrice) 
    {
        free(refPrice);
        refPrice = NULL;
    }

    if(refVega) 
    {
        free(refVega);
        refVega = NULL;
    }

    if(randNum)
    {
        #if defined (_WIN32)
            _aligned_free(randNum);
        #else
            free(randNum);
        #endif
        randNum = NULL;
    }

    if(priceVals) 
    {
        free(priceVals);
        priceVals = NULL;
    }

    if(priceDeriv)
    {
        free(priceDeriv);
        priceDeriv = NULL;
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
    MonteCarloAsianDP clMonteCarloAsianDP(
        "OpenCL Monte Carlo simulation for Asian Option Pricing");
    
    /* Initialization */
    if(clMonteCarloAsianDP.initialize() != SDK_SUCCESS)
        return SDK_FAILURE;

    /* Parse command line options */
    if(!clMonteCarloAsianDP.parseCommandLine(argc, argv))
        return SDK_FAILURE;

    if(clMonteCarloAsianDP.isDumpBinaryEnabled())
    {
        return clMonteCarloAsianDP.genBinaryImage();
    }
    else
    {
        /* Setup */
        int returnVal = clMonteCarloAsianDP.setup();
        if(returnVal != SDK_SUCCESS)
            return (returnVal == SDK_EXPECTED_FAILURE) ? SDK_SUCCESS : SDK_FAILURE;

        /* Run */
        if(clMonteCarloAsianDP.run() != SDK_SUCCESS)
            return SDK_FAILURE;

        if(clMonteCarloAsianDP.verifyResults()!=SDK_SUCCESS)
            return SDK_FAILURE;

        /* Cleanup resources created */
        if(clMonteCarloAsianDP.cleanup() != SDK_SUCCESS)
            return SDK_FAILURE;

        /* Print performance statistics */
        clMonteCarloAsianDP.printStats();
    }

    return SDK_SUCCESS;
}
