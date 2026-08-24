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

set(RDK_OTEL_LIB_DIR "/usr/lib" CACHE PATH
    "Directory containing rdk_otlp.so from the rbus install")

include_directories(${PROJECT_SOURCE_DIR}/include)

add_library(rbus_otel_bridge SHARED
    rbus_otel_bridge.cpp
)

set_target_properties(rbus_otel_bridge PROPERTIES
    SOVERSION 1
    VERSION 1.0.0
    OUTPUT_NAME rbus_otel_bridge
)

target_include_directories(rbus_otel_bridge
    PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${RBUS_INCLUDE_DIR}
)

find_library(RDK_OTLP_LIBRARY
    NAMES rdk_otlp
    PATHS ${RDK_OTEL_LIB_DIR}
    REQUIRED
)

message(STATUS "Found rdk_otlp: ${RDK_OTLP_LIBRARY}")

target_link_libraries(rbus_otel_bridge
    PRIVATE
        ${RDK_OTLP_LIBRARY}
)

install(TARGETS rbus_otel_bridge
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/
    DESTINATION "include/rbus"
)

