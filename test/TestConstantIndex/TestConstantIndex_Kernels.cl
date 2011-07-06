// The desired output is difficult for vectorization: it reads from a constant index
// so alll instances load from the same location while vectorization would create a vector load
// that reads 4 different values.
// Note that this would be a data race if the value was also written somewhere.
__kernel void TestConstantIndex(
   __global float* input,
   __global float* output)
{
#if 1
	output[get_global_id(0)] = input[1];
#else
	output[0] = input[1]; // if this is activated, TestConstantIndex.cpp::verifyResults() also has to be adjusted!
#endif
}
