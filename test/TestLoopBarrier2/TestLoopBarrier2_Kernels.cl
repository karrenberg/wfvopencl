
__kernel void TestLoopBarrier2(
   __global float* input,
   __global float* output,
   __local  float* shared)
{
	int i = get_global_id(0);
	int l = get_local_id(0);

	shared[l] = input[i];

	barrier(CLK_LOCAL_MEM_FENCE);

	for (unsigned j=0; j<10; ++j) {
		shared[l] += 1.f;
		
		barrier(CLK_LOCAL_MEM_FENCE);

		if (l != 0) shared[l] += shared[l-1];

		barrier(CLK_LOCAL_MEM_FENCE);
	}
	output[i] = shared[l];
}
