export PACKETIZER_INSTALL_DIR=$HOME/proj/packetizer
export LLVM_INSTALL_DIR=$HOME/proj/llvm
export PATH=./bin/x86_64:$PATH

export PATH=./include:$PATH
export PATH=$LLVM_INSTALL_DIR/bin:$PATH
export PATH=$LLVM_INSTALL_DIR/include:$PATH

export LD_LIBRARY_PATH=./lib:$LD_LIBRARY_PATH
#export LD_LIBRARY_PATH=./build/lib:$LD_LIBRARY_PATH
export LD_LIBRARY_PATH=$LLVM_INSTALL_DIR/lib:$LD_LIBRARY_PATH
export LD_LIBRARY_PATH=$PACKETIZER_INSTALL_DIR/lib:$LD_LIBRARY_PATH

# fix path of AMD APP SDK's as.exe and ld.exe (otherwise, applications exit with internal build error)
# unfortunately, MinGW does not fix the AMDAPPSDKROOT variable (it remains as C:\...) :(
#export PATH="$AMDAPPSDKROOT"/bin/x86_64:$PATH
export PATH="/c/Program Files (x86)/AMD APP/bin/x86_64":$PATH
