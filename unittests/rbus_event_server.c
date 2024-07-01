/*
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
*/
/*****************************************
Test server for unit test client testing
******************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include "rbuscore.h"

#include "rbus_test_util.h"

static char buffer[100];

static char data1[100] = "alpha init init init";



int main(int argc, char *argv[])
{
    (void) argc;
    rbusCoreError_t err = RBUSCORE_SUCCESS;
    printf("syntax: sample_server <server object name>\n");

    reset_stored_data();
    if((err = rbuscore_openBrokerConnection(argv[1])) == RBUSCORE_SUCCESS)
    {
        printf("Successfully connected to bus.\n");
    }

    /*Creating Object 1*/

    snprintf(buffer, (sizeof(buffer) - 1), "%s.obj1", argv[1]);
    printf("Registering object %s\n", buffer);

    if((err = rbuscore_registerObj(buffer, callback, NULL)) == RBUSCORE_SUCCESS)
    {
        printf("Successfully registered object %s \n", buffer);
    }

    rbus_method_table_entry_t table[2] = {{METHOD_SETPARAMETERVALUES, (void *)data1, handle_set2}, {METHOD_GETPARAMETERVALUES, (void *)data1, handle_get2}};

    /* registered the Methods */
    rbuscore_registerMethodTable(buffer, table, 2); 
   
    /* addelement to the object1 */
    rbuscore_addElement(buffer, "alpha_alias");

    /*registered the Events with name events */
    char data1[] = "data1";
    rbuscore_registerEvent(buffer,"event_1",sub1_callback, data1);

    rbusMessage msg1;
    rbusMessage_Init(&msg1);

    rbusMessage_SetString(msg1, "bar");

    rbuscore_publishEvent(buffer, "event_1", msg1);

    rbusMessage_Release(msg1);

  //  sleep(4);

    pause();

    if((err = rbuscore_closeBrokerConnection()) == RBUSCORE_SUCCESS)
    {
        printf("Successfully disconnected from bus.\n");
    }
    return 0;
}
