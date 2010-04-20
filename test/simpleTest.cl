
__kernel void square(
   __global float* input,
   __global float* output,
   const unsigned int count)
{
	//printf("input-addr: %p\n", input);
	//printf("output-addr: %p\n", output);

   int i = get_global_id(0);
	printf("global_id: %d\n", i);
	printf("input: %f\n", input[i]);
   output[i] = input[i];
   //if(i < count)
   //    output[i] = input[i] * input[i];
	printf("result: %f\n", output[i]);
}



// scalar wrapper function with unified signature for all kernels
// NOTE: not required if --march is specified (clc generates "stub"-function)
//
/*typedef struct {
	__global float* input;
	__global float* output;
	unsigned int count;
} argument_struct;

void square_wrapper(char* arguments) { // char* is llvm's void* ;)
	__global float* in = ((argument_struct*)arguments)->input;
	__global float* out = ((argument_struct*)arguments)->output;
	const unsigned int c = ((argument_struct*)arguments)->count;
	square(in, out, c);
	//arguments->output = out; //not required!
}*/


// target function for packetization
//
void square_SIMD(
    __global float4 * input,
    __global float4 * output,
    //const uint4 count); // for testing purposes...
    const unsigned int count); // const = uniform -> type remains scalar


// packetized wrapper function with unified signature for all kernels
//
/*
typedef struct {
	__global float4* input;
	__global float4* output;
	//uint4 count;
	unsigned int count;
} argument_struct_SIMD;

void square_SIMD_wrapper(char* arguments) { // char* is llvm's void* ;)
	__global float4* in = ((argument_struct_SIMD*)arguments)->input;
	__global float4* out = ((argument_struct_SIMD*)arguments)->output;
	//const uint4 c = ((argument_struct_SIMD*)arguments)->count;
	const unsigned int c = ((argument_struct_SIMD*)arguments)->count;
	//printf("input-addr: %p\n", in);
	//printf("output-addr: %p\n", out);
	//printf("input: %f %f %f %f\n", ((float*)&in)[0], ((float*)&in)[1], ((float*)&in)[2], ((float*)&in)[3]);
	square_SIMD(in, out, c);
	//arguments->output = out; //not required!
}
*/

// native simd function declarations for get_global_id
//
//uint4 get_global_id_SIMD(uint);
uint get_global_id_SIMD(uint);
uint4 get_local_id_SIMD(uint);


// fake call function to prevent deletion of declarations
// must never be called!
//
void __fakeCall(__global float4 * x, const uint4 y, const unsigned z) {
	//square_SIMD(x, x, y);
	square_SIMD(x, x, z);
	//square_SIMD_wrapper(0);
	get_global_id_SIMD(0);
	get_local_id_SIMD(0);
}
