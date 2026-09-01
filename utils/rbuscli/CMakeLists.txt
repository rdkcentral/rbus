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
include_directories(${PROJECT_SOURCE_DIR}/src/core)
include_directories(${PROJECT_SOURCE_DIR}/src/rtmessage)

add_executable(rbuscli rbuscli.c)
target_link_libraries(rbuscli rbus)
add_dependencies(rbuscli rbus linenoise)

if (BUILD_FOR_DESKTOP)
	target_link_libraries(rbuscli rbus -llinenoise)
else (BUILD_FOR_DESKTOP)
	target_link_libraries(rbuscli ${LINENOISE_LIBRARIES})
	target_include_directories (rbuscli PUBLIC ${LINENOISE_INCLUDE_DIRS})
endif (BUILD_FOR_DESKTOP)

install (TARGETS rbuscli RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

