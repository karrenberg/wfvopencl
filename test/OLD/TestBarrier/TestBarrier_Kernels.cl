// This kernel only tests if continuations for barriers are constructed
// properly. It produces the same results without the barrier.
__kernel void TestBarrier(
   __global float* input,
   __global float* output,
   const unsigned int count)
{
	int i = get_global_id(0);

	float x = input[i];
	float f = x * x - count;

	barrier(CLK_LOCAL_MEM_FENCE);

	output[i] = f;
}
