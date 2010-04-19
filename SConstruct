import os
#env = Environment(ENV = {'PATH'            : os.environ['PATH'],
#						 'LD_LIBRARY_PATH' : os.environ['LD_LIBRARY_PATH']})

env = Environment(ENV = os.environ)

env['CC'] = 'clang'
env['CXX'] = 'clang++'

#cxxflags = env.Split("-O3 -msse2")
cxxflags = env.Split("-O0 -g -DDEBUG -Wall -pedantic")
env.Append(CXXFLAGS = cxxflags)

env.Append(CPPPATH = env.Split("-Iinclude"))

#env.Command(os.path.join(env['ENV']['JITRT_INC'], 'jitRT/llvmWrapper.h'), 'include/jitRT/llvmWrapper.h', "cp $SOURCE $TARGET")

env.Object(target='build/obj/sseOpenCLDriver', source='src/sseOpenCLDriver.cpp')
env.Object(target='build/obj/simpleTest', source='test/simpleTest.cpp')

env.Append(LIBPATH = env['ENV']['JITRT_LIB'])
env.Program(target='build/bin/simpleTest', source=Split('build/obj/simpleTest.o build/obj/sseOpenCLDriver.o'), LIBS=['jitRT'])

# build bitcode from OpenCL file
env.Command('simpleTest.ll', 'test/simpleTest.cl', "clc --march=x86-64 --msse2 $SOURCE")
env.Command('simpleTest.bc', 'simpleTest.ll', "llvm-as $SOURCE -o $TARGET")
