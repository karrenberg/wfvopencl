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


#include "NBody.hpp"
#if defined __APPLE__
#	include <GLUT/glut.h>
#else
#	include<GL/glut.h>
#endif
#include <cmath>
#if !defined __APPLE__
#	include <malloc.h>
#endif

int numBodies;      /**< No. of particles*/
cl_float* pos;      /**< Output position */
void* me;           /**< Pointing to NBody class */
cl_bool display;

float
NBody::random(float randMax, float randMin)
{
    float result;
    result =(float)rand()/(float)RAND_MAX;

    return ((1.0f - result) * randMin + result *randMax);
}

int
NBody::setupNBody()
{
    // make sure numParticles is multiple of group size
    numParticles = (cl_int)(((size_t)numParticles 
        < groupSize) ? groupSize : numParticles);
    numParticles = (cl_int)((numParticles / groupSize) * groupSize);

    numBodies = numParticles;

    initPos = (cl_float*)malloc(numBodies * sizeof(cl_float4));
    if(initPos == NULL)	
    { 
        sampleCommon->error("Failed to allocate host memory. (initPos)");
        return SDK_FAILURE;
    }

    initVel = (cl_float*)malloc(numBodies * sizeof(cl_float4));
    if(initVel == NULL)	
    { 
        sampleCommon->error("Failed to allocate host memory. (initVel)");
        return SDK_FAILURE;
    }

#if defined (_WIN32)
    pos = (cl_float*)_aligned_malloc(numBodies * sizeof(cl_float4), 16);
#elif defined (__APPLE__)
    pos = (cl_float*)malloc(numBodies * sizeof(cl_float4));
#else
    pos = (cl_float*)memalign(16, numBodies * sizeof(cl_float4));
#endif
    if(pos == NULL)
    { 
        sampleCommon->error("Failed to allocate host memory. (pos)");
        return SDK_FAILURE;
    }

#if defined (_WIN32)
    vel = (cl_float*)_aligned_malloc(numBodies * sizeof(cl_float4), 16);
#elif defined (__APPLE__)
    vel = (cl_float*)malloc(numBodies * sizeof(cl_float4));
#else
    vel = (cl_float*)memalign(16, numBodies * sizeof(cl_float4));
#endif

    if(vel == NULL)
    { 
        sampleCommon->error("Failed to allocate host memory. (vel)");
        return SDK_FAILURE;
    }

    /* initialization of inputs */
    for(int i = 0; i < numBodies; ++i)
    {
        int index = 4 * i;

        // First 3 values are position in x,y and z direction
        for(int j = 0; j < 3; ++j)
        {
            initPos[index + j] = random(3, 50);
        }

        // Mass value
        initPos[index + 3] = random(1, 1000);

        // First 3 values are velocity in x,y and z direction
        for(int j = 0; j < 3; ++j)
        {
            initVel[index + j] = 0.0f;
        }

        // unused
        initVel[3] = 0.0f;
    }

    memcpy(pos, initPos, 4 * numBodies * sizeof(cl_float));
    memcpy(vel, initVel, 4 * numBodies * sizeof(cl_float));

    return SDK_SUCCESS;
}

