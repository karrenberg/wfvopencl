/**
 * This file is distributed under the University of Illinois Open Source
 * License. See the COPYING file in the root directory for details.
 *
 * Copyright (C) 2010, 2011 Saarland University
 */

#ifndef WFVOCL_H__
#define WFCOCL_H__

#include <cassert>
#include <cstdio> // remove, tmpnam
#include <cstring> // memcpy

#include <fstream>
#include <sstream>  // std::stringstream

//#include <xmmintrin.h> // test output etc.
//#include <emmintrin.h> // test output etc. (windows requires this for __m128i)

#ifdef __APPLE__
#include <OpenCL/cl_platform.h>
#include <OpenCL/cl.h>
#include <OpenCL/cl_ext.h>
#else
#include <CL/cl_platform.h> // e.g. for CL_API_ENTRY
#include <CL/cl.h>          // e.g. for cl_platform_id
#include <CL/cl_ext.h>      // e.g. for CL_PLATFORM_SUFFIX_KHR
#endif

#ifdef _WIN32
#	define WFVOPENCL_DLLEXPORT __declspec(dllexport)
#else
#	define WFVOPENCL_DLLEXPORT
#endif

#ifndef WFVOPENCL_NO_WFV
#include "packetizerAPI.hpp" // packetizer
#endif

#ifdef WFVOPENCL_ENABLE_JIT_PROFILING
#include "JITProfiling.h"
#endif

//#include "llvmTools.hpp" // LLVM functionality

#include "passes/callSiteBlockSplitter.h"
#include "passes/continuationGenerator.h"
#include "passes/livenessAnalyzer.h"
#include "llvmTools.hpp"
#include "wfvOpenCL.h"

#ifndef WFVOPENCL_FUNCTION_NAME_BARRIER
	#define WFVOPENCL_FUNCTION_NAME_BARRIER "barrier"
#endif

//----------------------------------------------------------------------------//
// Configuration
//----------------------------------------------------------------------------//
#define WFVOPENCL_VERSION_STRING "0.1" // <major_number>.<minor_number>

#define WFVOPENCL_EXTENSIONS "cl_khr_icd cl_amd_fp64 cl_khr_global_int32_base_atomics cl_khr_global_int32_extended_atomics cl_khr_local_int32_base_atomics cl_khr_local_int32_extended_atomics cl_khr_int64_base_atomics cl_khr_int64_extended_atomics cl_khr_byte_addressable_store cl_khr_gl_sharing cl_ext_device_fission cl_amd_device_attribute_query cl_amd_printf"
#define WFVOPENCL_ICD_SUFFIX "pkt"
#ifdef __APPLE__
#	define WFVOPENCL_LLVM_DATA_LAYOUT_64 "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
#else
#	define WFVOPENCL_LLVM_DATA_LAYOUT_64 "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-f80:128:128-n8:16:32:64"
#endif
#define WFVOPENCL_ADDRESS_BITS 32
#define WFVOPENCL_MAX_WORK_GROUP_SIZE 100000//8192
#define WFVOPENCL_MAX_NUM_DIMENSIONS 3

#ifdef WFVOPENCL_USE_AVX
	#undef WFVOPENCL_USE_SSE41
	#define WFVOPENCL_SIMD_WIDTH 8
#else
	#define WFVOPENCL_USE_SSE41
	#define WFVOPENCL_SIMD_WIDTH 4
#endif

#ifdef WFVOPENCL_USE_OPENMP // TODO: #ifdef _OPENMP
	#ifndef WFVOPENCL_NUM_CORES // can be supplied by build script
		#define WFVOPENCL_NUM_CORES 4 // TODO: determine somehow, omp_get_num_threads() does not work because it is dynamic (=1 if called here)
	#endif
#else
	#define WFVOPENCL_NUM_CORES 1
#endif
#define WFVOPENCL_MAX_NUM_THREADS WFVOPENCL_NUM_CORES*2 // *4 is too much for FloydWarshall (up to 50% slower than *2), NUM_CORES only is not enough (execution times very unstable for some kernels)
	// 5 threads: SimpleConvolution works with 2048/2048/3, segfaults starting somewhere above
	// 8 threads: SimpleConvolution works with 2048/x/3, where x can be as high as 32k (probably higher), 2048 for width is max (segfault above)
	// 5/8 threads: PrefixSum sometimes succeeds, sometimes fails
	// 5/8 threads: Dwt works up to 65536, segfaults above


// these defines are assumed to be set via build script:
//#define WFVOPENCL_NO_WFV
//#define WFVOPENCL_USE_OPENMP
//#define WFVOPENCL_SPLIT_EVERYTHING
//#define WFVOPENCL_ENABLE_JIT_PROFILING
//#define NDEBUG
//----------------------------------------------------------------------------//


#ifdef WFVOPENCL_USE_OPENMP
#include <omp.h>
#endif

#include "debug.h"

#include "consts.h"

///////////////////////////////////////////////////////////////////////////
//             Packetized OpenCL Internal Data Structures                //
///////////////////////////////////////////////////////////////////////////

struct _cl_icd_dispatch
{
	CL_API_ENTRY cl_int (CL_API_CALL *clGetPlatformIDs)(
			cl_uint          num_entries,
			cl_platform_id * platforms,
			cl_uint *        num_platforms) CL_API_SUFFIX__VERSION_1_0;

	CL_API_ENTRY cl_int (CL_API_CALL *clGetPlatformInfo)(
			cl_platform_id   platform,
			cl_platform_info param_name,
			size_t           param_value_size,
			void *           param_value,
			size_t *         param_value_size_ret) CL_API_SUFFIX__VERSION_1_0;

	CL_API_ENTRY cl_int (CL_API_CALL *clGetDeviceIDs)(
			cl_platform_id   platform,
			cl_device_type   device_type,
			cl_uint          num_entries,
			cl_device_id *   devices,
			cl_uint *        num_devices) CL_API_SUFFIX__VERSION_1_0;

	CL_API_ENTRY cl_int (CL_API_CALL *clGetDeviceInfo)(
			cl_device_id    device,
			cl_device_info  param_name,
			size_t          param_value_size,
			void *          param_value,
			size_t *        param_value_size_ret) CL_API_SUFFIX__VERSION_1_0;

	CL_API_ENTRY cl_context (CL_API_CALL *clCreateContext)(
			const cl_context_properties * properties,
			cl_uint                       num_devices,
			const cl_device_id *          devices,
			void (CL_CALLBACK *           pfn_notify)(const char *, const void *, size_t, void *),
			void *                        user_data,
			cl_int *                      errcode_ret) CL_API_SUFFIX__VERSION_1_0;

	CL_API_ENTRY cl_context (CL_API_CALL *clCreateContextFromType)(
			const cl_context_properties * properties,
			cl_device_type                device_type,
			void (CL_CALLBACK *           pfn_notify)(const char *, const void *, size_t, void *),
			void *                        user_data,
			cl_int *                      errcode_ret) CL_API_SUFFIX__VERSION_1_0;

