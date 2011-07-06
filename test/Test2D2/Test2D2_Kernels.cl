// This kernel produces an output that is of size input_size * input_size.
// The second output is difficult for vectorization: it writes to the uniform index of the non-simd dimension,
// so the write goes to the same location for all instances of the same dimension-2-id (this would be a data 
// race if the value was not the same for all instances).
__kernel void Test2D2(
   __global float* input,
   __global float* output,
   __global float* output2)
{
	int i = get_global_id(0);
	int j = get_global_id(1);

	const int idx = j + i*get_global_size(1);
	output[idx] = input[i] + input[j];
	output2[j] = input[0];
}
