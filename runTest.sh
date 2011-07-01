#!/bin/bash
build/bin/TestSimple "$@" && \
build/bin/Test2D "$@" && \
build/bin/Test2D2 "$@" && \
build/bin/TestLinearAccess "$@" && \
build/bin/TestBarrier "$@" && \
build/bin/TestBarrier2 "$@" && \
build/bin/TestLoopBarrier "$@" && \
build/bin/BitonicSort -q -e -x 32 "$@" && \
build/bin/BinomialOptionSimple -q -e -x 32 "$@" && \
build/bin/BlackScholesSimple -q -e "$@" && \
build/bin/DCT -q -e "$@" && \
build/bin/DwtHaar1D -q -e "$@" && \
build/bin/EigenValue -q -e -x 16 "$@" && \
build/bin/FastWalshTransform -q -e -x 32 "$@" && \
build/bin/FloydWarshall -q -e -x 16 "$@" && \
build/bin/Histogram -q -e "$@" && \
build/bin/MandelbrotSimple -q -e -x 16 "$@" && \
build/bin/MatrixTranspose -q -e -x 512 "$@" && \
build/bin/NBodySimple -q -e -x 128 "$@" && \
build/bin/PrefixSum -q -e "$@" && \
build/bin/RadixSort -q -e "$@" && \
build/bin/Reduction -q -e "$@" && \
build/bin/SimpleConvolution -q -e -x 8 "$@"

# TODO: add TestLoopBarrier2
