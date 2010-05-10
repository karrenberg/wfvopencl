#home
#export JITRT_INC=/home/ralf/Projekte/proj_anysl/install/build-icc-debug/jitRT/include
#export JITRT_LIB=/home/ralf/Projekte/proj_anysl/install/build-icc-debug/jitRT/lib
#export LLVM_BIN=/home/ralf/Projekte/proj_anysl/install/build-gcc-debug/llvm/bin
#uni
#export JITRT_INC=/local/karrenberg/proj/anysl/install/build-icc-debug/jitRT/include
#export JITRT_LIB=/local/karrenberg/proj/anysl/install/build-icc-debug/jitRT/lib
export LLVM_BIN=/local/karrenberg/proj/anysl/install/build-gcc-debug/llvm/bin

export PATH=$PATH:$LLVM_BIN:./include
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$JITRT_LIB:./lib

cp /local/karrenberg/proj/anysl/install/build-icc-debug/jitRT/include/jitRT/llvmWrapper.h ./include/jitRT/llvmWrapper.h

cp /local/karrenberg/proj/anysl/install/build-icc-debug/jitRT/lib/libjitRT.so ./lib/libjitRT.so
