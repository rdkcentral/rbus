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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <rbus.h>
#include <rtLog.h>
#include "../common/runningParamHelper.h"

int getDurationElementTree();
int getDurationValueAPI();
int getDurationPropertyAPI();
int getDurationObjectAPI();
int getDurationValue();
int getDurationValueChange();
int getDurationTables();
int getDurationSubscribe();
int getDurationSubscribeEx();
int getDurationEvents();
int getDurationMethods();
int getDurationFilter();
int getDurationPartialPath();

void testElementTree(rbusHandle_t handle, int* countPass, int* countFail);
void testValueAPI(rbusHandle_t handle, int* countPass, int* countFail);
void testPropertyAPI(rbusHandle_t handle, int* countPass, int* countFail);
void testObjectAPI(rbusHandle_t handle, int* countPass, int* countFail);
void testValue(rbusHandle_t handle, int* countPass, int* countFail);
void testValueChange(rbusHandle_t handle, int* countPass, int* countFail);
void testSubscribe(rbusHandle_t handle, int* countPass, int* countFail);
void testSubscribeEx(rbusHandle_t handle, int* countPass, int* countFail);
void testTables(rbusHandle_t handle, int* countPass, int* countFail);
void testEvents(rbusHandle_t handle, int* countPass, int* countFail);
void testMethods(rbusHandle_t handle, int* countPass, int* countFail);
void testFilter(rbusHandle_t handle, int* countPass, int* countFail);
void testPartialPath(rbusHandle_t handle, int* countPass, int* countFail);

typedef int (*getDurationFunc_t)();
typedef void (*runTestFunc_t)(rbusHandle_t handle, int* countPass, int* countFail);

typedef struct testInfo_t
{
    int enabled;
    char name[50];
    bool useBus;
    getDurationFunc_t getDuration;
    runTestFunc_t runTest;
    int countPass;
    int countFail;
}testInfo_t;

typedef enum testType_t
{
    TestElementTree,
    TestValueAPI,
    TestPropertyAPI,
    TestObjectAPI,
    TestValue,
    TestValueChange,
    TestSubscribe,
    TestSubscribeEx,
    TestTables,
    TestEvents,
    TestMethods,
    TestFilter,
    TestPartialPath,
    TestTypeMax
}testType_t;

testInfo_t testList[TestTypeMax] = {
    { 0, "ElementTree", false, getDurationElementTree, testElementTree, 0, 0 },
    { 0, "ValueAPI", false, getDurationValueAPI, testValueAPI, 0, 0 },
    { 0, "PropertyAPI", false, getDurationPropertyAPI, testPropertyAPI, 0, 0 },
    { 0, "ObjectAPI", false, getDurationObjectAPI, testObjectAPI, 0, 0 },
    { 0, "Value", true, getDurationValue, testValue, 0, 0 },
    { 0, "ValueChange", true, getDurationValueChange, testValueChange, 0, 0 },
    { 0, "Subscribe", true, getDurationSubscribe, testSubscribe, 0, 0 },
    { 0, "SubscribeEx", true, getDurationSubscribeEx, testSubscribeEx, 0, 0 },
    { 0, "Tables", true, getDurationTables, testTables, 0, 0 },
    { 0, "Events", true, getDurationEvents, testEvents, 0, 0 },
    { 0, "Methods", true, getDurationMethods, testMethods, 0, 0 },
    { 0, "Filter", true, getDurationFilter, testFilter, 0, 0 },
    { 0, "PartialPath", true, getDurationPartialPath, testPartialPath, 0, 0 },
};

void printUsage()
{
    int i;
    printf("rbusTestConsumer [OPTIONS] [TESTS]\n");
    printf(" OPTIONS:\n");
    printf(" -d : get the estimated duration in seconds the test will take (must include desired test flags)\n");
    printf(" -a : run all tests (ignores indiviual test flags)\n");
    printf(" -l : set log level\n");
    printf(" TESTS: (if -a is not set then specify which tests to run)\n");
    for(i=0; i<TestTypeMax; ++i)
    {
        printf(" --%s\n", testList[i].name);
    }
}

