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

#include <iostream>
using namespace std;

#include "Mandelbrot.hpp"

union uchar4
{
    struct __uchar_four
    {
        unsigned char s0;
        unsigned char s1;
        unsigned char s2;
        unsigned char s3;
    } ch;
    cl_uint num;
};

struct int4;
struct float4
{
    float s0;
    float s1;
    float s2;
    float s3;

    float4 operator * (float4 &fl)
    {
        float4 temp;
        temp.s0 = (this->s0) * fl.s0;
        temp.s1 = (this->s1) * fl.s1;
        temp.s2 = (this->s2) * fl.s2;
        temp.s3 = (this->s3) * fl.s3;
        return temp;
    }

    float4 operator * (float scalar)
    {
        float4 temp;
        temp.s0 = (this->s0) * scalar;
        temp.s1 = (this->s1) * scalar;
        temp.s2 = (this->s2) * scalar;
        temp.s3 = (this->s3) * scalar;
        return temp;
    }
        
    float4 operator + (float4 &fl)
    {
        float4 temp;
        temp.s0 = (this->s0) + fl.s0;
        temp.s1 = (this->s1) + fl.s1;
        temp.s2 = (this->s2) + fl.s2;
        temp.s3 = (this->s3) + fl.s3;
        return temp;
    }
    
    float4 operator - (float4 fl)
    {
        float4 temp;
        temp.s0 = (this->s0) - fl.s0;
        temp.s1 = (this->s1) - fl.s1;
        temp.s2 = (this->s2) - fl.s2;
        temp.s3 = (this->s3) - fl.s3;
        return temp;
    }

    friend float4 operator * (float scalar, float4 &fl);
    friend float4 convert_float4(int4 i);
};

float4 operator * (float scalar, float4 &fl)
{
    float4 temp;
    temp.s0 = fl.s0 * scalar;
    temp.s1 = fl.s1 * scalar;
    temp.s2 = fl.s2 * scalar;
    temp.s3 = fl.s3 * scalar;
    return temp;
}

struct int4
{
    int s0;
    int s1;
    int s2;
    int s3;

    int4 operator * (int4 &fl)
    {
        int4 temp;
        temp.s0 = (this->s0) * fl.s0;
        temp.s1 = (this->s1) * fl.s1;
        temp.s2 = (this->s2) * fl.s2;
        temp.s3 = (this->s3) * fl.s3;
        return temp;
    }

    int4 operator * (int scalar)
    {
        int4 temp;
        temp.s0 = (this->s0) * scalar;
        temp.s1 = (this->s1) * scalar;
        temp.s2 = (this->s2) * scalar;
        temp.s3 = (this->s3) * scalar;
        return temp;
    }
        
    int4 operator + (int4 &fl)
    {
        int4 temp;
        temp.s0 = (this->s0) + fl.s0;
        temp.s1 = (this->s1) + fl.s1;
        temp.s2 = (this->s2) + fl.s2;
        temp.s3 = (this->s3) + fl.s3;
        return temp;
    }
    
    int4 operator - (int4 fl)
    {
        int4 temp;
        temp.s0 = (this->s0) - fl.s0;
        temp.s1 = (this->s1) - fl.s1;
        temp.s2 = (this->s2) - fl.s2;
        temp.s3 = (this->s3) - fl.s3;
        return temp;
    }

    int4 operator += (int4 fl)
    {
        s0 += fl.s0;
        s1 += fl.s1;
        s2 += fl.s2;
        s3 += fl.s3;
        return (*this);
    }

    friend float4 convert_float4(int4 i);
};

float4 convert_float4(int4 i)
{
    float4 temp;
    temp.s0 = (float)i.s0;
    temp.s1 = (float)i.s1;
    temp.s2 = (float)i.s2;
    temp.s3 = (float)i.s3;
    return temp;
}

inline float native_log2(float in)
{
    return log(in)/log(2.0f);
}

inline float native_cos(float in)
{
    return cos(in);
}

#ifndef min
int min(int a1, int a2)
{
    return ((a1 < a2) ? a1 : a2);
}
#endif

int 
Mandelbrot::setupMandelbrot()
{
    cl_uint sizeBytes;

    /* allocate and init memory used by host */
    sizeBytes = width * height * sizeof(cl_uchar4);
    output = (cl_uint *) malloc(sizeBytes);
    if(output==NULL)    
    { 
        sampleCommon->error("Failed to allocate host memory. (output)");
        return SDK_FAILURE;
    }

    if(verify)
    {
        verificationOutput = (cl_uint *)malloc(sizeBytes);
        if(verificationOutput==NULL)    
        { 
            sampleCommon->error("Failed to allocate host memory. (verificationOutput)");
            return SDK_FAILURE;
        }

    }

    return SDK_SUCCESS;
}

