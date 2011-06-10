export PACKETIZER_INSTALL_DIR=$HOME/proj/packetizer
export LLVM_INSTALL_DIR=$HOME/proj/llvm-2.9
export PATH=$PATH:./bin/x86_64

export PATH=$PATH:./include
export PATH=$PATH:$LLVM_INSTALL_DIR/bin
export PATH=$PATH:$LLVM_INSTALL_DIR/include

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:./lib
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:./build/lib
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$LLVM_INSTALL_DIR/lib
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$PACKETIZER_INSTALL_DIR/lib

# fix path of AMD APP SDK's as.exe and ld.exe (otherwise, applications exit with internal build error)
# unfortunately, MinGW does not fix the AMDAPPSDKROOT variable (it remains as C:\...) :(
#export PATH="$AMDAPPSDKROOT"/bin/x86_64:$PATH
export PATH="/c/Program Files (x86)/AMD APP/bin/x86_64":$PATH
