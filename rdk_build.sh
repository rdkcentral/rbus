#!/bin/bash
##########################################################################
# If not stated otherwise in this file or this component's Licenses.txt
# file the following copyright and licenses apply:
#
# Copyright 2016 RDK Management
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
##########################################################################
#

#######################################
#
# Build Framework standard script for
#
# rdklogger component

# use -e to fail on any shell issue
# -e is the requirement from Build Framework
set -e


# default PATHs - use `man readlink` for more info
# the path to combined build
export RDK_PROJECT_ROOT_PATH=${RDK_PROJECT_ROOT_PATH-`readlink -m ..`}
export COMBINED_ROOT=$RDK_PROJECT_ROOT_PATH

# path to build script (this script)
export RDK_SCRIPTS_PATH=${RDK_SCRIPTS_PATH-`readlink -m $0 | xargs dirname`}

# path to components sources and target
export RDK_SOURCE_PATH=${RDK_SOURCE_PATH-`readlink -m .`}
export RDK_TARGET_PATH=${RDK_TARGET_PATH-$RDK_SOURCE_PATH}

# fsroot and toolchain (valid for all devices)
export RDK_FSROOT_PATH=${RDK_FSROOT_PATH-`readlink -m $RDK_PROJECT_ROOT_PATH/sdk/fsroot/ramdisk`}
export RDK_TOOLCHAIN_PATH=${RDK_TOOLCHAIN_PATH-`readlink -m $RDK_PROJECT_ROOT_PATH/sdk/toolchain/staging_dir`}

# default component name
export RDK_COMPONENT_NAME=${RDK_COMPONENT_NAME-`basename $RDK_SOURCE_PATH`}
export RDK_DIR=$RDK_PROJECT_ROOT_PATH
export RDK_DUMP_SYMS=${RDK_PROJECT_ROOT_PATH}/utility/prebuilts/breakpad-prebuilts/x86/dump_syms

if [ "$XCAM_MODEL" == "SCHC2" ]; then
    if [ "$RDK_COMPONENT_NAME" == "xwrbus" ]; then
        echo "Setting environmental variables and Pre rule makefile for xw-xCam2"
        source ${RDK_PROJECT_ROOT_PATH}/build/components/realtek/sdk/setenv2
    else
        echo "Setting environmental variables and Pre rule makefile for xCam2"
        source ${RDK_PROJECT_ROOT_PATH}/build/components/amba/sdk/setenv2
    fi
elif [ "$XCAM_MODEL" == "SERXW3" ] || [ "$XCAM_MODEL" == "SERICAM2" ] || [ "$XCAM_MODEL" == "XHB1" ]; then
    echo "Setting environmental variables and Pre rule makefile for xCam/iCam2/DBC"
    source ${RDK_PROJECT_ROOT_PATH}/build/components/sdk/setenv2
elif [ "$XCAM_MODEL" == "XHC3" ]; then
    echo "Setting environmental variables and Pre rule makefile for XHC3"
    source ${RDK_PROJECT_ROOT_PATH}/build/components/sdk/setenv2
else #No Matching platform
        echo "Source environment that include packages for your platform. The environment variables PROJ_PRERULE_MAK_FILE should refer to the platform s PreRule make"
fi

#export COMP_BASE_PATH=${RDK_SCRIPTS_PATH%/*}
export COMP_BASE_PATH=${RDK_SCRIPTS_PATH}/../
export SEARCH_PATH="$RDK_FSROOT_PATH/usr;$RDK_FSROOT_PATH/usr/local;$RDK_FSROOT_PATH/usr/include"
export INSTALL_PATH=$RDK_FSROOT_PATH/usr

