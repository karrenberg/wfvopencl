// scalar source function
//
__kernel void square(
   __global float* input,
   __global float* output,
   const unsigned int count)
{
	int i = get_global_id(0);
	//printf("global_id: %d\n", i);
	//printf("input: %f\n", input[i]);
	//output[i] = input[i];
	//if(i < count)
		output[i] = input[i] * input[i];
	//printf("result: %f\n", output[i]);
}
