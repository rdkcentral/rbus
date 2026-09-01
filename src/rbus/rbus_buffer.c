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
#define _GNU_SOURCE 1
#include <rbus.h>
#include "rbus_log.h"
#include <rbus_buffer.h>
#include "rtMemory.h"
#include <endian.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define VERIFY_NULL(T) if(NULL == T){ return; }
#define BUFFER_BLOCK_SIZE 64

#define rbusHostToLittleInt16(n) htole16(n)
#define rbusLittleToHostInt16(n) le16toh(n)
#define rbusHostToLittleInt32(n) htole32(n)
#define rbusLittleToHostInt32(n) le32toh(n)
#define rbusHostToLittleInt64(n) htole64(n)
#define rbusLittleToHostInt64(n) le64toh(n)

float rbusHostToLittleSingle(float n)
{
    union
    {
        float f32;
        uint32_t u32;
    } u;
    u.f32 = n;
    u.u32 = rbusHostToLittleInt32(u.u32);
    return u.f32;
}

float rbusLittleToHostSingle(float n)
{
    union
    {
        float f32;
        uint32_t u32;
    } u;
    u.f32 = n;
    u.u32 = rbusLittleToHostInt32(u.u32);
    return u.f32;
}

double rbusHostToLittleDouble(double n)
{
    union
    {
        double f64;
        uint64_t u64;
    } u;
    u.f64 = n;
    u.u64 = rbusHostToLittleInt64(u.u64);
    return u.f64;
}

double rbusLittleToHostDouble(double n)
{
    union
    {
        double f64;
        uint64_t u64;
    } u;
    u.f64 = n;
    u.u64 = rbusLittleToHostInt64(u.u64);
    return u.f64;
}

void rbusBuffer_Create(rbusBuffer_t* buff)
{
    (*buff) = rt_malloc(sizeof(struct _rbusBuffer));
    (*buff)->lenAlloc = BUFFER_BLOCK_SIZE;
    (*buff)->posWrite = 0;
    (*buff)->posRead = 0;
    (*buff)->data = (*buff)->block1;
}

void rbusBuffer_Destroy(rbusBuffer_t buff)
{
    VERIFY_NULL(buff);
    if(buff->data != buff->block1)
    {
        assert(buff->lenAlloc > BUFFER_BLOCK_SIZE);
        free(buff->data);
    }
    free(buff);
}

void rbusBuffer_Write(rbusBuffer_t buff, void const* data, int len)
{
    VERIFY_NULL(buff);
    VERIFY_NULL(data);
    rbusBuffer_Reserve(buff, len);
    memcpy(buff->data + buff->posWrite, data, len);
    buff->posWrite += len;
}

void rbusBuffer_Reserve(rbusBuffer_t buff, int len)
{
    VERIFY_NULL(buff);
    assert(buff->data);
    int posNext = buff->posWrite+len;
    if(posNext > buff->lenAlloc)
    {
        buff->lenAlloc = (posNext/BUFFER_BLOCK_SIZE+1)*BUFFER_BLOCK_SIZE;
        if(buff->data == buff->block1)
        {
            buff->data = rt_malloc(buff->lenAlloc);
            if(buff->posWrite > 0)
                memcpy(buff->data, buff->block1, buff->posWrite);
        }
        else
        {
            buff->data = rt_realloc(buff->data, buff->lenAlloc);
        }
    }
}

void rbusBuffer_WriteTypeLengthValue(
  rbusBuffer_t      buff,
  rbusValueType_t   type,
  uint16_t          length,
  void const*       value)
{
  uint16_t letype = rbusHostToLittleInt16(type);
  uint16_t lelength = rbusHostToLittleInt16(length);
  rbusBuffer_Write(buff, &letype, sizeof(uint16_t));
  rbusBuffer_Write(buff, &lelength, sizeof(uint16_t));
  rbusBuffer_Write(buff, value, length);
}

