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

/*
    The following will test 2 features:
    1) a provider can restores its subscription from cache after it restarts (e.g from a crash)
       and that consumers don't have to do anything to restore their subscriptions
    2) the internal retry logic of rbusEvent_Subscribe which allows 
       a consumer to attempt to subscribe to an event before a provider starts
       and, using retries, successfully subscribe once the provider starts

    ###############################################################
    ###############################################################
    # 1. Retry Test
    ###############################################################

    # cleanup
    rm -f /tmp/rbus_subs_*

    # console 1/2
    # enable retries (disabled by default) by setting timeout
    export RBUS_SUBSCRIBE_RETRY_TIMEOUT=120

    # start consumers before starting the provider
    # they will attempt to subscribe and retry until the provider comes up
    ./rbusRecoveryConsumer -n consumer1 -t 120 -e "Device.TestProvider.Event1!"
    ./rbusRecoveryConsumer -n consumer2 -t 120 -e "Device.TestProvider.Event2!"

    # consoler 3
    # wait a few seconds and then start the provider
    ./rbusTestProvider -t 60

    # verify consumer1 starts receiving Event1! and consumer2 starts receiving Event2! 

    ###############################################################
    # Test crash recover
    ###############################################################
    rm -f /tmp/rbus_subs_*
    unset RBUS_SUBSCRIBE_RETRY_TIMEOUT (run in all consoles)


    ###############################################################
    # 2. Test crash recover with single subscriber
    # Start provider
    # Start consumer
    # Kill provider
    # Restart provider
    # Verify consumer still get events
    ###############################################################

    #console1:  start provider
    ./rbusTestProvider -t 60 &

    #console2:  start consumer and verify events are coming
    ./rbusRecoveryConsumer -n consumer1 -t 120 -e "Device.TestProvider.Event1!"

    #console 3: verify cache log
    ls -l /tmp/rbus_subs_*

    #console1:  kill provider
    killall -9 rbusTestProvider

    #console2: verify events stopped

    #console1: restart provider (before rbusRecoveryConsumer ends)
    ./rbusTestProvider -t 60

    #console2: verify events are coming

    ###############################################################
    # 3. Test crash recover w/ 1 subscriber no longer running
    # Start provider
    # Start consumer
    # Kill provider
    # Kill consumer
    # Restart provider
    # Verify provider not sending events
    ###############################################################

    #console1:  start provider
    ./rbusTestProvider -t 60 &

    #console2:  start consumer and verify events are coming
    ./rbusRecoveryConsumer -n consumer1 -t 120 -e "Device.TestProvider.Event1!"

    #console 3: verify cache log
    ls -l /tmp/rbus_subs_*

    #console1:  kill provider
    killall -9 rbusTestProvider

    #console2: kill consumer
    killall -9 rbusRecoveryConsumer

    #console1: restart provider
    ./rbusTestProvider -t 60 -l debug

    #console1: verify in rbusSubscription logs that listener was not re-added

    #console 3: verify cache log removed
    ls -l /tmp/rbus_subs_*

    ###############################################################
    # 4. Test crash recovery 2 subscribers
    # Start provider
    # Start consumer1
    # Start consumer2
    # Kill provider
    # Restart provider
    # Verify consumer1 and consumer2 still get events
    ###############################################################
    #console1:  start provider
    ./rbusTestProvider -t 60 &
    #console2:  start consumer and verify events are coming
    ./rbusRecoveryConsumer -n consumer1 -t 120 -e "Device.TestProvider.Event1!"
    #console3:  start consumer and verify events are coming
    ./rbusRecoveryConsumer -n consumer2 -t 120 -e "Device.TestProvider.Event2!"
    #console1:  kill provider
    killall -9 rbusTestProvider
    #console2/console3: verify events stopped
    #console1: restart provider (before rbusRecoveryConsumer ends)
    ./rbusTestProvider -t 60
    #console2/console3: verify events are coming

    ###############################################################
    # Test 5. crash recover 2 subscribers with same consumer name
    ###############################################################
    #console1:  start provider
    ./rbusTestProvider -t 60 &
    #console2:  start consumer and verify events are coming
    ./rbusRecoveryConsumer -n consumer1 -t 120 -e "Device.TestProvider.Event1!"
    #console3:  start consumer and verify events are coming
    ./rbusRecoveryConsumer -n consumer1 -t 120 -e "Device.TestProvider.Event2!"
    #console1:  kill provider
    killall -9 rbusTestProvider
    #console2/console3: verify events stopped
    #console1: restart provider (before rbusRecoveryConsumer ends)
    ./rbusTestProvider -t 60
    #console2/console3: verify events are coming

    ##########################################################################
    # Test 5. crash recover 2 subscribers with same consumer name, same event
    ##########################################################################
    #console1:  start provider
    ./rbusTestProvider -t 60 &
    #console2:  start consumer and verify events are coming
    ./rbusRecoveryConsumer -n consumer1 -t 120 -e "Device.TestProvider.Event1!"
    #console3:  start consumer and verify events are coming
    ./rbusRecoveryConsumer -n consumer1 -t 120 -e "Device.TestProvider.Event1!"
    #console1:  kill provider
    killall -9 rbusTestProvider
    #console2/console3: verify events stopped
    #console1: restart provider (before rbusRecoveryConsumer ends)
    ./rbusTestProvider -t 60
    #console2/console3: verify events are coming


    ###############################################################
    # Test crash recover 2 subscriber2 w/1 1 sub no longer running
    # Start provider
    # Start consumer1
    # Start consumer2
    # Kill provider
    # Kill consumer1
    # Restart provider
    # Verify consumer2 still get events
    # Start consumer1 and verify it rejoins
    ###############################################################

    #console1:  start provider
    ./rbusTestProvider -t 60 &

    #console2:  start consumer and verify events are coming
    ./rbusRecoveryConsumer -n consumer1 -t 120 -e "Device.TestProvider.Event1!"

    #console3:  start consumer and verify events are coming
    ./rbusRecoveryConsumer -n consumer2 -t 120 -e "Device.TestProvider.Event2!"

    #console1:  kill provider
    killall -9 rbusTestProvider

    #console2: kill consumer
    kill %

    #console3: verify events stopped

    #console1: restart provider (before rbusRecoveryConsumer ends)
    ./rbusTestProvider -t 60

    #console3: verify events are coming
    #console1: verify events not firing to consumer1

    #console2:  start consumer and verify events are coming
    ./rbusRecoveryConsumer -n consumer1 -t 120 -e "Device.TestProvider.Event1!"

    ###############################################################
    # Repeat above with value change events
    # -e "Device.TestProvider.VCParamInt0"
    # -e "Device.TestProvider.VCParamInt1"
    # -e "Device.TestProvider.VCParamStr0"
    # -e "Device.TestProvider.VCParamStr1"
    ###############################################################

    ###############################################################
    # More complex tests
    ###############################################################
    ./rbusRecoveryConsumer -n consumer1 -t 120 -e "Device.TestProvider.Event1!" -e "Device.TestProvider.VCParamInt0!"
    ./rbusRecoveryConsumer -n consumer2 -t 120 -e "Device.TestProvider.Event2!" -e "Device.TestProvider.VCParamInt1!"
    ./rbusRecoveryConsumer -n consumer3 -t 120 -e "Device.TestProvider.Event1!" -e "Device.TestProvider.Event2!" -e "Device.TestProvider.VCParamInt1!"
    ./rbusRecoveryConsumer -n consumer4 -t 120 -e "Device.TestProvider.Event2!" -e "Device.TestProvider.VCParamInt1!"
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
#include <rtList.h>
#include <rtLog.h>
#include "../common/runningParamHelper.h"
#include "../common/test_macros.h"

char componentName[RBUS_MAX_NAME_LENGTH] = {0};

static void eventHandler(
    rbusHandle_t handle,
    rbusEvent_t const* event,
    rbusEventSubscription_t* subscription)
{
    (void)(handle);
    PRINT_TEST_EVENT("test_RecoveryConsumer", event, subscription);
}

int main(int argc, char *argv[])
{
    (void)(argc);
    (void)(argv);
    (void)(gCountPass);
    (void)(gCountFail);

    rbusHandle_t handle;
    int rc = RBUS_ERROR_SUCCESS;
    int loopFor = 60;
    int timeout = -1;
    rtList eventList;
    rtListItem eventItem;

    rtList_Create(&eventList);

    strncpy(componentName, "rbusRecoveryConsumer", RBUS_MAX_NAME_LENGTH);

    while (1)
    {
        int option_index = 0;
        int c;

        static struct option long_options[] = 
        {
            {"name",        required_argument,  0, 'n' },
            {"event",       required_argument,  0, 'e' },
            {"runtime",     required_argument,  0, 't' },
            {"timeout",     required_argument,  0, 'T' },
            {"log-level",   required_argument,  0, 'l' },
            {0, 0, 0, 0}
        };

        c = getopt_long(argc, argv, "n:e:t:l:T:", long_options, &option_index);
        if (c == -1)
            break;

        switch (c)
        {
        case 'n':
            snprintf(componentName, RBUS_MAX_NAME_LENGTH, "%s", optarg);
            printf("componentName: %s\n", componentName);
            break;
        case 'e':
            rtList_PushBack(eventList, optarg, NULL);
            printf("will subscribe to event %s\n", optarg);
            break;
        case 't':
            loopFor = atoi(optarg);
            printf("runtime %d seconds\n", loopFor);
            break;
        case 'T':
            timeout = atoi(optarg);
            printf("subscribe timeout %d seconds\n", timeout);
            break;
        case 'l':
            rtLog_SetLevel(rtLogLevelFromString(optarg));
            break;
        default:
            fprintf(stderr, "invalid command line arg %c", (char)c);
            exit(0);
            break;
        }
    }

    rc = rbus_open(&handle, componentName);
    if(rc != RBUS_ERROR_SUCCESS)
    {
        printf("rbus_open failed for %s rc=%d\n", componentName, rc);
        exit(-1);
    }

    for(rtList_GetFront(eventList, &eventItem); 
        eventItem != NULL; 
        rtListItem_GetNext(eventItem, &eventItem))
    {
        char* eventName;
        rtListItem_GetData(eventItem, (void**)&eventName);
        printf("subscribing to event %s\n", eventName);
        rc = rbusEvent_Subscribe(handle, eventName, eventHandler, componentName, timeout);
        if(rc != RBUS_ERROR_SUCCESS)
        {
            printf("rbusEvent_Subscribe failed for %s rc=%d\n", componentName, rc);
            goto exit1;
        }
    }


    while (loopFor--)
    {
        printf("exiting in %d seconds\n", loopFor);
        sleep(1);
#if 1
        rbusValue_t value;
        rc = rbus_get(handle, "Device.TestProvider.VCParamStr0", &value);
        if(rc == RBUS_ERROR_SUCCESS)
        {
            char* val;
            printf("_test_:rbus_get success for %s param:'%s' value:'%s'\n", 
                componentName, "Device.TestProvider.VCParamStr0", val=rbusValue_ToString(value,0,0));
            free(val);
            rbusValue_Release(value);
        }
        else
        {
            printf("_test_:rbus_get failed for %s param:'%s' rc:%d\n", 
                componentName, "Device.TestProvider.VCParamStr0", rc);
        }
        
#endif
    }

exit1:

    rc = rbus_close(handle);
    if(rc == RBUS_ERROR_SUCCESS)
    {
        printf("_test_:rbus_close result:SUCCESS component:%s\n", componentName);
    }
    else
    {
        printf("_test_:rbus_close result:FAIL component:%s rc:%d\n", componentName, rc);
    }

    if(eventList)
        rtList_Destroy(eventList,NULL);

    return rc;
}
