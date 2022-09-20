/*
  * If not stated otherwise in this file or this component's Licenses.txt file
  * the following copyright and licenses apply:
  *
  * Copyright 2016 RDK Management
  *
  * Licensed under the Apache License, Version 2.0 (the "License");
  * you may not use this file except in compliance with the License.
  * You may obtain a copy of the License at
  *
  * http://www.apache.org/licenses/LICENSE-2.0
  *
  * Unless required by applicable law or agreed to in writing, software
  * distributed under the License is distributed on an "AS IS" BASIS,
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
*/
/*************************************************************
Unit test application for testing rbus code.
Test Cases with identified Tests are available in the files:
rbus_unit_test_marshalling.cpp
rbus_unit_test_client.cpp
rbus_unit_test_server.cpp
rbus_unit_stresstest_server.cpp
**************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
extern "C" {
#include "rbuscore.h"
}
#include "gtest_app.h"


#define DEFAULT_RESULT_FILEPATH "/tmp/Gtest_Report/"
#define DEFAULT_RESULT_FILENAME "rbuscore_gtest_report.xml"
#define DEFAULT_RESULT_BUFFERSIZE 128

GTEST_API_ int main(int argc, char *argv[])
{
    char testresults_fullfilepath[DEFAULT_RESULT_BUFFERSIZE];
    char buffer[DEFAULT_RESULT_BUFFERSIZE];

    memset( testresults_fullfilepath, 0, DEFAULT_RESULT_BUFFERSIZE );
    memset( buffer, 0, DEFAULT_RESULT_BUFFERSIZE );

    //::testing::GTEST_FLAG(filter) = "TestServer*:TestClient*"; 

    snprintf( testresults_fullfilepath, DEFAULT_RESULT_BUFFERSIZE, "xml:%s%s" , DEFAULT_RESULT_FILEPATH , DEFAULT_RESULT_FILENAME);
    ::testing::GTEST_FLAG(output) = testresults_fullfilepath;

    ::testing::InitGoogleTest( &argc, argv );
    return RUN_ALL_TESTS();
}
