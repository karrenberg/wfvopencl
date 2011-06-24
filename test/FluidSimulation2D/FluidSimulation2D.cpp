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


#include "FluidSimulation2D.hpp"
#include "ColorScale.h"
#include <GL/glut.h>
#include <cmath>
#if !defined __APPLE__
#	include <malloc.h>
#endif

// Directions
double e[9][2] = {{0,0}, {1,0}, {0,1}, {-1,0}, {0,-1}, {1,1}, {-1,1}, {-1,-1}, {1,-1}};

// Weights
cl_double w[9] = {4.0/9.0, 1.0/9.0, 1.0/9.0, 1.0/9.0, 1.0/9.0, 1.0/36.0, 1.0/36.0, 1.0/36.0, 1.0/36.0};

// Omega
const double omega = 1.2f;

// Verify flag
bool verifyFlag = 0;

FluidSimulation2D* me;           /**< Pointing to FluidSimulation2D class */
cl_bool display;
GLuint texnum;
ColorScale bluewhite(6);
bool drawBoundary = false;
bool addVelocity = false;
int oldx, oldy;

int drawVelocities = 1;
int drawParticles = 1;

int frames = 0;
int t0 = 0, te;

// Calculates equivalent distribution 
double computefEq(cl_double weight, double dir[2], double rho, cl_double2 velocity)
{
    double u2 = velocity.s[0] * velocity.s[0] + velocity.s[1] * velocity.s[1];
    double eu = dir[0] * velocity.s[0] + dir[1] * velocity.s[1];

    return rho * weight * (1.0 + 3.0*eu + 4.5*eu*eu - 1.5*u2);
}

// Returns the velocity at (x, y) location relative to lattice
cl_double2 FluidSimulation2D::getVelocity(int x, int y)
{
    int pos = x + dims[0] * y;
    return u[pos];
}

// Returns the state of (x, y) position relative to Lattice
cl_bool FluidSimulation2D::isBoundary(int x, int y)
{
    return h_type[x + dims[0] * y];
}

// Returns the state of (x, y) position relative to Lattice
bool FluidSimulation2D::isFluid(int x, int y)
{
    if(h_type[x + dims[0] * y] == 0)
        return true;
    else
        return false;
}

void FluidSimulation2D::setUOutput(int x, int y, double v[2])
{
    double rho, uu[2];

    int pos = x + dims[0] * y;

    // Calculate density from input distribution
    rho = h_of0[pos] + h_of1234[pos * 4 + 0] +  h_of1234[pos * 4 + 1] + h_of1234[pos * 4 + 2] + h_of1234[pos * 4 + 3] + 
        h_of5678[pos * 4 + 0] + h_of5678[pos * 4 + 1] + h_of5678[pos * 4 + 2] + h_of5678[pos * 4 + 3];


    uu[0] = u[pos].s[0];
    uu[1] = u[pos].s[1];

    // Increase the speed by input speed
    uu[0] += v[0];
    uu[1] += v[1];

    cl_double2 newVel;
    newVel.s[0] = uu[0];
    newVel.s[1] = uu[1];

    // Calculate new distribution based on input speed
    h_if0[pos] = computefEq(w[0], e[0], rho, newVel);
    h_if1234[pos * 4 + 0] = computefEq(w[1], e[1], rho, newVel);
    h_if1234[pos * 4 + 1] = computefEq(w[2], e[2], rho, newVel);
    h_if1234[pos * 4 + 2] = computefEq(w[3], e[3], rho, newVel);
    h_if1234[pos * 4 + 3] = computefEq(w[4], e[4], rho, newVel);

    h_if5678[pos * 4 + 0] = computefEq(w[5], e[5], rho, newVel);
    h_if5678[pos * 4 + 1] = computefEq(w[6], e[6], rho, newVel);
    h_if5678[pos * 4 + 2] = computefEq(w[7], e[7], rho, newVel);
    h_if5678[pos * 4 + 3] = computefEq(w[8], e[8], rho, newVel);
}


void FluidSimulation2D::setSite(int x, int y, bool cellType, double u[2])
{
    // Set type
    h_type[x + dims[0] * y] = cellType;

    // Set velocity and calculate new distributions
    setUOutput(x, y, u);

}

void FluidSimulation2D::reset()
{
    // Initial velocity is 0
    cl_double2 u0;
    u0.s[0] = u0.s[1] = 0.0f;

    for (int y = 0; y < dims[1]; y++)
    {
        for (int x = 0; x < dims[0]; x++)
        {
            int pos = x + y * dims[0];

            double den = 10.0f;

            // Initialize the velocity buffer
            u[pos] = u0;

            //rho[pos] = 10.0f;
            h_if0[pos] = computefEq(w[0], e[0], den, u0);
            h_if1234[pos * 4 + 0] = computefEq(w[1], e[1], den, u0);
            h_if1234[pos * 4 + 1] = computefEq(w[2], e[2], den, u0);
            h_if1234[pos * 4 + 2] = computefEq(w[3], e[3], den, u0);
            h_if1234[pos * 4 + 3] = computefEq(w[4], e[4], den, u0);

            h_if5678[pos * 4 + 0] = computefEq(w[5], e[5], den, u0);
            h_if5678[pos * 4 + 1] = computefEq(w[6], e[6], den, u0);
            h_if5678[pos * 4 + 2] = computefEq(w[7], e[7], den, u0);
            h_if5678[pos * 4 + 3] = computefEq(w[8], e[8], den, u0);

            // Initialize boundary cells
            if (x == 0 || x == (dims[0] - 1)  || y == 0 || y == (dims[1] - 1))
                h_type[pos] = 1;

            // Initialize fluid cells
            else
                h_type[pos] = 0;
        }
    }
}


