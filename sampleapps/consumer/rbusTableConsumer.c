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
#include <getopt.h>
#include <rbus.h>

static void eventReceiveHandler(
    rbusHandle_t handle,
    rbusEvent_t const* event,
    rbusEventSubscription_t* subscription)
{
    (void)handle;

    if(event->type == RBUS_EVENT_OBJECT_CREATED)
    {
        rbusValue_t rowName = rbusObject_GetValue(event->data, "rowName");
        rbusValue_t instNum = rbusObject_GetValue(event->data, "instNum");
        rbusValue_t alias = rbusObject_GetValue(event->data, "alias");

        printf("Consumer received object created event\n");
        printf("  rowName=%s\n", rbusValue_GetString(rowName, NULL));
        printf("  instNum=%u\n", rbusValue_GetUInt32(instNum));
        printf("  alias=%s\n", rbusValue_GetString(alias, NULL));
    }
    else if(event->type == RBUS_EVENT_OBJECT_DELETED)
    {
        printf("Consumer received object deleted event\n");
        printf("  rowName=%s\n", rbusValue_GetString(rbusObject_GetValue(event->data, "rowName"), NULL));
    }
    else if(event->type == RBUS_EVENT_VALUE_CHANGED)
    {
        rbusValue_t newValue = rbusObject_GetValue(event->data, "value");
        rbusValue_t oldValue = rbusObject_GetValue(event->data, "oldValue");

        printf("Consumer received ValueChange event for param %s\n", event->name);
        printf("  New Value: %s\n", rbusValue_GetString(newValue, NULL));
        printf("  Old Value: %s\n", rbusValue_GetString(oldValue, NULL));
    }
    else if(event->type == RBUS_EVENT_INITIAL_VALUE)
    {
        int i = 0, row;
        rbusValue_t rowcount;
        rbusValue_t newValue;
        char rowinst[128];
        rowcount = rbusObject_GetValue(event->data, "numberOfEntries");
        row = rbusValue_GetInt32(rowcount);
        printf("Row details when subscribe is called\n");
        for(; i < row; i++)
        {
            snprintf(rowinst, 128, "path%d", i+1);
            newValue = rbusObject_GetValue(event->data, rowinst);
            printf("%s\n", rbusValue_GetString(newValue, NULL));
        }
    }

    printf("My user data: %s\n", (char*)subscription->userData);
}

