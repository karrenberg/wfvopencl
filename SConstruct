import os
#env = Environment(ENV = {'PATH'            : os.environ['PATH'],
#						 'LD_LIBRARY_PATH' : os.environ['LD_LIBRARY_PATH']})

env = Environment(ENV = os.environ)

env['CC'] = 'clang'
env['CXX'] = 'clang++'

#cxxflags = env.Split("-O3 -msse2")
cxxflags = env.Split("-O0 -g -DDEBUG -Wall -pedantic -msse3")
env.Append(CXXFLAGS = cxxflags)

env.Append(CPPPATH = env.Split("-Iinclude"))
env.Append(LIBPATH = env.Split("-Llib"))

#env.Command(os.path.join(env['ENV']['JITRT_INC'], 'jitRT/llvmWrapper.h'), 'include/jitRT/llvmWrapper.h', "cp $SOURCE $TARGET")

# build SSE OpenCL Driver
SSEOpenCLDriver = env.Object(target='build/obj/sseOpenCLDriver', source='src/sseOpenCLDriver.cpp')

# build AMD-ATI SDKUtil as a static library
SDKUtil = env.StaticLibrary(target='lib/SDKUtil', source=Split('test/SDKUtil/SDKApplication.cpp test/SDKUtil/SDKBitMap.cpp test/SDKUtil/SDKCommandArgs.cpp test/SDKUtil/SDKCommon.cpp test/SDKUtil/SDKFile.cpp'))


# build simpleTest
#env.Append(LIBPATH = env['ENV']['JITRT_LIB'])
env.Object(target='build/obj/simpleTest', source='test/simpleTest.cpp')
env.Program(target='build/bin/simpleTest', source=Split('build/obj/simpleTest.o build/obj/sseOpenCLDriver.o'), LIBS=['jitRT'])

# build NBody simulation
env.Object(target='build/obj/NBody', source='test/NBody.cpp')
Test_NBody = env.Program(target='build/bin/NBody', source=Split('build/obj/NBody.o build/obj/sseOpenCLDriver.o'), LIBS=['jitRT', 'SDKUtil', 'glut'])
env.Depends(Test_NBody, SDKUtil)

# build Mandelbrot application
env.Object(target='build/obj/Mandelbrot', source='test/Mandelbrot.cpp')
env.Object(target='build/obj/MandelbrotDisplay', source='test/MandelbrotDisplay.cpp')
Test_Mandelbrot = env.Program(target='build/bin/Mandelbrot', source=Split('build/obj/Mandelbrot.o build/obj/MandelbrotDisplay.o build/obj/sseOpenCLDriver.o'), LIBS=['jitRT', 'SDKUtil', 'glut', 'GLEW'])
env.Depends(Test_Mandelbrot, SDKUtil)

# build SimpleConvolution application
env.Object(target='build/obj/SimpleConvolution', source='test/SimpleConvolution.cpp')
Test_SimpleConvolution = env.Program(target='build/bin/SimpleConvolution', source=Split('build/obj/SimpleConvolution.o build/obj/sseOpenCLDriver.o'), LIBS=['jitRT', 'SDKUtil'])
env.Depends(Test_SimpleConvolution, SDKUtil)

# build FastWalshTransform application
env.Object(target='build/obj/FastWalshTransform', source='test/FastWalshTransform.cpp')
Test_FastWalshTransform = env.Program(target='build/bin/FastWalshTransform', source=Split('build/obj/FastWalshTransform.o build/obj/sseOpenCLDriver.o'), LIBS=['jitRT', 'SDKUtil'])
env.Depends(Test_FastWalshTransform, SDKUtil)

# build BitonicSort application
env.Object(target='build/obj/BitonicSort', source='test/BitonicSort.cpp')
Test_BitonicSort = env.Program(target='build/bin/BitonicSort', source=Split('build/obj/BitonicSort.o build/obj/sseOpenCLDriver.o'), LIBS=['jitRT', 'SDKUtil'])
env.Depends(Test_BitonicSort, SDKUtil)


# build bitcode from OpenCL files

# NOTE: using --march automatically generates a stub-function
# NOTE: using --march also inserts calls to builtin functions instead of e.g. printf
# NOTE: --march=x86-64 generates bad code for packetization :(
#       --march-x86 (or left out) generates 32bit data structures etc., making wrapper unusable

#env.Command('simpleTest_Kernels.ll', 'test/simpleTest_Kernels.cl', "clc --march=x86-64 --msse2 $SOURCE")
#env.Command('simpleTest_Kernels.ll', 'test/simpleTest_Kernels.cl', "clc --march=x86 --msse2 $SOURCE")
env.Command('simpleTest_Kernels.ll', 'test/simpleTest_Kernels.cl', "clc --msse2 $SOURCE")
env.Command('simpleTest_Kernels.bc', 'simpleTest_Kernels.ll', "llvm-as $SOURCE -o $TARGET")

env.Command('NBody_Kernels.ll', 'test/NBody_Kernels.cl', "clc --msse2 $SOURCE")
env.Command('NBody_Kernels.bc', 'NBody_Kernels.ll', "llvm-as $SOURCE -o $TARGET")

env.Command('Mandelbrot_Kernels.ll', 'test/Mandelbrot_Kernels.cl', "clc --msse2 $SOURCE")
env.Command('Mandelbrot_Kernels.bc', 'Mandelbrot_Kernels.ll', "llvm-as $SOURCE -o $TARGET")

env.Command('SimpleConvolution_Kernels.ll', 'test/SimpleConvolution_Kernels.cl', "clc --msse2 $SOURCE")
env.Command('SimpleConvolution_Kernels.bc', 'SimpleConvolution_Kernels.ll', "llvm-as $SOURCE -o $TARGET")

env.Command('FastWalshTransform_Kernels.ll', 'test/FastWalshTransform_Kernels.cl', "clc --msse2 $SOURCE")
env.Command('FastWalshTransform_Kernels.bc', 'FastWalshTransform_Kernels.ll', "llvm-as $SOURCE -o $TARGET")

env.Command('BitonicSort_Kernels.ll', 'test/BitonicSort_Kernels.cl', "clc --msse2 $SOURCE")
env.Command('BitonicSort_Kernels.bc', 'BitonicSort_Kernels.ll', "llvm-as $SOURCE -o $TARGET")
