/*
 * For a description of the algorithm and the terms used, please see the
 * documentation for this sample.
 *
 * Each group invocation of this kernel, calculates the option value for a 
 * given stoke price, option strike price, time to expiration date, risk 
 * free interest and volatility factor.
 *
 * Multiple groups calculate the same with different input values.
 * Number of work-items in each group is same as number of leaf nodes
 * So, the maximum number of steps is limited by loca memory size available
 *
 * Each work-item calculate the leaf-node value and update local memory. 
 * These leaf nodes are further reduced to the root of the tree (time step 0). 
 */

#define RISKFREE 0.02f
#define VOLATILITY 0.30f

__kernel
void 
binomial_options(
    int numSteps, // replace by get_local_id(0) !
    const __global float* randArray,
    __global float* output,
    __local float* callA,
    __local float* callB)
{
    // load shared mem
    unsigned int tid = get_local_id(0);
    unsigned int bid = get_group_id(0);

    float inRand = randArray[bid];

    float s = (1.0f - inRand) * 5.0f + inRand * 30.f;
    float x = (1.0f - inRand) * 1.0f + inRand * 100.f;
    float optionYears = (1.0f - inRand) * 0.25f + inRand * 10.f; 
    float dt = optionYears * (1.0f / (float)numSteps);
    float vsdt = VOLATILITY * sqrt(dt);
    float rdt = RISKFREE * dt;
    float r = exp(rdt);
    float rInv = 1.0f / r;
    float u = exp(vsdt);
    float d = 1.0f / u;
    float pu = (r - d)/(u - d);
    float pd = 1.0f - pu;
    float puByr = pu * rInv;
    float pdByr = pd * rInv;

    float profit = s * exp(vsdt * (2.0f * tid - (float)numSteps)) - x;
    callA[tid] = profit > 0 ? profit : 0.0f;

    barrier(CLK_LOCAL_MEM_FENCE);

    for(int j = numSteps; j > 0; j -= 2)
    {
        if(tid < j)
        {
            callB[tid] = puByr * callA[tid] + pdByr * callA[tid + 1];
        }
        barrier(CLK_LOCAL_MEM_FENCE);

        if(tid < j - 1)
        {
            callA[tid] = puByr * callB[tid] + pdByr * callB[tid + 1];
        }
        barrier(CLK_LOCAL_MEM_FENCE);
    }

    // write result for this block to global mem
    if(tid == 0) output[bid] = callA[0];
}
