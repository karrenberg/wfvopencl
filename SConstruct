import os

# simply clone entire environment
env = Environment(ENV = os.environ)

# These two variables have to be set in the environment
LLVM_INSTALL_DIR = env['ENV']['LLVM_INSTALL_DIR']
PACKETIZER_INSTALL_DIR = env['ENV']['PACKETIZER_INSTALL_DIR']

# build variables
debug             = ARGUMENTS.get('debug', 0)             # enable debug information
debug_runtime     = ARGUMENTS.get('debug_runtime', 0)     # enable debugging of runtime (JIT) code
profile           = ARGUMENTS.get('profile', 0)           # enable profiling
use_openmp        = ARGUMENTS.get('openmp', 0)            # enable OpenMP
num_threads       = ARGUMENTS.get('threads', 0)           # set number of threads to use in OpenMP
split             = ARGUMENTS.get('split', 0)             # disable load/store optimizations (= always perform scalar load/store)
packetize         = ARGUMENTS.get('packetize', 0)         # enable packetization
packetizer_shared = ARGUMENTS.get('packetizer_shared', 0) # should be set if Packetizer was compiled as a shared library (see below)
llvm_no_debug     = ARGUMENTS.get('llvm_no_debug', 0)     # should be set if LLVM was compiled in release mode (at least on windows)

compile_static_lib_driver = ARGUMENTS.get('static', 0)    # compile static library link applications statically (circumvents OpenCL ICD mechanism)

if int(debug_runtime):
	debug = 1

if int(debug) and int(use_openmp):
	print "\nWARNING: Using OpenMP in debug mode might lead to unknown behaviour!\n"

# find out if we are on windows (win32), linux (posix), or mac (darwin)
isWin = env['PLATFORM'] == 'win32' # query HOST_OS or TARGET_OS instead of PLATFORM?
isDarwin = env['PLATFORM'] == 'darwin' # query HOST_OS or TARGET_OS instead of PLATFORM?
if isWin:
	is32Bit = env['TARGET_ARCH'] == 'x86'
else:
	is32Bit = 0 # only 64bit for unix/darwin atm
#print env['PLATFORM']
#print env['ENV']
# In current SCons, the following are only defined by VS-compilers!!!
#print env['HOST_OS']
#print env['HOST_ARCH']
#print env['TARGET_OS']
#print env['TARGET_ARCH']

# NOTE: Dynamically linking the packetizer will not work because both the driver and the packetizer statically link LLVM
#       on Windows (because there are no shared libraries as of LLVM 2.9). Thus, pointer equality of LLVM types (e.g. for
#       Type* == Type*) will not work between the two libs (and possibly more stuff).
if isWin and int(packetize) and int(packetizer_shared):
	print "\nERROR: Dynamic linking of packetizer library does not work on Windows!\n"
	Exit(1)

# set up compiler
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
	"-I"+os.path.join(LLVM_INSTALL_DIR, 'include')+" -L"+os.path.join(LLVM_INSTALL_DIR, 'lib')+
	" -D_GNU_SOURCE -D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS -lLLVMObject -lLLVMMCJIT -lLLVMMCDisassembler -lLLVMLinker -lLLVMipo -lLLVMInterpreter -lLLVMInstrumentation -lLLVMJIT -lLLVMExecutionEngine -lLLVMBitWriter -lLLVMX86Disassembler -lLLVMX86AsmParser -lLLVMX86CodeGen -lLLVMX86AsmPrinter -lLLVMX86Utils -lLLVMX86Info -lLLVMAsmParser -lLLVMArchive -lLLVMBitReader -lLLVMSelectionDAG -lLLVMAsmPrinter -lLLVMMCParser -lLLVMCodeGen -lLLVMScalarOpts -lLLVMInstCombine -lLLVMTransformUtils -lLLVMipa -lLLVMAnalysis -lLLVMTarget -lLLVMCore -lLLVMMC -lLLVMSupport -lshell32 -ladvapi32"
	])
