// This kernel tests whether unaligned loads are correctly created.
__kernel void TestUnaligned(
   __global float* input,
   __global float* output)
{
	int i = get_global_id(0);
	output[i] = (i >= get_global_size(0)-1) ? 0.f : input[i+1];
}
