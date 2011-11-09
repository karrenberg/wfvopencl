//
// File:       hello.c
//
// Abstract:   A simple "Hello World" compute example showing basic usage of OpenCL which
//             calculates the mathematical square (X[i] = pow(X[i],2)) for a buffer of
//             floating point values.
//
//
// Version:    <1.0>
//
// Disclaimer: IMPORTANT:  This Apple software is supplied to you by Apple Inc. ("Apple")
//             in consideration of your agreement to the following terms, and your use,
//             installation, modification or redistribution of this Apple software
//             constitutes acceptance of these terms.  If you do not agree with these
//             terms, please do not use, install, modify or redistribute this Apple
//             software.
//
//             In consideration of your agreement to abide by the following terms, and
//             subject to these terms, Apple grants you a personal, non - exclusive
//             license, under Apple's copyrights in this original Apple software ( the
//             "Apple Software" ), to use, reproduce, modify and redistribute the Apple
//             Software, with or without modifications, in source and / or binary forms;
//             provided that if you redistribute the Apple Software in its entirety and
//             without modifications, you must retain this notice and the following text
//             and disclaimers in all such redistributions of the Apple Software. Neither
//             the name, trademarks, service marks or logos of Apple Inc. may be used to
//             endorse or promote products derived from the Apple Software without specific
//             prior written permission from Apple.  Except as expressly stated in this
//             notice, no other rights or licenses, express or implied, are granted by
//             Apple herein, including but not limited to any patent rights that may be
//             infringed by your derivative works or by other works in which the Apple
//             Software may be incorporated.
//
//             The Apple Software is provided by Apple on an "AS IS" basis.  APPLE MAKES NO
//             WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION THE IMPLIED
//             WARRANTIES OF NON - INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A
//             PARTICULAR PURPOSE, REGARDING THE APPLE SOFTWARE OR ITS USE AND OPERATION
//             ALONE OR IN COMBINATION WITH YOUR PRODUCTS.
//
//             IN NO EVENT SHALL APPLE BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL OR
//             CONSEQUENTIAL DAMAGES ( INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
//             SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
//             INTERRUPTION ) ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION, MODIFICATION
//             AND / OR DISTRIBUTION OF THE APPLE SOFTWARE, HOWEVER CAUSED AND WHETHER
//             UNDER THEORY OF CONTRACT, TORT ( INCLUDING NEGLIGENCE ), STRICT LIABILITY OR
//             OTHERWISE, EVEN IF APPLE HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Copyright ( C ) 2008 Apple Inc. All Rights Reserved.
//
////////////////////////////////////////////////////////////////////////////////

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <fstream>
#include <math.h>
#if !defined(_WIN32)
#include <unistd.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>

#include <SDKCommon.hpp>
#include <SDKApplication.hpp>
#include <SDKCommandArgs.hpp>
#include <SDKFile.hpp>

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/opencl.h>
#endif

////////////////////////////////////////////////////////////////////////////////

// Use a static data size for simplicity
//
#define DATA_SIZE (16)

////////////////////////////////////////////////////////////////////////////////

inline bool verifyResults(float* results, float* data, const unsigned i, const unsigned j) {
	//float correctRes = data[index] + (index == DATA_SIZE-1 ? data[0] : data[index+1]);
	float correctRes = data[i] + data[j];

	const int idx = j + i*DATA_SIZE;
	const bool correct = results[idx] == correctRes;
	return correct;
}

