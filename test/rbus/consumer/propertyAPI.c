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
#include <stdio.h>
#include <ctype.h>
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <endian.h>
#include <float.h>
#include <unistd.h>
#include <rbus.h>
#include "rbus_buffer.h"
#include <math.h>
#include "../common/test_macros.h"
#include "rbus_value.h"

int getDurationPropertyAPI()
{
    return 1;
}

void testPropertyName()
{
    rbusProperty_t prop;

    printf("%s\n",__FUNCTION__);

    rbusProperty_Init(&prop, NULL, NULL);
    TEST(rbusProperty_GetName(prop)==NULL);

    rbusProperty_SetName(prop, "Device.WiFi.AccessPoint.1.AssociatedDevice.1.SignalStrength");
    TEST(strcmp(rbusProperty_GetName(prop),"Device.WiFi.AccessPoint.1.AssociatedDevice.1.SignalStrength")==0);

    rbusProperty_SetName(prop, "Device.WiFi.Radio.1.Stats.X_COMCAST-COM_NoiseFloor");
    TEST(strcmp(rbusProperty_GetName(prop),"Device.WiFi.Radio.1.Stats.X_COMCAST-COM_NoiseFloor")==0);

    rbusProperty_Release(prop);

    rbusProperty_Init(&prop, "Device.ETSIM2M.SCL.{i}.SAFPolicySet.{i}.ANPPolicy.{i}.RequestCategory.{i}.Schedule.{i}.AbsTimeSpan.{i}.StartTime", NULL);
    TEST(strcmp(rbusProperty_GetName(prop),"Device.ETSIM2M.SCL.{i}.SAFPolicySet.{i}.ANPPolicy.{i}.RequestCategory.{i}.Schedule.{i}.AbsTimeSpan.{i}.StartTime")==0);

    rbusProperty_SetName(prop, "Device.WiFi.AccessPoint.2.AssociatedDevice.3.SignalStrength");
    TEST(strcmp(rbusProperty_GetName(prop),"Device.WiFi.AccessPoint.2.AssociatedDevice.3.SignalStrength")==0);

    rbusProperty_SetName(prop, NULL);
    TEST(rbusProperty_GetName(prop)==NULL);

    rbusProperty_Release(prop);
}

void testPropertyList()
{
    printf("%s\n",__FUNCTION__);

    /*first, a basic test where we have 3 properties*/
    {
        rbusProperty_t prop1, prop2, prop3, propNext;
        rbusValue_t val1, val2, val3;
        int i;


        rbusProperty_Init(&prop1, "prop1", NULL);
        rbusProperty_Init(&prop2, "prop2", NULL);
        rbusProperty_Init(&prop3, "prop3", NULL);

        rbusValue_Init(&val1);
        rbusValue_Init(&val2);
        rbusValue_Init(&val3);

        rbusValue_SetString(val1, "val1");
        rbusValue_SetString(val2, "val2");
        rbusValue_SetString(val3, "val3");

        rbusProperty_SetValue(prop1, val1);
        rbusProperty_SetValue(prop2, val2);
        rbusProperty_SetValue(prop3, val3);

        rbusValue_Release(val1);
        rbusValue_Release(val2);
        rbusValue_Release(val3);

        rbusProperty_SetNext(prop1, prop2);
        rbusProperty_Append(prop1, prop3);

        TEST(rbusProperty_GetNext(prop1) != NULL);
        TEST(rbusProperty_GetNext(rbusProperty_GetNext(prop1)) != NULL);
        TEST(strcmp(rbusValue_GetString(rbusProperty_GetValue(prop1), NULL), "val1")==0);
        TEST(strcmp(rbusValue_GetString(rbusProperty_GetValue(rbusProperty_GetNext(prop1)), NULL), "val2")==0);
        TEST(strcmp(rbusValue_GetString(rbusProperty_GetValue(rbusProperty_GetNext(rbusProperty_GetNext(prop1))), NULL), "val3")==0);

        i = 0;
        propNext = prop1;
        while(propNext)
        {
            char const* s;
            char buff[20];
            i++;
            sprintf(buff, "val%d", i);
            s = rbusValue_GetString(rbusProperty_GetValue(propNext), NULL);
            TEST(strcmp(s, buff)==0);
            propNext = rbusProperty_GetNext(propNext);
        }

        rbusProperty_Release(prop1);
        rbusProperty_Release(prop2);
        rbusProperty_Release(prop3);
    }

    /*create a long list*/
    {
        rbusProperty_t first = NULL;
        rbusProperty_t last = NULL;
        rbusProperty_t next = NULL;
        int i;

        /*create list of 100 Int32 properties*/
        for(i=0; i<100; ++i)
        {
            
            rbusValue_t val;
            char name[20];

            rbusValue_Init(&val);
            rbusValue_SetInt32(val, i);

            sprintf(name, "prop%d", i);
            rbusProperty_Init(&next, name, val);

            rbusValue_Release(val);

            if(!first)
            {
                first = last = next;
            }
            else
            {
                if((i%2)==0)/*test setting the last next*/
                {
                    rbusProperty_SetNext(last, next);
                }
                else/*or using PushBack helper*/
                {
                    rbusProperty_Append(first, next);
                }
                rbusProperty_Release(next);
                last = next;
            }
        }

        /*set every 10th property's value to a string*/
        next = first;
        while(next)
        {
            rbusValue_t val = rbusProperty_GetValue(next);
            int i = rbusValue_GetInt32(val);
            if((i%10)==0)
            {
                char str[20];
                sprintf(str, "val %d", i);
                rbusValue_SetString(val, str);
            }
            next = rbusProperty_GetNext(next);
        }

        i=0;
        next = first;
        while(next)
        {
            char sdbg[100];
            char final[100];
            rbusValue_t val = rbusProperty_GetValue(next);
            snprintf(final, 100, "%s: %s", rbusProperty_GetName(next), rbusValue_ToDebugString(val,sdbg,100));

            char expected[100];
            if((i%10)==0)
                snprintf(expected, 100, "prop%d: rbusValue type:RBUS_STRING value:val %d", i, i);
            else 
                snprintf(expected, 100, "prop%d: rbusValue type:RBUS_INT32 value:%d", i, i);

            TEST(!strcmp(final, expected));
            next = rbusProperty_GetNext(next);
            i++;
        }

        rbusProperty_Release(first);
    }
}

