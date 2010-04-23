/**
*  A fractal generator that calculates the mandlebrot set 
*  http://en.wikipedia.org/wiki/Mandelbrot_set 
* @param mandelbrotImage    mandelbrot images is stored in this
* @param scale              Represents the distance from which the fractal 
*                           is being seen if this is greater more area and 
*                           less detail is seen           
* @param maxIterations      More iterations gives more accurate mandelbrot image 
* @param width              size of the image 
*/

__kernel void mandelbrot (__global int * mandelbrotImage,
                          const    float scale, 
                          const    uint maxIterations,
                          const    int width
                          )
{
    int tid = get_global_id(0);
	printf("tid = %d\n", tid);
    
    int i = tid%width;
    int j = tid/width; 
    
    float x0 = ((i*scale) - ((scale/2)*width))/width;
    float y0 = ((j*scale) - ((scale/2)*width))/width;
    
    float x = x0;
    float y = y0;
    
    float x2 = x*x;
    float y2 = y*y;
    
    float scaleSquare = scale * scale;
    
    uint iter=0;
    for(iter=0; (x2+y2 <= scaleSquare) && (iter < maxIterations); ++iter)
    {
        y = 2 * x * y + y0;
        x = x2 - y2   + x0;
        
        x2 = x*x;
        y2 = y*y;
    }
    mandelbrotImage[tid] = 255*iter/maxIterations;    
}


__kernel void mandelbrot_SIMD (__global int4 * mandelbrotImage,
                          const    float scale,
                          const    uint maxIterations,
                          const    int width
                          );


// native simd function declarations for get_global_id
//
//uint4 get_global_id_SIMD(uint);
uint get_global_id_SIMD(uint);
uint4 get_local_id_SIMD(uint);


// fake call function to prevent deletion of declarations
// must never be called!
//
void __fakeCall(__global float4 * f4, __global int4 * i4, const uint4 ui4, const unsigned z) {
	mandelbrot_SIMD(i4, 0.f, 1, 1);
	get_global_id_SIMD(0);
	get_local_id_SIMD(0);
}
