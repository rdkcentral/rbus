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
include_directories(${PROJECT_SOURCE_DIR}/src/core)
include_directories(${PROJECT_SOURCE_DIR}/src/rtmessage)

if (BUILD_RBUS_UNIT_TEST)
find_package(GTest QUIET)
#Certain builds/branches don't come with GTest support. Do not attempt to build unit-test binaries if that's the case. 
if (NOT GTEST_INCLUDE_DIR)
    message("Warning! GTest wasn't found. Unit test capabilities won't be built.")
else ()
    if (BUILD_FOR_DESKTOP)
        add_compile_options(-D BUILD_FOR_DESKTOP)
    endif ()

    add_executable(rbus_test_server rbus_test_util.c rbus_test_server.c)
    add_dependencies(rbus_test_server rbuscore)
    target_link_libraries(rbus_test_server rbuscore)

    add_executable(rbus_event_server rbus_test_util.c rbus_event_server.c)
    add_dependencies(rbus_event_server rbuscore)
    target_link_libraries(rbus_event_server rbuscore)

   enable_testing()
    include_directories(${GTEST_INCLUDE_DIR})

    add_executable(rbuscore_gtest.bin
                   rbus_unit_test_marshalling.cpp
                   rbus_test_util.c
                   rbus_unit_test_client.cpp
                   rbus_unit_stresstest_server.cpp
                   rbus_unit_test_server.cpp
                   rbus_unit_test_multiple_objects.cpp
                   rbus_unit_test_multiple_servers.cpp
                   rbus_unit_test_nested_servers.cpp
                   rbus_unit_test_app.cpp
                   rbus_unit_test_event_client.cpp
                   rbus_unit_test_event_server.cpp)
    add_dependencies(rbuscore_gtest.bin rbuscore)
    target_link_libraries(rbuscore_gtest.bin rbuscore
                                           gtest)

    add_test(
        NAME rbuscore_gtest.bin
        COMMAND rbuscore_gtest.bin
    )

    install (TARGETS rbus_event_server rbus_test_server rbuscore_gtest.bin
            RUNTIME DESTINATION bin)
endif ()
endif (BUILD_RBUS_UNIT_TEST)