else:
	llvm_vars = env.ParseFlags('!$LLVM_INSTALL_DIR/bin/llvm-config --cflags --ldflags --libs')


# set up CXXFLAGS
cxxflags = llvm_vars.get('CCFLAGS')

if isWin:
	# /Wall gives LOTS of warnings -> leave at default warning level, or steal commandline of LLVM-VS-project
	cxxflags += env.Split("/EHsc")
	if int(llvm_no_debug):
		cxxflags += env.Split("/MD")
	else:
		cxxflags += env.Split("/MDd")
else:
	cxxflags += env.Split("-Wall -pedantic -Wno-long-long -msse3")

if int(debug_runtime):
	cxxflags=cxxflags+env.Split("-DDEBUG_RUNTIME")

if int(debug):
	cxxflags=cxxflags+env.Split("-DDEBUG -D_DEBUG")
	if isWin:
		cxxflags=cxxflags+env.Split("/Od /Zi")
		env.Append(LINKFLAGS = env.Split("/DEBUG")) # link.exe might not like this if compiling a static lib
	else:
		cxxflags=cxxflags+env.Split("-O0 -g")
else:
	if isWin:
		cxxflags=cxxflags+env.Split("/Ox /Ob2 /Oi /GL")
		env.Append(LINKFLAGS = env.Split("/LTCG"))
	else:
		cxxflags=cxxflags+env.Split("-O3 -DNDEBUG")

if int(profile):
	if isWin:
		cxxflags=cxxflags+env.Split("/Od /Zi")
	else:
		cxxflags=cxxflags+env.Split("-O0 -g")
	# disabled until we have 64bit VTune libraries
	#cxxflags=cxxflags+env.Split("-g -DPACKETIZED_OPENCL_ENABLE_JIT_PROFILING")
	#compile_static_lib_driver=1

if int(use_openmp):
	if isWin:
		cxxflags=cxxflags+env.Split("/DPACKETIZED_OPENCL_USE_OPENMP /openmp")
	else:
		cxxflags=cxxflags+env.Split("-DPACKETIZED_OPENCL_USE_OPENMP -fopenmp")
		env.Append(LINKFLAGS = env.Split("-fopenmp"))
	if int(num_threads):
		cxxflags=cxxflags+env.Split("-DPACKETIZED_OPENCL_NUM_CORES="+num_threads)

if not int(packetize):
	cxxflags=cxxflags+env.Split("-DPACKETIZED_OPENCL_NO_PACKETIZATION")

if int(split):
	cxxflags=cxxflags+env.Split("-DPACKETIZED_OPENCL_SPLIT_EVERYTHING")

# This is only required if the packetizer was built as a shared library (and only on Windows for dllexport/dllimport)
# TODO: Can we detect this automatically? :)
if int(packetize) and not int(packetizer_shared):
	cxxflags=cxxflags+env.Split("-DPACKETIZER_STATIC_LIBS")


env.Append(CXXFLAGS = cxxflags)

# set up paths
env.Append(CPPPATH = [os.path.join(LLVM_INSTALL_DIR, 'include')])
env.Append(CPPPATH = [os.path.join(PACKETIZER_INSTALL_DIR, 'include')])
env.Append(CPPPATH = llvm_vars.get('CPPPATH'))
env.Append(CPPPATH = env.Split("include"))

env.Append(LIBPATH = [os.path.join(LLVM_INSTALL_DIR, 'lib')])
env.Append(LIBPATH = [os.path.join(PACKETIZER_INSTALL_DIR, 'lib')])
env.Append(LIBPATH = llvm_vars.get('LIBPATH'))
env.Append(LIBPATH = env.Split("lib"))
if is32Bit:
	env.Append(LIBPATH = env.Split("lib/x86"))
else:
	env.Append(LIBPATH = env.Split("lib/x86_64"))


