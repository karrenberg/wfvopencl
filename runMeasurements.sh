#!/bin/bash
echo ""
echo "TestSimple"
build/bin/TestSimple "$@"
echo ""
echo "AmbientOcclusionRenderer"
build/bin/AmbientOcclusionRenderer -q -e -t -x 1024 -y 1024 "$@"
echo ""
echo "BinomialOption"
build/bin/BinomialOption -q -e -t -x 16384 -i 10 "$@"
echo ""
echo "BinomialOptionSimple"
build/bin/BinomialOptionSimple -q -e -t -x 16384 -i 10 "$@"
echo ""
echo "BitonicSort"
build/bin/BitonicSort -q -e -x 1048576 -t -i 5 "$@" # takes forever on AMD (~55sec)
echo ""
echo "BlackScholes"
build/bin/BlackScholes -q -e -t -x 16384 -i 50 "$@"
echo ""
echo "BlackScholesSimple"
build/bin/BlackScholesSimple -q -e -t -x 16384 -i 50 "$@"
echo ""
echo "DCT"
build/bin/DCT -q -e -t -x 8192 -y 8192 -i 5 "$@"
echo ""
echo "DwtHaar1D"
build/bin/DwtHaar1D -q -e -t -x 8388608 -i 5 "$@"
echo ""
echo "EigenValue"
build/bin/EigenValue -q -e -t -x 4096 -i 50 "$@" # fails on all platforms for values > 8192
echo ""
echo "FastWalshTransform"
build/bin/FastWalshTransform -q -e -t -x 134217728 -i 5 "$@"
echo ""
echo "FloydWarshall"
build/bin/FloydWarshall -q -e -t -x 512 -i 100 "$@" # max size 512
echo ""
echo "Histogram"
build/bin/Histogram -q -e -t -x 15872 -y 15872 -i 10 "$@" # max is 16384x15872
echo ""
echo "Mandelbrot"
build/bin/Mandelbrot -q -e -t -x 8192 -i 10 "$@"
echo ""
echo "MandelbrotSimple"
build/bin/MandelbrotSimple -q -e -t -x 8192 -i 10 "$@" # max size, takes forever on AMD (~35sec)
echo ""
echo "MatrixTranspose"
build/bin/MatrixTranspose -q -e -t -x 12000 -i 10 "$@" # verification takes forever at 14000+
echo ""
echo "MersenneTwisterSimple"
build/bin/MersenneTwisterSimple -q -e -t -x 45000 -i 20 "$@" # ~max
echo ""
echo "NBody"
build/bin/NBody -q -t -x 23808 -i 50 "$@" # verification fails for iterations > 1
echo ""
echo "NBodySimple"
build/bin/NBodySimple -q -t -x 23808 -i 50 "$@" # fails for -x > 25000 # verification fails for iterations > 1
echo ""
echo "PrefixSum"
build/bin/PrefixSum -q -e -t -x 256 -i 1000 "$@" # intel cannot do more
echo ""
echo "RadixSort"
build/bin/RadixSort -q -e -t -x 33554432 -i 10 "$@" # more possible
echo ""
echo "Reduction"
build/bin/Reduction -q -e -t -x 9999974 -i 50 "$@" # -x 1000000000 starts swapping :p
echo ""
echo "SimpleConvolution"
build/bin/SimpleConvolution -q -e -t -x 8192 -y 8192 -m 1 -i 50 "$@" # intel cannot do more

# EigenValue fails for size > 4096 (also with ATI's own implementation!)
# PrefixSum fails for larger sizes when using more than one iteration
# PrefixSum uses a maximum of  size 2048 in ATI's implementation
