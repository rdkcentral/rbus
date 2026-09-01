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
include_directories(.
                   ../../include
                   ../core
                    ../rtmessage)

add_executable(rbus_session_mgr session_manager.c)
add_dependencies(rbus_session_mgr rbus rbuscore)
target_link_libraries(rbus_session_mgr rbus rbuscore)

if (BUILD_SESSIONMGR_SAMPLE_APPS)
add_executable(rbus_session_mgr_client test/sample_client.c)
add_dependencies(rbus_session_mgr_client rbus rbuscore)
target_link_libraries(rbus_session_mgr_client rbus rbuscore)
endif (BUILD_SESSIONMGR_SAMPLE_APPS)

install (TARGETS rbus_session_mgr
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install(FILES rbus_session_mgr.h DESTINATION "include/rbus")