void testPropertyValue()
{
    printf("%s\n",__FUNCTION__);

    rbusProperty_t prop1, prop2;
    rbusValue_t val1, val2;
    rbusValue_t pval1, pval2;


    rbusValue_Init(&val1);
    rbusValue_SetInt32(val1, 1001);

    rbusValue_Init(&val2);
    rbusValue_SetInt32(val2, 1002);

    rbusProperty_Init(&prop1, "prop1", val1);/*test passing val into init*/
    rbusProperty_Init(&prop2, "prop2", NULL);

    rbusProperty_SetValue(prop2, val2);/*test passing val into set*/

    pval1 = rbusProperty_GetValue(prop1);
    pval2 = rbusProperty_GetValue(prop2);

    TEST(pval1 == val1);
    TEST(pval2 == val2);
    TEST(rbusValue_GetInt32(pval1) == 1001);
    TEST(rbusValue_GetInt32(pval2) == 1002);

    rbusProperty_SetValue(prop1, val2);
    rbusProperty_SetValue(prop2, val1);

    pval1 = rbusProperty_GetValue(prop1);
    pval2 = rbusProperty_GetValue(prop2);

    TEST(pval1 == val2);
    TEST(pval2 == val1);
    TEST(rbusValue_GetInt32(pval1) == 1002);
    TEST(rbusValue_GetInt32(pval2) == 1001);

    rbusProperty_SetValue(prop1, NULL);
    TEST(rbusProperty_GetValue(prop1) == NULL);

    rbusProperty_Release(prop1);
    rbusProperty_Release(prop2);

    TEST(rbusValue_GetInt32(val1) == 1001);
    TEST(rbusValue_GetInt32(val2) == 1002);

    rbusValue_Release(val1);
    rbusValue_Release(val2);
}

void testPropertyCompare()
{
    printf("%s\n",__FUNCTION__);

    rbusProperty_t prop1, prop2;
    rbusValue_t val1, val2;

    rbusValue_Init(&val1);
    rbusValue_SetInt32(val1, 1001);

    rbusValue_Init(&val2);
    rbusValue_SetInt32(val2, 1002);

    rbusProperty_Init(&prop1, "prop1", val1);
    rbusProperty_Init(&prop2, "prop2", val2);

    TEST(rbusProperty_Compare(prop1, prop2) != 0);

    rbusProperty_SetName(prop2, "prop1");

    TEST(rbusProperty_Compare(prop1, prop2) != 0);

    rbusProperty_SetValue(prop2, val1);

    TEST(rbusProperty_Compare(prop1, prop2) == 0);

    rbusProperty_Release(prop1);
    rbusProperty_Release(prop2);
    rbusValue_Release(val1);
    rbusValue_Release(val2);
}

