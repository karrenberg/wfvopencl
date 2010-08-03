// This kernel tests whether continuations for barriers are constructed properly
// and live values are correctly stored and transferred. The test fails if the
// barrier is removed.
__kernel void TestBarrier2(
   __global float* input,
   __global float* output,
   __local  float* shared)
{
	int i = get_global_id(0);
	int l = get_local_id(0);

	shared[l] = input[i];

	barrier(CLK_LOCAL_MEM_FENCE);

	// shared[l+1] is not initialized if there is no barrier :)
	output[i] = (l == get_local_size(0)-1) ? 0.f : shared[l+1];
}
