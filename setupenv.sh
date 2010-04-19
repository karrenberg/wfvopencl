#home
#export JITRT_INC=/home/ralf/Projekte/proj_anysl/install/build-icc-opt/jitRT/include
#export JITRT_LIB=/home/ralf/Projekte/proj_anysl/install/build-icc-opt/jitRT/lib
#export LLVM_BIN=/home/ralf/Projekte/proj_anysl/install/build-gcc-opt/llvm/bin
#uni
export JITRT_INC=/local/karrenberg/proj/anysl/install/build-icc-opt/jitRT/include
export JITRT_LIB=/local/karrenberg/proj/anysl/install/build-icc-opt/jitRT/lib
export LLVM_BIN=/local/karrenberg/proj/anysl/install/build-gcc-opt/llvm/bin

export PATH=$PATH:$LLVM_BIN
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$JITRT_LIB