void rbusBuffer_WriteStringTLV(rbusBuffer_t buff, char const* s, int len)
{
    /*len should be strlen(s)+1 for null terminator*/
    rbusBuffer_WriteTypeLengthValue(buff, RBUS_STRING, len, s);
}

void rbusBuffer_WriteBooleanTLV(rbusBuffer_t buff, bool b)
{
    assert(sizeof(bool) == 1);/*otherwise we need to handle endian*/    
    rbusBuffer_WriteTypeLengthValue(buff, RBUS_BOOLEAN, sizeof(bool), &b);
}

void rbusBuffer_WriteCharTLV(rbusBuffer_t buff, char c)
{
    assert(sizeof(char) == 1);/*otherwise we need to handle endian*/    
    rbusBuffer_WriteTypeLengthValue(buff, RBUS_CHAR, sizeof(char), &c);
}

void rbusBuffer_WriteByteTLV(rbusBuffer_t buff, unsigned char u)
{
    assert(sizeof(unsigned char) == 1);/*otherwise we need to handle endian*/    
    rbusBuffer_WriteTypeLengthValue(buff, RBUS_BYTE, sizeof(unsigned char), &u);
}

void rbusBuffer_WriteInt8TLV(rbusBuffer_t buff, int8_t i8)
{
    rbusBuffer_WriteTypeLengthValue(buff, RBUS_INT8, sizeof(int8_t), &i8);
}

void rbusBuffer_WriteUInt8TLV(rbusBuffer_t buff, uint8_t u8)
{
    rbusBuffer_WriteTypeLengthValue(buff, RBUS_UINT8, sizeof(uint8_t), &u8);
}

void rbusBuffer_WriteInt16TLV(rbusBuffer_t buff, int16_t i16)
{
    int16_t temp = rbusHostToLittleInt16(i16);
    rbusBuffer_WriteTypeLengthValue(buff, RBUS_INT16, sizeof(int16_t), &temp);
}

void rbusBuffer_WriteUInt16TLV(rbusBuffer_t buff, uint16_t u16)
{
    uint16_t temp = rbusHostToLittleInt16(u16);
    rbusBuffer_WriteTypeLengthValue(buff, RBUS_UINT16, sizeof(uint16_t), &temp);
}

void rbusBuffer_WriteInt32TLV(rbusBuffer_t buff, int32_t i32)
{
    int32_t temp = rbusHostToLittleInt32(i32);
    rbusBuffer_WriteTypeLengthValue(buff, RBUS_INT32, sizeof(int32_t), &temp);
}

void rbusBuffer_WriteUInt32TLV(rbusBuffer_t buff, uint32_t u32)
{
    uint32_t temp = rbusHostToLittleInt32(u32);
    rbusBuffer_WriteTypeLengthValue(buff, RBUS_UINT32, sizeof(uint32_t), &temp);
}

void rbusBuffer_WriteInt64TLV(rbusBuffer_t buff, int64_t i64)
{
    int64_t temp = rbusHostToLittleInt64(i64);
    rbusBuffer_WriteTypeLengthValue(buff, RBUS_INT64, sizeof(int64_t), &temp);
}

void rbusBuffer_WriteUInt64TLV(rbusBuffer_t buff, uint64_t u64)
{
    uint64_t temp = rbusHostToLittleInt64(u64);
    rbusBuffer_WriteTypeLengthValue(buff, RBUS_UINT64, sizeof(uint64_t), &temp);
}

void rbusBuffer_WriteSingleTLV(rbusBuffer_t buff, float f32)
{
    float temp = rbusHostToLittleSingle(f32);
    rbusBuffer_WriteTypeLengthValue(buff, RBUS_SINGLE, sizeof(float), &temp);
}

void rbusBuffer_WriteDoubleTLV(rbusBuffer_t buff, double f64)
{
    double temp = rbusHostToLittleDouble(f64);
    rbusBuffer_WriteTypeLengthValue(buff, RBUS_DOUBLE, sizeof(double), &temp);
}

