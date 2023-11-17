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
#include <unistd.h>
#include "rbus_core.h"

#include "rtLog.h"


static void dumpMessage(rbusMessage message)
{
    char* buff = NULL;
    uint32_t buff_length = 0;

    rbusMessage_ToDebugString(message, &buff, &buff_length);
    printf("dumpMessage: %.*s\n", buff_length, buff);
    free(buff);
}

static void pullAndDumpObject(const char * object)
{
    rbusCoreError_t err = RBUSCORE_SUCCESS;
    rbusMessage response;
    if((err = rbuscore_pullObj(object, 1000, &response)) == RBUSCORE_SUCCESS)
    {
        printf("Received object %s\n", object);
        dumpMessage(response);
        rbusMessage_Release(response);
    }
    else
        printf("Could not pull object %s\n", object);
}


static void queryWildcardExpression(const char * expression)
{
    char** dests;
    int num_entries = 0;
    
    if(RBUSCORE_SUCCESS == rbuscore_discoverWildcardDestinations(expression, &num_entries, &dests))
    {
        printf("Query for expression %s was successful. See result below:\n", expression);
        for(int i = 0; i < num_entries; i++)
        {
            printf("Destination %d is %s\n", i, dests[i]);
            free(dests[i]);
        }
        free(dests);
    }
    else
        printf("Query for expression %s was not successful.\n", expression);
}

int main(int argc, char *argv[])
{
    (void) argc;
    (void) argv;
    rbusCoreError_t err = RBUSCORE_SUCCESS;
    rtLog_SetLevel(RT_LOG_INFO);

    if((err = rbuscore_openBrokerConnection("wildcard_client")) == RBUSCORE_SUCCESS)
    {
        printf("Successfully connected to bus.\n");
    }


    /*Pull the object from remote end.*/
    pullAndDumpObject("noobject");
    pullAndDumpObject("obj1");
    pullAndDumpObject("device.wifi.x.y");
    pullAndDumpObject("device.wifi.");
    pullAndDumpObject("device.wifi.a");
    pullAndDumpObject("device.wifi.aa");
    pullAndDumpObject("device.wifi.ab");
    pullAndDumpObject("device.wifi.abbb");
    pullAndDumpObject("device.");
    pullAndDumpObject("device.tr69.x.y");
    pullAndDumpObject("device.wifii.a");
    pullAndDumpObject("device.tr69.");
    
    queryWildcardExpression("obj1");
    queryWildcardExpression("device.wifi.x.");
    queryWildcardExpression("device.wifi.");
    queryWildcardExpression("device.");
    queryWildcardExpression("device.tr69.x.y");
    queryWildcardExpression("device.tr69.");
    queryWildcardExpression("noobject");

    if((err = rbuscore_closeBrokerConnection()) == RBUSCORE_SUCCESS)
    {
        printf("Successfully disconnected from bus.\n");
    }
    return 0;
}
