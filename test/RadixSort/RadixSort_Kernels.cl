/*
 * For a description of the algorithm and the terms used, please see the
 * documentation for this sample.
 */

#pragma OPENCL EXTENSION cl_khr_byte_addressable_store : enable 

#define RADIX 8
#define RADICES (1 << RADIX)
#define GROUP_SIZE 16

/**
 * @brief   Calculates block-histogram bin whose bin size is 256
 * @param   unsortedData    array of unsorted elements
 * @param   buckets         histogram buckets    
 * @param   shiftCount      shift count
 * @param   sharedArray     shared array for thread-histogram bins
  */
__kernel
void histogram(__global const uint* unsortedData,
               __global uint* buckets,
               uint shiftCount,
               __local ushort* sharedArray)
{
    size_t localId = get_local_id(0);
    size_t globalId = get_global_id(0);
    size_t groupId = get_group_id(0);
    size_t groupSize = get_local_size(0);

    /* Initialize shared array to zero */
    for(int i = 0; i < RADICES; ++i)
        sharedArray[localId * RADICES + i] = 0;

    barrier(CLK_LOCAL_MEM_FENCE);
    
    /* Calculate thread-histograms */
    for(int i = 0; i < RADICES; ++i)
    {
        uint value = unsortedData[globalId * RADICES + i];
        value = (value >> shiftCount) & 0xFFU;
        sharedArray[localId * RADICES + value]++;
    }
    
    barrier(CLK_LOCAL_MEM_FENCE);
    
    /* Copy calculated histogram bin to global memory */
    for(int i = 0; i < RADICES; ++i)
    {
        uint bucketPos = groupId * RADICES * groupSize + localId * RADICES + i;
        buckets[bucketPos] = sharedArray[localId * RADICES + i];
    }
}

//#define USE_PRESCAN_KERNEL
/**
 * @brief   Permutes the element to appropriate places based on
 *          prescaned buckets values
 * @param   unsortedData        array of unsorted elments
 * @param   prescanedBuckets    prescaned buckets for permuations
 * @param   shiftCount          shift count
 * @param   sharedBuckets       shared array for prescaned buckets
 * @param   sortedData          array for sorted elements
 */
__kernel
void permute(__global const uint* unsortedData,
             __global const uint* prescanedBuckets,
	     __global const uint* prescanedSums,
	     __global const uint* finalSums,
             uint shiftCount,
             __local ushort* sharedBuckets,
             __global uint* sortedData)
{

    size_t groupId = get_group_id(0);
    size_t localId = get_local_id(0);
    size_t globalId = get_global_id(0);
    size_t groupSize = get_local_size(0);
    
    
    /* Copy prescaned thread histograms to corresponding thread shared block */
#ifndef USE_PRESCAN_KERNEL
    for(int i = 0; i < RADICES; ++i)
    {
        uint bucketPos = groupId * RADICES * groupSize + localId * RADICES + i;
        sharedBuckets[localId * RADICES + i] = prescanedBuckets[bucketPos];
    }
#else
    for(int i = 0; i < RADICES; ++i)
    {
        uint bucketPos = groupId * RADICES * groupSize + localId * RADICES + i;
	uint sharedIndex = localId * RADICES + i;
        sharedBuckets[sharedIndex] = prescanedBuckets[bucketPos];
	if(groupId == 0)
	{
	    if(i != 0)
	        sharedBuckets[sharedIndex] += finalSums[i - 1];
	}
	else
	{
	    if(i == 0)
	        sharedBuckets[sharedIndex] += prescanedSums[(groupId - 1) * RADICES + i];
	    else
	        sharedBuckets[sharedIndex] += finalSums[i - 1] + prescanedSums[(groupId - 1) * RADICES + i];
	}
    }
    
#endif //USE_PRESCAN_KERNEL
    barrier(CLK_LOCAL_MEM_FENCE);
    
    /* Premute elements to appropriate location */
    for(int i = 0; i < RADICES; ++i)
    {
        uint value = unsortedData[globalId * RADICES + i];
        value = (value >> shiftCount) & 0xFFU;
        uint index = sharedBuckets[localId * RADICES + value];
        sortedData[index] = unsortedData[globalId * RADICES + i];
        sharedBuckets[localId * RADICES + value] = index + 1;
    }
}

/**
 * @param prescanedArray  prescaned array 
 * @param scanedSums scaned sums 
 * @param sharedArray  local memory used in the kernel
 * @param unscanedBucket unscaned array
 */

__kernel 
void prescan(__global uint *unscanedBucket,
             __local uint *sharedArray,
	     __global uint *prescanedArray,
             __global uint *scanedSums)
{
    size_t localId = get_local_id(0);
    size_t globalId = get_global_id(0);
    size_t groupId = get_group_id(0);
    size_t groupSize = get_local_size(0);
	
    /* load block of unscaned elements in to shared memory */
    int offset = 1;    
    for(int i = 0; i < RADICES; ++i)
    {
	int unscanedIndex = groupId * RADICES *groupSize + localId * RADICES + i;
	int sharedIndex = i * GROUP_SIZE + localId;
        sharedArray[sharedIndex] = unscanedBucket[unscanedIndex];
    }

    barrier(CLK_LOCAL_MEM_FENCE);

    /* build the sum in place up the tree */

    for(int i = 0; i < RADICES; ++i)
    {
       offset = 1;
       for(int d = groupSize >> 1; d > 0; d >>= 1)
       {
	    barrier(CLK_LOCAL_MEM_FENCE);	
	    if(localId < d)
	    {
	      	int ai = offset * (2 * localId + 1) - 1;
		int bi = offset * (2 * localId + 2) - 1;
		
		ai += i * groupSize;
		bi += i * groupSize;

	        sharedArray[bi] += sharedArray[ai];
	    }
            offset *= 2;
        }
    }

    for(int i = 0; i < RADICES / groupSize; ++i)
    {
	int sumsIndex = groupId * RADICES + i * (RADICES / groupSize) + localId;  
	int sharedIndex = (i * (RADICES / GROUP_SIZE)) * groupSize + localId * groupSize + groupSize - 1;
        scanedSums[sumsIndex] = sharedArray[sharedIndex];
	sharedArray[sharedIndex] = 0;
    }

    barrier(CLK_LOCAL_MEM_FENCE | CLK_GLOBAL_MEM_FENCE);

    /* scan back down the tree */

    for(int i = 0; i < RADICES; ++i)
    {
       offset = groupSize;
       /* traverse down the tree building the scan in the place */
       for(int d = 1; d < groupSize; d *= 2)
       {
	   offset >>= 1;
	   barrier(CLK_LOCAL_MEM_FENCE);
		
	   if(localId < d)
	   {
	       int ai = offset * (2 * localId + 1) - 1;
	       int bi = offset * (2 * localId + 2) - 1;
			
	       ai += i * groupSize;
	       bi += i * groupSize;	       
	       
	       uint t = sharedArray[ai];
	       sharedArray[ai] = sharedArray[bi];
	       sharedArray[bi] += t;
	   }
        }
    }
	
    barrier(CLK_LOCAL_MEM_FENCE);	

    /*write the results back to global memory */
    for(int i = 0; i < RADICES; ++i)
    {
        int sharedIndex = i * groupSize + localId;
	int scanedIndex = groupId * RADICES * groupSize + localId * RADICES + i;
	prescanedArray[scanedIndex] = sharedArray[sharedIndex];
    }

}

