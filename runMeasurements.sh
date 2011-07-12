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


NUM_SAMPLES=1
NUM_PLATFORMS=3

echo ""
echo "Packetizer configuration:"
build/bin/TestSimple
echo ""

for (( PLATFORM = 0; PLATFORM < $NUM_PLATFORMS; PLATFORM++ ))
do
	# First test if all benchmarks produce correct results.
	echo ""
	echo "-------------------------------------------------------------------"
	echo "Verifying benchmark results on platform $PLATFORM..."
	echo "-------------------------------------------------------------------"
	echo ""
	build/bin/AmbientOcclusionRenderer -q -e -x 512 -y 512 -p "$PLATFORM" && \
		build/bin/BinomialOptionSimple -q -e -x 16384 -p $"$PLATFORM" && \
		build/bin/BitonicSort -q -x 1048576 -e -p "$PLATFORM" && \
		build/bin/BlackScholesSimple -q -e -x 16384 -p "$PLATFORM" && \
		build/bin/DCT -q -e -x 8192 -y 8192 -p "$PLATFORM" && \
		build/bin/DwtHaar1D -q -e -x 8388608 -p "$PLATFORM" && \
		build/bin/EigenValue -q -e -x 4096 -p "$PLATFORM" && \
		build/bin/FastWalshTransform -q -e -x 134217728 -p "$PLATFORM" && \
		build/bin/FloydWarshall -q -e -x 512 -p "$PLATFORM" && \
		build/bin/Histogram -q -e -x 15872 -y 15872 -p "$PLATFORM" && \
		build/bin/MandelbrotSimple -q -e -x 8192 -p "$PLATFORM" && \
		build/bin/MatrixTranspose -q -e -x 12000 -p "$PLATFORM" && \
		build/bin/MersenneTwisterSimple -q -e -x 45000 -p "$PLATFORM" && \
		build/bin/NBodySimple -q -e -x 23808 -p "$PLATFORM" && \
		build/bin/PrefixSum -q -e -x 256 -p "$PLATFORM" && \
		build/bin/RadixSort -q -e -x 33554432 -p "$PLATFORM" && \
		build/bin/Reduction -q -e -x 9999974 -p "$PLATFORM" && \
		build/bin/SimpleConvolution -q -e -x 8192 -y 8192 -m 1 -p "$PLATFORM"

	SUCCESS=$? # read output

	if [ $SUCCESS -ne 0 ]
	then
		echo ""
		echo "-------------------------------------------------------------------"
		echo "ERROR: at least one benchmark failed, skipping measurements for platform $PLATFORM!"
		echo "-------------------------------------------------------------------"
		echo ""
		continue
	fi

	echo ""
	echo "-------------------------------------------------------------------"
	echo "Measuring benchmarks on platform $PLATFORM..."
	echo "-------------------------------------------------------------------"
	echo ""

	for (( i = 0; i < $NUM_SAMPLES; i++ ))
	do
		echo ""
		echo "AmbientOcclusionRenderer ($i)"
		build/bin/AmbientOcclusionRenderer -q -t -x 512 -y 512 -p "$PLATFORM"
		echo ""
		echo "BinomialOptionSimple ($i)"
		build/bin/BinomialOptionSimple -q -t -x 16384 -p "$PLATFORM"
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
		echo "DwtHaar1D ($i)"
		build/bin/DwtHaar1D -q -t -x 8388608 -p "$PLATFORM"
		echo ""
		echo "EigenValue ($i)"
		build/bin/EigenValue -q -t -x 4096 -p "$PLATFORM"
		echo ""
		echo "FastWalshTransform ($i)"
		build/bin/FastWalshTransform -q -t -x 134217728 -p "$PLATFORM"
		echo ""
		echo "FloydWarshall ($i)"
		build/bin/FloydWarshall -q -t -x 512 -p "$PLATFORM"
		echo ""
		echo "Histogram ($i)"
		build/bin/Histogram -q -t -x 15872 -y 15872 -p "$PLATFORM"
		echo ""
		echo "MandelbrotSimple ($i)"
		build/bin/MandelbrotSimple -q -t -x 8192 -p "$PLATFORM"
		echo ""
		echo "MatrixTranspose ($i)"
		build/bin/MatrixTranspose -q -t -x 12000 -p "$PLATFORM"
		echo ""
		echo "MersenneTwisterSimple ($i)"
		build/bin/MersenneTwisterSimple -q -t -x 45000 -p "$PLATFORM"
		echo ""
		echo "NBodySimple ($i)"
		build/bin/NBodySimple -q -t -x 23808 -p "$PLATFORM"
		echo ""
		echo "PrefixSum ($i)"
		build/bin/PrefixSum -q -t -x 256 -p "$PLATFORM"
		echo ""
		echo "RadixSort ($i)"
		build/bin/RadixSort -q -t -x 33554432 -p "$PLATFORM"
		echo ""
		echo "Reduction ($i)"
		build/bin/Reduction -q -t -x 9999974 -p "$PLATFORM"
		echo ""
		echo "SimpleConvolution ($i)"
		build/bin/SimpleConvolution -q -t -x 8192 -y 8192 -m 1 -p "$PLATFORM"
	done

	echo ""
	echo "-------------------------------------------------------------------"
	echo "Measuring of benchmarks on platform $PLATFORM finished!"
	echo "-------------------------------------------------------------------"
	echo ""
done

echo ""
echo "-------------------------------------------------------------------"
echo "Computing statistical significance, confidence intervals, etc. ..."
echo "-------------------------------------------------------------------"
echo ""

./SpeedupTest.sh benchmark_intel.cfg
./SpeedupTest.sh benchmark_amd.cfg

echo ""
echo "-------------------------------------------------------------------"
echo "Archiving the results..."
echo "-------------------------------------------------------------------"
echo ""

### And move everything into a  different folder
DIR=$(date +%Y%m%d_%H%M)
mkdir -p benchmark/$DIR/
mv *.pkt.txt benchmark/$DIR/
mv *.amd.txt benchmark/$DIR/
mv *.intel.txt benchmark/$DIR/
mv benchmark_intel.cfg.* benchmark/$DIR/
mv benchmark_amd.cfg.* benchmark/$DIR/

echo ""
echo "-------------------------------------------------------------------"
echo "Measurements finished!"
echo "-------------------------------------------------------------------"
echo ""
