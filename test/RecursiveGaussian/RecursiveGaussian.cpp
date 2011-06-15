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


#include "RecursiveGaussian.hpp"
#include <cmath>


int
RecursiveGaussian::readInputImage(std::string inputImageName)
{
    /* load input bitmap image */
    inputBitmap.load(inputImageName.c_str());

    /* error if image did not load */
    if(!inputBitmap.isLoaded())
    {
        sampleCommon->error("Failed to load input image!");
        return SDK_FAILURE;
    }

    /* get width and height of input image */
    height = inputBitmap.getHeight();
    width = inputBitmap.getWidth();

    /* Check width against blockSizeX */
    if(width % GROUP_SIZE || height % GROUP_SIZE)
    {
        char err[2048];
        sprintf(err, "Width should be a multiple of %d \n", GROUP_SIZE); 
        sampleCommon->error(err);
        return SDK_FAILURE;
    }

    /* allocate memory for input & output image data  */
    inputImageData  = (cl_uchar4*)malloc(width * height * sizeof(cl_uchar4));
    verificationInput = (cl_uchar4*)malloc(width * height * sizeof(cl_uchar4));

    /* error check */
    if(inputImageData == NULL)
    {
        sampleCommon->error("Failed to allocate memory! (inputImageData)");
        return SDK_FAILURE;
    }

    /* allocate memory for output image data */
    outputImageData = (cl_uchar4*)malloc(width * height * sizeof(cl_uchar4));

    /* error check */
    if(outputImageData == NULL)
    {
        sampleCommon->error("Failed to allocate memory! (outputImageData)");
        return SDK_FAILURE;
    }

    /* initializa the Image data to NULL */
    memset(outputImageData, 0, width * height * sizeof(cl_uchar4));

    /* get the pointer to pixel data */
    pixelData = inputBitmap.getPixels();

    /* error check */
    if(pixelData == NULL)
    {
        sampleCommon->error("Failed to read pixel Data!");
        return SDK_FAILURE;
    }

    /* Copy pixel data into inputImageData */
    memcpy(inputImageData, pixelData, width * height * sizeof(cl_uchar4));
    memcpy(verificationInput, pixelData, width * height * sizeof(cl_uchar4));

    /* allocate memory for verification output */
    verificationOutput = (cl_uchar4*)malloc(width * height * sizeof(cl_uchar4));

    /* error check */
    if(verificationOutput == NULL)
    {
        sampleCommon->error("Failed to allocate memory! (verificationOutput)");
        return SDK_FAILURE;
    }

    /* initialize the data to NULL */
    memset(verificationOutput, 0, width * height * sizeof(cl_uchar4));

    return SDK_SUCCESS;

}


int
RecursiveGaussian::writeOutputImage(std::string outputImageName)
{
    /* copy output image data back to original pixel data */
    memcpy(pixelData, outputImageData, width * height * sizeof(cl_uchar4));

    /* write the output bmp file */
    if(!inputBitmap.write(outputImageName.c_str()))
    {
        sampleCommon->error("Failed to write output image!");
        return SDK_FAILURE;
    }

    return SDK_SUCCESS;
}


