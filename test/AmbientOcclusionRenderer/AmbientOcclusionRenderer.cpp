/*
 OpenCL AO Bench
 coded by kioku @ System K 2009
 
 about AO Bench[http://lucille.atso-net.jp/aobench/]

 compile:
 gcc -framework OpenGL -framework GLUT -framework Foundation -framework
 OpenCL -lstdc++ -O3 main.cpp
  or 
 use opencl_ao.xcodeproj
 
 test machine:
 - MacOSX 10.6(Snow Leopard) @ macbook 2008late(aluminum)
 - Core2Duo P7350 @ 2.0GHz
 - GeForce 9400M
*/

#include "AmbientOcclusionRenderer.hpp"


AmbientOcclusionRenderer* clAmbientOcclusionRenderer;
cl_uchar* teximage;


int AmbientOcclusionRenderer::setupAmbientOcclusionRenderer()
{
    return SDK_SUCCESS;
}

int 
AmbientOcclusionRenderer::genBinaryImage()
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
    kernelPath.append("AmbientOcclusionRenderer_Kernels.cl");
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


int AmbientOcclusionRenderer::setupCL()
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
                               "clGetDeviceInfo"
                               "CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS failed."))
    {
        return SDK_FAILURE;
    }

    /*
    * Create and initialize memory objects
    */

    /* Create memory object for output image */
    hDevMem = clCreateBuffer(
        context,
        CL_MEM_WRITE_ONLY,
        width * height * 4,
        0,
        &status);
    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clCreateBuffer failed. (hDevMem)"))
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
		if (!strcmp(vName, "Intel(R) Corporation")) {
			vendorName = "intel";
		} else if (!strcmp(vName, "Advanced Micro Devices, Inc.")) {
			vendorName = "amd";
		} else if (platformIsPacketizedOpenCL) {
			vendorName = "pkt";
		} else {
			printf("ERROR: vendor not recognized: %s\n", vName);
		}

		kernelPath.append("AmbientOcclusionRenderer_Kernels.cl");
		if(!kernelFile.open(kernelPath.c_str()))
		{
			std::cout << "Failed to load kernel file : " << kernelPath << std::endl;
			return SDK_FAILURE;
		}

		const char * source = platformIsPacketizedOpenCL ?
			"AmbientOcclusionRenderer_Kernels.bc" :
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
    kernel = clCreateKernel(program, "AmbientOcclusionRenderer", &status);
    if(!sampleCommon->checkVal(status,
                               CL_SUCCESS,
                               "clCreateKernel failed."))
    {
        return SDK_FAILURE;
    }

    return SDK_SUCCESS;
}