# set up libraries
# glut and GLEW are not required for all, but this is easier :P
driverLibs = llvm_vars.get('LIBS') + env.Split('Packetizer')
if isWin:
	# get glut from http://www.idfun.de/glut64/
	if int(compile_static_lib_driver):
		appLibs = env.Split('PacketizedOpenCL SDKUtil glut32')
	else:
		appLibs = env.Split('OpenCL SDKUtil glut32')
	if is32Bit:
		appLibs+=env.Split('glew32s')
		env.Append(CXXFLAGS = '-DGLEW_STATIC')
	else:
		appLibs+=env.Split('glew32')
elif isDarwin:
	if int(compile_static_lib_driver):
		appLibs = env.Split('PacketizedOpenCL SDKUtil glew')
	else:
		# The following will only work as soon as Apple uses ICD etc. (OpenCL 1.1)
		#appLibs = env.Split('SDKUtil glew')
		appLibs = env.Split('OpenCL SDKUtil glew')
else:
	if int(compile_static_lib_driver):
		appLibs = env.Split('PacketizedOpenCL SDKUtil glut GLEW')
	else:
		appLibs = env.Split('OpenCL SDKUtil glut GLEW')

# disabled until we have 64bit VTune libraries
#if int(profile):
	#driverLibs=driverLibs+env.Split("dl pthread JITProfiling")
	#env.Append(CPPPATH = [os.path.join(env['ENV']['VTUNE_GLOBAL_DIR'], 'analyzer/include')])
	#env.Append(LIBPATH = [os.path.join(env['ENV']['VTUNE_GLOBAL_DIR'], 'analyzer/bin')])


###
### build Packetized OpenCL library
###

driverSrc = env.Glob('src/*.cpp')
if int(compile_static_lib_driver):
	PacketizedOpenCL = env.StaticLibrary(target='lib/PacketizedOpenCL', source=driverSrc, CPPDEFINES=llvm_vars.get('CPPDEFINES'), LIBS=driverLibs)
else:
	PacketizedOpenCL = env.SharedLibrary(target='lib/PacketizedOpenCL', source=driverSrc, CPPDEFINES=llvm_vars.get('CPPDEFINES'), LIBS=driverLibs)

###
### build AMD-ATI SDKUtil as a static library (required for test applications)
###

sdkSrc = env.Glob('test/SDKUtil/*.cpp')
SDKUtil = env.StaticLibrary(target='lib/SDKUtil', source=sdkSrc)


###
### build test applications
###

# Those work in all configurations, including packetizer:
testApps = env.Split("""
BitonicSort
BlackScholesSimple
FastWalshTransform
Histogram
MandelbrotSimple
MatrixTranspose
SimpleConvolution
""")

#testApps = env.Split("""
#BinarySearch
#BinomialOption
#BitonicSort
#BlackScholes
#BlackScholesSimple
#DCT
#DwtHaar1D
#EigenValue
#FFT
#FastWalshTransform
#FloydWarshall
#Histogram
#Mandelbrot
#MandelbrotSimple
#MatrixMulImage
#MatrixMultiplication
#MatrixTranspose
#MersenneTwister
#MersenneTwisterSimple
#MonteCarloAsian
#NBody
#NBodySimple
#PrefixSum
#QuasiRandomSequence
#RadixSort
#RecursiveGaussian
#Reduction
#ScanLargeArrays
#SimpleConvolution
#SobelFilter
#URNG
#""")

# These use AMD specific extensions not available in our clc (TODO: do we use clc from 2.3 already?)
#testApps = env.Split("""
#AESEncryptDecrypt
#FluidSimulation2D
#HistogramAtomics
#LUDecomposition
#URNGNoiseGL #uses opencl1.1 gl stuff
#""")

# Currently disabled (have to be adjusted to latest SDK)
#testApps = env.Split("""
#AmbientOcclusionRenderer
#BinomialOptionSimple
#TestSimple
#TestBarrier
#TestBarrier2
#TestLoopBarrier
#TestLoopBarrier2
#Test2D
#TestLinearAccess
#""")

