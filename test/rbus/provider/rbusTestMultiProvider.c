/*
 * If not stated otherwise in this file or this component's Licenses.txt file the
 * following copyright and licenses apply:
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
#include <rtHashMap.h>
#include <rtLog.h>
#include "../common/test_macros.h"

#define REG_ELEMENTS 1
#define PUB_EVENTS 1

static int gNumProviders = 5;/*each provider will get its own thread*/
static int gNumIterations = 1;/*for each iteration, any previous providers are destroyed and new providers created*/
static int gIterationDuration = 60; /*seconds for each iteration*/
static int gMaxProviderWait = 0; /*max time in usecs a provider thread sleeps before starting*/
static int* gRunFlags = NULL;
static int* gSubscribeFlags = NULL;
static rtHashMap paramValues = NULL;

int parseID(char const* name)
{
    char* pfirst;
    char* pend;
    char buff[64];
    int i = 0;
    pfirst = strstr(name, "Device.MultiProvider");
    if(!pfirst)
    {
        printf("parseID failed %s\n", name);
        return 0;
    }
    pfirst += strlen("Device.MultiProvider");
    pend = pfirst;
    while(*pend >= '0' && *pend <= '9')
    {
        buff[i++] = *pend;
        pend++;
    }
    buff[i] = 0;
    int id = atoi(buff);
    printf("parseID %s=%d\n", name, id);
    return id; 
}

int parseName(char const* name, char* hashName)
{
    char const* pend;
    if(strncmp(name, "Device.MultiProvider", strlen("Device.MultiProvider")) != 0)
    {
        printf("parseName failed %s\n", name);
        return -1;
    }
    pend = strchr(name + strlen("Device.MultiProvider"), '.');
    strncpy(hashName, name, pend-name);
    hashName[pend-name] = 0;
    printf("parseName %s=%s\n", name, hashName);
    return 0;
}

rbusError_t getHandler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    char const* name = rbusProperty_GetName(property);
    char hashName[100];

    (void)handle;
    (void)opts;

    if(parseName(name, hashName))
    {
        TEST_BOOL(0);
        printf("parseName failed %s\n", name);
        return RBUS_ERROR_INVALID_INPUT;
    }
    
    char* sVal = rtHashMap_Get(paramValues, hashName);
    if(!sVal)
    {
        TEST_BOOL(0);
        printf("rtHashMap_Get failed %s\n", hashName);
        return RBUS_ERROR_INVALID_INPUT;
    }

    rbusValue_t value;
    rbusValue_Init(&value);
    rbusValue_SetString(value, sVal);
    rbusProperty_SetValue(property, value);
    rbusValue_Release(value);
    TEST_BOOL(1);
    return RBUS_ERROR_SUCCESS;
}

rbusError_t setHandler(rbusHandle_t handle, rbusProperty_t property, rbusSetHandlerOptions_t* opts)
{
    char const* name;
    char* sVal;
    rbusValue_t value;
    char hashName[100];

    (void)handle;
    (void)opts;

    name = rbusProperty_GetName(property);
    value = rbusProperty_GetValue(property);

    if(strstr(name, ".Run"))
    {
        int id = parseID(name);
        if(!id)
        {
            TEST_BOOL(0);
            printf("parseID failed %s\n", name);
            return RBUS_ERROR_INVALID_INPUT;
        }
        else if(rbusValue_GetType(value) == RBUS_BOOLEAN)
        {
            gRunFlags[id-1] = rbusValue_GetBoolean(value);
            TEST_BOOL(1);
            return RBUS_ERROR_SUCCESS;
        }
        else
        {
            TEST_BOOL(0);
            printf("value type not boolean %s\n", name);
            return RBUS_ERROR_INVALID_INPUT;
        }
    }
    else
    {
        if(parseName(name, hashName))
        {
            TEST_BOOL(0);
            printf("parseName failed %s\n", name);
            return RBUS_ERROR_INVALID_INPUT;
        }
        else if(rbusValue_GetType(value) == RBUS_STRING)
        {
            TEST_BOOL(1);
            sVal=rbusValue_ToString(value, 0,0);
            rtHashMap_Set(paramValues, hashName, sVal);
            return RBUS_ERROR_SUCCESS;
        }
        else
        {
            TEST_BOOL(0);
            printf("value type not string %s\n", name);
            return RBUS_ERROR_INVALID_INPUT;
        }
    }
}

rbusError_t eventSubHandler(rbusHandle_t handle, rbusEventSubAction_t action, char const* eventName, rbusFilter_t filter, int32_t interval, bool* autoPublish)
{
    (void)handle;
    (void)filter;
    (void)interval;
    (void)autoPublish;

    int id = parseID(eventName);
    if(!id)
        return RBUS_ERROR_INVALID_INPUT;
    if(action == RBUS_EVENT_ACTION_SUBSCRIBE)
        gSubscribeFlags[id - 1]++;
    else
        gSubscribeFlags[id - 1]--;
    TEST_BOOL(1);
    return RBUS_ERROR_SUCCESS;
}