int AmbientOcclusionRenderer::runCLKernels()
{
    cl_int status;

    /* 
    * Enqueue a kernel run call.
    */
    size_t globalThreads[] = {height, width};
    size_t localThreads[] = {16, 16};

    //if(localThreads[0] > maxWorkItemSizes[0] ||
       //localThreads[0] > maxWorkGroupSize)
    //{
        //std::cout << "Unsupported: Device"
            //"does not support requested number of work items.";
        //return SDK_FAILURE;
    //}

	/* only kernel argument - output */
	status = clSetKernelArg(
			kernel, 
			0, 
			sizeof(cl_mem), 
			(void *)&hDevMem);
	if(!sampleCommon->checkVal(
				status,
				CL_SUCCESS,
				"clSetKernelArg failed. (outputBuffer)"))
		return SDK_FAILURE;

	status = clSetKernelArg(
			kernel,
			1,
			sizeof(cl_uint),
			(void *)&width);
	if(!sampleCommon->checkVal(
				status,
				CL_SUCCESS,
				"clSetKernelArg failed. (outputBuffer)"))
		return SDK_FAILURE;

	status = clSetKernelArg(
			kernel,
			2,
			sizeof(cl_uint),
			(void *)&height);
	if(!sampleCommon->checkVal(
				status,
				CL_SUCCESS,
				"clSetKernelArg failed. (outputBuffer)"))
		return SDK_FAILURE;


    status = clEnqueueNDRangeKernel(
        commandQueue,
        kernel,
        2,
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


	clEnqueueReadBuffer(commandQueue, hDevMem, CL_TRUE, 0, width*height*4, teximage, 0, NULL, NULL);
	clFinish(commandQueue);

    return SDK_SUCCESS;
}

// not used
int AmbientOcclusionRenderer::run()
{
	/*
	int argc = 1;
	char* argv[] = {"."};
	glutInit(&argc, argv);
	glutInitWindowSize(width, height);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE);
	glutCreateWindow("OpenCL AO");
	initGL();
		
	glutIdleFunc(idle);
	glutDisplayFunc(displayGLWindow);
	glutKeyboardFunc(keyboard);
	glutMainLoop();
	*/
    return SDK_SUCCESS;
}


int AmbientOcclusionRenderer::initialize()
{
    /* Call base class Initialize to get default configuration */
    if(!this->SDKSample::initialize())
        return SDK_FAILURE;

    streamsdk::Option *num_pixels_x = new streamsdk::Option;
    if(!num_pixels_x)
    {
        std::cout << "error. Failed to allocate memory (num_pixels_x)\n";
        return SDK_FAILURE;
    }

    num_pixels_x->_sVersion = "x";
    num_pixels_x->_lVersion = "width";
    num_pixels_x->_description = "Width of window";
    num_pixels_x->_type = streamsdk::CA_ARG_INT;
    num_pixels_x->_value = &width;

    sampleArgs->AddOption(num_pixels_x);
    delete num_pixels_x;

    streamsdk::Option *num_pixels_y = new streamsdk::Option;
    if(!num_pixels_y)
    {
        std::cout << "error. Failed to allocate memory (num_pixels_y)\n";
        return SDK_FAILURE;
    }

    num_pixels_y->_sVersion = "y";
    num_pixels_y->_lVersion = "height";
    num_pixels_y->_description = "Height of window";
    num_pixels_y->_type = streamsdk::CA_ARG_INT;
    num_pixels_y->_value = &height;

    sampleArgs->AddOption(num_pixels_y);
    delete num_pixels_y;


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

int AmbientOcclusionRenderer::setup()
{
    if(setupAmbientOcclusionRenderer() != SDK_SUCCESS)
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

void AmbientOcclusionRenderer::printStats()
{
	std::string strArray[3] = {"WxH" , "SetupTime(sec)", "kernelTime(sec)"};
	std::string stats[3];

	stats[0]  = sampleCommon->toString(width, std::dec)
		+"x"+sampleCommon->toString(height, std::dec);
	stats[1]  = sampleCommon->toString(setupTime, std::dec);
	stats[2]  = sampleCommon->toString(kernelTime, std::dec);

	this->SDKSample::printStats(strArray, stats, 3);
	this->SDKSample::logStats(kernelTime, "AmbientOcclusionRenderer", vendorName);
}


int AmbientOcclusionRenderer::cleanup()
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

    status = clReleaseMemObject(hDevMem);
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

AmbientOcclusionRenderer::~AmbientOcclusionRenderer()
{
    /* release program resources */
    if(devices)
    {
        free(devices);
        devices = NULL;
    }

    if(hDevMem)
    {
        free(hDevMem);
        hDevMem = NULL;
    }
	
	if (teximage)
	{
		delete [] teximage;
		teximage = NULL;
	}
}

int AmbientOcclusionRenderer::verifyResults()
{
	return 1;
}


void initGL()
{
#ifdef USE_TEXTURE
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glGenTextures(1, &tex);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA, width, height, 0, GL_RGBA,GL_UNSIGNED_BYTE, 0);
	glDisable(GL_TEXTURE_2D);
#endif
}


double gettimeofday_sec()
{
#ifdef _WIN32
	SYSTEMTIME st;
	GetSystemTime(&st);
	return st.wSecond + (double)st.wMilliseconds*1e-3;
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + (double)tv.tv_usec*1e-6;
#endif
}

void keyboard(unsigned char key, int x, int y)
{
	if (key == 27)// ESC
	{
		clAmbientOcclusionRenderer->printStats();

		if(clAmbientOcclusionRenderer->cleanup()!=SDK_SUCCESS)
			exit(1);

		exit(0);
	}
}

void displayGLWindow()
{
	double stm = gettimeofday_sec();
	
	clAmbientOcclusionRenderer->runCLKernels();
	
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#ifdef USE_TEXTURE
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, teximage);
	glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0);
		glVertex2i(-1,-1);
		glTexCoord2f(1.0, 0.0);
		glVertex2i( 1,-1);
		glTexCoord2f(1.0, 1.0);
		glVertex2i( 1, 1);
		glTexCoord2f(0.0, 1.0);
		glVertex2i(-1, 1);
	glEnd();
	glDisable(GL_TEXTURE_2D);
#else
	glRasterPos2d(-1,-1);
	glDrawPixels(clAmbientOcclusionRenderer->getWidth(), clAmbientOcclusionRenderer->getHeight(), GL_RGBA, GL_UNSIGNED_BYTE, teximage);
#endif
	glutSwapBuffers();
	
	// calc fps
	double etm = gettimeofday_sec();
	printf("fps = %3.5f (%3.5f sec)\n", (double)1.0/(etm - stm), (etm-stm));

	// store first run for statistics
	clAmbientOcclusionRenderer->setKernelTime(etm - stm);
}

void idle()
{
	glutPostRedisplay();
}

int main (int argc, char* argv[])
{
	clAmbientOcclusionRenderer = new AmbientOcclusionRenderer("OpenCL AmbientOcclusionRenderer");
	teximage = new unsigned char[clAmbientOcclusionRenderer->getWidth() * clAmbientOcclusionRenderer->getHeight() * 4];

	if(clAmbientOcclusionRenderer->initialize() != SDK_SUCCESS)
        return SDK_FAILURE;
    if(!clAmbientOcclusionRenderer->parseCommandLine(argc, argv))
        return SDK_FAILURE;
    if(clAmbientOcclusionRenderer->setup() != SDK_SUCCESS)
        return SDK_FAILURE;
    //if(clAmbientOcclusionRenderer.run() != SDK_SUCCESS)
        //return SDK_FAILURE;
	
	if (clAmbientOcclusionRenderer->isQuiet()) {
		// do not create OpenGL window
		double stm = gettimeofday_sec();
		clAmbientOcclusionRenderer->runCLKernels();
		double etm = gettimeofday_sec();
		clAmbientOcclusionRenderer->setKernelTime(etm - stm);
		clAmbientOcclusionRenderer->printStats();

		if(clAmbientOcclusionRenderer->cleanup()!=SDK_SUCCESS)
			return SDK_FAILURE;

	} else {
		glutInit(&argc, argv);
		glutInitWindowSize(clAmbientOcclusionRenderer->getWidth(), clAmbientOcclusionRenderer->getHeight());
		glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE);
		glutCreateWindow("OpenCL AO");
		initGL();

		glutIdleFunc(idle);
		glutDisplayFunc(displayGLWindow);
		glutKeyboardFunc(keyboard);
		glutMainLoop();

		// we cannot return from glutMainLoop() easily.
		// Thus, all has been moved to keyboard()
	}

	return SDK_SUCCESS;
}
