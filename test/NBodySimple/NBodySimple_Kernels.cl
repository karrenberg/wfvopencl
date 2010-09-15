/*
 * For a description of the algorithm and the terms used, please see the
 * documentation for this sample.
 *
 * Each work-item invocation of this kernel, calculates the position for 
 * one particle
 *
 * Work-items use local memory to reduce memory bandwidth and reuse of data
 */



__kernel

void 

nbody_sim(
    __global float* posX,
    __global float* posY,
    __global float* posZ,
    __global float* posW,
    __global float* velX,
    __global float* velY,
    __global float* velZ,
    int numBodies,
    float deltaTime,
    float epsSqr,
    __local float* localPosX,
    __local float* localPosY,
    __local float* localPosZ,
    __local float* localPosW,
    __global float* newPositionX,
    __global float* newPositionY,
    __global float* newPositionZ,
    __global float* newPositionW,
    __global float* newVelocityX,
    __global float* newVelocityY,
    __global float* newVelocityZ)

{
    unsigned int tid = get_local_id(0);
    unsigned int gid = get_global_id(0);
    unsigned int localSize = get_local_size(0);

    // Number of tiles we need to iterate
    unsigned int numTiles = numBodies / localSize;

    // position of this work-item
    float myPosX = posX[gid];
    float myPosY = posY[gid];
    float myPosZ = posZ[gid];
    float myPosW = posW[gid];
    float accX = 0.0f;
    float accY = 0.0f;
    float accZ = 0.0f;

    for(int i = 0; i < numTiles; ++i)
    {
        // load one tile into local memory
        int idx = i * localSize + tid;
        localPosX[tid] = posX[idx];
        localPosY[tid] = posY[idx];
        localPosZ[tid] = posZ[idx];
        localPosW[tid] = posW[idx];

        // Synchronize to make sure data is available for processing
        barrier(CLK_LOCAL_MEM_FENCE);

        // calculate acceleration effect due to each body
        // a[i->j] = m[j] * r[i->j] / (r^2 + epsSqr)^(3/2)
        for(int j = 0; j < localSize; ++j)
        {
            // Calculate acceleration caused by particle j on particle i
            float rX = localPosX[j] - myPosX;
            float rY = localPosY[j] - myPosY;
            float rZ = localPosZ[j] - myPosZ;
            float distSqr = rX * rX  +  rY * rY  +  rZ * rZ;
            float invDist = 1.0f / sqrt(distSqr + epsSqr);
            float invDistCube = invDist * invDist * invDist;
            float s = localPosW[j] * invDistCube;

            // accumulate effect of all particles
            accX += s * rX;
            accY += s * rY;
            accZ += s * rZ;
        }

        // Synchronize so that next tile can be loaded
        barrier(CLK_LOCAL_MEM_FENCE); // not required? (at least scalar, single-threaded mode passes)
    }

    float oldVelX = velX[gid];
    float oldVelY = velY[gid];
    float oldVelZ = velZ[gid];

    // updated position and velocity
    float newPosX = myPosX + oldVelX * deltaTime + accX * 0.5f * deltaTime * deltaTime;
    float newPosY = myPosY + oldVelY * deltaTime + accY * 0.5f * deltaTime * deltaTime;
    float newPosZ = myPosZ + oldVelZ * deltaTime + accZ * 0.5f * deltaTime * deltaTime;
    float newPosW = myPosW;

    float newVelX = oldVelX + accX * deltaTime;
    float newVelY = oldVelY + accY * deltaTime;
    float newVelZ = oldVelZ + accZ * deltaTime;

    // write to global memory
    newPositionX[gid] = newPosX;
    newPositionY[gid] = newPosY;
    newPositionZ[gid] = newPosZ;
    newPositionW[gid] = newPosW;
    newVelocityX[gid] = newVelX;
    newVelocityY[gid] = newVelY;
    newVelocityZ[gid] = newVelZ;
}
