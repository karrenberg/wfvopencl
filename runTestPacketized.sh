#!/bin/bash
./runPacketizerBitonicSort.sh -q -e -x 32 && \
#./runPacketizerEigenValue.sh -q -e -x 16 && \
./runPacketizerFastWalshTransform.sh -q -e -x 32 && \
#./runPacketizerFloydWarshall.sh -q -e -x 16 && \
#./runPacketizerHistogram.sh -q -e && \
./runPacketizerMandelbrot.sh -q -e -x 16
#./runPacketizerPrefixSum.sh -q -e && \
#./runPacketizerSimpleConvolution.sh -q -e -x 8
