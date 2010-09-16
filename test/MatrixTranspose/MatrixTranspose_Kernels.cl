/*
 * Copies a block to the local memory 
 * and copies back the transpose from local memory to output
 * @param output output matrix
 * @param input  input matrix
 * @param block  local memory of size blockSize x blockSize
 * @param width  width of the input matrix
 * @param height height of the input matrix
 * @param blockSize size of the block
 */

#define USE_PACKETIZER
#define OPTIMIZE_BARRIER_LIVE_VALUES

__kernel 
void matrixTranspose(__global float * output,
                     __global float * input,
                     __local  float * block,
                     const    uint    width,
                     const    uint    height,
                     const    uint blockSize
                       )
{
#ifdef USE_PACKETIZER
#ifdef OPTIMIZE_BARRIER_LIVE_VALUES
	/* copy from input to local memory */
	block[get_local_id(1)*get_local_size(0) + get_local_id(0)] = input[get_global_id(1)*get_global_size(0) + get_global_id(0)];

    /* wait until the whole block is filled */
	barrier(CLK_LOCAL_MEM_FENCE);

    /* calculate the corresponding target location for transpose  by inverting x and y values*/
	//const uint targetGlobalIdx = get_global_id(1);//get_group_id(1)*get_local_size(0) + get_local_id(1);
	//const uint targetGlobalIdy = get_global_id(0);//get_group_id(0)*get_local_size(0) + get_local_id(0);

    /* calculate the corresponding raster indices of source and target */
	const uint targetIndex = get_global_id(0)*get_global_size(1) + get_global_id(1);
	const uint sourceIndex = get_local_id(1)*get_local_size(0)  + get_local_id(0);

	output[targetIndex] = block[sourceIndex];
#else
	// This demonstrates the overhead of the barrier-live values
	uint globalIdx = get_global_id(0);
	uint globalIdy = get_global_id(1);
	uint localIdx = get_local_id(0);
	uint localIdy = get_local_id(1);
	block[localIdy*get_local_size(0) + localIdx] = input[globalIdy*get_global_size(0) + globalIdx];
	barrier(CLK_LOCAL_MEM_FENCE);
	const uint targetIndex = globalIdx*get_global_size(1) + globalIdy;
	const uint sourceIndex = localIdy*get_local_size(0)  + localIdx;

	output[targetIndex] = block[sourceIndex];
#endif
#else
	uint globalIdx = get_global_id(0);
	uint globalIdy = get_global_id(1);
	
	uint localIdx = get_local_id(0);
	uint localIdy = get_local_id(1);
	
    /* copy from input to local memory */
	block[localIdy*blockSize + localIdx] = input[globalIdy*width + globalIdx];

    /* wait until the whole block is filled */
	barrier(CLK_LOCAL_MEM_FENCE);

	uint groupIdx = get_group_id(0);
	uint groupIdy = get_group_id(1);

    /* calculate the corresponding target location for transpose  by inverting x and y values*/
	uint targetGlobalIdx = groupIdy*blockSize + localIdy;
	uint targetGlobalIdy = groupIdx*blockSize + localIdx;

    /* calculate the corresponding raster indices of source and target */
	uint targetIndex  = targetGlobalIdy*height     + targetGlobalIdx;
	uint sourceIndex  = localIdy       * blockSize + localIdx;
	
	output[targetIndex] = block[sourceIndex];
#endif
}
