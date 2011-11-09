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


#include "EigenValue.hpp"

int EigenValue::setupEigenValue()
{
    /* allocate and init memory used by host */
    /* allocate memory for diagonal elements of the matrix  of size lengthxlength*/

	if(length < 256)
	{
		length = 256;
	}

    cl_uint diagonalSizeBytes = length * sizeof(cl_float);
    diagonal = (cl_float *) malloc(diagonalSizeBytes);

    if(diagonal==NULL)    
    { 
        sampleCommon->error("Failed to allocate host memory. (diagonal)");
        return SDK_FAILURE;
    }

    /* allocate memory for offdiagonal elements of the matrix of length (length-1) */
    cl_uint offDiagonalSizeBytes = (length-1) * sizeof(cl_float);
    offDiagonal = (cl_float *) malloc(offDiagonalSizeBytes);

    if(offDiagonal == NULL)    
    { 
        sampleCommon->error("Failed to allocate host memory. (offDiagonal)");
        return SDK_FAILURE;
    }

    /* allocate memory to store the number of eigenvalues in an interval */
    numEigenIntervals = (cl_uint *) malloc(diagonalSizeBytes);
    
    if(numEigenIntervals == NULL)    
    { 
        sampleCommon->error("Failed to allocate host memory. (NumEigenIntervals)");
        return SDK_FAILURE;
    }

    /* 
     * allocate memory to store the eigenvalue intervals interleaved with upperbound followed
     * by the lower bound interleaved 
     * An array of two is used for using it for two different passes
     */
    cl_uint eigenIntervalsSizeBytes = (2*length) * sizeof(cl_float);
    for(int i=0 ; i < 2; ++i)
    {
        eigenIntervals[i] = (cl_float *) malloc(eigenIntervalsSizeBytes);

        if(eigenIntervals[i] == NULL)
        {
            sampleCommon->error("Failed to allocate host memory. (eigenIntervals)");
            return SDK_FAILURE;
        }
    }

    /* random initialisation of input using a seed*/
    sampleCommon->fillRandom<cl_float>(diagonal   , length  , 1, 0, 255, seed);
    sampleCommon->fillRandom<cl_float>(offDiagonal, length-1, 1, 0, 255, seed+10);

    /* calculate the upperbound and the lowerbound of the eigenvalues of the matrix */
    cl_float lowerLimit;
    cl_float upperLimit;
    computeGerschgorinInterval(&lowerLimit, &upperLimit, diagonal, offDiagonal, length);

    /* initialize the eigenvalue intervals */
    eigenIntervals[0][0]= lowerLimit;
    eigenIntervals[0][1]= upperLimit;

    /* the following intervals have no eigenvalues */
    for(cl_int i=2 ; i < 2*length ; i++)
    {
        eigenIntervals[0][i] = upperLimit;
    }

    epsilon   = 0.001f;
    tolerance = 0.001f;
    /* 
     * Unless quiet mode has been enabled, print the INPUT array.
     */
    if(!quiet) 
    {
        sampleCommon->printArray<cl_float>(
            "Diagonal", 
            diagonal, 
            length, 
            1);
        sampleCommon->printArray<cl_float>(
            "offDiagonal", 
            offDiagonal, 
            length-1, 
            1);
    }

    return SDK_SUCCESS;
}

