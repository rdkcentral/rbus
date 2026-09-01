##########################################################################
# If not stated otherwise in this file or this component's LICENSE
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
project(dmProvider)
option(BUILD_DMPROVIDER_LIB "BUILD_DMPROVIDER_LIB" ON)
option(BUILD_DM_SAMPLE_APP "BUILD_DM_SAMPLE_APP" ON)

set(DM_SOURCE_FILES dmValue.cpp dmProvider.cpp)
set(DM_APP_FILES dm_provider.cpp)

  add_library(
    dataProvider
    SHARED
    dataProvider/dmProviderDatabase.cpp
    dataProvider/dmProviderHost.cpp
    dataProvider/dmPropertyInfo.cpp
    dataProvider/dmProviderInfo.cpp
    dataProvider/dmQueryResult.cpp
    dataProvider/dmProvider.cpp
    dataProvider/dmError.cpp
    dataProvider/dmValue.cpp
    dataProvider/dmClient.cpp)
  add_dependencies(
    dataProvider
    rtMessage)
  target_link_libraries(
    dataProvider
    rtMessage)

if (BUILD_DMCLI)
  add_executable(dmcli dataProvider/dmcli.cpp)
  if (BUILD_FOR_DESKTOP)
    add_dependencies(dmcli cjson rtMessage dataProvider)
    add_definitions("-DDEFAULT_DATAMODELDIR=\"${CMAKE_CURRENT_SOURCE_DIR}/dataProvider/data\"")
  else()
    add_definitions("-DDEFAULT_DATAMODELDIR=\"${RDK_FSROOT_PATH}/etc/model\"")
  endif (BUILD_FOR_DESKTOP)
  if (INCLUDE_BREAKPAD)
    add_dependencies(dmcli ${BREAKPAD_PATH})
    target_link_libraries(dmcli dataProvider -lbreakpadwrap rtMessage)
  else ()
    target_link_libraries(dmcli dataProvider rtMessage)
  endif (INCLUDE_BREAKPAD)
endif (BUILD_DMCLI)

if (BUILD_DMCLI_SAMPLE_APP)
    # sample provider for general
    add_executable(sample_provider_gen dataProvider/sample_provider_gen.cpp)
    if (BUILD_FOR_DESKTOP)
      add_dependencies(sample_provider_gen dataProvider)
    endif (BUILD_FOR_DESKTOP)
    target_link_libraries(sample_provider_gen ${LIBRARY_LINKER_OPTIONS} rtMessage dataProvider)
    # sample provider for wifi
    add_executable(sample_provider_wifi dataProvider/sample_provider_wifi.cpp)
    if (BUILD_FOR_DESKTOP)
      add_dependencies(sample_provider_wifi dataProvider)
    endif (BUILD_FOR_DESKTOP)
    target_link_libraries(sample_provider_wifi ${LIBRARY_LINKER_OPTIONS} rtMessage dataProvider)
if (ENABLE_DM_PROFILE)
    message("Enabling dm profile")
    set(CMAKE_CFLAGS "${CMAKE_C_FLAGS} -pg")
endif (ENABLE_DM_PROFILE)

if (BUILD_DMPROVIDER_LIB)
    message("Building dm provider lib")
    add_library(dmProvider_lib SHARED ${DM_SOURCE_FILES})
    set_target_properties(dmProvider_lib PROPERTIES OUTPUT_NAME "dmprovider")
    target_link_libraries(dmProvider_lib ${LIBRARY_LINKER_OPTIONS} -pthread -lcjson -lrtMessaging)
endif (BUILD_DMPROVIDER_LIB)

if (BUILD_DM_SAMPLE_APP)
    message ("Building dm sample app")
    set(CMAKE_CFLAGS " ${CMAKE_C_FLAGS}")
    link_directories(${DM_LINK_DIRECTORIES})
    add_executable(sample_app_dmcli ${DM_APP_FILES})
    set_target_properties(sample_app_dmcli PROPERTIES OUTPUT_NAME "dm_provider")
    target_link_libraries(sample_app_dmcli ${LIBRARY_LINKER_OPTIONS} dmProvider_lib)
    target_compile_definitions(sample_app_dmcli PRIVATE RT_PLATFORM_LINUX)
endif (BUILD_DM_SAMPLE_APP)

# These use the full path versions we derive in the top level CMakeLists
install (TARGETS LIBRARY DESTINATION ${FULL_CMAKE_INSTALL_LIBDIR})
install (TARGETS ARCHIVE DESTINATION ${FULL_CMAKE_INSTALL_LIBDIR})
