#############################################################################
# If not stated otherwise in this file or this component's Licenses.txt file
# the following copyright and licenses apply:
#
# Copyright 2016 RDK Management
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#############################################################################


include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/src/rtmessage)
include_directories(${PROJECT_SOURCE_DIR}/src/core)
include_directories(${PROJECT_SOURCE_DIR}/src/session_manager)

add_library(
    rbus
    SHARED
    rbus.c
    rbus_handle.c
    rbus_message.c
    rbus_value.c
    rbus_property.c
    rbus_object.c
    rbus_buffer.c
    rbus_filter.c
    rbus_element.c
    rbus_valuechange.c
    rbus_subscriptions.c
    rbus_tokenchain.c
    rbus_asyncsubscribe.c
    rbus_intervalsubscription.c
    rbus_config.c)

target_link_libraries(rbus rbuscore rtMessage -fPIC -pthread)

target_include_directories (rbus PUBLIC ${RDKLOGGER_INCLUDE_DIRS})

set_target_properties(rbus
    PROPERTIES SOVERSION "0"
    VERSION "${PROJECT_VERSION}")

install (TARGETS rbus
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

install (DIRECTORY ${PROJECT_SOURCE_DIR}/include/ DESTINATION "include/rbus")

if (MSG_ROUNDTRIP_TIME)
  message("Message rountrip time calculation enabled")
  add_definitions(-DMSG_ROUNDTRIP_TIME=1)
endif (MSG_ROUNDTRIP_TIME)
