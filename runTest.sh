#!/bin/bash
./runPacketizerBlackScholes.sh -q -e && \
./runPacketizerBlackScholesSimple.sh -q -e && \
./runPacketizerBitonicSort.sh -q -e -x 32 && \
./runPacketizerDCT.sh -q -e && \
./runPacketizerEigenValue.sh -q -e -x 16 && \
./runPacketizerFastWalshTransform.sh -q -e -x 32 && \
./runPacketizerFloydWarshall.sh -q -e -x 16 && \
./runPacketizerHistogram.sh -q -e && \
./runPacketizerMandelbrot.sh -q -e -x 16 && \
./runPacketizerMatrixTranspose.sh -q -e -x 512 && \
./runPacketizerPrefixSum.sh -q -e && \
./runPacketizerRadixSort.sh -q -e && \
#./runPacketizerReduction.sh -q -e && \
./runPacketizerSimpleConvolution.sh -q -e -x 8