int 
NBody::genBinaryImage()
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
    kernelPath.append("NBody_Kernels.cl");
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
NBody::setupCL()
{
    cl_int status = CL_SUCCESS;

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

    //Exit if deviceId option is used
    if(isDeviceIdEnabled())
    {
        sampleCommon->expectedError("-d(--deviceId) is not a supported");
        return SDK_EXPECTED_FAILURE;
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

    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clCreateContextFromType failed."))
    {
        return SDK_FAILURE;
    }

    size_t deviceListSize;

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


    /* Create command queue */

    commandQueue = clCreateCommandQueue(
        context,
        devices[deviceId],
        0,
        &status);

    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clCreateCommandQueue failed."))
    {
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


    /*
    * Create and initialize memory objects
    */

    /* Create memory objects for position */
    currPos = clCreateBuffer(
        context,
        CL_MEM_READ_WRITE,
        numBodies * sizeof(cl_float4),
        0,
        &status);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clCreateBuffer failed. (oldPos)"))
    {
        return SDK_FAILURE;
    }

    /* Initialize position buffer */
    status = clEnqueueWriteBuffer(commandQueue,
                                  currPos,
                                  1,
                                  0,
                                  numBodies * sizeof(cl_float4),
                                  pos,
                                  0,
                                  0,
                                  0);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clEnqueueWriteBuffer failed. (oldPos)"))
    {
        return SDK_FAILURE;
    }


    /* Create memory objects for position */
    newPos = clCreateBuffer(
        context,
        CL_MEM_READ_WRITE,
        numBodies * sizeof(cl_float4),
        0,
        &status);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clCreateBuffer failed. (newPos)"))
    {
        return SDK_FAILURE;
    }

    /* Create memory objects for velocity */
    currVel = clCreateBuffer(
        context,
        CL_MEM_READ_WRITE,
        numBodies * sizeof(cl_float4),
        0,
        &status);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clCreateBuffer failed. (oldVel)"))
    {
        return SDK_FAILURE;
    }

    /* Initialize velocity buffer */
    status = clEnqueueWriteBuffer(commandQueue,
                                  currVel,
                                  1,
                                  0,
                                  numBodies * sizeof(cl_float4),
                                  vel,
                                  0,
                                  0,
                                  0);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clEnqueueWriteBuffer failed. (oldVel)"))
    {
        return SDK_FAILURE;
    }

    /* Create memory objects for velocity */
    newVel = clCreateBuffer(
        context,
        CL_MEM_READ_ONLY,
        numBodies * sizeof(cl_float4),
        0,
        &status);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clCreateBuffer failed. (newVel)"))
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
	if (!strcmp(vName, "Intel(R) Corporation")) {
		vendorName = "intel";
	} else if (!strcmp(vName, "Advanced Micro Devices, Inc.")) {
		vendorName = "amd";
	} else if (platformIsPacketizedOpenCL) {
		vendorName = "pkt";
	} else {
		printf("ERROR: vendor not recognized: %s\n", vName);
	}

	kernelPath.append("NBody_Kernels.cl");
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
    kernel = clCreateKernel(
        program,
        "nbody_sim",
        &status);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clCreateKernel failed."))
    {
        return SDK_FAILURE;
    }

    return SDK_SUCCESS;
}


int 
NBody::setupCLKernels()
{
    cl_int status;

    /* Set appropriate arguments to the kernel */

    /* Particle positions */
    status = clSetKernelArg(
        kernel,
        0,
        sizeof(cl_mem),
        (void*)&currPos);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS, 
        "clSetKernelArg failed. (updatedPos)"))
    {
        return SDK_FAILURE;
    }

    /* Particle velocity */
    status = clSetKernelArg(
        kernel,
        1,
        sizeof(cl_mem),
        (void *)&currVel);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS, 
        "clSetKernelArg failed. (updatedVel)"))
    {
        return SDK_FAILURE;
    }

    /* numBodies */
    status = clSetKernelArg(
        kernel,
        2,
        sizeof(cl_int),
        (void *)&numBodies);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS, 
        "clSetKernelArg failed. (numBodies)"))
    {
        return SDK_FAILURE;
    }

    /* time step */
    status = clSetKernelArg(
        kernel,
        3,
        sizeof(cl_float),
        (void *)&delT);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS, 
        "clSetKernelArg failed. (delT)"))
    {
        return SDK_FAILURE;
    }

    /* upward Pseudoprobability */
    status = clSetKernelArg(
        kernel,
        4,
        sizeof(cl_float),
        (void *)&espSqr);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS, 
        "clSetKernelArg failed. (espSqr)"))
    {
        return SDK_FAILURE;
    }


    /* local memory */
    status = clSetKernelArg(
        kernel,
        5,
        GROUP_SIZE * 4 * sizeof(float),
        NULL);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS, 
        "clSetKernelArg failed. (localPos)"))
    {
        return SDK_FAILURE;
    }

    /* Particle positions */
    status = clSetKernelArg(
        kernel,
        6,
        sizeof(cl_mem),
        (void*)&newPos);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS, 
        "clSetKernelArg failed. (unewPos)"))
    {
        return SDK_FAILURE;
    }

    /* Particle velocity */
    status = clSetKernelArg(
        kernel,
        7,
        sizeof(cl_mem),
        (void *)&newVel);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS, 
        "clSetKernelArg failed. (newVel)"))
    {
        return SDK_FAILURE;
    }

    status = clGetKernelWorkGroupInfo(kernel,
        devices[deviceId],
        CL_KERNEL_LOCAL_MEM_SIZE,
        sizeof(cl_ulong),
        &usedLocalMemory,
        NULL);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS, 
        "clGetKernelWorkGroupInfo CL_KERNEL_LOCAL_MEM_SIZE failed."))
    {
        return SDK_FAILURE;
    }

    if(usedLocalMemory > totalLocalMemory)
    {
        std::cout << "Unsupported: Insufficient local memory on device" <<
            std::endl;
        return SDK_FAILURE;
    }

    /* Check group size against group size returned by kernel */
    status = clGetKernelWorkGroupInfo(kernel,
        devices[deviceId],
        CL_KERNEL_WORK_GROUP_SIZE,
        sizeof(size_t),
        &kernelWorkGroupSize,
        0);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS, 
        "clGetKernelWorkGroupInfo CL_KERNEL_COMPILE_WORK_GROUP_SIZE failed."))
    {
        return SDK_FAILURE;
    }

    if(groupSize > kernelWorkGroupSize)
    {
        if(!quiet)
        {
            std::cout << "Out of Resources!" << std::endl;
            std::cout << "Group Size specified : " << groupSize << std::endl;
            std::cout << "Max Group Size supported on the kernel : "
                      << kernelWorkGroupSize << std::endl;
            std::cout << "Falling back to " << kernelWorkGroupSize << std::endl;
        }
        groupSize = kernelWorkGroupSize;
    }

    return SDK_SUCCESS;
}

