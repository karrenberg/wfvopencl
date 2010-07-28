
__kernel void TestBarrier(
   __global float* input,
   __global float* output,
   const unsigned int count)
{
	int i = get_global_id(0);

	float x = input[i];
	float f = x * x - count;

	//input[i*2] = f;

	barrier(CLK_LOCAL_MEM_FENCE);

	//output[i] = input[i*2];
	output[i] = f;
}
