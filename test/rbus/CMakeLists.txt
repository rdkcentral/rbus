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
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#############################################################################

include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/src/rtmessage)
include_directories(${PROJECT_SOURCE_DIR}/src/core)
include_directories(${PROJECT_SOURCE_DIR}/src/rbus)
include_directories(${PROJECT_SOURCE_DIR}/test/rbus/common)

add_executable(rbusTestProvider 
    provider/rbusTestProvider.c
    common/runningParamHelper.c
    common/testValueHelper.c)
add_dependencies(rbusTestProvider rbus)
target_link_libraries(rbusTestProvider rbus)

add_executable(multiRbusOpenProvider 
    multiRbusOpenProvider/multiRbusOpenProvider.c
    common/runningParamHelper.c
    common/testValueHelper.c)
add_dependencies(multiRbusOpenProvider rbus)
target_link_libraries(multiRbusOpenProvider rbus)

add_executable(multiRbusOpenMethodProvider 
    multiRbusOpenMethodProvider/multiRbusOpenMethodProvider.c
    common/runningParamHelper.c
    common/testValueHelper.c)
add_dependencies(multiRbusOpenMethodProvider rbus)
target_link_libraries(multiRbusOpenMethodProvider rbus)

add_executable(rbusOpenRegRbusOpenProvider 
    multiRbusOpenProvider/rbusOpenRegRbusOpenProvider.c
    common/runningParamHelper.c
    common/testValueHelper.c)
add_dependencies(rbusOpenRegRbusOpenProvider rbus)
target_link_libraries(rbusOpenRegRbusOpenProvider rbus)

add_executable(multiRbusOpenRbusGetProvider 
    multiRbusOpenRbusGetProvider/multiRbusOpenRbusGetProvider.c
    common/runningParamHelper.c
    common/testValueHelper.c)
add_dependencies(multiRbusOpenRbusGetProvider rbus)
target_link_libraries(multiRbusOpenRbusGetProvider rbus)

add_executable(multiProviderThreadsForSingleEvent 
    multiProviderThreadsForSingleEvent/multiProviderThreadsForSingleEvent.c)
add_dependencies(multiProviderThreadsForSingleEvent rbus)
target_link_libraries(multiProviderThreadsForSingleEvent rbus)

add_executable(multiConsumerThreadsForSingleEvent 
    multiConsumerThreadsForSingleEvent/multiConsumerThreadsForSingleEvent.c)
add_dependencies(multiConsumerThreadsForSingleEvent rbus)
target_link_libraries(multiConsumerThreadsForSingleEvent rbus)

add_executable(rbusTestConsumer 
    consumer/rbusTestConsumer.c
    consumer/elementTree.c
    consumer/valueAPI.c
    consumer/propertyAPI.c
    consumer/objectAPI.c
    consumer/value.c
    consumer/valueChange.c
    consumer/subscribe.c
    consumer/subscribeEx.c
    consumer/tables.c
    consumer/events.c
    consumer/methods.c
    consumer/filter.c
    consumer/partialPath.c
    common/runningParamHelper.c
    common/testValueHelper.c)
add_dependencies(rbusTestConsumer rbus)
target_link_libraries(rbusTestConsumer rbus)

add_executable(multiRbusOpenConsumer 
    multiRbusOpenConsumer/multiRbusOpenConsumer.c
    consumer/elementTree.c
    consumer/valueAPI.c
    consumer/propertyAPI.c
    consumer/objectAPI.c
    consumer/value.c
    consumer/valueChange.c
    consumer/subscribe.c
    consumer/subscribeEx.c
    consumer/tables.c
    consumer/events.c
    consumer/methods.c
    consumer/filter.c
    consumer/partialPath.c
    common/runningParamHelper.c
    common/testValueHelper.c)
add_dependencies(multiRbusOpenConsumer rbus)
target_link_libraries(multiRbusOpenConsumer rbus)

add_executable(multiRbusOpenMethodConsumer 
    multiRbusOpenMethodConsumer/multiRbusOpenMethodConsumer.c
    consumer/elementTree.c
    consumer/valueAPI.c
    consumer/propertyAPI.c
    consumer/objectAPI.c
    consumer/value.c
    consumer/valueChange.c
    consumer/subscribe.c
    consumer/subscribeEx.c
    consumer/tables.c
    consumer/events.c
    consumer/methods.c
    consumer/filter.c
    consumer/partialPath.c
    common/runningParamHelper.c
    common/testValueHelper.c)
