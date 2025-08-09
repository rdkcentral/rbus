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

cmake_minimum_required (VERSION 2.8.12)

if (POLICY CMP0048)
  cmake_policy(SET CMP0048 NEW)
  project(rbus VERSION 2.7.0)
else ()
  project(rbus)
  set(PROJECT_VERSION "2.7.0")
endif (POLICY CMP0048)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/")

enable_testing()

find_package(PkgConfig REQUIRED)

set(CMAKE_C_FLAGS_DEBUG "-fPIC -Wall -Werror -Wextra -Wno-type-limits -fno-inline ${CMAKE_C_FLAGS_DEBUG} -DRBUS_ALWAYS_ON  -D_TIME_BITS=64 -D_FILE_OFFSET_BITS=64")
set(CMAKE_C_FLAGS_RELEASE "-fPIC -Wall -Werror -Wextra ${CMAKE_C_FLAGS_RELEASE} -DRBUS_ALWAYS_ON  -D_TIME_BITS=64 -D_FILE_OFFSET_BITS=64")

set(CMAKE_CXX_FLAGS_DEBUG "-fPIC -Wall -Werror -Wextra -Wno-type-limits -fno-inline ${CMAKE_C_FLAGS_DEBUG} -DRBUS_ALWAYS_ON  -D_TIME_BITS=64 -D_FILE_OFFSET_BITS=64 ")
set(CMAKE_CXX_FLAGS_RELEASE "-fPIC -Wall -Werror -Wextra ${CMAKE_C_FLAGS_RELEASE} -DRBUS_ALWAYS_ON  -D_TIME_BITS=64 -D_FILE_OFFSET_BITS=64")

if(ENABLE_ADDRESS_SANITIZER)
	set(CMAKE_C_FLAGS "-fsanitize=address ${CMAKE_C_FLAGS}")
	set(CMAKE_CXX_FLAGS "-fsanitize=address ${CMAKE_CXX_FLAGS}")
endif(ENABLE_ADDRESS_SANITIZER)

include(GNUInstallDirs)

IF (NOT DEFINED CMAKE_INSTALL_BINDIR)
    message ("Setting Bindir")
    SET(CMAKE_INSTALL_BINDIR bin)
ENDIF ()

IF (NOT DEFINED CMAKE_INSTALL_LIBDIR)
    message ("Setting Libdir")
	SET(CMAKE_INSTALL_LIBDIR lib)
ENDIF ()

message ("Using ${CMAKE_INSTALL_BINDIR}  and ${CMAKE_INSTALL_LIBDIR}")

SET(FULL_CMAKE_INSTALL_BINDIR ${CMAKE_INSTALL_PREFIX}/bin)
SET(FULL_CMAKE_INSTALL_LIBDIR ${CMAKE_INSTALL_PREFIX}/lib)


include(CMakePackageConfigHelpers)

SET (CMAKE_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/rbus")

configure_package_config_file(rbusConfig.cmake.in
  rbusConfig.cmake
  INSTALL_DESTINATION "${CMAKE_INSTALL_CMAKEDIR}")

write_basic_package_version_file(
  rbusConfigVersion.cmake
  VERSION 1.1.1
  COMPATIBILITY AnyNewerVersion )

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/rbusConfig.cmake"
              "${CMAKE_CURRENT_BINARY_DIR}/rbusConfigVersion.cmake"
        DESTINATION "${CMAKE_INSTALL_CMAKEDIR}" )


option(BUILD_RBUS_DAEMON "BUILD_RBUS_DAEMON" ON)
option(BUILD_RBUS_SAMPLE_APPS "BUILD_RBUS_SAMPLE_APPS" ON)
option(BUILD_RBUS_TEST_APPS "BUILD_RBUS_TEST_APPS" ON)
option(BUILD_ONLY_RTMESSAGE "BUILD_ONLY_RTMESSAGE" OFF)

option(BUILD_RTMESSAGE_SAMPLE_APP "BUILD_RTMESSAGE_SAMPLE_APP" OFF)
option(BUILD_RTMESSAGE_DATAPROVIDER_LIB "BUILD_RTMESSAGE_DATAPROVIDER_LIB" OFF)
option(BUILD_RBUSCORE_BENCHMARK_TEST "BUILD_RBUSCORE_BENCHMARK_TEST" OFF)
option(BUILD_RBUSCORE_TEST_APPS "BUILD_RBUSCORE_TEST_APPS" OFF)
option(BUILD_SESSIONMGR_SAMPLE_APPS "BUILD_SESSIONMGR_SAMPLE_APPS" OFF)

option(ENABLE_RDKLOGGER "ENABLE_RDKLOGGER" OFF)
option(RDKC_BUILD "RDKC_BUILD" OFF)
option(WITH_SPAKE2 "WITH_SPAKE2" OFF)
option(MSG_ROUNDTRIP_TIME "MSG_ROUNDTRIP_TIME" OFF)