int
FluidSimulation2D::setupFluidSimulation2D()
{
    
    size_t temp = dims[0] * dims[1];

    // Allocate memory for host buffers
    h_if0 = (cl_double*)malloc(sizeof(cl_double) * temp);
    h_if1234 = (cl_double*)malloc(sizeof(cl_double4) * temp);
    h_if5678 = (cl_double*)malloc(sizeof(cl_double4) * temp);

    h_of0 = (cl_double*)malloc(sizeof(cl_double) * temp);
    h_of1234 = (cl_double*)malloc(sizeof(cl_double4) * temp);
    h_of5678 = (cl_double*)malloc(sizeof(cl_double4) * temp);

    if(verify)
    {
        v_of0 = (cl_double*)malloc(sizeof(cl_double) * temp);
        v_of1234 = (cl_double*)malloc(sizeof(cl_double4) * temp);
        v_of5678 = (cl_double*)malloc(sizeof(cl_double4) * temp);

        v_ef0 = (cl_double*)malloc(sizeof(cl_double) * temp);
        v_ef1234 = (cl_double*)malloc(sizeof(cl_double4) * temp);
        v_ef5678 = (cl_double*)malloc(sizeof(cl_double4) * temp);
    }


    h_type = (cl_bool*)malloc(sizeof(cl_bool) * temp);
    rho = (cl_double*)malloc(sizeof(cl_double) * temp);
    u = (cl_double2*)malloc(sizeof(cl_double2) * temp);

    reset();

    return SDK_SUCCESS;
}

