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


#ifndef MANDELBROT_H_
#define MANDELBROT_H_




#include <CL/cl.h>

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <SDKUtil/SDKCommon.hpp>
#include <SDKUtil/SDKApplication.hpp>
#include <SDKUtil/SDKCommandArgs.hpp>
#include <SDKUtil/SDKFile.hpp>

/**
 * SHA1 
 * Class implements OpenCL SHA1 sample
 * Derived from SDKSample base class
 */

class SHA1 : public SDKSample
{
    cl_uint                  seed;       /**< Seed value for random number generation */
    cl_double           setupTime;       /**< Time for setting up Opencl */
    cl_double     totalKernelTime;       /**< Time for kernel execution */
    cl_double    totalProgramTime;       /**< Time for program execution */
    cl_double referenceKernelTime;       /**< Time for reference implementation */
    cl_int    *verificationOutput;       /**< Output array from reference implementation */
    cl_int              scale_int;       /**< for command line arguments */
    cl_float                scale;       /**< paramters of mandelbrot */
    cl_uint         maxIterations;       /**< paramters of mandelbrot */
    cl_context            context;       /**< CL context */
    cl_device_id         *devices;       /**< CL device list */
    cl_mem           outputBuffer;       /**< CL memory buffer */
    cl_command_queue commandQueue;       /**< CL command queue */
    cl_program            program;       /**< CL program  */
    cl_kernel              kernel;       /**< CL kernel */
    cl_int                 width;        /**< width of the output image */
    cl_int                height;        /**< height of the output image */
    size_t    kernelWorkGroupSize;       /**< Group Size returned by kernel */
    int                iterations;       /**< Number of iterations for kernel execution */

	static const cl_uint H0 = 0x67452301L;
	static const cl_uint H1 = 0xEFCDAB89L;
	static const cl_uint H2 = 0x98BADCFEL;
	static const cl_uint H3 = 0x10325476L;
	static const cl_uint H4 = 0xC3D2E1F0L;

	cl_uint* word_buffer;
	cl_uint* work_items;
	size_t len;
	cl_uint* result;
	cl_uint* H_array;
	cl_uint* W_array;

public:
    cl_int *output;                      /**< Output array */

public:
    /** 
     * Constructor 
     * Initialize member variables
     * @param name name of sample (string)
     */
    SHA1(std::string name)
        : SDKSample(name)    {
            seed = 123;
            output = NULL;
            verificationOutput = NULL;
            scale = 3.0f;
            scale_int = (int)scale;
            maxIterations = 20;
            width = 256;
            setupTime = 0;
            totalKernelTime = 0;
            iterations = 1;
        }

    /** 
     * Constructor 
     * Initialize member variables
     * @param name name of sample (const char*)
     */
    SHA1(const char* name)
        : SDKSample(name)    {
            seed = 123;
            output = NULL;
            verificationOutput = NULL;
            scale = 3.0f;
            scale_int = (int)scale;
            maxIterations = 20;
            width = 256;
            setupTime = 0;
            totalKernelTime = 0;
            iterations = 1;
        }

    /**
     * Allocate and initialize host memory array with random values
     * @return 1 on success and 0 on failure
     */
    int setupSHA1();

    /**
     * OpenCL related initialisations. 
     * Set up Context, Device list, Command Queue, Memory buffers
     * Build CL kernel program executable
     * @return 1 on success and 0 on failure
     */
    int setupCL();

    /**
     * Set values for kernels' arguments, enqueue calls to the kernels
     * on to the command queue, wait till end of kernel execution.
     * Get kernel start and end time if timing is enabled
     * @return 1 on success and 0 on failure
     */
    int runCLKernels();


    /**
     * SHA1 image generated with CPU reference implementation
     * @param verificationOutput mandelbrot images is stored in this
     * @param mandelbrotImage    mandelbrot images is stored in this
     * @param scale              Represents the distance from which the fractal 
     *                           is being seen if this is greater more area and 
     *                           less detail is seen           
     * @param maxIterations      More iterations gives more accurate mandelbrot image 
     * @param width              size of the image 
     */
    void sha1CPUReference(cl_uint *W, cl_uint *H, cl_uint *workItems);

    /**
     * Override from SDKSample. Print sample stats.
     */
    void printStats();

    /**
     * Override from SDKSample. Initialize 
     * command line parser, add custom options
     */
    int initialize();

    /**
     * Override from SDKSample, adjust width and height 
     * of execution domain, perform all sample setup
     */
    int setup();

    /**
     * Override from SDKSample
     * Run OpenCL SHA1 Sample
     */
    int run();

    /**
     * Override from SDKSample
     * Cleanup memory allocations
     */
    int cleanup();

    /**
     * Override from SDKSample
     * Verify against reference implementation
     */
    int verifyResults();
};



#endif