Execute(Mkdir('build/bin'))
if int(compile_static_lib_driver):
	for a in testApps:
		if isDarwin:
			# On Darwin, SDK currently lacks code to find the execution path, so we need to copy the .cl files somewhere else than on unix/windows
			Execute(Copy('.', 'test/'+a+'/'+a+'_Kernels.cl'))
			Execute(Copy('.', 'test/'+a+'/'+a+'_Input.bmp')) # TODO: only if existant
			App = env.Program('build/bin/'+a, env.Glob('test/'+a+'/*.cpp'), LIBS=appLibs+driverLibs, LINKFLAGS=env['LINKFLAGS']+['-framework', 'GLUT', '-framework', 'OpenGL'])
		else:
			Execute(Copy('build/bin', 'test/'+a+'/'+a+'_Kernels.cl'))
			Execute(Copy('build/bin', 'test/'+a+'/'+a+'_Input.bmp')) # TODO: only if existant
			App = env.Program('build/bin/'+a, env.Glob('test/'+a+'/*.cpp'), LIBS=appLibs+driverLibs)
		env.Depends(App, SDKUtil)
else:
	for a in testApps:
		Execute(Copy('build/bin', 'test/'+a+'/'+a+'_Kernels.cl'))
		Execute(Copy('build/bin', 'test/'+a+'/'+a+'_Input.bmp')) # TODO: only if existant
		if isDarwin:
			# The following will only work as soon as Apple uses ICD etc. (OpenCL 1.1)
			#App = env.Program('build/bin/'+a, env.Glob('test/'+a+'/*.cpp'), LIBS=appLibs, LINKFLAGS=env['LINKFLAGS']+['-framework', 'OpenCL', '-framework', 'GLUT', '-framework', 'OpenGL'])
			App = env.Program('build/bin/'+a, env.Glob('test/'+a+'/*.cpp'), LIBS=appLibs, LINKFLAGS=env['LINKFLAGS']+['-framework', 'GLUT', '-framework', 'OpenGL'])
		else:
			App = env.Program('build/bin/'+a, env.Glob('test/'+a+'/*.cpp'), LIBS=appLibs)
		env.Depends(App, SDKUtil)


###
### build bitcode from OpenCL files
###

# NOTE: using --march automatically generates a stub-function
# NOTE: using --march also inserts calls to builtin functions instead of e.g. printf
# NOTE: --march=x86-64 generates bad code for packetization :(
#       --march=x86 (or left out) generates 32bit data structures etc., making wrapper unusable

#env.Command('build/obj/simpleTest_Kernels.ll', 'test/simpleTest_Kernels.cl', "clc --march=x86-64 --msse2 $SOURCE")
#env.Command('build/obj/simpleTest_Kernels.ll', 'test/simpleTest_Kernels.cl', "clc --march=x86 --msse2 $SOURCE")

#cmd_ll = "clc -o $TARGET --msse2 $SOURCE"
#cmd_bc = "llvm-as $SOURCE -o $TARGET"

if not isWin and not isDarwin:
	for a in testApps:
		env.Command('build/obj/'+a+'_Kernels', 'test/'+a+'/'+a+'_Kernels.cl',
					["bin/x86_64/clc -o ${TARGET.file}.ll --msse2 ${SOURCE}",
					LLVM_INSTALL_DIR+"/bin/llvm-as"+" -o ${TARGET.file}.bc ${TARGET.file}.ll",
					Delete('${TARGET.file}.ll')])

###
### clean up
###

# For some reason, those are not removed automatically by scons -c :(
if GetOption("clean"):
	Execute(Delete('build'))
	Execute(Delete('lib/PacketizedOpenCL.dll'))
	Execute(Delete('lib/PacketizedOpenCL.dll.manifest'))
	Execute(Delete('lib/PacketizedOpenCL.exp'))
	Execute(Delete('lib/libPacketizedOpenCL.so'))
	for a in testApps:
		if isDarwin:
			Execute(Delete(a+'_Kernels.cl'))
			Execute(Delete(a+'_Input.bmp'))
		if not isWin and not isDarwin:
			Execute(Delete(a+'_Kernels.bc'))
