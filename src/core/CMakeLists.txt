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
include_directories(${PROJECT_SOURCE_DIR}/src/rtmessage)
add_library(
    rbuscore
    SHARED
    rbuscore.c
    rbuscore_message.c)

add_dependencies(rbuscore msgpack rtMessage safeclib)

target_link_libraries(rbuscore rtMessage ${MSGPACK_LIBRARIES} -lpthread ${SAFEC_LIBRARIES})

set_target_properties(rbuscore
    PROPERTIES SOVERSION "0"
    VERSION "${PROJECT_VERSION}")


install (TARGETS rbuscore
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

install (FILES rbuscore.h rbuscore_message.h  rbuscore_types.h  DESTINATION "include/rbus")