	CL_API_ENTRY cl_int (CL_API_CALL *clRetainContext)(
			cl_context /* context */) CL_API_SUFFIX__VERSION_1_0;

	CL_API_ENTRY cl_int (CL_API_CALL *clReleaseContext)(
			cl_context /* context */) CL_API_SUFFIX__VERSION_1_0;

	CL_API_ENTRY cl_int (CL_API_CALL *clGetContextInfo)(
			cl_context         /* context */,
			cl_context_info    /* param_name */,
			size_t             /* param_value_size */,
			void *             /* param_value */,
			size_t *           /* param_value_size_ret */) CL_API_SUFFIX__VERSION_1_0;

	/* Command Queue APIs */
	CL_API_ENTRY cl_command_queue (CL_API_CALL *clCreateCommandQueue)(
			cl_context                     /* context */,
			cl_device_id                   /* device */,
			cl_command_queue_properties    /* properties */,
			cl_int *                       /* errcode_ret */) CL_API_SUFFIX__VERSION_1_0;

	CL_API_ENTRY cl_int (CL_API_CALL *clRetainCommandQueue)(
			cl_command_queue /* command_queue */) CL_API_SUFFIX__VERSION_1_0;

	CL_API_ENTRY cl_int (CL_API_CALL *clReleaseCommandQueue)(
			cl_command_queue /* command_queue */) CL_API_SUFFIX__VERSION_1_0;

	CL_API_ENTRY cl_int (CL_API_CALL *clGetCommandQueueInfo)(
			cl_command_queue      /* command_queue */,
			cl_command_queue_info /* param_name */,
			size_t                /* param_value_size */,
			void *                /* param_value */,
			size_t *              /* param_value_size_ret */) CL_API_SUFFIX__VERSION_1_0;

//#ifdef CL_USE_DEPRECATED_OPENCL_1_0_APIS
//#warning CL_USE_DEPRECATED_OPENCL_1_0_APIS is defined. These APIs are unsupported and untested in OpenCL 1.1!
	/*
	 *  WARNING:
	 *     This API introduces mutable state into the OpenCL implementation. It has been REMOVED
	 *  to better facilitate thread safety.  The 1.0 API is not thread safe. It is not tested by the
	 *  OpenCL 1.1 conformance test, and consequently may not work or may not work dependably.
	 *  It is likely to be non-performant. Use of this API is not advised. Use at your own risk.
	 *
	 *  Software developers previously relying on this API are instructed to set the command queue
	 *  properties when creating the queue, instead.
	 */
	CL_API_ENTRY cl_int (CL_API_CALL *clSetCommandQueueProperty)(
			cl_command_queue              /* command_queue */,
			cl_command_queue_properties   /* properties */,
			cl_bool                        /* enable */,
			cl_command_queue_properties * /* old_properties */) CL_EXT_SUFFIX__VERSION_1_0_DEPRECATED;
//#endif /* CL_USE_DEPRECATED_OPENCL_1_0_APIS */

	/* Memory Object APIs */
	CL_API_ENTRY cl_mem (CL_API_CALL *clCreateBuffer)(
			cl_context   /* context */,
			cl_mem_flags /* flags */,
			size_t       /* size */,
			void *       /* host_ptr */,
			cl_int *     /* errcode_ret */) CL_API_SUFFIX__VERSION_1_0;

#ifdef CL_VERSION_1_1_DECLARED_IN_ORDER
	CL_API_ENTRY cl_mem (CL_API_CALL *clCreateSubBuffer)(
			cl_mem                   /* buffer */,
			cl_mem_flags             /* flags */,
			cl_buffer_create_type    /* buffer_create_type */,
			const void *             /* buffer_create_info */,
			cl_int *                 /* errcode_ret */) CL_API_SUFFIX__VERSION_1_1;
#endif

	CL_API_ENTRY cl_mem (CL_API_CALL *clCreateImage2D)(
			cl_context              /* context */,
			cl_mem_flags            /* flags */,
			const cl_image_format * /* image_format */,
			size_t                  /* image_width */,
			size_t                  /* image_height */,
			size_t                  /* image_row_pitch */,
			void *                  /* host_ptr */,
			cl_int *                /* errcode_ret */) CL_API_SUFFIX__VERSION_1_0;

	CL_API_ENTRY cl_mem (CL_API_CALL *clCreateImage3D)(
			cl_context              /* context */,
			cl_mem_flags            /* flags */,
			const cl_image_format * /* image_format */,
			size_t                  /* image_width */,
			size_t                  /* image_height */,
			size_t                  /* image_depth */,
			size_t                  /* image_row_pitch */,
			size_t                  /* image_slice_pitch */,
			void *                  /* host_ptr */,
			cl_int *                /* errcode_ret */) CL_API_SUFFIX__VERSION_1_0;

	CL_API_ENTRY cl_int (CL_API_CALL *clRetainMemObject)(
			cl_mem /* memobj */) CL_API_SUFFIX__VERSION_1_0;

	CL_API_ENTRY cl_int (CL_API_CALL *clReleaseMemObject)(
			cl_mem /* memobj */) CL_API_SUFFIX__VERSION_1_0;

	CL_API_ENTRY cl_int (CL_API_CALL *clGetSupportedImageFormats)(
			cl_context           /* context */,
			cl_mem_flags         /* flags */,
			cl_mem_object_type   /* image_type */,
			cl_uint              /* num_entries */,
			cl_image_format *    /* image_formats */,
			cl_uint *            /* num_image_formats */) CL_API_SUFFIX__VERSION_1_0;

	CL_API_ENTRY cl_int (CL_API_CALL *clGetMemObjectInfo)(
			cl_mem           /* memobj */,
			cl_mem_info      /* param_name */,
			size_t           /* param_value_size */,
			void *           /* param_value */,
			size_t *         /* param_value_size_ret */) CL_API_SUFFIX__VERSION_1_0;

	CL_API_ENTRY cl_int (CL_API_CALL *clGetImageInfo)(
			cl_mem           /* image */,
			cl_image_info    /* param_name */,
			size_t           /* param_value_size */,
			void *           /* param_value */,
			size_t *         /* param_value_size_ret */) CL_API_SUFFIX__VERSION_1_0;

#ifdef CL_VERSION_1_1_DECLARED_IN_ORDER
	CL_API_ENTRY cl_int (CL_API_CALL *clSetMemObjectDestructorCallback)(
			cl_mem /* memobj */,
			void (CL_CALLBACK * /*pfn_notify*/)( cl_mem /* memobj */, void* /*user_data*/),
			void * /*user_data */ )             CL_API_SUFFIX__VERSION_1_1;
#endif

