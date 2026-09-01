#!/bin/bash
set -e
set -x
##############################
#      Setup WorkSpace       #
##############################
GITHUB_WORKSPACE="${PWD}"
ls -la ${GITHUB_WORKSPACE}
cd ${GITHUB_WORKSPACE}

##############################
#        Build RBUS          #
##############################
echo "======================================================================================"
echo "buliding rbus for coverity"

cd ${GITHUB_WORKSPACE}
cmake -S "$GITHUB_WORKSPACE" -B build/rbus  -DCMAKE_INSTALL_PREFIX="$GITHUB_WORKSPACE/install/usr" -DBUILD_FOR_DESKTOP=ON -DENABLE_UNIT_TESTING=ON -DCMAKE_BUILD_TYPE=Debug

cmake --build build/rbus --target install
echo "======================================================================================"
exit 0
