import os
#env = Environment(ENV = {'PATH'            : os.environ['PATH'],
#						 'LD_LIBRARY_PATH' : os.environ['LD_LIBRARY_PATH']})

# build variables
debug = ARGUMENTS.get('debug', 0)
use_openmp = ARGUMENTS.get('openmp', 0)
packetize = ARGUMENTS.get('packetize', 0)
force_nd_iteration = ARGUMENTS.get('force_nd', 0)

if int(debug) and int(use_openmp):
	print "\nWARNING: Using OpenMP in debug mode might lead to unknown behaviour!\n"

# simply clone entire environment
env = Environment(ENV = os.environ)

#env['CC'] = 'clang'    # no -fopenmp :(
#env['CXX'] = 'clang++' # no -fopenmp :(
env['CC'] = 'gcc'
env['CXX'] = 'g++'

# query llvm-config
llvm_vars = env.ParseFlags('!llvm-config --cflags --ldflags --libs')

# set up CXXFLAGS
cxxflags = env.Split("-Wall -pedantic -Wno-long-long -msse3")+llvm_vars.get('CCFLAGS')

if int(debug):
	cxxflags=cxxflags+env.Split("-O0 -g -DDEBUG")
else:
	cxxflags=cxxflags+env.Split("-O3 -DNDEBUG")

if int(use_openmp):
	cxxflags=cxxflags+env.Split("-DPACKETIZED_OPENCL_DRIVER_USE_OPENMP -fopenmp")
	env.Append(LINKFLAGS = env.Split("-fopenmp"))

if not int(packetize):
	cxxflags=cxxflags+env.Split("-DPACKETIZED_OPENCL_DRIVER_NO_PACKETIZATION")

if int(force_nd_iteration):
	cxxflags=cxxflags+env.Split("-DPACKETIZED_OPENCL_DRIVER_FORCE_ND_ITERATION_SCHEME")

env.Append(CXXFLAGS = cxxflags)

# set up paths
env.Append(CPPPATH = env.Split("include"))
env.Append(CPPPATH = llvm_vars.get('CPPPATH'))
env.Append(CPPPATH = [os.path.join(env['ENV']['LLVM_INSTALL_DIR'], 'include')])
env.Append(CPPPATH = [os.path.join(env['ENV']['PACKETIZER_INSTALL_DIR'], 'include')])

env.Append(LIBPATH = env.Split("lib build/lib"))
env.Append(LIBPATH = [os.path.join(env['ENV']['LLVM_INSTALL_DIR'], 'lib')])
env.Append(LIBPATH = [os.path.join(env['ENV']['PACKETIZER_INSTALL_DIR'], 'lib')])
env.Append(LIBPATH = llvm_vars.get('LIBPATH'))

# set up libraries
driverLibs = llvm_vars.get('LIBS') + env.Split('Packetizer')
appLibs = env.Split('PacketizedOpenCLDriver SDKUtil glut GLEW') # glut, GLEW not required for all


# build Packetized OpenCL Driver
driverSrc = env.Glob('src/*.cpp')
PacketizedOpenCLDriver = env.SharedLibrary(target='lib/PacketizedOpenCLDriver', source=driverSrc, CPPDEFINES=llvm_vars.get('CPPDEFINES'), LIBS=driverLibs)


# build AMD-ATI SDKUtil as a static library (required for test applications)
SDKUtil = env.StaticLibrary(target='lib/SDKUtil', source=Split('test/SDKUtil/SDKApplication.cpp test/SDKUtil/SDKBitMap.cpp test/SDKUtil/SDKCommandArgs.cpp test/SDKUtil/SDKCommon.cpp test/SDKUtil/SDKFile.cpp'))
env.Depends(SDKUtil, PacketizedOpenCLDriver)

###
### test applications ###
###

testApps = env.Split("""
BitonicSort
DwtHaar1D
EigenValue
FastWalshTransform
FloydWarshall
Histogram
Mandelbrot
MatrixTranspose
NBody
PrefixSum
RadixSort
ScanLargeArrays
SimpleConvolution
TestSimple
TestBarrier
TestLoopBarrier
""")

for a in testApps:
	App = env.Program('build/bin/'+a, env.Glob('test/'+a+'/*.cpp'), LIBS=appLibs)
	env.Depends(App, SDKUtil)

###
### build bitcode from OpenCL files ###
###

# NOTE: using --march automatically generates a stub-function
# NOTE: using --march also inserts calls to builtin functions instead of e.g. printf
# NOTE: --march=x86-64 generates bad code for packetization :(
#       --march=x86 (or left out) generates 32bit data structures etc., making wrapper unusable

#env.Command('build/obj/simpleTest_Kernels.ll', 'test/simpleTest_Kernels.cl', "clc --march=x86-64 --msse2 $SOURCE")
#env.Command('build/obj/simpleTest_Kernels.ll', 'test/simpleTest_Kernels.cl', "clc --march=x86 --msse2 $SOURCE")

#cmd_ll = "clc -o $TARGET --msse2 $SOURCE"
#cmd_bc = "llvm-as $SOURCE -o $TARGET"

for a in testApps:
	env.Command('build/obj/'+a+'_Kernels', 'test/'+a+'/'+a+'_Kernels.cl',
				["clc -o ${TARGET.file}.ll --msse2 ${SOURCE}",
				"llvm-as -o ${TARGET.file}.bc ${TARGET.file}.ll",
				Delete('${TARGET.file}.ll')])

