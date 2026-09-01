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
#   example usage:
#   CFLAGS="-I./ -I../src -I../../includes" LD_FLAGS="../../../lib" ../generator/rbus_code_generator_ccsp_style.sh ../generator/rbus_code_generator_ccsp_style.py ./Sample.xml Sample

echo "CFLAGS=$CFLAGS"
echo "LD_FLAGS=$LD_FLAGS"
echo "PYTHON SCRIPT=$1"
echo "User datamodel XML=$2"
echo "File Base Name=$3"

#run the python code generator
python $1 $2 $3

#build test app to verify generated code compiles
gcc $CFLAGS $LD_FLAGS -o "$3_app" "$3_app.c" "$3_rbus.c" ../../src/rbus_context_helpers.c -lrbus -lrbuscore -lrtMessage -lmsgpackc

echo "Done"