int main(int argc, char *argv[])
{
    int i,a;
    int duration = 0;
    bool getDuration = false;
    bool useBus = false;
    int rc = RBUS_ERROR_SUCCESS;
    rbusHandle_t handle;
    rtLogLevel logLevel = RT_LOG_WARN;

    for(a=1; a<argc; ++a)
    {
        if(strcmp(argv[a], "--help")==0)
        {
            printUsage();
            return 0;
        }

        /*get estimated time to run all tests*/
        if(strcmp(argv[a], "-d")==0)
        {
            getDuration = true;
        }
        else if(strcmp(argv[a], "-a")==0)
        {
            for(i=0; i<TestTypeMax; ++i)
            {
                testList[i].enabled = 1;
            }
        }
        else if(strcmp(argv[a], "-l")==0)
        {
            a++;
            if(a < argc)
                logLevel = rtLogLevelFromString(argv[a]);
        }
        else
        {
            int argKnown = 0;
            if(strlen(argv[a]) > 2)
            {
                for(i=0; i<TestTypeMax; ++i)
                {
                    if(strcmp(testList[i].name, argv[a]+2)==0)
                    {
                        testList[i].enabled = 1;
                        argKnown = 1;
                        break;
                    }
                }
            }
            if(!argKnown)
            {
                printf("Invalid arg %s\n", argv[a]);
                printUsage();   
                return 0;
            }
        }
    }

    for(i=0; i<TestTypeMax; ++i)
    {
        if(testList[i].enabled)
        {
            duration += testList[i].getDuration();

            if(testList[i].useBus)
                useBus = true;
        }
    }

    if(getDuration)
    {
        printf("%d\n", duration);
        return 0;
    }

    printf("consumer: start\n");

    srand(time(NULL));
    
    rtLog_SetLevel(logLevel);

    if(useBus)
    {
        rc = rbus_open(&handle, "TestConsumer");

        rtLog_SetLevel(logLevel);/*set it again in case we still are overriding in rbus_open*/

        printf("consumer: rbus_open=%d\n", rc);
        if(rc != RBUS_ERROR_SUCCESS)
            goto exit1;

        /*tell provider we are starting*/
        if(runningParamConsumer_Set(handle, "Device.TestProvider.TestRunning", true) != RBUS_ERROR_SUCCESS)
        {
            printf("consumer: provider didn't get ready in time\n");
            goto exit2;
        }
        sleep(1);
    }

    /*run all enabled tests*/
    for(i=0; i<TestTypeMax; ++i)
    {
        if(testList[i].enabled)
        {
            printf(
                "\n"
                "####################################\n"
                "#\n#\n" 
                "# RUNNING TEST\n"
                "#\n" 
                "# NAME: %s\n"
                "#\n" 
                "# DURATION: %ds\n"
                "#\n#\n" 
                "####################################\n\n", 
                testList[i].name, 
                testList[i].getDuration()
            );

            testList[i].runTest(handle, &testList[i].countPass, &testList[i].countFail);
        }
    }

    /*tell provider we are done*/
exit2:
    /*close*/
    if(useBus)
    {
        usleep(250000);
        runningParamConsumer_Set(handle, "Device.TestProvider.TestRunning", false);
        usleep(250000);

        rc = rbus_close(handle);
        printf("consumer: rbus_close=%d\n", rc);
    }

exit1:
    printf("run this test with valgrind to look for memory issues:\n");
    printf("valgrind --leak-check=full --show-leak-kinds=all ./rbusTestConsumer -a");
    printf("\n");
    printf("###################################################\n");
    printf("#                                                 #\n");
    printf("#                  TEST RESULTS                   #\n");
    printf("#                                                 #\n");
    printf("# %-15s|%-15s|%-15s #\n", "Test Name", "Pass Count", "Fail Count");
    printf("# ---------------|---------------|--------------- #\n");
    for(i=0; i<TestTypeMax; ++i)
    {
        if(testList[i].enabled)
        {
            printf("# %-15s|%-15d|%-15d #\n",
                testList[i].name, 
                testList[i].countPass, 
                testList[i].countFail);
            if(testList[i].countFail)
                rc = RBUS_ERROR_BUS_ERROR;
        }
    }
    printf("#                                                 #\n");
    printf("###################################################\n");

    printf("consumer: exit\n");
    return rc;
}


