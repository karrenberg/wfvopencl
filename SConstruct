import os
#env = Environment(ENV = {'PATH'            : os.environ['PATH'],
#						 'LD_LIBRARY_PATH' : os.environ['LD_LIBRARY_PATH']})

# simply clone entire environment
env = Environment(ENV = os.environ)

#env['CC'] = 'clang'
#env['CXX'] = 'clang++'
env['CC'] = 'gcc'
env['CXX'] = 'g++'

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

# build BitonicSort application
Test_BitonicSort = env.Program(target='build/bin/BitonicSort', source=PacketizedOpenCLDriver+Split('test/BitonicSort.cpp'), LIBS=['jitRT', 'SDKUtil'])

# build DwtHaar1D application
Test_DwtHaar1D = env.Program(target='build/bin/DwtHaar1D', source=PacketizedOpenCLDriver+Split('test/DwtHaar1D.cpp'), LIBS=['jitRT', 'SDKUtil'])

# build EigenValue application
Test_EigenValue = env.Program(target='build/bin/EigenValue', source=PacketizedOpenCLDriver+Split('test/EigenValue.cpp'), LIBS=['jitRT', 'SDKUtil'])

# build FastWalshTransform application
Test_FastWalshTransform = env.Program(target='build/bin/FastWalshTransform', source=PacketizedOpenCLDriver+Split('test/FastWalshTransform.cpp'), LIBS=['jitRT', 'SDKUtil'])

# build FloydWarshall application
Test_FloydWarshall = env.Program(target='build/bin/FloydWarshall', source=PacketizedOpenCLDriver+Split('test/FloydWarshall.cpp'), LIBS=['jitRT', 'SDKUtil'])

# build Histogram application
Test_Histogram = env.Program(target='build/bin/Histogram', source=PacketizedOpenCLDriver+Split('test/Histogram.cpp'), LIBS=['jitRT', 'SDKUtil'])

# build Mandelbrot application
Test_Mandelbrot = env.Program(target='build/bin/Mandelbrot', source=PacketizedOpenCLDriver+Split('test/Mandelbrot.cpp test/MandelbrotDisplay.cpp'), LIBS=['jitRT', 'SDKUtil', 'glut', 'GLEW'])

# build MatrixTranspose application
Test_MatrixTranspose = env.Program(target='build/bin/MatrixTranspose', source=PacketizedOpenCLDriver+Split('test/MatrixTranspose.cpp'), LIBS=['jitRT', 'SDKUtil'])

# build NBody simulation -> uses float4 -> can not be used with packetization atm
Test_NBody = env.Program(target='build/bin/NBody', source=PacketizedOpenCLDriver+Split('test/NBody.cpp'), LIBS=['jitRT', 'SDKUtil', 'glut'])

# build PrefixSum application
Test_PrefixSum = env.Program(target='build/bin/PrefixSum', source=PacketizedOpenCLDriver+Split('test/PrefixSum.cpp'), LIBS=['jitRT', 'SDKUtil'])

# build RadixSort application
Test_RadixSort = env.Program(target='build/bin/RadixSort', source=PacketizedOpenCLDriver+Split('test/RadixSort.cpp'), LIBS=['jitRT', 'SDKUtil'])

# build ScanLargeArrays application
Test_ScanLargeArrays = env.Program(target='build/bin/ScanLargeArrays', source=PacketizedOpenCLDriver+Split('test/ScanLargeArrays.cpp'), LIBS=['jitRT', 'SDKUtil'])

# build SimpleConvolution application
Test_SimpleConvolution = env.Program(target='build/bin/SimpleConvolution', source=PacketizedOpenCLDriver+Split('test/SimpleConvolution.cpp'), LIBS=['jitRT', 'SDKUtil'])




#### dependencies ###
env.Depends(Test_BitonicSort, SDKUtil)
env.Depends(Test_DwtHaar1D, SDKUtil)
env.Depends(Test_EigenValue, SDKUtil)
env.Depends(Test_FastWalshTransform, SDKUtil)
env.Depends(Test_FloydWarshall, SDKUtil)
env.Depends(Test_Histogram, SDKUtil)
env.Depends(Test_Mandelbrot, SDKUtil)
env.Depends(Test_MatrixTranspose, SDKUtil)
env.Depends(Test_NBody, SDKUtil)
env.Depends(Test_PrefixSum, SDKUtil)
env.Depends(Test_RadixSort, SDKUtil)
env.Depends(Test_ScanLargeArrays, SDKUtil)
env.Depends(Test_SimpleConvolution, SDKUtil)



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

