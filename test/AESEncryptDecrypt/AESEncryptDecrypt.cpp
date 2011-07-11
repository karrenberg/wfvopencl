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


#include "AESEncryptDecrypt.hpp"

using namespace AES;

int AESEncryptDecrypt::setupAESEncryptDecrypt()
{
    cl_uint sizeBytes = width*height*sizeof(cl_uchar);

    input = (cl_uchar*)malloc(sizeBytes); 
    if(input == NULL)
    {
        sampleCommon->error("Failed to allocate host memory. (input)");
        return SDK_FAILURE;
    }

    /* initialize the input array, do NOTHING but assignment when decrypt*/
    if(!decrypt)
       convertColorToGray(pixels, input);
    else
       convertGrayToGray(pixels, input);

    /* 1 Byte = 8 bits */
    keySize = keySizeBits/8;
  
    /* due to unknown represenation of cl_uchar */ 
    keySizeBits = keySize*sizeof(cl_uchar); 

    key = (cl_uchar*)malloc(keySizeBits);

    /* random initialization of key */
    sampleCommon->fillRandom<cl_uchar>(key, keySize, 1, 0, 255, seed); 

    /* expand the key */
    explandedKeySize = (rounds+1)*keySize;  
    expandedKey = (cl_uchar*)malloc(explandedKeySize*sizeof(cl_uchar));
    roundKey    = (cl_uchar*)malloc(explandedKeySize*sizeof(cl_uchar));

    keyExpansion(key, expandedKey, keySize, explandedKeySize);
    for(cl_uint i = 0; i< rounds+1; ++i)
    {
        createRoundKey(expandedKey + keySize*i, roundKey + keySize*i);
    }

    output = (cl_uchar*)malloc(sizeBytes);
    if(output == NULL)
    {
        sampleCommon->error("Failed to allocate host memory. (output)");
        return SDK_FAILURE;
    } 

    if(!quiet) 
    {
        if(decrypt)
        {
            std::cout << "Decrypting Image ...." << std::endl;
        }
        else
        {
            std::cout << "Encrypting Image ...." << std::endl;
        }

        std::cout << "Input Image : " << inFilename << std::endl;
        std::cout << "Key : ";
        for(cl_uint i=0; i < keySize; ++i)
        {
            std::cout << (cl_uint)key[i] << " ";
        }
        std::cout << std::endl;
    }

    return SDK_SUCCESS;
}

void
AESEncryptDecrypt::convertColorToGray(const uchar4 *pixels, cl_uchar *gray)
{
    for(cl_int i=0; i< height; ++i)
        for(cl_int j=0; j<width; ++j)
        {
            cl_uint index = i*width + j;
            // gray = (0.3*R + 0.59*G + 0.11*B)
            gray[index] = cl_uchar (pixels[index].x * 0.3  + 
                                    pixels[index].y * 0.59 + 
                                    pixels[index].z * 0.11 );
        }
}

void
AESEncryptDecrypt::convertGrayToGray(const uchar4 *pixels, cl_uchar *gray)
{
    for(cl_int i=0; i< height; ++i)
        for(cl_int j=0; j<width; ++j)
        {
            cl_uint index = i*width + j;
            gray[index] = pixels[index].x;
        }
}

void
AESEncryptDecrypt::convertGrayToPixels(const cl_uchar *gray, uchar4 *pixels)
{
    for(cl_int i=0; i< height; ++i)
        for(cl_int j=0; j<width; ++j)
        {
            cl_uint index = i*width + j;
            pixels[index].x = gray[index];
            pixels[index].y = gray[index];
            pixels[index].z = gray[index];
        }
}