int 
NBody::runCLKernels()
{
    cl_int status;
    cl_event events[1];

    /* 
    * Enqueue a kernel run call.
    */
    size_t globalThreads[] = {numBodies};
    size_t localThreads[] = {groupSize};

    if(localThreads[0] > maxWorkItemSizes[0] ||
       localThreads[0] > maxWorkGroupSize)
    {
        std::cout << "Unsupported: Device"
            "does not support requested number of work items.";
        return SDK_FAILURE;
    }

    status = clEnqueueNDRangeKernel(
        commandQueue,
        kernel,
        1,
        NULL,
        globalThreads,
        localThreads,
        0,
        NULL,
        NULL);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS, 
        "clEnqueueNDRangeKernel failed."))
    {
        return SDK_FAILURE;
    }

    status = clFinish(commandQueue);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS, 
        "clFinish failed."))
    {
        return SDK_FAILURE;
    }

    /* Copy data from new to old */
    status = clEnqueueCopyBuffer(commandQueue,
                                 newPos,
                                 currPos,
                                 0,
                                 0,
                                 sizeof(cl_float4) * numBodies,
                                 0,
                                 0,
                                 0);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS, 
        "clEnqueueCopyBuffer failed.(newPos->oldPos)"))
    {
        return SDK_FAILURE;
    }

    status = clEnqueueCopyBuffer(commandQueue,
                                 newVel,
                                 currVel,
                                 0,
                                 0,
                                 sizeof(cl_float4) * numBodies,
                                 0,
                                 0,
                                 0);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS, 
        "clEnqueueCopyBuffer failed.(newVel->oldVels)"))
    {
        return SDK_FAILURE;
    }

    status = clFinish(commandQueue);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS, 
        "clFinish failed."))
    {
        return SDK_FAILURE;
    }

    /* Enqueue readBuffer*/
    status = clEnqueueReadBuffer(
        commandQueue,
        currPos,
        CL_TRUE,
        0,
        numBodies* sizeof(cl_float4),
        pos,
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

    clReleaseEvent(events[0]);

    return SDK_SUCCESS;
}

/*
* n-body simulation on cpu
*/
void 
NBody::nBodyCPUReference()
{
    //Iterate for all samples
    for(int i = 0; i < numBodies; ++i)
    {
        int myIndex = 4 * i;
        float acc[3] = {0.0f, 0.0f, 0.0f};
        for(int j = 0; j < numBodies; ++j)
        {
            float r[3];
            int index = 4 * j;

            float distSqr = 0.0f;
            for(int k = 0; k < 3; ++k)
            {
                r[k] = refPos[index + k] - refPos[myIndex + k];

                distSqr += r[k] * r[k];
            }

            float invDist = 1.0f / sqrt(distSqr + espSqr);
            float invDistCube =  invDist * invDist * invDist;
            float s = refPos[index + 3] * invDistCube;

            for(int k = 0; k < 3; ++k)
            {
                acc[k] += s * r[k];
            }
        }

        for(int k = 0; k < 3; ++k)
        {
            refPos[myIndex + k] += refVel[myIndex + k] * delT + 0.5f * acc[k] * delT * delT;
            refVel[myIndex + k] += acc[k] * delT;
        }
    }
}

