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
#include <msgpack.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "rbuscore_logger.h"
#include "rtRetainable.h"
#include "rtMemory.h"
#include "rbuscore_message.h"

#define VERIFY_UNPACK_NEXT_ITEM()\
    if(msgpack_unpack_next(&message->upk, message->sbuf.data, message->sbuf.size, &message->read_offset) != MSGPACK_UNPACK_SUCCESS)\
    {\
        RBUSCORELOG_ERROR("%s failed to unpack next item", __FUNCTION__);\
        return RT_FAIL;\
    }

#define VERIFY_UNPACK(T)\
    VERIFY_UNPACK_NEXT_ITEM()\
    if(message->upk.data.type != T)\
    {\
        RBUSCORELOG_ERROR("%s unexpected data type %d", __FUNCTION__, message->upk.data.type);\
        return RT_FAIL;\
    }

#define VERIFY_UNPACK2(T,T2)\
    VERIFY_UNPACK_NEXT_ITEM()\
    if(message->upk.data.type != T && message->upk.data.type != T2)\
    {\
        RBUSCORELOG_ERROR("%s unexpected data type %d", __FUNCTION__, message->upk.data.type);\
        return RT_FAIL;\
    }

#define VERIFY_PACK(T)\
    if(msgpack_pack_##T(&message->pk, value) != 0)\
    {\
        RBUSCORELOG_ERROR("%s failed pack value", __FUNCTION__);\
        return RT_FAIL;\
    }\
    return RT_OK;

#define VERIFY_PACK_BUFFER(T, V, L)\
    if(msgpack_pack_##T(&message->pk, (L)) != 0)\
    {\
        RBUSCORELOG_ERROR("%s failed pack buffer length", __FUNCTION__);\
        return RT_FAIL;\
    }\
    if(msgpack_pack_##T##_body(&message->pk, (V), (L)) != 0)\
    {\
        RBUSCORELOG_ERROR("%s failed pack buffer body", __FUNCTION__);\
        return RT_FAIL;\
    }\
    return RT_OK;


struct _rbusMessage
{
    rtRetainable retainable;
    msgpack_sbuffer sbuf;
    msgpack_packer pk;
    msgpack_unpacked upk;
    size_t read_offset;
    int meta_offset;
};

void rbusMessage_Init(rbusMessage* message)
{
    struct _rbusMessage * ptr = rt_malloc(sizeof(struct _rbusMessage));
    msgpack_sbuffer_init(&ptr->sbuf);
    msgpack_packer_init(&ptr->pk, &ptr->sbuf, msgpack_sbuffer_write);
    msgpack_unpacked_init(&ptr->upk);
    *message = ptr;
    ptr->read_offset = 0;
    ptr->meta_offset = 0;
    (*message)->retainable.refCount = 1;
}

void rbusMessage_Destroy(rtRetainable* r)
{
    rbusMessage m = (rbusMessage)r;

    msgpack_sbuffer_destroy(&m->sbuf);
    msgpack_unpacked_destroy(&m->upk);
    free(m);
}

void rbusMessage_Retain(rbusMessage message)
{
    rtRetainable_retain(message);
}

void rbusMessage_Release(rbusMessage message)
{
    if(message)
        rtRetainable_release(message, rbusMessage_Destroy);
}

void rbusMessage_FromBytes(rbusMessage* message, uint8_t const* buff, uint32_t n)
{
    struct _rbusMessage * ptr = rt_malloc(sizeof(struct _rbusMessage));
    msgpack_sbuffer_init(&ptr->sbuf);
    msgpack_unpacked_init(&ptr->upk);
    msgpack_sbuffer_write((void *)&ptr->sbuf, (const char *)buff, n);
    *message = ptr;
    ptr->read_offset = 0;
    ptr->meta_offset = 0;
    (*message)->retainable.refCount = 1;
}

void rbusMessage_ToBytes(rbusMessage message, uint8_t** buff, uint32_t* n)
{
    *buff = (uint8_t *)message->sbuf.data;
    *n = message->sbuf.size;
}

void rbusMessage_ToDebugString(rbusMessage const m, char** s, uint32_t* n)
{
    const int ALLOC_INCREMENT = 2048;
    int size = ALLOC_INCREMENT;
    char * buffer = (char *)rt_malloc(size);
    *s = buffer;

    int saved_offset = m->read_offset;
    m->read_offset = 0;

    int write_offset = 0;
    while(msgpack_unpack_next(&m->upk, m->sbuf.data, m->sbuf.size, &m->read_offset) == MSGPACK_UNPACK_SUCCESS)
    {
        if((1 >= (size - write_offset)) || 
                //Special handling for text as snprintf will write past a buffer boundary if precision calls for it.
                ((MSGPACK_OBJECT_STR == m->upk.data.type) && ((int)m->upk.data.via.str.size > (size - write_offset - 3/*account for quotes + terminator in the output*/))))
        {
            //Truncated output. Indicate so.
            //First, make room for ellipsis

            if((size - write_offset) <= 4)
                write_offset = size - 4;
            buffer[write_offset++] = '.';
            buffer[write_offset++] = '.';
            buffer[write_offset++] = '.';
            buffer[write_offset++] = 0;
            break;
        }
        write_offset += msgpack_object_print_buffer(buffer + write_offset, size - write_offset, m->upk.data);
        if(1 < (size - write_offset))
        {
            buffer[write_offset++] = ' ';
            buffer[write_offset] = '\0';
        }
    }
    //Trim the last space.
    if(write_offset && (' ' == buffer[write_offset - 1]))
    {
        write_offset--;
        buffer[write_offset] = '\0';
    }

    m->read_offset = saved_offset;
    *n = write_offset;
}

rtError rbusMessage_SetString(rbusMessage message, char const* value)
{
    static const char dummy[] = "";
    if(!value)
        value = dummy;
    int length = strlen(value) + 1;
    VERIFY_PACK_BUFFER(str, value, length);
}

rtError rbusMessage_GetString(rbusMessage const message, char const** value)
{
    VERIFY_UNPACK(MSGPACK_OBJECT_STR);
    *value = message->upk.data.via.str.ptr;
    return RT_OK;
}

rtError rbusMessage_SetBytes(rbusMessage message, uint8_t const* bytes, const uint32_t size)
{
    VERIFY_PACK_BUFFER(bin, bytes, size);
}

rtError rbusMessage_GetBytes(rbusMessage message, uint8_t const** value, uint32_t* size)
{
    VERIFY_UNPACK(MSGPACK_OBJECT_BIN);
    *size = message->upk.data.via.bin.size;
    *value = (uint8_t const*)message->upk.data.via.bin.ptr;
    return RT_OK;
}

rtError rbusMessage_SetInt32(rbusMessage message, int32_t value)
{
    VERIFY_PACK(int32);
}

rtError rbusMessage_GetInt32(rbusMessage const message, int32_t* value)
{
    VERIFY_UNPACK2(MSGPACK_OBJECT_POSITIVE_INTEGER, MSGPACK_OBJECT_NEGATIVE_INTEGER);
    *value = (int32_t)message->upk.data.via.i64;
    return RT_OK;
}

rtError rbusMessage_GetUInt32(rbusMessage const message, uint32_t* value)
{
    VERIFY_UNPACK2(MSGPACK_OBJECT_POSITIVE_INTEGER, MSGPACK_OBJECT_NEGATIVE_INTEGER);
    *value = (uint32_t)message->upk.data.via.i64;
    return RT_OK;
}

rtError rbusMessage_SetInt64(rbusMessage message, int64_t value)
{
    VERIFY_PACK(int64);
}

rtError rbusMessage_GetInt64(rbusMessage const message, int64_t* value)
{
    VERIFY_UNPACK2(MSGPACK_OBJECT_POSITIVE_INTEGER, MSGPACK_OBJECT_NEGATIVE_INTEGER);
    *value = message->upk.data.via.i64;
    return RT_OK;
}

rtError rbusMessage_SetDouble(rbusMessage message, double value)
{
    VERIFY_PACK(double);
}

rtError rbusMessage_GetDouble(rbusMessage const message, double* value)
{
    VERIFY_UNPACK(MSGPACK_OBJECT_FLOAT);
    *value = message->upk.data.via.f64;
    return RT_OK;
}

rtError rbusMessage_SetMessage(rbusMessage message, rbusMessage const item)
{
    VERIFY_PACK_BUFFER(bin, item->sbuf.data, item->sbuf.size);
}

rtError rbusMessage_GetMessage(rbusMessage const message, rbusMessage* value)
{
    VERIFY_UNPACK(MSGPACK_OBJECT_BIN);
    rbusMessage_FromBytes(value, (uint8_t*)message->upk.data.via.bin.ptr, message->upk.data.via.bin.size);
    return RT_OK;
}

void rbusMessage_BeginMetaSectionWrite(rbusMessage message)
{
    message->meta_offset = message->sbuf.size;
}

void rbusMessage_EndMetaSectionWrite(rbusMessage message)
{
    msgpack_pack_int32(&message->pk, message->meta_offset | 0x80000000);
    message->sbuf.data[message->sbuf.size - 4] &= 0x7F; //Clear the effects of mask, now that offset is stored as a 4-byte integer.
}

void rbusMessage_BeginMetaSectionRead(rbusMessage message)
{
    int section_offset = 0;
    message->meta_offset = message->read_offset; //For safekeeping.
    message->read_offset = message->sbuf.size - 5;
    rbusMessage_GetInt32(message, &section_offset);
    message->read_offset = section_offset;
}

void rbusMessage_EndMetaSectionRead(rbusMessage message)
{
    message->read_offset = message->meta_offset;
}

#if 0

#define VERIFY(T)\
    do \
    {  \
        int rc = (T); \
        printf("_test_%s " #T  " %s: %d\n", __FUNCTION__, rc == RT_OK ? "passed" : "failed", rc); \
    }while(0);

#define TEST(T) \
    do \
    {  \
        printf("_test_%s " #T  " %s\n", __FUNCTION__, (T) ? "passed" : "failed"); \
    }while(0);

void rbusMessage_Test()
{
    rbusMessage m1;
    rbusMessage m2;
    rbusMessage m3;
    rbusMessage m4;
    int32_t i1, i2;
    double f1, f2;
    char const* s1, *s2, *s3;
    uint8_t const* b1;
    uint8_t const* b2;
    uint32_t bl1, bl2;

    uint8_t* bytes;
    uint32_t len;

    printf("rbusMessage_Test Begin \n");

    rbusMessage_Init(&m1);

    VERIFY(rbusMessage_SetInt32(m1, 100));
    VERIFY(rbusMessage_SetDouble(m1, 100.001));
    VERIFY(rbusMessage_SetString(m1, "string_1"));
    VERIFY(rbusMessage_SetBytes(m1, (uint8_t*)"bytes_1", 8));
    VERIFY(rbusMessage_SetInt32(m1, 200));
    VERIFY(rbusMessage_SetDouble(m1, 200.001));
    VERIFY(rbusMessage_SetString(m1, "string_2"));
    VERIFY(rbusMessage_SetBytes(m1, (uint8_t*)"bytes_2", 8));

    rbusMessage_Init(&m3);
    VERIFY(rbusMessage_SetString(m3, "string_3"));
    VERIFY(rbusMessage_SetMessage(m1, m3));

    VERIFY(rbusMessage_GetInt32(m1, &i1));
    VERIFY(rbusMessage_GetDouble(m1, &f1));
    VERIFY(rbusMessage_GetString(m1, &s1));
    VERIFY(rbusMessage_GetBytes(m1, &b1, &bl1));
    VERIFY(rbusMessage_GetInt32(m1, &i2));
    VERIFY(rbusMessage_GetDouble(m1, &f2));
    VERIFY(rbusMessage_GetString(m1, &s2));
    VERIFY(rbusMessage_GetBytes(m1, &b2, &bl2));
    VERIFY(rbusMessage_GetMessage(m1, &m3));
    VERIFY(rbusMessage_GetString(m3, &s3));

    printf("Got %d, %f, %s, %s, %d, %f, %s, %s, %s\n", i1, f1, s1, (char const*)b1, i2, f2, s2, (char const*)b2, s3);

    TEST(i1 == 100);
    TEST(i2 == 200);
    TEST(f1 == 100.001);
    TEST(f2 == 200.001);
    TEST(strcmp(s1, "string_1")==0);
    TEST(strcmp(s2, "string_2")==0);
    TEST(strcmp((char const*)b1, "bytes_1")==0);
    TEST(strcmp((char const*)b2, "bytes_2")==0);
    TEST(strcmp(s3, "string_3")==0);

    rbusMessage_ToBytes(m1, &bytes, &len);

    rbusMessage_FromBytes(&m2, bytes, len);

    i1 = 0; f1 = 0; s1 = 0; b1 = 0; i2 = 0; f2 = 0; s2 = 0, b2 = 0;

    VERIFY(rbusMessage_GetInt32(m2, &i1));
    VERIFY(rbusMessage_GetDouble(m2, &f1));
    VERIFY(rbusMessage_GetString(m2, &s1));
    VERIFY(rbusMessage_GetBytes(m2, &b1, &bl1));
    VERIFY(rbusMessage_GetInt32(m2, &i2));
    VERIFY(rbusMessage_GetDouble(m2, &f2));
    VERIFY(rbusMessage_GetString(m2, &s2));
    VERIFY(rbusMessage_GetBytes(m2, &b2, &bl2));

    printf("Got %d, %f, %s, %s, %d, %f, %s, %s\n", i1, f1, s1, (char const*)b1, i2, f2, s2, (char const*)b2);

    TEST(i1 == 100);
    TEST(i2 == 200);
    TEST(f1 == 100.001);
    TEST(f2 == 200.001);
    TEST(strcmp(s1, "string_1")==0);
    TEST(strcmp(s2, "string_2")==0);
    TEST(strcmp((char const*)b1, "bytes_1")==0);
    TEST(strcmp((char const*)b2, "bytes_2")==0);

    rbusMessage_Release(m1);
    rbusMessage_Release(m2);
    rbusMessage_Release(m3);

    printf("rbusMessage_Test End \n");
}
#endif
