#!/bin/bash
scons -c
scons
./runMeasurementsNoIntelAMD.sh PLATFORM=0 2>&1 | tee LOG1
scons -c
scons packetize=1
./runMeasurementsNoIntelAMD.sh PLATFORM=0 2>&1 | tee LOG2
scons -c
scons openmp=1
./runMeasurementsNoIntelAMD.sh PLATFORM=0 2>&1 | tee LOG3
scons -c
scons packetize=1 openmp=1
./runMeasurementsNoIntelAMD.sh PLATFORM=0 2>&1 | tee LOG4
