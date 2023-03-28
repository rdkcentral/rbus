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

#define     TotalParams   6

char            componentName[] = "rbusSampleConsumer";
rbusHandle_t    handle;
char const*     paramNames[TotalParams] = {
    "Device.DeviceInfo.SampleProvider.Manufacturer",
    "Device.DeviceInfo.SampleProvider.ModelName",
    "Device.DeviceInfo.SampleProvider.SoftwareVersion",
    "Device.SampleProvider.SampleData.IntData",
    "Device.SampleProvider.SampleData.BoolData",
    "Device.SampleProvider.SampleData.UIntData"/*,
    "Device.SampleProvider.NestedObject1.TestParam",
    "Device.SampleProvider.NestedObject1.AnotherTestParam",
    "Device.SampleProvider.NestedObject2.TestParam",
    "Device.SampleProvider.NestedObject2.AnotherTestParam"*/
};

rbusValueType_t getDataType_fromString(const char* pType)
{
    rbusValueType_t rc = RBUS_NONE;

    if (strncasecmp ("boolean", pType, 4) == 0)
        rc = RBUS_BOOLEAN;
    else if (strncasecmp("char", pType, 4) == 0)
        rc = RBUS_CHAR;
    else if (strncasecmp("byte", pType, 4) == 0)
        rc = RBUS_BYTE;
    else if (strncasecmp("int8", pType, 4) == 0)
        rc = RBUS_INT8;
    else if (strncasecmp("uint8", pType, 5) == 0)
        rc = RBUS_UINT8;
    else if (strncasecmp("int16", pType, 5) == 0)
        rc = RBUS_INT16;
    else if (strncasecmp("uint16", pType, 6) == 0)
        rc = RBUS_UINT16;
    else if (strncasecmp("int32", pType, 5) == 0)
        rc = RBUS_INT32;
    else if (strncasecmp("uint32", pType, 6) == 0)
        rc = RBUS_UINT32;
    else if (strncasecmp("int64", pType, 5) == 0)
        rc = RBUS_INT64;
    else if (strncasecmp("uint64", pType, 6) == 0)
        rc = RBUS_UINT64;
    else if (strncasecmp("single", pType, 5) == 0)
        rc = RBUS_SINGLE;
    else if (strncasecmp("double", pType, 6) == 0)
        rc = RBUS_DOUBLE;
    else if (strncasecmp("datetime", pType, 4) == 0)
        rc = RBUS_DATETIME;
    else if (strncasecmp("string", pType, 6) == 0)
        rc = RBUS_STRING;
    else if (strncasecmp("bytes", pType, 3) == 0)
        rc = RBUS_BYTES;
    /* Risk handling, if the user types just int, lets consider int32; same for unsigned too  */
    else if (strncasecmp("int", pType, 3) == 0)
        rc = RBUS_INT32;
    else if (strncasecmp("uint", pType, 4) == 0)
        rc = RBUS_UINT32;

    return rc;
}

int main(int argc, char *argv[])
{
    int rc;

    (void)argc;
    (void)argv;

    printf("constumer: start\n");

    if(argv[1] != NULL && argc == 2)
    {
        if((strcmp(argv[1], "--help")==0) || (strcmp(argv[1], "-h")==0))
        {
            printf("rbusSampleConsumer [OPTIONS] \n");
            printf("[OPTIONS]");
            printf("-h/--help: to display help\n");
            printf("\t\t**********************************************************************\n\
                *************************  rbus_set testing  *************************\n\
                **********************************************************************\n\
                Please pass the arguments for testing in below order\n\
                rbusSampleConsumer <data model> <data type> <data value>\n\
                For Ex: rbusSampleConsumer Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.Telemetry.MessageBusSource.Enable boolean true\n\
                caution: If invalid data model, data type or data value is passed this case is expected to be failed\n");
            return 0;
        }
    }

    rc = rbus_open(&handle, componentName);
    if(rc != RBUS_ERROR_SUCCESS)
    {
        printf("consumer: rbus_open failed: %d\n", rc);
        goto exit1;
    }

    rbus_setLogLevel(RBUS_LOG_DEBUG);
    {
        rbusValue_t value;
        printf("calling rbus get for [%s]\n", "Device.SampleProvider.SampleData.IntData");

        printf ("###############   GET 1 #####################################################\n");
        rc = rbus_get(handle, "Device.SampleProvider.SampleData.IntData", &value);
        rbusValue_fwrite(value, 0, stdout); printf("\n");
        rbusValue_Release(value);


        sleep(2);
        rbusHandle_t  lolHandle = NULL;
        rbusHandle_t  lolHandle2 = NULL;
        printf ("###############   OPEN DIRECT ################################################\n");
        rbus_openDirect(handle, &lolHandle, "Device.SampleProvider.SampleData.IntData");

        sleep(2);
        rbus_openDirect(handle, &lolHandle, "Device.SampleProvider.SampleData.IntData");

        sleep(2);
        rbus_openDirect(handle, &lolHandle2, "Device.SampleProvider.AllTypes.StringData");

        sleep(5);
        printf ("###############   GET 2 #####################################################\n");
        rc = rbus_get(handle, "Device.SampleProvider.SampleData.IntData", &value);
        rbusValue_fwrite(value, 0, stdout); printf("\n");
        rbusValue_Release(value);

        sleep(4);
        printf ("###############   GET 3 #####################################################\n");
        rc = rbus_get(handle, "Device.SampleProvider.AllTypes.StringData", &value);
        rbusValue_fwrite(value, 0, stdout); printf("\n");
        rbusValue_Release(value);

        rbus_closeDirect(lolHandle);
        sleep(1);
        rbus_closeDirect(lolHandle2);

        sleep(3);
        printf ("###############   GET 4 #####################################################\n");
        rc = rbus_get(handle, "Device.SampleProvider.SampleData.IntData", &value);
        rbusValue_fwrite(value, 0, stdout); printf("\n");
        rbusValue_Release(value);

        printf ("###############   GET 5 #####################################################\n");
        rc = rbus_get(handle, "Device.SampleProvider.AllTypes.BoolData", &value);
        rbusValue_fwrite(value, 0, stdout); printf("\n");
        rbusValue_Release(value);
        sleep(2);
        printf ("###############   OPEN DIRECT AGAIN  #####################################################\n");
        rbus_openDirect(handle, &lolHandle, "Device.SampleProvider.SampleData.IntData");
        printf ("###############   GET 6 #####################################################\n");
        rc = rbus_get(handle, "Device.SampleProvider.SampleData.IntData", &value);
        rbusValue_fwrite(value, 0, stdout); printf("\n");
        rbusValue_Release(value);
    (void) lolHandle;
    (void) lolHandle2;
    }

    //pause();

    sleep(10);
    rbus_close(handle);

exit1:
    printf("consumer: exit\n");
    return 0;
}


