#!/bin/bash
./runPacketizerBlackScholesSimple.sh -q -e && \
./runPacketizerBitonicSort.sh -q -e -x 32 && \
#./runPacketizerEigenValue.sh -q -e -x 16 && \
./runPacketizerFastWalshTransform.sh -q -e -x 32 && \
#./runPacketizerFloydWarshall.sh -q -e -x 16 && \
./runPacketizerHistogram.sh -q -e && \
./runPacketizerMandelbrot.sh -q -e -x 16 && \
./runPacketizerMatrixTranspose.sh -q -e && \
#./runPacketizerPrefixSum.sh -q -e && \
./runPacketizerReduction.sh -q -e -t
#./runPacketizerSimpleConvolution.sh -q -e -x 8
