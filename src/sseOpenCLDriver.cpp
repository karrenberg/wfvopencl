/**
 * @file   sseOpenCLDriver.cpp
 * @date   14.04.2010
 * @author Ralf Karrenberg
 *
 * Copyright (C) 2010 Saarland University
 * Released under the GPL
 */
#include <assert.h>
#include <iostream>
#include <sstream> //stringstream

//llvm wrapper that provides all functionality needed
#include <jitRT/llvmWrapper.h>


#ifdef __APPLE__
#include <OpenCL/cl_platform.h>
#else
#include <CL/cl_platform.h> // e.g. for CL_API_ENTRY
#include <CL/cl.h> // e.g. for cl_platform_id
#endif

#ifdef __cplusplus
extern "C" {
#endif


///////////////////////////////////////////////////////////////////////////
//                 OpenCL Runtime Implementation                         //
///////////////////////////////////////////////////////////////////////////

#include <xmmintrin.h>
#include <string.h>
// a class 'OpenCLRuntime' would be nicer,
// but we cannot store address of member function to bitcode
namespace {

	// scalar implementation
	//
	static const cl_uint simdWidth = 4;

	cl_uint dimensions; // max 3
	size_t* globalThreads; // total # work items per dimension, arbitrary size
	size_t* localThreads;  // size of each work group per dimension

	size_t* currentGlobal; // 0 -> globalThreads[D] -1
	size_t* currentLocal;  // 0 -> SIMD width -1
	size_t* currentGroup;  // 0 -> (globalThreads[D] / localThreads[D]) -1

	void initializeOpenCL(const cl_uint dim) {
		assert (dim < 4 && "max # dimensions is 3!");
		dimensions = dim;

		globalThreads = new size_t[dim]();
		localThreads = new size_t[dim]();

		currentGlobal = new size_t[dim]();
		currentLocal = new size_t[dim]();
		currentGroup = new size_t[dim]();

		for (cl_uint i=0; i<dimensions; ++i) {
			currentGlobal[i] = 0;
			currentLocal[i] = 0;
			currentGroup[i] = 0;
		}
	}

	inline void initializeThreads(size_t* gThreads, size_t* lThreads) {
		for (cl_uint i=0; i<dimensions; ++i) {
			globalThreads[i] = gThreads[i];
			localThreads[i] = lThreads[i];
		}
	}

	// D is dimension index.

	/* Num. of dimensions in use */
	inline cl_uint get_work_dim() {
		return dimensions;
	}

	/* Num. of global work-items */
	inline size_t get_global_size(cl_uint D) {
		assert (D < dimensions);
		if (D >= dimensions) return 1;
		return globalThreads[D];
	}

	/* Global work-item ID value */
	inline size_t get_global_id(cl_uint D) {
		assert (D < dimensions);
		if (D >= dimensions) return CL_SUCCESS;
		return currentGlobal[D];
	}

	/* Num. of local work-items */
	inline size_t get_local_size(cl_uint D) {
		assert (D < dimensions);
		if (D >= dimensions) return 1;
		return localThreads[D];
	}

	/* Local work-item ID */
	inline size_t get_local_id(cl_uint D) {
		assert (D < dimensions);
		if (D >= dimensions) return CL_SUCCESS;
		return currentLocal[D];
	}

	/* Num. of work-groups */
	inline size_t get_num_groups(cl_uint D) {
		assert (D < dimensions);
		if (D >= dimensions) return 1;
		return globalThreads[D] / localThreads[D];
	}

	/* Returns the work-group ID */
	inline size_t get_group_id(cl_uint D) {
		assert (D < dimensions);
		if (D >= dimensions) return CL_SUCCESS;
		return currentGroup[D];
	}


	inline void setCurrentGlobal(cl_uint D, size_t id) {
		assert (D < dimensions);
		assert (id < get_global_size(D));
		currentGlobal[D] = id;
	}
	inline void setCurrentLocal(cl_uint D, size_t id) {
		assert (D < dimensions);
		assert (id < get_local_size(D));
		currentLocal[D] = id;
	}
	inline void setCurrentGroup(cl_uint D, size_t id) {
		assert (D < dimensions);
		assert (id < get_num_groups(D));
		currentGroup[D] = id;
	}


	// packetized implementations
	//
	__m128i* currentGlobal_SIMD; // 0 -> globalThreads[D] -1
	__m128i* currentLocal_SIMD;  // 0 -> SIMD width -1

	cl_uint simdDimension;


	// called automatically by initializeOpenCL_SIMD
	inline void initializeThreads_SIMD(size_t* gThreads, size_t* lThreads) {
		size_t globalThreadNum = 0;
		size_t localThreadNum = 0;
		bool* alignedGlobalDims = new bool[dimensions]();
		bool* alignedLocalDims = new bool[dimensions]();

		bool error = false;

		for (cl_uint i=0; i<dimensions; ++i) {
			const size_t globalThreadsDimI = gThreads[i];
			globalThreadNum += globalThreadsDimI;
			alignedGlobalDims[i] = (globalThreadsDimI % simdWidth == 0);
			globalThreads[i] = i == simdDimension ? (globalThreadsDimI / simdWidth) : globalThreadsDimI;

			const size_t localThreadsDimI = lThreads[i];
			localThreadNum += localThreadsDimI;
			alignedLocalDims[i] = (localThreadsDimI % simdWidth == 0);
			localThreads[i] = i == simdDimension ? (localThreadsDimI / simdWidth) : localThreadsDimI;

			if (i == simdDimension && !alignedGlobalDims[i]) {
				std::cerr << "ERROR: chosen SIMD dimension " << i << " is globally not dividable by " << simdWidth << " (global dimension)!\n";
				error = true;
			}
			if (i == simdDimension && !alignedLocalDims[i]) {
				std::cerr << "ERROR: chosen SIMD dimension " << i << " is locally not dividable by " << simdWidth << " (work-group dimension)!\n";
				error = true;
			}
			if (globalThreadsDimI % localThreadsDimI != 0) {
				std::cerr << "ERROR: global dimension " << i << " not dividable by local dimension (" << globalThreadsDimI << " / " << localThreadsDimI << ")!\n";
				error = true;
			}
		}
		if (globalThreadNum % simdWidth != 0) {
			std::cerr << "ERROR: global number of threads is not dividable by " << simdWidth << "!\n";
			error = true;
		}
		if (localThreadNum % simdWidth != 0) {
			std::cerr << "ERROR: number of threads in a group is not dividable by " << simdWidth << "!\n";
			error = true;
		}

		if (error) exit(-1);
	}

	void initializeOpenCL_SIMD(const cl_uint dims, const cl_uint simdDim, size_t* gThreads, size_t* lThreads) {
		if (dims > 3) {
			std::cerr << "ERROR: max # dimensions is 3!\n";
			exit(-1);
		}
		if (simdDim > dims) {
			std::cerr << "ERROR: chosen SIMD dimension out of bounds (" << simdWidth << " > " << dims << ")!\n";
			exit(-1);
		}
		dimensions = dims;
		simdDimension = simdDim-1; //-1 for array access

		globalThreads = new size_t[dimensions]();
		localThreads = new size_t[dimensions]();

		currentGlobal_SIMD = new __m128i[dimensions]();
		currentLocal_SIMD = new __m128i[dimensions]();
		currentGroup = new size_t[dimensions]();

		for (cl_uint i=0; i<dimensions; ++i) {
			if (i == simdDimension) {
				currentGlobal_SIMD[i] = _mm_set_epi32(0, 1, 2, 3);
				currentLocal_SIMD[i] = _mm_set_epi32(0, 1, 2, 3);
			} else {
				currentGlobal_SIMD[i] = _mm_set_epi32(0, 0, 0, 0);
				currentLocal_SIMD[i] = _mm_set_epi32(0, 0, 0, 0);
			}
			currentGroup[i] = 0;
		}

		initializeThreads_SIMD(gThreads, lThreads);
	}

	inline __m128i get_global_id_SIMD(cl_uint D) {
		assert (D < dimensions);
		return currentGlobal_SIMD[D];
	}
	inline __m128i get_local_id_SIMD(cl_uint D) {
		assert (D < dimensions);
		return currentLocal_SIMD[D];
	}

	inline void setCurrentGlobal_SIMD(cl_uint D, __m128i id) {
		assert (D < dimensions);
		assert (((size_t*)&id)[0] < get_global_size(D));
		assert (((size_t*)&id)[1] < get_global_size(D));
		assert (((size_t*)&id)[2] < get_global_size(D));
		assert (((size_t*)&id)[3] < get_global_size(D));
		currentGlobal_SIMD[D] = id;
	}
	inline void setCurrentLocal_SIMD(cl_uint D, __m128i id) {
		assert (D < dimensions);
		assert (((size_t*)&id)[0] < get_local_size(D));
		assert (((size_t*)&id)[1] < get_local_size(D));
		assert (((size_t*)&id)[2] < get_local_size(D));
		assert (((size_t*)&id)[3] < get_local_size(D));
		currentLocal_SIMD[D] = id;
	}


	void resolveRuntimeCalls(llvm::Module* mod) {
		std::vector< std::pair<llvm::Function*, void*> > funs;
		funs.push_back(std::make_pair(jitRT::getFunction("get_work_dim", mod), (void*)get_work_dim));
		funs.push_back(std::make_pair(jitRT::getFunction("get_global_size", mod), (void*)get_global_size));
		funs.push_back(std::make_pair(jitRT::getFunction("get_global_id", mod), (void*)get_global_id));
		funs.push_back(std::make_pair(jitRT::getFunction("get_local_size", mod), (void*)get_local_size));
		funs.push_back(std::make_pair(jitRT::getFunction("get_local_id", mod), (void*)get_local_id));
		funs.push_back(std::make_pair(jitRT::getFunction("get_num_groups", mod), (void*)get_num_groups));
		funs.push_back(std::make_pair(jitRT::getFunction("get_group_id", mod), (void*)get_group_id));

		funs.push_back(std::make_pair(jitRT::getFunction("get_global_id_SIMD", mod), (void*)get_global_id_SIMD));
		funs.push_back(std::make_pair(jitRT::getFunction("get_local_id_SIMD", mod), (void*)get_local_id_SIMD));

		for (cl_uint i=0, e=funs.size(); i<e; ++i) {
			llvm::Function* funDecl = funs[i].first;
			void* funImpl = funs[i].second;

			if (funDecl) jitRT::replaceAllUsesWith(funDecl, jitRT::createFunctionPointer(funDecl, funImpl));
		}
	}

	bool packetizeKernelFunction(const std::string& kernelName, const std::string& targetKernelName, llvm::Module* mod, const cl_uint packetizationSize, const bool use_sse41, const bool verbose) {
		if (!jitRT::getFunction(kernelName, mod)) {
			std::cerr << "ERROR: source function '" << kernelName << "' not found in module!\n";
			return false;
		}
		if (!jitRT::getFunction(targetKernelName, mod)) {
			std::cerr << "ERROR: target function '" << targetKernelName  << "' not found in module!\n";
			return false;
		}

		jitRT::Packetizer* packetizer = jitRT::getPacketizer(use_sse41, verbose);
		jitRT::addFunctionToPacketizer(packetizer, kernelName, targetKernelName, packetizationSize);

		jitRT::addNativeFunctionToPacketizer(packetizer, "get_global_id", -1, jitRT::getFunction("get_global_id_SIMD", mod), true);
		jitRT::addNativeFunctionToPacketizer(packetizer, "get_local_id", -1, jitRT::getFunction("get_local_id_SIMD", mod), true);

		jitRT::runPacketizer(packetizer, mod);

		if (!jitRT::getFunction(targetKernelName, mod)) {
			std::cerr << "ERROR: packetized target function not found in module!\n";
			return false;
		}

		return true;
	}
}


///////////////////////////////////////////////////////////////////////////
//                SSE OpenCL Internal Data Structures                    //
///////////////////////////////////////////////////////////////////////////

/*
An OpenCL context is created with one or more devices. Contexts
are used by the OpenCL runtime for managing objects such as command-queues, memory,
program and kernel objects and for executing kernels on one or more devices specified in the
context.
*/
struct _cl_context {
	llvm::TargetData* targetData;
	llvm::ExecutionEngine* engine;
};

/*
OpenCL objects such as memory, program and kernel objects are created using a context.
Operations on these objects are performed using a command-queue. The command-queue can be
used to queue a set of operations (referred to as commands) in order. Having multiple
command-queues allows applications to queue multiple independent commands without
requiring synchronization. Note that this should work as long as these objects are not being
shared. Sharing of objects across multiple command-queues will require the application to
perform appropriate synchronization. This is described in Appendix A.
*/
struct _cl_command_queue {
	_cl_context* context;
};

/*
Memory objects are categorized into two types: buffer objects, and image objects. A buffer
object stores a one-dimensional collection of elements whereas an image object is used to store a
two- or three- dimensional texture, frame-buffer or image.
Elements of a buffer object can be a scalar data type (such as an int, float), vector data type, or a
user-defined structure. An image object is used to represent a buffer that can be used as a texture
or a frame-buffer. The elements of an image object are selected from a list of predefined image
formats. The minimum number of elements in a memory object is one.
*/
struct _cl_mem {
private:
	_cl_context* context;
	size_t size; //entire size in bytes
	void* data;
public:
	_cl_mem(_cl_context* ctx, size_t bytes, void* values) : context(ctx), size(bytes), data(values) {}
	
	inline _cl_context* get_context() const { return context; }
	inline void* get_data() const { return data; }
	inline size_t get_size() const { return size; }

	inline void set_data(void* values) { data = values; }
	inline void set_data(const void* values, const size_t bytes, const size_t offset=0) {
		assert (bytes+offset <= size);
		if (offset == 0) memcpy(data, values, bytes);
		else {
			for (cl_uint i=offset; i<bytes; ++i) {
				((char*)data)[i] = ((const char*)values)[i];
			}
		}
	}
};

/*
A sampler object describes how to sample an image when the image is read in the kernel. The
built-in functions to read from an image in a kernel take a sampler as an argument. The sampler
arguments to the image read function can be sampler objects created using OpenCL functions
and passed as argument values to the kernel or can be samplers declared inside a kernel. In this
section we discuss how sampler objects are created using OpenCL functions.
*/
struct _cl_sampler {
	_cl_context* context;
};

/*
An OpenCL program consists of a set of kernels that are identified as functions declared with
the __kernel qualifier in the program source. OpenCL programs may also contain auxiliary
functions and constant data that can be used by __kernel functions. The program executable
can be generated online or offline by the OpenCL compiler for the appropriate target device(s).
A program object encapsulates the following information:
       An associated context.
       A program source or binary.
       The latest successfully built program executable, the list of devices for which the
       program executable is built, the build options used and a build log.
       The number of kernel objects currently attached.
*/
struct _cl_program {
	_cl_context* context;
	void* clProgram;
	llvm::Module* module;
};


#define CL_SHARED 0x3 // does not exist in specification 1.0
#define CL_CONSTANT 0x4 // does not exist in specification 1.0
struct _cl_kernel_arg {
private:
	_cl_mem* values; // stores *all* values for this argument
	size_t size; // size of one item in bits
	cl_uint qualifier;
public:
	//~_cl_kernel_arg() { free(values); }
	inline void set_values(const _cl_mem* vals) {
		assert (vals);
		// ALWAYS create new _cl_mem object and copy values!
		//if (values) free(values);

		size_t bytes = vals->get_size();
		void* data_cp = malloc(bytes);
		memcpy(data_cp, vals->get_data(), bytes);
		values = new _cl_mem(vals->get_context(), bytes, data_cp);
		std::cout << "_cl_mem(" << bytes << ", " << data_cp << ") -> " << values << "\n";
	}
	inline void set_size(const size_t s) {
		assert (s > 0);
		size = s;
	}
	inline void set_qualifier(const cl_uint qual) {
		assert (qual == CL_LOCAL || qual == CL_GLOBAL);
		qualifier = qual;
	}

	inline _cl_mem* get_values() { assert(values); return values; }
	inline size_t get_size() const { return size; }
	inline cl_uint get_qualifier() const { return qualifier; }
};

/*
A kernel is a function declared in a program. A kernel is identified by the __kernel qualifier
applied to any function in a program. A kernel object encapsulates the specific __kernel
function declared in a program and the argument values to be used when executing this
__kernel function.
*/
struct _cl_kernel {
private:
	_cl_context* context;
	void* function;
	_cl_kernel_arg* args;
	cl_uint num_args;

public:
	_cl_kernel() : context(NULL), function(NULL), args(NULL) {}

	inline void set_context(_cl_context* ctx) {
		assert (ctx);
		context = ctx;
	}
	inline void set_function(void* f) {
		assert (f);
		function = f;
	}
	inline void set_num_args(const cl_uint num) {
		assert (num > 0);
		num_args = num;
		args = new _cl_kernel_arg[num]();
	}
	inline void arg_set_info(const cl_uint arg_index, const size_t size, const cl_uint qualifier) {
		assert (args && "set_num_args() has to be called before arg_set_info()!");
		assert ((qualifier == CL_LOCAL || qualifier == CL_GLOBAL) && "only local and global allowed as qualifiers so far!");
		args[arg_index].set_size(size);
		args[arg_index].set_qualifier(qualifier);
	}

	inline _cl_context* get_context() const { return context; }
	inline void* get_function() const { return function; }
	inline cl_uint get_num_args() const { return num_args; }

	inline bool arg_is_global(const cl_uint arg_index) const {
		assert (arg_index < num_args);
		return args[arg_index].get_qualifier() == CL_GLOBAL;
	}
	inline bool arg_is_local(const cl_uint arg_index) const {
		assert (arg_index < num_args);
		return args[arg_index].get_qualifier() == CL_LOCAL;
	}
	inline bool arg_is_shared(const cl_uint arg_index) const {
		assert (false && "no support for shared qualifier impemented yet");
		assert (arg_index < num_args);
		return args[arg_index].get_qualifier() == CL_SHARED;
	}
	inline bool arg_is_constant(const cl_uint arg_index) const {
		assert (false && "no support for constant qualifier impemented yet");
		assert (arg_index < num_args);
		return args[arg_index].get_qualifier() == CL_CONSTANT;
	}
	inline size_t arg_get_size(const cl_uint arg_index) const {
		assert (arg_index < num_args);
		return args[arg_index].get_size();
	}
	inline void arg_set_values(const cl_uint arg_index, const _cl_mem* values) {
		assert (arg_index < num_args);
		args[arg_index].set_values(values);
	}
};

struct _cl_event {
	_cl_context* context;
};

///////////////////////////////////////////////////////////////////////////
//                 SSE OpenCL Driver Implementation                      //
///////////////////////////////////////////////////////////////////////////

/* Platform API */
CL_API_ENTRY cl_int CL_API_CALL
clGetPlatformIDs(cl_uint          num_entries,
                 cl_platform_id * platforms,
                 cl_uint *        num_platforms) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clGetPlatformInfo(cl_platform_id   platform,
                  cl_platform_info param_name,
                  size_t           param_value_size,
                  void *           param_value,
                  size_t *         param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

/* Device APIs */
CL_API_ENTRY cl_int CL_API_CALL
clGetDeviceIDs(cl_platform_id   platform,
               cl_device_type   device_type,
               cl_uint          num_entries,
               cl_device_id *   devices,
               cl_uint *        num_devices) CL_API_SUFFIX__VERSION_1_0
{
	if (device_type != CL_DEVICE_TYPE_CPU) {
		std::cerr << "ERROR: can not handle devices other than CPU!\n";
		return CL_INVALID_DEVICE_TYPE;
	}
	if (devices && num_entries < 1) return CL_INVALID_VALUE;
	if (!devices && !num_devices) return CL_INVALID_VALUE;
	if (devices) devices = new cl_device_id();
	if (num_devices) num_devices = new cl_uint(1);
	return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clGetDeviceInfo(cl_device_id    device,
                cl_device_info  param_name,
                size_t          param_value_size,
                void *          param_value,
                size_t *        param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

/* Context APIs  */
CL_API_ENTRY cl_context CL_API_CALL
clCreateContext(const cl_context_properties * properties,
                cl_uint                       num_devices,
                const cl_device_id *          devices,
                void (*pfn_notify)(const char *, const void *, size_t, void *),
                void *                        user_data,
                cl_int *                      errcode_ret) CL_API_SUFFIX__VERSION_1_0
{
	*errcode_ret = CL_SUCCESS;
	return new _cl_context();
}

CL_API_ENTRY cl_context CL_API_CALL
clCreateContextFromType(const cl_context_properties * properties,
                        cl_device_type                device_type,
                        void (*pfn_notify)(const char *, const void *, size_t, void *),
                        void *                        user_data,
                        cl_int *                      errcode_ret) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return NULL;
}

CL_API_ENTRY cl_int CL_API_CALL
clRetainContext(cl_context context) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clReleaseContext(cl_context context) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clGetContextInfo(cl_context         context,
                 cl_context_info    param_name,
                 size_t             param_value_size,
                 void *             param_value,
                 size_t *           param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

/* Command Queue APIs */

/*
creates a command-queue on a specific device.
*/
// -> ??
CL_API_ENTRY cl_command_queue CL_API_CALL
clCreateCommandQueue(cl_context                     context,
                     cl_device_id                   device,
                     cl_command_queue_properties    properties,
                     cl_int *                       errcode_ret) CL_API_SUFFIX__VERSION_1_0
{
	errcode_ret = CL_SUCCESS;
	_cl_command_queue* cq = new _cl_command_queue();
	cq->context = context;
	return cq;
}

CL_API_ENTRY cl_int CL_API_CALL
clRetainCommandQueue(cl_command_queue command_queue) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clReleaseCommandQueue(cl_command_queue command_queue) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clGetCommandQueueInfo(cl_command_queue      command_queue,
                      cl_command_queue_info param_name,
                      size_t                param_value_size,
                      void *                param_value,
                      size_t *              param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clSetCommandQueueProperty(cl_command_queue              command_queue,
                          cl_command_queue_properties   properties,
                          cl_bool                        enable,
                          cl_command_queue_properties * old_properties) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

/* Memory Object APIs  */

/*
Memory objects are categorized into two types: buffer objects, and image objects. A buffer
object stores a one-dimensional collection of elements whereas an image object is used to store a
two- or three- dimensional texture, frame-buffer or image.
Elements of a buffer object can be a scalar data type (such as an int, float), vector data type, or a
user-defined structure. An image object is used to represent a buffer that can be used as a texture
or a frame-buffer. The elements of an image object are selected from a list of predefined image
formats. The minimum number of elements in a memory object is one.
*/
CL_API_ENTRY cl_mem CL_API_CALL
clCreateBuffer(cl_context   context,
               cl_mem_flags flags,
               size_t       size, //in bytes
               void *       host_ptr,
               cl_int *     errcode_ret) CL_API_SUFFIX__VERSION_1_0
{
	if (!context) { if (errcode_ret) *errcode_ret = CL_INVALID_CONTEXT; return NULL; }
	if (size == 0 || size > CL_DEVICE_MAX_MEM_ALLOC_SIZE) { if (errcode_ret) *errcode_ret = CL_INVALID_BUFFER_SIZE; return NULL; }

	_cl_mem* mem = new _cl_mem(context, size, host_ptr ? host_ptr : malloc(size));
	std::cout << "new buffer(" << size << ", " << mem->get_data() << ") -> " << mem << "\n";

	if (errcode_ret) *errcode_ret = CL_SUCCESS;
	return mem;
}

CL_API_ENTRY cl_mem CL_API_CALL
clCreateImage2D(cl_context              context,
                cl_mem_flags            flags,
                const cl_image_format * image_format,
                size_t                  image_width,
                size_t                  image_height,
                size_t                  image_row_pitch,
                void *                  host_ptr,
                cl_int *                errcode_ret) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return NULL;
}

CL_API_ENTRY cl_mem CL_API_CALL
clCreateImage3D(cl_context              context,
                cl_mem_flags            flags,
                const cl_image_format * image_format,
                size_t                  image_width,
                size_t                  image_height,
                size_t                  image_depth,
                size_t                  image_row_pitch,
                size_t                  image_slice_pitch,
                void *                  host_ptr,
                cl_int *                errcode_ret) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return NULL;
}

CL_API_ENTRY cl_int CL_API_CALL
clRetainMemObject(cl_mem memobj) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clReleaseMemObject(cl_mem memobj) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clGetSupportedImageFormats(cl_context           context,
                           cl_mem_flags         flags,
                           cl_mem_object_type   image_type,
                           cl_uint              num_entries,
                           cl_image_format *    image_formats,
                           cl_uint *            num_image_formats) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clGetMemObjectInfo(cl_mem           memobj,
                   cl_mem_info      param_name,
                   size_t           param_value_size,
                   void *           param_value,
                   size_t *         param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clGetImageInfo(cl_mem           image,
               cl_image_info    param_name,
               size_t           param_value_size,
               void *           param_value,
               size_t *         param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

/* Sampler APIs  */
CL_API_ENTRY cl_sampler CL_API_CALL
clCreateSampler(cl_context          context,
                cl_bool             normalized_coords,
                cl_addressing_mode  addressing_mode,
                cl_filter_mode      filter_mode,
                cl_int *            errcode_ret) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return NULL;
}

CL_API_ENTRY cl_int CL_API_CALL
clRetainSampler(cl_sampler sampler) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clReleaseSampler(cl_sampler sampler) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clGetSamplerInfo(cl_sampler         sampler,
                 cl_sampler_info    param_name,
                 size_t             param_value_size,
                 void *             param_value,
                 size_t *           param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

/* Program Object APIs  */

/*
creates a program object for a context, and loads the source code specified by the text strings in
the strings array into the program object. The devices associated with the program object are the
devices associated with context.
*/
// -> read strings and store as .cl representation
CL_API_ENTRY cl_program CL_API_CALL
clCreateProgramWithSource(cl_context        context,
                          cl_uint           count,
                          const char **     strings,
                          const size_t *    lengths,
                          cl_int *          errcode_ret) CL_API_SUFFIX__VERSION_1_0
{
	errcode_ret = CL_SUCCESS;
	_cl_program* p = new _cl_program();
	p->context = context;
	return p;
}

// -> read binary and store as .cl representation
CL_API_ENTRY cl_program CL_API_CALL
clCreateProgramWithBinary(cl_context                     context,
                          cl_uint                        num_devices,
                          const cl_device_id *           device_list,
                          const size_t *                 lengths,
                          const unsigned char **         binaries,
                          cl_int *                       binary_status,
                          cl_int *                       errcode_ret) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return NULL;
}

CL_API_ENTRY cl_int CL_API_CALL
clRetainProgram(cl_program program) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clReleaseProgram(cl_program program) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

/*
builds (compiles & links) a program executable from the program source or binary for all the
devices or a specific device(s) in the OpenCL context associated with program. OpenCL allows
program executables to be built using the source or the binary. clBuildProgram must be called
for program created using either clCreateProgramWithSource or
clCreateProgramWithBinary to build the program executable for one or more devices
associated with program.
*/
// -> build LLVM module from .cl representation (from createProgramWithSource/Binary)
// -> invoke clc
// -> invoke llvm-as
// -> store module in _cl_program object
CL_API_ENTRY cl_int CL_API_CALL
clBuildProgram(cl_program           program,
               cl_uint              num_devices,
               const cl_device_id * device_list,
               const char *         options,
               void (*pfn_notify)(cl_program program, void * user_data),
               void *               user_data) CL_API_SUFFIX__VERSION_1_0
{
	if (!program) return CL_INVALID_PROGRAM;
	if (!device_list && num_devices > 0) return CL_INVALID_VALUE;
	if (device_list && num_devices == 0) return CL_INVALID_VALUE;
	if (user_data && !pfn_notify) return CL_INVALID_VALUE;

	//TODO: read .cl representation, invoke clc, invoke llvm-as

	//llvm::Module* mod = jitRT::createModuleFromFile("sseOpenCL.tmp.module.bc");

	//FIXME: hardcoded for testing ;)
	llvm::Module* mod = jitRT::createModuleFromFile("simpleTest.bc");
	if (!mod) return CL_BUILD_PROGRAM_FAILURE;

	// initialize context
	program->context->targetData = jitRT::getTargetData(mod);
	program->context->engine = jitRT::getExecutionEngine(mod);

	program->module = mod;
	return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clUnloadCompiler(void) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clGetProgramInfo(cl_program         program,
                 cl_program_info    param_name,
                 size_t             param_value_size,
                 void *             param_value,
                 size_t *           param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clGetProgramBuildInfo(cl_program            program,
                      cl_device_id          device,
                      cl_program_build_info param_name,
                      size_t                param_value_size,
                      void *                param_value,
                      size_t *              param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

/* Kernel Object APIs */

// -> compile bitcode of function from .bc file to native code
// -> store void* in _cl_kernel object
CL_API_ENTRY cl_kernel CL_API_CALL
clCreateKernel(cl_program      program,
               const char *    kernel_name,
               cl_int *        errcode_ret) CL_API_SUFFIX__VERSION_1_0
{
	if (!program) { *errcode_ret = CL_INVALID_PROGRAM; return NULL; }

	// does this mean we should compile before??
	llvm::Module* module = program->module;
	if (!module) { *errcode_ret = CL_INVALID_PROGRAM_EXECUTABLE; return NULL; }

	if (!kernel_name) { *errcode_ret = CL_INVALID_VALUE; return NULL; }

	std::stringstream strs;
	strs << "__OpenCL_" << kernel_name << "_kernel";
	const std::string new_kernel_name = strs.str();

	const llvm::Function* f = jitRT::getFunction(new_kernel_name, module);
	if (!f) { *errcode_ret = CL_INVALID_KERNEL_NAME; return NULL; }

	resolveRuntimeCalls(module);

	jitRT::resetTargetData(module);
	void* compiledFnPtr = jitRT::getPointerToFunction(module, new_kernel_name);
	if (!compiledFnPtr) { *errcode_ret = CL_INVALID_PROGRAM_EXECUTABLE; return NULL; }

	// create kernel object
	_cl_kernel* kernel = new _cl_kernel();
	kernel->set_context(program->context);
	kernel->set_function(compiledFnPtr);

	// analyze arguments & initialize kernel argument info
	const cl_uint num_args = jitRT::getNumArgs(f);
	kernel->set_num_args(num_args);
	for (cl_uint i=0; i<num_args; ++i) {
		const size_t arg_size = jitRT::getArgSizeInBits(f, i, program->context->targetData);
		const cl_uint qualifier = CL_GLOBAL; //everything global on CPU?
		kernel->arg_set_info(i, arg_size, qualifier);
	}

	*errcode_ret = CL_SUCCESS;
	return kernel;
}

CL_API_ENTRY cl_int CL_API_CALL
clCreateKernelsInProgram(cl_program     program,
                         cl_uint        num_kernels,
                         cl_kernel *    kernels,
                         cl_uint *      num_kernels_ret) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clRetainKernel(cl_kernel    kernel) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clReleaseKernel(cl_kernel   kernel) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clSetKernelArg(cl_kernel    kernel,
               cl_uint      arg_index,
               size_t       arg_size,
               const void * arg_value) CL_API_SUFFIX__VERSION_1_0
{
	std::cout << "clSetKernelArg(" << arg_index << ", " << arg_size << ", " << *(const _cl_mem**)arg_value << ")\n";
	if (!kernel) return CL_INVALID_KERNEL;
	if (arg_index > kernel->get_num_args()) return CL_INVALID_ARG_INDEX;
	const bool is_local = kernel->arg_is_local(arg_index);
	if ((!arg_value && !is_local) || (arg_value && is_local)) return CL_INVALID_ARG_VALUE;
	//const size_t kernel_arg_size = kernel->arg_get_size(arg_index);
	//if (arg_size != kernel_arg_size) return CL_INVALID_ARG_SIZE; //more checks required

	// NOTE: This function can be called with arg_size = sizeof(_cl_mem), which means
	//       that this is not the actual size of the argument data type.
	// NOTE: This code relies on the fact that the value is an object of type _cl_mem ...
	// NOTE: This is not everything: we have to check what kind of argument this index is
	//       We must not access arg_value as a _cl_mem** if it is e.g. an unsigned int



	kernel->arg_set_values(arg_index, *(const _cl_mem**)arg_value);

	return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clGetKernelInfo(cl_kernel       kernel,
                cl_kernel_info  param_name,
                size_t          param_value_size,
                void *          param_value,
                size_t *        param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clGetKernelWorkGroupInfo(cl_kernel                  kernel,
                         cl_device_id               device,
                         cl_kernel_work_group_info  param_name,
                         size_t                     param_value_size,
                         void *                     param_value,
                         size_t *                   param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

/* Event Object APIs  */
CL_API_ENTRY cl_int CL_API_CALL
clWaitForEvents(cl_uint             num_events,
                const cl_event *    event_list) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clGetEventInfo(cl_event         event,
               cl_event_info    param_name,
               size_t           param_value_size,
               void *           param_value,
               size_t *         param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clRetainEvent(cl_event event) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clReleaseEvent(cl_event event) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

/* Profiling APIs  */
CL_API_ENTRY cl_int CL_API_CALL
clGetEventProfilingInfo(cl_event            event,
                        cl_profiling_info   param_name,
                        size_t              param_value_size,
                        void *              param_value,
                        size_t *            param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

/* Flush and Finish APIs */
CL_API_ENTRY cl_int CL_API_CALL
clFlush(cl_command_queue command_queue) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clFinish(cl_command_queue command_queue) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

/* Enqueued Commands APIs */
CL_API_ENTRY cl_int CL_API_CALL
clEnqueueReadBuffer(cl_command_queue    command_queue,
                    cl_mem              buffer,
                    cl_bool             blocking_read,
                    size_t              offset,
                    size_t              cb,
                    void *              ptr,
                    cl_uint             num_events_in_wait_list,
                    const cl_event *    event_wait_list,
                    cl_event *          event) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueWriteBuffer(cl_command_queue   command_queue,
                     cl_mem             buffer,
                     cl_bool            blocking_write,
                     size_t             offset,
                     size_t             cb,
                     const void *       ptr,
                     cl_uint            num_events_in_wait_list,
                     const cl_event *   event_wait_list,
                     cl_event *         event) CL_API_SUFFIX__VERSION_1_0
{
	if (!command_queue) return CL_INVALID_COMMAND_QUEUE;
	if (!buffer) return CL_INVALID_MEM_OBJECT;
	if (!ptr || buffer->get_size() < cb+offset) return CL_INVALID_VALUE;
	if (!event_wait_list && num_events_in_wait_list > 0) return CL_INVALID_EVENT_WAIT_LIST;
	if (event_wait_list && num_events_in_wait_list == 0) return CL_INVALID_EVENT_WAIT_LIST;
	if (command_queue->context != buffer->get_context()) return CL_INVALID_CONTEXT;
	//clEnqueueWriteBuffer(commands, input, CL_TRUE, 0, sizeof(float) * count, data, 0, NULL, NULL);

	// Write data into 'device memory' (buffer)
	// In our case, we actually should not have to copy data
	// because we are still on the CPU. However, const void* prevents this.
	// Thus, just copy over each byte.
	// TODO: specification seems to require something different?
	//       storing access patterns to command_queue or sth like that?
	buffer->set_data(ptr, cb, offset); //cb is size in bytes
	
	return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueCopyBuffer(cl_command_queue    command_queue,
                    cl_mem              src_buffer,
                    cl_mem              dst_buffer,
                    size_t              src_offset,
                    size_t              dst_offset,
                    size_t              cb,
                    cl_uint             num_events_in_wait_list,
                    const cl_event *    event_wait_list,
                    cl_event *          event) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueReadImage(cl_command_queue     command_queue,
                   cl_mem               image,
                   cl_bool              blocking_read,
                   const size_t *       origin[3],
                   const size_t *       region[3],
                   size_t               row_pitch,
                   size_t               slice_pitch,
                   void *               ptr,
                   cl_uint              num_events_in_wait_list,
                   const cl_event *     event_wait_list,
                   cl_event *           event) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueWriteImage(cl_command_queue    command_queue,
                    cl_mem              image,
                    cl_bool             blocking_write,
                    const size_t *      origin[3],
                    const size_t *      region[3],
                    size_t              input_row_pitch,
                    size_t              input_slice_pitch,
                    const void *        ptr,
                    cl_uint             num_events_in_wait_list,
                    const cl_event *    event_wait_list,
                    cl_event *          event) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueCopyImage(cl_command_queue     command_queue,
                   cl_mem               src_image,
                   cl_mem               dst_image,
                   const size_t *       src_origin[3],
                   const size_t *       dst_origin[3],
                   const size_t *       region[3],
                   cl_uint              num_events_in_wait_list,
                   const cl_event *     event_wait_list,
                   cl_event *           event) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueCopyImageToBuffer(cl_command_queue command_queue,
                           cl_mem           src_image,
                           cl_mem           dst_buffer,
                           const size_t *   src_origin[3],
                           const size_t *   region[3],
                           size_t           dst_offset,
                           cl_uint          num_events_in_wait_list,
                           const cl_event * event_wait_list,
                           cl_event *       event) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueCopyBufferToImage(cl_command_queue command_queue,
                           cl_mem           src_buffer,
                           cl_mem           dst_image,
                           size_t           src_offset,
                           const size_t *   dst_origin[3],
                           const size_t *   region[3],
                           cl_uint          num_events_in_wait_list,
                           const cl_event * event_wait_list,
                           cl_event *       event) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

CL_API_ENTRY void * CL_API_CALL
clEnqueueMapBuffer(cl_command_queue command_queue,
                   cl_mem           buffer,
                   cl_bool          blocking_map,
                   cl_map_flags     map_flags,
                   size_t           offset,
                   size_t           cb,
                   cl_uint          num_events_in_wait_list,
                   const cl_event * event_wait_list,
                   cl_event *       event,
                   cl_int *         errcode_ret) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return NULL;
}

CL_API_ENTRY void * CL_API_CALL
clEnqueueMapImage(cl_command_queue  command_queue,
                  cl_mem            image,
                  cl_bool           blocking_map,
                  cl_map_flags      map_flags,
                  const size_t *    origin[3],
                  const size_t *    region[3],
                  size_t *          image_row_pitch,
                  size_t *          image_slice_pitch,
                  cl_uint           num_events_in_wait_list,
                  const cl_event *  event_wait_list,
                  cl_event *        event,
                  cl_int *          errcode_ret) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return NULL;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueUnmapMemObject(cl_command_queue command_queue,
                        cl_mem           memobj,
                        void *           mapped_ptr,
                        cl_uint          num_events_in_wait_list,
                        const cl_event *  event_wait_list,
                        cl_event *        event) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueNDRangeKernel(cl_command_queue command_queue,
                       cl_kernel        kernel,
                       cl_uint          work_dim,
                       const size_t *   global_work_offset,
                       const size_t *   global_work_size,
                       const size_t *   local_work_size,
                       cl_uint          num_events_in_wait_list,
                       const cl_event * event_wait_list,
                       cl_event *       event) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueTask(cl_command_queue  command_queue,
              cl_kernel         kernel,
              cl_uint           num_events_in_wait_list,
              const cl_event *  event_wait_list,
              cl_event *        event) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueNativeKernel(cl_command_queue  command_queue,
					  void (*user_func)(void *),
                      void *            args,
                      size_t            cb_args,
                      cl_uint           num_mem_objects,
                      const cl_mem *    mem_list,
                      const void **     args_mem_loc,
                      cl_uint           num_events_in_wait_list,
                      const cl_event *  event_wait_list,
                      cl_event *        event) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueMarker(cl_command_queue    command_queue,
                cl_event *          event) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueWaitForEvents(cl_command_queue command_queue,
                       cl_uint          num_events,
                       const cl_event * event_list) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueBarrier(cl_command_queue command_queue) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return CL_SUCCESS;
}

/* Extension function access
 *
 * Returns the extension function address for the given function name,
 * or NULL if a valid function can not be found.  The client must
 * check to make sure the address is not NULL, before using or
 * calling the returned function address.
 */
CL_API_ENTRY void * CL_API_CALL clGetExtensionFunctionAddress(const char * func_name) CL_API_SUFFIX__VERSION_1_0
{
	assert (false && "NOT IMPLEMENTED!");
	return NULL;
}

#ifdef __cplusplus
}
#endif



// old



///////////////////////////////////////////////////////////////////////////
//                 OpenCL Runtime Implementation                         //
///////////////////////////////////////////////////////////////////////////

#if 0
#include <xmmintrin.h>
// a class 'OpenCLRuntime' would be nicer,
// but we cannot store address of member function to bitcode
namespace {

	//
	// scalar implementation
	//
	static const unsigned simdWidth = 4;

	unsigned dimensions; // max 3
	size_t* globalThreads; // total # work items per dimension, arbitrary size
	size_t* localThreads;  // size of each work group per dimension

	size_t* currentGlobal; // 0 -> globalThreads[D] -1
	size_t* currentLocal;  // 0 -> SIMD width -1
	size_t* currentGroup;  // 0 -> (globalThreads[D] / localThreads[D]) -1

	void initializeOpenCL(const unsigned dim) {
		assert (dim < 4 && "max # dimensions is 3!");
		dimensions = dim;

		globalThreads = new size_t[dim]();
		localThreads = new size_t[dim]();

		currentGlobal = new size_t[dim]();
		currentLocal = new size_t[dim]();
		currentGroup = new size_t[dim]();
		
		for (unsigned i=0; i<dimensions; ++i) {
			currentGlobal[i] = 0;
			currentLocal[i] = 0;
			currentGroup[i] = 0;
		}
	}

	inline void initializeThreads(size_t* gThreads, size_t* lThreads) {
		for (unsigned i=0; i<dimensions; ++i) {
			globalThreads[i] = gThreads[i];
			localThreads[i] = lThreads[i];
		}
	}
	
	// D is dimension index.

	Num. of dimensions in use */
	inline unsigned get_work_dim() {
		return dimensions;
	}

	Num. of global work-items */
	inline size_t get_global_size(unsigned D) {
		assert (D < dimensions);
		if (D >= dimensions) return 1;
		return globalThreads[D];
	}

	Global work-item ID value */
	inline size_t get_global_id(unsigned D) {
		assert (D < dimensions);
		if (D >= dimensions) return CL_SUCCESS;
		return currentGlobal[D];
	}

	Num. of local work-items */
	inline size_t get_local_size(unsigned D) {
		assert (D < dimensions);
		if (D >= dimensions) return 1;
		return localThreads[D];
	}

	Local work-item ID */
	inline size_t get_local_id(unsigned D) {
		assert (D < dimensions);
		if (D >= dimensions) return CL_SUCCESS;
		return currentLocal[D];
	}

	Num. of work-groups */
	inline size_t get_num_groups(unsigned D) {
		assert (D < dimensions);
		if (D >= dimensions) return 1;
		return globalThreads[D] / localThreads[D];
	}

	Returns the work-group ID */
	inline size_t get_group_id(unsigned D) {
		assert (D < dimensions);
		if (D >= dimensions) return CL_SUCCESS;
		return currentGroup[D];
	}


	inline void setCurrentGlobal(unsigned D, size_t id) {
		assert (D < dimensions);
		assert (id < get_global_size(D));
		currentGlobal[D] = id;
	}
	inline void setCurrentLocal(unsigned D, size_t id) {
		assert (D < dimensions);
		assert (id < get_local_size(D));
		currentLocal[D] = id;
	}
	inline void setCurrentGroup(unsigned D, size_t id) {
		assert (D < dimensions);
		assert (id < get_num_groups(D));
		currentGroup[D] = id;
	}

	//
	// packetized implementations
	//

	__m128i* currentGlobal_SIMD; // 0 -> globalThreads[D] -1
	__m128i* currentLocal_SIMD;  // 0 -> SIMD width -1

	unsigned simdDimension;


	// called automatically by initializeOpenCL_SIMD
	inline void initializeThreads_SIMD(size_t* gThreads, size_t* lThreads) {
		size_t globalThreadNum = 0;
		size_t localThreadNum = 0;
		bool alignedGlobalDims[dimensions];
		bool alignedLocalDims[dimensions];

		bool error = false;

		for (unsigned i=0; i<dimensions; ++i) {
			const size_t globalThreadsDimI = gThreads[i];
			globalThreadNum += globalThreadsDimI;
			alignedGlobalDims[i] = (globalThreadsDimI % simdWidth == 0);
			globalThreads[i] = i == simdDimension ? (globalThreadsDimI / simdWidth) : globalThreadsDimI;

			const size_t localThreadsDimI = lThreads[i];
			localThreadNum += localThreadsDimI;
			alignedLocalDims[i] = (localThreadsDimI % simdWidth == 0);
			localThreads[i] = i == simdDimension ? (localThreadsDimI / simdWidth) : localThreadsDimI;

			if (i == simdDimension && !alignedGlobalDims[i]) {
				std::cerr << "ERROR: chosen SIMD dimension " << i << " is globally not dividable by " << simdWidth << " (global dimension)!\n";
				error = true;
			}
			if (i == simdDimension && !alignedLocalDims[i]) {
				std::cerr << "ERROR: chosen SIMD dimension " << i << " is locally not dividable by " << simdWidth << " (work-group dimension)!\n";
				error = true;
			}
			if (globalThreadsDimI % localThreadsDimI != 0) {
				std::cerr << "ERROR: global dimension " << i << " not dividable by local dimension (" << globalThreadsDimI << " / " << localThreadsDimI << ")!\n";
				error = true;
			}
		}
		if (globalThreadNum % simdWidth != 0) {
			std::cerr << "ERROR: global number of threads is not dividable by " << simdWidth << "!\n";
			error = true;
		}
		if (localThreadNum % simdWidth != 0) {
			std::cerr << "ERROR: number of threads in a group is not dividable by " << simdWidth << "!\n";
			error = true;
		}

		if (error) exit(-1);
	}

	void initializeOpenCL_SIMD(const unsigned dims, const unsigned simdDim, size_t* gThreads, size_t* lThreads) {
		if (dims > 3) {
			std::cerr << "ERROR: max # dimensions is 3!\n";
			exit(-1);
		}
		if (simdDim > dims) {
			std::cerr << "ERROR: chosen SIMD dimension out of bounds (" << simdWidth << " > " << dims << ")!\n";
			exit(-1);
		}
		dimensions = dims;
		simdDimension = simdDim-1; //-1 for array access

		globalThreads = new size_t[dimensions]();
		localThreads = new size_t[dimensions]();

		currentGlobal_SIMD = new __m128i[dimensions]();
		currentLocal_SIMD = new __m128i[dimensions]();
		currentGroup = new size_t[dimensions]();

		for (unsigned i=0; i<dimensions; ++i) {
			if (i == simdDimension) {
				currentGlobal_SIMD[i] = _mm_set_epi32(0, 1, 2, 3);
				currentLocal_SIMD[i] = _mm_set_epi32(0, 1, 2, 3);
			} else {
				currentGlobal_SIMD[i] = _mm_set_epi32(0, 0, 0, 0);
				currentLocal_SIMD[i] = _mm_set_epi32(0, 0, 0, 0);
			}
			currentGroup[i] = 0;
		}

		initializeThreads_SIMD(gThreads, lThreads);
	}

	inline __m128i get_global_id_SIMD(unsigned D) {
		assert (D < dimensions);
		return currentGlobal_SIMD[D];
	}
	inline __m128i get_local_id_SIMD(unsigned D) {
		assert (D < dimensions);
		return currentLocal_SIMD[D];
	}

	inline void setCurrentGlobal_SIMD(unsigned D, __m128i id) {
		assert (D < dimensions);
		assert (((size_t*)&id)[0] < get_global_size(D));
		assert (((size_t*)&id)[1] < get_global_size(D));
		assert (((size_t*)&id)[2] < get_global_size(D));
		assert (((size_t*)&id)[3] < get_global_size(D));
		currentGlobal_SIMD[D] = id;
	}
	inline void setCurrentLocal_SIMD(unsigned D, __m128i id) {
		assert (D < dimensions);
		assert (((size_t*)&id)[0] < get_local_size(D));
		assert (((size_t*)&id)[1] < get_local_size(D));
		assert (((size_t*)&id)[2] < get_local_size(D));
		assert (((size_t*)&id)[3] < get_local_size(D));
		currentLocal_SIMD[D] = id;
	}

};
#endif


///////////////////////////////////////////////////////////////////////////
//                  OpenCL Driver Implementation                         //
///////////////////////////////////////////////////////////////////////////

#if 0
#include <stdlib.h> //exit
#include <string.h> //strcmp
#include <iostream>
#include <vector>

int main(int argc, char** argv) {

	bool verbose = false;
    bool use_sse41 = true;
	bool optimize = true;
	bool verify = false;
	std::vector<std::string> linkModuleNames;
    std::string moduleName = "";
    std::string functionName = "";
    std::string targetFunctionName = "";
	std::string outputFileName = "";

    ///////////////////////////////////////////////////////////////////////////
    //                           read input                                  //
    ///////////////////////////////////////////////////////////////////////////

	bool displayUsage = false;

	//TODO: checks for existance of name do not work :P
	//      -> same in packetizeFuntion.cpp
	for (int i=1; i<argc; ++i) {
		if (strcmp(argv[i], "-m") == 0) {
			if (i+1 > argc) {
				std::cerr << "ERROR: no bitcode module specified!\n";
				return -1;
			}
			moduleName = argv[++i];
			continue;
		}
		if (strcmp(argv[i], "-f") == 0) {
			if (i+1 > argc) {
				std::cerr << "ERROR: no function name specified!\n";
				return -1;
			}
			functionName = argv[++i];
			continue;
		}
		if (strcmp(argv[i], "-t") == 0) {
			if (i+1 > argc) {
				std::cerr << "ERROR: no target function name specified!\n";
				return -1;
			}
			targetFunctionName = argv[++i];
			continue;
		}
		if (strcmp(argv[i], "-o") == 0) {
			if (i+1 > argc) {
				std::cerr << "ERROR: output filename missing!\n";
				return -1;
			}
			outputFileName = argv[++i];
			continue;
		}
		if (strcmp(argv[i], "-l") == 0) {
			if (i+1 > argc) {
				std::cerr << "ERROR: link module filename missing!\n";
				return -1;
			}
			linkModuleNames.push_back(argv[++i]);
			continue;
		}

		if (strcmp(argv[i], "-msse41") == 0) {
			use_sse41 = true;
			continue;
		}
		if (strcmp(argv[i], "-no-sse41") == 0) {
			use_sse41 = false;
			continue;
		}
		if (strcmp(argv[i], "-O0") == 0 || strcmp(argv[i], "-no-opt") == 0) {
			optimize = false;
			continue;
		}
		if (strcmp(argv[i], "-verify") == 0) {
			verify = true;
			continue;
		}
		if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "-verbose") == 0) {
			verbose = true;
			continue;
		}
		if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "-help") == 0) {
			displayUsage = true;
			continue;
		}
		std::cerr << "WARNING: Unknown input found: " << argv[i] << "\n";
	}

	if (!displayUsage) {
		// print error messages if mandatory input is missing
		if (argc < 1) {
			std::cerr << "ERROR: Wrong number of arguments!\n";
			displayUsage = true;
		}
		if (moduleName == "") {
			std::cerr << "ERROR: no source module specified!\n";
			displayUsage = true;
		}
		if (functionName == "") {
			std::cerr << "ERROR: no source function specified!\n";
			displayUsage = true;
		}
		if (targetFunctionName == "") {
			std::cerr << "ERROR: no target function specified!\n";
			displayUsage = true;
		}
	}

	//if necessary, print usage message
	if (displayUsage) {
		std::cerr << "\nUsage: " << argv[0] << " -m module.bc -f functionName -t targetFunctionName [options]\n";
        std::cerr << "Parameters:\n";
		std::cerr << "  -m FILE     : source bitcode module\n";
		std::cerr << "  -f NAME     : name of scalar source function in specified module\n";
		std::cerr << "  -t NAME     : name of target function prototype in specified module\n";
        std::cerr << "Options:\n";
		std::cerr << "  -o FILE     : output filename for human readable bitcode (.ll file)\n";
		std::cerr << "  -l FILE     : additional module that should be linked to source module\n";
        std::cerr << "  -no-sse41   : disable generation of SSE4.1 intrinsics\n";
		std::cerr << "  -O0 -no-opt : disable module optimization after packetization\n";
        std::cerr << "  -v -verbose : enable verbose output (only valid for debug builds)\n";
        std::cerr << "  -verify     : enable verification of module after packetization\n";
		std::cerr << "  -h -help    : display help message\n";
		return CL_SUCCESS;
    }

    ///////////////////////////////////////////////////////////////////////////
    //                       1. create source module                         //
    ///////////////////////////////////////////////////////////////////////////

    llvm::Module* mod = jitRT::createModuleFromFile(moduleName);
	if (!mod) {
		std::cerr << "ERROR: could not create source module from file '" << moduleName << "'!\n";
		return -1;
	}

    ///////////////////////////////////////////////////////////////////////////
    //                2. check if module is OpenCL bitcode                   //
    ///////////////////////////////////////////////////////////////////////////

	const std::string& targetTriple = jitRT::getTargetTriple(mod);
	if (strstr(targetTriple.c_str(), "opencl") == 0) {
		std::cerr << "ERROR: module '" << moduleName << "' does not target OpenCL!\n";
		std::cerr << "       bad target triple: " << targetTriple << "\n";
		return -1;
	}

	///////////////////////////////////////////////////////////////////////////
    //                       3. create link module(s)                        //
    ///////////////////////////////////////////////////////////////////////////

	const unsigned moduleNr = linkModuleNames.size();
    llvm::Module** linkModules = new llvm::Module*[moduleNr]();
    for (unsigned i=0; i<moduleNr; ++i) {
        linkModules[i] = jitRT::createModuleFromFile(linkModuleNames[i]);
        if (linkModules[i] == NULL) {
			std::cerr << "ERROR: could not create module from file '" << linkModuleNames[i] << "'!\n";
			return -1;
		}
    }

    ///////////////////////////////////////////////////////////////////////////
    //                          4. link modules                              //
    ///////////////////////////////////////////////////////////////////////////
	
	if (moduleNr > 1) {
		mod = jitRT::linkEverything(moduleNr, linkModules);
	}
    
	if (!mod) {
		std::cerr << "ERROR: linking of modules failed!\n";
		return -1;
	}

	///////////////////////////////////////////////////////////////////////////
    //                       5. packetize function                           //
    ///////////////////////////////////////////////////////////////////////////

	if (!jitRT::getFunction(functionName, mod)) {
		std::cerr << "ERROR: source function '" << functionName << "' not found in module!\n";
		return -1;
	}
	if (!jitRT::getFunction(targetFunctionName, mod)) {
		std::cerr << "ERROR: target function '" << targetFunctionName  << "' not found in module!\n";
		return -1;
	}

    const unsigned packetizationSize = 4;
    jitRT::Packetizer* packetizer = jitRT::getPacketizer(use_sse41, verbose);
    jitRT::addFunctionToPacketizer(packetizer, functionName, targetFunctionName, packetizationSize);

	jitRT::addNativeFunctionToPacketizer(packetizer, "get_global_id", -1, jitRT::getFunction("get_global_id_SIMD", mod), true);
	jitRT::addNativeFunctionToPacketizer(packetizer, "get_local_id", -1, jitRT::getFunction("get_local_id_SIMD", mod), true);

    jitRT::runPacketizer(packetizer, mod);

	if (!jitRT::getFunction(targetFunctionName, mod)) {
		std::cerr << "ERROR: packetized target function not found in module!\n";
		return -1;
	}

    ///////////////////////////////////////////////////////////////////////////
    //                 6. create, set up, and link runtime                   //
    ///////////////////////////////////////////////////////////////////////////

	// don't do this for functions that are replaced by packet functions above
	std::vector< std::pair<llvm::Function*, void*> > funs;
	funs.push_back(std::make_pair(jitRT::getFunction("get_work_dim", mod), (void*)get_work_dim));
	funs.push_back(std::make_pair(jitRT::getFunction("get_global_size", mod), (void*)get_global_size));
	funs.push_back(std::make_pair(jitRT::getFunction("get_global_id", mod), (void*)get_global_id));
	funs.push_back(std::make_pair(jitRT::getFunction("get_local_size", mod), (void*)get_local_size));
	funs.push_back(std::make_pair(jitRT::getFunction("get_local_id", mod), (void*)get_local_id));
	funs.push_back(std::make_pair(jitRT::getFunction("get_num_groups", mod), (void*)get_num_groups));
	funs.push_back(std::make_pair(jitRT::getFunction("get_group_id", mod), (void*)get_group_id));

	funs.push_back(std::make_pair(jitRT::getFunction("get_global_id_SIMD", mod), (void*)get_global_id_SIMD));
	funs.push_back(std::make_pair(jitRT::getFunction("get_local_id_SIMD", mod), (void*)get_local_id_SIMD));

	for (unsigned i=0, e=funs.size(); i<e; ++i) {
		llvm::Function* funDecl = funs[i].first;
		void* funImpl = funs[i].second;

		if (funDecl) jitRT::replaceAllUsesWith(funDecl, jitRT::createFunctionPointer(funDecl, funImpl));
	}

	// initialize OpenCL runtime
	const unsigned numDims = 1;
	const unsigned simdDim = 1;
	size_t gThreads[numDims] = { 16 };
	size_t lThreads[numDims] = { 4 };
	initializeOpenCL_SIMD(numDims, simdDim, gThreads, lThreads);

    
    ///////////////////////////////////////////////////////////////////////////
    //                    7. run optimization pass(es)                       //
    ///////////////////////////////////////////////////////////////////////////
    
    if (optimize) jitRT::optimizeFunction(targetFunctionName, mod);
    
    ///////////////////////////////////////////////////////////////////////////
    //                         8. verify module                              //
    ///////////////////////////////////////////////////////////////////////////
    
    if (verify) jitRT::verifyModule(mod);

    ///////////////////////////////////////////////////////////////////////////
    //                          9. print bytecode                            //
    ///////////////////////////////////////////////////////////////////////////


	if (outputFileName != "") {
		jitRT::writeFunctionToFile(jitRT::getFunction(targetFunctionName, mod), outputFileName);
	} else {
		jitRT::printFunction(jitRT::getFunction(targetFunctionName, mod));
		//outputFileName = targetFunctionName + ".ll";
	}

    return CL_SUCCESS;
}
#endif