#The cross compile tools are exported already in XHB1;lets avoid only that (Because SOURCETOOLCHAIN is not exported in XHB1)
if [ "$XCAM_MODEL" != "XHB1" ]; then
    export CC=${SOURCETOOLCHAIN}gcc
    export CXX=${SOURCETOOLCHAIN}g++
    export GXX=${SOURCETOOLCHAIN}g++
    export AR=${SOURCETOOLCHAIN}ar
    export LD=${SOURCETOOLCHAIN}ld
    export STRIP=${SOURCETOOLCHAIN}strip
fi

# parse arguments
INITIAL_ARGS=$@

function usage()
{
    set +x
    echo "Usage: `basename $0` [-h|--help] [-v|--verbose] [action]"
    echo "    -h    --help                  : this help"
    echo "    -v    --verbose               : verbose output"
    echo "    -p    --platform  =PLATFORM   : specify platform for rbus"
    echo
    echo "Supported actions:"
    echo "      configure, clean, build (DEFAULT), rebuild, install"
}

# options may be followed by one colon to indicate they have a required argument
if ! GETOPT=$(getopt -n "build.sh" -o hvp: -l help,verbose,platform: -- "$@")
then
    usage
    exit 1
fi

eval set -- "$GETOPT"

while true; do
  case "$1" in
    -h | --help ) usage; exit 0 ;;
    -v | --verbose ) set -x ;;
    -p | --platform ) CC_PLATFORM="$2" ; shift ;;
    -- ) shift; break;;
    * ) break;;
  esac
  shift
done

ARGS=$@

# functional modules
export CFLAGS=" -Wno-format-truncation -I${RDK_PROJECT_ROOT_PATH}/rdklogger/include -I${RDK_FSROOT_PATH}/usr/include"
function configure()
{
    pd=`pwd`
    echo "rbus Compiling started"
    mkdir -p ${RDK_PROJECT_ROOT_PATH}/opensource/src/rbus/build
    cd ${RDK_PROJECT_ROOT_PATH}/opensource/src/rbus/build
    cmake -DCMAKE_INSTALL_PREFIX=${INSTALL_PATH} -DCMAKE_PREFIX_PATH=${SEARCH_PATH} -DENABLE_RDKLOGGER=OFF -DCMAKE_EXE_LINKER_FLAGS="-Wl,-rpath-link,${RDK_FSROOT_PATH}/usr/lib" ..
}

function clean()
{
    rm -rf ${RDK_PROJECT_ROOT_PATH}/opensource/src/rbus/build
}

function build()
{
    cd ${RDK_PROJECT_ROOT_PATH}/opensource/src/rbus/build
    make
    $RDK_DUMP_SYMS src/rtmessage/librtMessage.so > src/rtmessage/librtMessage.so.sym
    $RDK_DUMP_SYMS src/rtmessage/rtrouted > src/rtmessage/rtrouted.sym
    mv src/rtmessage/*.sym $RDK_PROJECT_ROOT_PATH/sdk/fsroot/syms

    $STRIP src/rtmessage/rtrouted

    cp -f src/rtmessage/librt* ${RDK_PROJECT_ROOT_PATH}/opensource/lib
    cp -f src/rtmessage/rtrouted ${RDK_PROJECT_ROOT_PATH}/opensource/bin
    cp -f ${RDK_PROJECT_ROOT_PATH}/opensource/src/rbus/src/rtmessage/rtrouted_default.conf ${RDK_FSROOT_PATH}/etc/rtrouted.conf
    cd -
}

function rebuild()
{
    clean
    configure
    build
}

function install()
{
    cd ${RDK_PROJECT_ROOT_PATH}/opensource/src/rbus/build
    make install
}

# run the logic

#these args are what left untouched after parse_args
HIT=false

for i in "$ARGS"; do
    case $i in
        configure)  HIT=true; configure ;;
        clean)      HIT=true; clean ;;
        build)      HIT=true; build ;;
        rebuild)    HIT=true; rebuild ;;
        install)    HIT=true; install ;;
        *)
            #skip unknown
        ;;
    esac
done

# if not HIT do build by default
if ! $HIT; then
  build
fi
