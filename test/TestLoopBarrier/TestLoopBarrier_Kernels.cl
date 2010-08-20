
__kernel void TestLoopBarrier(
   __global float* input,
   __global float* output,
   const unsigned int count)
{
	int i = get_global_id(0);

	float x = input[i];
	float f = x * x;

	for (unsigned j=0; j<10; ++j) {
		barrier(CLK_LOCAL_MEM_FENCE);
		f -= count;
		barrier(CLK_LOCAL_MEM_FENCE);
	}

	output[i] = f;
}
