import os
import sys

#env = Environment(ENV = {'PATH'            : os.environ['PATH'],
#						 'LD_LIBRARY_PATH' : os.environ['LD_LIBRARY_PATH']})

# build variables
debug         = ARGUMENTS.get('debug', 0)
debug_runtime = ARGUMENTS.get('debug-runtime', 0)
profile       = ARGUMENTS.get('profile', 0)
use_openmp    = ARGUMENTS.get('openmp', 0)
num_threads   = ARGUMENTS.get('threads', 0)
packetize     = ARGUMENTS.get('packetize', 0)
split         = ARGUMENTS.get('split', 0)

compile_dynamic_lib_driver = ARGUMENTS.get('dynamic', 0)

if int(debug) and int(use_openmp):
	print "\nWARNING: Using OpenMP in debug mode might lead to unknown behaviour!\n"

# simply clone entire environment
env = Environment(ENV = os.environ)

# find out if we are on windows
isWin = env['PLATFORM'] == 'win32' # query HOST_OS or TARGET_OS instead of PLATFORM?
#print env['HOST_OS']
#print env['HOST_ARCH']
#print env['TARGET_OS']
#print env['TARGET_ARCH']
#print env['PLATFORM']
#print env['ENV']

if isWin:
	env['CC'] = 'cl'
	env['CXX'] = 'cl'
else:
	#env['CC'] = 'clang'    # no -fopenmp :(
	#env['CXX'] = 'clang++' # no -fopenmp :(
	env['CC'] = 'gcc'
	env['CXX'] = 'g++'

# query llvm-config
if isWin:
	llvm_vars = env.ParseFlags([
	"-I/local/karrenberg/include -D_DEBUG -D_GNU_SOURCE -D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS -L/local/karrenberg/lib -lLLVMObject -lLLVMMCJIT -lLLVMMCDisassembler -lLLVMLinker -lLLVMipo -lLLVMInterpreter -lLLVMInstrumentation -lLLVMJIT -lLLVMExecutionEngine -lLLVMBitWriter -lLLVMX86Disassembler -lLLVMX86AsmParser -lLLVMX86CodeGen -lLLVMX86AsmPrinter -lLLVMX86Utils -lLLVMX86Info -lLLVMAsmParser -lLLVMArchive -lLLVMBitReader -lLLVMSelectionDAG -lLLVMAsmPrinter -lLLVMMCParser -lLLVMCodeGen -lLLVMScalarOpts -lLLVMInstCombine -lLLVMTransformUtils -lLLVMipa -lLLVMAnalysis -lLLVMTarget -lLLVMCore -lLLVMMC -lLLVMSupport -lshell32 -ladvapi32"
	])
else:
	llvm_vars = env.ParseFlags('!llvm-config --cflags --ldflags --libs')

# set up CXXFLAGS
if isWin:
	# 4710 = function is not 'inline'
	# 4100 = unreferenzierter formaler parameter (unimplementierte API funktionen)
	# 4541 = inline function without reference was removed
	cxxflags = env.Split("/Wall /Zi /MDd /EHsc /wd4820 /wd4668 /wd4710 /wd4625 /wd4127 /wd4548 /wd4100")+llvm_vars.get('CCFLAGS')
else:
	cxxflags = env.Split("-Wall -pedantic -Wno-long-long -msse3")+llvm_vars.get('CCFLAGS')

if int(debug) or int(debug_runtime):
	cxxflags=cxxflags+env.Split("-O0 -g")

if int(debug):
	cxxflags=cxxflags+env.Split("-DDEBUG")

if int(debug_runtime):
	cxxflags=cxxflags+env.Split("-DDEBUG_RUNTIME")

if not int(debug) and not int(debug_runtime):
	cxxflags=cxxflags+env.Split("-O3 -DNDEBUG")

if int(profile):
	cxxflags=cxxflags+env.Split("-g")
	# disabled until we have 64bit VTune libraries
	#cxxflags=cxxflags+env.Split("-g -DPACKETIZED_OPENCL_DRIVER_ENABLE_JIT_PROFILING")
	#compile_dynamic_lib_driver=0