void rbusBuffer_WriteDateTimeTLV(rbusBuffer_t buff, rbusDateTime_t const* tv)
{
    rbusDateTime_t temp = {{0},{0}};
    VERIFY_NULL(tv);
    temp.m_time.tm_sec      = rbusHostToLittleInt32(tv->m_time.tm_sec);
    temp.m_time.tm_min      = rbusHostToLittleInt32(tv->m_time.tm_min);
    temp.m_time.tm_hour     = rbusHostToLittleInt32(tv->m_time.tm_hour);
    temp.m_time.tm_mday     = rbusHostToLittleInt32(tv->m_time.tm_mday);
    temp.m_time.tm_mon      = rbusHostToLittleInt32(tv->m_time.tm_mon);
    temp.m_time.tm_year     = rbusHostToLittleInt32(tv->m_time.tm_year);
    temp.m_time.tm_wday     = rbusHostToLittleInt32(tv->m_time.tm_wday);
    temp.m_time.tm_yday     = rbusHostToLittleInt32(tv->m_time.tm_yday);
    temp.m_time.tm_isdst    = rbusHostToLittleInt32(tv->m_time.tm_isdst);
    temp.m_tz.m_tzhour      = rbusHostToLittleInt32(tv->m_tz.m_tzhour);
    temp.m_tz.m_tzmin       = rbusHostToLittleInt32(tv->m_tz.m_tzmin);
    temp.m_tz.m_isWest      = rbusHostToLittleInt32(tv->m_tz.m_isWest);
    rbusBuffer_WriteTypeLengthValue(buff, RBUS_DATETIME, sizeof(temp), &temp);
}

void rbusBuffer_WriteBytesTLV(rbusBuffer_t buff, uint8_t* bytes, int len)
{
    rbusBuffer_WriteTypeLengthValue(buff, RBUS_BYTES, len, bytes);
}

int rbusBuffer_Read(rbusBuffer_t const buff, void* data, int len)
{
    if((!buff) || (!data))
        return -1;
    if(!(buff->posRead + len < buff->lenAlloc))
    {
        RBUSLOG_WARN("rbusBuffer_Read failed");
        return -1;
    }
    memcpy(data, buff->data + buff->posRead, len);
    buff->posRead += len;
    return 0;
}

int rbusBuffer_ReadString(rbusBuffer_t const buff, char** s, int* len)
{
    if(!buff)
        return -1;
    *len = buff->posRead;
    *s = rt_malloc(buff->posRead+1);/*TODO is +1 needed?*/
    return rbusBuffer_Read(buff, *s, *len);
}

int rbusBuffer_ReadBoolean(rbusBuffer_t const buff, bool* b)
{
    assert(sizeof(bool) == 1);/*otherwise we need to handle endian*/
    return rbusBuffer_Read(buff, b, sizeof(bool));
}

int rbusBuffer_ReadChar(rbusBuffer_t const buff, char* c)
{
    assert(sizeof(char) == 1);/*otherwise we need to handle endian*/
    return rbusBuffer_Read(buff, c, sizeof(char));
}

int rbusBuffer_ReadByte(rbusBuffer_t const buff, unsigned char* u)
{
    assert(sizeof(unsigned char) == 1);/*otherwise we need to handle endian*/
    return rbusBuffer_Read(buff, u, sizeof(unsigned char));
}

int rbusBuffer_ReadInt8(rbusBuffer_t const buff, int8_t* i8)
{
    return rbusBuffer_Read(buff, i8, sizeof(int8_t));
}

int rbusBuffer_ReadUInt8(rbusBuffer_t const buff, uint8_t* u8)
{
    return rbusBuffer_Read(buff, u8, sizeof(uint8_t));
}

int rbusBuffer_ReadInt16(rbusBuffer_t const buff, int16_t* i16)
{
    int16_t temp;
    int rc = rbusBuffer_Read(buff, &temp, sizeof(int16_t));
    *i16 = rbusLittleToHostInt16(temp);
    return rc;
}

