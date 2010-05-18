#home
#export JITRT_INSTALL_DIR=/home/ralf/PRojekte/proj_anysl/install/build-icc-debug/jitRT
#export LLVM_BIN=/home/ralf/Projekte/proj_anysl/install/build-gcc-debug/llvm/bin
#uni
export JITRT_INSTALL_DIR=/local/karrenberg/proj/anysl/install/build-icc-debug/jitRT
export LLVM_BIN=/local/karrenberg/proj/anysl/install/build-gcc-debug/llvm/bin

export PATH=$PATH:$LLVM_BIN:./include
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:./lib

cp $JITRT_INSTALL_DIR/include/jitRT/llvmWrapper.h ./include/jitRT/llvmWrapper.h
cp $JITRT_INSTALL_DIR/lib/libjitRT.so ./lib/libjitRT.so
