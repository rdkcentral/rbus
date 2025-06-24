#!/bin/bash
set -e
set -x

##############################
#      Setup WorkSpace       #
##############################
GITHUB_WORKSPACE="${PWD}"
ls -la ${GITHUB_WORKSPACE}
cd ${GITHUB_WORKSPACE}

#######################################
#  Install Dependencies and packages  #
#######################################
apt update && apt install -y libcurl4-openssl-dev libgtest-dev lcov gcovr libmsgpack* libcjson-dev  build-essential

