// This kernel tests whether access to arrays via linear
// combinations of local/global id and per-group-constant
// values is correctly combined to a packet load.
// An Analysis has to prove that the access is linear,
// i.e. the accessed elements are stored consecutively,
// and that the access is exactly at a simd group boundary.
__kernel void TestLinearAccess(
   __global float* input,
   __global float* output)
{
	int gid = get_global_id(0);
	int lid = get_local_id(0);
	int bid = get_group_id(0);

	int gsize = get_global_size(0);
	int lsize = get_local_size(0);

	int inidx = gid;
	int outidx = lid + bid * lsize; // = gid
	//int outidx = lid + bid * lsize * 4;
	
	output[outidx] = input[inidx];
}