int 
FluidSimulation2D::genBinaryImage()
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
    kernelPath.append("FluidSimulation2D_Kernels.cl");
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
FluidSimulation2D::setupCL()
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


    /* Check whether the device supports double-precision */
    char deviceExtensions[8192];

    /* Get device extensions */
    status = clGetDeviceInfo(devices[deviceId], 
        CL_DEVICE_EXTENSIONS, 
        sizeof(deviceExtensions), 
        deviceExtensions, 
        0);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS, 
        "clGetDeviceInfo failed.(extensions)"))
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
            reqdExtSupport = false;
            sampleCommon->expectedError("Device does not support cl_amd_fp64 extension!");
            return SDK_EXPECTED_FAILURE;
        }
    }

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

    size_t temp = dims[0] * dims[1];
    d_if0 = clCreateBuffer(context, 
        CL_MEM_READ_WRITE, 
        sizeof(cl_double) * temp, 
        0, 
        &status);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS, 
        "clCreateBuffer failed. (d_if0)"))
        return SDK_FAILURE;

    status = clEnqueueWriteBuffer(commandQueue,
        d_if0,
        1,
        0,
        sizeof(cl_double) * temp,
        h_if0,
        0, 0, 0);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS, 
        "clEnqueueWriteBuffer failed. (d_if0)"))
        return SDK_FAILURE;

    d_if1234 = clCreateBuffer(context, 
        CL_MEM_READ_WRITE, 
        sizeof(cl_double4) * temp, 
        0, 
        &status);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS, 
        "clCreateBuffer failed. (d_if1234)"))
        return SDK_FAILURE;

    status = clEnqueueWriteBuffer(commandQueue,
        d_if1234,
        1,
        0,
        sizeof(cl_double4) * temp,
        h_if1234,
        0, 0, 0);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS, 
        "clEnqueueWriteBuffer failed. (d_if1234)"))
        return SDK_FAILURE;


    d_if5678 = clCreateBuffer(context, 
        CL_MEM_READ_WRITE, 
        sizeof(cl_double4) * temp, 
        0, 
        &status);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS, 
        "clCreateBuffer failed. (d_if5678)"))
        return SDK_FAILURE;

    status = clEnqueueWriteBuffer(commandQueue,
        d_if5678,
        1,
        0,
        sizeof(cl_double4) * temp,
        h_if5678,
        0, 0, 0);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS, 
        "clEnqueueWriteBuffer failed. (d_if5678)"))
        return SDK_FAILURE;


    d_of0 = clCreateBuffer(context, 
        CL_MEM_READ_WRITE, 
        sizeof(cl_double) * temp, 
        0, 
        &status);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS, 
        "clCreateBuffer failed. (d_of0)"))
        return SDK_FAILURE;

    d_of1234 = clCreateBuffer(context, 
        CL_MEM_READ_WRITE, 
        sizeof(cl_double4) * temp, 
        0, 
        &status);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS, 
        "clCreateBuffer failed. (d_of1234)"))
        return SDK_FAILURE;

    d_of5678 = clCreateBuffer(context, 
        CL_MEM_READ_WRITE, 
        sizeof(cl_double4) * temp, 
        0, 
        &status);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS, 
        "clCreateBuffer failed. (d_of5678)"))
        return SDK_FAILURE;

    status = clEnqueueCopyBuffer(commandQueue,
        d_if0,
        d_of0,
        0, 0, sizeof(cl_double) * temp,
        0, 0, 0);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS, 
        "clEnqueueCopyBuffer failed. (d_if0->d_of0)"))
        return SDK_FAILURE;

    status = clEnqueueCopyBuffer(commandQueue,
        d_if1234,
        d_of1234,
        0, 0, sizeof(cl_double4) * temp,
        0, 0, 0);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS, 
        "clEnqueueCopyBuffer failed. (d_if1234->d_of1234)"))
        return SDK_FAILURE;

    status = clEnqueueCopyBuffer(commandQueue,
        d_if5678,
        d_of5678,
        0, 0, sizeof(cl_double4) * temp,
        0, 0, 0);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS, 
        "clEnqueueCopyBuffer failed. (d_if5678->d_of5678)"))
        return SDK_FAILURE;

    status = clFinish(commandQueue);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS, 
        "clFinish failed."))
        return SDK_FAILURE;


    //Constant arrays
    type = clCreateBuffer(context, 
        CL_MEM_READ_ONLY, 
        sizeof(cl_bool) * temp,
        0,
        &status);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS, 
        "clCreateBuffer failed. (type)"))
        return SDK_FAILURE;

    weight = clCreateBuffer(context,
        CL_MEM_READ_ONLY,
        sizeof(cl_double) * 9,
        0,
        &status);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS, 
        "clCreateBuffer failed. (weight)"))
        return SDK_FAILURE;

    status = clEnqueueWriteBuffer(commandQueue,
        weight, 
        1, 0, sizeof(cl_double) * 9,
        w,
        0, 0, 0);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS, 
        "clEnqueueWriteBuffer failed. (weight)"))
        return SDK_FAILURE;

    velocity = clCreateBuffer(context,
        CL_MEM_WRITE_ONLY,
        sizeof(cl_double2) * temp,
        0, &status);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS, 
        "clCreateBuffer failed. (velocity)"))
        return SDK_FAILURE;

    status = clEnqueueWriteBuffer(commandQueue,
        velocity, 
        1, 0, sizeof(cl_double2) * temp,
        u,
        0, 0, 0);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS, 
        "clEnqueueWriteBuffer failed. (velocity)"))
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

	kernelPath.append("FluidSimulation2D_Kernels.cl");
	if(!kernelFile.open(kernelPath.c_str()))
	{
		std::cout << "Failed to load kernel file : " << kernelPath << std::endl;
		return SDK_FAILURE;
	}

	const char * source = platformIsPacketizedOpenCL ?
		"FluidSimulation2D_Kernels.bc" :
		kernelFile.source().c_str();

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

    //Add buildOptions if any
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
        "lbm",
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
FluidSimulation2D::setupCLKernels()
{
    cl_int status;

    /* Set appropriate arguments to the kernel */

    // initialize direction buffer
    for(int i = 0; i < 8; i++)
    {
        dirX.s[i] = e[i + 1][0];
        dirY.s[i] = e[i + 1][1];
    }

    // Set kernel arguments
    status = clSetKernelArg(kernel, 0, sizeof(cl_mem), &d_if0);
    status |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &d_of0);
    status |= clSetKernelArg(kernel, 2, sizeof(cl_mem), &d_if1234);
    status |= clSetKernelArg(kernel, 3, sizeof(cl_mem), &d_of1234);
    status |= clSetKernelArg(kernel, 4, sizeof(cl_mem), &d_if5678);
    status |= clSetKernelArg(kernel, 5, sizeof(cl_mem), &d_of5678);
    status |= clSetKernelArg(kernel, 6, sizeof(cl_mem), &type);
    status |= clSetKernelArg(kernel, 7, sizeof(cl_double8), &dirX);
    status |= clSetKernelArg(kernel, 8, sizeof(cl_double8), &dirY);
    status |= clSetKernelArg(kernel, 9, sizeof(cl_mem), &weight);
    status |= clSetKernelArg(kernel, 10, sizeof(cl_double), &omega);

    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS, 
        "clSetKernelArgs failed."))
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
FluidSimulation2D::runCLKernels()
{
    cl_int status;
    static int i = 1;
    size_t temp = dims[0] * dims[1];

    //Enqueue write data to device

    // Write the cell type data each frame
    status = clEnqueueWriteBuffer(commandQueue,
        type,
        1,
        0,
        sizeof(cl_bool) * temp,
        h_type,
        0, 0, 0);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS, 
        "clEnqueueWriteBuffer failed. (h_type)"))
    {
        return SDK_FAILURE;
    }

    // If odd frame (starts from odd frame)
    // Then inputs : d_if0, d_if1234, d_if5678
    // Outputs : d_of0, f_of1234, d_of5678
    // Else they are swapped
    if(i % 2)
    {
        status = clEnqueueWriteBuffer(commandQueue,
            d_if0,
            1,
            0,
            sizeof(cl_double) * temp,
            h_if0,
            0, 0, 0);
        if(!sampleCommon->checkVal(
            status,
            CL_SUCCESS, 
            "clEnqueueWriteBuffer failed. (d_if0)"))
        {
            return SDK_FAILURE;
        }

        status = clEnqueueWriteBuffer(commandQueue,
            d_if1234,
            1,
            0,
            sizeof(cl_double4) * temp,
            h_if1234,
            0, 0, 0);
        if(!sampleCommon->checkVal(
            status,
            CL_SUCCESS, 
            "clEnqueueWriteBuffer failed. (d_if1234)"))
        {
            return SDK_FAILURE;
        }

        status = clEnqueueWriteBuffer(commandQueue,
            d_if5678,
            1,
            0,
            sizeof(cl_double4) * temp,
            h_if5678,
            0, 0, 0);
        if(!sampleCommon->checkVal(
            status,
            CL_SUCCESS, 
            "clEnqueueWriteBuffer failed. (d_if5678)"))
        {
            return SDK_FAILURE;
        }     

    }
    else
    {
        status = clEnqueueWriteBuffer(commandQueue,
            d_of0,
            1,
            0,
            sizeof(cl_double) * temp,
            h_if0,
            0, 0, 0);
        if(!sampleCommon->checkVal(
            status,
            CL_SUCCESS, 
            "clEnqueueWriteBuffer failed. (d_of0)"))
        {
            return SDK_FAILURE;
        }

        status = clEnqueueWriteBuffer(commandQueue,
            d_of1234,
            1,
            0,
            sizeof(cl_double4) * temp,
            h_if1234,
            0, 0, 0);
        if(!sampleCommon->checkVal(
            status,
            CL_SUCCESS, 
            "clEnqueueWriteBuffer failed. (d_of1234)"))
        {
            return SDK_FAILURE;
        }

        status = clEnqueueWriteBuffer(commandQueue,
            d_of5678,
            1,
            0,
            sizeof(cl_double4) * temp,
            h_if5678,
            0, 0, 0);
        if(!sampleCommon->checkVal(
            status,
            CL_SUCCESS, 
            "clEnqueueWriteBuffer failed. (d_of5678)"))
        {
            return SDK_FAILURE;
        }
    }

    // Set kernel arguments
    status = clSetKernelArg(kernel, 0, sizeof(cl_mem), &d_if0);
    status |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &d_of0);
    status |= clSetKernelArg(kernel, 2, sizeof(cl_mem), &d_if1234);
    status |= clSetKernelArg(kernel, 3, sizeof(cl_mem), &d_of1234);
    status |= clSetKernelArg(kernel, 4, sizeof(cl_mem), &d_if5678);
    status |= clSetKernelArg(kernel, 5, sizeof(cl_mem), &d_of5678);
    status |= clSetKernelArg(kernel, 6, sizeof(cl_mem), &type);
    status |= clSetKernelArg(kernel, 7, sizeof(cl_double8), &dirX);
    status |= clSetKernelArg(kernel, 8, sizeof(cl_double8), &dirY);
    status |= clSetKernelArg(kernel, 9, sizeof(cl_mem), &weight);
    status |= clSetKernelArg(kernel, 10, sizeof(cl_double), &omega);
    status |= clSetKernelArg(kernel, 11, sizeof(cl_mem), &velocity);

    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS, 
        "clSetKernelArgs failed.)"))
    {
        return SDK_FAILURE;
    }

    size_t localThreads[2] = {groupSize, 1};
    size_t globalThreads[2] = {dims[0], dims[1]};

    status = clEnqueueNDRangeKernel(commandQueue,
        kernel,
        2,
        0,
        globalThreads,
        localThreads,
        0, 0, 0);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS, 
        "clEnqueueNDRangeKernel failed.)"))
    {
        return SDK_FAILURE;
    }

    status = clFinish(commandQueue);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS, 
        "clFinish failed.)"))
    {
        return SDK_FAILURE;
    }

    status = clEnqueueReadBuffer(commandQueue,
        velocity,
        1,
        0,
        sizeof(cl_double2) * temp,
        u,
        0, 0, 0);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS, 
        "clEnqueueReadBuffer failed.(velocity)"))
    {
        return SDK_FAILURE;
    }

    //Read back the data into host buffer
    if(i % 2)
    {
        status = clEnqueueReadBuffer(commandQueue,
            d_of0,
            1,
            0,
            sizeof(cl_double) * temp,
            h_of0,
            0, 0, 0);
        if(!sampleCommon->checkVal(
            status,
            CL_SUCCESS, 
            "clEnqueueReadBuffer failed.(d_of0)"))
        {
            return SDK_FAILURE;
        }

        status = clEnqueueReadBuffer(commandQueue,
            d_of1234,
            1,
            0,
            sizeof(cl_double4) * temp,
            h_of1234,
            0, 0, 0);
        if(!sampleCommon->checkVal(
            status,
            CL_SUCCESS, 
            "clEnqueueReadBuffer failed.(d_of1234)"))
        {
            return SDK_FAILURE;
        }

        status = clEnqueueReadBuffer(commandQueue,
            d_of5678,
            1,
            0,
            sizeof(cl_double4) * temp,
            h_of5678,
            0, 0, 0);
        if(!sampleCommon->checkVal(
            status,
            CL_SUCCESS, 
            "clEnqueueReadBuffer failed.(d_of5678)"))
        {
            return SDK_FAILURE;
        }

    }
    else
    {
        status = clEnqueueReadBuffer(commandQueue,
            d_if0,
            1,
            0,
            sizeof(cl_double) * temp,
            h_of0,
            0, 0, 0);
        if(!sampleCommon->checkVal(
            status,
            CL_SUCCESS, 
            "clEnqueueReadBuffer failed.(d_if0)"))
        {
            return SDK_FAILURE;
        }

        status = clEnqueueReadBuffer(commandQueue,
            d_if1234,
            1,
            0,
            sizeof(cl_double4) * temp,
            h_of1234,
            0, 0, 0);
        if(!sampleCommon->checkVal(
            status,
            CL_SUCCESS, 
            "clEnqueueReadBuffer failed.(d_if1234)"))
        {
            return SDK_FAILURE;
        }

        status = clEnqueueReadBuffer(commandQueue,
            d_if5678,
            1,
            0,
            sizeof(cl_double4) * temp,
            h_of5678,
            0, 0, 0);
        if(!sampleCommon->checkVal(
            status,
            CL_SUCCESS, 
            "clEnqueueReadBuffer failed.(d_if5678)"))
        {
            return SDK_FAILURE;
        }
    }

    // Copy from host output to the next input
    memcpy(h_if0, h_of0, sizeof(cl_double) * temp);
    memcpy(h_if1234, h_of1234, sizeof(cl_double4) * temp);
    memcpy(h_if5678, h_of5678, sizeof(cl_double4) * temp);

    cl_mem temp0, temp1234, temp5678;

    //swap input and output buffers
    temp0 = d_of0;
    temp1234 = d_of1234;
    temp5678 = d_of5678;

    d_of0 = d_if0;
    d_of1234 = d_if1234;
    d_of5678 = d_if5678;

    d_if0 = temp0;
    d_if1234 = temp1234;
    d_if5678 = temp5678;

    i++;

    return SDK_SUCCESS;
}

