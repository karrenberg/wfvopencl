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

// you will use GPU mode, try!
#define USE_GPU //also for packetization



//#define USE_TEXTURE

#include <stdio.h>
#include <iostream>
#include <sys/time.h>
#include <cstdlib>
#include <string.h>

#if defined(__APPLE__) || defined(__MACOSX)
#include <OpenCL/cl.h>
#include <GLUT/GLUT.h>
#else
#include <CL/cl.h>
#include <GL/glut.h>
#endif


unsigned int tex;
unsigned char* teximage;
int width = 256;
int height = 256;
cl_device_id device_id;
cl_context context;
cl_program program;
cl_kernel kernel;
cl_command_queue cmdqueue;
cl_mem hDevMem;

void executekernel(cl_device_id device_id, cl_command_queue hCmdQueue, cl_kernel hKernel, size_t dim1, size_t dim2)
{
	int err;
	// Get the maximum work-group size for executing the kernel on the device
	//size_t local;
	//err = clGetKernelWorkGroupInfo(hKernel, device_id, CL_KERNEL_WORK_GROUP_SIZE, sizeof(local), &local, NULL);
	//if (err != CL_SUCCESS)
	//{
	//	printf("Error: clGetKernelWorkGroupInfo Failed\n");
	//	exit(1);
	//}
	
	// execute kernel
	const size_t dim[2] = {dim1, dim2};
#ifdef USE_GPU
	const size_t local2[2] = {16, 16};
#else
	const size_t local2[2] = {1, 1};
#endif
	err = clEnqueueNDRangeKernel(hCmdQueue, hKernel, 2, NULL, (size_t*)(dim), local2, 0, NULL, NULL);
	if (err != CL_SUCCESS)
	{
		printf("Error: clEnqueueNDRangeKernel Failed\n");
		exit(1);
	}
	
}

void createProgram(cl_device_id device_id, cl_context hContext, const char* programSource, cl_program& hProgram, cl_kernel& hKernel)
{
	int err;
	size_t len;
	char buffer[4096];
	// create & compile program
	hProgram = clCreateProgramWithSource(hContext, 1, (const char **) &programSource, NULL, &err);
	if (!hProgram || err != CL_SUCCESS)
	{
		printf("Error: Failed to Create program with source\n");
		exit(1);
	}
	
	err = clBuildProgram(hProgram, 0, NULL, NULL, NULL, NULL);
	if (err != CL_SUCCESS)
	{
		printf("Error: Failed to build program executable\n");
		clGetProgramBuildInfo(hProgram, device_id, CL_PROGRAM_BUILD_LOG,
							  sizeof(buffer), buffer, &len);
		printf("%s\n", buffer);
		exit(1);
	}
	
	// create kernel
	hKernel = clCreateKernel(hProgram, "AmbientOcclusionRenderer", &err);
	if (!hKernel || err != CL_SUCCESS)
	{
		printf("Error: Failed to create kernel\n");
		exit(1);
	}
}

void initCL(cl_device_id& device_id, cl_context& hContext, cl_command_queue& hCmdQueue)
{
#ifdef USE_GPU
	const int gpu = 0;//1; //hacked ;)
#else
	const int gpu = 0;
#endif
	int err=CL_SUCCESS;
	size_t len;
	char buffer[2048];
	clGetDeviceIDs(NULL, gpu==1 ? CL_DEVICE_TYPE_GPU : CL_DEVICE_TYPE_CPU, 1, &device_id, NULL);
	if (err != CL_SUCCESS)
	{
		printf("Error: Failed to get device ID\n");
		exit(1);
	}
	clGetDeviceInfo(device_id, CL_DEVICE_NAME, sizeof(buffer), buffer, &len);
	printf("CL_DEVICE_NAME: %s\n", buffer);
	clGetDeviceInfo(device_id, CL_DEVICE_VENDOR, sizeof(buffer), buffer, &len);
	printf("CL_DEVICE_VENDOR: %s\n", buffer);
	
	hContext = clCreateContext(0, 1, &device_id, NULL, NULL, &err);
	if (err != CL_SUCCESS)
	{
		printf("Error: Failed to create context\n");
		exit(1);
	}
	
	// create a command queue for our device
	hCmdQueue = clCreateCommandQueue(hContext, device_id, 0, 0);
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

#if 0
void init(const char* path)
{
	printf("\n\n");
	initCL(device_id, context, cmdqueue);
	
	char pathbuf[1024];
	sprintf(pathbuf, "%sao.cl",path);
	FILE* fp;
	if ((fp = fopen(pathbuf, "rt")) == 0)
	{
		printf("Error: Failed to open ao.cl\n");
		exit(1);
	}
	fseek(fp, 0, SEEK_END);
	int srcSize = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	char* srcPrg = new char[srcSize+1];
	memset(srcPrg, 0, srcSize+1);
	fread(srcPrg, 1, srcSize, fp);
	createProgram(device_id, context, srcPrg, program, kernel);
	
	hDevMem = clCreateBuffer(context, CL_MEM_WRITE_ONLY, width * height * 4, 0, 0);
	int err;
	err  = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&hDevMem);
	if (err != CL_SUCCESS)
	{
		printf("Error: Failed to set kernel args\n");
		exit(1);
	}
	
	teximage = new unsigned char[width * height * 4];
}
#else
void init(const char* path)
{
	printf("\n\n");
	initCL(device_id, context, cmdqueue);

	int err;

	// Create the compute program from the source buffer
    //
    const char * srcPrg = "AmbientOcclusionRenderer_Kernels.bc";
	createProgram(device_id, context, srcPrg, program, kernel);

	hDevMem = clCreateBuffer(context, CL_MEM_WRITE_ONLY, width * height * 4, 0, 0);
	err  = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&hDevMem);
	if (err != CL_SUCCESS)
	{
		printf("Error: Failed to set kernel args\n");
		exit(1);
	}

	teximage = new unsigned char[width * height * 4];
}
#endif

void deinit()
{
	delete [] teximage;
	clReleaseMemObject(hDevMem);
	clReleaseProgram(program);
	clReleaseKernel(kernel);
	clReleaseCommandQueue(cmdqueue);
	clReleaseContext(context);
}

double gettimeofday_sec()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + (double)tv.tv_usec*1e-6;
}

void display()
{
	double stm = gettimeofday_sec();
	
	executekernel(device_id, cmdqueue, kernel, width, height);
	clEnqueueReadBuffer(cmdqueue, hDevMem, CL_TRUE, 0, width*height*4, teximage, 0, NULL, NULL);
	clFinish(cmdqueue);
		
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
	glDrawPixels(width, height, GL_RGBA, GL_UNSIGNED_BYTE, teximage);
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
	std::string exepath(argv[0]);
	size_t p = exepath.rfind("/");
	if (p != std::string::npos) {
		exepath.erase(exepath.begin()+p+1, exepath.end());
	}
	else {
		exepath = std::string("");
	}
	init(exepath.c_str());
	
	glutInit(&argc, argv);
	glutInitWindowSize(width, height);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE);
	glutCreateWindow("OpenCL AO");
	initGL();
		
	glutIdleFunc(idle);
	glutDisplayFunc(display);
	glutKeyboardFunc(keyboard);
	glutMainLoop();
	deinit();
    return 0;
}
