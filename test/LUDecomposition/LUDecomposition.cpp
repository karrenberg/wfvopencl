
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


#include "LUDecomposition.hpp"
#define VECTOR_SIZE 4
#define SIZE effectiveDimension * effectiveDimension * sizeof(double)

int LUD::setupLUD()
{
    /*
     *Check if openCL 1.1 is supported
     */
#ifndef CL_VERSION_1_1
        sampleCommon->expectedError("OpenCL 1.1 is not supported. It is necessary for this sample");
        return SDK_EXPECTED_FAILURE;
#endif
    
    /*
     *Changing dimension to a multiple of block if it is not.
     *Currently execution will be done on the effective dimention
     *and results will be shown upto what user requests 
     */
    

    if(actualDimension % VECTOR_SIZE != 0)
        effectiveDimension = actualDimension 
                                - (actualDimension % VECTOR_SIZE) 
                                + VECTOR_SIZE;
    else
        effectiveDimension = actualDimension;

    blockSize = effectiveDimension / VECTOR_SIZE;
    
#ifdef _WIN32
    input = static_cast<double*>(_aligned_malloc(SIZE, 4096));
#else
    input = static_cast<double*>(memalign(4096, SIZE));
#endif
    if(input == NULL)
    {
        sampleCommon->error("Unable to allocate input memory");
        return SDK_FAILURE;
    }

    //initialize with random double type elements
    sampleCommon->fillRandom<double>(
                        input,
                        effectiveDimension, 
                        effectiveDimension,
                        1,
                        2,
                        1);

#ifdef _WIN32
    matrixGPU = static_cast<double*>(_aligned_malloc(SIZE, 4096));
#else
    matrixGPU = static_cast<double*>(memalign(4096, SIZE));
#endif
    if(matrixGPU == NULL)
    {
        sampleCommon->error("Unable to allocate memory for GPU input");
        return SDK_FAILURE;
    }
    
    if(verify)
    {
        matrixCPU = static_cast<double*>(malloc(SIZE));
        if(matrixCPU == NULL)
        {
            sampleCommon->error("Unable to allocate memory for refernce implementation");
                return SDK_FAILURE;
        }

        memcpy((void*)matrixCPU, (const void*)input, SIZE);
    }

    if(!quiet)
    {
        sampleCommon->printArray<double>(
                        "Input Matrix",
                        input,
                        actualDimension,
                        actualDimension);
    }

    return SDK_SUCCESS;
}