int 
EigenValue::genBinaryImage()
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
    kernelPath.append("EigenValue_Kernels.cl");
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
EigenValue::setupCL(void)
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

    /* cl mem to store the diagonal elements of the matrix */
    diagonalBuffer = clCreateBuffer(
                      context, 
                      CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
                      sizeof(cl_float) * length,
                      diagonal, 
                      &status);

    if(!sampleCommon->checkVal(
            status,
            CL_SUCCESS,
            "clCreateBuffer failed. (diagonalBuffer)"))
        return SDK_FAILURE;

    /* cl mem to store the number of eigenvalues in each interval */
    numEigenValuesIntervalBuffer = clCreateBuffer(
                                    context, 
                                    CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR,
                                    sizeof(cl_uint) * length,
                                    numEigenIntervals, 
                                    &status);

    if(!sampleCommon->checkVal(
            status,
            CL_SUCCESS,
            "clCreateBuffer failed. (diagonalBuffer)"))
        return SDK_FAILURE;

    /* cl mem to store the offDiagonal elements of the matrix */
    offDiagonalBuffer = clCreateBuffer(
                         context, 
                         CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
                         sizeof(cl_float) * (length-1),
                         offDiagonal, 
                         &status);

    if(!sampleCommon->checkVal(
            status,
            CL_SUCCESS,
            "clCreateBuffer failed. (offDiagonalBuffer)"))
        return SDK_FAILURE;

    /* cl mem to store the eigenvalue intervals */
    for(int i = 0 ; i < 2 ; ++ i)
    {
        eigenIntervalBuffer[i] = clCreateBuffer(
                                 context, 
                                 CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR,
                                 sizeof(cl_uint) * length * 2,
                                 eigenIntervals[i], 
                                 &status);

        if(!sampleCommon->checkVal(
                status,
                CL_SUCCESS,
                "clCreateBuffer failed. (eigenIntervalBuffer)"))
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
	if (!strcmp(vName, "Intel(R) Corporation")) {
		vendorName = "intel";
	} else if (!strcmp(vName, "Advanced Micro Devices, Inc.")) {
		vendorName = "amd";
	} else if (platformIsPacketizedOpenCL) {
		vendorName = "pkt";
	} else {
		printf("ERROR: vendor not recognized: %s\n", vName);
	}

	kernelPath.append("EigenValue_Kernels.cl");
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

    /* get a kernel object handle for a kernel with the given name */
    kernel[0] = clCreateKernel(program, "calNumEigenValueInterval", &status);
    if(!sampleCommon->checkVal(
            status,
            CL_SUCCESS,
            "clCreateKernel failed."))
        return SDK_FAILURE;
    
    /* get a kernel object handle for a kernel with the given name */
    kernel[1] = clCreateKernel(program, "recalculateEigenIntervals", &status);
    if(!sampleCommon->checkVal(
            status,
            CL_SUCCESS,
            "clCreateKernel failed."))
        return SDK_FAILURE;

    return SDK_SUCCESS;
}

/* 
 * Checks if the difference between lowerlimit and upperlimit of all intervals is below
 * tolerance levels
 */
int
EigenValue::isComplete(cl_float * eigenIntervals)
{
    for(cl_int i=0; i< length; i++)
    {
        cl_uint lid = 2*i;
        cl_uint uid = lid + 1;
        if(eigenIntervals[uid] - eigenIntervals[lid] >= tolerance)
        {
            return 1;
        }
    }
    return 0;
}

