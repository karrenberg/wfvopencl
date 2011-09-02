// Use this kernel with combinations of packetizer DISABLE_MEM_VEC and DYNCHECK flags
__kernel 
void TestDynCheckSpeed(__global int * output,
         __global int * input,
         __global float * dct8x8,
         __local  float * inter,
         const    uint    width,
         const    uint  blockWidth,
         const    uint    inverse)

{
    /* get global indices of the element */
    uint globalIdx = get_global_id(0);
    
    //output[globalIdx] = ((globalIdx << 3) + 1) & 15; // always consecutive
    output[globalIdx*2] = ((globalIdx << 3) + 1) & 15; // always non-consecutive
}