add_dependencies(multiRbusOpenMethodConsumer rbus)
target_link_libraries(multiRbusOpenMethodConsumer rbus)

add_executable(rbusOpenSubRbusOpenConsumer  
    multiRbusOpenConsumer/rbusOpenSubRbusOpenConsumer.c
    consumer/elementTree.c
    consumer/valueAPI.c
    consumer/propertyAPI.c
    consumer/objectAPI.c
    consumer/value.c
    consumer/valueChange.c
    consumer/subscribe.c
    consumer/subscribeEx.c
    consumer/tables.c
    consumer/events.c
    consumer/methods.c
    consumer/filter.c
    consumer/partialPath.c
    common/runningParamHelper.c
    common/testValueHelper.c)
add_dependencies(rbusOpenSubRbusOpenConsumer rbus)
target_link_libraries(rbusOpenSubRbusOpenConsumer rbus)

add_executable(multiRbusOpenRbusGetConsumer 
    multiRbusOpenRbusGetConsumer/multiRbusOpenRbusGetConsumer.c
    consumer/elementTree.c
    consumer/valueAPI.c
    consumer/propertyAPI.c
    consumer/objectAPI.c
    consumer/value.c
    consumer/valueChange.c
    consumer/subscribe.c
    consumer/subscribeEx.c
    consumer/tables.c
    consumer/events.c
    consumer/methods.c
    consumer/filter.c
    consumer/partialPath.c
    common/runningParamHelper.c
    common/testValueHelper.c)
add_dependencies(multiRbusOpenRbusGetConsumer rbus)
target_link_libraries(multiRbusOpenRbusGetConsumer rbus)

add_executable(multiRbusOpenRbusSetConsumer 
    multiRbusOpenRbusSetConsumer/multiRbusOpenRbusSetConsumer.c
    consumer/elementTree.c
    consumer/valueAPI.c
    consumer/propertyAPI.c
    consumer/objectAPI.c
    consumer/value.c
    consumer/valueChange.c
    consumer/subscribe.c
    consumer/subscribeEx.c
    consumer/tables.c
    consumer/events.c
    consumer/methods.c
    consumer/filter.c
    consumer/partialPath.c
    common/runningParamHelper.c
    common/testValueHelper.c)
add_dependencies(multiRbusOpenRbusSetConsumer rbus)
target_link_libraries(multiRbusOpenRbusSetConsumer rbus)


set (CMAKE_C_FLAGS "-Wno-overflow ${CMAKE_C_FLAGS}")

install (TARGETS
 multiRbusOpenConsumer multiRbusOpenProvider
 multiRbusOpenMethodProvider multiRbusOpenMethodConsumer
 multiRbusOpenRbusGetProvider multiRbusOpenRbusGetConsumer multiRbusOpenRbusSetConsumer
 multiProviderThreadsForSingleEvent multiConsumerThreadsForSingleEvent 
 rbusOpenRegRbusOpenProvider rbusOpenSubRbusOpenConsumer 
 rbusTestProvider rbusTestConsumer 
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install(FILES rbus_test.sh
        DESTINATION ${CMAKE_INSTALL_BINDIR}
        PERMISSIONS
          OWNER_READ OWNER_WRITE OWNER_EXECUTE
          GROUP_READ GROUP_EXECUTE
          WORLD_READ WORLD_EXECUTE)

add_executable(rbusTestMultiProvider 
    provider/rbusTestMultiProvider.c
    common/runningParamHelper.c)
add_dependencies(rbusTestMultiProvider rbus)
target_link_libraries(rbusTestMultiProvider rbus)

add_executable(rbusTestMultiConsumer 
    consumer/rbusTestMultiConsumer.c
    common/runningParamHelper.c)
add_dependencies(rbusTestMultiConsumer rbus)
target_link_libraries(rbusTestMultiConsumer rbus)

install (TARGETS rbusTestMultiProvider rbusTestMultiConsumer 
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_executable(rbusRecoveryConsumer 
    consumer/rbusRecoveryConsumer.c
    common/runningParamHelper.c)
add_dependencies(rbusRecoveryConsumer rbus)
target_link_libraries(rbusRecoveryConsumer rbus)
install (TARGETS rbusRecoveryConsumer 
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