	/* Sampler APIs  */
	CL_API_ENTRY cl_sampler (CL_API_CALL *clCreateSampler)(
			cl_context          /* context */,
			cl_bool             /* normalized_coords */,
			cl_addressing_mode  /* addressing_mode */,
			cl_filter_mode      /* filter_mode */,
			cl_int *            /* errcode_ret */) CL_API_SUFFIX__VERSION_1_0;

	CL_API_ENTRY cl_int (CL_API_CALL *clRetainSampler)(
			cl_sampler /* sampler */) CL_API_SUFFIX__VERSION_1_0;

	CL_API_ENTRY cl_int (CL_API_CALL *clReleaseSampler)(
			cl_sampler /* sampler */) CL_API_SUFFIX__VERSION_1_0;

	CL_API_ENTRY cl_int (CL_API_CALL *clGetSamplerInfo)(
			cl_sampler         /* sampler */,
			cl_sampler_info    /* param_name */,
			size_t             /* param_value_size */,
			void *             /* param_value */,
			size_t *           /* param_value_size_ret */) CL_API_SUFFIX__VERSION_1_0;

	/* Program Object APIs  */
	CL_API_ENTRY cl_program (CL_API_CALL *clCreateProgramWithSource)(
			cl_context        /* context */,
			cl_uint           /* count */,
			const char **     /* strings */,
			const size_t *    /* lengths */,
			cl_int *          /* errcode_ret */) CL_API_SUFFIX__VERSION_1_0;

	CL_API_ENTRY cl_program (CL_API_CALL *clCreateProgramWithBinary)(
			cl_context                     /* context */,
			cl_uint                        /* num_devices */,
			const cl_device_id *           /* device_list */,
			const size_t *                 /* lengths */,
			const unsigned char **         /* binaries */,
			cl_int *                       /* binary_status */,
			cl_int *                       /* errcode_ret */) CL_API_SUFFIX__VERSION_1_0;

	CL_API_ENTRY cl_int (CL_API_CALL *clRetainProgram)(
			cl_program /* program */) CL_API_SUFFIX__VERSION_1_0;

	CL_API_ENTRY cl_int (CL_API_CALL *clReleaseProgram)(
			cl_program /* program */) CL_API_SUFFIX__VERSION_1_0;

	CL_API_ENTRY cl_int (CL_API_CALL *clBuildProgram)(
			cl_program           /* program */,
			cl_uint              /* num_devices */,
			const cl_device_id * /* device_list */,
			const char *         /* options */,
			void (CL_CALLBACK *  /* pfn_notify */)(cl_program /* program */, void * /* user_data */),
			void *               /* user_data */) CL_API_SUFFIX__VERSION_1_0;

	CL_API_ENTRY cl_int (CL_API_CALL *clUnloadCompiler)(void) CL_API_SUFFIX__VERSION_1_0;

	CL_API_ENTRY cl_int (CL_API_CALL *clGetProgramInfo)(
			cl_program         /* program */,
			cl_program_info    /* param_name */,
			size_t             /* param_value_size */,
			void *             /* param_value */,
			size_t *           /* param_value_size_ret */) CL_API_SUFFIX__VERSION_1_0;

	CL_API_ENTRY cl_int (CL_API_CALL *clGetProgramBuildInfo)(
			cl_program            /* program */,
			cl_device_id          /* device */,
			cl_program_build_info /* param_name */,
			size_t                /* param_value_size */,
			void *                /* param_value */,
			size_t *              /* param_value_size_ret */) CL_API_SUFFIX__VERSION_1_0;

	/* Kernel Object APIs */
	CL_API_ENTRY cl_kernel (CL_API_CALL *clCreateKernel)(
			cl_program      /* program */,
			const char *    /* kernel_name */,
			cl_int *        /* errcode_ret */) CL_API_SUFFIX__VERSION_1_0;

	CL_API_ENTRY cl_int (CL_API_CALL *clCreateKernelsInProgram)(
			cl_program     /* program */,
			cl_uint        /* num_kernels */,
			cl_kernel *    /* kernels */,
			cl_uint *      /* num_kernels_ret */) CL_API_SUFFIX__VERSION_1_0;

	CL_API_ENTRY cl_int (CL_API_CALL *clRetainKernel)(
			cl_kernel    /* kernel */) CL_API_SUFFIX__VERSION_1_0;

	CL_API_ENTRY cl_int (CL_API_CALL *clReleaseKernel)(
			cl_kernel   /* kernel */) CL_API_SUFFIX__VERSION_1_0;

	CL_API_ENTRY cl_int (CL_API_CALL *clSetKernelArg)(
			cl_kernel    /* kernel */,
			cl_uint      /* arg_index */,
			size_t       /* arg_size */,
			const void * /* arg_value */) CL_API_SUFFIX__VERSION_1_0;

	CL_API_ENTRY cl_int (CL_API_CALL *clGetKernelInfo)(
			cl_kernel       /* kernel */,
			cl_kernel_info  /* param_name */,
			size_t          /* param_value_size */,
			void *          /* param_value */,
			size_t *        /* param_value_size_ret */) CL_API_SUFFIX__VERSION_1_0;

	CL_API_ENTRY cl_int (CL_API_CALL *clGetKernelWorkGroupInfo)(
			cl_kernel                  /* kernel */,
			cl_device_id               /* device */,
			cl_kernel_work_group_info  /* param_name */,
			size_t                     /* param_value_size */,
			void *                     /* param_value */,
			size_t *                   /* param_value_size_ret */) CL_API_SUFFIX__VERSION_1_0;

	/* Event Object APIs  */
	CL_API_ENTRY cl_int (CL_API_CALL *clWaitForEvents)(
			cl_uint             /* num_events */,
			const cl_event *    /* event_list */) CL_API_SUFFIX__VERSION_1_0;

	CL_API_ENTRY cl_int (CL_API_CALL *clGetEventInfo)(
			cl_event         /* event */,
			cl_event_info    /* param_name */,
			size_t           /* param_value_size */,
			void *           /* param_value */,
			size_t *         /* param_value_size_ret */) CL_API_SUFFIX__VERSION_1_0;

#ifdef CL_VERSION_1_1_DECLARED_IN_ORDER
	CL_API_ENTRY cl_event (CL_API_CALL *clCreateUserEvent)(
			cl_context    /* context */,
			cl_int *      /* errcode_ret */) CL_API_SUFFIX__VERSION_1_1;
#endif

	CL_API_ENTRY cl_int (CL_API_CALL *clRetainEvent)(
			cl_event /* event */) CL_API_SUFFIX__VERSION_1_0;

