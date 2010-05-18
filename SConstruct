import os
#env = Environment(ENV = {'PATH'            : os.environ['PATH'],
#						 'LD_LIBRARY_PATH' : os.environ['LD_LIBRARY_PATH']})

# simply clone entire environment
env = Environment(ENV = os.environ)

env['CC'] = 'clang'
env['CXX'] = 'clang++'
#env['CC'] = 'gcc'
#env['CXX'] = 'g++'

# TODO: add debug/opt mode
#cxxflags = env.Split("-O3 -msse2")
cxxflags = env.Split("-O0 -g -DDEBUG -Wall -pedantic -msse3 -fopenmp")
env.Append(CXXFLAGS = cxxflags)

env.Append(CPPPATH = env.Split("include"))
env.Append(LIBPATH = env.Split("lib"))
env.Append(LINKFLAGS = env.Split("-fopenmp"))

# build Packetized OpenCL Driver
PacketizedOpenCLDriver = env.Object(target='build/obj/packetizedOpenCLDriver', source='src/packetizedOpenCLDriver.cpp')


# build AMD-ATI SDKUtil as a static library (required for test applications)
SDKUtil = env.StaticLibrary(target='lib/SDKUtil', source=Split('test/SDKUtil/SDKApplication.cpp test/SDKUtil/SDKBitMap.cpp test/SDKUtil/SDKCommandArgs.cpp test/SDKUtil/SDKCommon.cpp test/SDKUtil/SDKFile.cpp'))
env.Depends(SDKUtil, PacketizedOpenCLDriver)

###
### test applications ###
###

# build simpleTest
env.Program(target='build/bin/simpleTest', source=PacketizedOpenCLDriver+Split('test/simpleTest.cpp'), LIBS=['jitRT'])

# build NBody simulation
Test_NBody = env.Program(target='build/bin/NBody', source=PacketizedOpenCLDriver+Split('test/NBody.cpp'), LIBS=['jitRT', 'SDKUtil', 'glut'])
env.Depends(Test_NBody, SDKUtil)

# build Mandelbrot application
Test_Mandelbrot = env.Program(target='build/bin/Mandelbrot', source=PacketizedOpenCLDriver+Split('test/Mandelbrot.cpp test/MandelbrotDisplay.cpp'), LIBS=['jitRT', 'SDKUtil', 'glut', 'GLEW'])
env.Depends(Test_Mandelbrot, SDKUtil)

# build SimpleConvolution application
Test_SimpleConvolution = env.Program(target='build/bin/SimpleConvolution', source=PacketizedOpenCLDriver+Split('test/SimpleConvolution.cpp'), LIBS=['jitRT', 'SDKUtil'])
env.Depends(Test_SimpleConvolution, SDKUtil)

# build FastWalshTransform application
Test_FastWalshTransform = env.Program(target='build/bin/FastWalshTransform', source=PacketizedOpenCLDriver+Split('test/FastWalshTransform.cpp'), LIBS=['jitRT', 'SDKUtil'])
env.Depends(Test_FastWalshTransform, SDKUtil)

# build BitonicSort application
Test_BitonicSort = env.Program(target='build/bin/BitonicSort', source=PacketizedOpenCLDriver+Split('test/BitonicSort.cpp'), LIBS=['jitRT', 'SDKUtil'])
env.Depends(Test_BitonicSort, SDKUtil)

###
### build bitcode from OpenCL files ###
###

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



#env.Command(os.path.join(env['ENV']['JITRT_INC'], 'jitRT/llvmWrapper.h'), 'include/jitRT/llvmWrapper.h', "cp $SOURCE $TARGET")
#env.Append(LIBPATH = env['ENV']['JITRT_LIB'])
