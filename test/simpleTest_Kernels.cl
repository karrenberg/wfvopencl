// scalar source function
//
__kernel void square(
   __global float* input,
   __global float* output,
   const unsigned int count)
{
	int i = get_global_id(0);
	//printf("global_id: %d\n", i);
	//printf("input: %f\n", input[i]);
	//output[i] = input[i];
	if(i < count)
		output[i] = input[i] * input[i];
	//printf("result: %f\n", output[i]);
}

// target function for packetization
//
void square_SIMD(
    __global float4 * input,
    __global float4 * output,
    //const uint4 count); // for testing purposes...
    const unsigned int count); // const = uniform -> type remains scalar



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
	get_global_id_SIMD(0);
	get_local_id_SIMD(0);
}