	CL_API_ENTRY cl_int (CL_API_CALL *clReleaseEvent)(
			cl_event /* event */) CL_API_SUFFIX__VERSION_1_0;

#ifdef CL_VERSION_1_1_DECLARED_IN_ORDER
	CL_API_ENTRY cl_int (CL_API_CALL *clSetUserEventStatus)(
			cl_event   /* event */,
			cl_int     /* execution_status */) CL_API_SUFFIX__VERSION_1_1;
#endif

#ifdef CL_VERSION_1_1_DECLARED_IN_ORDER
	CL_API_ENTRY cl_int (CL_API_CALL *clSetEventCallback)(
			cl_event    /* event */,
			cl_int      /* command_exec_callback_type */,
			void (CL_CALLBACK * /* pfn_notify */)(cl_event, cl_int, void *),
			void *      /* user_data */) CL_API_SUFFIX__VERSION_1_1;
#endif

	/* Profiling APIs  */
	CL_API_ENTRY cl_int (CL_API_CALL *clGetEventProfilingInfo)(
			cl_event            /* event */,
			cl_profiling_info   /* param_name */,
			size_t              /* param_value_size */,
			void *              /* param_value */,
			size_t *            /* param_value_size_ret */) CL_API_SUFFIX__VERSION_1_0;

	/* Flush and Finish APIs */
	CL_API_ENTRY cl_int (CL_API_CALL *clFlush)(
			cl_command_queue /* command_queue */) CL_API_SUFFIX__VERSION_1_0;

	CL_API_ENTRY cl_int (CL_API_CALL *clFinish)(
			cl_command_queue /* command_queue */) CL_API_SUFFIX__VERSION_1_0;

	/* Enqueued Commands APIs */
	CL_API_ENTRY cl_int (CL_API_CALL *clEnqueueReadBuffer)(
			cl_command_queue    /* command_queue */,
			cl_mem              /* buffer */,
			cl_bool             /* blocking_read */,
			size_t              /* offset */,
			size_t              /* cb */,
			void *              /* ptr */,
			cl_uint             /* num_events_in_wait_list */,
			const cl_event *    /* event_wait_list */,
			cl_event *          /* event */) CL_API_SUFFIX__VERSION_1_0;

#ifdef CL_VERSION_1_1_DECLARED_IN_ORDER
	CL_API_ENTRY cl_int (CL_API_CALL *clEnqueueReadBufferRect)(
			cl_command_queue    /* command_queue */,
			cl_mem              /* buffer */,
			cl_bool             /* blocking_read */,
			const size_t *      /* buffer_origin */,
			const size_t *      /* host_origin */,
			const size_t *      /* region */,
			size_t              /* buffer_row_pitch */,
			size_t              /* buffer_slice_pitch */,
			size_t              /* host_row_pitch */,
			size_t              /* host_slice_pitch */,
			void *              /* ptr */,
			cl_uint             /* num_events_in_wait_list */,
			const cl_event *    /* event_wait_list */,
			cl_event *          /* event */) CL_API_SUFFIX__VERSION_1_1;
#endif

	CL_API_ENTRY cl_int (CL_API_CALL *clEnqueueWriteBuffer)(
			cl_command_queue   /* command_queue */,
			cl_mem             /* buffer */,
			cl_bool            /* blocking_write */,
			size_t             /* offset */,
			size_t             /* cb */,
			const void *       /* ptr */,
			cl_uint            /* num_events_in_wait_list */,
			const cl_event *   /* event_wait_list */,
			cl_event *         /* event */) CL_API_SUFFIX__VERSION_1_0;

#ifdef CL_VERSION_1_1_DECLARED_IN_ORDER
	CL_API_ENTRY cl_int (CL_API_CALL *clEnqueueWriteBufferRect)(
			cl_command_queue    /* command_queue */,
			cl_mem              /* buffer */,
			cl_bool             /* blocking_write */,
			const size_t *      /* buffer_origin */,
			const size_t *      /* host_origin */,
			const size_t *      /* region */,
			size_t              /* buffer_row_pitch */,
			size_t              /* buffer_slice_pitch */,
			size_t              /* host_row_pitch */,
			size_t              /* host_slice_pitch */,
			const void *        /* ptr */,
			cl_uint             /* num_events_in_wait_list */,
			const cl_event *    /* event_wait_list */,
			cl_event *          /* event */) CL_API_SUFFIX__VERSION_1_1;
#endif

	CL_API_ENTRY cl_int (CL_API_CALL *clEnqueueCopyBuffer)(
			cl_command_queue    /* command_queue */,
			cl_mem              /* src_buffer */,
			cl_mem              /* dst_buffer */,
			size_t              /* src_offset */,
			size_t              /* dst_offset */,
			size_t              /* cb */,
			cl_uint             /* num_events_in_wait_list */,
			const cl_event *    /* event_wait_list */,
			cl_event *          /* event */) CL_API_SUFFIX__VERSION_1_0;

#ifdef CL_VERSION_1_1_DECLARED_IN_ORDER
	CL_API_ENTRY cl_int (CL_API_CALL *clEnqueueCopyBufferRect)(
			cl_command_queue    /* command_queue */,
			cl_mem              /* src_buffer */,
			cl_mem              /* dst_buffer */,
			const size_t *      /* src_origin */,
			const size_t *      /* dst_origin */,
			const size_t *      /* region */,
			size_t              /* src_row_pitch */,
			size_t              /* src_slice_pitch */,
			size_t              /* dst_row_pitch */,
			size_t              /* dst_slice_pitch */,
			cl_uint             /* num_events_in_wait_list */,
			const cl_event *    /* event_wait_list */,
			cl_event *          /* event */) CL_API_SUFFIX__VERSION_1_1;
#endif

	CL_API_ENTRY cl_int (CL_API_CALL *clEnqueueReadImage)(
			cl_command_queue     /* command_queue */,
			cl_mem               /* image */,
			cl_bool              /* blocking_read */,
			const size_t *       /* origin[3] */,
			const size_t *       /* region[3] */,
			size_t               /* row_pitch */,
			size_t               /* slice_pitch */,
			void *               /* ptr */,
			cl_uint              /* num_events_in_wait_list */,
			const cl_event *     /* event_wait_list */,
			cl_event *           /* event */) CL_API_SUFFIX__VERSION_1_0;

	CL_API_ENTRY cl_int (CL_API_CALL *clEnqueueWriteImage)(
			cl_command_queue    /* command_queue */,
			cl_mem              /* image */,
			cl_bool             /* blocking_write */,
			const size_t *      /* origin[3] */,
			const size_t *      /* region[3] */,
			size_t              /* input_row_pitch */,
			size_t              /* input_slice_pitch */,
			const void *        /* ptr */,
			cl_uint             /* num_events_in_wait_list */,
			const cl_event *    /* event_wait_list */,
			cl_event *          /* event */) CL_API_SUFFIX__VERSION_1_0;

	CL_API_ENTRY cl_int (CL_API_CALL *clEnqueueCopyImage)(
			cl_command_queue     /* command_queue */,
			cl_mem               /* src_image */,
			cl_mem               /* dst_image */,
			const size_t *       /* src_origin[3] */,
			const size_t *       /* dst_origin[3] */,
			const size_t *       /* region[3] */,
			cl_uint              /* num_events_in_wait_list */,
			const cl_event *     /* event_wait_list */,
			cl_event *           /* event */) CL_API_SUFFIX__VERSION_1_0;

