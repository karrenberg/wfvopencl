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
