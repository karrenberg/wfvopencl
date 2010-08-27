#!/bin/bash
echo ""
echo "BlackScholes"
./runPacketizerBlackScholes.sh -q -e -t -i 50
echo ""
echo "BlackScholesSimple"
./runPacketizerBlackScholesSimple.sh -q -e -t -i 50
echo ""
echo "BitonicSort"
./runPacketizerBitonicSort.sh -q -e -x 131072 -t -i 50
echo ""
echo "DCT"
./runPacketizerDCT.sh -q -e -t -x 8192 -y 8192 -i 5
echo ""
echo "EigenValue"
./runPacketizerEigenValue.sh -q -e -t -x 4096 -i 50
echo ""
echo "FastWalshTransform"
./runPacketizerFastWalshTransform.sh -q -e -t -x 1000000 -i 50
echo ""
echo "FloydWarshall"
./runPacketizerFloydWarshall.sh -q -e -t -x 512 -i 50
echo ""
echo "Histogram"
./runPacketizerHistogram.sh -q -e -t -x 8192 -y 8192 -i 50
echo ""
echo "Mandelbrot"
./runPacketizerMandelbrot.sh -q -e -t -x 8192
echo ""
echo "MatrixTranspose"
./runPacketizerMatrixTranspose.sh -q -e -t -x 10000 -i 20
echo ""
echo "PrefixSum"
./runPacketizerPrefixSum.sh -q -e -t -x 16384 -i 1000
echo ""
echo "RadixSort"
./runPacketizerRadixSort.sh -q -e -t -x 1000000 -i 50
#echo ""
#echo "Reduction"
#./runPacketizerReduction.sh -q -e
echo ""
echo "SimpleConvolution"
./runPacketizerSimpleConvolution.sh -q -e -t -x 1024 -y 1024 -m 1 -i 50

# EigenValue fails for size > 4096 (also with ATI's own implementation!)
# PrefixSum fails for larger sizes when using more than one iteration
# PrefixSum uses a maximum of  size 2048 in ATI's implementation
