#!/bin/bash

#
#  If not stated otherwise in this file or this component's Licenses.txt file
#  the following copyright and licenses apply:
#
#  Copyright 2016 RDK Management
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
# 
#  http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
#  

APPNAME=Sample
XML="../Sample_dm.xml"
RBUS_INSTALL_DIR=../../../../../install/
INCS="-I./ -I../../src -I$RBUS_INSTALL_DIR/usr/include/rbus -I$RBUS_INSTALL_DIR/usr/include/rtmessage -I$RBUS_INSTALL_DIR/usr/include/"
LIBS="-L$RBUS_INSTALL_DIR/usr/lib -lcjson"
BUILD_SCRIPT="../../scripts/rbus_code_generator_ccsp_style.sh"
PYTHON_SCRIPT="../../scripts/rbus_code_generator_ccsp_style.py"

#run the script
CFLAGS=$INCS LD_FLAGS=$LIBS $BUILD_SCRIPT $PYTHON_SCRIPT $XML $APPNAME


