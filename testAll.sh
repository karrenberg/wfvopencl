#!/bin/bash

total=0
success=0

run() {
    # show info
    echo " *** Running $@"

    # run
    if [ -f $1 ]; then
        $@
    else
        echo " *** Skipping (not found)"
        echo
        echo
        echo
        return
    fi


    # keep stats
    if [ $? -eq 0 ]; then
        success=$((success+1))
    fi
    total=$((total+1))

    # space
    echo
    echo
    echo
}

printStats() {
    echo " *** Number of tests: $total"
    echo " *** Successful:      $success"
    echo " *** Failed:          $((total - success))"
}

run build/bin/TestSimple "$@"
run build/bin/Test2D "$@"
run build/bin/Test2D2 "$@"
run build/bin/TestLinearAccess "$@"
run build/bin/TestBarrier "$@"
run build/bin/TestBarrier2 "$@"
run build/bin/TestLoopBarrier "$@"
run build/bin/TestLoopBarrier2 "$@"
run build/bin/AmbientOcclusionRenderer -q -e -x 256 -y 256 "$@"
run build/bin/BinarySearch -q -e "$@"
run build/bin/BinomialOption -q -e "$@"
run build/bin/BinomialOptionSimple -q -e "$@"
run build/bin/BitonicSort -q -e -x 32 "$@"
run build/bin/BlackScholes -q -e "$@"
run build/bin/BlackScholesSimple -q -e "$@"
run build/bin/DCT -q -e "$@"
run build/bin/DwtHaar1D -q -e "$@"
run build/bin/EigenValue -q -e -x 16 "$@"
run build/bin/FFT -q -e "$@"
run build/bin/FastWalshTransform -q -e -x 32 "$@"
run build/bin/FloydWarshall -q -e -x 16 "$@"
run build/bin/Histogram -q -e "$@"
run build/bin/Mandelbrot -q -e -x 16 "$@"
run build/bin/MandelbrotSimple -q -e -x 16 "$@"
run build/bin/MatrixMulImage -q -e "$@"
run build/bin/MatrixMultiplication -q -e "$@"
run build/bin/MatrixTranspose -q -e -x 512 "$@"
run build/bin/MersenneTwister -q -e "$@"
run build/bin/MersenneTwisterSimple -q -e "$@"
run build/bin/MonteCarloAsian -q -e "$@"
run build/bin/NBody -q -e "$@"
run build/bin/NBodySimple -q -e "$@"
run build/bin/PrefixSum -q -e "$@"
run build/bin/QuasiRandomSequence -q -e "$@"
run build/bin/RadixSort -q -e "$@"
run build/bin/RecursiveGaussian -q -e "$@"
run build/bin/Reduction -q -e "$@"
run build/bin/ScanLargeArrays -q -e "$@"
run build/bin/SimpleConvolution -q -e -x 8 "$@"
run build/bin/SobelFilter -q -e "$@"
run build/bin/URNG -q -e "$@"

printStats