	CL_API_ENTRY cl_int (CL_API_CALL *clEnqueueCopyImageToBuffer)(
			cl_command_queue /* command_queue */,
			cl_mem           /* src_image */,
			cl_mem           /* dst_buffer */,
			const size_t *   /* src_origin[3] */,
			const size_t *   /* region[3] */,
			size_t           /* dst_offset */,
			cl_uint          /* num_events_in_wait_list */,
			const cl_event * /* event_wait_list */,
			cl_event *       /* event */) CL_API_SUFFIX__VERSION_1_0;

	CL_API_ENTRY cl_int (CL_API_CALL *clEnqueueCopyBufferToImage)(
			cl_command_queue /* command_queue */,
			cl_mem           /* src_buffer */,
			cl_mem           /* dst_image */,
			size_t           /* src_offset */,
			const size_t *   /* dst_origin[3] */,
			const size_t *   /* region[3] */,
			cl_uint          /* num_events_in_wait_list */,
			const cl_event * /* event_wait_list */,
			cl_event *       /* event */) CL_API_SUFFIX__VERSION_1_0;

	CL_API_ENTRY void * (CL_API_CALL *clEnqueueMapBuffer)(
			cl_command_queue /* command_queue */,
			cl_mem           /* buffer */,
			cl_bool          /* blocking_map */,
			cl_map_flags     /* map_flags */,
			size_t           /* offset */,
			size_t           /* cb */,
			cl_uint          /* num_events_in_wait_list */,
			const cl_event * /* event_wait_list */,
			cl_event *       /* event */,
			cl_int *         /* errcode_ret */) CL_API_SUFFIX__VERSION_1_0;

	CL_API_ENTRY void * (CL_API_CALL *clEnqueueMapImage)(
			cl_command_queue  /* command_queue */,
			cl_mem            /* image */,
			cl_bool           /* blocking_map */,
			cl_map_flags      /* map_flags */,
			const size_t *    /* origin[3] */,
			const size_t *    /* region[3] */,
			size_t *          /* image_row_pitch */,
			size_t *          /* image_slice_pitch */,
			cl_uint           /* num_events_in_wait_list */,
			const cl_event *  /* event_wait_list */,
			cl_event *        /* event */,
			cl_int *          /* errcode_ret */) CL_API_SUFFIX__VERSION_1_0;

	CL_API_ENTRY cl_int (CL_API_CALL *clEnqueueUnmapMemObject)(
			cl_command_queue /* command_queue */,
			cl_mem           /* memobj */,
			void *           /* mapped_ptr */,
			cl_uint          /* num_events_in_wait_list */,
			const cl_event *  /* event_wait_list */,
			cl_event *        /* event */) CL_API_SUFFIX__VERSION_1_0;

	CL_API_ENTRY cl_int (CL_API_CALL *clEnqueueNDRangeKernel)(
			cl_command_queue /* command_queue */,
			cl_kernel        /* kernel */,
			cl_uint          /* work_dim */,
			const size_t *   /* global_work_offset */,
			const size_t *   /* global_work_size */,
			const size_t *   /* local_work_size */,
			cl_uint          /* num_events_in_wait_list */,
			const cl_event * /* event_wait_list */,
			cl_event *       /* event */) CL_API_SUFFIX__VERSION_1_0;

	CL_API_ENTRY cl_int (CL_API_CALL *clEnqueueTask)(
			cl_command_queue  /* command_queue */,
			cl_kernel         /* kernel */,
			cl_uint           /* num_events_in_wait_list */,
			const cl_event *  /* event_wait_list */,
			cl_event *        /* event */) CL_API_SUFFIX__VERSION_1_0;

	CL_API_ENTRY cl_int (CL_API_CALL *clEnqueueNativeKernel)(
			cl_command_queue  /* command_queue */,
			void (*user_func)(void *),
			void *            /* args */,
			size_t            /* cb_args */,
			cl_uint           /* num_mem_objects */,
			const cl_mem *    /* mem_list */,
			const void **     /* args_mem_loc */,
			cl_uint           /* num_events_in_wait_list */,
			const cl_event *  /* event_wait_list */,
			cl_event *        /* event */) CL_API_SUFFIX__VERSION_1_0;

	CL_API_ENTRY cl_int (CL_API_CALL *clEnqueueMarker)(
			cl_command_queue    /* command_queue */,
			cl_event *          /* event */) CL_API_SUFFIX__VERSION_1_0;

	CL_API_ENTRY cl_int (CL_API_CALL *clEnqueueWaitForEvents)(
			cl_command_queue /* command_queue */,
			cl_uint          /* num_events */,
			const cl_event * /* event_list */) CL_API_SUFFIX__VERSION_1_0;

	CL_API_ENTRY cl_int (CL_API_CALL *clEnqueueBarrier)(
			cl_command_queue /* command_queue */) CL_API_SUFFIX__VERSION_1_0;

	/* Extension function access
	 *
	 * Returns the extension function address for the given function name,
	 * or NULL if a valid function can not be found.  The client must
	 * check to make sure the address is not NULL, before using or
	 * calling the returned function address.
	 */
	CL_API_ENTRY void * (CL_API_CALL *clGetExtensionFunctionAddress)(
			const char * /* func_name */) CL_API_SUFFIX__VERSION_1_0;
};

