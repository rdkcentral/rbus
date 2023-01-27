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
#include "rbus_value.h"
#include "testValueHelper.h"

TestValueProperty gTestData[] = {
    { RBUS_BOOLEAN, "Device.TestProvider.Value.BOOLEAN", {NULL} },
    { RBUS_CHAR, "Device.TestProvider.Value.CHAR", {NULL} },
    { RBUS_BYTE, "Device.TestProvider.Value.BYTE", {NULL} },
    { RBUS_INT16, "Device.TestProvider.Value.INT16", {NULL} },
    { RBUS_INT16, "Device.TestProvider.Value.UINT16", {NULL} },
    { RBUS_INT32, "Device.TestProvider.Value.INT32", {NULL} },
    { RBUS_UINT32, "Device.TestProvider.Value.UINT32", {NULL} },
    { RBUS_INT64, "Device.TestProvider.Value.INT64", {NULL} },
    { RBUS_UINT64, "Device.TestProvider.Value.UINT64", {NULL} },
    { RBUS_SINGLE, "Device.TestProvider.Value.SINGLE", {NULL} },
    { RBUS_DOUBLE, "Device.TestProvider.Value.DOUBLE", {NULL} },
    { RBUS_DATETIME, "Device.TestProvider.Value.DATETIME", {NULL} },
    { RBUS_STRING, "Device.TestProvider.Value.STRING", {NULL} },
    { RBUS_BYTES, "Device.TestProvider.Value.BYTES", {NULL} },
    { RBUS_PROPERTY, "Device.TestProvider.Value.PROPERTY", {NULL} },
    { RBUS_OBJECT, "Device.TestProvider.Value.OBJECT", {NULL} },
    { RBUS_NONE, NULL, {NULL} }
};