void FluidSimulation2D::collide(int x, int y)
{
    int pos = x + y * dims[0];

    // Boundary
    if (h_type[pos] == 1)
    {
        // Swap the frequency distributions
        cl_double4 temp1234, temp5678;

        v_ef0[pos] = h_if0[pos];

        temp1234.s[0] = h_if1234[pos * 4 + 0];
        temp1234.s[1] = h_if1234[pos * 4 + 1];
        temp1234.s[2] = h_if1234[pos * 4 + 2];
        temp1234.s[3] = h_if1234[pos * 4 + 3];

        v_ef1234[pos * 4 + 0] = temp1234.s[2];
        v_ef1234[pos * 4 + 1] = temp1234.s[3];
        v_ef1234[pos * 4 + 2] = temp1234.s[0];
        v_ef1234[pos * 4 + 3] = temp1234.s[1];

        temp5678.s[0] = h_if5678[pos * 4 + 0];
        temp5678.s[1] = h_if5678[pos * 4 + 1];
        temp5678.s[2] = h_if5678[pos * 4 + 2];
        temp5678.s[3] = h_if5678[pos * 4 + 3];

        v_ef5678[pos * 4 + 0] = temp5678.s[2];
        v_ef5678[pos * 4 + 1] = temp5678.s[3];
        v_ef5678[pos * 4 + 2] = temp5678.s[0];
        v_ef5678[pos * 4 + 3] = temp5678.s[1];

        rho[pos] = 0;
        u[pos].s[0] = u[pos].s[1] = 0;
    }
    //Fluid
    else
    {
        cl_double2 vel;

        // Calculate density from input distribution
        cl_double den = h_if0[pos] + h_if1234[pos * 4 + 0] +  h_if1234[pos * 4 + 1] + h_if1234[pos * 4 + 2] + h_if1234[pos * 4 + 3] + 
            h_if5678[pos * 4 + 0] + h_if5678[pos * 4 + 1] + h_if5678[pos * 4 + 2] + h_if5678[pos * 4 + 3];

        // Calculate velocity vector in x-direction
        vel.s[0] = h_if0[pos] * e[0][0] + h_if1234[pos * 4 + 0] * e[1][0] + h_if1234[pos * 4 + 1] * e[2][0] 
        + h_if1234[pos * 4 + 2] * e[3][0] + h_if1234[pos * 4 + 3] * e[4][0] +  h_if5678[pos * 4 + 0] * e[5][0]
        + h_if5678[pos * 4 + 1] * e[6][0] + h_if5678[pos * 4 + 2] * e[7][0] +  h_if5678[pos * 4 + 3] * e[8][0];

        // Calculate velocity vector in y-direction
        vel.s[1] = h_if0[pos] * e[0][1] + h_if1234[pos * 4 + 0] * e[1][1] + h_if1234[pos * 4 + 1] * e[2][1] 
        + h_if1234[pos * 4 + 2] * e[3][1] + h_if1234[pos * 4 + 3] * e[4][1] +  h_if5678[pos * 4 + 0] * e[5][1]
        + h_if5678[pos * 4 + 1] * e[6][1] + h_if5678[pos * 4 + 2] * e[7][1] +  h_if5678[pos * 4 + 3] * e[8][1];

        vel.s[0] /= den;
        vel.s[0] /= den;

        // Calculate Equivalent distribution
        v_ef0[pos] = computefEq(w[0], e[0], den, vel);
        v_ef1234[pos * 4 + 0] = computefEq(w[1], e[1], den, vel);
        v_ef1234[pos * 4 + 1] = computefEq(w[2], e[2], den, vel);
        v_ef1234[pos * 4 + 2] = computefEq(w[3], e[3], den, vel);
        v_ef1234[pos * 4 + 3] = computefEq(w[4], e[4], den, vel);

        v_ef5678[pos * 4 + 0] = computefEq(w[5], e[5], den, vel);
        v_ef5678[pos * 4 + 1] = computefEq(w[6], e[6], den, vel);
        v_ef5678[pos * 4 + 2] = computefEq(w[7], e[7], den, vel);
        v_ef5678[pos * 4 + 3] = computefEq(w[8], e[8], den, vel);

        v_ef0[pos] = (1 - omega) * h_if0[pos] + omega * v_ef0[pos];
        v_ef1234[pos * 4 + 0] = (1 - omega) * h_if1234[pos * 4 + 0] + omega * v_ef1234[pos * 4 + 0];
        v_ef1234[pos * 4 + 1] = (1 - omega) * h_if1234[pos * 4 + 1] + omega * v_ef1234[pos * 4 + 1];
        v_ef1234[pos * 4 + 2] = (1 - omega) * h_if1234[pos * 4 + 2] + omega * v_ef1234[pos * 4 + 2];
        v_ef1234[pos * 4 + 3] = (1 - omega) * h_if1234[pos * 4 + 3] + omega * v_ef1234[pos * 4 + 3];
        v_ef5678[pos * 4 + 0] = (1 - omega) * h_if5678[pos * 4 + 0] + omega * v_ef5678[pos * 4 + 0];
        v_ef5678[pos * 4 + 1] = (1 - omega) * h_if5678[pos * 4 + 1] + omega * v_ef5678[pos * 4 + 1];
        v_ef5678[pos * 4 + 2] = (1 - omega) * h_if5678[pos * 4 + 2] + omega * v_ef5678[pos * 4 + 2];
        v_ef5678[pos * 4 + 3] = (1 - omega) * h_if5678[pos * 4 + 3] + omega * v_ef5678[pos * 4 + 3];
    }
}