static _cl_icd_dispatch static_dispatch =
{
	clGetPlatformIDs,
	clGetPlatformInfo,
	clGetDeviceIDs,
	clGetDeviceInfo,
	clCreateContext,
	clCreateContextFromType,
	clRetainContext,
	clReleaseContext,
	clGetContextInfo,
	clCreateCommandQueue,
	clRetainCommandQueue,
	clReleaseCommandQueue,
	clGetCommandQueueInfo,
//#ifdef CL_USE_DEPRECATED_OPENCL_1_0_APIS
	clSetCommandQueueProperty,
//#endif /* CL_USE_DEPRECATED_OPENCL_1_0_APIS */
	clCreateBuffer,
#ifdef CL_VERSION_1_1_DECLARED_IN_ORDER
	clCreateSubBuffer,
#endif
	clCreateImage2D,
	clCreateImage3D,
	clRetainMemObject,
	clReleaseMemObject,
	clGetSupportedImageFormats,
	clGetMemObjectInfo,
	clGetImageInfo,
#ifdef CL_VERSION_1_1_DECLARED_IN_ORDER
	clSetMemObjectDestructorCallback,
#endif
	clCreateSampler,
	clRetainSampler,
	clReleaseSampler,
	clGetSamplerInfo,
	clCreateProgramWithSource,
	clCreateProgramWithBinary,
	clRetainProgram,
	clReleaseProgram,
	clBuildProgram,
	clUnloadCompiler,
	clGetProgramInfo,
	clGetProgramBuildInfo,
	clCreateKernel,
	clCreateKernelsInProgram,
	clRetainKernel,
	clReleaseKernel,
	clSetKernelArg,
	clGetKernelInfo,
	clGetKernelWorkGroupInfo,
	clWaitForEvents,
	clGetEventInfo,
#ifdef CL_VERSION_1_1_DECLARED_IN_ORDER
	clCreateUserEvent,
#endif
	clRetainEvent,
	clReleaseEvent,
#ifdef CL_VERSION_1_1_DECLARED_IN_ORDER
	clSetUserEventStatus,
#endif
#ifdef CL_VERSION_1_1_DECLARED_IN_ORDER
	clSetEventCallback,
#endif
	clGetEventProfilingInfo,
	clFlush,
	clFinish,
	clEnqueueReadBuffer,
#ifdef CL_VERSION_1_1_DECLARED_IN_ORDER
	clEnqueueReadBufferRect,
#endif
	clEnqueueWriteBuffer,
#ifdef CL_VERSION_1_1_DECLARED_IN_ORDER
	clEnqueueWriteBufferRect,
#endif
	clEnqueueCopyBuffer,
#ifdef CL_VERSION_1_1_DECLARED_IN_ORDER
	clEnqueueCopyBufferRect,
#endif
	clEnqueueReadImage,
	clEnqueueWriteImage,
	clEnqueueCopyImage,
	clEnqueueCopyImageToBuffer,
	clEnqueueCopyBufferToImage,
	clEnqueueMapBuffer,
	clEnqueueMapImage,
	clEnqueueUnmapMemObject,
	clEnqueueNDRangeKernel,
	clEnqueueTask,
	clEnqueueNativeKernel,
	clEnqueueMarker,
	clEnqueueWaitForEvents,
	clEnqueueBarrier,
	clGetExtensionFunctionAddress
};

struct _cl_platform_id { struct _cl_icd_dispatch* dispatch; };

static struct _cl_platform_id static_platform = { &static_dispatch };


struct _cl_device_id { struct _cl_icd_dispatch* dispatch; };

static struct _cl_device_id static_device = { &static_dispatch };

/*
An OpenCL context is created with one or more devices. Contexts
are used by the OpenCL runtime for managing objects such as command-queues,
memory, program and kernel objects and for executing kernels on one or more
devices specified in the context.
*/
struct _cl_context { struct _cl_icd_dispatch* dispatch; };

/*
OpenCL objects such as memory, program and kernel objects are created using a
context.
Operations on these objects are performed using a command-queue. The
command-queue can be used to queue a set of operations (referred to as commands)
in order. Having multiple command-queues allows applications to queue multiple
independent commands without requiring synchronization. Note that this should
work as long as these objects are not being shared. Sharing of objects across
multiple command-queues will require the application to perform appropriate
synchronization. This is described in Appendix A.
*/
struct _cl_command_queue {
	struct _cl_icd_dispatch* dispatch;
	_cl_context* context;
};

/*
Memory objects are categorized into two types: buffer objects, and image
objects. A buffer object stores a one-dimensional collection of elements whereas
an image object is used to store a two- or three- dimensional texture,
frame-buffer or image.
Elements of a buffer object can be a scalar data type (such as an int, float),
vector data type, or a user-defined structure. An image object is used to
represent a buffer that can be used as a texture or a frame-buffer. The elements
of an image object are selected from a list of predefined image formats. The
minimum number of elements in a memory object is one.
*/
struct _cl_mem {
	struct _cl_icd_dispatch* dispatch;
private:
	_cl_context* context;
	size_t size; //entire size in bytes
	void* data;
	const bool canRead;
	const bool canWrite;
public:
	_cl_mem(_cl_context* ctx, size_t bytes, void* values, bool can_read, bool can_write)
			: dispatch(&static_dispatch), context(ctx), size(bytes), data(values), canRead(can_read), canWrite(can_write) {}

	inline _cl_context* get_context() const { return context; }
	inline void* get_data() const { return data; }
	inline size_t get_size() const { return size; }
	inline bool isReadOnly() const { return canRead && !canWrite; }
	inline bool isWriteOnly() const { return !canRead && canWrite; }

	inline void copy_data(
			const void* values,
			const size_t bytes,
			const size_t dst_offset=0,
			const size_t src_offset=0)
	{
		assert (bytes+dst_offset <= size);
		if (dst_offset == 0) memcpy(data, (char*)values+src_offset, bytes);
		else {
			for (cl_uint i=src_offset; i<bytes; ++i) {
				((char*)data)[i+dst_offset] = ((const char*)values)[i];
			}
		}
	}
};

/*
A sampler object describes how to sample an image when the image is read in the
kernel. The built-in functions to read from an image in a kernel take a sampler
as an argument. The sampler arguments to the image read function can be sampler
objects created using OpenCL functions and passed as argument values to the
kernel or can be samplers declared inside a kernel. In this section we discuss
how sampler objects are created using OpenCL functions.
*/
struct _cl_sampler {
	struct _cl_icd_dispatch* dispatch;
	_cl_context* context;
};

/*
An OpenCL program consists of a set of kernels that are identified as functions
declared with the __kernel qualifier in the program source. OpenCL programs may
also contain auxiliary functions and constant data that can be used by __kernel
functions. The program executable can be generated online or offline by the
OpenCL compiler for the appropriate target device(s).
A program object encapsulates the following information:
       An associated context.
       A program source or binary.
       The latest successfully built program executable, the list of devices for
       which the program executable is built, the build options used and a build
       log.
       The number of kernel objects currently attached.
*/
struct _cl_program {
	struct _cl_icd_dispatch* dispatch;
	_cl_context* context;
	const char* fileName;
	llvm::Module* module;
	llvm::TargetData* targetData;
};


struct _cl_kernel_arg {
private:
	// known at kernel creation time
	const size_t element_size; // size of one item in bytes
	const cl_uint address_space;
	void* mem_address; // values are inserted by kernel::set_arg_data()

	// only known after clSetKernelArg
	size_t size; // size of entire argument value

public:
	_cl_kernel_arg(
			const size_t _elem_size,
			const cl_uint _address_space,
			void* _mem_address,
			const size_t _size=0)
		: element_size(_elem_size),
		address_space(_address_space),
		mem_address(_mem_address),
		size(_size)
	{}

	inline void set_size(size_t _size) { size = _size; }

	inline size_t get_size() const { return size; }
	inline size_t get_element_size() const { return element_size; }
	inline cl_uint get_address_space() const { return address_space; }
	inline void* get_mem_address() const { return mem_address; } // must not assert (data) -> can be 0 if non-pointer type (e.g. float)
};