int 
EigenValue::runCLKernels(void)
{
    cl_int   status;
    cl_event events[2];

    size_t globalThreads[1];
    size_t localThreads[1];

	globalThreads[0] = length;
    localThreads[0]  = 256;

    /* Check group size against kernelWorkGroupSize */
    status = clGetKernelWorkGroupInfo(kernel[0],
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
			std::cout<<"Out of Resources!" << std::endl;
			std::cout<<"Group Size specified : "<<localThreads[0]<<std::endl;
			std::cout<<"Max Group Size supported on the kernel : " 
				<<kernelWorkGroupSize<<std::endl;
			std::cout<<"Changing the group size to " << kernelWorkGroupSize 
                << std::endl;
		}

        localThreads[0] = kernelWorkGroupSize;
    }

    totalKernelTime = 0;

    /*
     * The number of eigenvalues in each interval are calculated using kernel[0] i.e. "calNumEigenValueInterval".
     * Once we have the number of eigenvalues in each interval, the intervals are recalculated such that
      * 1. Discards the intervals with no eigenvalues
      * 2. Checks if the number of eigenvalues in an interval is 1 and further splits that
      *    interval into two halfs and considers the interval in which the eigenvalue exists
      * 3. if the number of eigenvalues is more than n i.e. more than 1. The interval is split
      *    into n equal intervals.
      */

    /*** Set appropriate arguments to the kernel ***/
    /* first argument for kernel[0] - number of eigenvalues in each interval*/ 
    status = clSetKernelArg(
                    kernel[0], 
                    0, 
                    sizeof(cl_mem), 
                    (void *)&numEigenValuesIntervalBuffer);
    if(!sampleCommon->checkVal(
            status,
            CL_SUCCESS,
            "clSetKernelArg failed. (numEigenValuesIntervalBuffer)"))
        return SDK_FAILURE;


    /* third argument for kernel[0] - Diagonal elements of the matrix*/ 
    status = clSetKernelArg(
                    kernel[0], 
                    2,
                    sizeof(cl_mem), 
                    (void *)&diagonalBuffer);
    if(!sampleCommon->checkVal(
            status,
            CL_SUCCESS,
            "clSetKernelArg failed. (diagonalBuffer)"))
    return SDK_FAILURE;

    /* fourth argument for kernel[0] - offDiagonal elemnts of the matrix*/ 
    status = clSetKernelArg(
                    kernel[0], 
                    3, 
                    sizeof(cl_mem), 
                    (void *)&offDiagonalBuffer);
    if(!sampleCommon->checkVal(
            status,
            CL_SUCCESS,
            "clSetKernelArg failed. (offDiagonalBuffer)"))
    return SDK_FAILURE;

    /* length - i.e number of elements in the array */
    status = clSetKernelArg(
                    kernel[0], 
                    4, 
                    sizeof(cl_uint), 
                    (void *)&length);
    if(!sampleCommon->checkVal(
            status,
            CL_SUCCESS,
            "clSetKernelArg failed. (length)"))
        return SDK_FAILURE;

    /* third argument for kernel[1] - Number of eigenvalues in each interval*/ 
    status = clSetKernelArg(
                    kernel[1], 
                    2, 
                    sizeof(cl_mem), 
                    (void *)&numEigenValuesIntervalBuffer);
    if(!sampleCommon->checkVal(
            status,
            CL_SUCCESS,
            "clSetKernelArg failed. (numEigenValuesIntervalBuffer)"))
        return SDK_FAILURE;

    /* fourth argument for kernel[1]- Diagonal elements of the matrix*/ 
    status = clSetKernelArg(
                    kernel[1], 
                    3, 
                    sizeof(cl_mem), 
                    (void *)&diagonalBuffer);
    if(!sampleCommon->checkVal(
            status,
            CL_SUCCESS,
            "clSetKernelArg failed. (diagonalBuffer)"))
        return SDK_FAILURE;

    /* fifth argument for kernel[1] - offDiagonal elements of the matrix*/ 
    status = clSetKernelArg(
                    kernel[1], 
                    4, 
                    sizeof(cl_mem), 
                    (void *)&offDiagonalBuffer);
    if(!sampleCommon->checkVal(
            status,
            CL_SUCCESS,
            "clSetKernelArg failed. (offDiagonalBuffer)"))
        return SDK_FAILURE;

    /* sixth argument for kernel[1] - length*/ 
    status = clSetKernelArg(
                    kernel[1], 
                    5, 
                    sizeof(cl_uint), 
                    (void *)&length);
    if(!sampleCommon->checkVal(
            status,
            CL_SUCCESS,
            "clSetKernelArg failed. (length)"))
        return SDK_FAILURE;

    /* sixth argument for kernel[1] - tolerance*/ 
    status = clSetKernelArg(
                    kernel[1], 
                    6, 
                    sizeof(cl_float), 
                    (void *)&tolerance);
    if(!sampleCommon->checkVal(
            status,
            CL_SUCCESS,
            "clSetKernelArg failed. (tolerance)"))
        return SDK_FAILURE;

    in = 0;
    while(isComplete(eigenIntervals[in]))
    {
        /* second argument for kernel[0] - input eigenvalue intervals */
        status = clSetKernelArg(
                        kernel[0], 
                        1, 
                        sizeof(cl_mem), 
                        (void *)&eigenIntervalBuffer[in]);
        if(!sampleCommon->checkVal(
                status,
                CL_SUCCESS,
                "clSetKernelArg failed. (inputBuffer)"))
            return SDK_FAILURE;
        /* first argument for kernel[0] - recalculated eigenvalue intervals */
        status = clSetKernelArg(
                        kernel[1], 
                        0, 
                        sizeof(cl_mem), 
                        (void *)&eigenIntervalBuffer[1 - in]);
        if(!sampleCommon->checkVal(
                status,
                CL_SUCCESS,
                "clSetKernelArg failed. (inputBuffer)"))
            return SDK_FAILURE;

        /* second argument for kernel[0] - original eigenvalue intervals */
        status = clSetKernelArg(
                        kernel[1], 
                        1, 
                        sizeof(cl_mem), 
                        (void *)&eigenIntervalBuffer[in]);
        if(!sampleCommon->checkVal(
                status,
                CL_SUCCESS,
                "clSetKernelArg failed. (inputBuffer)"))
            return SDK_FAILURE;

        status = clEnqueueNDRangeKernel(
                     commandQueue,
                     kernel[0],
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


        status = clReleaseEvent(events[0]);
        if(!sampleCommon->checkVal(
                status,
                CL_SUCCESS,
                "clReleaseEvent failed.(event[0]"))
            return SDK_FAILURE;

        status = clEnqueueNDRangeKernel(
                 commandQueue,
                 kernel[1],
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

        in = 1 - in;

        /* Enqueue readBuffer*/
        status = clEnqueueReadBuffer(
                    commandQueue,
                    eigenIntervalBuffer[in],
                    CL_TRUE,
                    0,
                    length * 2 * sizeof(cl_uint),
                    eigenIntervals[in],
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
    

    /* Enqueue readBuffer*/
    status = clEnqueueReadBuffer(
                commandQueue,
                eigenIntervalBuffer[in],
                CL_TRUE,
                0,
                length * 2 * sizeof(cl_uint),
                eigenIntervals[in],
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

    return SDK_SUCCESS;
}

/*
 * function to calculate the gerschgorin interval(lowerbound and upperbound of the eigenvalues) 
 *                                              of a tridiagonal symmetric matrix
 */
void 
EigenValue::computeGerschgorinInterval(cl_float * lLimit,
                                    cl_float * uLimit,
                                    const cl_float * diagonal,
                                    const cl_float * offDiagonal,
                                    const cl_uint  length)
{
    
    cl_float lowerLimit = diagonal[0] - fabs(offDiagonal[0]);
    cl_float upperLimit = diagonal[0] + fabs(offDiagonal[0]);

    for(cl_uint i = 1; i < length-1; ++i)
    {
        float r =  fabs(offDiagonal[i-1]) + fabs(offDiagonal[i]);
        lowerLimit = (lowerLimit > (diagonal[i] - r))? (diagonal[i] - r): lowerLimit;
        upperLimit = (upperLimit < (diagonal[i] + r))? (diagonal[i] + r): upperLimit;
    }

    lowerLimit = (lowerLimit > (diagonal[length-1] - fabs(offDiagonal[length-2])))? 
                    (diagonal[length-1] - fabs(offDiagonal[length-2])): lowerLimit;
    upperLimit = (upperLimit < (diagonal[length-1] + fabs(offDiagonal[length-2])))?
                    (diagonal[length-1] + fabs(offDiagonal[length-2])): upperLimit;

    *lLimit = lowerLimit;
    *uLimit = upperLimit;

}


/*
 * function to calculate the number of eigenvalues less than (x) for a tridiagonal symmetric matrix
 */

cl_uint  
EigenValue::calNumEigenValuesLessThan(const cl_float *diagonal, 
                                 const cl_float *offDiagonal, 
                                 const cl_uint  length, 
                                 const cl_float x)
{
    cl_uint count = 0;

    float prev_diff = (diagonal[0] - x);
    count += (prev_diff < 0)? 1 : 0;
    for(cl_uint i = 1; i < length; i += 1)
    {
        float diff = (diagonal[i] - x) - ((offDiagonal[i-1] * offDiagonal[i-1])/prev_diff);

        count += (diff < 0)? 1 : 0;
        prev_diff = diff;
    }
    return count;
}

/*
 * Calculates the eigenvalues of a tridiagonal symmetrix matrix
 */
cl_uint
EigenValue::eigenValueCPUReference(cl_float * diagonal,
                                   cl_float * offDiagonal,
                                   cl_uint    length,
                                   cl_float * eigenIntervals,
                                   cl_float * newEigenIntervals)
{
    cl_uint offset = 0;
    for(cl_uint i =0; i < length; ++i)
    {
        cl_uint lid = 2*i;
        cl_uint uid = lid + 1;

        cl_uint eigenValuesLessLowerBound = calNumEigenValuesLessThan(diagonal, offDiagonal, length, eigenIntervals[lid]);
        cl_uint eigenValuesLessUpperBound = calNumEigenValuesLessThan(diagonal, offDiagonal, length, eigenIntervals[uid]);

        cl_uint numSubIntervals = eigenValuesLessUpperBound - eigenValuesLessLowerBound;

        if(numSubIntervals > 1)
        {
            cl_float avgSubIntervalWidth = (eigenIntervals[uid] - eigenIntervals[lid])/numSubIntervals;

            for(cl_uint j=0; j < numSubIntervals; ++j)
            {
                cl_uint newLid = 2* (offset+j);
                cl_uint newUid = newLid + 1;

                newEigenIntervals[newLid] = eigenIntervals[lid]       + j * avgSubIntervalWidth;
                newEigenIntervals[newUid] = newEigenIntervals[newLid] +     avgSubIntervalWidth;
            }
        }
        else if(numSubIntervals == 1)
        {
            cl_float lowerBound = eigenIntervals[lid];
            cl_float upperBound = eigenIntervals[uid];

            cl_float mid        = (lowerBound + upperBound)/2;
            
            cl_uint newLid = 2* offset;
            cl_uint newUid = newLid + 1;

            if(upperBound - lowerBound < tolerance)
            {
                newEigenIntervals[newLid] = lowerBound;
                newEigenIntervals[newUid] = upperBound;
            }
            else if(calNumEigenValuesLessThan(diagonal,offDiagonal, length, mid) == eigenValuesLessUpperBound)
            {
                newEigenIntervals[newLid] = lowerBound;
                newEigenIntervals[newUid] = mid;
            }
            else
            {
                newEigenIntervals[newLid] = mid;
                newEigenIntervals[newUid] = upperBound;
            }
        }
        offset += numSubIntervals;
    }
    return offset;
}

int 
EigenValue::initialize()
{
    // Call base class Initialize to get default configuration
    if(!this->SDKSample::initialize())
        return SDK_FAILURE;

    // Now add customized options
    streamsdk::Option* diagonal_length = new streamsdk::Option;
    if(!diagonal_length)
    {
        sampleCommon->error("Memory allocation error.\n");
        return SDK_FAILURE;
    }
    
    diagonal_length->_sVersion = "x";
    diagonal_length->_lVersion = "length";
    diagonal_length->_description = "Length of diagonal of the square matrix";
    diagonal_length->_type = streamsdk::CA_ARG_INT;
    diagonal_length->_value = &length;
    sampleArgs->AddOption(diagonal_length);
    delete diagonal_length;

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
EigenValue::setup()
{
    if(!sampleCommon->isPowerOf2(length))
        length = sampleCommon->roundToPowerOf2(length);

    if(setupEigenValue()!=SDK_SUCCESS)
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


int 
EigenValue::run()
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
        sampleCommon->printArray<cl_float>("Output", eigenIntervals[1], 2*length, 1);
    }

    return SDK_SUCCESS;
}

int 
EigenValue::verifyResults()
{
    cl_uint offset = 0;
    if(verify)
    {
        cl_uint eigenIntervalsSizeBytes = (2*length) * sizeof(cl_float);
        for(int i=0 ; i < 2; ++i)
        {
            verificationEigenIntervals[i] = (cl_float *) malloc(eigenIntervalsSizeBytes);

            if(verificationEigenIntervals[i] == NULL)
            {
                sampleCommon->error("Failed to allocate host memory. (verificationEigenIntervals)");
                return SDK_FAILURE;
            }
        }

        cl_float lowerLimit;
        cl_float upperLimit;
        computeGerschgorinInterval(&lowerLimit, &upperLimit, diagonal, offDiagonal, length);

        verificationIn = 0;
        verificationEigenIntervals[verificationIn][0]= lowerLimit;
        verificationEigenIntervals[verificationIn][1]= upperLimit;

        for(cl_int i=2 ; i < 2*length ; i++)
        {
            verificationEigenIntervals[verificationIn][i] = upperLimit;
        }


        int refTimer = sampleCommon->createTimer();
        sampleCommon->resetTimer(refTimer);
        sampleCommon->startTimer(refTimer);

        while(isComplete(verificationEigenIntervals[verificationIn]))
        {
            offset = eigenValueCPUReference(diagonal,offDiagonal, length, verificationEigenIntervals[verificationIn],
                                            verificationEigenIntervals[1-verificationIn]);
            verificationIn = 1 - verificationIn;
        }

        sampleCommon->stopTimer(refTimer);
        referenceKernelTime = sampleCommon->readTimer(refTimer);
        
        if(sampleCommon->compare(eigenIntervals[in], verificationEigenIntervals[verificationIn], 2*length))
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
EigenValue::printStats()
{
    std::string strArray[3] = {"DiagonalLength", "Time(sec)", "kernelTime(sec)"};
    std::string stats[3];

    totalTime = setupTime + totalKernelTime;

    stats[0] = sampleCommon->toString(length, std::dec);
    stats[1] = sampleCommon->toString(totalTime, std::dec);
	stats[2] = sampleCommon->toString(totalKernelTime, std::dec);

    this->SDKSample::printStats(strArray, stats, 3);
    this->SDKSample::logStats(totalKernelTime, "EigenValue", vendorName);
}

int 
EigenValue::cleanup()
{
    /* Releases OpenCL resources (Context, Memory etc.) */
    cl_int status;

    for(cl_uint i=0; i < 2; ++i)
    {
        status = clReleaseKernel(kernel[i]);
        if(!sampleCommon->checkVal(
            status,
            CL_SUCCESS,
            "clReleaseKernel failed."))
            return SDK_FAILURE;
    }

    status = clReleaseProgram(program);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clReleaseProgram failed."))
        return SDK_FAILURE;

    status = clReleaseMemObject(diagonalBuffer);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clReleaseMemObject failed."))
        return SDK_FAILURE;

    status = clReleaseMemObject(offDiagonalBuffer);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clReleaseMemObject failed."))
        return SDK_FAILURE;
   
    status = clReleaseMemObject(eigenIntervalBuffer[0]);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clReleaseMemObject failed."))
        return SDK_FAILURE;

    status = clReleaseMemObject(eigenIntervalBuffer[1]);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clReleaseMemObject failed."))
        return SDK_FAILURE;

    status = clReleaseMemObject(numEigenValuesIntervalBuffer);
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
    if(diagonal) 
        free(diagonal);

    if(offDiagonal)
        free(offDiagonal);

    if(eigenIntervals[0])
        free(eigenIntervals[0]);

    if(eigenIntervals[1])
        free(eigenIntervals[1]);

    if(numEigenIntervals)
        free(numEigenIntervals);

    if(verificationEigenIntervals[0]) 
        free(verificationEigenIntervals[0]);

    if(verificationEigenIntervals[1]) 
        free(verificationEigenIntervals[1]);

    if(devices)
        free(devices);

    return SDK_SUCCESS;
}

int 
main(int argc, char * argv[])
{
    EigenValue clEigenValue("OpenCL Eigen Value");

    clEigenValue.initialize();
    if(!clEigenValue.parseCommandLine(argc, argv))
        return SDK_FAILURE;

    if(clEigenValue.isDumpBinaryEnabled())
    {
        return clEigenValue.genBinaryImage();
    }
    else
    {
        if(clEigenValue.setup()!=SDK_SUCCESS)
		    return SDK_FAILURE;
        if(clEigenValue.run()!=SDK_SUCCESS)
		    return SDK_FAILURE;
        if(clEigenValue.verifyResults()!=SDK_SUCCESS)
		    return SDK_FAILURE;
        if(clEigenValue.cleanup()!=SDK_SUCCESS)
		    return SDK_FAILURE;
        clEigenValue.printStats();
    }

    return SDK_SUCCESS;
}