void TestValueProperties_Init(TestValueProperty** list)
{
    TestValueProperty* data;

    rbusDateTime_t rbus_time[3] = {{{0},{0}}};
    time_t nowtime[3] = { 100,300,500};

    char* strings[3] = {
        "short string",
        "",/*empty*/
        "Lorem ipsum dolor sit amet, consectetur adipiscing elit. "
        "Sed condimentum nibh vel justo mattis, ac blandit ex egestas. "
        "Integer eu ex vel mi ullamcorper rhoncus. Integer auctor venenatis "
        "neque, a sodales velit auctor id. Duis massa justo, consequat acet."
    };

    data = gTestData;
    while(data->name)
    {
        int i;
        for(i = 0; i < 3; ++i)
        {
            rbusValue_Init(&data->values[i]);
        }        
        data++;
    }

    rbusValue_SetBoolean(gTestData[0].values[0], true);
    rbusValue_SetBoolean(gTestData[0].values[1], false);
    rbusValue_SetBoolean(gTestData[0].values[2], true);

    rbusValue_SetChar(gTestData[1].values[0], 'a');
    rbusValue_SetChar(gTestData[1].values[1], 'Z');
    rbusValue_SetChar(gTestData[1].values[2], '!');

    rbusValue_SetByte(gTestData[2].values[0], 0x0a);
    rbusValue_SetByte(gTestData[2].values[1], 0x64);
    rbusValue_SetByte(gTestData[2].values[2], 0xff);

    rbusValue_SetInt16(gTestData[3].values[0], -10000);
    rbusValue_SetInt16(gTestData[3].values[1], INT16_MIN);
    rbusValue_SetInt16(gTestData[3].values[2], INT16_MAX);

    rbusValue_SetUInt16(gTestData[4].values[0], 10000);
    rbusValue_SetUInt16(gTestData[4].values[1], 0);
    rbusValue_SetUInt16(gTestData[4].values[2], UINT16_MAX);

    rbusValue_SetInt32(gTestData[5].values[0], -10000000);
    rbusValue_SetInt32(gTestData[5].values[1], INT32_MIN);
    rbusValue_SetInt32(gTestData[5].values[2], INT32_MAX);

    rbusValue_SetUInt32(gTestData[6].values[0], 10000000);
    rbusValue_SetUInt32(gTestData[6].values[1], 0);
    rbusValue_SetUInt32(gTestData[6].values[2], UINT32_MAX);

    rbusValue_SetInt64(gTestData[7].values[0], -12345678900001);
    rbusValue_SetInt64(gTestData[7].values[1], INT64_MIN);
    rbusValue_SetInt64(gTestData[7].values[2], INT64_MAX);

    rbusValue_SetUInt64(gTestData[8].values[0], 98765432100023);
    rbusValue_SetUInt64(gTestData[8].values[1], 0);
    rbusValue_SetUInt64(gTestData[8].values[2], UINT64_MAX);

    rbusValue_SetSingle(gTestData[9].values[0], -0.000001f);
    rbusValue_SetSingle(gTestData[9].values[1], -0.54321f);
    rbusValue_SetSingle(gTestData[9].values[2], 3.141592653589793f);

    rbusValue_SetDouble(gTestData[10].values[0], -0.000000000000001);
    rbusValue_SetDouble(gTestData[10].values[1], 999999999999999.1);
    rbusValue_SetDouble(gTestData[10].values[2], 3.141592653589793);

    rbusValue_MarshallTMtoRBUS(&rbus_time[0], localtime(&nowtime[0]));
    rbusValue_MarshallTMtoRBUS(&rbus_time[0], localtime(&nowtime[1]));
    rbusValue_MarshallTMtoRBUS(&rbus_time[0], localtime(&nowtime[2]));

    rbusValue_SetTime(gTestData[11].values[0], &rbus_time[0]);
    rbusValue_SetTime(gTestData[11].values[1], &rbus_time[1]);
    rbusValue_SetTime(gTestData[11].values[2], &rbus_time[2]);

    rbusValue_SetString(gTestData[12].values[0], strings[0]);
    rbusValue_SetString(gTestData[12].values[1], strings[1]);
    rbusValue_SetString(gTestData[12].values[2], strings[2]);

    rbusValue_SetBytes(gTestData[13].values[0], (uint8_t*)strings[0], strlen(strings[0]));
    rbusValue_SetBytes(gTestData[13].values[1], (uint8_t*)strings[1], strlen(strings[1]));
    rbusValue_SetBytes(gTestData[13].values[2], (uint8_t*)strings[2], strlen(strings[2]));

    /*create 3 different properties*/
    {
        rbusValue_t val;
        rbusProperty_t prop1, prop2, prop3, propIn;

        rbusValue_Init(&val);
        rbusValue_SetString(val, "property 1 value 1");
        rbusProperty_Init(&prop1, "property 1", val);
        rbusValue_Release(val);

        rbusValue_Init(&val);
        rbusValue_SetString(val, "property 2 value 2");
        rbusProperty_Init(&prop2, "property 2", val);
        rbusValue_Release(val);

        rbusValue_Init(&val);
        rbusValue_SetString(val, "inner property of 3 value");
        rbusProperty_Init(&propIn, "inner property of 3", val);
        rbusValue_Release(val);

        rbusValue_Init(&val);
        rbusValue_SetProperty(val, propIn);
        rbusProperty_Release(propIn);
        rbusProperty_Init(&prop3, "property 3", val);
        rbusValue_Release(val);

        rbusValue_SetProperty(gTestData[14].values[0], prop1);
        rbusValue_SetProperty(gTestData[14].values[1], prop2);
        rbusValue_SetProperty(gTestData[14].values[2], prop3);
        
        rbusProperty_Release(prop1);
        rbusProperty_Release(prop2);
        rbusProperty_Release(prop3);
    }
    /*create 3 different objects*/
    {
        rbusObject_t o1, o2, o3;

        /*object one*/
        {
            rbusProperty_t p1, p2, p3, p4, p5, p6;
            rbusValue_t v1, v2, v3, v4, v5, v6;

            rbusValue_Init(&v1);
            rbusValue_Init(&v2);
            rbusValue_Init(&v3);
            rbusValue_Init(&v4);
            rbusValue_Init(&v5);
            rbusValue_Init(&v6);

            rbusValue_SetString(v1, "property_1_string_value");
            rbusValue_SetUInt32(v2, 987654321);
            rbusValue_SetChar(v3, 'z');
            rbusValue_SetBoolean(v4, false);
            rbusValue_SetInt64(v5, 99887766554433211);
            rbusValue_SetDouble(v6, 3.141592653589793);
        
            rbusProperty_Init(&p1, "property_1_string_name", v1);
            rbusProperty_Init(&p2, "property_2_uint32_name", v2);
            rbusProperty_Init(&p3, "property_3_char_name", v3);
            rbusProperty_Init(&p4, "property_4_bool_name", v4);
            rbusProperty_Init(&p5, "property_5_int64_name", v5);
            rbusProperty_Init(&p6, "property_6_double_name", v6);

            rbusValue_Release(v1);
            rbusValue_Release(v2);
            rbusValue_Release(v3);
            rbusValue_Release(v4);
            rbusValue_Release(v5);
            rbusValue_Release(v6);

            rbusProperty_SetNext(p5, p6);
            rbusProperty_SetNext(p4, p5);
            rbusProperty_SetNext(p3, p4);
            rbusProperty_SetNext(p2, p3);
            rbusProperty_SetNext(p1, p2);

            rbusObject_Init(&o1, "object 1");
            rbusObject_SetProperties(o1, p1);

            rbusProperty_Release(p1);
            rbusProperty_Release(p2);
            rbusProperty_Release(p3);
            rbusProperty_Release(p4);
            rbusProperty_Release(p5);
            rbusProperty_Release(p6);
        }

        {
            rbusObject_t o2_2, o2_3;
            rbusProperty_t p1, p2, p3, p4, p5;
            rbusValue_t v1, v2, v3, v4, v5;

            rbusObject_Init(&o2, "object 2");
            rbusObject_Init(&o2_2, "depth 2");
            rbusObject_Init(&o2_3, "depth 3");

            rbusValue_Init(&v1);
            rbusValue_Init(&v2);
            rbusValue_Init(&v3);
            rbusValue_Init(&v4);
            rbusValue_Init(&v5);

            rbusValue_SetString(v1, "level 1");
            rbusValue_SetObject(v2, o2_2);
            rbusValue_SetString(v3, "level 2");
            rbusValue_SetObject(v4, o2_3);
            rbusValue_SetString(v5, "level 3");
        
            rbusProperty_Init(&p1, "property_level_1_string", v1);
            rbusProperty_Init(&p2, "property_level_1_object", v2);
            rbusProperty_Init(&p3, "property_level_2_string", v3);
            rbusProperty_Init(&p4, "property_level_2_object", v4);
            rbusProperty_Init(&p5, "property_level_3_string", v5);

            rbusValue_Release(v1);
            rbusValue_Release(v2);
            rbusValue_Release(v3);
            rbusValue_Release(v4);
            rbusValue_Release(v5);

            rbusProperty_SetNext(p1, p2);
            rbusProperty_SetNext(p3, p4);

            rbusObject_SetProperties(o1, p1);
            rbusObject_SetProperties(o2_2, p3);
            rbusObject_SetProperties(o2_3, p5);

            rbusProperty_Release(p1);
            rbusProperty_Release(p2);
            rbusProperty_Release(p3);
            rbusProperty_Release(p4);
            rbusProperty_Release(p5);

            rbusObject_Release(o2_2);
            rbusObject_Release(o2_3);
        }

        {

            rbusObject_t oDevice, oHosts, oHost, oHost1, oHost2, oWiFi, oRadio, oRadio1, oRadio2, oStats1, oStats2;

            rbusObject_Init(&oDevice, "Device");
            rbusObject_Init(&oHosts, "Hosts");
            rbusObject_InitMultiInstance(&oHost, "Host");
            rbusObject_Init(&oHost1, "1");
            rbusObject_Init(&oHost2, "2");
            rbusObject_Init(&oWiFi, "WiFi");
            rbusObject_InitMultiInstance(&oRadio, "Radio");
            rbusObject_Init(&oRadio1, "1");
            rbusObject_Init(&oStats1, "Stats");
            rbusObject_Init(&oRadio2, "2");
            rbusObject_Init(&oStats2, "Stats");

            rbusValue_t vHostNumberOfEntries, vPhysAddress1, vPhysAddress2, vIPAddress1, vIPAddress2, vActive1, vActive2, 
                vRadioNumberOfEntries, vAlias1, vAlias2, vNoise1, vNoise2, vPacketsSent1, vPacketsSent2;

            rbusValue_Init(&vHostNumberOfEntries);            
            rbusValue_Init(&vPhysAddress1);
            rbusValue_Init(&vPhysAddress2);
            rbusValue_Init(&vIPAddress1);
            rbusValue_Init(&vIPAddress2);
            rbusValue_Init(&vActive1);
            rbusValue_Init(&vActive2);
            rbusValue_Init(&vRadioNumberOfEntries);
            rbusValue_Init(&vAlias1);
            rbusValue_Init(&vAlias2);
            rbusValue_Init(&vNoise1);
            rbusValue_Init(&vNoise2);
            rbusValue_Init(&vPacketsSent1);
            rbusValue_Init(&vPacketsSent2);

            rbusProperty_t pHostNumberOfEntries, pPhysAddress1, pPhysAddress2, pIPAddress1, pIPAddress2, pActive1, pActive2, 
                pRadioNumberOfEntries, pAlias1, pAlias2, pNoise1, pNoise2, pPacketsSent1, pPacketsSent2;

            rbusProperty_Init(&pHostNumberOfEntries, "HostNumberOfEntries", vHostNumberOfEntries);
            rbusProperty_Init(&pPhysAddress1, "PhysAddress", vPhysAddress1);
            rbusProperty_Init(&pPhysAddress2, "PhysAddress", vPhysAddress2);
            rbusProperty_Init(&pIPAddress1, "IPAddress", vIPAddress1);
            rbusProperty_Init(&pIPAddress2, "IPAddress", vIPAddress2);
            rbusProperty_Init(&pActive1, "Active", vActive1);
            rbusProperty_Init(&pActive2, "Active", vActive2);
            rbusProperty_Init(&pRadioNumberOfEntries, "RadioNumberOfEntries", vRadioNumberOfEntries);
            rbusProperty_Init(&pAlias1, "Alias", vAlias1);
            rbusProperty_Init(&pAlias2, "Alias", vAlias2);
            rbusProperty_Init(&pNoise1, "NoiseFloor", vNoise1);
            rbusProperty_Init(&pNoise2, "NoiseFloor", vNoise2);
            rbusProperty_Init(&pPacketsSent1, "PacketsSent1", vPacketsSent1);
            rbusProperty_Init(&pPacketsSent2, "PacketsSent2", vPacketsSent2);

            rbusValue_SetInt32(vHostNumberOfEntries, 2);            
            rbusValue_SetString(vPhysAddress1, "64:00:6a:52:d7:b9");
            rbusValue_SetString(vPhysAddress2, "64:00:4b:21:c5:8a");
            rbusValue_SetString(vIPAddress1, "10.0.0.192");
            rbusValue_SetString(vIPAddress2, "10.0.0.21");
            rbusValue_SetBoolean(vActive1, true);
            rbusValue_SetBoolean(vActive2, false);
            rbusValue_SetInt32(vRadioNumberOfEntries, 2);
            rbusValue_SetString(vAlias1, "Radio1");
            rbusValue_SetString(vAlias2, "Radio2");
            rbusValue_SetInt32(vNoise1, -105);
            rbusValue_SetInt32(vNoise2, -98);
            rbusValue_SetInt32(vPacketsSent1, 1698321);
            rbusValue_SetInt32(vPacketsSent2, 12800);

            rbusProperty_SetNext(pPhysAddress1, pIPAddress1);
            rbusProperty_SetNext(pIPAddress1, pActive1);

            rbusProperty_SetNext(pPhysAddress2, pIPAddress2);
            rbusProperty_SetNext(pIPAddress2, pActive2);

            rbusProperty_SetNext(pNoise1, pPacketsSent1);
            rbusProperty_SetNext(pNoise2, pPacketsSent2);

            rbusObject_SetProperties(oHosts, pHostNumberOfEntries);
            rbusObject_SetProperties(oHost1, pPhysAddress1);
            rbusObject_SetProperties(oHost2, pPhysAddress2);

            rbusObject_SetProperties(oWiFi, pRadioNumberOfEntries);

            rbusObject_SetProperties(oStats1, pNoise1);
            rbusObject_SetProperties(oStats2, pNoise2);

            /*TODO improve rbusObject API to make wiring up tree simpler*/
            rbusObject_SetChildren(oDevice, oHosts);
            rbusObject_SetParent(oHosts, oDevice);
            rbusObject_SetParent(oWiFi, oDevice);
            rbusObject_SetNext(oHosts, oWiFi);
            rbusObject_SetChildren(oHosts, oHost);
            rbusObject_SetParent(oHost, oHosts);
            rbusObject_SetChildren(oHost, oHost1);
            rbusObject_SetParent(oHost1, oHost);
            rbusObject_SetNext(oHost1, oHost2);
            rbusObject_SetParent(oHost2, oHost);
            rbusObject_SetChildren(oWiFi, oRadio);
            rbusObject_SetParent(oRadio, oWiFi);
            rbusObject_SetChildren(oRadio, oRadio1);
            rbusObject_SetParent(oRadio1, oRadio);
            rbusObject_SetNext(oRadio1, oRadio2);
            rbusObject_SetParent(oRadio2, oRadio);
            rbusObject_SetChildren(oRadio1, oStats1);
            rbusObject_SetParent(oStats1, oRadio1);
            rbusObject_SetChildren(oRadio2, oStats2);
            rbusObject_SetParent(oStats2, oRadio2);

            o3 = oDevice;

            rbusObject_Release(oHosts); 
            rbusObject_Release(oHost);
            rbusObject_Release(oHost1); 
            rbusObject_Release(oHost2);
            rbusObject_Release(oWiFi);
            rbusObject_Release(oRadio); 
            rbusObject_Release(oRadio1); 
            rbusObject_Release(oRadio2);
            rbusObject_Release(oStats1);
            rbusObject_Release(oStats2);

            rbusValue_Release(vHostNumberOfEntries);
            rbusValue_Release(vPhysAddress1);
            rbusValue_Release(vPhysAddress2);
            rbusValue_Release(vIPAddress1);
            rbusValue_Release(vIPAddress2);
            rbusValue_Release(vActive1);
            rbusValue_Release(vActive2);
            rbusValue_Release(vRadioNumberOfEntries);
            rbusValue_Release(vAlias1);
            rbusValue_Release(vAlias2);
            rbusValue_Release(vNoise1);
            rbusValue_Release(vNoise2);
            rbusValue_Release(vPacketsSent1);
            rbusValue_Release(vPacketsSent2);

            rbusProperty_Release(pHostNumberOfEntries);
            rbusProperty_Release(pPhysAddress1);
            rbusProperty_Release(pPhysAddress2);
            rbusProperty_Release(pIPAddress1);
            rbusProperty_Release(pIPAddress2);
            rbusProperty_Release(pActive1);
            rbusProperty_Release(pActive2);
            rbusProperty_Release(pRadioNumberOfEntries);
            rbusProperty_Release(pAlias1);
            rbusProperty_Release(pAlias2);
            rbusProperty_Release(pNoise1);
            rbusProperty_Release(pNoise2);
            rbusProperty_Release(pPacketsSent1);
            rbusProperty_Release(pPacketsSent2);
        }

        rbusValue_SetObject(gTestData[15].values[0], o1);
        rbusValue_SetObject(gTestData[15].values[1], o2);
        rbusValue_SetObject(gTestData[15].values[2], o3);
        
        rbusObject_Release(o1);
        rbusObject_Release(o2);
        rbusObject_Release(o3);
    }

    *list = gTestData;
}

void TestValueProperties_Release(TestValueProperty* list)
{
    TestValueProperty* data = list;
    while(data->name)
    {
        rbusValue_Release(data->values[0]);
        rbusValue_Release(data->values[1]);
        rbusValue_Release(data->values[2]);
        data++;
    }
}


