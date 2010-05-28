#home
#export PACKETIZER_INSTALL_DIR=/home/ralf/Projekte/proj_anysl/install/build-gcc-opt/packetizer
#export LLVM_INSTALL_DIR=/home/ralf/Projekte/proj_anysl/install/build-gcc-opt/llvm
#uni
export PACKETIZER_INSTALL_DIR=/local/karrenberg/proj/anysl/install/build-gcc-opt/packetizer
export LLVM_INSTALL_DIR=/local/karrenberg/proj/anysl/install/build-gcc-opt/llvm

export PATH=$PATH:./include
export PATH=$PATH:$LLVM_INSTALL_DIR/bin
export PATH=$PATH:$LLVM_INSTALL_DIR/include
export PATH=$PATH:$PACKETIZER_INSTALL_DIR/include

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:./lib
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:./build/lib
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$LLVM_INSTALL_DIR/lib
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$PACKETIZER_INSTALL_DIR/lib