int 
AESEncryptDecrypt::genBinaryImage()
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
    kernelPath.append("AESEncryptDecrypt_Kernels.cl");
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
AESEncryptDecrypt::setupCL(void)
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
                    sizeof(cl_uchar ) * width * height,
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
                    sizeof(cl_uchar ) * width * height,
                    output, 
                    &status);

    if(!sampleCommon->checkVal(
                status,
                CL_SUCCESS,
                "clCreateBuffer failed. (outputBuffer)"))
        return SDK_FAILURE;

    rKeyBuffer = clCreateBuffer(
                    context, 
                    CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
                    sizeof(cl_uchar ) * explandedKeySize,
                    roundKey,
                    &status);

    if(!sampleCommon->checkVal(
                        status,
                        CL_SUCCESS,
                        "clCreateBuffer failed. (rKeyBuffer)"))
        return SDK_FAILURE;

    cl_uchar * sBox;
    sBox = (cl_uchar *)sbox;
    sBoxBuffer = clCreateBuffer(
                    context, 
                    CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
                    sizeof(cl_uchar ) * 256,
                    sBox,
                    &status);

    cl_uchar * rsBox;
    rsBox = (cl_uchar *)rsbox;
    rsBoxBuffer = clCreateBuffer(
                    context, 
                    CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
                    sizeof(cl_uchar ) * 256,
                    rsBox,
                    &status);

    if(!sampleCommon->checkVal(
                        status,
                        CL_SUCCESS,
                        "clCreateBuffer failed. (sBoxBuffer)"))
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
        kernelPath.append("AESEncryptDecrypt_Kernels.cl");
        if(!kernelFile.open(kernelPath.c_str()))
        {
            std::cout << "Failed to load kernel file: " << kernelPath << std::endl;
            return SDK_FAILURE;
        }
        const char * source = kernelFile.source().c_str();
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
    if(decrypt)
    {
        kernel = clCreateKernel(program, "AESDecrypt", &status);
    }
    else
    {
        kernel = clCreateKernel(program, "AESEncrypt", &status);
    }
        
    if(!sampleCommon->checkVal(
                        status,
                        CL_SUCCESS,
                        "clCreateKernel failed."))
        return SDK_FAILURE;

    return SDK_SUCCESS;
}


