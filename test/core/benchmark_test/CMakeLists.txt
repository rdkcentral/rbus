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
            ../../..
            ../../include)

find_package(benchmark REQUIRED)

add_executable(rbus_benchmark_test_app
               rbus_test_util.c
               rbus_benchmark_test_app.cpp)
add_definitions("-DHAVE_POSIX_REGEX")
target_link_libraries(rbus_benchmark_test_app rbuscore benchmark ${CMAKE_THREAD_LIBS_INIT})

add_test(rbus_benchmark_test rbus_benchmark_test_app --benchmark_min_time=0.01)

enable_testing()
install (TARGETS rbus_benchmark_test_app
         RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
