
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
__kernel void
square_4(
    __global float4 * input,
    __global float4 * output,
    const uint4 count);

// native simd function declarations for get_global_id
uint4 get_global_id_SIMD(uint);
uint4 get_local_id_SIMD(uint);



__kernel void __fakeCall(__global float4 * x, const uint4 y) {
	square_4(x, x, y);
	get_global_id_SIMD(0);
	get_local_id_SIMD(0);
}
