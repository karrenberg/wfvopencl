#!/bin/bash
echo ""
echo "BitonicSort"
build/bin/BitonicSort -q -e -x 131072 -t -i 50 "$@"
echo ""
echo "BlackScholesSimple"
build/bin/BlackScholesSimple -q -e -t -x 16384 -i 50 "$@"
echo ""
echo "FastWalshTransform"
build/bin/FastWalshTransform -q -e -t -x 1000000 -i 50 "$@"
echo ""
echo "Histogram"
build/bin/Histogram -q -e -t -x 8192 -y 8192 -i 50 "$@"
echo ""
echo "MandelbrotSimple"
build/bin/MandelbrotSimple -q -e -t -x 4096 -i 5 "$@"
echo ""
echo "MatrixTranspose"
build/bin/MatrixTranspose -q -e -t -x 10000 -i 20 "$@"
echo ""
echo "SimpleConvolution"
build/bin/SimpleConvolution -q -e -t -x 1024 -y 1024 -m 1 -i 50 "$@"

# EigenValue fails for size > 4096 (also with ATI's own implementation!)
# PrefixSum fails for larger sizes when using more than one iteration
# PrefixSum uses a maximum of  size 2048 in ATI's implementation
