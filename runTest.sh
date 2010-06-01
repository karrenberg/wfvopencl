#!/bin/bash
./runPacketizerMandelbrot.sh -q -e -x 64 && \
./runPacketizerFastWalshTransform.sh -q -e && \
./runPacketizerBitonicSort.sh -q -e && \
./runPacketizerSimpleConvolution.sh -q -e && \
./runPacketizerEigenValue.sh -q -e