int rbusBuffer_ReadUInt16(rbusBuffer_t const buff, uint16_t* u16)
{
    uint16_t temp;
    int rc = rbusBuffer_Read(buff, &temp, sizeof(uint16_t));
    *u16 = rbusLittleToHostInt16(temp);
    return rc;
}

int rbusBuffer_ReadInt32(rbusBuffer_t const buff, int32_t* i32)
{
    int32_t temp;
    int rc = rbusBuffer_Read(buff, &temp, sizeof(int32_t));
    *i32 = rbusLittleToHostInt32(temp);
    return rc;
}

int rbusBuffer_ReadUInt32(rbusBuffer_t const buff, uint32_t* u32)
{
    uint32_t temp;
    int rc = rbusBuffer_Read(buff, &temp, sizeof(uint32_t));
    *u32 = rbusLittleToHostInt32(temp);
    return rc;
}

int rbusBuffer_ReadInt64(rbusBuffer_t const buff, int64_t* i64)
{
    int64_t temp;
    int rc = rbusBuffer_Read(buff, &temp, sizeof(int64_t));
    *i64 = rbusLittleToHostInt64(temp);
    return rc;
}

int rbusBuffer_ReadUInt64(rbusBuffer_t const buff, uint64_t* u64)
{
    uint64_t temp;
    int rc = rbusBuffer_Read(buff, &temp, sizeof(uint64_t));
    *u64 = rbusLittleToHostInt64(temp);
    return rc;
}

int rbusBuffer_ReadSingle(rbusBuffer_t const buff, float* f32)
{
    float temp;
    int rc = rbusBuffer_Read(buff, &temp, sizeof(float));
    *f32 = rbusLittleToHostSingle(temp);
    return rc;
}

int rbusBuffer_ReadDouble(rbusBuffer_t const buff, double* f64)
{
    double temp;
    int rc = rbusBuffer_Read(buff, &temp, sizeof(double));
    *f64 = rbusLittleToHostDouble(temp);
    return rc;
}

int rbusBuffer_ReadDateTime(rbusBuffer_t const buff, rbusDateTime_t* tv)
{
    rbusDateTime_t temp;
    if(!tv)
        return -1;
    int rc = rbusBuffer_Read(buff, &temp, sizeof(rbusDateTime_t));
    tv->m_time.tm_sec   = rbusLittleToHostInt32(temp.m_time.tm_sec);
    tv->m_time.tm_min   = rbusLittleToHostInt32(temp.m_time.tm_min);
    tv->m_time.tm_hour  = rbusLittleToHostInt32(temp.m_time.tm_hour);
    tv->m_time.tm_mday  = rbusLittleToHostInt32(temp.m_time.tm_mday);
    tv->m_time.tm_mon   = rbusLittleToHostInt32(temp.m_time.tm_mon);
    tv->m_time.tm_year  = rbusLittleToHostInt32(temp.m_time.tm_year);
    tv->m_time.tm_wday  = rbusLittleToHostInt32(temp.m_time.tm_wday);
    tv->m_time.tm_yday  = rbusLittleToHostInt32(temp.m_time.tm_yday);
    tv->m_time.tm_isdst = rbusLittleToHostInt32(temp.m_time.tm_isdst);
    tv->m_tz.m_tzhour   = rbusLittleToHostInt32(temp.m_tz.m_tzhour);
    tv->m_tz.m_tzmin    = rbusLittleToHostInt32(temp.m_tz.m_tzmin);
    tv->m_tz.m_isWest   = rbusLittleToHostInt32(temp.m_tz.m_isWest);
    return rc;
}

int rbusBuffer_ReadBytes(rbusBuffer_t const buff, uint8_t** bytes, int* len)
{
    if(!buff)
        return -1;
    *len = buff->posWrite;
    *bytes = rt_malloc(buff->posWrite);
    return rbusBuffer_Read(buff, bytes, buff->posWrite);
}