/*
A kernel is a function declared in a program. A kernel is identified by the
__kernel qualifier applied to any function in a program. A kernel object
encapsulates the specific __kernel function declared in a program and the
argument values to be used when executing this __kernel function.
*/
struct _cl_kernel {
	struct _cl_icd_dispatch* dispatch;
private:
	_cl_context* context;
	_cl_program* program;
	const void* compiled_function;

	const cl_uint num_args;
	std::vector<_cl_kernel_arg*> args;

	void* argument_struct;
	size_t argument_struct_size;

	cl_uint num_dimensions;
	cl_uint best_simd_dim;

public:
	_cl_kernel(_cl_context* ctx, _cl_program* prog, llvm::Function* f,
			llvm::Function* f_wrapper, llvm::Function* f_SIMD=NULL)
		: dispatch(&static_dispatch), context(ctx), program(prog), compiled_function(NULL), num_args(WFVOpenCL::getNumArgs(f)), args(num_args),
		argument_struct(NULL), argument_struct_size(0), num_dimensions(0), best_simd_dim(0),
		function(f), function_wrapper(f_wrapper), function_SIMD(f_SIMD)
	{
		WFVOPENCL_DEBUG( outs() << "  creating kernel object... \n"; );
		assert (ctx && prog && f && f_wrapper);

		// compile wrapper function (to be called in clEnqueueNDRangeKernel())
		// NOTE: be sure that f_SIMD or f are inlined and f_wrapper was optimized to the max :p
		WFVOPENCL_DEBUG( outs() << "    compiling function '" << f_wrapper->getNameStr() << "'... "; );
		WFVOPENCL_DEBUG( verifyModule(*prog->module); );
		WFVOPENCL_DEBUG( WFVOpenCL::writeModuleToFile(prog->module, "debug_kernel_final_before_compilation.mod.ll"); );
//		prog->module = WFVOpenCL::createModuleFromFile("KERNELTEST.bc");
//		f_wrapper = prog->module->getFunction(f_wrapper->getNameStr());
//		f = prog->module->getFunction(f->getNameStr());
//		f_wrapper->viewCFG();
#if 0
		for (Function::iterator BB=f_wrapper->begin(), BBE=f_wrapper->end(); BB!=BBE; ++BB) {
			for (BasicBlock::iterator I=BB->begin(), IE=BB->end(); I!=IE; ++I) {
				if (isa<FPToUIInst>(I)) {
					Value* castedVal = I->getOperand(0);
					for (Instruction::use_iterator U=I->use_begin(), UE=I->use_end(); U!=UE; ++U) {
						if (isa<UIToFPInst>(U)) {
							assert (U->getType() == castedVal->getType());
							U->replaceAllUsesWith(castedVal);
						}
					}
				}
				if (isa<UIToFPInst>(I)) {
					Value* castedVal = I->getOperand(0);
					for (Instruction::use_iterator U=I->use_begin(), UE=I->use_end(); U!=UE; ++U) {
						if (isa<FPToUIInst>(U)) {
							assert (U->getType() == castedVal->getType());
							U->replaceAllUsesWith(castedVal);
						}
					}
				}
			}
		}
#endif
		compiled_function = WFVOpenCL::getPointerToFunction(prog->module, f_wrapper);
		if (!compiled_function) {
			errs() << "\nERROR: JIT compilation of kernel function failed!\n";
		}
#ifdef WFVOPENCL_ENABLE_JIT_PROFILING
		iJIT_Method_Load ml;
		ml.method_id = iJIT_GetNewMethodID();
		const unsigned mnamesize = f_wrapper->getNameStr().size();
		char* mname = new char[mnamesize]();
		for (unsigned i=0; i<mnamesize; ++i) {
			mname[i] = f_wrapper->getNameStr().c_str()[i];
		}
		ml.method_name = mname;
		ml.method_load_address = const_cast<void*>(compiled_function);
		ml.method_size = 42;
		ml.line_number_size = 0;
		ml.line_number_table = NULL;
		ml.class_id = 0;
		ml.class_file_name = NULL;
		ml.source_file_name = NULL;
		iJIT_NotifyEvent(iJVM_EVENT_TYPE_METHOD_LOAD_FINISHED, (void*)&ml);
#endif
		WFVOPENCL_DEBUG( if (compiled_function) outs() << "done.\n"; );

		// get argument information
		WFVOPENCL_DEBUG( outs() << "    collecting argument information...\n"; );

		assert (num_args > 0); // TODO: don't we allow kernels without arguments? do they make sense?

		// determine size of each argument
		size_t max_elem_size = 0;
		for (cl_uint arg_index=0; arg_index<num_args; ++arg_index) {
			// get type of argument and corresponding size
			const llvm::Type* argType = WFVOpenCL::getArgumentType(f, arg_index);
			const size_t arg_size_bytes = WFVOpenCL::getTypeSizeInBits(program->targetData, argType) / 8;

			if (max_elem_size < arg_size_bytes) max_elem_size = arg_size_bytes;

			//outs() << "\nargument_struct_size: " << argument_struct_size << "\n";
			//outs() << "arg_size_bytes: " << arg_size_bytes << "\n";

			size_t gap_bytes = argument_struct_size % arg_size_bytes;
			if (gap_bytes != 0) argument_struct_size += arg_size_bytes - gap_bytes;

			//outs() << "after padding:\n";
			//outs() << "argument_struct_size: " << argument_struct_size << "\n";
			//outs() << "arg_size_bytes: " << arg_size_bytes << "\n";

			argument_struct_size += arg_size_bytes;
		}
		size_t gap_bytes = argument_struct_size % max_elem_size;
		if (gap_bytes != 0) argument_struct_size += max_elem_size - gap_bytes;

		// allocate memory for argument_struct
		// TODO: do we have to care about type padding?
		argument_struct = malloc(argument_struct_size);
		WFVOPENCL_DEBUG( outs() << "      size of argument-struct: " << argument_struct_size << " bytes\n"; );
		WFVOPENCL_DEBUG( outs() << "      address of argument-struct: " << argument_struct << "\n"; );
		WFVOPENCL_DEBUG(
			const llvm::Type* argType = WFVOpenCL::getArgumentType(f_wrapper, 0);
			outs() << "      LLVM type: " << *argType << "\n";
			const llvm::Type* sType = WFVOpenCL::getContainedType(argType, 0); // get size of struct, not of pointer to struct
			outs() << "      LLVM type size: " << WFVOpenCL::getTypeSizeInBits(prog->targetData, sType)/8 << "\n";
		);

		// create argument objects
		size_t current_size=0;
		for (cl_uint arg_index=0; arg_index<num_args; ++arg_index) {

			const llvm::Type* argType = WFVOpenCL::getArgumentType(f, arg_index);
			const size_t arg_size_bytes = WFVOpenCL::getTypeSizeInBits(program->targetData, argType) / 8;
			const cl_uint address_space = WFVOpenCL::convertLLVMAddressSpace(WFVOpenCL::getAddressSpace(argType));

			// if necessary, add padding
			size_t gap_bytes = current_size % arg_size_bytes;
			if (gap_bytes != 0) current_size += arg_size_bytes - gap_bytes;

			// save pointer to address of argument inside argument_struct
			void* arg_struct_addr = (char*)argument_struct + current_size;
			current_size += arg_size_bytes;

			WFVOPENCL_DEBUG( outs() << "      argument " << arg_index << "\n"; );
			WFVOPENCL_DEBUG( outs() << "        size     : " << arg_size_bytes << " bytes\n"; );
			WFVOPENCL_DEBUG( outs() << "        address  : " << (void*)arg_struct_addr << "\n"; );
			WFVOPENCL_DEBUG( outs() << "        addrspace: " << WFVOpenCL::getAddressSpaceString(address_space) << "\n"; );

			args[arg_index] = new _cl_kernel_arg(arg_size_bytes, address_space, arg_struct_addr);
		}

		WFVOPENCL_DEBUG( outs() << "  kernel object created successfully!\n\n"; );
	}