int 
Mandelbrot::genBinaryImage()
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
    kernelPath.append("Mandelbrot_Kernels.cl");
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
Mandelbrot::setupCL(void)
{
    cl_int status = 0;
    size_t deviceListSize;

    if(deviceType.compare("cpu") == 0)
    {
        dType = CL_DEVICE_TYPE_CPU;
    }
    else if(deviceType.compare("all") == 0)
    {
        dType = CL_DEVICE_TYPE_ALL;
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
     * If we could find our platform, use it. Otherwise pass a NULL and get whatever the
     * implementation thinks we should be using.
     */

    cl_context_properties cps[3] = 
    {
        CL_CONTEXT_PLATFORM, 
        (cl_context_properties)platform, 
        0
    };
    /* Use NULL for backward compatibility */
    cl_context_properties* cprops = (NULL == platform) ? NULL : cps;

    context = clCreateContextFromType(
                  cprops,
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

    /* Now allocate memory for device list based on the size we got earlier */
    devices = (cl_device_id *)malloc(deviceListSize);

    if(devices==NULL)
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

    numDevices = (cl_uint)(deviceListSize/sizeof(cl_device_id));
    numDevices = min(MAX_DEVICES, numDevices);

    if(numDevices != 1 && isLoadBinaryEnabled())
    {
        sampleCommon->expectedError("--load option is not supported if devices are more one");
        return SDK_EXPECTED_FAILURE;
    }

    if(numDevices == 3)
    {
        if(!quiet)
        {
            cout << "Number of devices must be even,"
                 << "\nChanging number of devices from three to two\n";
        }
        numDevices = 2;
    }

    for (cl_uint i = 0; i < numDevices; i++)
    {
        /* The block is to move the declaration of prop closer to its use */
        cl_command_queue_properties prop = 0;
        if(timing)
            prop |= CL_QUEUE_PROFILING_ENABLE;

        commandQueue[i] = clCreateCommandQueue(
                context, 
                devices[i], 
                prop, 
                &status);
        if(!sampleCommon->checkVal(
                    status,
                    0,
                    "clCreateCommandQueue failed."))
            return SDK_FAILURE;

        outputBuffer[i] = clCreateBuffer(
                context, 
                CL_MEM_WRITE_ONLY | CL_MEM_USE_HOST_PTR,
                (sizeof(cl_uint) * width * height) / numDevices,
                output + ((width * height) / numDevices) * i, 
                &status);
        if(!sampleCommon->checkVal(
                    status,
                    CL_SUCCESS,
                    "clCreateBuffer failed. (outputBuffer)"))
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

		kernelPath.append("Mandelbrot_Kernels.cl");
		if(!kernelFile.open(kernelPath.c_str()))
		{
			std::cout << "Failed to load kernel file : " << kernelPath << std::endl;
			return SDK_FAILURE;
		}

		const char * source = platformIsPacketizedOpenCL ?
			"Mandelbrot_Kernels.bc" :
			kernelFile.source().c_str();

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
                            numDevices, 
                            devices, 
                            flagsStr.c_str(), 
                            NULL, 
                            NULL);

    if(status != CL_SUCCESS)
    {
        if(status == CL_BUILD_PROGRAM_FAILURE)
        {
            for (cl_uint i = 0; i < numDevices; i++)
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
                    sampleCommon->error("Failed to allocate host memory."
                        "(buildLog)");
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
        }

        if(!sampleCommon->checkVal(
            status,
            CL_SUCCESS,
            "clBuildProgram failed."))
            return SDK_FAILURE;
    }

    for (cl_uint i = 0; i < numDevices; i++)
    {
        
       
        /* get a kernel object handle for a kernel with the given name */
        kernel_vector[i] = clCreateKernel(program, "mandelbrot_vector", &status);
        if(!sampleCommon->checkVal(
                    status,
                    CL_SUCCESS,
                    "clCreateKernel failed."))
            return SDK_FAILURE;
    }

    return SDK_SUCCESS;
}


int 
Mandelbrot::runCLKernels(void)
{
    cl_int   status;
    cl_event events[MAX_DEVICES];
    cl_kernel kernel;

    size_t globalThreads[1];
    size_t localThreads[1];

    benched = 0;
    globalThreads[0] = (width * height) / numDevices;
    localThreads[0]  = 256;

    globalThreads[0] >>= 2;

    for (cl_uint i = 0; i < numDevices; i++)
    {
        kernel = kernel_vector[i];

        /* Check group size against kernelWorkGroupSize */
        status = clGetKernelWorkGroupInfo(kernel,
                                          devices[i],
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
            localThreads[0] = kernelWorkGroupSize;


        double aspect = (double)width / (double)height;
        xstep = (float)(xsize / (double)width);
        // Adjust for aspect ratio
        double ysize = xsize / aspect;
        ystep = (float)(-(xsize / aspect) / height);
        leftx = (float)(xpos - xsize / 2.0);
        topy = (float)(ypos + ysize / 2.0 -((double)i * ysize) / (double)numDevices);

        if(i == 0)
        {
            topy0 = topy;
        }

        /*** Set appropriate arguments to the kernel ***/
        status = clSetKernelArg(
                        kernel, 
                        0, 
                        sizeof(cl_mem),
                       (void *)&outputBuffer[i]);
        if(!sampleCommon->checkVal(
                    status,
                    CL_SUCCESS,
                    "clSetKernelArg failed. (outputBuffer)"))
            return SDK_FAILURE;

        status = clSetKernelArg(
                        kernel, 
                        1, 
                        sizeof(cl_float),
                       (void *)&leftx);
        if(!sampleCommon->checkVal(
                    status,
                    CL_SUCCESS,
                    "clSetKernelArg failed. (leftx)"))
            return SDK_FAILURE;

        status = clSetKernelArg(
                        kernel, 
                        2, 
                        sizeof(cl_float),
                       (void *)&topy);
        if(!sampleCommon->checkVal(
                    status,
                    CL_SUCCESS,
                    "clSetKernelArg failed. (topy)"))
            return SDK_FAILURE;

        status = clSetKernelArg(
                        kernel, 
                        3, 
                        sizeof(cl_float),
                       (void *)&xstep);
        if(!sampleCommon->checkVal(
                    status,
                    CL_SUCCESS,
                    "clSetKernelArg failed. (xstep)"))
            return SDK_FAILURE;

        status = clSetKernelArg(
                        kernel, 
                        4, 
                        sizeof(cl_float),
                       (void *)&ystep);
        if(!sampleCommon->checkVal(
                    status,
                    CL_SUCCESS,
                    "clSetKernelArg failed. (ystep)"))
            return SDK_FAILURE;

        status = clSetKernelArg(
                        kernel, 
                        5, 
                        sizeof(cl_uint), 
                        (void *)&maxIterations);
        if(!sampleCommon->checkVal(
                    status,
                    CL_SUCCESS,
                    "clSetKernelArg failed. (maxIterations)"))
            return SDK_FAILURE;

        /* width - i.e number of elements in the array */
        status = clSetKernelArg(
                        kernel, 
                        6, 
                        sizeof(cl_int), 
                        (void *)&width);
        if(!sampleCommon->checkVal(
                    status,
                    CL_SUCCESS,
                    "clSetKernelArg failed. (width)"))
            return SDK_FAILURE;

        /* bench - flag to indicate benchmark mode */
        status = clSetKernelArg(
                        kernel, 
                        7, 
                        sizeof(cl_int), 
                        (void *)&bench);
        if(!sampleCommon->checkVal(
                    status,
                    CL_SUCCESS,
                    "clSetKernelArg failed. (bench)"))
            return SDK_FAILURE;

        /* 
         * Enqueue a kernel run call.
         */
        status = clEnqueueNDRangeKernel(
                commandQueue[i],
                kernel,
                1,
                NULL,
                globalThreads,
                localThreads,
                0,
                NULL,
                &events[i]);

        if(!sampleCommon->checkVal(
                    status,
                    CL_SUCCESS,
                    "clEnqueueNDRangeKernel failed."))
            return SDK_FAILURE;
    }
    /* flush the queues to get things started */
    for (cl_uint i = 0; i < numDevices; i++)
    {
        clFlush(commandQueue[i]);
    }


    /* wait for the kernel call to finish execution */
    for (cl_uint i = 0; i < numDevices; i++)
    {
        // Wait in reverse order
        status = clWaitForEvents(1, &events[numDevices-i-1]);
        if(!sampleCommon->checkVal(
                    status,
                    CL_SUCCESS,
                    "clWaitForEvents failed."))
            return SDK_FAILURE;
    }

    if (timing && bench)
    {
        cl_ulong start;
        cl_ulong stop;
        status = clGetEventProfilingInfo(events[0],
                                         CL_PROFILING_COMMAND_SUBMIT,
                                         sizeof(cl_ulong),
                                         &start,
                                         NULL);
        if(!sampleCommon->checkVal(
                    status,
                    CL_SUCCESS,
                    "clGetEventProfilingInfo failed."))
            return SDK_FAILURE;

        status = clGetEventProfilingInfo(events[0],
                                         CL_PROFILING_COMMAND_END,
                                         sizeof(cl_ulong),
                                         &stop,
                                         NULL);
        if(!sampleCommon->checkVal(
                    status,
                    CL_SUCCESS,
                    "clGetEventProfilingInfo failed."))
            return SDK_FAILURE;

        time = (cl_double)(stop - start)*(cl_double)(1e-09);
    }
    for (cl_uint i = 0; i < numDevices; i++)
    {
        /* Enqueue readBuffer*/
        status = clEnqueueReadBuffer(
                    commandQueue[i],
                    outputBuffer[i],
                    CL_TRUE,
                    0,
                    (width * height * sizeof(cl_int)) / numDevices,
                    output + (width * height / numDevices) * i,
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

    if (timing && bench)
    {
        cl_ulong totalIterations = 0;
        for (int i = 0; i < (width * height); i++)
        {
            totalIterations += output[i];
        }
        cl_double flops = 7.0*totalIterations;
        printf("%lf MFLOPs\n", flops*(double)(1e-6)/time);
        printf("%lf MFLOPs according to CPU\n", flops*(double)(1e-6)/totalKernelTime);
        bench = 0;
        benched = 1;
    }
    clReleaseEvent(events[0]);
    return SDK_SUCCESS;
}

/**
* Mandelbrot fractal generated with CPU reference implementation
*/

void 
Mandelbrot::mandelbrotCPUReference(cl_uint * verificationOutput,
                                   cl_float posx, 
                                   cl_float posy, 
                                   cl_float stepSizeX,
                                   cl_float stepSizeY,
                                   cl_int maxIterations,
                                   cl_int width,
                                   cl_int bench
                                  )
{
    int tid;

    for(tid = 0; tid < (height * width / 4); tid++)
    {				
        int i = tid%(width/4);
        int j = tid/(width/4); 

        int4 veci = {4*i, 4*i+1, 4*i+2, 4*i+3};
        int4 vecj = {j, j, j, j};
        float4 x0;
        x0.s0 = (float)(posx + stepSizeX * (float)veci.s0);
        x0.s1 = (float)(posx + stepSizeX * (float)veci.s1);
        x0.s2 = (float)(posx + stepSizeX * (float)veci.s2);
        x0.s3 = (float)(posx + stepSizeX * (float)veci.s3);
        float4 y0;
        y0.s0 = (float)(posy + stepSizeY * (float)vecj.s0);
        y0.s1 = (float)(posy + stepSizeY * (float)vecj.s1);
        y0.s2 = (float)(posy + stepSizeY * (float)vecj.s2);
        y0.s3 = (float)(posy + stepSizeY * (float)vecj.s3);

        float4 x = x0;
        float4 y = y0;
        
        cl_int iter=0;
        float4 tmp;
        int4 stay;
        int4 ccount = {0, 0, 0, 0};

        stay.s0 = (x.s0*x.s0 + y.s0*y.s0) <= 4.0f;
        stay.s1 = (x.s1*x.s1 + y.s1*y.s1) <= 4.0f;
        stay.s2 = (x.s2*x.s2 + y.s2*y.s2) <= 4.0f;
        stay.s3 = (x.s3*x.s3 + y.s3*y.s3) <= 4.0f;
        float4 savx = x;
        float4 savy = y;

        for(iter=0; (stay.s0 | stay.s1 | stay.s2 | stay.s3) && (iter < maxIterations); iter+= 16)
        {
            x = savx;
            y = savy;

            // Two iterations
            tmp = x * x + x0 - y * y;
            y = 2.0f * x * y + y0;
            x = tmp * tmp + x0 - y * y;
            y = 2.0f * tmp * y + y0;

            // Two iterations
            tmp = x * x + x0 - y * y;
            y = 2.0f * x * y + y0;
            x = tmp * tmp + x0 - y * y;
            y = 2.0f * tmp * y + y0;

            // Two iterations
            tmp = x * x + x0 - y * y;
            y = 2.0f * x * y + y0;
            x = tmp * tmp + x0 - y * y;
            y = 2.0f * tmp * y + y0;

            // Two iterations
            tmp = x * x + x0 - y * y;
            y = 2.0f * x * y + y0;
            x = tmp * tmp + x0 - y * y;
            y = 2.0f * tmp * y + y0;

            // Two iterations
            tmp = x * x + x0 - y * y;
            y = 2.0f * x * y + y0;
            x = tmp * tmp + x0 - y * y;
            y = 2.0f * tmp * y + y0;

            // Two iterations
            tmp = x * x + x0 - y * y;
            y = 2.0f * x * y + y0;
            x = tmp * tmp + x0 - y * y;
            y = 2.0f * tmp * y + y0;

            // Two iterations
            tmp = x * x + x0 - y * y;
            y = 2.0f * x * y + y0;
            x = tmp * tmp + x0 - y * y;
            y = 2.0f * tmp * y + y0;

            // Two iterations
            tmp = x * x + x0 - y * y;
            y = 2.0f * x * y + y0;
            x = tmp * tmp + x0 - y * y;
            y = 2.0f * tmp * y + y0;

            stay.s0 = (x.s0*x.s0 + y.s0*y.s0) <= 4.0f;
            stay.s1 = (x.s1*x.s1 + y.s1*y.s1) <= 4.0f;
            stay.s2 = (x.s2*x.s2 + y.s2*y.s2) <= 4.0f;
            stay.s3 = (x.s3*x.s3 + y.s3*y.s3) <= 4.0f;

            savx.s0 = (stay.s0 ? x.s0 : savx.s0);
            savx.s1 = (stay.s1 ? x.s1 : savx.s1);
            savx.s2 = (stay.s2 ? x.s2 : savx.s2);
            savx.s3 = (stay.s3 ? x.s3 : savx.s3);
            savy.s0 = (stay.s0 ? y.s0 : savy.s0);
            savy.s1 = (stay.s1 ? y.s1 : savy.s1);
            savy.s2 = (stay.s2 ? y.s2 : savy.s2);
            savy.s3 = (stay.s3 ? y.s3 : savy.s3);
            ccount += stay*16;
        }
        // Handle remainder
        if (!(stay.s0 & stay.s1 & stay.s2 & stay.s3))
        {
            iter = 16;
            do
            {
                x = savx;
                y = savy;
                stay.s0 = ((x.s0*x.s0 + y.s0*y.s0) <= 4.0f) && (ccount.s0 < maxIterations);
                stay.s1 = ((x.s1*x.s1 + y.s1*y.s1) <= 4.0f) && (ccount.s1 < maxIterations);
                stay.s2 = ((x.s2*x.s2 + y.s2*y.s2) <= 4.0f) && (ccount.s2 < maxIterations);
                stay.s3 = ((x.s3*x.s3 + y.s3*y.s3) <= 4.0f) && (ccount.s3 < maxIterations);
                tmp = x;
                x = x * x + x0 - y * y;
                y = 2.0f * tmp * y + y0;
                ccount += stay;
                iter--;
                savx.s0 = (stay.s0 ? x.s0 : savx.s0);
                savx.s1 = (stay.s1 ? x.s1 : savx.s1);
                savx.s2 = (stay.s2 ? x.s2 : savx.s2);
                savx.s3 = (stay.s3 ? x.s3 : savx.s3);
                savy.s0 = (stay.s0 ? y.s0 : savy.s0);
                savy.s1 = (stay.s1 ? y.s1 : savy.s1);
                savy.s2 = (stay.s2 ? y.s2 : savy.s2);
                savy.s3 = (stay.s3 ? y.s3 : savy.s3);
            } while ((stay.s0 | stay.s1 | stay.s2 | stay.s3) && iter);
        }
        x = savx;
        y = savy;
        float4 fc = convert_float4(ccount);	

        fc.s0 = (float)ccount.s0 + 1 - native_log2(native_log2(x.s0*x.s0 + y.s0*y.s0));
        fc.s1 = (float)ccount.s1 + 1 - native_log2(native_log2(x.s1*x.s1 + y.s1*y.s1));
        fc.s2 = (float)ccount.s2 + 1 - native_log2(native_log2(x.s2*x.s2 + y.s2*y.s2));
        fc.s3 = (float)ccount.s3 + 1 - native_log2(native_log2(x.s3*x.s3 + y.s3*y.s3));

        float c = fc.s0 * 2.0f * 3.1416f / 256.0f;
        uchar4 color[4];
        color[0].ch.s0 = (unsigned char)(((1.0f + native_cos(c))*0.5f)*255);
        color[0].ch.s1 = (unsigned char)(((1.0f + native_cos(2.0f*c + 2.0f*3.1416f/3.0f))*0.5f)*255);
        color[0].ch.s2 = (unsigned char)(((1.0f + native_cos(c - 2.0f*3.1416f/3.0f))*0.5f)*255);
        color[0].ch.s3 = 0xff;
        if (ccount.s0 == maxIterations)
        {
            color[0].ch.s0 = 0;
            color[0].ch.s1 = 0;
            color[0].ch.s2 = 0;
        }
        if (bench)
        {
            color[0].ch.s0 = ccount.s0 & 0xff;
            color[0].ch.s1 = (ccount.s0 & 0xff00)>>8;
            color[0].ch.s2 = (ccount.s0 & 0xff0000)>>16;
            color[0].ch.s3 = (ccount.s0 & 0xff000000)>>24;
        }
        verificationOutput[4*tid] = color[0].num;    

        c = fc.s1 * 2.0f * 3.1416f / 256.0f;
        color[1].ch.s0 = (unsigned char)(((1.0f + native_cos(c))*0.5f)*255);
        color[1].ch.s1 = (unsigned char)(((1.0f + native_cos(2.0f*c + 2.0f*3.1416f/3.0f))*0.5f)*255);
        color[1].ch.s2 = (unsigned char)(((1.0f + native_cos(c - 2.0f*3.1416f/3.0f))*0.5f)*255);
        color[1].ch.s3 = 0xff;
        if (ccount.s1 == maxIterations)
        {
            color[1].ch.s0 = 0;
            color[1].ch.s1 = 0;
            color[1].ch.s2 = 0;
        }
        if (bench)
        {
            color[1].ch.s0 = ccount.s1 & 0xff;
            color[1].ch.s1 = (ccount.s1 & 0xff00)>>8;
            color[1].ch.s2 = (ccount.s1 & 0xff0000)>>16;
            color[1].ch.s3 = (ccount.s1 & 0xff000000)>>24;
        }
        verificationOutput[4*tid+1] = color[1].num;    

        c = fc.s2 * 2.0f * 3.1416f / 256.0f;
        color[2].ch.s0 = (unsigned char)(((1.0f + native_cos(c))*0.5f)*255);
        color[2].ch.s1 = (unsigned char)(((1.0f + native_cos(2.0f*c + 2.0f*3.1416f/3.0f))*0.5f)*255);
        color[2].ch.s2 = (unsigned char)(((1.0f + native_cos(c - 2.0f*3.1416f/3.0f))*0.5f)*255);
        color[2].ch.s3 = 0xff;
        if (ccount.s2 == maxIterations)
        {
            color[2].ch.s0 = 0;
            color[2].ch.s1 = 0;
            color[2].ch.s2 = 0;
        }
        if (bench)
        {
            color[2].ch.s0 = ccount.s2 & 0xff;
            color[2].ch.s1 = (ccount.s2 & 0xff00)>>8;
            color[2].ch.s2 = (ccount.s2 & 0xff0000)>>16;
            color[2].ch.s3 = (ccount.s2 & 0xff000000)>>24;
        }
        verificationOutput[4*tid+2] = color[2].num;    

        c = fc.s3 * 2.0f * 3.1416f / 256.0f;
        color[3].ch.s0 = (unsigned char)(((1.0f + native_cos(c))*0.5f)*255);
        color[3].ch.s1 = (unsigned char)(((1.0f + native_cos(2.0f*c + 2.0f*3.1416f/3.0f))*0.5f)*255);
        color[3].ch.s2 = (unsigned char)(((1.0f + native_cos(c - 2.0f*3.1416f/3.0f))*0.5f)*255);
        color[3].ch.s3 = 0xff;
        if (ccount.s3 == maxIterations)
        {
            color[3].ch.s0 = 0;
            color[3].ch.s1 = 0;
            color[3].ch.s2 = 0;
        }
        if (bench)
        {
            color[3].ch.s0 = ccount.s3 & 0xff;
            color[3].ch.s1 = (ccount.s3 & 0xff00)>>8;
            color[3].ch.s2 = (ccount.s3 & 0xff0000)>>16;
            color[3].ch.s3 = (ccount.s3 & 0xff000000)>>24;
        }
        verificationOutput[4*tid+3] = color[3].num;    
    }
}


int Mandelbrot::initialize()
{
    // Call base class Initialize to get default configuration
    if(!this->SDKSample::initialize())
        return SDK_FAILURE;

    streamsdk::Option* image_width = new streamsdk::Option;
    if(!image_width)
    {
        sampleCommon->error("Memory allocation error.\n");
        return SDK_FAILURE;
    }
    
    image_width->_sVersion = "W";
    image_width->_lVersion = "width";
    image_width->_description = "width of the mandelbrot image";
    image_width->_type = streamsdk::CA_ARG_INT;
    image_width->_value = &width;
    sampleArgs->AddOption(image_width);
    delete image_width;

    streamsdk::Option* image_height = new streamsdk::Option;
    if(!image_height)
    {
        sampleCommon->error("Memory allocation error.\n");
        return SDK_FAILURE;
    }
    
    image_height->_sVersion = "H";
    image_height->_lVersion = "height";
    image_height->_description = "height of the mandelbrot image";
    image_height->_type = streamsdk::CA_ARG_INT;
    image_height->_value = &height;
    sampleArgs->AddOption(image_height);
    delete image_height;

    streamsdk::Option* xpos_param = new streamsdk::Option;
    if(!xpos_param)
    {
        sampleCommon->error("Memory allocation error.\n");
        return SDK_FAILURE;
    }

    xpos_param->_sVersion = "x";
    xpos_param->_lVersion = "xpos";
    xpos_param->_description = "xpos to generate the mandelbrot fractal";
    xpos_param->_type = streamsdk::CA_ARG_STRING;
    xpos_param->_value = &xpos_str;
    sampleArgs->AddOption(xpos_param);
    delete xpos_param;

    streamsdk::Option* ypos_param = new streamsdk::Option;
    if(!ypos_param)
    {
        sampleCommon->error("Memory allocation error.\n");
        return SDK_FAILURE;
    }

    ypos_param->_sVersion = "y";
    ypos_param->_lVersion = "ypos";
    ypos_param->_description = "ypos to generate the mandelbrot fractal";
    ypos_param->_type = streamsdk::CA_ARG_STRING;
    ypos_param->_value = &ypos_str;
    sampleArgs->AddOption(ypos_param);
    delete ypos_param;

    streamsdk::Option* xsize_param = new streamsdk::Option;
    if(!xsize_param)
    {
        sampleCommon->error("Memory allocation error.\n");
        return SDK_FAILURE;
    }

    xsize_param->_sVersion = "xs";
    xsize_param->_lVersion = "xsize";
    xsize_param->_description = "Width of window for the mandelbrot fractal";
    xsize_param->_type = streamsdk::CA_ARG_STRING;
    xsize_param->_value = &xsize_str;
    sampleArgs->AddOption(xsize_param);
    delete xsize_param;

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

    if (xpos_str != "")
    {
        xpos = atof(xpos_str.c_str());
    }
    if (ypos_str != "")
    {
        ypos = atof(ypos_str.c_str());
    }
    if (xsize_str != "")
    {
        xsize = atof(xsize_str.c_str());
    }
    else
    {
        xsize = 4.0;
    }
    return SDK_SUCCESS;
}

int Mandelbrot::setup()
{
    // Make sure width is a multiple of 4
    width = (width + 3) & ~(4 - 1);

    iterations = 1;

    if(setupMandelbrot()!=SDK_SUCCESS)
        return SDK_FAILURE;

    int timer = sampleCommon->createTimer();
    sampleCommon->resetTimer(timer);
    sampleCommon->startTimer(timer);

    int returnVal = setupCL();
    if(returnVal != SDK_SUCCESS)
        return (returnVal == SDK_EXPECTED_FAILURE)? SDK_EXPECTED_FAILURE : SDK_FAILURE;

    sampleCommon->stopTimer(timer);

    setupTime = (cl_double)sampleCommon->readTimer(timer);

    return SDK_SUCCESS;
}


int Mandelbrot::run()
{
    int timer = sampleCommon->createTimer();
    sampleCommon->resetTimer(timer);
    sampleCommon->startTimer(timer);   

    for(int i = 0; i < iterations; i++)
    {
        /* Arguments are set and execution call is enqueued on command buffer */
        if(runCLKernels()!=SDK_SUCCESS)
            return SDK_FAILURE;
    }

    sampleCommon->stopTimer(timer);
    totalKernelTime = (double)(sampleCommon->readTimer(timer)) / iterations;

    return SDK_SUCCESS;
}

int Mandelbrot::verifyResults()
{
    if(verify)
    {
        /* reference implementation
         * it overwrites the input array with the output
         */
        int refTimer = sampleCommon->createTimer();
        sampleCommon->resetTimer(refTimer);
        sampleCommon->startTimer(refTimer);
        mandelbrotCPUReference(verificationOutput, leftx, topy0, xstep, ystep, maxIterations, width, bench);
        sampleCommon->stopTimer(refTimer);
        referenceKernelTime = sampleCommon->readTimer(refTimer);

        int i, j;
        int counter = 0;
       
        for(j = 0; j < height; j++)
        {
            for(i = 0; i < width; i++)
            {
                uchar4 temp_ver, temp_out;
                temp_ver.num = verificationOutput[j * width + i];
                temp_out.num = output[j * width + i];

                unsigned char threshold = 2;

                if( ((temp_ver.ch.s0 - temp_out.ch.s0) > threshold) ||
                    ((temp_out.ch.s0 - temp_ver.ch.s0) > threshold) ||

                    ((temp_ver.ch.s1 - temp_out.ch.s1) > threshold) ||
                    ((temp_out.ch.s1 - temp_ver.ch.s1) > threshold) ||

                    ((temp_ver.ch.s2 - temp_out.ch.s2) > threshold) ||
                    ((temp_out.ch.s2 - temp_ver.ch.s2) > threshold) ||

                    ((temp_ver.ch.s3 - temp_out.ch.s3) > threshold) ||
                    ((temp_out.ch.s3 - temp_ver.ch.s3) > threshold))
                {
                    counter++;
                }

            }
        }

        int numPixels = height * width;
        double ratio = (double)counter / numPixels;
        
        // compare the results and see if they match 
        
        if( ratio < 0.002)
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

void Mandelbrot::printStats()
{
    std::string strArray[4] = {"Width", "Height", "Time(sec)", "KernelTime(sec)"};
    std::string stats[4];

    totalTime = setupTime + totalKernelTime;

    stats[0] = sampleCommon->toString(width, std::dec);
    stats[1] = sampleCommon->toString(height, std::dec);
    stats[2] = sampleCommon->toString(totalTime, std::dec);
    stats[3] = sampleCommon->toString(totalKernelTime, std::dec);

    this->SDKSample::printStats(strArray, stats, 4);
}

int Mandelbrot::cleanup()
{
    /* Releases OpenCL resources (Context, Memory etc.) */
    cl_int status;

    status = clReleaseProgram(program);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clReleaseProgram failed."))
        return SDK_FAILURE;

    for (cl_uint i = 0; i < numDevices; i++)
    {
        status = clReleaseKernel(kernel_vector[i]);
        if(!sampleCommon->checkVal(
            status,
            CL_SUCCESS,
            "clReleaseKernel failed."))
            return SDK_FAILURE;

        status = clReleaseMemObject(outputBuffer[i]);
        if(!sampleCommon->checkVal(
                    status,
                    CL_SUCCESS,
                    "clReleaseMemObject failed."))
            return SDK_FAILURE;

        status = clReleaseCommandQueue(commandQueue[i]);
        if(!sampleCommon->checkVal(
            status,
            CL_SUCCESS,
            "clReleaseCommandQueue failed."))
            return SDK_FAILURE;
    }

    status = clReleaseContext(context);
    if(!sampleCommon->checkVal(
                status,
                CL_SUCCESS,
                "clReleaseContext failed."))
        return SDK_FAILURE;

    /* release program resources (input memory etc.) */
    if(output) 
        free(output);

    if(verificationOutput) 
        free(verificationOutput);

    if(devices)
        free(devices);

    return SDK_SUCCESS;
}

cl_uint Mandelbrot::getWidth(void)
{
    return width;
}

cl_uint Mandelbrot::getHeight(void)
{
    return height;
}


cl_uint * Mandelbrot::getPixels(void)
{
    return output;
}

cl_bool Mandelbrot::showWindow(void)
{
    return !quiet && !verify;
}
