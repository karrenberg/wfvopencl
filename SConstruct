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

env.Object(target='build/obj/sseOpenCLDriver', source='src/sseOpenCLDriver.cpp')


# build simpleTest
#env.Append(LIBPATH = env['ENV']['JITRT_LIB'])
env.Object(target='build/obj/simpleTest', source='test/simpleTest.cpp')
env.Program(target='build/bin/simpleTest', source=Split('build/obj/simpleTest.o build/obj/sseOpenCLDriver.o'), LIBS=['jitRT'])

# build NBody simulation
env.Object(target='build/obj/NBody', source='test/NBody.cpp')
env.Program(target='build/bin/NBody', source=Split('build/obj/NBody.o build/obj/sseOpenCLDriver.o'), LIBS=['glut', 'jitRT'])

# build Mandelbrot application
env.Object(target='build/obj/Mandelbrot', source='test/Mandelbrot.cpp')
env.Object(target='build/obj/MandelbrotDisplay', source='test/MandelbrotDisplay.cpp')
env.Program(target='build/bin/Mandelbrot', source=Split('build/obj/Mandelbrot.o build/obj/MandelbrotDisplay.o build/obj/sseOpenCLDriver.o'), LIBS=['jitRT', 'glut', 'GLEW'])

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

