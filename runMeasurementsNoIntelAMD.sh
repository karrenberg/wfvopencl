#!/bin/bash

# Notes on benchmarks:

# BitonicSort is incredibly slow on AMD
# EigenValue fails for size > 4096 on all platforms
# FloydWarshall has a maximum size of 512 (not enough memory on Intel/AMD)
# Histogram has a maximum size of 16384 x 15872
# MandelbrotSimple has a maximum size of 8192x8192 on Intel/AMD
# MandelbrotSimple is incredibly slow on AMD (slower than reference CPU implementation!)
# MatrixTranspose verification takes forever for size > 14000
# MersenneTwisterSimple has an approximate maximum size of 45000 (starts spilling?)
# NBodySimple fails for size > 25000 
# NBodySimple verification fails for iterations > 1
# PrefixSum uses a maximum of size 2048 in AMD's implementation
# PrefixSum uses a maximum of size 256 in Intel's implementation
# PrefixSum verification fails for iterations > 1 (only larger sizes?)
# RadixSort can work on even larger sizes than 33554432
# Reduction starts swapping at size 1000000000
# SimpleConvolution has a maximum size of 8192x8192 on Intel


while [ $# != 0 ]; do
	case "$1" in
		PLATFORM=*) PLATFORM="${1#PLATFORM=}"; shift;;
		OPENMP=*) OPENMP="${1#OPENMP=}"; shift;;
		*) echo "unknown argument(s): $@">&2; exit 1;;
	esac
done


NUM_SAMPLES=50

echo ""
echo "Packetizer configuration:"
build/bin/TestSimple
echo ""

echo ""
echo "-------------------------------------------------------------------"
echo "Measuring benchmarks on platform $PLATFORM..."
echo "-------------------------------------------------------------------"
echo ""

for (( i = 0; i < $NUM_SAMPLES; i++ ))
do
	echo ""
	echo "AmbientOcclusionRenderer ($i)"
	build/bin/AmbientOcclusionRenderer -q -t -p "$PLATFORM"
	echo ""
	echo "BitonicSort ($i)"
	build/bin/BitonicSort -q -x 1048576 -t -p "$PLATFORM" # takes forever on AMD (~55sec)
	echo ""
	echo "BlackScholesSimple ($i)"
	build/bin/BlackScholesSimple -q -t -x 16384 -p "$PLATFORM"
	echo ""
	echo "DCT ($i)"
	build/bin/DCT -q -t -x 8192 -y 8192 -p "$PLATFORM"
	echo ""
	echo "FastWalshTransform ($i)"
	build/bin/FastWalshTransform -q -t -x 134217728 -p "$PLATFORM"
	echo ""
	echo "FloydWarshall ($i)"
	build/bin/FloydWarshall -q -t -x 512 -p "$PLATFORM"
	echo ""
	echo "Histogram ($i)"
	build/bin/Histogram -q -t -x 15600 -y 15600 -p "$PLATFORM"
	echo ""
	echo "MandelbrotSimple ($i)"
	build/bin/MandelbrotSimple -q -t -x 8192 -p "$PLATFORM"
	echo ""
	echo "MatrixTranspose ($i)"
	build/bin/MatrixTranspose -q -t -x 12000 -p "$PLATFORM"
	#echo ""
	#echo "NBodySimple ($i)"
	#build/bin/NBodySimple -q -t -x 19968 -p "$PLATFORM"
	#echo ""
	#echo "SimpleConvolution ($i)"
	#build/bin/SimpleConvolution -q -t -x 8192 -y 8192 -m 1 -p "$PLATFORM"
done

echo ""
echo "-------------------------------------------------------------------"
echo "Archiving the results..."
echo "-------------------------------------------------------------------"
echo ""

### Move everything into a  different folder
DIR=$(date +%Y%m%d_%H%M)
mkdir -p benchmark/$DIR/
mv *.pkt.txt benchmark/$DIR/

cp benchmark/intel_win/*.txt benchmark/$DIR/
cp benchmark/amd_win/*.txt benchmark/$DIR/
cp benchmark_intel.cfg benchmark/$DIR/
cp benchmark_amd.cfg benchmark/$DIR/

# Sort files
for file in benchmark/$DIR/*.txt ; do
	sort -n $file -o $file
done

echo ""
echo "-------------------------------------------------------------------"
echo "Measurements finished (Time: $(date +%H:%M))"
echo "-------------------------------------------------------------------"
echo ""