int
NBody::initialize()
{
    /* Call base class Initialize to get default configuration */
    if(!this->SDKSample::initialize())
        return SDK_FAILURE;

    streamsdk::Option *num_particles = new streamsdk::Option;
    if(!num_particles)
    {
        std::cout << "error. Failed to allocate memory (num_particles)\n";
        return SDK_FAILURE;
    }

    num_particles->_sVersion = "x";
    num_particles->_lVersion = "particles";
    num_particles->_description = "Number of particles";
    num_particles->_type = streamsdk::CA_ARG_INT;
    num_particles->_value = &numParticles;

    sampleArgs->AddOption(num_particles);
    delete num_particles;

    streamsdk::Option *num_iterations = new streamsdk::Option;
    if(!num_iterations)
    {
        std::cout << "error. Failed to allocate memory (num_iterations)\n";
        return SDK_FAILURE;
    }

    num_iterations->_sVersion = "i";
    num_iterations->_lVersion = "iterations";
    num_iterations->_description = "Number of iterations";
    num_iterations->_type = streamsdk::CA_ARG_INT;
    num_iterations->_value = &iterations;

    sampleArgs->AddOption(num_iterations);
    delete num_iterations;

    return SDK_SUCCESS;
}

int
NBody::setup()
{
    if(setupNBody() != SDK_SUCCESS)
        return SDK_FAILURE;

    int timer = sampleCommon->createTimer();
    sampleCommon->resetTimer(timer);
    sampleCommon->startTimer(timer);

    cl_int retValue = setupCL();
    if(retValue != SDK_SUCCESS)
        return retValue;

    sampleCommon->stopTimer(timer);
    /* Compute setup time */
    setupTime = (double)(sampleCommon->readTimer(timer));

    display = !quiet && !verify;

    return SDK_SUCCESS;
}

/** 
* @brief Initialize GL 
*/
void 
GLInit()
{
    glClearColor(0.0 ,0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT);
    glClear(GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);	
    glLoadIdentity();
}

/** 
* @brief Glut Idle function
*/
void 
idle()
{
    glutPostRedisplay();
}

/** 
* @brief Glut reshape func
* 
* @param w numParticles of OpenGL window
* @param h height of OpenGL window 
*/
void 
reShape(int w,int h)
{
    glViewport(0, 0, w, h);

    glViewport(0, 0, w, h);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluPerspective(45.0f, w/h, 1.0f, 1000.0f);
    gluLookAt (0.0, 0.0, -2.0, 0.0, 0.0, 1.0, 0.0, 1.0, 0.0);
}

/** 
* @brief OpenGL display function
*/
void displayfunc()
{
    glClearColor(0.0 ,0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT);
    glClear(GL_DEPTH_BUFFER_BIT);

    glPointSize(1.0);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glEnable(GL_BLEND);
    glDepthMask(GL_FALSE);

    glColor3f(1.0f,0.6f,0.0f);

    //Calling kernel for calculatig subsequent positions
    ((NBody*)me)->runCLKernels();

    glBegin(GL_POINTS);
    for(int i=0; i < numBodies; ++i)
    {
        //divided by 300 just for scaling
        glVertex3d(pos[i*4+ 0]/300, pos[i*4+1]/300, pos[i*4+2]/300);
    }
    glEnd();

    glFlush();
    glutSwapBuffers();
}

/* keyboard function */
void
keyboardFunc(unsigned char key, int mouseX, int mouseY)
{
    switch(key)
    {
        /* If the user hits escape or Q, then exit */
        /* ESCAPE_KEY = 27 */
    case 27:
    case 'q':
    case 'Q':
        {
            if(((NBody*)me)->cleanup() != SDK_SUCCESS)
                exit(1);
            else
                exit(0);
        }
    default:
        break;
    }
}


int 
NBody::run()
{
    /* Arguments are set and execution call is enqueued on command buffer */
    if(setupCLKernels() != SDK_SUCCESS)
    {
        return SDK_FAILURE;
    }

    if(verify || timing)
    {
        int timer = sampleCommon->createTimer();
        sampleCommon->resetTimer(timer);
        sampleCommon->startTimer(timer);

        for(int i = 0; i < iterations; ++i)
        {
            runCLKernels();
        }

        sampleCommon->stopTimer(timer);
        /* Compute kernel time */
        kernelTime = (double)(sampleCommon->readTimer(timer)) / iterations;
    }

    if(!quiet)
    {
        sampleCommon->printArray<cl_float>("Output", pos, numBodies, 1);
    }

    return SDK_SUCCESS;
}

