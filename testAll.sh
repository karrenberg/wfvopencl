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

run build/bin/Test2D "$@"
run build/bin/Test2D2 "$@"
run build/bin/TestBarrier "$@"
run build/bin/TestBarrier2 "$@"
run build/bin/TestConstantIndex "$@"
run build/bin/TestDynCheckSpeed "$@"
run build/bin/TestLinearAccess "$@"
run build/bin/TestLoopBarrier "$@"
run build/bin/TestLoopBarrier2 "$@"
run build/bin/TestSimple "$@"
run build/bin/TestUnaligned "$@"

printStats