int main(int argc, char *argv[])
{
    (void)(argc);
    (void)(argv);
    int additionalWaitTime = 0;

    if(argc == 2)
    {
        additionalWaitTime = atoi(argv[1]);
    }

    int rc = RBUS_ERROR_SUCCESS;
    rbusHandle_t handle;
    uint32_t instNum, instColors, instShapes, instR, instG, instB, instOctagon;
    char* value;
    char name[RBUS_MAX_NAME_LENGTH];

    printf("consumer: start\n");

    rc = rbus_open(&handle, "EventConsumer");
    if(rc != RBUS_ERROR_SUCCESS)
    {
        printf("consumer: rbus_open failed: %d\n", rc);
        goto exit1;
    }

    rbusEventSubscription_t subscriptions[1] = {
        {"Device.Tables1.T1.", NULL, 0, 0, eventReceiveHandler, "Initial_value", NULL, NULL, true}
    };

    /*add rows to the T1 table with aliases 'row1' and 'row2'*/
    rbusTable_addRow(handle, "Device.Tables1.T1.", "row1", &instColors);
    rbusTable_addRow(handle, "Device.Tables1.T1.", "row2", &instShapes);

    /*subscribe to table T1 events*/
    rc = rbusEvent_SubscribeEx(handle, subscriptions, 1, 0);
    if(rc != RBUS_ERROR_SUCCESS)
    {
        printf("consumer: rbusEvent_Subscribe failed: %d\n", rc);
        goto exit2;
    }

    /*add rows to the T1 table with aliases 'colors' and 'shapes'*/
    rbusTable_addRow(handle, "Device.Tables1.T1.", "colors", &instColors);
    rbusTable_addRow(handle, "Device.Tables1.T1.", "shapes", &instShapes);

    /*subscribe to T2 table inside the new T1 rows
      in order to subscribe to table events where the table name includes a parent table,
      only instance numbers, not aliases, can be used to identify the row,
      thus we use instColors instead of alias [colors]*/
    snprintf(name, RBUS_MAX_NAME_LENGTH, "Device.Tables1.T1.%d.T2.", instColors);
    rbusEvent_Subscribe(handle, name, eventReceiveHandler, NULL, 0);

    snprintf(name, RBUS_MAX_NAME_LENGTH, "Device.Tables1.T1.%d.T2.", instShapes);
    rbusEvent_Subscribe(handle, name, eventReceiveHandler, NULL, 0);

    /*add rows to the T2 table inside the new T1 'colors' row
      using the alias 'colors' to find the row */
    rbusTable_addRow(handle, "Device.Tables1.T1.[colors].T2.", "red", &instR);
    rbusTable_addRow(handle, "Device.Tables1.T1.[colors].T2.", "green", &instG);
    rbusTable_addRow(handle, "Device.Tables1.T1.[colors].T2.", "blue", &instB);
    rbusTable_addRow(handle, "Device.Tables1.T1.[colors].T2.", "octagon", &instOctagon);/*we will fix this below*/

    /*add rows to the T2 table inside the new T1 'shapes' row
      but this time use the instance number to find the row*/
    snprintf(name, RBUS_MAX_NAME_LENGTH, "Device.Tables1.T1.%d.T2.", instShapes);
    rbusTable_addRow(handle, name, "circle", &instNum);
    rbusTable_addRow(handle, name, "square", &instNum);
    rbusTable_addRow(handle, name, "orange", &instNum);/*we will fix this below*/
    rbusTable_addRow(handle, name, "triangle", &instNum);

    sleep(1);/*not required but to ensure we get eventReceiveHandler callbacks from bus before we exit*/

    /*receiver a ValueChange event whenever any color data changes*/
    snprintf(name, RBUS_MAX_NAME_LENGTH, "Device.Tables1.T1.%d.T2.*.Data", instColors);
    rbusEvent_Subscribe(handle, name, eventReceiveHandler, "my color user data", 0);

    /*set property 'Data' on the T1 rows*/
    rbus_setStr(handle, "Device.Tables1.T1.[colors].Data", "Has some colors");
    rbus_setStr(handle, "Device.Tables1.T1.[shapes].Data", "Has some shapes");

    /*set color red with T1 alias(colors) and T2 alias(red)*/
    rbus_setStr(handle, "Device.Tables1.T1.[colors].T2.[red].Data", "The color red");

    /*set color green with T1 instance number and T2 alias(green)*/
    snprintf(name, RBUS_MAX_NAME_LENGTH, "Device.Tables1.T1.%d.T2.[green].Data", instColors);
    rbus_setStr(handle, name, "The color green");

    /*set blue with T1 instance number and T2 instance number*/
    snprintf(name, RBUS_MAX_NAME_LENGTH, "Device.Tables1.T1.%d.T2.%d.Data", instColors, instB);
    rbus_setStr(handle, name, "The color blue");

    /*set the shapes by alias*/
    rbus_setStr(handle, "Device.Tables1.T1.[shapes].T2.[circle].Data", "The shape circle");
    rbus_setStr(handle, "Device.Tables1.T1.[shapes].T2.[square].Data", "The shape square");
    rbus_setStr(handle, "Device.Tables1.T1.[shapes].T2.[triangle].Data", "The shape triangle");

    sleep(3);/*not required but to demonstrate we can get ValueChange callbacks from bus before we exit*/

    /*fix octagon and orange being in the wrong tables*/
    rbusTable_removeRow(handle, "Device.Tables1.T1.[colors].T2.[octagon].");
    rbusTable_removeRow(handle, "Device.Tables1.T1.[shapes].T2.[orange].");

    rbusTable_addRow(handle, "Device.Tables1.T1.[colors].T2.", "orange", &instNum);
    rbusTable_addRow(handle, "Device.Tables1.T1.[shapes].T2.", "octagon", &instOctagon);

    rbus_setStr(handle, "Device.Tables1.T1.[colors].T2.[orange].Data", "The color orange");
    rbus_setStr(handle, "Device.Tables1.T1.[shapes].T2.[octagon].Data", "The shape octagon");

    /*get orange's data by alias 'orange'*/
    if((rc = rbus_getStr(handle, "Device.Tables1.T1.[colors].T2.[orange].Data", &value)) == RBUS_ERROR_SUCCESS)
    {
        printf("orange=%s\n", value);
        free(value);
    }
    else
    {
        printf("rbus_getStr error:%d\n", rc);
    }

    /*get octagon's data by instance number instOctagon*/
    snprintf(name, RBUS_MAX_NAME_LENGTH, "Device.Tables1.T1.[shapes].T2.%d.Data", instOctagon);
    if((rc = rbus_getStr(handle, name, &value)) == RBUS_ERROR_SUCCESS)
    {
        printf("octagon=%s\n", value);
        free(value);
    }
    else
    {
        printf("rbus_getStr error:%d\n", rc);
    }

    if(additionalWaitTime)
    {
        printf("waiting for an additional %d seconds\n", additionalWaitTime);
        sleep(additionalWaitTime);
    }

    printf("########################## remove colors ###############################\n");
    rbusTable_removeRow(handle, "Device.Tables1.T1.[colors].");
    printf("########################## remove shapes ###############################\n");
    snprintf(name, RBUS_MAX_NAME_LENGTH, "Device.Tables1.T1.%d", instShapes);
    rbusTable_removeRow(handle,name);
    sleep(1);
    printf("########################## unsubscribing ###############################\n");

    rbusEvent_Unsubscribe(handle, "Device.Tables1.T1.");

exit2:
    rbus_close(handle);

exit1:
    printf("consumer: exit\n");
    return rc;
}