if int(use_openmp):
	cxxflags=cxxflags+env.Split("-DPACKETIZED_OPENCL_DRIVER_USE_OPENMP -fopenmp")
	env.Append(LINKFLAGS = env.Split("-fopenmp"))
	if int(num_threads):
		cxxflags=cxxflags+env.Split("-DPACKETIZED_OPENCL_DRIVER_NUM_CORES="+num_threads)

if not int(packetize):
	cxxflags=cxxflags+env.Split("-DPACKETIZED_OPENCL_DRIVER_NO_PACKETIZATION")

if int(split):
	cxxflags=cxxflags+env.Split("-DPACKETIZED_OPENCL_DRIVER_SPLIT_EVERYTHING")

if int(packetize) and not int(compile_dynamic_lib_driver):
	cxxflags=cxxflags+env.Split("-DPACKETIZER_STATIC_LIBS")


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
# glut and GLEW are not required for all, but this is easier :P
driverLibs = llvm_vars.get('LIBS') + env.Split('Packetizer')
if isWin:
	# get glut from http://www.idfun.de/glut64/
	appLibs = env.Split('PacketizedOpenCLDriver SDKUtil glut32 glew32')
else:
	appLibs = env.Split('PacketizedOpenCLDriver SDKUtil glut GLEW')

# disabled until we have 64bit VTune libraries
#if int(profile):
	#driverLibs=driverLibs+env.Split("dl pthread JITProfiling")
	#env.Append(CPPPATH = [os.path.join(env['ENV']['VTUNE_GLOBAL_DIR'], 'analyzer/include')])
	#env.Append(LIBPATH = [os.path.join(env['ENV']['VTUNE_GLOBAL_DIR'], 'analyzer/bin')])

# build Packetized OpenCL Driver
driverSrc = env.Glob('src/*.cpp')
if int(compile_dynamic_lib_driver):
	PacketizedOpenCLDriver = env.SharedLibrary(target='lib/PacketizedOpenCLDriver', source=driverSrc, CPPDEFINES=llvm_vars.get('CPPDEFINES'), LIBS=driverLibs)
else:
	PacketizedOpenCLDriver = env.StaticLibrary(target='lib/PacketizedOpenCLDriver', source=driverSrc, CPPDEFINES=llvm_vars.get('CPPDEFINES'), LIBS=driverLibs)


# build AMD-ATI SDKUtil as a static library (required for test applications)
SDKUtil = env.StaticLibrary(target='lib/SDKUtil', source=Split('test/SDKUtil/SDKApplication.cpp test/SDKUtil/SDKBitMap.cpp test/SDKUtil/SDKCommandArgs.cpp test/SDKUtil/SDKCommon.cpp test/SDKUtil/SDKFile.cpp'))
env.Depends(SDKUtil, PacketizedOpenCLDriver)

###
### test applications ###
###

testApps = env.Split("""
AmbientOcclusionRenderer
BinomialOption
BinomialOptionSimple
BitonicSort
BlackScholes
BlackScholesSimple
DCT
DwtHaar1D
EigenValue
FastWalshTransform
FloydWarshall
Histogram
Mandelbrot
MatrixTranspose
NBody
NBodySimple
PrefixSum
RadixSort
Reduction
ScanLargeArrays
SHA1
SimpleConvolution
TestSimple
TestBarrier
TestBarrier2
TestLoopBarrier
TestLoopBarrier2
Test2D
TestLinearAccess
""")

if int(compile_dynamic_lib_driver):
	for a in testApps:
		App = env.Program('build/bin/'+a, env.Glob('test/'+a+'/*.cpp'), LIBS=appLibs)
		env.Depends(App, SDKUtil)
else:
	for a in testApps:
		App = env.Program('build/bin/'+a, env.Glob('test/'+a+'/*.cpp'), LIBS=appLibs+driverLibs)
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