env.Command('BitonicSort_Kernels.ll', 'test/BitonicSort_Kernels.cl', "clc --msse2 $SOURCE")
env.Command('BitonicSort_Kernels.bc', 'BitonicSort_Kernels.ll', "llvm-as $SOURCE -o $TARGET")

env.Command('DwtHaar1D_Kernels.ll', 'test/DwtHaar1D_Kernels.cl', "clc --msse2 $SOURCE")
env.Command('DwtHaar1D_Kernels.bc', 'DwtHaar1D_Kernels.ll', "llvm-as $SOURCE -o $TARGET")

env.Command('EigenValue_Kernels.ll', 'test/EigenValue_Kernels.cl', "clc --msse2 $SOURCE")
env.Command('EigenValue_Kernels.bc', 'EigenValue_Kernels.ll', "llvm-as $SOURCE -o $TARGET")

env.Command('FastWalshTransform_Kernels.ll', 'test/FastWalshTransform_Kernels.cl', "clc --msse2 $SOURCE")
env.Command('FastWalshTransform_Kernels.bc', 'FastWalshTransform_Kernels.ll', "llvm-as $SOURCE -o $TARGET")

env.Command('FloydWarshall_Kernels.ll', 'test/FloydWarshall_Kernels.cl', "clc --msse2 $SOURCE")
env.Command('FloydWarshall_Kernels.bc', 'FloydWarshall_Kernels.ll', "llvm-as $SOURCE -o $TARGET")

env.Command('Histogram_Kernels.ll', 'test/Histogram_Kernels.cl', "clc --msse2 $SOURCE")
env.Command('Histogram_Kernels.bc', 'Histogram_Kernels.ll', "llvm-as $SOURCE -o $TARGET")

env.Command('Mandelbrot_Kernels.ll', 'test/Mandelbrot_Kernels.cl', "clc --msse2 $SOURCE")
env.Command('Mandelbrot_Kernels.bc', 'Mandelbrot_Kernels.ll', "llvm-as $SOURCE -o $TARGET")

env.Command('MatrixTranspose_Kernels.ll', 'test/MatrixTranspose_Kernels.cl', "clc --msse2 $SOURCE")
env.Command('MatrixTranspose_Kernels.bc', 'MatrixTranspose_Kernels.ll', "llvm-as $SOURCE -o $TARGET")

env.Command('NBody_Kernels.ll', 'test/NBody_Kernels.cl', "clc --msse2 $SOURCE")
env.Command('NBody_Kernels.bc', 'NBody_Kernels.ll', "llvm-as $SOURCE -o $TARGET")

env.Command('PrefixSum_Kernels.ll', 'test/PrefixSum_Kernels.cl', "clc --msse2 $SOURCE")
env.Command('PrefixSum_Kernels.bc', 'PrefixSum_Kernels.ll', "llvm-as $SOURCE -o $TARGET")

env.Command('RadixSort_Kernels.ll', 'test/RadixSort_Kernels.cl', "clc --msse2 $SOURCE")
env.Command('RadixSort_Kernels.bc', 'RadixSort_Kernels.ll', "llvm-as $SOURCE -o $TARGET")

env.Command('ScanLargeArrays_Kernels.ll', 'test/ScanLargeArrays_Kernels.cl', "clc --msse2 $SOURCE")
env.Command('ScanLargeArrays_Kernels.bc', 'ScanLargeArrays_Kernels.ll', "llvm-as $SOURCE -o $TARGET")

env.Command('SimpleConvolution_Kernels.ll', 'test/SimpleConvolution_Kernels.cl', "clc --msse2 $SOURCE")
env.Command('SimpleConvolution_Kernels.bc', 'SimpleConvolution_Kernels.ll', "llvm-as $SOURCE -o $TARGET")


#env.Command(os.path.join(env['ENV']['JITRT_INC'], 'jitRT/llvmWrapper.h'), 'include/jitRT/llvmWrapper.h', "cp $SOURCE $TARGET")
#env.Append(LIBPATH = env['ENV']['JITRT_LIB'])
