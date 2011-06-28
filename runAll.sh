#!/bin/bash
echo
echo BinarySearch
build/bin/BinarySearch -q -e "$@"
echo
echo BinomialOption
build/bin/BinomialOption -q -e "$@"
echo
echo BinomialOptionSimple
build/bin/BinomialOptionSimple -q -e "$@"
echo
echo BitonicSort
build/bin/BitonicSort -q -e -x 32 "$@"
echo
echo BlackScholes
build/bin/BlackScholes -q -e "$@"
echo
echo BlackScholesSimple
build/bin/BlackScholesSimple -q -e "$@"
echo
echo DCT
build/bin/DCT -q -e "$@"
echo
echo DwtHaar1D
build/bin/DwtHaar1D -q -e "$@"
echo
echo EigenValue
build/bin/EigenValue -q -e -x 16 "$@"
echo
echo FFT
build/bin/FFT -q -e "$@"
echo
echo FastWalshTransform
build/bin/FastWalshTransform -q -e -x 32 "$@"
echo
echo FloydWarshall
build/bin/FloydWarshall -q -e -x 16 "$@"
echo
echo Histogram
build/bin/Histogram -q -e "$@"
echo
echo Mandelbrot
build/bin/Mandelbrot -q -e -x 16 "$@"
echo
echo MandelbrotSimple
build/bin/MandelbrotSimple -q -e -x 16 "$@"
echo
echo MatrixMulImage
build/bin/MatrixMulImage -q -e "$@"
echo
echo MatrixMultiplication
build/bin/MatrixMultiplication -q -e "$@"
echo
echo MatrixTranspose
build/bin/MatrixTranspose -q -e -x 512 "$@"
echo
echo MersenneTwister
build/bin/MersenneTwister -q -e "$@"
echo
echo MersenneTwisterSimple
build/bin/MersenneTwisterSimple -q -e "$@"
echo
echo MonteCarloAsian
build/bin/MonteCarloAsian -q -e "$@"
echo
echo NBody
build/bin/NBody -q -e "$@"
echo
echo NBodySimple
build/bin/NBodySimple -q -e "$@"
echo
echo PrefixSum
build/bin/PrefixSum -q -e "$@"
echo
echo QuasiRandomSequence
build/bin/QuasiRandomSequence -q -e "$@"
echo
echo RadixSort
build/bin/RadixSort -q -e "$@"
echo
echo RecursiveGaussian
build/bin/RecursiveGaussian -q -e "$@"
echo
echo Reduction
build/bin/Reduction -q -e "$@"
echo
echo ScanLargeArrays
build/bin/ScanLargeArrays -q -e "$@"
echo
echo SimpleConvolution
build/bin/SimpleConvolution -q -e -x 8 "$@"
echo
echo SobelFilter
build/bin/SobelFilter -q -e "$@"
echo
echo URNG
build/bin/URNG -q -e "$@"
echo