void testValue_InitGetSetByType()
{
    rbusDateTime_t tv1 = {{0},{0}};
    time_t nowtime = 0;
    const char teststring[] = "Hello World";
    const char teststring2[] = "Goodby Cruel World";
    rbusProperty_t prop;
    rbusProperty_t prop2;
    rbusObject_t obj;
    rbusObject_t obj2;

    printf("%s\n",__FUNCTION__);

    rbusValue_MarshallTMtoRBUS(&tv1, localtime(&nowtime));
    rbusProperty_Init(&prop, "MyProp", NULL);
    rbusObject_Init(&obj, "MyObj");
    rbusProperty_Init(&prop2, "MyProp2", NULL);
    rbusObject_Init(&obj2, "MyObj2");

    rbusProperty_t vbtrue = rbusProperty_InitBoolean("vbtrue",true);
    rbusProperty_t vbfalse = rbusProperty_InitBoolean(NULL,false);
    rbusProperty_t vi16_n1234 = rbusProperty_InitInt16("vi16_n1234",-1234);
    rbusProperty_t vu16_4321 = rbusProperty_InitUInt16("vu16_4321",4321);
    rbusProperty_t vi32_689013 = rbusProperty_InitInt32("vi32_689013",689013);
    rbusProperty_t vu32_856712 = rbusProperty_InitUInt32("vu32_856712",856712);
    rbusProperty_t vi64_987654321213 = rbusProperty_InitInt64("vi64_987654321213",987654321213);
    rbusProperty_t vu64_987654321213 = rbusProperty_InitUInt64("vu64_987654321213",987654321213);
    rbusProperty_t vf32_354dot678 = rbusProperty_InitSingle("vf32_354dot678",354.678f);
    rbusProperty_t vf64_789dot4738291023 = rbusProperty_InitDouble("vf64_789dot4738291023",789.4738291023);
    rbusProperty_t vtv = rbusProperty_InitTime("vtv",&tv1);
    rbusProperty_t vs = rbusProperty_InitString("vs",teststring);
    rbusProperty_t vbytes = rbusProperty_InitBytes("vbytes",(uint8_t const*)teststring, strlen(teststring)+1);
    rbusProperty_t vprop = rbusProperty_InitProperty("vprop",prop);
    rbusProperty_t vobj = rbusProperty_InitObject("vobj",obj);    

    TEST(rbusProperty_GetName(vbtrue) && !strcmp(rbusProperty_GetName(vbtrue), "vbtrue"));
    TEST(!rbusProperty_GetName(vbfalse));
    TEST(rbusProperty_GetName(vu64_987654321213) && !strcmp(rbusProperty_GetName(vu64_987654321213), "vu64_987654321213"));

    TEST(rbusProperty_GetBoolean(vbtrue) == true);
    TEST(rbusProperty_GetBoolean(vbfalse) == false);
    TEST(rbusProperty_GetInt16(vi16_n1234) == -1234);
    TEST(rbusProperty_GetUInt16(vu16_4321) == 4321);
    TEST(rbusProperty_GetInt32(vi32_689013) == 689013);
    TEST(rbusProperty_GetUInt32(vu32_856712) == 856712);
    TEST(rbusProperty_GetInt64(vi64_987654321213) == 987654321213);
    TEST(rbusProperty_GetUInt64(vu64_987654321213) == 987654321213);
    TEST(rbusProperty_GetSingle(vf32_354dot678) == 354.678f);
    TEST(rbusProperty_GetDouble(vf64_789dot4738291023) == 789.4738291023);
    TEST(memcmp(rbusProperty_GetTime(vtv), &tv1, sizeof(rbusDateTime_t)) == 0);
    TEST(rbusProperty_GetString(vs, NULL) && !strcmp(rbusProperty_GetString(vs, NULL), "Hello World"));
    TEST(rbusProperty_GetBytes(vbytes, NULL) && !strcmp((char const*)rbusProperty_GetBytes(vbytes, NULL), "Hello World"));
    TEST(rbusProperty_GetProperty(vprop) != NULL && rbusProperty_GetName(rbusProperty_GetProperty(vprop)) && !strcmp(rbusProperty_GetName(rbusProperty_GetProperty(vprop)), "MyProp"));
    TEST(rbusProperty_GetObject(vobj) != NULL && rbusObject_GetName(rbusProperty_GetObject(vobj)) && !strcmp(rbusObject_GetName(rbusProperty_GetObject(vobj)), "MyObj"));

    sleep(1);
    rbusValue_MarshallTMtoRBUS(&tv1, localtime(&nowtime));

    rbusProperty_SetBoolean(vbtrue, false);
    rbusProperty_SetBoolean(vbfalse, true);
    rbusProperty_SetInt16(vi16_n1234, 1234);
    rbusProperty_SetUInt16(vu16_4321, (uint16_t)-4321);
    rbusProperty_SetInt32(vi32_689013, -689013);
    rbusProperty_SetUInt32(vu32_856712, (uint32_t)-856712);
    rbusProperty_SetInt64(vi64_987654321213, -987654321213);
    rbusProperty_SetUInt64(vu64_987654321213, (uint64_t)-987654321213);
    rbusProperty_SetSingle(vf32_354dot678, -354.678f);
    rbusProperty_SetDouble(vf64_789dot4738291023, -789.4738291023);
    rbusProperty_SetTime(vtv, &tv1);
    rbusProperty_SetString(vs, teststring2);
    rbusProperty_SetBytes(vbytes, (uint8_t const*)teststring2, strlen(teststring2)+1);
    rbusProperty_SetProperty(vprop, prop2);
    rbusProperty_SetObject(vobj, obj2);

    TEST(rbusProperty_GetBoolean(vbtrue) == false);
    TEST(rbusProperty_GetBoolean(vbfalse) == true);
    TEST(rbusProperty_GetInt16(vi16_n1234) == 1234);
    TEST(rbusProperty_GetUInt16(vu16_4321) == (uint16_t)-4321);
    TEST(rbusProperty_GetInt32(vi32_689013) == -689013);
    TEST(rbusProperty_GetUInt32(vu32_856712) == (uint32_t)-856712);
    TEST(rbusProperty_GetInt64(vi64_987654321213) == -987654321213);
    TEST(rbusProperty_GetUInt64(vu64_987654321213) == (uint64_t)-987654321213);
    TEST(rbusProperty_GetSingle(vf32_354dot678) == -354.678f);
    TEST(rbusProperty_GetDouble(vf64_789dot4738291023) == -789.4738291023);
    TEST(memcmp(rbusProperty_GetTime(vtv), &tv1, sizeof(rbusDateTime_t)) == 0);
    TEST(rbusProperty_GetString(vs, NULL) && !strcmp(rbusProperty_GetString(vs, NULL), "Goodby Cruel World"));
    TEST(rbusProperty_GetBytes(vbytes, NULL) && !strcmp((char const*)rbusProperty_GetBytes(vbytes, NULL), "Goodby Cruel World"));
    TEST(rbusProperty_GetProperty(vprop) != NULL && rbusProperty_GetName(rbusProperty_GetProperty(vprop)) && !strcmp(rbusProperty_GetName(rbusProperty_GetProperty(vprop)), "MyProp2"));
    TEST(rbusProperty_GetObject(vobj) != NULL && rbusObject_GetName(rbusProperty_GetObject(vobj)) && !strcmp(rbusObject_GetName(rbusProperty_GetObject(vobj)), "MyObj2"));

    rbusProperty_Releases(17, vbtrue, vbfalse, vi16_n1234, vu16_4321, vi32_689013, vu32_856712, vi64_987654321213, vu64_987654321213, vf32_354dot678, vf64_789dot4738291023, vtv, vs, vbytes, vprop, vobj, prop, prop2);
    rbusObject_Releases(2, obj, obj2);
}

void testPropertyAPI(rbusHandle_t handle, int* countPass, int* countFail)
{
    (void)handle;

    testPropertyName();
    testPropertyList();
    testPropertyValue();
    testPropertyCompare();
    testValue_InitGetSetByType();

    *countPass = gCountPass;
    *countFail = gCountFail;
    PRINT_TEST_RESULTS("test_PropertyAPI");
}