void FluidSimulation2D::streamToNeighbors(int x, int y)
{
    if (x == 0 || x == dims[0]-1 || y == 0 || y == dims[1]-1)
        return;

    for (int k=0; k<9; k++)
    {
        int nx = x + (int)e[k][0];
        int ny = y + (int)e[k][1];

        int pos = nx + dims[0] * ny;
        switch(k)
        {
        case 0:
            v_of0[pos] = v_ef0[pos];
            break;
        case 1:
            v_of1234[pos * 4 + 0] = v_ef1234[pos * 4 + 0];
            break;
        case 2:
            v_of1234[pos * 4 + 1] = v_ef1234[pos * 4 + 1];
            break;
        case 3:
            v_of1234[pos * 4 + 2] = v_ef1234[pos * 4 + 2];
            break;
        case 4:
            v_of1234[pos * 4 + 3] = v_ef1234[pos * 4 + 3];
            break;
        case 5:
            v_of5678[pos * 4 + 0] = v_ef5678[pos * 4 + 0];
            break;
        case 6:
            v_of5678[pos * 4 + 1] = v_ef5678[pos * 4 + 1];
            break;
        case 7:
            v_of5678[pos * 4 + 2] = v_ef5678[pos * 4 + 2];
            break;
        case 8:
            v_of5678[pos * 4 + 3] = v_ef5678[pos * 4 + 3];
            break;

        }
    }
}


