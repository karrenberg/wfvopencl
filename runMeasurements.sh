#!/bin/bash
# Assumes an integer to be supplied for the platform the tests should be run on.

# EigenValue fails for size > 4096 (also with ATI's own implementation!)
# PrefixSum fails for larger sizes when using more than one iteration
# PrefixSum uses a maximum of  size 2048 in ATI's implementation
for i (( i=0; i<=5; i++ ))
do
	echo ""
	echo "TestSimple ($i)"
	build/bin/TestSimple "$@"
	echo ""
	echo "AmbientOcclusionRenderer ($i)"
	build/bin/AmbientOcclusionRenderer -q -t -x 1024 -y 1024 "$@"
	echo ""
	echo "BinomialOptionSimple ($i)"
	build/bin/BinomialOptionSimple -q -t -x 16384 "$@"
	echo ""
	echo "BitonicSort ($i)"
	build/bin/BitonicSort -q -x 1048576 -t "$@" # takes forever on AMD (~55sec)
	echo ""
	echo "BlackScholesSimple ($i)"
	build/bin/BlackScholesSimple -q -t -x 16384 "$@"
	echo ""
	echo "DCT ($i)"
	build/bin/DCT -q -t -x 8192 -y 8192 "$@"
	echo ""
	echo "DwtHaar1D ($i)"
	build/bin/DwtHaar1D -q -t -x 8388608 "$@"
	echo ""
	echo "EigenValue ($i)"
	build/bin/EigenValue -q -t -x 4096 "$@" # fails on all platforms for values > 8192
	echo ""
	echo "FastWalshTransform ($i)"
	build/bin/FastWalshTransform -q -t -x 134217728 "$@"
	echo ""
	echo "FloydWarshall ($i)"
	build/bin/FloydWarshall -q -t -x 512 "$@" # max size 512
	echo ""
	echo "Histogram ($i)"
	build/bin/Histogram -q -t -x 15872 -y 15872 "$@" # max is 16384x15872
	echo ""
	echo "Mandelbrot ($i)"
	build/bin/Mandelbrot -q -t -x 8192 "$@"
	echo ""
	echo "MandelbrotSimple ($i)"
	build/bin/MandelbrotSimple -q -t -x 8192 "$@" # max size, takes forever on AMD (~35sec)
	echo ""
	echo "MatrixTranspose ($i)"
	build/bin/MatrixTranspose -q -t -x 12000 "$@" # verification takes forever at 14000+
	echo ""
	echo "MersenneTwisterSimple ($i)"
	build/bin/MersenneTwisterSimple -q -t -x 45000 "$@" # ~max
	echo ""
	echo "NBodySimple ($i)"
	build/bin/NBodySimple -q -t -x 23808 "$@" # fails for -x > 25000 # verification fails for iterations > 1
	echo ""
	echo "PrefixSum ($i)"
	build/bin/PrefixSum -q -t -x 256 "$@" # intel cannot do more
	echo ""
	echo "RadixSort ($i)"
	build/bin/RadixSort -q -t -x 33554432 "$@" # more possible
	echo ""
	echo "Reduction ($i)"
	build/bin/Reduction -q -t -x 9999974 "$@" # -x 1000000000 starts swapping :p
	echo ""
	echo "SimpleConvolution ($i)"
	build/bin/SimpleConvolution -q -t -x 8192 -y 8192 -m 1 "$@" # intel cannot do more
done

### Now run the speedup test
./SpeedupTest.sh benchmark.cfg

### And move everything into a  different folder
mkdir -p benchmark
mv *.pkt.txt benchmark/
mv *.amd.txt benchmark/
mv *.intel.txt benchmark/
mv benchmark.cfg.* benchmark
