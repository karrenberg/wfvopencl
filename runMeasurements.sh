#!/bin/bash
./runPacketizerBlackScholes.sh -q -e -t -i 50 && \
./runPacketizerBlackScholesSimple.sh -q -e -t -i 50 && \
./runPacketizerBitonicSort.sh -q -e -x 131072 -t -i 50 && \
./runPacketizerDCT.sh -q -e -t -i 50 && \
./runPacketizerEigenValue.sh -q -e -t -x 4096 -i 50 && \
./runPacketizerFastWalshTransform.sh -q -e -t -x 1000000 -i 50 && \
./runPacketizerFloydWarshall.sh -q -e -t -x 512 -i 50 && \
./runPacketizerHistogram.sh -q -e -t -x 8192 -y 8192 -i 50 && \
./runPacketizerMandelbrot.sh -q -e -t -x 8192 && \
./runPacketizerMatrixTranspose.sh -q -e -t -x 10000 -i 20 && \
./runPacketizerPrefixSum.sh -q -e -t -x 16384 -i 1000 && \
./runPacketizerRadixSort.sh -q -e -t -x 1000000 -i 50 && \
#./runPacketizerReduction.sh -q -e && \
./runPacketizerSimpleConvolution.sh -q -e -x 1024 -m 16 -i 50 && \
build/bin/AmbientOcclusionRenderer

# EigenValue fails for size > 4096 (also with ATI's own implementation!)
# PrefixSum fails for larger sizes when using more than one iteration
# PrefixSum uses a maximum of  size 2048 in ATI's implementation
