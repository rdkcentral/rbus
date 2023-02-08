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
include_directories(
	../..
    ../include)

if (BUILD_FOR_DESKTOP)
	include_directories(
        ${CMAKE_SOURCE_DIR}/msgpack/src/msgpack/include)
	link_directories(
		${CMAKE_SOURCE_DIR}/msgpack/build)
endif (BUILD_FOR_DESKTOP)

add_executable(sample_server sample_server.c)
add_dependencies(sample_server rbus-core)
target_link_libraries(sample_server rbus-core)

add_executable(sample_client sample_client.c)
add_dependencies(sample_client rbus-core)
target_link_libraries(sample_client rbus-core)

install (TARGETS sample_server sample_client 
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_executable(bin_client bin_client.c)
add_dependencies(bin_client rbus-core)
target_link_libraries(bin_client rbus-core)

add_executable(bin_server bin_server.c)
add_dependencies(bin_server rbus-core)
target_link_libraries(bin_server rbus-core)

#add_executable(rbus_perf rbus_perf.c)
#add_dependencies(rbus_perf rbus-core)
#target_link_libraries(rbus_perf rbus-core)

install (TARGETS bin_server bin_client #rbus_perf 
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_executable(rbus_sample_event_server sample_event_server.c)
add_dependencies(rbus_sample_event_server rbus-core)
target_link_libraries(rbus_sample_event_server rbus-core)

add_executable(rbus_sample_event_client sample_event_client.c)
add_dependencies(rbus_sample_event_client rbus-core)
target_link_libraries(rbus_sample_event_client rbus-core)

add_executable(rbus_sample_wildcard_server sample_wildcard_server.c)
add_dependencies(rbus_sample_wildcard_server rbus-core)
target_link_libraries(rbus_sample_wildcard_server rbus-core)

add_executable(rbus_sample_wildcard_server2 sample_wildcard_server2.c)
add_dependencies(rbus_sample_wildcard_server2 rbus-core)
target_link_libraries(rbus_sample_wildcard_server2 rbus-core)

add_executable(rbus_sample_wildcard_client sample_wildcard_client.c)
add_dependencies(rbus_sample_wildcard_client rbus-core)
target_link_libraries(rbus_sample_wildcard_client rbus-core)

add_executable(rbus_every_client every_client.c)
add_dependencies(rbus_every_client rbus-core)
target_link_libraries(rbus_every_client rbus-core)

add_executable(rbus_nested_test_server nested_server.c)
add_dependencies(rbus_nested_test_server rbus-core)
target_link_libraries(rbus_nested_test_server rbus-core)

#add_executable(rbus_load_test loadtest.c)
#add_dependencies(rbus_load_test rbus-core)
#target_link_libraries(rbus_load_test rbus-core)

add_executable(rbus_obj_lookup test_obj_lookup.c)
add_dependencies(rbus_obj_lookup rbus-core)
target_link_libraries(rbus_obj_lookup rbus-core)

install (TARGETS rbus_sample_event_server rbus_sample_event_client rbus_sample_wildcard_server rbus_sample_wildcard_client #rbus_load_test 
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})


if (BUILD_RBUSCORE_BENCHMARK_TEST)
add_subdirectory(benchmark_test)
endif (BUILD_RBUSCORE_BENCHMARK_TEST)
