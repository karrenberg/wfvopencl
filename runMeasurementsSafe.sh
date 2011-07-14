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


NUM_SAMPLES=50
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
	echo "Verifying benchmark results on platform $PLATFORM... (Time: $(date +%H:%M))"
	echo "-------------------------------------------------------------------"
	echo ""
	build/bin/BitonicSort -q -x 1048576 -e -p "$PLATFORM" && \
		build/bin/BlackScholesSimple -q -e -x 16384 -p "$PLATFORM" && \
		build/bin/FastWalshTransform -q -e -x 134217728 -p "$PLATFORM" && \
		build/bin/Histogram -q -e -x 15872 -y 15872 -p "$PLATFORM" && \
		build/bin/MandelbrotSimple -q -e -x 8192 -p "$PLATFORM" && \
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
		echo "BitonicSort ($i)"
		build/bin/BitonicSort -q -x 1048576 -t -p "$PLATFORM" # takes forever on AMD (~55sec)
		echo ""
		echo "BlackScholesSimple ($i)"
		build/bin/BlackScholesSimple -q -t -x 16384 -p "$PLATFORM"
		echo ""
		echo "FastWalshTransform ($i)"
		build/bin/FastWalshTransform -q -t -x 134217728 -p "$PLATFORM"
		echo ""
		echo "Histogram ($i)"
		build/bin/Histogram -q -t -x 15872 -y 15872 -p "$PLATFORM"
		echo ""
		echo "MandelbrotSimple ($i)"
		build/bin/MandelbrotSimple -q -t -x 8192 -p "$PLATFORM"
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
echo "Measurements finished (Time: $(date +%H:%M))"
echo "-------------------------------------------------------------------"
echo ""