void
RecursiveGaussian::computeGaussParms(float fSigma, int iOrder, GaussParms* pGP)
{
    // pre-compute filter coefficients
    pGP->nsigma = fSigma; // note: fSigma is range-checked and clamped >= 0.1f upstream
    pGP->alpha = 1.695f / pGP->nsigma;
    pGP->ema = exp(-pGP->alpha);
    pGP->ema2 = exp(-2.0f * pGP->alpha);
    pGP->b1 = -2.0f * pGP->ema;
    pGP->b2 = pGP->ema2;
    pGP->a0 = 0.0f;
    pGP->a1 = 0.0f;
    pGP->a2 = 0.0f;
    pGP->a3 = 0.0f;
    pGP->coefp = 0.0f;
    pGP->coefn = 0.0f;

    switch (iOrder) 
    {
    case 0: 
        {
            const float k = (1.0f - pGP->ema)*(1.0f - pGP->ema)/(1.0f + (2.0f * pGP->alpha * pGP->ema) - pGP->ema2);
            pGP->a0 = k;
            pGP->a1 = k * (pGP->alpha - 1.0f) * pGP->ema;
            pGP->a2 = k * (pGP->alpha + 1.0f) * pGP->ema;
            pGP->a3 = -k * pGP->ema2;
        } 
        break;
    case 1: 
        {
            pGP->a0 = (1.0f - pGP->ema) * (1.0f - pGP->ema);
            pGP->a1 = 0.0f;
            pGP->a2 = -pGP->a0;
            pGP->a3 = 0.0f;
        } 
        break;
    case 2: 
        {
            const float ea = exp(-pGP->alpha);
            const float k = -(pGP->ema2 - 1.0f)/(2.0f * pGP->alpha * pGP->ema);
            float kn = -2.0f * (-1.0f + (3.0f * ea) - (3.0f * ea * ea) + (ea * ea * ea));
            kn /= (((3.0f * ea) + 1.0f + (3.0f * ea * ea) + (ea * ea * ea)));
            pGP->a0 = kn;
            pGP->a1 = -kn * (1.0f + (k * pGP->alpha)) * pGP->ema;
            pGP->a2 = kn * (1.0f - (k * pGP->alpha)) * pGP->ema;
            pGP->a3 = -kn * pGP->ema2;
        } 
        break;
    default:
        // note: iOrder is range-checked and clamped to 0-2 upstream
        return;
    }
    pGP->coefp = (pGP->a0 + pGP->a1)/(1.0f + pGP->b1 + pGP->b2);
    pGP->coefn = (pGP->a2 + pGP->a3)/(1.0f + pGP->b1 + pGP->b2);
}


int 
RecursiveGaussian::genBinaryImage()
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
    kernelPath.append("RecursiveGaussian_Kernels.cl");
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
RecursiveGaussian::setupCL()
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

    size_t deviceListSize;


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
    {
        return SDK_FAILURE;
    }

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


    /**
    * Create and initialize memory objects
    */

    /* Create memory object for input Image */
    inputImageBuffer = clCreateBuffer(
        context,
        CL_MEM_READ_ONLY,
        width * height * pixelSize,
        0,
        &status);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clCreateBuffer failed. (inputImageBuffer)"))
    {
        return SDK_FAILURE;
    }

    /* Create memory objects for output Image */
    outputImageBuffer = clCreateBuffer(context,
        CL_MEM_WRITE_ONLY | CL_MEM_USE_HOST_PTR,
        width * height * pixelSize,
        outputImageData,
        &status);
    if(!sampleCommon->checkVal(status,
        CL_SUCCESS,
        "clCreateBuffer failed. (outputImageBuffer)"))
    {
        return SDK_FAILURE;
    }

    /* create memory object for temp buffer */
    tempImageBuffer = clCreateBuffer(context,
        CL_MEM_READ_WRITE,
        width * height * pixelSize,
        0,
        &status);

    if(!sampleCommon->checkVal(status,
        CL_SUCCESS,
        "clCreateBuffer failed. (tempImageBuffer)"))
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

	kernelPath.append("RecursiveGaussian_Kernels.cl");
	if(!kernelFile.open(kernelPath.c_str()))
	{
		std::cout << "Failed to load kernel file : " << kernelPath << std::endl;
		return SDK_FAILURE;
	}

	const char * source = platformIsPacketizedOpenCL ?
		"RecursiveGaussian_Kernels.bc" :
		kernelFile.source().c_str();

        size_t sourceSize[] = {strlen(source)};
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

    /* kernel object for transpose kernel */
    kernelTranspose = clCreateKernel(program,
        "transpose_kernel",
        &status);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clCreateKernel failed.(transpose_kernel)"))
    {
        return SDK_FAILURE;
    }

    /* kernel object for recursive gaussian kernel */
    kernelRecursiveGaussian = clCreateKernel(program,
        "RecursiveGaussian_kernel",
        &status);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clCreateKernel failed.(RecursiveGaussian_kernel)"))
    {
        return SDK_FAILURE;
    }

    /* Check group size against group size returned by RecursiveGaussian kernel */
    status = clGetKernelWorkGroupInfo(kernelRecursiveGaussian,
        devices[deviceId],
        CL_KERNEL_WORK_GROUP_SIZE,
        sizeof(size_t),
        &rKernelWorkGroupSize,
        0);
    if(!sampleCommon->checkVal(status,
        CL_SUCCESS,
        "clGetKernelWorkGroupInfo failed."))
    {
        return SDK_FAILURE;
    }

    // Calculte block size according to required work-group size by kernel
    if((blockSizeX * blockSizeY) > rKernelWorkGroupSize)
    {
        blockSizeX = rKernelWorkGroupSize;
        blockSizeY = 1;
    }


    /* Check group size against group size returned by Tranpose kernel */
    status = clGetKernelWorkGroupInfo(kernelTranspose,
        devices[deviceId],
        CL_KERNEL_WORK_GROUP_SIZE,
        sizeof(size_t),
        &tKernelWorkGroupSize,
        0);
    if(!sampleCommon->checkVal(status,
        CL_SUCCESS,
        "clGetKernelWorkGroupInfo failed."))
    {
        return SDK_FAILURE;
    }

    // Calculte 2D block size according to required work-group size by transpose kernel
    blockSize = (size_t)sqrt((float)tKernelWorkGroupSize);
    //blockSize should a multiple of power of 2
    blockSize = (size_t)pow((float)2, (int)(log((float)blockSize) / log((float)2)));

    return SDK_SUCCESS;
}

