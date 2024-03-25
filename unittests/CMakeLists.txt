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
find_package(GTest QUIET)

include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/src/rbus)
include_directories(${PROJECT_SOURCE_DIR}/src/core)
include_directories(${PROJECT_SOURCE_DIR}/src/rtmessage)
include_directories(${PROJECT_SOURCE_DIR}/src/session_manager)

string(REPLACE "-Werror" "" CMAKE_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG})
string(REPLACE "-Werror" "" CMAKE_CXX_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE})
if (NOT GTEST_INCLUDE_DIR)
    message("Warning GTest wasn't found. GTest library will be built.")
    include(ExternalProject)
    #include(GoogleTest)

    ExternalProject_Add(
      gtest
      GIT_REPOSITORY    https://github.com/google/googletest.git
      GIT_TAG           release-1.12.0
      PREFIX ${CMAKE_CURRENT_BINARY_DIR}/gtest
      INSTALL_COMMAND "")

    ExternalProject_Get_Property(gtest source_dir binary_dir)

    add_library(libgtest IMPORTED STATIC GLOBAL)
    add_dependencies(libgtest gtest)
    set_target_properties(libgtest PROPERTIES
      "IMPORTED_LOCATION" "${binary_dir}/lib/libgtest.a"
      "IMPORTED_LINK_INTERFACE_LIBRARIES" "${CMAKE_THREAD_LIBS_INIT}")

    add_library(libgmock IMPORTED STATIC GLOBAL)
    add_dependencies(libgmock gtest)
    set_target_properties(libgmock PROPERTIES
      "IMPORTED_LOCATION" "${binary_dir}/lib/libgmock.a"
      "IMPORTED_LINK_INTERFACE_LIBRARIES" "${CMAKE_THREAD_LIBS_INIT}")

    include_directories("${source_dir}/googletest/include" "${source_dir}/googlemock/include")
else ()
    include_directories("${GTEST_INCLUDE_DIR}")
endif()

# someone needs to figure out how to run this stuff under valgrind
add_custom_target(test_memcheck
  COMMAND "${CMAKE_CTEST_COMMAND} --force-new-ctest-process --test-action memcheck"
  COMMAND cat "${CMAKE_BINARY_DIR}/Testing/Temporary/MemoryChecker.*.log")

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

add_executable(rbus_gtest.bin
  rbusValueTest.cpp
  rbusTokenTest.cpp
  rbusElementTest.cpp
  rbusFunctionalityTest.cpp
  rbusProvider.cpp
  rbusConsumer.cpp
  rbusObjectTest.cpp
  rbusPropertyTest.cpp
  rbusFilterTest.cpp
  rbusSessionTest.cpp
  rbusApiNegTest.cpp
  rbus_unit_test_marshalling.cpp
  rbus_test_util.c
  rbus_unit_test_client.cpp
  rbus_unit_stresstest_server.cpp
  rbus_unit_test_server.cpp
  rbus_unit_test_multiple_objects.cpp
  rbus_unit_test_multiple_servers.cpp
  rbus_unit_test_nested_servers.cpp
  rbus_unit_test_event_client.cpp
  rbus_unit_test_event_server.cpp
  util.cpp
  main.cpp)
add_dependencies(rbus_gtest.bin rbus)

if (NOT GTEST_INCLUDE_DIR)
    target_link_libraries(rbus_gtest.bin rbus libgtest libgmock gcov)
else ()
    target_link_libraries(rbus_gtest.bin rbus gtest gcov)
endif()

install (TARGETS rbus_event_server rbus_test_server rbus_gtest.bin
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

if (ENABLE_CODE_COVERAGE)
    install(CODE "execute_process(COMMAND find . -name *.gcno -exec tar -rvf rbus_src_gcno.tar {} \;)")
    install(FILES ${CMAKE_BINARY_DIR}/rbus_src_gcno.tar DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

# not sure why this isn't working
#gtest_discover_tests(rbus_gtest.bin)
