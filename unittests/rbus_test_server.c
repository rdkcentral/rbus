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
static char buffer2[100];

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

    rbus_method_table_entry_t table[4] = {{METHOD_SETPARAMETERVALUES, NULL, handle_set1}, {METHOD_GETPARAMETERVALUES, NULL, handle_get1},
                                          {METHOD_SETPARAMETERATTRIBUTES, NULL, handle_setAttributes1}, {METHOD_GETPARAMETERATTRIBUTES, NULL, handle_getAttributes1}};
    rbuscore_registerMethodTable(buffer, table, 4); 


    /*Creating Object 2*/

    snprintf(buffer2, (sizeof(buffer2) - 1), "%s.obj2", argv[1]);
    printf("Registering object %s\n", buffer2);

    if((err = rbuscore_registerObj(buffer2, callback, NULL)) == RBUSCORE_SUCCESS)
    {
        printf("Successfully registered object %s \n",buffer2);
    }

    rbus_method_table_entry_t table2[4] = {{METHOD_SETPARAMETERVALUES, NULL, handle_set2}, {METHOD_GETPARAMETERVALUES, NULL, handle_get2},
                                          {METHOD_SETPARAMETERATTRIBUTES, NULL, handle_setAttributes2}, {METHOD_GETPARAMETERATTRIBUTES, NULL, handle_getAttributes2}};
    rbuscore_registerMethodTable(buffer2, table2, 4); 

    fflush(stdout);

    pause();

    if((err = rbuscore_closeBrokerConnection()) == RBUSCORE_SUCCESS)
    {
        printf("Successfully disconnected from bus.\n");
    }
    return 0;
}