int
NBody::verifyResults()
{
    if(verify)
    {
        /* reference implementation
        * it overwrites the input array with the output
        */

        refPos = (cl_float*)malloc(numBodies * sizeof(cl_float4));
        if(refPos == NULL)
        { 
            sampleCommon->error("Failed to allocate host memory. (refPos)");
            return SDK_FAILURE;
        }

        refVel = (cl_float*)malloc(numBodies * sizeof(cl_float4));
        if(refVel == NULL)
        { 
            sampleCommon->error("Failed to allocate host memory. (refVel)");
            return SDK_FAILURE;
        }

        memcpy(refPos, initPos, 4 * numBodies * sizeof(cl_float));
        memcpy(refVel, initVel, 4 * numBodies * sizeof(cl_float));

        for(int i = 0; i < iterations; ++i)
        {
            nBodyCPUReference();
        }

        /* compare the results and see if they match */
        if(sampleCommon->compare(pos, refPos, 4 * numBodies, 0.00001))
        {
            std::cout << "Passed!\n";
            return SDK_SUCCESS;
        }
        else
        {
            std::cout << "Failed!\n";
            return SDK_FAILURE;
        }
    }

    return SDK_SUCCESS;
}

void 
NBody::printStats()
{
    std::string strArray[4] = 
    {
        "Particles", 
        "Iterations", 
        "Time(sec)", 
        "kernelTime(sec)"
    };

    std::string stats[4];
    totalTime = setupTime + kernelTime;

    stats[0] = sampleCommon->toString(numParticles, std::dec);
    stats[1] = sampleCommon->toString(iterations, std::dec);
    stats[2] = sampleCommon->toString(totalTime, std::dec);
    stats[3] = sampleCommon->toString(kernelTime, std::dec);

    this->SDKSample::printStats(strArray, stats, 4);
    this->SDKSample::logStats(kernelTime, "NBody", vendorName);
}

int
NBody::cleanup()
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
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clReleaseProgram failed."))
    {
        return SDK_FAILURE;
    }

    status = clReleaseMemObject(currPos);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clReleaseMemObject failed."))
    {
        return SDK_FAILURE;
    }

    status = clReleaseMemObject(currVel);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clReleaseMemObject failed."))
    {
        return SDK_FAILURE;
    }

    status = clReleaseCommandQueue(commandQueue);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clReleaseCommandQueue failed."))
    {
        return SDK_FAILURE;
    }

    status = clReleaseContext(context);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clReleaseContext failed."))
    {
        return SDK_FAILURE;
    }

    return SDK_SUCCESS;
}

NBody::~NBody()
{
    /* release program resources */
    if(initPos)
    {
        free(initPos);
        initPos = NULL;
    }

    if(initVel)
    {
        free(initVel);
        initVel = NULL;
    }

    if(pos)
    {
#if defined (_WIN32)
        _aligned_free(pos);
#else
        free(pos);
#endif
        pos = NULL;
    }
    if(vel)
    {
#if defined (_WIN32)
        _aligned_free(vel);
#else
        free(vel);
#endif
        vel = NULL;
    }

    if(devices)
    {
        free(devices);
        devices = NULL;
    }

    if(refPos)
    {
        free(refPos);
        refPos = NULL;
    }

    if(refVel)
    {
        free(refVel);
        refVel = NULL;
    }

    if(maxWorkItemSizes)
    {
        free(maxWorkItemSizes);
        maxWorkItemSizes = NULL;
    }
}


int 
main(int argc, char * argv[])
{
    NBody clNBody("OpenCL NBody");
    me = &clNBody;

    if(clNBody.initialize() != SDK_SUCCESS)
        return SDK_FAILURE;
    if(!clNBody.parseCommandLine(argc, argv))
        return SDK_FAILURE;

    if(clNBody.isDumpBinaryEnabled())
    {
        return clNBody.genBinaryImage();
    }
    else
    {
        cl_int retValue = clNBody.setup();
        if(retValue != SDK_SUCCESS)
            return (retValue == SDK_EXPECTED_FAILURE) ? SDK_SUCCESS : SDK_FAILURE;

        if(clNBody.run() != SDK_SUCCESS)
            return SDK_FAILURE;
        if(clNBody.verifyResults() != SDK_SUCCESS)
            return SDK_FAILURE;

        clNBody.printStats();

        if(display)
        {
            // Run in  graphical window if requested 
            glutInit(&argc, argv);
            glutInitWindowPosition(100,10);
            glutInitWindowSize(600,600); 
            glutInitDisplayMode( GLUT_RGB | GLUT_DOUBLE );
            glutCreateWindow("nbody simulation"); 
            GLInit(); 
            glutDisplayFunc(displayfunc); 
            glutReshapeFunc(reShape);
            glutIdleFunc(idle); 
            glutKeyboardFunc(keyboardFunc);
            glutMainLoop();
        }

        if(clNBody.cleanup()!=SDK_SUCCESS)
            return SDK_FAILURE;
    }

    return SDK_SUCCESS;
}