int 
RecursiveGaussian::runCLKernels()
{

    cl_int status;
    cl_event events[2];

    /* initialize Gaussian parameters */ 
    float fSigma = 10.0f;               // filter sigma (blur factor)
    int iOrder = 0;                     // filter order

    /* compute gaussian parameters */
    computeGaussParms(fSigma, iOrder, &oclGP);

    /* Write inputImageData to inputImageBuffer on device */
    status = clEnqueueWriteBuffer(commandQueue,
        inputImageBuffer,
        1,
        0,
        width * height * pixelSize,
        inputImageData,
        0,
        0,
        0);

    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS, 
        "clSetKernelArg failed. (inputImageBuffer)"))
    {
        return SDK_FAILURE;
    }

    /*** Set appropriate arguments to the kernel (Recursive Gaussian) ***/

    /* input : input buffer image */
    status = clSetKernelArg(
        kernelRecursiveGaussian,
        0,
        sizeof(cl_mem),
        &inputImageBuffer);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS, 
        "clSetKernelArg failed. (inputImageBuffer)"))
    {
        return SDK_FAILURE;
    }

    /* output : temp Buffer */
    status = clSetKernelArg(
        kernelRecursiveGaussian,
        1,
        sizeof(cl_mem),
        &tempImageBuffer);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS, 
        "clSetKernelArg failed. (outputImageBuffer)"))
    {
        return SDK_FAILURE;
    }

    /* image width */ 
    status = clSetKernelArg(kernelRecursiveGaussian,
        2,
        sizeof(cl_int),
        &width);
    if(!sampleCommon->checkVal(status, CL_SUCCESS,
        "clSetKernelArg Failed. (width)"))
    {
        return SDK_FAILURE;
    }

    /* image height */ 
    status = clSetKernelArg(kernelRecursiveGaussian,
        3,
        sizeof(cl_int),
        &height);
    if(!sampleCommon->checkVal(status, CL_SUCCESS,
        "clSetKernelArg Failed. (height)"))
    {
        return SDK_FAILURE;
    }

    /* gaussian parameter : a0 */ 
    status = clSetKernelArg(kernelRecursiveGaussian,
        4,
        sizeof(cl_float),
        &oclGP.a0);
    if(!sampleCommon->checkVal(status, CL_SUCCESS,
        "clSetKernelArg Failed. (oclGP.a0)"))
    {
        return SDK_FAILURE;
    }

    /* gaussian parameter : a1 */
    status = clSetKernelArg(kernelRecursiveGaussian,
        5,
        sizeof(cl_float),
        &oclGP.a1);
    if(!sampleCommon->checkVal(status, CL_SUCCESS,
        "clSetKernelArg Failed. (oclGP.a1)"))
    {
        return SDK_FAILURE;
    }


    /* gaussian parameter : a2 */
    status = clSetKernelArg(kernelRecursiveGaussian,
        6,
        sizeof(cl_float),
        &oclGP.a2);
    if(!sampleCommon->checkVal(status, CL_SUCCESS,
        "clSetKernelArg Failed. (oclGP.a2)"))
    {
        return SDK_FAILURE;
    }

    /* gaussian parameter : a3 */
    status = clSetKernelArg(kernelRecursiveGaussian,
        7,
        sizeof(cl_float),
        &oclGP.a3);
    if(!sampleCommon->checkVal(status, CL_SUCCESS,
        "clSetKernelArg Failed. (oclGP.a3)"))
    {
        return SDK_FAILURE;
    }

    /* gaussian parameter : b1 */
    status = clSetKernelArg(kernelRecursiveGaussian,
        8,
        sizeof(cl_float),
        &oclGP.b1);
    if(!sampleCommon->checkVal(status, CL_SUCCESS,
        "clSetKernelArg Failed. (oclGP.b1)"))
    {
        return SDK_FAILURE;
    }

    /* gaussian parameter : b2 */
    status = clSetKernelArg(kernelRecursiveGaussian,
        9,
        sizeof(cl_float),
        &oclGP.b2);
    if(!sampleCommon->checkVal(status, CL_SUCCESS,
        "clSetKernelArg Failed. (oclGP.b2)"))
    {
        return SDK_FAILURE;
    }

    /* gaussian parameter : coefp */
    status = clSetKernelArg(kernelRecursiveGaussian,
        10,
        sizeof(cl_float),
        &oclGP.coefp);
    if(!sampleCommon->checkVal(status, CL_SUCCESS,
        "clSetKernelArg Failed. (oclGP.coefp)"))
    {
        return SDK_FAILURE;
    }

    /* gaussian parameter : coefn */
    status = clSetKernelArg(kernelRecursiveGaussian,
        11,
        sizeof(cl_float),
        &oclGP.coefn);
    if(!sampleCommon->checkVal(status, CL_SUCCESS,
        "clSetKernelArg Failed. (oclGP.coefn)"))
    {
        return SDK_FAILURE;
    }



    /* set global index and group size */
    size_t globalThreads[] = {width, 1};
    size_t localThreads[] = {blockSizeX, blockSizeY};

    if(localThreads[0] > maxWorkItemSizes[0] ||
       localThreads[0] > maxWorkGroupSize)
    {
        std::cout << "Unsupported: Device does not"
            "support requested number of work items.";
        return SDK_FAILURE;
    }

    /* 
    * Enqueue a kernel run call.
    */
    status = clEnqueueNDRangeKernel(
        commandQueue,
        kernelRecursiveGaussian,
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
    {
        return SDK_FAILURE;
    }

    /* Wait for kernel to finish */
    status = clWaitForEvents(1, &events[0]);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clWaitForEvents failed."))
        return SDK_FAILURE;


    /*** Set appropriate arguments to the kernel (Transpose) ***/

    /* output : input buffer image  */
    status = clSetKernelArg(
        kernelTranspose,
        0,
        sizeof(cl_mem),
        &inputImageBuffer);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS, 
        "clSetKernelArg failed. (inputImageBuffer)"))
    {
        return SDK_FAILURE;
    }

    /* input : temp Buffer */
    status = clSetKernelArg(
        kernelTranspose,
        1,
        sizeof(cl_mem),
        &tempImageBuffer);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS, 
        "clSetKernelArg failed. (tempImageBuffer)"))
    {
        return SDK_FAILURE;
    }

    /* local memory for block transpose */ 
    status = clSetKernelArg(kernelTranspose,
        2,
        blockSize * blockSize * sizeof(cl_uchar4),
        NULL);
    if(!sampleCommon->checkVal(status, CL_SUCCESS,
        "clSetKernelArg Failed. (local)"))
    {
        return SDK_FAILURE;
    }

    /* image width */ 
    status = clSetKernelArg(kernelTranspose,
        3,
        sizeof(cl_int),
        &width);
    if(!sampleCommon->checkVal(status, CL_SUCCESS,
        "clSetKernelArg Failed. (width)"))
    {
        return SDK_FAILURE;
    }

    /* image height */ 
    status = clSetKernelArg(kernelTranspose,
        4,
        sizeof(cl_int),
        &height);
    if(!sampleCommon->checkVal(status, CL_SUCCESS,
        "clSetKernelArg Failed. (height)"))
    {
        return SDK_FAILURE;
    }


    /* block_size */
    status = clSetKernelArg(kernelTranspose,
        5,
        sizeof(cl_int),
        &blockSize);
    if(!sampleCommon->checkVal(status, CL_SUCCESS,
        "clSetKernelArg Failed. (blockSize)"))
    {
        return SDK_FAILURE;
    }

    /* group dimensions for transpose kernel */
    size_t localThreadsT[] = {blockSize, blockSize};
    size_t globalThreadsT[] = {width, height};

    if(localThreadsT[0] > maxWorkItemSizes[0] ||
       localThreadsT[1] > maxWorkItemSizes[1] ||
       localThreadsT[0] * localThreadsT[1] > maxWorkGroupSize)
    {
        std::cout << "Unsupported: Device does not support requested"
            "number of work items.";
        return SDK_FAILURE;
    }

    status = clGetKernelWorkGroupInfo(kernelTranspose,
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
        std::cout << "Unsupported: Insufficient"
            "local memory on device." << std::endl;
        return SDK_FAILURE;
    }

    /* Enqueue Transpose Kernel */
    status = clEnqueueNDRangeKernel(
        commandQueue,
        kernelTranspose,
        2,
        NULL,
        globalThreadsT,
        localThreadsT,
        0,
        NULL,
        &events[1]);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS, 
        "clEnqueueNDRangeKernel failed."))

        /* Wait for transpose Kernel to finish */
        status = clWaitForEvents(1, &events[0]);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clWaitForEvents failed."))
        return SDK_FAILURE;


    /* Set Arguments for Recursive Gaussian Kernel 
    Image is now transposed  
    new_width = height
    new_height = width */


    /* image width : swap with height */ 
    status = clSetKernelArg(kernelRecursiveGaussian,
        2,
        sizeof(cl_int),
        &height);

    if(!sampleCommon->checkVal(status, 
        CL_SUCCESS,
        "clSetKernelArg Failed. (height)"))
    {
        return SDK_FAILURE;
    }

    /* image height */ 
    status = clSetKernelArg(kernelRecursiveGaussian,
        3, 
        sizeof(cl_int),
        &width);

    if(!sampleCommon->checkVal(status, 
        CL_SUCCESS,
        "clSetKernelArg Failed. (width)"))
    {
        return SDK_FAILURE;
    }


    /* Set new global index */
    globalThreads[0] = height;
    globalThreads[1] = 1;

    status = clEnqueueNDRangeKernel(
        commandQueue,
        kernelRecursiveGaussian,
        2,
        NULL,
        globalThreads,
        localThreads,
        0,
        NULL,
        &events[1]);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS, 
        "clEnqueueNDRangeKernel failed."))
    {
        return SDK_FAILURE;
    }

    /* Wait for Recursive Gaussian Kernel to finish */
    status = clWaitForEvents(1, &events[1]);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clWaitForEvents failed."))
        return SDK_FAILURE;


    /* Set Arguments to Transpose Kernel */

    /* output : output buffer image  */
    status = clSetKernelArg(
        kernelTranspose,
        0,
        sizeof(cl_mem),
        &outputImageBuffer);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS, 
        "clSetKernelArg failed. (outputImageBuffer)"))
    {
        return SDK_FAILURE;
    }

    /* input : temp Buffer */
    status = clSetKernelArg(
        kernelTranspose,
        1,
        sizeof(cl_mem),
        &tempImageBuffer);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS, 
        "clSetKernelArg failed. (tempImageBuffer)"))
    {
        return SDK_FAILURE;
    }

    /* local memory for block transpose */ 
    status = clSetKernelArg(kernelTranspose,
        2,
        blockSize * blockSize * sizeof(cl_uchar4),
        NULL);
    if(!sampleCommon->checkVal(status, CL_SUCCESS,
        "clSetKernelArg Failed. (local)"))
    {
        return SDK_FAILURE;
    }

    /* image width : is height actually as the image is currently transposed*/ 
    status = clSetKernelArg(kernelTranspose,
        3,
        sizeof(cl_int),
        &height);
    if(!sampleCommon->checkVal(status, CL_SUCCESS,
        "clSetKernelArg Failed. (height)"))
    {
        return SDK_FAILURE;
    }

    /* image height */ 
    status = clSetKernelArg(kernelTranspose,
        4,
        sizeof(cl_int),
        &width);
    if(!sampleCommon->checkVal(status, CL_SUCCESS,
        "clSetKernelArg Failed. (width)"))
    {
        return SDK_FAILURE;
    }

    /* block_size */
    status = clSetKernelArg(kernelTranspose,
        5,
        sizeof(cl_int),
        &blockSize);
    if(!sampleCommon->checkVal(status, CL_SUCCESS,
        "clSetKernelArg Failed. (blockSize)"))
    {
        return SDK_FAILURE;
    }

    /* group dimensions for transpose kernel */
    globalThreadsT[0] = height;
    globalThreadsT[1] = width;

    /* Enqueue final Transpose Kernel */
    status = clEnqueueNDRangeKernel(
        commandQueue,
        kernelTranspose,
        2,
        NULL,
        globalThreadsT,
        localThreadsT,
        0,
        NULL,
        &events[1]);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS, 
        "clEnqueueNDRangeKernel failed."))

        /* Wait for transpose kernel to finish execution */
        status = clWaitForEvents(1, &events[1]);
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

    /* Enqueue read output buffer to outputImageData */
    status = clEnqueueReadBuffer(commandQueue,
        outputImageBuffer,
        1,
        0,
        width * height * sizeof(cl_uchar4),
        outputImageData, 
        0, 0, 0);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clEnqueueReadBuffer failed."))
        return SDK_FAILURE;

    /* wait for read command to finish */
    status = clFinish(commandQueue);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clFinish failed."))
        return SDK_FAILURE;

    return SDK_SUCCESS;
}