int 
AESEncryptDecrypt::runCLKernels(void)
{
    cl_int   status;
    cl_event events[2];
    
    size_t globalThreads[2]= {width/4, height};
    size_t localThreads[2] = {64, 4};

    if (localThreads[1] != 4)
    {
        sampleCommon->error("localThreads[1] value should be 4 \n");
        return SDK_FAILURE;
    }

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

    neededLocalMemory  = 2 * localThreads[0] * localThreads[1] * 4;

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

    if((cl_uint)(localThreads[0] * localThreads[1]) > kernelWorkGroupSize )
    {
        localThreads[0] = kernelWorkGroupSize / 4;
    }

    
    if(localThreads[0] > maxWorkItemSizes[0] ||
       localThreads[1] > maxWorkItemSizes[1] ||
       localThreads[0]*localThreads[1] > maxWorkGroupSize)
    {
        std::cout << "Unsupported: Device does not support requested number of work items."<<std::endl;
        return SDK_SUCCESS;
    }
    

    /*** Set appropriate arguments to the kernel ***/
    status = clSetKernelArg(
                    kernel, 
                    0, 
                    sizeof(cl_mem), 
                    (void *)&outputBuffer);
    if(!sampleCommon->checkVal(
                        status,
                        CL_SUCCESS,
                        "clSetKernelArg failed. (outputBuffer)"))
        return SDK_FAILURE;

    status = clSetKernelArg(
                    kernel, 
                    1, 
                    sizeof(cl_mem), 
                    (void *)&inputBuffer);
    if(!sampleCommon->checkVal(
                status,
                CL_SUCCESS,
                "clSetKernelArg failed. (inputBuffer)"))
        return SDK_FAILURE;

    status = clSetKernelArg(
                    kernel, 
                    2, 
                    sizeof(cl_mem), 
                    (void *)&rKeyBuffer);
    if(!sampleCommon->checkVal(
                status,
                CL_SUCCESS,
                "clSetKernelArg failed. (rKeyBuffer)"))
        return SDK_FAILURE;

    if(decrypt)
    {
        status = clSetKernelArg(
                    kernel, 
                    3, 
                    sizeof(cl_mem), 
                    (void *)&rsBoxBuffer);
    }
    else
    {
        status = clSetKernelArg(
                    kernel, 
                    3, 
                    sizeof(cl_mem), 
                    (void *)&sBoxBuffer);
    }
    if(!sampleCommon->checkVal(
                status,
                CL_SUCCESS,
                "clSetKernelArg failed. (SBoxBuffer)"))
        return SDK_FAILURE;

    status = clSetKernelArg(
                    kernel, 
                    4, 
                    localThreads[0] * localThreads[1] * 4 * sizeof (cl_uchar), 
                    NULL);
    if(!sampleCommon->checkVal(
                status,
                CL_SUCCESS,
                "clSetKernelArg failed. (block0)"))
        return SDK_FAILURE;

    status = clSetKernelArg(
                    kernel, 
                    5, 
                    localThreads[0] * localThreads[1] * 4 * sizeof(cl_uchar), 
                    NULL);
    if(!sampleCommon->checkVal(
                status,
                CL_SUCCESS,
                "clSetKernelArg failed. (block1)"))
        return SDK_FAILURE;

    status = clSetKernelArg(
                    kernel, 
                    6, 
                    sizeof(cl_uint), 
                    (void *)&width);
    if(!sampleCommon->checkVal(
                status,
                CL_SUCCESS,
                "clSetKernelArg failed. (width)"))
        return SDK_FAILURE;

    status = clSetKernelArg(
                    kernel, 
                    7, 
                    sizeof(cl_uint), 
                    (void *)&rounds);
    if(!sampleCommon->checkVal(
                status,
                CL_SUCCESS,
                "clSetKernelArg failed. (rounds)"))
        return SDK_FAILURE;


    /* 
     * Enqueue a kernel run call.
     */
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

    /* Enqueue the results to application pointer*/
    status = clEnqueueReadBuffer(
                commandQueue,
                outputBuffer,
                CL_TRUE,
                0,
                width * height * sizeof(cl_uchar),
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

    clReleaseEvent(events[0]);
    clReleaseEvent(events[1]);

    return SDK_SUCCESS;
}

cl_uchar 
AESEncryptDecrypt::getRconValue(cl_uint num)
{
    return Rcon[num];
}

void
AESEncryptDecrypt::rotate(cl_uchar * word)
{
    cl_uchar c = word[0];
    for(cl_uint i=0; i<3; ++i)
    {
        word[i] = word[i+1];
    }
    word[3] = c;
}

void
AESEncryptDecrypt::core(cl_uchar * word, cl_uint iter)
{
    rotate(word);
    
    for(cl_uint i=0; i < 4; ++i)
    {
        word[i] = getSBoxValue(word[i]);
    }    
    
    word[0] = word[0]^getRconValue(iter);
}

void
AESEncryptDecrypt::keyExpansion(cl_uchar * key, cl_uchar * expandedKey,
                                cl_uint keySize, cl_uint explandedKeySize)
{
    cl_uint currentSize    = 0;
    cl_uint rConIteration = 1;
    cl_uchar temp[4]      = {0};
    
    for(cl_uint i=0; i < keySize; ++i)
    {
        expandedKey[i] = key[i];
    }
    
    currentSize += keySize;

    while(currentSize < explandedKeySize)
    {
        for(cl_uint i=0; i < 4; ++i)
        {
            temp[i] = expandedKey[(currentSize - 4) + i];
        }

        if(currentSize%keySize == 0)
        {
            core(temp, rConIteration++);
        }
        
        //XXX: add extra SBOX here if the keySize is 32 Bytes
        
        for(cl_uint i=0; i < 4; ++i)
        {
            expandedKey[currentSize] = expandedKey[currentSize - keySize]^temp[i];
            currentSize++;
        }
    }
}

cl_uchar 
AESEncryptDecrypt::getSBoxValue(cl_uint num)
{
    return sbox[num];
}

cl_uchar 
AESEncryptDecrypt::getSBoxInvert(cl_uint num)
{
    return rsbox[num];
}

cl_uchar
AESEncryptDecrypt::galoisMultiplication(cl_uchar a, cl_uchar b)
{
    cl_uchar p = 0; 
    for(cl_uint i=0; i < 8; ++i)
    {
        if((b&1) == 1)
        {
            p^=a;
        }
        cl_uchar hiBitSet = (a & 0x80);
        a <<= 1;
        if(hiBitSet == 0x80)
        {
            a ^= 0x1b;
        }
        b >>= 1;
    }
    return p;
}

void
AESEncryptDecrypt::mixColumn(cl_uchar *column)
{
    cl_uchar cpy[4];
    for(cl_uint i=0; i < 4; ++i)
    {
        cpy[i] = column[i];
    }
    column[0] = galoisMultiplication(cpy[0], 2)^
                galoisMultiplication(cpy[3], 1)^
                galoisMultiplication(cpy[2], 1)^
                galoisMultiplication(cpy[1], 3);
    
    column[1] = galoisMultiplication(cpy[1], 2)^
                galoisMultiplication(cpy[0], 1)^
                galoisMultiplication(cpy[3], 1)^
                galoisMultiplication(cpy[2], 3);
    
    column[2] = galoisMultiplication(cpy[2], 2)^
                galoisMultiplication(cpy[1], 1)^
                galoisMultiplication(cpy[0], 1)^
                galoisMultiplication(cpy[3], 3);
    
    column[3] = galoisMultiplication(cpy[3], 2)^
                galoisMultiplication(cpy[2], 1)^
                galoisMultiplication(cpy[1], 1)^
                galoisMultiplication(cpy[0], 3);
}

void
AESEncryptDecrypt::mixColumnInv(cl_uchar *column)
{
    cl_uchar cpy[4];
    for(cl_uint i=0; i < 4; ++i)
    {
        cpy[i] = column[i];
    }
    column[0] = galoisMultiplication(cpy[0], 14 )^
                galoisMultiplication(cpy[3], 9 )^
                galoisMultiplication(cpy[2], 13)^
                galoisMultiplication(cpy[1], 11);
    
    column[1] = galoisMultiplication(cpy[1], 14 )^
                galoisMultiplication(cpy[0], 9 )^
                galoisMultiplication(cpy[3], 13)^
                galoisMultiplication(cpy[2], 11);
    
    column[2] = galoisMultiplication(cpy[2], 14 )^
                galoisMultiplication(cpy[1], 9 )^
                galoisMultiplication(cpy[0], 13)^
                galoisMultiplication(cpy[3], 11);
    
    column[3] = galoisMultiplication(cpy[3], 14 )^
                galoisMultiplication(cpy[2], 9 )^
                galoisMultiplication(cpy[1], 13)^
                galoisMultiplication(cpy[0], 11);
}

void
AESEncryptDecrypt::mixColumns(cl_uchar * state, cl_bool inverse)
{
    cl_uchar column[4];
    for(cl_uint i=0; i < 4; ++i)
    {
        for(cl_uint j=0; j < 4; ++j)
        {
            column[j] = state[j*4 + i];
        }
        
        if(inverse)
        {
            mixColumnInv(column);
        }
        else
        {
            mixColumn(column);
        }
       
         for(cl_uint j=0; j < 4; ++j)
        {
            state[j*4 + i] = column[j];
        }
    }
}

void
AESEncryptDecrypt::subBytes(cl_uchar * state, cl_bool inverse)
{
    for(cl_uint i=0; i < keySize; ++i)
    {
        state[i] = inverse ? getSBoxInvert(state[i]): getSBoxValue(state[i]);
    }
}

void
AESEncryptDecrypt::shiftRow(cl_uchar *state, cl_uchar nbr)
{
    for(cl_uint i=0; i < nbr; ++i)
    {
        cl_uchar tmp = state[0];
        for(cl_uint j = 0; j < 3; ++j)
        {
            state[j] = state[j+1];
        }
        state[3] = tmp;
    }
}

void
AESEncryptDecrypt::shiftRowInv(cl_uchar *state, cl_uchar nbr)
{
    for(cl_uint i=0; i < nbr; ++i)
    {
        cl_uchar tmp = state[3];
        for(cl_uint j = 3; j > 0; --j)
        {
            state[j] = state[j-1];
        }
        state[0] = tmp;
    }
}

void
AESEncryptDecrypt::shiftRows(cl_uchar * state, cl_bool inverse)
{
    for(cl_uint i=0; i < 4; ++i)
    {
        if(inverse)
            shiftRowInv(state + i*4, i);
        else
            shiftRow(state + i*4, i);
    }
}

void
AESEncryptDecrypt::addRoundKey(cl_uchar * state, cl_uchar * rKey)
{	
    for(cl_uint i=0; i < keySize; ++i)
    {	
        state[i] = state[i] ^ rKey[i];
    }

}

void
AESEncryptDecrypt::createRoundKey(cl_uchar * eKey, cl_uchar * rKey)
{
    for(cl_uint i=0; i < 4; ++i)
        for(cl_uint j=0; j < 4; ++j)
        {
            rKey[i+ j*4] = eKey[i*4 + j];
        }
}

void
AESEncryptDecrypt::aesRound(cl_uchar * state, cl_uchar * rKey)
{
    subBytes(state, decrypt);
    shiftRows(state, decrypt);
    mixColumns(state, decrypt);
    addRoundKey(state, rKey);
}

void
AESEncryptDecrypt::aesMain(cl_uchar * state, cl_uchar * rKey, cl_uint rounds)
{
    addRoundKey(state, rKey);

    for(cl_uint i=1; i < rounds; ++i)
    {
        aesRound(state, rKey + keySize*i);
    }

    subBytes(state, decrypt);
    shiftRows(state, decrypt);
    addRoundKey(state, rKey + keySize*rounds);
}

void
AESEncryptDecrypt::aesRoundInv(cl_uchar * state, cl_uchar * rKey)
{
    shiftRows(state, decrypt);
    subBytes(state, decrypt);
    addRoundKey(state, rKey);
    mixColumns(state, decrypt);
}

void
AESEncryptDecrypt::aesMainInv(cl_uchar * state, cl_uchar * rKey, cl_uint rounds)
{
    addRoundKey(state, rKey + keySize * rounds);
    for(cl_uint i=rounds-1; i > 0; --i)
    {
        aesRoundInv(state, rKey + keySize*i);
    } 
    shiftRows(state, decrypt);
    subBytes(state, decrypt);
    addRoundKey(state, rKey);
}

/**
 *
 *
 */
void 
AESEncryptDecrypt::AESEncryptDecryptCPUReference(cl_uchar * output       ,
                                                 cl_uchar * input        ,
                                                 cl_uchar * rKey         ,
                                                 cl_uint explandedKeySize,
                                                 cl_uint width           ,
                                                 cl_uint height          ,
                                                 cl_bool inverse         )
{
    cl_uchar block[16];
   
    for(cl_uint blocky = 0; blocky < height/4; ++blocky)
        for(cl_uint blockx= 0; blockx < width/4; ++blockx)
        { 
            for(cl_uint i=0; i < 4; ++i)
            {
                for(cl_uint j=0; j < 4; ++j)
                {
                    cl_uint x = blockx * 4 + j;
                    cl_uint y = blocky * 4 + i;
                    cl_uint index = y * width + x;
                    block[i * 4 + j] = input[index];
                }
            }

            if(inverse)
                aesMainInv(block, rKey, rounds);
            else
                aesMain(block, rKey, rounds);
            
            for(cl_uint i=0; i <4 ; ++i)
            {
                for(cl_uint j=0; j <4; ++j)
                {
                    cl_uint x = blockx * 4 + j;
                    cl_uint y = blocky * 4 + i;
                    cl_uint index = y * width + x;
                    output[index] =  block[i * 4 + j];
                } 
            }
        }
}


int 
AESEncryptDecrypt::initialize()
{
   // Call base class Initialize to get default configuration
   if(!this->SDKSample::initialize())
      return SDK_FAILURE;

   streamsdk::Option* ifilename_opt = new streamsdk::Option;
   if(!ifilename_opt)
   {
      sampleCommon->error("Memory allocation error.\n");
      return SDK_FAILURE;
   }
   ifilename_opt->_sVersion = "x";
   ifilename_opt->_lVersion = "input";
   ifilename_opt->_description = "Image as Input";
   ifilename_opt->_type = streamsdk::CA_ARG_STRING;
   ifilename_opt->_value = &inFilename;
   sampleArgs->AddOption(ifilename_opt);

   delete ifilename_opt;

   ////////////////
   streamsdk::Option* ofilename_opt = new streamsdk::Option;
   if(!ofilename_opt)
   {
      sampleCommon->error("Memory allocation error.\n");
      return SDK_FAILURE;
   }
   ofilename_opt->_sVersion = "y";
   ofilename_opt->_lVersion = "output";
   ofilename_opt->_description = "Image as Ouput";
   ofilename_opt->_type = streamsdk::CA_ARG_STRING;
   ofilename_opt->_value = &outFilename;
   sampleArgs->AddOption(ofilename_opt);

   delete ofilename_opt;

    ////////////////
    streamsdk::Option* decrypt_opt = new streamsdk::Option;
    if(!decrypt_opt)
    {
        sampleCommon->error("Memory allocation error.\n");
        return SDK_FAILURE;
    }
    decrypt_opt->_sVersion = "z";
    decrypt_opt->_lVersion = "decrypt";
    decrypt_opt->_description = "Decrypt the Input Image"; 
    decrypt_opt->_type     = streamsdk::CA_NO_ARGUMENT;
    decrypt_opt->_value    = &decrypt;
    sampleArgs->AddOption(decrypt_opt);

    delete decrypt_opt;

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
AESEncryptDecrypt::setup()
{
    std::string filePath = sampleCommon->getPath() + inFilename;
    image.load(filePath.c_str());

    width  = image.getWidth();
    height = image.getHeight();


    /* check condition for the bitmap to be initialized */
    if(width<0 || height <0)
        return SDK_FAILURE;

    pixels = image.getPixels(); 

    if(setupAESEncryptDecrypt()!=SDK_SUCCESS)
      return SDK_FAILURE;
    
    int timer = sampleCommon->createTimer();
    sampleCommon->resetTimer(timer);
    sampleCommon->startTimer(timer);

    if(setupCL()!=SDK_SUCCESS)
      return SDK_FAILURE;

    sampleCommon->stopTimer(timer);

    setupTime = (double)(sampleCommon->readTimer(timer));

    return SDK_SUCCESS;
}


int 
AESEncryptDecrypt::run()
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
    

    //XXX: Write output to an output Image
   
    convertGrayToPixels(output, pixels);
    image.write(outFilename.c_str());
 
    if(!quiet) {
        std::cout << "Output Filename : " << outFilename << std::endl;
    }
    
    
    return SDK_SUCCESS;
}

int 
AESEncryptDecrypt::verifyResults()
{
    if (verify && !decrypt)
    {
        verificationOutput = (cl_uchar *) malloc(width*height*sizeof(cl_uchar));
        if(verificationOutput==NULL)   { 
            sampleCommon->error("Failed to allocate host memory. (verificationOutput)");
            return SDK_FAILURE;
        }

        /* 
         * reference implementation
         */
        int refTimer = sampleCommon->createTimer();
        sampleCommon->resetTimer(refTimer);
        sampleCommon->startTimer(refTimer);
        AESEncryptDecryptCPUReference(verificationOutput, input, roundKey, explandedKeySize, 
                                                                width, height, decrypt);
        sampleCommon->stopTimer(refTimer);
        referenceKernelTime = sampleCommon->readTimer(refTimer);

        /* compare the results and see if they match */
        if(memcmp(output, verificationOutput, height*width*sizeof(cl_uchar)) == 0)
        {
            std::cout<<"Encryption Passed!\n";

            if (cleanup() != SDK_SUCCESS)
            {
                return SDK_FAILURE;
            }

            printStats();

            decrypt = true;
            char tempOutFilename[256];
            
            std::string tempInputFilename = inFilename.substr(0, (inFilename.length() - 4));
            sprintf(tempOutFilename, "%s_result.bmp", tempInputFilename.c_str());
            inFilename.assign(outFilename);
            outFilename.assign(tempOutFilename);

            seed = 123;
            input  = NULL;
            output = NULL;
            key    = NULL;
            verificationOutput = NULL;
            keySizeBits = 128;
            rounds = 10;
            setupTime = 0;
            totalKernelTime = 0;
            iterations = 1;

            if (setup() != SDK_SUCCESS)
            {
                return SDK_FAILURE;
            }

            if (run()!=SDK_SUCCESS)
            {
                return SDK_FAILURE;
            }

            if (verifyResults() != SDK_SUCCESS)
            {
                return SDK_FAILURE;
            }
            
            return SDK_SUCCESS;
        }
        else
        {  
            std::cout<<"Encryption Failed\n";
            return SDK_FAILURE;
        }
    }
    else if(verify && decrypt)
    {
        verificationOutput = (cl_uchar *) malloc(width*height*sizeof(cl_uchar));
        if(verificationOutput==NULL)   { 
            sampleCommon->error("Failed to allocate host memory. (verificationOutput)");
            return SDK_FAILURE;
        }

        /* 
         * reference implementation
         */
        int refTimer = sampleCommon->createTimer();
        sampleCommon->resetTimer(refTimer);
        sampleCommon->startTimer(refTimer);
        AESEncryptDecryptCPUReference(verificationOutput, input, roundKey, explandedKeySize, 
                                                                width, height, decrypt);
        sampleCommon->stopTimer(refTimer);
        referenceKernelTime = sampleCommon->readTimer(refTimer);

        /* compare the results and see if they match */
        if(memcmp(output, verificationOutput, height*width*sizeof(cl_uchar)) == 0)
        {
            std::cout<<"Decryption Passed!\n";
            return SDK_SUCCESS;
        }
        else
        {  
            std::cout<<"Decryption Failed\n";
            return SDK_FAILURE;
        }
    }

    return SDK_SUCCESS;
}

void AESEncryptDecrypt::printStats()
{
    std::string strArray[4] = {"Width", "Height", "Time(sec)", "KernelTime(sec)"};
    std::string stats[4];

    totalTime = setupTime + totalKernelTime;
    
    stats[0] = sampleCommon->toString(width    , std::dec);
    stats[1] = sampleCommon->toString(height   , std::dec);
    stats[2] = sampleCommon->toString(totalTime, std::dec);
    stats[3] = sampleCommon->toString(totalKernelTime, std::dec);

    this->SDKSample::printStats(strArray, stats, 4);
}

int AESEncryptDecrypt::cleanup()
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

   status = clReleaseMemObject(rKeyBuffer);
   if(!sampleCommon->checkVal(
      status,
      CL_SUCCESS,
      "clReleaseMemObject failed."))
      return SDK_FAILURE;

   status = clReleaseMemObject(sBoxBuffer);
   if(!sampleCommon->checkVal(
      status,
      CL_SUCCESS,
      "clReleaseMemObject failed."))
      return SDK_FAILURE;

   status = clReleaseMemObject(rsBoxBuffer);
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
    
    if(key)
        free(key);
    
    if(expandedKey)
        free(expandedKey);
    
    if(roundKey)
        free(roundKey);

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
    AESEncryptDecrypt clAESEncryptDecrypt("OpenCL AES Encrypt Decrypt");

    if(clAESEncryptDecrypt.initialize()!=SDK_SUCCESS)
        return SDK_FAILURE;
    if(!clAESEncryptDecrypt.parseCommandLine(argc, argv))
        return SDK_FAILURE;

    if(clAESEncryptDecrypt.isDumpBinaryEnabled())
    {
        return clAESEncryptDecrypt.genBinaryImage();
    }
    else
    {
        if(clAESEncryptDecrypt.setup()!=SDK_SUCCESS)
            return SDK_FAILURE;
        if(clAESEncryptDecrypt.run()!=SDK_SUCCESS)
            return SDK_FAILURE;
        if(clAESEncryptDecrypt.verifyResults()!=SDK_SUCCESS)
            return SDK_FAILURE;
        if(clAESEncryptDecrypt.cleanup()!=SDK_SUCCESS)
            return SDK_FAILURE;
        clAESEncryptDecrypt.printStats();
    }

    return SDK_SUCCESS;
}

