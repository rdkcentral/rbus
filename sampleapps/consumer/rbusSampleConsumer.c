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
    "Device.SampleProvider.SampleData.UIntData"
    };

int main(int argc, char *argv[])
{
    int rc;
    int count;

    (void)argc;
    (void)argv;

    printf("constumer: start\n");

    rc = rbus_open(&handle, componentName);
    if(rc != RBUS_ERROR_SUCCESS)
    {
        printf("consumer: rbus_open failed: %d\n", rc);
        goto exit1;
    }

    for(count=0; count < TotalParams; count++)
    {
        rbusValue_t value;
        printf("calling rbus get for [%s]\n", paramNames[count]);

        rc = rbus_get(handle, paramNames[count], &value);

        if(rc != RBUS_ERROR_SUCCESS)
        {
            printf ("rbus_get failed for [%s] with error [%d]\n", paramNames[count], rc);
            continue;
        }

        switch(count)
        {
            case 0:
                printf("Manufacturer name = [%s]\n", rbusValue_GetString(value, NULL));
                break;
            case 1:
                printf("Model name = [%s]\n", rbusValue_GetString(value, NULL));
                break;
            case 2:
                printf("Software Version = [%f]\n", rbusValue_GetSingle(value));
                break;
            case 3:
                printf("The value is = [%d]\n", rbusValue_GetInt32(value));
                break;
            case 4:
                printf("The value is = [%d]\n", rbusValue_GetBoolean(value));
                break;
            case 5:
                printf("The value is = [%u]\n", rbusValue_GetUInt32(value));
                break;
        }

        rbusValue_Release(value);
    }

    int numOfOutVals = 0;
    rbusProperty_t outputVals = NULL;
    const char* input[] =  {"Device.SampleProvider."}; 

    rc = rbus_getExt(handle, 1, input, &numOfOutVals, &outputVals);
    if(rc != RBUS_ERROR_SUCCESS)
    {
        printf ("rbus_get failed for [%s] with error [%d]\n", "Device.SampleProvider.", rc);
    }
    else
    {
        rbusProperty_t next = outputVals;
        printf ("List of params\r\n");
        for (int i = 0; i < numOfOutVals; i++)
        {
            rbusValue_t val = rbusProperty_GetValue(next);
            char *pStrVal = rbusValue_ToString(val,NULL,0);

            printf ("\tProp  : %s\n", rbusProperty_GetName(next));
            printf ("\tValue : %s\n\n", pStrVal);

            if(pStrVal)
                free(pStrVal);

            next = rbusProperty_GetNext(next);
        }
        /* Free the memory */
        rbusProperty_Release(outputVals);
    }

    {
        int loop = 0;
        struct timeval tv1, tv2;
        char tmpBuff[25] = "";
        rbusValue_t value;
        printf ("Get 8 KB of Data for every 250ms over 100 times n check the latency\n");
        while (loop < 100)
        {
            loop++;

            gettimeofday(&tv1, NULL);
            rc = rbus_get(handle, "Device.SampleProvider.AllTypes.BytesData" ,&value);
            gettimeofday(&tv2, NULL);
            if(RBUS_ERROR_SUCCESS != rc)
            {
                printf ("Failed to get the Bytes Data\n");
                continue;
            }
            rbusValue_Release(value);

            strftime(tmpBuff, 25, "%T", localtime(&tv1.tv_sec));
            printf ("StartTime : %s.%06ld\t", tmpBuff, tv1.tv_usec);

            strftime(tmpBuff, 25, "%T", localtime(&tv2.tv_sec));
            printf ("EndTime: %s.%06ld\t", tmpBuff, tv2.tv_usec);

            /* As we expecting only millisecond latency, */
            if(tv1.tv_usec > tv2.tv_usec)
                tv2.tv_usec += 1000000;

            printf ("Latency is, %f ms\n", (tv2.tv_usec - tv1.tv_usec)/1000.f);
            usleep (250000);
        }
    }
    
    rbus_close(handle);
exit1:
    printf("consumer: exit\n");
    return 0;
}


