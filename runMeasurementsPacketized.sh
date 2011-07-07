#!/bin/bash
echo ""
echo "TestSimple"
build/bin/TestSimple "$@"
echo ""
echo "BlackScholesSimple"
build/bin/BlackScholesSimple -q -e -t -x 16384 -i 50 "$@"
echo ""
echo "BitonicSort"
build/bin/BitonicSort -q -e -x 1048576 -t -i 50 "$@"
echo ""
echo "DCT"
build/bin/DCT -q -e -t -x 8192 -y 8192 -i 5 "$@"
echo ""
echo "FastWalshTransform"
build/bin/FastWalshTransform -q -e -t -x 134217728 -i 5 "$@"
echo ""
echo "Histogram"
build/bin/Histogram -q -e -t -x 15872 -y 15872 -i 10 "$@" # max is 16384x15872
echo ""
echo "MandelbrotSimple"
build/bin/MandelbrotSimple -q -e -t -x 8192 -i 5 "$@" # max size
echo ""
echo "MatrixTranspose"
build/bin/MatrixTranspose -q -e -t -x 12000 -i 10 "$@" # verification takes forever at 14000+
echo ""
echo "SimpleConvolution"
build/bin/SimpleConvolution -q -e -t -x 8192 -y 8192 -m 1 -i 50 "$@" # intel cannot do more

# EigenValue fails for size > 4096 (also with ATI's own implementation!)
# PrefixSum fails for larger sizes when using more than one iteration
# PrefixSum uses a maximum of  size 2048 in ATI's implementation
