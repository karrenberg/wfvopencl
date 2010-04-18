
__kernel void square(
   __global float* input,
   __global float* output,
   const unsigned int count)
{
   int i = get_global_id(0);
   if(i < count)
       output[i] = input[i] * input[i];
}



// target function for packetization
//
void square_4(
    __global float4 * input,
    __global float4 * output,
    const uint4 count);


// native simd function declarations for get_global_id
//
uint4 get_global_id_SIMD(uint);
uint4 get_local_id_SIMD(uint);


// wrapper function with unified signature for all kernels
//
// TODO: implement packet version
typedef struct {
	__global float* input;
	__global float* output;
	unsigned int count;
} argument_struct;

void sse_opencl_wrapper(char* arguments) { //char* = void* for LLVM ;)
	argument_struct* arg_str = (argument_struct*)arguments;
	__global float* in = arg_str->input;
	__global float* out = arg_str->output;
	const unsigned int c = arg_str->count;
	square(in, out, c);
	arg_str->output = out; //required?
}



// fake call function to prevent deletion of declarations
//
void __fakeCall(__global float4 * x, const uint4 y) {
	square_4(x, x, y);
	get_global_id_SIMD(0);
	get_local_id_SIMD(0);
}
