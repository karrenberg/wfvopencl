import os
#env = Environment(ENV = {'PATH'            : os.environ['PATH'],
#						 'LD_LIBRARY_PATH' : os.environ['LD_LIBRARY_PATH']})

env = Environment(ENV = os.environ)

env['CC'] = '/local/karrenberg/proj/anysl/install/build-gcc-opt/llvm/bin/clang'
env['CXX'] = '/local/karrenberg/proj/anysl/install/build-gcc-opt/llvm/bin/clang++'

cxxflags = env.Split("-O3 -Wall -pedantic -Iinclude")
env.Append(CXXFLAGS = cxxflags)

env.Append(CPPPATH = env.Split("-Iinclude"))

env.Object(target='build/obj/sseOpenCLDriver', source='src/sseOpenCLDriver.cpp')
env.Object(target='build/obj/simpleTest', source='test/simpleTest.cpp')

env.Append(LIBPATH = env.Split("/local/karrenberg/proj/anysl/install/build-icc-opt/jitRT/lib/"))
env.Program(target='build/bin/simpleTest', source=Split('build/obj/simpleTest.o build/obj/sseOpenCLDriver.o'), LIBS=['jitRT'])

