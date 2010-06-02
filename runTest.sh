#!/bin/bash
./runPacketizerMandelbrot.sh -q -e -x 16 && \
./runPacketizerFastWalshTransform.sh -q -e -x 32 && \
./runPacketizerBitonicSort.sh -q -e -x 32 && \
./runPacketizerSimpleConvolution.sh -q -e -x 8 && \
./runPacketizerEigenValue.sh -q -e -x 16