int 
RecursiveGaussian::initialize()
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

int 
RecursiveGaussian::setup()
{
    /* Allocate host memory and read input image */
    std::string filePath = sampleCommon->getPath() + std::string(INPUT_IMAGE);
    std::cout << "Searching for input Image at following location : " << 
        filePath << std::endl;
    if(readInputImage(filePath) != SDK_SUCCESS)
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

int 
RecursiveGaussian::run()
{
    /* create and initialize timers */
    int timer = sampleCommon->createTimer();
    sampleCommon->resetTimer(timer);
    sampleCommon->startTimer(timer);

    std::cout << "Executing kernel for " << 
        iterations << " iterations" << std::endl;
    std::cout << "-------------------------------------------" << std::endl;

    for(int i = 0; i < iterations; i++)
    {
        /* Set kernel arguments and run kernel */
        if(runCLKernels() != SDK_SUCCESS)
            return SDK_FAILURE;
    }

    sampleCommon->stopTimer(timer);    
    /* Compute kernel time */
    kernelTime = (double)(sampleCommon->readTimer(timer)) / iterations;

    /* write the output image to bitmap file */
    std::string filePath = std::string(OUTPUT_IMAGE);
    if(writeOutputImage(filePath) != SDK_SUCCESS)
        return SDK_FAILURE;
    
    return SDK_SUCCESS;
}

int 
RecursiveGaussian::cleanup()
{
    /* Releases OpenCL resources (Context, Memory etc.) */
    cl_int status;

    status = clReleaseKernel(kernelRecursiveGaussian);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clReleaseKernel(kernelRecursiveGaussian) failed."))
        return SDK_FAILURE;

    status = clReleaseKernel(kernelTranspose);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clReleaseKernel(kernelTranspose) failed."))
        return SDK_FAILURE;    

    status = clReleaseProgram(program);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clReleaseProgram failed."))
        return SDK_FAILURE;

    status = clReleaseMemObject(inputImageBuffer);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clReleaseMemObject(inputImageBuffer) failed."))
        return SDK_FAILURE;

    status = clReleaseMemObject(outputImageBuffer);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clReleaseMemObject(outputImageBuffer) failed."))
        return SDK_FAILURE;

    status = clReleaseMemObject(tempImageBuffer);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clReleaseMemObject(tempImageBuffer) failed."))
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
    if(inputImageData) 
        free(inputImageData);

    if(outputImageData)
        free(outputImageData);

    if(verificationInput) 
        free(verificationInput);

    if(verificationOutput) 
        free(verificationOutput);

    if(devices)
        free(devices);

    if(maxWorkItemSizes)
        free(maxWorkItemSizes);

    return SDK_SUCCESS;
}

void
RecursiveGaussian::recursiveGaussianCPU(cl_uchar4* input, cl_uchar4* output,
                                        const int width, const int height,
                                        const float a0, const float a1, 
                                        const float a2, const float a3, 
                                        const float b1, const float b2, 
                                        const float coefp, const float coefn)
{

    /* outer loop over all columns within image */
    for (int X = 0; X < width; X++)
    {
        // start forward filter pass
        float xp[4] = {0.0f, 0.0f, 0.0f, 0.0f};  // previous input
        float yp[4] = {0.0f, 0.0f, 0.0f, 0.0f};  // previous output
        float yb[4] = {0.0f, 0.0f, 0.0f, 0.0f};  // previous output by 2

        float xc[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        float yc[4] = {0.0f, 0.0f, 0.0f, 0.0f}; 

        for (int Y = 0; Y < height; Y++) 
        {
            /* output position to write */
            int pos = Y * width + X;

            /* convert input element to float4 */
            xc[0] = input[pos].s[0];
            xc[1] = input[pos].s[1];
            xc[2] = input[pos].s[2];
            xc[3] = input[pos].s[3];

            yc[0] = (a0 * xc[0]) + (a1 * xp[0]) - (b1 * yp[0]) - (b2 * yb[0]);
            yc[1] = (a0 * xc[1]) + (a1 * xp[1]) - (b1 * yp[1]) - (b2 * yb[1]);
            yc[2] = (a0 * xc[2]) + (a1 * xp[2]) - (b1 * yp[2]) - (b2 * yb[2]);
            yc[3] = (a0 * xc[3]) + (a1 * xp[3]) - (b1 * yp[3]) - (b2 * yb[3]);

            /* convert float4 element to output */
            output[pos].s[0] = (cl_uchar)yc[0];
            output[pos].s[1] = (cl_uchar)yc[1];
            output[pos].s[2] = (cl_uchar)yc[2];
            output[pos].s[3] = (cl_uchar)yc[3];

            for (int i = 0; i < 4; i++)
            {
                xp[i] = xc[i]; 
                yb[i] = yp[i]; 
                yp[i] = yc[i]; 
            }
        }

        // start reverse filter pass: ensures response is symmetrical
        float xn[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        float xa[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        float yn[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        float ya[4] = {0.0f, 0.0f, 0.0f, 0.0f};


        float fTemp[4] = {0.0f, 0.0f, 0.0f, 0.0f};

        for (int Y = height - 1; Y > -1; Y--) 
        {
            int pos = Y * width + X;

            /* convert uchar4 to float4 */
            xc[0] = input[pos].s[0];
            xc[1] = input[pos].s[1];
            xc[2] = input[pos].s[2];
            xc[3] = input[pos].s[3];

            yc[0] = (a2 * xn[0]) + (a3 * xa[0]) - (b1 * yn[0]) - (b2 * ya[0]);
            yc[1] = (a2 * xn[1]) + (a3 * xa[1]) - (b1 * yn[1]) - (b2 * ya[1]);
            yc[2] = (a2 * xn[2]) + (a3 * xa[2]) - (b1 * yn[2]) - (b2 * ya[2]);
            yc[3] = (a2 * xn[3]) + (a3 * xa[3]) - (b1 * yn[3]) - (b2 * ya[3]);

            for (int i = 0; i< 4; i++)
            {
                xa[i] = xn[i]; 
                xn[i] = xc[i]; 
                ya[i] = yn[i]; 
                yn[i] = yc[i];
            }

            /* convert uhcar4 to float4 */
            fTemp[0] = output[pos].s[0];
            fTemp[1] = output[pos].s[1];
            fTemp[2] = output[pos].s[2];
            fTemp[3] = output[pos].s[3];

            fTemp[0] += yc[0];
            fTemp[1] += yc[1];
            fTemp[2] += yc[2];
            fTemp[3] += yc[3];

            /* convert float4 to uchar4 */
            output[pos].s[0] = (cl_uchar)fTemp[0];
            output[pos].s[1] = (cl_uchar)fTemp[1];
            output[pos].s[2] = (cl_uchar)fTemp[2];
            output[pos].s[3] = (cl_uchar)fTemp[3];
        }
    }

}

void 
RecursiveGaussian::transposeCPU(cl_uchar4* input, 
                                cl_uchar4* output,
                                const int width, 
                                const int height)
{
    /* transpose matrix */
    for(int Y = 0; Y < height; Y++) 
    {
        for(int X = 0; X < width; X++) 
        {
            output[Y + X*height] = input[X + Y*width];
        }
    }  
}

void 
RecursiveGaussian::recursiveGaussianCPUReference()
{

    /* Create a temp uchar4 array */
    cl_uchar4* temp = (cl_uchar4*)malloc(width * height * sizeof(cl_uchar4));
    if(temp == NULL)
    {
        sampleCommon->error("Failed to allocate host memory! (temp)");
        return;
    }

    /* Call recursive Gaussian CPU */
    recursiveGaussianCPU(verificationInput, temp, width, height, 
        oclGP.a0, oclGP.a1, oclGP.a2, oclGP.a3,
        oclGP.b1, oclGP.b2, oclGP.coefp, oclGP.coefn);

    /* Transpose the temp buffer */
    transposeCPU(temp, verificationOutput, width, height);

    /* again Call recursive Gaussian CPU */
    recursiveGaussianCPU(verificationOutput, temp, height, width, 
        oclGP.a0, oclGP.a1, oclGP.a2, oclGP.a3,
        oclGP.b1, oclGP.b2, oclGP.coefp, oclGP.coefn);

    /* Do a final Transpose */
    transposeCPU(temp, verificationOutput, height, width);  

    if(temp)
        free(temp);

}

/* convert uchar4 data to uint */
unsigned int rgbaUchar4ToUint(const cl_uchar4 rgba)
{
    unsigned int uiPackedRGBA = 0U;
    uiPackedRGBA |= 0x000000FF & (unsigned int)rgba.s[0];
    uiPackedRGBA |= 0x0000FF00 & (((unsigned int)rgba.s[1]) << 8);
    uiPackedRGBA |= 0x00FF0000 & (((unsigned int)rgba.s[2]) << 16);
    uiPackedRGBA |= 0xFF000000 & (((unsigned int)rgba.s[3]) << 24);
    return uiPackedRGBA;
}


int 
RecursiveGaussian::verifyResults()
{

    if(verify)
    {     
        recursiveGaussianCPUReference();
        
        float *outputDevice = new float[width * height * 4];
        if(outputDevice == NULL)
        {
            sampleCommon->error("Failed to allocate host"
                "memory! (outputDevice)");
            return SDK_FAILURE;
        }
        float *outputReference = new float[width * height * 4];
        if(outputReference == NULL)
        {
            sampleCommon->error("Failed to allocate host"
                "memory! (outputReference)");
            return SDK_FAILURE;
        }

        int m = 0;

        /* copy uchar4 data to float array */
        for(int i=0; i < (int)(width * height); i++)
        {
            outputDevice[4 * i + 0] = outputImageData[i].s[0];
            outputDevice[4 * i + 1] = outputImageData[i].s[1];
            outputDevice[4 * i + 2] = outputImageData[i].s[2];
            outputDevice[4 * i + 3] = outputImageData[i].s[3];

            outputReference[4 * i + 0] = verificationOutput[i].s[0];
            outputReference[4 * i + 1] = verificationOutput[i].s[1];
            outputReference[4 * i + 2] = verificationOutput[i].s[2];
            outputReference[4 * i + 3] = verificationOutput[i].s[3];
        }


        /* compare the results and see if they match */
        if(sampleCommon->compare(outputReference, 
                                 outputDevice, 
                                 width * height, 
                                 (float)0.0001))
        {
            std::cout <<"Passed!\n";
            delete[] outputDevice;
            delete[] outputReference;
            return SDK_SUCCESS;
        }
        else
        {
            std::cout << "Failed\n";
            delete[] outputDevice;
            delete[] outputReference;
            return SDK_FAILURE;
        }
    }

    return SDK_SUCCESS;
}

void 
RecursiveGaussian::printStats()
{
    std::string strArray[4] = 
    {   
        "Width", 
        "Height", 
        "Time(sec)", 
        "kernelTime(sec)"
    };
    std::string stats[4];

    totalTime = setupTime + kernelTime;

    stats[0]  = sampleCommon->toString(width, std::dec);
    stats[1]  = sampleCommon->toString(height, std::dec);
    stats[2]  = sampleCommon->toString(totalTime, std::dec);
    stats[3]  = sampleCommon->toString(kernelTime, std::dec);

    this->SDKSample::printStats(strArray, stats, 4);
}


int 
main(int argc, char * argv[])
{
    RecursiveGaussian clRecursiveGaussian("OpenCL RecursiveGaussian");

    if(clRecursiveGaussian.initialize() != SDK_SUCCESS)
        return SDK_FAILURE;
    if(!clRecursiveGaussian.parseCommandLine(argc, argv))
        return SDK_FAILURE;

    if(clRecursiveGaussian.isDumpBinaryEnabled())
    {
        return clRecursiveGaussian.genBinaryImage();
    }
    else
    {
        if(clRecursiveGaussian.setup() != SDK_SUCCESS)
            return SDK_FAILURE;
        if(clRecursiveGaussian.run() != SDK_SUCCESS)
            return SDK_FAILURE;
        if(clRecursiveGaussian.verifyResults() != SDK_SUCCESS)
            return SDK_FAILURE;
        if(clRecursiveGaussian.cleanup() != SDK_SUCCESS)
            return SDK_FAILURE;
        clRecursiveGaussian.printStats();
    }

    return SDK_SUCCESS;
}



