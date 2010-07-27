#!/bin/bash
./runPacketizerMandelbrot.sh -q -e -x 16 && \
./runPacketizerFastWalshTransform.sh -q -e -x 32 && \
./runPacketizerBitonicSort.sh -q -e -x 32 && \
./runPacketizerSimpleConvolution.sh -q -e -x 8 && \
./runPacketizerEigenValue.sh -q -e -x 16 && \
./runPacketizerFloydWarshall.sh -q -e -x 16 && \
./runPacketizerHistogram.sh -q -e
# packetized currently only passes first 3