int main(int argc, char** argv) {
	int err; // error code returned from api calls

	float data[DATA_SIZE]; // original data set given to device
	float results[DATA_SIZE * DATA_SIZE]; // results returned from device
	unsigned int correct; // number of correct results returned

	size_t global[2]; // global domain size for our calculation
	size_t local[2]; // local domain size for our calculation

	cl_device_id device_id; // compute device id
	cl_context context; // compute context
	cl_command_queue commands; // compute command queue
	cl_program program; // compute program
	cl_kernel kernel; // compute kernel

	cl_mem input; // device memory used for the input array
	cl_mem output; // device memory used for the output array

	bool useAMD = false;
	bool useIntel = false;
	bool usePacketizer = true;

	char* requestedPlatformString = NULL;

	// Check command line arguments for desired platform
	//
	for (int i=1; i<argc; ++i) {
		requestedPlatformString = argv[i];
		if (!strcmp(requestedPlatformString, "AMD") || !strcmp(requestedPlatformString, "amd")) {
			useAMD = true;
			useIntel = false;
			usePacketizer = false;
		} else if (!strcmp(requestedPlatformString, "Intel") || !strcmp(requestedPlatformString, "intel")) {
			useAMD = false;
			useIntel = true;
			usePacketizer = false;
		} else if (!strcmp(requestedPlatformString, "packetizer") || !strcmp(requestedPlatformString, "PacketizedOpenCL") || !strcmp(requestedPlatformString, "pkt")) {
			useAMD = false;
			useIntel = false;
			usePacketizer = true;
		}
	}
	
	// Fill our data set with random float values
	//
	unsigned i = 0;
	const unsigned int dataSize = DATA_SIZE;
	for (i = 0; i < dataSize; i++) {
		//data[i] = rand() / (float) RAND_MAX;
		data[i] = i;
		//if (i < 8) printf("  data[%d] = %f\n", i, data[i]);
	}

	cl_uint numPlatforms;
    cl_platform_id platform = NULL;
    err = clGetPlatformIDs(0, NULL, &numPlatforms);
    if(err != CL_SUCCESS)
    {
        printf("Error: Getting Platforms. (clGetPlatformsIDs)\n");
        return 1;
    }
    
	char vendorName[100];
	char platformName[100];
    if(numPlatforms > 0)
    {
        cl_platform_id* platforms = new cl_platform_id[numPlatforms];
        err = clGetPlatformIDs(numPlatforms, platforms, NULL);
        if(err != CL_SUCCESS)
        {
            printf("Error: Getting Platform Ids. (clGetPlatformsIDs)\n");
            return 1;
        }
        for(unsigned int i=0; i < numPlatforms; ++i)
        {
            err = clGetPlatformInfo(
                        platforms[i],
                        CL_PLATFORM_VENDOR,
                        sizeof(vendorName),
                        vendorName,
                        NULL);
            if(err != CL_SUCCESS)
            {
                printf("Error: Getting Platform Info.(clGetPlatformInfo)\n");
                return 1;
            }
            err = clGetPlatformInfo(
                        platforms[i],
                        CL_PLATFORM_NAME,
                        sizeof(platformName),
                        platformName,
                        NULL);
            if(err != CL_SUCCESS)
            {
                printf("Error: Getting Platform Info.(clGetPlatformInfo)\n");
                return 1;
            }
			if (useAMD && !strcmp(vendorName, "Advanced Micro Devices, Inc.")) {
				platform = platforms[i];
                break;
            } else if (useIntel && !strcmp(vendorName, "Intel(R) Corporation")) {
				platform = platforms[i];
                break;
            } else if (usePacketizer && !strcmp(vendorName, "Ralf Karrenberg, Saarland University")) {
				platform = platforms[i];
                break;
            }
        }
        delete platforms;
    }

    if(NULL == platform)
    {
		printf("Requested platform '%s' not found!\n", requestedPlatformString);
        return 1;
    }
	
	printf("\nPlatform vendor: %s\n", vendorName);
	printf("Platform name  : %s\n", platformName);

    // If we could find our platform, use it. Otherwise use just available platform.
    //
    cl_context_properties cps[3] = { CL_CONTEXT_PLATFORM, (cl_context_properties)platform, 0 };

    // Create a compute context
    //
    context = clCreateContextFromType(cps, 
                                      CL_DEVICE_TYPE_CPU, 
                                      NULL, 
                                      NULL, 
                                      &err);
    if (!context)
    {
        printf("Error: Failed to create a compute context!\n");
        return EXIT_FAILURE;
    }

    // Get the size of device list data
	//
    size_t deviceListSize;
    err = clGetContextInfo(context, 
                              CL_CONTEXT_DEVICES, 
                              0, 
                              NULL, 
                              &deviceListSize);
    if(err != CL_SUCCESS) 
	{  
		printf("Error: Getting Context Info \
		    (device list size, clGetContextInfo)\n");
		return 1;
	}

    cl_device_id* devices = (cl_device_id *)malloc(deviceListSize);

	// Detect OpenCL devices
	//
    devices = (cl_device_id *)malloc(deviceListSize);
	if(devices == 0)
	{
		printf("Error: No devices found.\n");
		return 1;
	}

    // Now, get the device list data
	//
    err = clGetContextInfo(
			     context, 
                 CL_CONTEXT_DEVICES, 
                 deviceListSize, 
                 devices, 
                 NULL);
    if(err != CL_SUCCESS) 
	{ 
		printf("Error: Getting Context Info \
		    (device list, clGetContextInfo)\n");
		return 1;
	}

	device_id = devices[0];

    // Create a command commands
    //
    commands = clCreateCommandQueue(context, device_id, 0, &err);
    if (!commands)
    {
        printf("Error: Failed to create a command commands!\n");
        return EXIT_FAILURE;
    }

    // Create the compute program from the source buffer
    //
    streamsdk::SDKFile kernelFile;
	streamsdk::SDKCommon* sampleCommon = new streamsdk::SDKCommon();
    std::string kernelPath = sampleCommon->getPath();
	kernelPath.append("Test2D_Kernels.cl");
	if(!kernelFile.open(kernelPath.c_str()))
	{
		printf("Failed to load kernel file : %s\n", kernelPath.c_str());
		return SDK_FAILURE;
	}

	const char * source = kernelFile.source().c_str();
    size_t sourceSize[]    = { strlen(source) };

    program = clCreateProgramWithSource(
			      context, 
                  1, 
                  &source,
				  sourceSize,
                  &err);
	if(err != CL_SUCCESS) 
	{ 
		printf("Error: Loading Binary into cl_program \
			   (clCreateProgramWithBinary)\n");
	  return 1;
	}

    // Build the program executable
    //
    /* create a cl program executable for all the devices specified */
    err = clBuildProgram(program, 1, devices, NULL, NULL, NULL);
	if (err != CL_SUCCESS) {
		size_t len;
		char buffer[2048];

		printf("Error: Failed to build program executable!\n");
		clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, sizeof (buffer), buffer, &len);
		printf("%s\n", buffer);
		exit(1);
	}

	// Create the compute kernel in the program we wish to run
	//
	kernel = clCreateKernel(program, "Test2D", &err);
	if (!kernel || err != CL_SUCCESS) {
		printf("Error: Failed to create compute kernel!\n");
		exit(1);
	}

	// Create the input and output arrays in device memory for our calculation
	//
	input = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof (float) * dataSize, NULL, NULL);
	output = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof (float) * dataSize * dataSize, NULL, NULL);
	if (!input || !output) {
		printf("Error: Failed to allocate device memory!\n");
		exit(1);
	}

	// Write our data set into the input array in device memory
	//
	err = clEnqueueWriteBuffer(commands, input, CL_TRUE, 0, sizeof (float) * dataSize, data, 0, NULL, NULL);
	if (err != CL_SUCCESS) {
		printf("Error: Failed to write to source array!\n");
		exit(1);
	}

	// Set the arguments to our compute kernel
	//
	err = 0;
	err = clSetKernelArg(kernel, 0, sizeof (cl_mem), &input);
	err |= clSetKernelArg(kernel, 1, sizeof (cl_mem), &output);
	if (err != CL_SUCCESS) {
		printf("Error: Failed to set kernel arguments! %d\n", err);
		exit(1);
	}


	// Execute the kernel over the entire range of our 2d input data set
	// using the maximum number of work group items for this device
	//
	global[0] = dataSize;
	global[1] = dataSize;
	local[0] = global[0];
	local[1] = global[1];
	err = clEnqueueNDRangeKernel(commands, kernel, 2, NULL, global, local, 0, NULL, NULL);
	if (err) {
		printf("Error: Failed to execute kernel!\n");
		return EXIT_FAILURE;
	}

	// Wait for the command commands to get serviced before reading back results
	//
	clFinish(commands);

	// Read back the results from the device to verify the output
	//
	err = clEnqueueReadBuffer(commands, output, CL_TRUE, 0, sizeof (float) * dataSize * dataSize, results, 0, NULL, NULL);
	if (err != CL_SUCCESS) {
		printf("Error: Failed to read output array! %d\n", err);
		exit(1);
	}

	// Validate our results
	//
	correct = 0;
	for (i = 0; i < dataSize; i++) {
		for (unsigned j = 0; j < dataSize; j++) {
			if (verifyResults(results, data, i, j)) {
				//printf("results[%d/%d][%d]: %f (correct)\n", i, j, j+i*dataSize, results[j+i*dataSize]);
				correct++;
			} else {
				//printf("results[%d/%d][%d]: %f (wrong, expected: %f)\n", i, j, j+i*dataSize, results[j+i*dataSize], data[i]+data[j]);
			}
		}
	}

	// Print a brief summary detailing the results
	//
	printf("Computed '%d/%d' correct values!\n", correct, dataSize*dataSize);
	const bool allCorrect = correct == dataSize*dataSize;

	// Shutdown and cleanup
	//
	clReleaseMemObject(input);
	clReleaseMemObject(output);
	clReleaseProgram(program);
	clReleaseKernel(kernel);
	clReleaseCommandQueue(commands);
	clReleaseContext(context);
	free (devices);

	return allCorrect ? 0 : 1; // 0 = successful
}