int LUD::genBinaryImage()
{
    cl_int status = CL_SUCCESS;

    /*
     * Have a look at the available platforms and pick either
     * the AMD one if available or a reasonable default.
     */
    cl_platform_id platform = NULL;
    cl_uint numPlatforms = 0;
    cl_char platformName[100];
    
    status = clGetPlatformIDs(
                    0,
                    NULL,
                    &numPlatforms);
    if(!sampleCommon->checkVal<cl_int>(
            status,
            CL_SUCCESS,
            "Error Getting number of platforms"))
        return SDK_FAILURE;
    
    if (0 < numPlatforms) 
    {
        cl_platform_id *platforms = new cl_platform_id[numPlatforms];
        
        status = clGetPlatformIDs(numPlatforms, platforms, NULL);
        if(!sampleCommon->checkVal<cl_int>(
                status,
                CL_SUCCESS,
                "Error Getting  platforms"))
            return SDK_FAILURE;

        for(unsigned int i = 0; i < numPlatforms; i++)
        {
            status = clGetPlatformInfo(
                            platforms[i],
                            CL_PLATFORM_VENDOR,
                            sizeof(platformName),
                            platformName,
                            NULL);
            if(! sampleCommon->checkVal<cl_int>(
                    status,
                    CL_SUCCESS,
                    "Error Getting  platforms info"))
                return SDK_FAILURE;

            platform = platforms[i];
            if(!strcmp(
                    (const char*)platformName, 
                    "Advanced Micro Devices, Inc."))
                break;
        }
        std::cout<<"Platform Found: " << platformName<<"\n";
    }

    
    //creating context
    cl_context_properties cps[] = {CL_CONTEXT_PLATFORM,
                                   (cl_context_properties)platform,
                                   CL_CONTEXT_OFFLINE_DEVICES_AMD,
                                   (cl_context_properties)1,0};

    context = clCreateContextFromType(cps,
                                CL_DEVICE_TYPE_ALL,
                                NULL,
                                NULL,
                                &status);
    if(!sampleCommon->checkVal<cl_int>(
            status,
            CL_SUCCESS,
           "Error creating context"))
        return SDK_FAILURE;

    // create a CL program using the kernel source 
    streamsdk::SDKFile kernelFile;
    std::string kernelPath = sampleCommon->getPath();
    kernelPath.append("LUDecomposition_Kernels.cl");
    if(!kernelFile.open(kernelPath.c_str()))
    {
        std::cout << "Failed to load kernel file : " << kernelPath << std::endl;
        return SDK_FAILURE;
    }
    const char * source = kernelFile.source().c_str();
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

    std::string flagsStr = std::string("");

    // Get additional options
    if(isComplierFlagsSpecified())
    {
        streamsdk::SDKFile flagsFile;
        std::string flagsPath = sampleCommon->getPath();
        flagsPath.append(flags.c_str());
        if(!flagsFile.open(flagsPath.c_str()))
        {
            std::cout << "Failed to load flags file: " 
                      << flagsPath 
                      << std::endl;
            return SDK_FAILURE;
        }
        flagsFile.replaceNewlineWithSpaces();
        const char * flags = flagsFile.source().c_str();
        flagsStr.append(flags);
    }

    if(flagsStr.size() != 0)
        std::cout << "Build Options are : " << flagsStr.c_str() << std::endl;


    // create a cl program executable for all the devices specified 
    status = clBuildProgram(program, 0, NULL, flagsStr.c_str(), NULL, NULL);
    sampleCommon->checkVal(status, CL_SUCCESS, "clBuildProgram failed.");
    size_t numDevices;
    status = clGetProgramInfo(
                    program,
                    CL_PROGRAM_NUM_DEVICES,
                    sizeof(numDevices),
                    &numDevices,
                    NULL );
    if(!sampleCommon->checkVal(
            status,
            CL_SUCCESS,
            "clGetProgramInfo(CL_PROGRAM_NUM_DEVICES) failed."))
           return SDK_FAILURE;
    
    std::cout << "Number of devices found : " << numDevices << "\n\n";
    devices = (cl_device_id *)malloc(sizeof(cl_device_id) * numDevices);
    if(devices == NULL)
    {
        sampleCommon->error("Failed to allocate host memory.(devices)");
        return SDK_FAILURE;
    }
    
    // grab the handles to all of the devices in the program. 
    status = clGetProgramInfo(
                    program,
                    CL_PROGRAM_DEVICES,
                    sizeof(cl_device_id) * numDevices,
                    devices,NULL);
    if(!sampleCommon->checkVal(
            status,
            CL_SUCCESS,
            "clGetProgramInfo(CL_PROGRAM_DEVICES) failed."))
            return SDK_FAILURE;
    
    // figure out the sizes of each of the binaries. 
    size_t *binarySizes = (size_t*)malloc(sizeof(size_t) * numDevices);
    if(devices == NULL)
    {
        sampleCommon->error("Failed to allocate host memory.(binarySizes)");
        return SDK_FAILURE;
    }
    status = clGetProgramInfo(
                program,
                CL_PROGRAM_BINARY_SIZES,
                sizeof(size_t) * numDevices,
                binarySizes,
                NULL);
    if(!sampleCommon->checkVal(
            status,
            CL_SUCCESS,
            "clGetProgramInfo(CL_PROGRAM_BINARY_SIZES) failed."))
            return SDK_FAILURE;

    size_t i = 0;
    // copy over all of the generated binaries. 
    char **binaries = (char **)malloc(sizeof(char *) * numDevices);
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
    status = clGetProgramInfo(
                program,
                CL_PROGRAM_BINARIES,
                sizeof(char *) * numDevices, 
                binaries,
                NULL);
    if(!sampleCommon->checkVal(
            status,
            CL_SUCCESS,
            "clGetProgramInfo(CL_PROGRAM_BINARIES) failed."))
            return SDK_FAILURE;
    
    // dump out each binary into its own separate file. 
    for(i = 0; i < numDevices; i++)
    {
        char fileName[100];
        sprintf(fileName, "%s.%d", dumpBinary.c_str(), (int)i);
        if(binarySizes[i] != 0)
        {
            char deviceName[1024];
            status = clGetDeviceInfo(
                        devices[i],
                        CL_DEVICE_NAME, 
                        sizeof(deviceName),
                        deviceName,
                        NULL);
            if(!sampleCommon->checkVal(
                    status,
                    CL_SUCCESS,
                    "clGetDeviceInfo(CL_DEVICE_NAME) failed."))
                   return SDK_FAILURE;
            
            printf( "%s binary kernel: %s\n", deviceName, fileName);
            streamsdk::SDKFile BinaryFile;
            if(!BinaryFile.writeBinaryToFile(
                    fileName,
                    binaries[i],
                    binarySizes[i]))
            {
                std::cout << "Failed to load kernel file : " 
                          << fileName << std::endl;
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
    if(!sampleCommon->checkVal(
            status,
            CL_SUCCESS,
            "clReleaseProgram failed."))
           return SDK_FAILURE;
    

    status = clReleaseContext(context);
    if(!sampleCommon->checkVal(
            status,
            CL_SUCCESS,
            "clReleaseContext failed."))
            return SDK_FAILURE;
    
    return SDK_SUCCESS;
}

int LUD::setupCL(void)
{
    cl_int status = 0;
    size_t deviceListSize;
    cl_device_type dType;
    if(deviceType.compare("cpu") == 0)
           dType = CL_DEVICE_TYPE_CPU;
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
    if(!sampleCommon->checkVal(
            status,
            CL_SUCCESS,
            "clGetPlatformIDs failed."))
           return SDK_FAILURE;
    
    if (0 < numPlatforms) 
    {
        cl_platform_id* platforms = new cl_platform_id[numPlatforms];
        status = clGetPlatformIDs(numPlatforms, platforms, NULL);
        if(!sampleCommon->checkVal(
                status,
                CL_SUCCESS,
                "clGetPlatformIDs failed."))
            return SDK_FAILURE;
        
        if(isPlatformEnabled())
            platform = platforms[platformId];
        else
        {
        for(unsigned i = 0; i < numPlatforms; ++i) 
        {
            char pbuf[100];
            status = clGetPlatformInfo(
                            platforms[i],
                            CL_PLATFORM_VENDOR,
                            sizeof(pbuf),
                            pbuf,
                            NULL);
            if(!sampleCommon->checkVal(status,
                    CL_SUCCESS,
                    "clGetPlatformInfo failed."))
                return SDK_FAILURE;
            
            platform = platforms[i];
            if(!strcmp(pbuf, "Advanced Micro Devices, Inc.")) 
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

    context = clCreateContextFromType(cps, dType, NULL, NULL, &status);
    if(!sampleCommon->checkVal(
            status,
            CL_SUCCESS,
            "clCreateContextFromType failed."))
        return SDK_FAILURE;

    // First, get the size of device list data 
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
    if(!sampleCommon -> validateDeviceId(deviceId, deviceCount))
    {
        sampleCommon->error("sampleCommon::validateDeviceId() failed");
        return SDK_FAILURE;
    }

    // Now allocate memory for device list based on the size we got earlier 
    devices = (cl_device_id *)malloc(deviceListSize);
    if(devices == NULL)
    {
        sampleCommon->error("Failed to allocate memory (devices).");
        return SDK_FAILURE;
    }

    // Now, get the device list data 
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
  
    // The block is to move the declaration of prop closer to its use 
    cl_command_queue_properties prop = 0;
    commandQueue = clCreateCommandQueue(
                        context,
                        devices[deviceId], 
                        prop,
                        &status);
    if(!sampleCommon->checkVal(
            status,
            CL_SUCCESS,
            "clCreateCommandQueue failed."))
        return SDK_FAILURE;

    /*
     *To check if double is supported
     *in the device
     */
    char* deviceExtensions = NULL;;
    size_t extStringSize = 0;

    // Get device extensions 
    status = clGetDeviceInfo(
                    devices[deviceId],
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

    status = clGetDeviceInfo(
                    devices[deviceId],
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
            if(devices)
                free(devices);
            delete deviceExtensions;

            return SDK_EXPECTED_FAILURE;
        }
    }

    delete deviceExtensions;
    
    // Total Local Memory Available
    status = clGetDeviceInfo(
                    devices[deviceId],
                    CL_DEVICE_LOCAL_MEM_SIZE,
                    sizeof(cl_ulong),
                    &totalLocalMemory,
                    NULL);
    if(!sampleCommon->checkVal(
            status,
            CL_SUCCESS,
            "clGetDeviceInfo failed (CL_DEVICE_LOCAL_MEM_SIZE)"))
        return SDK_FAILURE;


    // Maximum Work Group Size supported by Device
    status = clGetDeviceInfo(
                devices[deviceId],
                CL_DEVICE_MAX_WORK_GROUP_SIZE,
                sizeof(size_t),
                &maxWorkGroupSize,
                NULL);
    if(!sampleCommon->checkVal(
            status,
            CL_SUCCESS,
        "clGetDeviceInfo failed (CL_DEVICE_MAX_WORK_GROUP_SIZE)"))
        return SDK_FAILURE;

    //Local Memory Required in worst case
    localMemoryNeeded = (cl_ulong)((maxWorkGroupSize / 2) * sizeof(cl_double));

    status = setupLUD();
    if(status != SDK_SUCCESS)
        return status;  

    //Creating Buffers
    inplaceBuffer = clCreateBuffer(
                        context,
                        CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
                        sizeof(double) * effectiveDimension * effectiveDimension,
                        matrixGPU,
                        &status);
    if(!sampleCommon->checkVal(
            status,
            CL_SUCCESS,
            "clCreateBuffer failed. (inputBuffer)"))
        return SDK_FAILURE;

    inputBuffer2 = clCreateBuffer(
                        context,
                        CL_MEM_WRITE_ONLY,
                        sizeof(double) * effectiveDimension * effectiveDimension,
                        NULL,
                        &status);
    if(!sampleCommon->checkVal(
            status,
            CL_SUCCESS,
            "clCreateBuffer failed. (outputBuffer)"))
        return SDK_FAILURE;


    // create a CL program using the kernel source 
    streamsdk::SDKFile kernelFile;
    std::string kernelPath = sampleCommon->getPath();
    
    if(isLoadBinaryEnabled())
    {
        kernelPath.append(loadBinary.c_str());
        if(!kernelFile.readBinaryFromFile(kernelPath.c_str()))
        {
            std::cout << "Failed to load kernel file : " 
                      << kernelPath << std::endl;
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

	kernelPath.append("LUDecomposition_Kernels.cl");
	if(!kernelFile.open(kernelPath.c_str()))
	{
		std::cout << "Failed to load kernel file : " << kernelPath << std::endl;
		return SDK_FAILURE;
	}

	const char * source = platformIsPacketizedOpenCL ?
		"LUDecomposition_Kernels.bc" :
		kernelFile.source().c_str();

        size_t sourceSize[] = {strlen(source)};
        program = clCreateProgramWithSource(context,
                                            1,
                                            &source,
                                            sourceSize,
                                            &status);
        if(!sampleCommon->checkVal(status,
                                   CL_SUCCESS,
                                   "clCreateProgramWithSource failed."))
            return SDK_FAILURE;
    }
   
    
    std::string flagsStr = std::string("");

    // Get build options if any
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

    // create a cl program executable for all the devices specified 
    status = clBuildProgram(
                    program, 
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
            logStatus = clGetProgramBuildInfo(
                            program,
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

            logStatus = clGetProgramBuildInfo(
                            program,
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

    // get a kernel object handle for a kernel with the given name 
    kernelLUD = clCreateKernel(program, "kernelLUDecompose", &status);
    if(!sampleCommon->checkVal(
            status,
            CL_SUCCESS,
            "clCreateKernel failed. : kernelLUD"))
        return SDK_FAILURE;
    
    kernelCombine = clCreateKernel(program, "kernelLUCombine", &status);
    if(!sampleCommon->checkVal(
            status,
            CL_SUCCESS,
            "clCreateKernel failed. : kernelCombine"))
        return SDK_FAILURE;

    return SDK_SUCCESS;
}

int LUD::runCLKernels(void)
{
    cl_int   status;
    cl_event events[2];

   /*
    *This code will create the memory objects back for 
    *the other iterations if they are present
    */
    
    status = clEnqueueWriteBuffer(
                    commandQueue,
                    inplaceBuffer,
                    CL_TRUE,
                    0,
                    effectiveDimension * effectiveDimension * sizeof(double),
                    (const void*)input,
                    0,
                    NULL,
                    NULL);
    if(!sampleCommon->checkVal(
            status,
            CL_SUCCESS,
            "clEnqueueWriteBuffer failed."))
        return SDK_FAILURE;

    size_t globalThreads[2]= {blockSize, effectiveDimension};
    size_t localThreads[2] = {blockSize, 1};
    size_t offset[2] = {0, 0};

    /*
     *Querying some important device paramneters
     *Finding out the appropritate Work group size for the kernel	 
     * See if local memory can be used 
     * Check group size against kernelWorkGroupSize 
     */

    status = clGetKernelWorkGroupInfo(
                    kernelLUD,
                    devices[deviceId],
                    CL_KERNEL_WORK_GROUP_SIZE,
                    sizeof(size_t),
                    &kernelWorkGroupSize,
                    0);
    if(!sampleCommon->checkVal(
            status,
            CL_SUCCESS, 
            "clGetKernelWorkGroupInfo (CL_KERNEL_WORK_GROUP_SIZE) failed."))
        return SDK_FAILURE;
    
    status = clGetKernelWorkGroupInfo(
                    kernelLUD,
                    devices[deviceId],
                    CL_KERNEL_LOCAL_MEM_SIZE,
                    sizeof(cl_ulong),
                    &localMemoryUsed,
                    0);
    if(!sampleCommon->checkVal(
            status,
            CL_SUCCESS, 
            "clGetKernelWorkGroupInfo (CL_KERNEL_LOCAL_MEM_SIZE) failed."))
        return SDK_FAILURE;

    /*
     *Setting Common Kernel Arguments
     */
    {
        //kernelLUD
        // 1st kernel argument - output 
        status = clSetKernelArg(
                        kernelLUD,
                        0,
                        sizeof(cl_mem),
                        (void *)&inputBuffer2);
        if(!sampleCommon->checkVal(
                status,
                CL_SUCCESS,
                "clSetKernelArg failed. :kernelLUD(inputBuffer2)"))
            return SDK_FAILURE;

        // 2nd kernel argument - input 
        status = clSetKernelArg(
                        kernelLUD,
                        1,
                        sizeof(cl_mem),
                        (void *)&inplaceBuffer);
        if(!sampleCommon->checkVal(
                status,
                CL_SUCCESS,
                "clSetKernelArg failed. :kernelLUD(inplaceBuffer)"))
            return SDK_FAILURE;
    }

    for(int index = 0;index < effectiveDimension - 1;++index)
    {
        /*
         *  code to find :
         *  1. new offset 
         *  2. Local Work group size
         *  3. Global ND range
         */
        if(index % VECTOR_SIZE == 0)
        {
            offset[0] = (index / VECTOR_SIZE);
            offset[1] = VECTOR_SIZE * (index / VECTOR_SIZE);
        
            if(!index)
            {
                globalThreads[0] += 1;
                globalThreads[1] += VECTOR_SIZE;
            }
            globalThreads[0] -= 1;
            globalThreads[1] -= VECTOR_SIZE;
        
            if(globalThreads[0] <= kernelWorkGroupSize)
                localThreads[0] = globalThreads[0];
            else
            {
                size_t temp = (int)kernelWorkGroupSize;
                for(;temp > 1;temp--)
                {
                    if(globalThreads[0] % temp == 0)
                        break;
                }
                localThreads[0] = temp;
            }

            if( globalThreads[1] < kernelWorkGroupSize / localThreads[0])
                localThreads[1] = globalThreads[1];
            else
            {
                size_t temp = kernelWorkGroupSize / localThreads[0];
                for(;temp > 1; temp--)
                {
                    if(globalThreads[1] % temp == 0)
                        break;
                }
                localThreads[1] = temp;
            }
        }
        
        // 3rd kernel argument - index for the matrix 
        status = clSetKernelArg(
                        kernelLUD,
                        2,
                        sizeof(cl_uint),
                        &index);
        if(!sampleCommon->checkVal(
                status,
                CL_SUCCESS,
                "clSetKernelArg failed. :kernelLUD(index)"))
            return SDK_FAILURE;

        // 4th kernel argument - local spcae for holding ratio  
        status = clSetKernelArg(
                        kernelLUD,
                        3,
                        sizeof(cl_double) * localThreads[1],
                        NULL);
        if(!sampleCommon->checkVal(
                status,
                CL_SUCCESS,
                "clSetKernelArg failed. :kernelLUD(localBuffer)"))
            return SDK_FAILURE;
            
        /* 
         * Enqueue a kernel run call.
         * kernelLUD
         */
        status = clEnqueueNDRangeKernel(
                        commandQueue,
                        kernelLUD,
                        2,
                        offset,
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

        // wait for the kernel call to finish execution 
        status = clFinish(commandQueue);
        if(!sampleCommon->checkVal(
                status,
                CL_SUCCESS,
                "clFlush Failed."))
            return SDK_FAILURE;
    }

    status = clFinish(commandQueue);
    if(!sampleCommon->checkVal(
            status,
            CL_SUCCESS,
            "clFlush Failed."))
        return SDK_FAILURE;
    
        
    /*
     * This will combine the L & U matrices at the GPU side
     * so that they can be bought in CPU space as a single matrix
     */

    // 2nd kernel argument - L matrix 
    status = clSetKernelArg(
                    kernelCombine, 
                    0, 
                    sizeof(cl_mem), 
                    (void *)&inputBuffer2);
    if(!sampleCommon->checkVal(
            status,
            CL_SUCCESS,
            "clSetKernelArg failed. :kernelCombine(inputBuffer2)"))
        return SDK_FAILURE;

    // 2nd kernel argument - inplace matrix 
    status = clSetKernelArg(
                    kernelCombine,
                    1,
                    sizeof(cl_mem),
                    (void *)&inplaceBuffer);
    if(!sampleCommon->checkVal(
            status,
            CL_SUCCESS,
            "clSetKernelArg failed. :kernelCombine(inplaceBuffer)"))
        return SDK_FAILURE;
   
    globalThreads[0] = effectiveDimension;
    globalThreads[1] = effectiveDimension;

    status = clEnqueueNDRangeKernel(
                    commandQueue,
                    kernelCombine,
                    2,
                    NULL,
                    globalThreads,
                    NULL,
                    0,
                    NULL,
                    NULL);
    if(!sampleCommon->checkVal(
            status,
            CL_SUCCESS,
            "clEnqueueNDRangeKernel failed. "))
        return SDK_FAILURE;

    status = clFinish(commandQueue);
    if(!sampleCommon->checkVal(
            status,
            CL_SUCCESS,
            "clFinish() failed."))
        return SDK_FAILURE;

    // Enqueue readBuffer
    status = clEnqueueReadBuffer(
                    commandQueue,
                    inplaceBuffer,
                    CL_TRUE,
                    0,
                    SIZE,
                    matrixGPU,
                    0,
                    NULL,
                    &events[1]);
    if(!sampleCommon->checkVal(
            status,
            CL_SUCCESS,
            "clEnqueueReadBuffer failed."))
        return SDK_FAILURE;
    
    // Wait for the read buffer to finish execution 
    status = clWaitForEvents(1, &events[1]);
    if(!sampleCommon->checkVal(
            status,
            CL_SUCCESS,
            "clWaitForEvents failed1."))
        return SDK_FAILURE;
    
    status = clReleaseEvent(events[1]);
    if(!sampleCommon->checkVal(
            status,
            CL_SUCCESS,
            "clReleaseEvent() failed(events[1])"))
        return SDK_FAILURE;
    return SDK_SUCCESS;
}

void LUD::LUDCPUReference(double* matrixCPU, const cl_uint effectiveDimension)
{
    for(unsigned int d = 0 ; d < effectiveDimension-1 ; d++)
    {
        for(unsigned i = d + 1 ;i < effectiveDimension; i++)
        {
            double ratio = matrixCPU[i * effectiveDimension + d]
                            / matrixCPU[d * effectiveDimension + d];
            for(unsigned int j = d; j < effectiveDimension; j++)
            { 
                matrixCPU[ i * effectiveDimension + j] 
                    -= matrixCPU[d * effectiveDimension + j] * ratio;
                if(j == d)
                    matrixCPU[i * effectiveDimension + j] = ratio;
            }
        }
    }
}

int LUD::initialize()
{
    // Call base class Initialize to get default configuration 
    if(!this->SDKSample::initialize())
        return SDK_FAILURE;

    // add command line option for matrix actualDimension 
    streamsdk::Option* xParam = new streamsdk::Option;
    if(!xParam)
    {
        sampleCommon->error("Memory Allocation error.\n");
        return SDK_FAILURE;
    }

    xParam->_sVersion = "x";
    xParam->_lVersion = "dimension";
    xParam->_description = "actualDimension of input matrix";
    xParam->_type     = streamsdk::CA_ARG_INT;
    xParam->_value    = &actualDimension;

    sampleArgs->AddOption(xParam);
    delete xParam;

    streamsdk::Option* iter = new streamsdk::Option;
    if(!iter)
    {
        sampleCommon->error("Memory allocation for Option failed\n");
        return SDK_FAILURE;
    }

    iter->_sVersion = "i";
    iter->_lVersion = "iterations";
    iter->_description = "Number of iteration of kernel";
    iter->_type = streamsdk::CA_ARG_INT;
    iter->_value = &iterations;

    sampleArgs->AddOption(iter);
    delete iter;

    return SDK_SUCCESS;
}


int LUD::setup()
{  
    int timer = sampleCommon->createTimer();
    sampleCommon->resetTimer(timer);
    sampleCommon->startTimer(timer);

    cl_int status = setupCL();
    if(status != SDK_SUCCESS)
        return status;

    sampleCommon->stopTimer(timer);
    setupTime = (cl_double)sampleCommon->readTimer(timer);

    return SDK_SUCCESS;
}


int LUD::run()
{
    int timer = sampleCommon->createTimer();
    sampleCommon->resetTimer(timer);
    sampleCommon->startTimer(timer);   

    std::cout << "Executing kernel for " 
              << iterations << " iterations" << std::endl;
    std::cout << "-------------------------------------------"
              << std::endl;

    for(int i = 0; i < iterations; i++)
    {
        // Arguments are set and execution call is enqueued on command buffer 
        if(runCLKernels()!=SDK_SUCCESS)
            return SDK_FAILURE;
    }

    sampleCommon->stopTimer(timer);
    totalKernelTime = (double)(sampleCommon->readTimer(timer)) / iterations;

    if(!quiet)
        sampleCommon->printArray<double>(
                            "LU Matrix GPU implementation", 
                            matrixGPU, 
                            actualDimension, 
                            actualDimension);

    return SDK_SUCCESS;
}

int LUD::verifyResults()
{
    if(verify)
    {
        //reference implementation
        int refTimer = sampleCommon->createTimer();
        sampleCommon->resetTimer(refTimer);
        sampleCommon->startTimer(refTimer);
        LUDCPUReference(matrixCPU, effectiveDimension);
        sampleCommon->stopTimer(refTimer);
        referenceKernelTime = sampleCommon->readTimer(refTimer);

        if(!quiet)
            sampleCommon->printArray<double>(
                                "LU Matrix CPU Reference",
                                matrixCPU,
                                actualDimension,
                                actualDimension);

        // compare the results and see if they match 
        if(sampleCommon->compare(
                matrixGPU, 
                matrixCPU, 
                effectiveDimension * effectiveDimension))
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

void LUD::printStats()
{
    std::string strArray[4] = {"WxH" , "Time(sec)", "KernelTime(sec)"};
    std::string stats[3];

    totalTime = setupTime + totalKernelTime;

    stats[0]  = sampleCommon->toString(actualDimension, std::dec)
                + "x" + sampleCommon->toString(actualDimension, std::dec);
    stats[1]  = sampleCommon->toString(totalTime, std::dec);
    stats[2]  = sampleCommon->toString(totalKernelTime, std::dec);
    
    this->SDKSample::printStats(strArray, stats, 3);
}


int LUD::cleanup()
{
    // Releases OpenCL resources (Context, Memory etc.) 
    cl_int status;

    status = clReleaseKernel(kernelLUD);
    if(!sampleCommon->checkVal(
            status,
            CL_SUCCESS,
            "clReleaseKernel failed."))
        return SDK_FAILURE;

    status = clReleaseKernel(kernelCombine);
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

    status = clReleaseMemObject(inplaceBuffer);
    if(!sampleCommon->checkVal(
            status,
            CL_SUCCESS,
            "clReleaseMemObject failed."))
        return SDK_FAILURE;

    status = clReleaseMemObject(inputBuffer2);
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

    // release program resources (input memory etc.) 
#ifdef WIN32
    if(input) 
        _aligned_free(input);

    if(matrixGPU) 
        _aligned_free(matrixGPU);
#else
    if(input) 
        free(input);

    if(matrixGPU) 
        free(matrixGPU);
#endif

    if(matrixCPU)
        free(matrixCPU);

    if(devices)
        free(devices);

    return SDK_SUCCESS;
}


int main(int argc, char * argv[])
{
    LUD clLUDecompose("OpenCL LU Decomposition");
    clLUDecompose.initialize();
    if(!clLUDecompose.parseCommandLine(argc, argv))
        return SDK_FAILURE;

    if(clLUDecompose.isDumpBinaryEnabled())
    {
        return clLUDecompose.genBinaryImage();
    }
    else
    {        
        int status = clLUDecompose.setup();
        if(status != SDK_SUCCESS)
            return (status == SDK_FAILURE) ? SDK_FAILURE : SDK_SUCCESS;

        if(clLUDecompose.run()== SDK_FAILURE)
            return SDK_FAILURE;
        else
        {
            if(clLUDecompose.verifyResults()==SDK_FAILURE)
                return SDK_FAILURE;
        }
        if(clLUDecompose.cleanup()==SDK_FAILURE)
            return SDK_FAILURE;
        clLUDecompose.printStats();
    }

    return SDK_SUCCESS;
}

