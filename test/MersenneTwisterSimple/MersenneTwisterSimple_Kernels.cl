/*
 * Implemented Gaussian Random Number Generator. SIMD-oriented Fast Mersenne
 * Twister(SFMT) used to generate random numbers and Box mullar transformation used
 * to convert them to Gaussian random numbers.
 * 
 * The SFMT algorithm details could be found at 
 * http://www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/SFMT/index.html
 *
 * Box-Muller Transformation 
 * http://mathworld.wolfram.com/Box-MullerTransformation.html
 *
 * One invocation of this kernel(gaussianRand), i.e one work thread writes
 * mulFactor output values.
 */

/**
 * @brief Generates gaussian random numbers by using 
 *        Mersenenne Twister algo and box muller transformation
 * @param seedArray  array of seeds
 * @param width width of 2D seedArray
 * @param mulFactor No of generated random numbers for each seed
 * @param randArray  array of random number from given seeds
 */
__kernel
void gaussianRand(const __global uint *seedArray,
            uint width,
            uint mulFactor,
            __global float *gaussianRand)
{

    uint temp[8];
    
    size_t xPid = get_global_id(0);
    size_t yPid = get_global_id(1);
    
    uint state1 = seedArray[yPid *width + xPid];
    uint state2 = (uint)(0); 
    uint state3 = (uint)(0); 
    uint state4 = (uint)(0); 
    uint state5 = (uint)(0);
    
    uint stateMask = 1812433253u;

    uint r1 = (uint)(0);
    uint r2 = (uint)(0);

    uint a = (uint)(0);
    uint b = (uint)(0); 

    uint e = (uint)(0); 
    uint f = (uint)(0); 
    
    unsigned int thirteen  = 13u;
    unsigned int fifteen = 15u;
    unsigned int shift = 8u * 3u;

    unsigned int mask11 = 0xfdff37ffu;
    unsigned int mask12 = 0xef7f3f7du;
    unsigned int mask13 = 0xff777b7du;
    unsigned int mask14 = 0x7ff7fb2fu;
    
    size_t actualPos = (size_t)0;
    
    
    const float intMax = 4294967296.0f;
    const float PI = 3.14159265358979f;

    float r; 
    float phi;

    float temp1;
    float temp2;
    
    //Initializing states.
    state2 = stateMask * (state1 ^ (state1 >> 30u)) + 1u;
    state3 = stateMask * (state2 ^ (state2 >> 30u)) + 2u;
    state4 = stateMask * (state3 ^ (state3 >> 30u)) + 3u;
    state5 = stateMask * (state4 ^ (state4 >> 30u)) + 4u;
    
    uint i = 0;
    for(i = 0; i < mulFactor; ++i)
    {
        switch(i)
        {
            case 0:
                r1 = state4;
                r2 = state5;
                a = state1;
                b = state3;
                break;
            case 1:
                r1 = r2;
                r2 = temp[0];
                a = state2;
                b = state4;
                break;
            case 2:
                r1 = r2;
                r2 = temp[1];
                a = state3;
                b = state5;
                break;
            case 3:
                r1 = r2;
                r2 = temp[2];
                a = state4;
                b = state1;
                break;
            case 4:
                r1 = r2;
                r2 = temp[3];
                a = state5;
                b = state2;            
                break;
            case 5:
                r1 = r2;
                r2 = temp[4];
                a = temp[0];
                b = temp[2];            
                break;
            case 6:
                r1 = r2;
                r2 = temp[5];
                a = temp[1];
                b = temp[3];            
                break;
            case 7:
                r1 = r2;
                r2 = temp[6];
                a = temp[2];
                b = temp[4];            
                break;
            default:
                break;            
                
        }
        
        e = a << shift;
        f = r1 >> shift;

        temp[i] = a ^ e ^ ((b >> thirteen) & mask11) ^ f ^ (r2 << fifteen);
    }        

    actualPos = (yPid * width + xPid) * mulFactor;    

    for(i = 0; i < mulFactor / 2; ++i)
    {
        temp1 = temp[i] / intMax;
        temp2 = temp[i + 1] / intMax;
        
        // Applying Box Mullar Transformations.
        r = sqrt((-2.f) * log(temp1));
        phi  = 2.f * PI * temp2;
        gaussianRand[actualPos + i * 2 + 0] = r * cos(phi);
        gaussianRand[actualPos + i * 2 + 1] = r * sin(phi);
    }
}
