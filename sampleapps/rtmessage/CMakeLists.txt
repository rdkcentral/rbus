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
# sample_super
add_executable(sample_super sample_super.c)
#add_dependencies(sample_super cjson)
add_dependencies(sample_super rtMessage)
target_link_libraries(sample_super rtMessage) 

# sample_send
add_executable(sample_send sample_send.c)
#add_dependencies(sample_send cjson)
add_dependencies(sample_send rtMessage)
target_link_libraries(sample_send rtMessage)

# sample_recv
add_executable(sample_recv sample_recv.c)
add_dependencies(sample_recv rtMessage)
target_link_libraries(sample_recv rtMessage)

# sample_req
add_executable(sample_req sample_req.c)
add_dependencies(sample_req cjson)
add_dependencies(sample_send rtMessage)
target_link_libraries(sample_req rtMessage)

# sample_res
add_executable(sample_res sample_res.c)
add_dependencies(sample_res cjson)
add_dependencies(sample_send rtMessage)
target_link_libraries(sample_res rtMessage)

# rtsend
add_executable(rtsend rtsend.c)
add_dependencies(rtsend cjson)
add_dependencies(rtsend rtMessage)
target_link_libraries(rtsend rtMessage)