	~_cl_kernel() {
		args.clear();
		free(argument_struct);
	}

	const llvm::Function* function;
	const llvm::Function* function_wrapper;
	const llvm::Function* function_SIMD;

	// Copy 'arg_size' bytes from 'data' into argument_struct at the position
	// of argument at index 'arg_index'.
	// There are some possible issues with the type of data being copied:
	// We simply distinguish different address spaces to know whether the data
	// is actually a _cl_mem object, raw data, or a local pointer - all three
	// cases have to be treated differently:
	// _cl_mem** - CL_GLOBAL  - access the mem object and copy its data
	// raw data  - CL_PRIVATE - copy the data directly
	// local ptr - CL_LOCAL   - copy the pointer
	//
	// OpenCL Specification 1.0 for clSetKernelArg:
	// The argument data pointed to by arg_value is copied and the arg_value
	// pointer can therefore be reused by the application after clSetKernelArg returns.
	//
	// arg_size specifies the size of the argument value. If the argument is a memory object, the size is
	// the size of the buffer or image object type. For arguments declared with the __local qualifier,
	// the size specified will be the size in bytes of the buffer that must be allocated for the __local
	// argument.
	inline cl_uint set_arg_data(const cl_uint arg_index, const void* data, const size_t arg_size) {
		assert (arg_index < num_args);
		assert (args[arg_index] && "kernel object not completely initialized?");

		// store argument size
		args[arg_index]->set_size(arg_size);

		void* arg_pos = arg_get_data(arg_index); //((char*)argument_struct)+current_size;

		// NOTE: for pointers, we supply &data because we really want to copy the pointer!
		switch (arg_get_address_space(arg_index)) {
			case CL_GLOBAL: {
				assert (arg_size == sizeof(_cl_mem*)); // = sizeof(cl_mem)
				assert (data);
				// data is actually a _cl_mem* given by reference
				const _cl_mem* mem = *(const _cl_mem**)data;
				// copy the pointer, not what is pointed to
				//const void* datax = mem->get_data();
				//memcpy(arg_pos, &datax, arg_size);
				*(void**)arg_pos = mem->get_data();
				break;
			}
			case CL_PRIVATE: {
				assert (data);
				// copy the data itself
				memcpy(arg_pos, data, arg_size);
				break;
			}
			case CL_LOCAL: {
				assert (!data);
				// allocate memory of size 'arg_size' and copy the pointer
				//const void* datax = malloc(arg_size);
				//memcpy(arg_pos, &datax, sizeof(void*));
				*(void**)arg_pos = malloc(arg_size);
				break;
			}
			case CL_CONSTANT: {
				errs() << "ERROR: support for constant memory not implemented yet!\n";
				assert (false && "support for constant memory not implemented yet!");
				return CL_INVALID_VALUE; // sth like that :p
			}
			default: {
				errs() << "ERROR: unknown address space found: " << arg_get_address_space(arg_index) << "\n";
				assert (false && "unknown address space found!");
				return CL_INVALID_VALUE; // sth like that :p
			}
		}

		WFVOPENCL_DEBUG( outs() << "  data source: " << data << "\n"; );
		WFVOPENCL_DEBUG( outs() << "  target pointer: " << arg_pos << "\n"; );

		return CL_SUCCESS;
	}
	inline void set_num_dimensions(const cl_uint num_dim) { num_dimensions = num_dim; }
	inline void set_best_simd_dim(const cl_uint dim) { best_simd_dim = dim; }

	inline _cl_context* get_context() const { return context; }
	inline _cl_program* get_program() const { return program; }
	inline const void* get_compiled_function() const { return compiled_function; }
	inline cl_uint get_num_args() const { return num_args; }
	inline const void* get_argument_struct() const { return argument_struct; }
	inline size_t get_argument_struct_size() const { return argument_struct_size; }
	inline cl_uint get_num_dimensions() const { return num_dimensions; }
	inline cl_uint get_best_simd_dim() const { return best_simd_dim; }

	inline size_t arg_get_size(const cl_uint arg_index) const {
		assert (arg_index < num_args);
		assert (args[arg_index] && "kernel object not completely initialized?");
		return args[arg_index]->get_size();
	}
	inline size_t arg_get_element_size(const cl_uint arg_index) const {
		assert (arg_index < num_args);
		assert (args[arg_index] && "kernel object not completely initialized?");
		return args[arg_index]->get_element_size();
	}
	inline cl_uint arg_get_address_space(const cl_uint arg_index) const {
		assert (arg_index < num_args);
		assert (args[arg_index] && "kernel object not completely initialized?");
		return args[arg_index]->get_address_space();
	}
	inline bool arg_is_global(const cl_uint arg_index) const {
		assert (arg_index < num_args);
		assert (args[arg_index] && "kernel object not completely initialized?");
		return args[arg_index]->get_address_space() == CL_GLOBAL;
	}
	inline bool arg_is_local(const cl_uint arg_index) const {
		assert (arg_index < num_args);
		assert (args[arg_index] && "kernel object not completely initialized?");
		return args[arg_index]->get_address_space() == CL_LOCAL;
	}
	inline bool arg_is_private(const cl_uint arg_index) const {
		assert (arg_index < num_args);
		assert (args[arg_index] && "kernel object not completely initialized?");
		return args[arg_index]->get_address_space() == CL_PRIVATE;
	}
	inline bool arg_is_constant(const cl_uint arg_index) const {
		assert (arg_index < num_args);
		assert (args[arg_index] && "kernel object not completely initialized?");
		return args[arg_index]->get_address_space() == CL_CONSTANT;
	}
	inline void* arg_get_data(const cl_uint arg_index) const {
		assert (arg_index < num_args);
		assert (args[arg_index] && "kernel object not completely initialized?");
		return args[arg_index]->get_mem_address();
	}

};

struct _cl_event {
	struct _cl_icd_dispatch* dispatch;
	_cl_context* context;
};

#endif
