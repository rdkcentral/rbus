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
#include "../common/test_macros.h"

#define TEST_EVENTS 1
#define TEST_PARAMS 1

static int gNumConsumers = 5;/*each consumer will get its own thread*/
static int gNumIterations = 1;/*for each iteration, any previous consumers are destroyed and new consumer created*/
static int gIterationDuration = 60; /*seconds for each iteration*/
static int gMaxConsumerWait = 0; /*max time in usecs a consumer thread sleeps before starting*/
static int* gEventCounts = NULL;
int reopened = 0;

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

#if TEST_EVENTS
static void eventHandler(
    rbusHandle_t handle,
    rbusEvent_t const* event,
    rbusEventSubscription_t* subscription)
{
    char const* str = NULL;
    char expect[100];
    int id = parseID(event->name);
    (void)(handle);
    (void)(subscription);
    TEST_EXEC(rbusObject_GetPropertyString(event->data, "value", &str, NULL));
    sprintf(expect, "MultiProvider%d %d", id, ++gEventCounts[id-1]);
    printf("eventHandler expect=%s actual=%s\n", expect, str);
    TEST_EXEC(strcmp(str, expect));
}
#endif

void* run_consumer(void* p)
{
    rbusError_t rc;
    int id;
    (void)p;
    rbusHandle_t handle;
    char componentName[100];
    char elemName[3][100];
    int usecWait = 0;
    int timeLeft = gIterationDuration;
    int paramValNum = 0;
#if TEST_PARAMS
    int numGets = 0;
#endif    
    static int sLastIndex = 0;

    id = ++sLastIndex;

    if(gMaxConsumerWait > 0)
    {
        usecWait = rand() % gMaxConsumerWait;
        usleep(usecWait);
    }

    sprintf(componentName, "MultiConsumer%d", id);
    sprintf(elemName[0], "Device.MultiProvider%d.Run", id);
    sprintf(elemName[1], "Device.MultiProvider%d.Param", id);
    sprintf(elemName[2], "Device.MultiProvider%d.Event!", id);

    TEST_EXEC_RC(rbus_open(&handle, componentName), rc)
    if(rc)
    {
        return NULL;
    }

    TEST_EXEC_RC(rbus_setBoolean(handle, elemName[0], true), rc)
    if(rc)
    {
        rbus_close(handle);
        return NULL;
    }

#if TEST_EVENTS
    TEST_EXEC(rbusEvent_Subscribe(handle, elemName[2], eventHandler, componentName, 0));
#endif

    while(timeLeft >= 0)
    {
        sleep(1);
        timeLeft--;
#if TEST_PARAMS
        if(numGets++ < 3)
        {
            char setval[50];
            sprintf(setval, "value-%d-%d", id, paramValNum++);
            TEST_EXEC_RC(rbus_setStr(handle, elemName[1], setval), rc)
            if(!rc)
            {
                char* getval = NULL;
                TEST_EXEC_RC(rbus_getStr(handle, elemName[1], &getval), rc)
                if(!rc)
                {
                    TEST_EXEC(strcmp(getval, setval))
                    free(getval);
                }
            }
        }
#endif        
    }

#if TEST_EVENTS
    TEST_EXEC(rbusEvent_Unsubscribe(handle, elemName[2]));
#endif

    TEST_EXEC(rbus_setBoolean(handle, elemName[0], false));

    TEST_EXEC(rbus_close(handle));

    return NULL;
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
            gNumConsumers = atoi(optarg);
            break;
        case 'i':
            gNumIterations = atoi(optarg);
            break;
        case 'd':
            gIterationDuration = atoi(optarg);
            break;
        case 'w':
            gMaxConsumerWait = atoi(optarg);
            break;
        case 'l':
            rtLog_SetLevel(rtLogLevelFromString(optarg));
            break;
        case 'h':
        default:
            printf("rbusTestMultiConsumer help:\n");
            printf("-p --providers: number of providers running. 1 consumer per provider will be created per iteration\n");
            printf("-i --iterations: number of iterations\n");
            printf("-d --duration: duration of each iteration\n");
            printf("-w --wait: wait time each thread will take before running\n");
            printf("-l --log: log level (debug, info, warn, error, fatal)\n");
            exit(0);
            break;
        }
    }

    printf("consumers=%d, iterations=%d, iteration duration=%d seconds, max consumer wait=%d usecs\n", 
        gNumConsumers, gNumIterations, gIterationDuration, gMaxConsumerWait);

    gEventCounts = calloc(1, gNumConsumers * sizeof(int));

    for(j = 0; j < gNumIterations; ++j)
    {
        printf("starting iteration %d of %d with %d consumer threads\n", j+1, gNumIterations, gNumConsumers);

        threads = malloc(gNumConsumers * sizeof(pthread_t));

        for(i = 0; i < gNumConsumers; ++i)
            pthread_create(&threads[i], NULL, run_consumer, NULL);

        for(i = 0; i < gIterationDuration; ++i)
        {
            printf("iteration %d of %d with %d of %d seconds remaining for this iteration\n", j+1, gNumIterations, i+1, gIterationDuration);
            sleep(1);
        }

        for(i = 0; i < gNumConsumers; ++i)
            pthread_join(threads[i], NULL);

        free(threads);
    }
    
    free(gEventCounts);

    PRINT_TEST_RESULTS_EXPECTED("rbusTestMultiConsumer", 21 * gNumConsumers);

    return 0;
}
