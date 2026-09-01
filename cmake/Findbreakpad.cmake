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

if (RDKC_BUILD)
  find_library(BREAKPAD_LIBRARIES NAMES breakpadwrap)
  find_path(BREAKPAD_INCLUDE_DIRS NAMES breakpadwrap.h PATH_SUFFIXES ${CMAKE_INSTALL_PREFIX})
else ()
  find_library(BREAKPAD_LIBRARIES NAMES breakpadwrapper)
  find_path(BREAKPAD_INCLUDE_DIRS NAMES breakpad_wrapper.h PATH_SUFFIXES ${CMAKE_INSTALL_PREFIX})
endif (RDKC_BUILD)

set(BREAKPAD_LIBRARIES ${BREAKPAD_LIBRARIES} CACHE PATH "Path to breakpad library")
set(BREAKPAD_INCLUDE_DIRS ${BREAKPAD_INCLUDE_DIRS} )
set(BREAKPAD_INCLUDE_DIRS ${BREAKPAD_INCLUDE_DIRS} CACHE PATH "Path to breakpad include")

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(BREAKPAD DEFAULT_MSG BREAKPAD_INCLUDE_DIRS BREAKPAD_LIBRARIES)
mark_as_advanced(
    BREAKPAD_FOUND
    BREAKPAD_INCLUDE_DIRS
    BREAKPAD_LIBRARIES
    BREAKPAD_LIBRARY_DIRS
    BREAKPAD_FLAGS)
