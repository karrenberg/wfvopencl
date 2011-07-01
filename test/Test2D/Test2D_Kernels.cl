// This kernel produces an output that is of size input_size * input_size.
__kernel void Test2D(
   __global float* input,
   __global float* output)
{
	int i = get_global_id(0);
	int j = get_global_id(1);

	const int idx = j + i*get_global_size(1);
	output[idx] = input[i] + input[j];
}
