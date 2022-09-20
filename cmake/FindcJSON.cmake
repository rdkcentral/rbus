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


find_package(PkgConfig)

find_library(CJSON_LIBRARIES NAMES cjson)
#find_path(CJSON_INCLUDE_DIRS NAMES cJSON.h PATH_SUFFIXES cjson)

set(CJSON_LIBRARIES ${CJSON_LIBRARIES} CACHE PATH "Path to cjson library")

if (RDKC_BUILD)
  set(CJSON_INCLUDE_DIRS ${CMAKE_INSTALL_PREFIX}/include/cjson)
else ()
  set(CJSON_INCLUDE_DIRS ${CMAKE_INSTALL_PREFIX})
endif (RDKC_BUILD)

message(STATUS "cmake prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "cjson lib: ${CJSON_LIBRARIES}")
message(STATUS "cjson inc dir :${CJSON_INCLUDE_DIRS}")

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(cJSON DEFAULT_MSG CJSON_INCLUDE_DIRS CJSON_LIBRARIES)
mark_as_advanced(
    CJSON_FOUND
    CJSON_INCLUDE_DIRS
    CJSON_LIBRARIES
    CJSON_LIBRARY_DIRS
    CJSON_FLAGS)
