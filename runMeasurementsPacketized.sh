#!/bin/bash
echo ""
echo "TestSimple"
build/bin/TestSimple "$@"
echo ""
echo "BlackScholesSimple"
build/bin/BlackScholesSimple -q -t -x 16384 "$@"
echo ""
echo "BitonicSort"
build/bin/BitonicSort -q -x 1048576 -t "$@"
echo ""
echo "DCT"
build/bin/DCT -q -t -x 8192 -y 8192 "$@"
echo ""
echo "FastWalshTransform"
build/bin/FastWalshTransform -q -t -x 134217728 "$@"
echo ""
echo "Histogram"
build/bin/Histogram -q -t -x 15872 -y 15872 "$@" # max is 16384x15872
echo ""
echo "MandelbrotSimple"
build/bin/MandelbrotSimple -q -t -x 8192 "$@" # max size
echo ""
echo "MatrixTranspose"
build/bin/MatrixTranspose -q -t -x 12000 "$@" # verification takes forever at 14000+
echo ""
echo "SimpleConvolution"
build/bin/SimpleConvolution -q -t -x 8192 -y 8192 -m 1 "$@" # intel cannot do more

# EigenValue fails for size > 4096 (also with ATI's own implementation!)
# PrefixSum fails for larger sizes when using more than one iteration
# PrefixSum uses a maximum of  size 2048 in ATI's implementation
