#!/bin/bash
echo ""
echo "BinomialOptionSimple"
./runATIBinomialOptionSimple.sh -q -e -t -x 1000 -i 50
echo ""
echo "BlackScholes"
./runATIBlackScholes.sh -q -e -t -i 50
echo ""
echo "BlackScholesSimple"
./runATIBlackScholesSimple.sh -q -e -t -i 50
echo ""
echo "BitonicSort"
./runATIBitonicSort.sh -q -e -x 131072 -t -i 50
echo ""
echo "DCT"
./runATIDCT.sh -q -e -t -x 8192 -y 8192 -i 5
echo ""
echo "EigenValue"
./runATIEigenValue.sh -q -e -t -x 4096 -i 50
echo ""
echo "FastWalshTransform"
./runATIFastWalshTransform.sh -q -e -t -x 1000000 -i 50
echo ""
echo "FloydWarshall"
./runATIFloydWarshall.sh -q -e -t -x 512 -i 50
echo ""
echo "Histogram"
./runATIHistogram.sh -q -e -t -x 8192 -y 8192 -i 50
echo ""
echo "Mandelbrot"
./runATIMandelbrot.sh -q -e -t -x 8192
echo ""
echo "MatrixTranspose"
./runATIMatrixTranspose.sh -q -e -t -x 10000 -i 20
echo ""
echo "PrefixSum"
./runATIPrefixSum.sh -q -e -t -x 16384 -i 1000
echo ""
echo "RadixSort"
./runATIRadixSort.sh -q -e -t -x 1000000 -i 50
echo ""
echo "Reduction"
./runATIReduction.sh -q -e -t -x 100000 -i 50
echo ""
echo "SimpleConvolution"
./runATISimpleConvolution.sh -q -e -t -x 1024 -y 1024 -m 16 -i 50

# EigenValue fails for size > 4096 (also with ATI's own implementation!)
# PrefixSum fails for larger sizes when using more than one iteration
# PrefixSum uses a maximum of  size 2048 in ATI's implementation