void* run_provider(void* p)
{
    int rc;
    int id;
    (void)p;
    rbusHandle_t handle;
    char componentName[64];
    char elemName[3][64];
    char hashName[64];
    char defaultValue[64];
    rbusDataElement_t dataElements[3] = {
        {elemName[0], RBUS_ELEMENT_TYPE_PROPERTY, {getHandler,setHandler,NULL,NULL,NULL,NULL}},
        {elemName[1], RBUS_ELEMENT_TYPE_PROPERTY, {getHandler,setHandler,NULL,NULL,NULL,NULL}},
        {elemName[2], RBUS_ELEMENT_TYPE_EVENT, {NULL,NULL,NULL,NULL,eventSubHandler,NULL}}
    };
    int usecWait = 0;
    static int sLastIndex = 0;

    id = ++sLastIndex;

    if(gMaxProviderWait > 0)
    {
        usecWait = rand() % gMaxProviderWait;
        usleep(usecWait);
    }

    sprintf(componentName, "MultiProvider%d", id);
    sprintf(dataElements[0].name, "Device.MultiProvider%d.Run", id);
    sprintf(dataElements[1].name, "Device.MultiProvider%d.Param", id);
    sprintf(dataElements[2].name, "Device.MultiProvider%d.Event!", id);

    sprintf(hashName, "Device.MultiProvider%d", id);
    sprintf(defaultValue, "Default Value %d", id);
    rtHashMap_Set(paramValues, hashName, strdup(defaultValue));

    TEST_EXEC_RC(rbus_open(&handle, componentName), rc)
    if(rc)
    {printf("What\n\n");    
        return NULL;
    }

#if REG_ELEMENTS
    TEST_EXEC_RC(rbus_regDataElements(handle, 3, dataElements), rc)
    if(rc)
    {
        rbus_close(handle);
        return NULL;
    }

    printf("provider %s: waiting\n", componentName);
    time_t start = time(NULL);
    while(!gRunFlags[id-1] && time(NULL) - start < gIterationDuration)
    {
        usleep(10);
    }
    printf("provider %s: running\n", componentName);

#if PUB_EVENTS
    int eventCount = 0;
    while(gRunFlags[id-1] && time(NULL) - start < gIterationDuration)
    {
        char buffer[200];
        eventCount++;
        sleep(1);
        if(gSubscribeFlags[id-1] > 0 && eventCount <= 3)
        {
            sprintf(buffer, "MultiProvider%d %d", id, eventCount);
            rbusObject_t data;
            rbusObject_Init(&data, NULL);
            rbusObject_SetPropertyString(data, "value", buffer);
            rbusEvent_t event = {0};
            event.name = dataElements[2].name;
            event.data = data;
            event.type = RBUS_EVENT_GENERAL;
            TEST_EXEC(rbusEvent_Publish(handle, &event));
            rbusObject_Release(data);
        }
    }
#else
    sleep(gIterationDuration);
#endif
#else
    sleep(gIterationDuration);
#endif

    if(handle)
    {
#ifdef REG_ELEMENTS        
        TEST_EXEC(rbus_unregDataElements(handle, 3, dataElements))
#endif
        TEST_EXEC(rbus_close(handle))
    }

    return 0;
}

int main(int argc, char *argv[])
{
    (void)(argc);
    (void)(argv);

    pthread_t* threads = NULL;
    int i,j;
    srand(time(NULL));

    while (1)
    {
        int option_index = 0;
        int c;

        static struct option long_options[] = 
        {
            {"providers",       required_argument,  0, 'p' },
            {"iterations",      required_argument,  0, 'i' },
            {"duration",        required_argument,  0, 'd' },
            {"wait",            required_argument,  0, 'w' },
            {"log",             required_argument,  0, 'l' },
            {"help",            no_argument,        0, 'h' },
            {0, 0, 0, 0}
        };

        c = getopt_long(argc, argv, "p:i:d:w:l:h", long_options, &option_index);
        if (c == -1)
            break;

        switch (c)
        {
        case 'p':
            gNumProviders = atoi(optarg);
            break;
        case 'i':
            gNumIterations = atoi(optarg);
            break;
        case 'd':
            gIterationDuration = atoi(optarg);
            break;
        case 'w':
            gMaxProviderWait = atoi(optarg);
            break;
        case 'l':
            rtLog_SetLevel(rtLogLevelFromString(optarg));
            break;
        case 'h':
        default:
            printf("rbusTestMultiProvider help:\n");
            printf("-p --providers: provider count per iteration\n");
            printf("-i --iterations: number of iterations\n");
            printf("-d --duration: duration of each iteration\n");
            printf("-w --wait: wait time each thread will take before running\n");
            printf("-l --log: log level (debug, info, warn, error, fatal)\n");
            exit(0);
            break;
        }
    }

    printf("provider=%d, iterations=%d, iteration duration=%d seconds, max provider wait=%d usecs\n", 
        gNumProviders, gNumIterations, gIterationDuration, gMaxProviderWait);


    gRunFlags = calloc(1, gNumProviders * sizeof(int));
    gSubscribeFlags = calloc(1, gNumProviders * sizeof(int));
    

    rtHashMap_CreateEx(&paramValues, 0, NULL, NULL, NULL, NULL, NULL, rtHashMap_Destroy_Func_Free);

    for(j = 0; j < gNumIterations; ++j)
    {
        printf("starting iteration %d of %d with %d provider threads\n", j+1, gNumIterations, gNumProviders);

        threads = malloc(gNumProviders * sizeof(pthread_t));

        for(i = 0; i < gNumProviders; ++i)
            pthread_create(&threads[i], NULL, run_provider, NULL);

        for(i = 0; i < gIterationDuration; ++i)
        {
            printf("iteration %d of %d with %d of %d seconds remaining for this iteration\n", j+1, gNumIterations, i+1, gIterationDuration);
            sleep(1);
        }

        for(i = 0; i < gNumProviders; ++i)
            pthread_join(threads[i], NULL);

        free(threads);
    }

    free(gRunFlags);
    free(gSubscribeFlags);
    rtHashMap_Destroy(paramValues);

    PRINT_TEST_RESULTS_EXPECTED("rbusTestMultiProvider", gNumProviders * 17);

    return 0;
}
