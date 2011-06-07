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


int AmbientOcclusionRenderer::setupCL()
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

    /* Now allocate memory for device list based on the size we got earlier */
    devices = (cl_device_id*)malloc(deviceListSize);
    if(devices==NULL) {
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
        devices[0],
        0,
        &status);

    if(!sampleCommon->checkVal(
        status,
        CL_SUCCESS,
        "clCreateCommandQueue failed."))
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
#ifdef TESTBENCH_BUILD_FROM_CL
    streamsdk::SDKFile kernelFile;
    std::string kernelPath = sampleCommon->getPath();
    kernelPath.append("AmbientOcclusionRenderer_Kernels.cl");
    if(!kernelFile.open(kernelPath.c_str()))
    {
        std::cout << "Failed to load kernel file : " << kernelPath << std::endl;
        return SDK_FAILURE;
    }
    const char * source = kernelFile.source().c_str();
#else
    const char * source = "AmbientOcclusionRenderer_Kernels.bc";
#endif
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

    /* create a cl program executable for all the devices specified */
    status = clBuildProgram(
        program,
        1,
        &devices[0],
        NULL,
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
                devices[0], 
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
                devices[0], 
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
        "AmbientOcclusionRenderer",
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

void AmbientOcclusionRenderer::printStats()
{
	std::string strArray[2] = {"WxH" , "SetupTime(sec)"};
	std::string stats[2];

	stats[0]  = sampleCommon->toString(width, std::dec)
		+"x"+sampleCommon->toString(height, std::dec);
	stats[1]  = sampleCommon->toString(setupTime, std::dec);

	this->SDKSample::printStats(strArray, stats, 3);
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
	printf("fps = %3.5f\n", (double)1.0/(etm - stm));
}

void keyboard(unsigned char key, int x, int y)
{
	if (key == 27)// ESC
		exit(0);
}

void idle()
{
	glutPostRedisplay();
}

int main (int argc, char* argv[])
{
	clAmbientOcclusionRenderer = new AmbientOcclusionRenderer("OpenCL AmbientOcclusionRenderer");
	teximage = new unsigned char[clAmbientOcclusionRenderer->getWidth() * clAmbientOcclusionRenderer->getHeight() * 4];

	//AmbientOcclusionRenderer clAmbientOcclusionRenderer("OpenCL AmbientOcclusionRenderer");
	//me = &clAmbientOcclusionRenderer;

	if(clAmbientOcclusionRenderer->initialize() != SDK_SUCCESS)
        return SDK_FAILURE;
    if(!clAmbientOcclusionRenderer->parseCommandLine(argc, argv))
        return SDK_FAILURE;
    if(clAmbientOcclusionRenderer->setup() != SDK_SUCCESS)
        return SDK_FAILURE;
    //if(clAmbientOcclusionRenderer.run() != SDK_SUCCESS)
        //return SDK_FAILURE;
	
	glutInit(&argc, argv);
	glutInitWindowSize(clAmbientOcclusionRenderer->getWidth(), clAmbientOcclusionRenderer->getHeight());
	glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE);
	glutCreateWindow("OpenCL AO");
	initGL();
		
	glutIdleFunc(idle);
	glutDisplayFunc(displayGLWindow);
	glutKeyboardFunc(keyboard);
	glutMainLoop();

	clAmbientOcclusionRenderer->printStats();

    if(clAmbientOcclusionRenderer->cleanup()!=SDK_SUCCESS)
        return SDK_FAILURE;

    return SDK_SUCCESS;
}