if (BUILD_FOR_DESKTOP)
  set(MSG_ROUNDTRIP_TIME ON)
  message("Building for desktop")
  include_directories(${CMAKE_INSTALL_PREFIX}/include)

  include(ExternalProject)
  ExternalProject_Add(
    linenoise
    GIT_REPOSITORY https://github.com/antirez/linenoise.git
    PREFIX deps
    UPDATE_DISCONNECTED 1
    INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}
    INSTALL_COMMAND ""
    CONFIGURE_COMMAND ""
    BUILD_IN_SOURCE 1
    CMAKE_ARGS "-DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}"
    #PATCH_COMMAND patch -p1 < ${CMAKE_CURRENT_SOURCE_DIR}/003-linenoise-oflag.patch
    BUILD_COMMAND gcc -c -Wall -Werror -fpic linenoise.c COMMAND gcc --shared -o liblinenoise.so linenoise.o)

  include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}/deps/src/linenoise)

  link_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}/deps/src/linenoise
    ${CMAKE_BINARY_DIR}
    ${LIBRARY_DIR})

  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/deps/src/linenoise/liblinenoise.so DESTINATION ${CMAKE_INSTALL_LIBDIR})
  find_package(cJSON REQUIRED)
  find_package(msgpack REQUIRED) 
  

else()
  find_package(cJSON REQUIRED)
  IF (NOT BUILD_ONLY_RTMESSAGE)
   find_package(msgpack REQUIRED)
   find_package(linenoise REQUIRED)
  ENDIF()
  set(INCLUDE_DIR ${CMAKE_INSTALL_PREFIX}/include)
  set(LIBRARY_DIR ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR})

  include_directories(${CMAKE_CURRENT_SOURCE_DIR})
  include_directories(${CJSON_INCLUDE_DIRS})
  IF (NOT BUILD_ONLY_RTMESSAGE)
   include_directories(${MSGPACK_INCLUDE_DIRS})
  ENDIF ()

  link_directories(${CMAKE_CURRENT_SOURCE_DIR})

  if (ENABLE_RDKLOGGER)
    include_directories(${RDKLOGGER__INCLUDE_DIRS})
    add_definitions(-DENABLE_RDKLOGGER)
  endif (ENABLE_RDKLOGGER)
  
  if (INCLUDE_BREAKPAD)
    add_definitions(-DINCLUDE_BREAKPAD)
  endif (INCLUDE_BREAKPAD)

  if (RDKC_BUILD)
    add_definitions(-DRDKC_BUILD)
  endif (RDKC_BUILD)
endif (BUILD_FOR_DESKTOP)


if (ENABLE_CODE_COVERAGE)
    include (CodeCoverage)
    if (NOT BUILD_FOR_DESKTOP)
        string(APPEND COVERAGE_COMPILER_FLAGS " -fprofile-generate=/tmp/")
    endif()
    append_coverage_compiler_flags()
    set(COVERAGE_EXCLUDES
      "${CMAKE_CURRENT_BINARY_DIR}/unittests/gtest/*"
      "test/*"
      "utils/*"
      "/usr/include/*"
      "sampleapps/*")
    option (ENABLE_UNIT_TESTING "ENABLE_UNIT_TESTING" ON)
    if (BUILD_FOR_DESKTOP)
        if (ENABLE_CODE_COVERAGE_GOVR)
            setup_target_for_coverage_gcovr_html(NAME "${PROJECT_NAME}CodeCoverageUI")
            setup_target_for_coverage_gcovr_xml(NAME "${PROJECT_NAME}CodeCoverage")
        else ()
            setup_target_for_coverage_lcov(NAME "${PROJECT_NAME}CodeCoverageUI")
        endif()
    endif()
endif()

if (ENABLE_STATIC_CODE_ANALYZER)
    find_program(CLANG_TIDY_EXE run-clang-tidy REQUIRED)
               set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
    #set(CLANG_TIDY_CHECKS "-checks=-*,modernize-*,performance-*,bugprone-*,clang-diagnostic-*,clang-analyzer-*")
    set(CLANG_TIDY_CHECKS "-checks=-google-*,-llvm-*,bugprone-*,cert-*,modernize-*,clang-diagnostic-*")
    add_custom_target(
      code-analysis
      ALL
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      COMMAND ${CLANG_TIDY_EXE} ${CLANG_TIDY_CHECKS} 2>&1 > ${CMAKE_CURRENT_BINARY_DIR}/clang-tidy-output.txt
                 DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/compile_commands.json)
endif()

if (ENABLE_RDKLOGGER)
    find_package(rdklogger REQUIRED)
    add_definitions(-DENABLE_RDKLOGGER)
endif (ENABLE_RDKLOGGER)


add_subdirectory(src)

IF (NOT BUILD_ONLY_RTMESSAGE)
   add_subdirectory(utils)
   add_subdirectory(test)
ENDIF ()

if(BUILD_RBUS_SAMPLE_APPS)
    add_subdirectory(sampleapps)
endif()

if (ENABLE_UNIT_TESTING)
    add_subdirectory(unittests)
endif()