/*
* lbm simulation on cpu
*/
void 
FluidSimulation2D::CPUReference()
{
    // Copy from host output to the next input
    memcpy(v_of0, h_if0, sizeof(cl_double) * dims[0] * dims[1]);
    memcpy(v_of1234, h_if1234, sizeof(cl_double4) * dims[0] * dims[1]);
    memcpy(v_of5678, h_if5678, sizeof(cl_double4) * dims[0] * dims[1]);

    for (int y = 0; y < dims[1]; y++)
    {
        for (int x = 0; x < dims[0]; x++)
        {
            collide(x, y);       
        }
    }
    for (int y = 0; y < dims[1]; y++)
    {
        for (int x = 0; x < dims[0]; x++)
        {
             streamToNeighbors(x, y);     
        }
    }
   
    int flag0 = 0;
    for (int y = 0; y < dims[1]; y++)
    {
        for (int x = 0; x < dims[0]; x++)
        {
            int pos = x + y * dims[0];
            if(h_of0[pos] - v_of0[pos] > 1e-5)
            {
                flag0 = 1;
                break;
            }
        }
    }

    int flag1234 = 0;
    for (int y = 0; y < dims[1]; y++)
    {
        for (int x = 0; x < dims[0]; x++)
        {
            int pos = x + y * dims[0];
            if(h_of1234[pos] - v_of1234[pos] > 1e-2)
            {
                std::cout << pos << "=" << h_of1234[pos] - v_of1234[pos] << std::endl;
                flag1234 = 1;
                break;
            } 
        }
    }

    int flag5678 = 0;
    for (int y = 0; y < dims[1]; y++)
    {
        for (int x = 0; x < dims[0]; x++)
        {
            int pos = x + y * dims[0];
            if(h_of5678[pos] - v_of5678[pos] > 1e-2)
            {
                std::cout << pos << "=" << h_of1234[pos] - v_of1234[pos] << std::endl;
                flag5678 = 1;
                break;
            } 
        }
    }

    if(!flag0 && !flag1234 && !flag5678)
        verifyFlag = 1;

}

