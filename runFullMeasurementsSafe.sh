#!/bin/bash
scons -c
scons -j4
./runMeasurementsSafe.sh 2>&1 | tee LOG1
scons -c
scons packetize=1 -j4
./runMeasurementsSafe.sh 2>&1 | tee LOG2
scons -c
scons openmp=1 -j4
./runMeasurementsSafe.sh 2>&1 | tee LOG3
scons -c
scons packetize=1 openmp=1 -j4
./runMeasurementsSafe.sh 2>&1 | tee LOG4