int
FluidSimulation2D::initialize()
{
    /* Call base class Initialize to get default configuration */
    if(!this->SDKSample::initialize())
        return SDK_FAILURE;

    streamsdk::Option *num_iterations = new streamsdk::Option;
    if(!num_iterations)
    {
        std::cout << "Error. Failed to allocate memory (num_iterations)\n";
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
FluidSimulation2D::setup()
{
    if(setupFluidSimulation2D() != SDK_SUCCESS)
        return SDK_FAILURE;

    int timer = sampleCommon->createTimer();
    sampleCommon->resetTimer(timer);
    sampleCommon->startTimer(timer);

    int status = setupCL();
    if(status != SDK_SUCCESS)
    {
        return status;
    }

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

void render()
{
	glClear(GL_COLOR_BUFFER_BIT);

    glBindTexture(GL_TEXTURE_2D, texnum);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

    unsigned char bitmap[LBWIDTH * LBHEIGHT * 4];	// rgba unsigned bytes

    double m, r, g, b;

    for(int y = 0; y < LBHEIGHT; y++)
    {
        for(int x = 0; x < LBWIDTH; x++)
        {
            if(me->isBoundary(x , y))
            {
                r = g = b = 0;
            }
            else
            {
                cl_double2 vel = me->getVelocity(x, y);
                m = sqrt(vel.s[0] * vel.s[0] + vel.s[1] * vel.s[1]);
                bluewhite.GetColor(m*20, r, g, b);
            }

            bitmap[(x+y*LBWIDTH)*4 + 0] = (unsigned char)(r * 255);
            bitmap[(x+y*LBWIDTH)*4 + 1] = (unsigned char)(g * 255);
            bitmap[(x+y*LBWIDTH)*4 + 2] = (unsigned char)(b * 255);
            bitmap[(x+y*LBWIDTH)*4 + 3] = 255;
        }
    }



    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, LBWIDTH, LBHEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, bitmap);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glEnable(GL_TEXTURE_2D);


	// setup 2d pixel plotting camera
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0f, (GLdouble) winwidth, 0.0f, (GLdouble) winheight, 0.0f, 1.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glViewport(0, 0, winwidth, winheight);

    glBegin(GL_QUADS);

    glColor3f(1.0, 1.0, 1.0);

    glTexCoord2f(0.0, 0.0);
    glVertex2i(0, 0);

    glTexCoord2f(1.0, 0.0);
    glVertex2i(winwidth, 0);

    glTexCoord2f(1.0, 1.0);
    glVertex2i(winwidth, winheight);

    glTexCoord2f(0.0, 1.0);
    glVertex2i(0, winheight);

    glEnd();

    glDisable(GL_TEXTURE_2D);

    glFlush();
	glutSwapBuffers();
}

void update()
{
	me->runCLKernels();

	// redraw
	glutPostRedisplay();

	frames++;

	te = glutGet(GLUT_ELAPSED_TIME);
	
	// every second approximately
	if (te-t0 >= 1000)
	{
		char title[80];
		sprintf(title, "Lattice Boltzmann demo    %.1f fps", (1000.0*frames/(te-t0)));
		glutSetWindowTitle(title);

		frames = 0;
		t0 = te;
	}		
}

void mouse(int button, int state, int x, int y)
{
	double u[2] = {0, 0};

	drawBoundary = false;
	addVelocity = false;

    x = (double)x * (double)((double)LBWIDTH / (double)winwidth);
    y = (double)y * (double)((double)LBHEIGHT / (double)winheight);

	if (state == GLUT_DOWN)
	{
		if (button == GLUT_LEFT_BUTTON)
		{
			if (me->isFluid(x, y) && x >= 0 && x < LBWIDTH && y >= 0 && y < LBHEIGHT)
			{
				addVelocity = true;
				oldx = x;
				oldy = y;
			}
		}

		if (button == GLUT_RIGHT_BUTTON)
		{
			drawBoundary = true;

			if (x >= 0 && x < LBWIDTH && y >= 0 && y < LBHEIGHT)
				me->setSite(x, LBHEIGHT-1-y, 1, u);
		}
	}
}

void motion(int x, int y)
{
	double m, u[2] = {0, 0};

    x = (double)x * (double)((double)LBWIDTH / (double)winwidth);
    y = (double)y * (double)((double)LBHEIGHT / (double)winheight);

	if (drawBoundary && (x >= 0 && x < LBWIDTH && y >= 0 && y < LBHEIGHT))
		me->setSite(x, LBHEIGHT-1-y, 1, u);

	if (addVelocity && (x >= 0 && x < LBWIDTH && y >= 0 && y < LBHEIGHT))
	{
		if (me->isFluid(x, y))
		{
			u[0] = (x-oldx);
			u[1] = (oldy-y);

			m = sqrt(u[0]*u[0]+u[1]*u[1]);
			u[0] /= (1+2*m);
			u[1] /= (1+2*m);

			me->setSite(x, LBHEIGHT-1-y, 0, u);
		}
	}
}

void reshape(int w, int h)
{
	winwidth = w;
	winheight = h;
}

void keyboard(unsigned char key, int x, int y)
{
	if (key == 27)
		exit(0);
	
	if (key == 'r')
	{
		me->reset();
	}
	else if (key == 'v')
	{
		drawVelocities *= -1;
	}
	else if (key == 'p')
	{
		drawParticles *= -1;
	}
}


int 
FluidSimulation2D::run()
{
    if(!reqdExtSupport)
        return SDK_SUCCESS;

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

    return SDK_SUCCESS;
}

int
FluidSimulation2D::verifyResults()
{
    if(verify)
    {
        /* reference implementation
        * it overwrites the input array with the output
        */

        for(int i = 0; i < iterations; ++i)
        {
            CPUReference();
        }

        /* compare the results and see if they match */
        if(verifyFlag)
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
FluidSimulation2D::printStats()
{
    std::string strArray[5] = 
    {
        "Width",
        "Height",
        "Iterations", 
        "Time(sec)", 
        "kernelTime(sec)"
    };

    std::string stats[5];
    totalTime = setupTime + kernelTime;

    stats[0] = sampleCommon->toString(dims[0], std::dec);
    stats[1] = sampleCommon->toString(dims[1], std::dec);
    stats[2] = sampleCommon->toString(iterations, std::dec);
    stats[3] = sampleCommon->toString(totalTime, std::dec);
    stats[4] = sampleCommon->toString(kernelTime, std::dec);

    this->SDKSample::printStats(strArray, stats, 5);
}

int
FluidSimulation2D::cleanup()
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

    status = clReleaseMemObject(d_if0);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clReleaseMemObject failed."))
    {
        return SDK_FAILURE;
    }

    status = clReleaseMemObject(d_of0);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clReleaseMemObject failed."))
    {
        return SDK_FAILURE;
    }

    status = clReleaseMemObject(d_if1234);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clReleaseMemObject failed."))
    {
        return SDK_FAILURE;
    }

    status = clReleaseMemObject(d_of1234);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clReleaseMemObject failed."))
    {
        return SDK_FAILURE;
    }

    status = clReleaseMemObject(d_if5678);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clReleaseMemObject failed."))
    {
        return SDK_FAILURE;
    }

    status = clReleaseMemObject(d_of5678);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clReleaseMemObject failed."))
    {
        return SDK_FAILURE;
    }

    status = clReleaseMemObject(type);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clReleaseMemObject failed."))
    {
        return SDK_FAILURE;
    }

    status = clReleaseMemObject(weight);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clReleaseMemObject failed."))
    {
        return SDK_FAILURE;
    }

    status = clReleaseMemObject(velocity);
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

FluidSimulation2D::~FluidSimulation2D()
{
    /* release program resources */
    if(h_if0)
    {
        free(h_if0);
        h_if0 = NULL;
    }

    if(h_if1234)
    {
        free(h_if1234);
        h_if1234 = NULL;
    }

    if(h_if5678)
    {
        free(h_if1234);
        h_if1234 = NULL;
    }

    if(h_of0)
    {
        free(h_of0);
        h_of0 = NULL;
    }

    if(h_of1234)
    {
        free(h_of1234);
        h_of1234 = NULL;
    }

    if(h_of5678)
    {
        free(h_of1234);
        h_of1234 = NULL;
    }

    if(v_ef0)
    {
        free(v_ef0);
        v_ef0 = NULL;
    }

    if(v_ef1234)
    {
        free(v_ef1234);
        v_ef1234 = NULL;
    }

    if(v_ef5678)
    {
        free(v_ef5678);
        v_ef5678 = NULL;
    }

    if(v_of0)
    {
        free(v_of0);
        v_of0 = NULL;
    }

    if(v_of1234)
    {
        free(v_of1234);
        v_of1234 = NULL;
    }

    if(v_of5678)
    {
        free(v_of1234);
        v_of1234 = NULL;
    }

    if(h_type)
    {
        free(h_type);
        h_type = NULL;
    }

    if(h_weight)
    {
        free(h_weight);
        h_weight = NULL;
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
}


int 
main(int argc, char * argv[])
{
    FluidSimulation2D clFluidSim("OpenCL FluidSimulation2D");
    me = &clFluidSim;

    // create color scale
    bluewhite.AddPoint(0.0, 0, 0, 0);
    bluewhite.AddPoint(0.2, 0, 0, 1);
    bluewhite.AddPoint(0.4, 0, 1, 1);
    bluewhite.AddPoint(0.8, 0, 1, 0);
    bluewhite.AddPoint(1.6, 1, 1, 0);
    bluewhite.AddPoint(3.2, 1, 0, 0);

    if(clFluidSim.initialize() != SDK_SUCCESS)
        return SDK_FAILURE;
    if(!clFluidSim.parseCommandLine(argc, argv))
        return SDK_FAILURE;

    if(clFluidSim.isDumpBinaryEnabled())
    {
        return clFluidSim.genBinaryImage();
    }
    else
    {
        int status = clFluidSim.setup();
        if(status != SDK_SUCCESS)
        {
            if(status == SDK_EXPECTED_FAILURE)
                return SDK_SUCCESS;
            else
                return SDK_FAILURE;
        }
        if(clFluidSim.run() != SDK_SUCCESS)
            return SDK_FAILURE;

        if(clFluidSim.verifyResults() != SDK_SUCCESS)
            return SDK_FAILURE;

        clFluidSim.printStats();

        if(display)
        {
            // Run in  graphical window if requested 
            glutInit(&argc, argv);
            glutInitWindowPosition(100,10);
            glutInitWindowSize(400,400); 
            glutInitDisplayMode( GLUT_RGB | GLUT_DOUBLE );
            glutCreateWindow("LBM simulation"); 
            GLInit(); 
            glutDisplayFunc(render);
            glutIdleFunc(update);
            glutMouseFunc(mouse);
            glutMotionFunc(motion);
            glutReshapeFunc(reshape);
            glutKeyboardFunc(keyboard);

            std::cout << "Use Left-Mouse button to move the fluid\n";
            std::cout << "Use Right-Mouse button to draw boundary\n";
            std::cout << "Press r to reset the simulation\n";

            glutMainLoop();
        }

        if(clFluidSim.cleanup()!=SDK_SUCCESS)
            return SDK_FAILURE;
    }

    return SDK_SUCCESS;
}
