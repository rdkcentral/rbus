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

#include <rbus.h>
#include <rtRetainable.h>
#include <rtMemory.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#define VERIFY_NULL(T)      if(NULL == T){ return; }

struct _rbusProperty
{
    rtRetainable retainable;
    char* name;
    rbusValue_t value;
    struct _rbusProperty* next;
};

rbusProperty_t rbusProperty_Init(rbusProperty_t* pproperty, char const* name, rbusValue_t value)
{
    rbusProperty_t p = rt_calloc(1, sizeof(struct _rbusProperty));
    p->retainable.refCount = 1;
    if(name)
        p->name = strdup(name);
    if(value)
        rbusProperty_SetValue(p, value);
    if(pproperty)
        *pproperty = p;
    return p;
}

void rbusProperty_Destroy(rtRetainable* r)
{
    rbusProperty_t property = (rbusProperty_t)r;
    VERIFY_NULL(property);

    if(property->name)
    {
        free(property->name);
        property->name = NULL;
    }

    rbusValue_Release(property->value);
    if(property->next)
    {
        rbusProperty_Release(property->next);
        property->next = NULL;
    }

    free(property);
}

void rbusProperty_Retain(rbusProperty_t property)
{
    VERIFY_NULL(property);
    rtRetainable_retain(property);
}

void rbusProperty_Release(rbusProperty_t property)
{
    VERIFY_NULL(property);
    rtRetainable_release(property, rbusProperty_Destroy);
}

void rbusProperty_Releases(int count, ...)
{
    int i;
    va_list vl;
    va_start(vl, count);
    for(i = 0; i < count; ++i)
    {
        rbusProperty_t prop = va_arg(vl, rbusProperty_t);
        if(prop)
            rtRetainable_release(prop, rbusProperty_Destroy);
    }
    va_end(vl);
}

void rbusProperty_fwrite(rbusProperty_t prop, int depth, FILE* fout)
{
    int i;
    VERIFY_NULL(prop);
    VERIFY_NULL(fout);
    for(i=0; i<depth; ++i)
        fprintf(fout, " ");
    fprintf(fout, "rbusProperty name=%s\r\n", rbusProperty_GetName(prop));
    rbusValue_fwrite(rbusProperty_GetValue(prop), depth+1, fout);
    fprintf(fout,"\r\n");
    prop = rbusProperty_GetNext(prop);
    if(prop)
        rbusProperty_fwrite(prop, depth, fout);
}

#if 0
void rbusProperty_Copy(rbusProperty_t destination, rbusProperty_t source)
{
    if(destination->name)
    {
        free(destination->name);
        destination->name = NULL;
    }

    if(source->name)
        destination->name = strdup(source->name);

    rbusValue_Copy(destination->value, source->value);

    if(source->next)
    {
        rbusProperty_SetNext(destination, source->next);
    }
}
#endif

int rbusProperty_Compare(rbusProperty_t property1, rbusProperty_t property2)
{
    int rc;

    if(property1 == property2)
        return 0;
    if((!property1)||(!property2))
        return -1;

    rc = strcmp(property1->name, property2->name);
    if(rc)
        return rc;/*return strcmp result so we can use rbusProperty_Compare to sort properties by name*/

    return rbusValue_Compare(property1->value, property2->value);
}

char const* rbusProperty_GetName(rbusProperty_t property)
{
    return property->name;
}

void rbusProperty_SetName(rbusProperty_t property, char const* name)
{
    VERIFY_NULL(property);
    if(property->name)
        free(property->name);
    if(name)
        property->name = strdup(name);
    else
        property->name = NULL;
}

rbusValue_t rbusProperty_GetValue(rbusProperty_t property)
{
    return property->value;
}

void rbusProperty_SetValue(rbusProperty_t property, rbusValue_t value)
{
    VERIFY_NULL(property);
    if(property->value)
        rbusValue_Release(property->value);
    property->value = value;
    if(property->value)
        rbusValue_Retain(property->value);
}

rbusProperty_t rbusProperty_GetNext(rbusProperty_t property)
{
    return property->next;
}

void rbusProperty_SetNext(rbusProperty_t property, rbusProperty_t next)
{
    VERIFY_NULL(property);
    if(property->next)
        rbusProperty_Release(property->next);
    property->next = next;
    if(property->next)
        rbusProperty_Retain(property->next);
}

void rbusProperty_Append(rbusProperty_t property, rbusProperty_t back)
{
    rbusProperty_t last = property;
    VERIFY_NULL(last);
    while(last->next)
        last = last->next;
    rbusProperty_SetNext(last, back);
}

uint32_t rbusProperty_Count(rbusProperty_t property)
{
    uint32_t count = 1;
    rbusProperty_t prop = property;
    if(!prop)
        return 0;
    while(prop->next)
    {
        count++;
        prop = prop->next;
    }
    return count;
}

rbusProperty_t rbusProperty_InitString(char const* name, char const* s)
{
    rbusValue_t v = rbusValue_InitString(s);
    rbusProperty_t p = rbusProperty_Init(NULL, name, v);
    rbusValue_Release(v);
    return p;
}

rbusProperty_t rbusProperty_InitBytes(char const* name, uint8_t const* bytes, int len)
{
    rbusValue_t v = rbusValue_InitBytes(bytes, len);
    rbusProperty_t p = rbusProperty_Init(NULL, name, v);
    rbusValue_Release(v);
    return p;
}

char const* rbusProperty_GetString(rbusProperty_t property, int* len)
{
    if(!property)
        return NULL;
    return rbusValue_GetString(property->value, len);
}

uint8_t const* rbusProperty_GetBytes(rbusProperty_t property, int* len)
{
    if(!property)
        return NULL;
    return rbusValue_GetBytes(property->value, len);
}

rbusValueError_t rbusProperty_GetStringEx(rbusProperty_t property, char const** s, int* len)
{
    if(property)
        return rbusValue_GetStringEx(property->value, s, len);
    return RBUS_VALUE_ERROR_NULL;
}

rbusValueError_t rbusProperty_GetBytesEx(rbusProperty_t property, uint8_t const** bytes, int* len)
{
    if(property)
        return rbusValue_GetBytesEx(property->value, bytes, len);
    return RBUS_VALUE_ERROR_NULL;
}

void rbusProperty_SetString(rbusProperty_t property, char const* s)
{
    VERIFY_NULL(property);
    if(property->value)
        rbusValue_Release(property->value);
    property->value = rbusValue_InitString(s);
}

void rbusProperty_SetBytes(rbusProperty_t property, uint8_t const* bytes, int len)
{
    VERIFY_NULL(property);
    if(property->value)
        rbusValue_Release(property->value);
    property->value = rbusValue_InitBytes(bytes, len);
}

rbusProperty_t rbusProperty_AppendString(rbusProperty_t property, char const* name, char const* s)
{
    rbusProperty_t p = rbusProperty_InitString(name, s);
    if(property)
    {
        rbusProperty_Append(property, p);
        rbusProperty_Release(p);
    }
    return p;
}

rbusProperty_t rbusProperty_AppendBytes(rbusProperty_t property, char const* name, uint8_t const* bytes, int len)
{
    rbusProperty_t p = rbusProperty_InitBytes(name, bytes, len);
    if(property)
    {
        rbusProperty_Append(property, p);
        rbusProperty_Release(p);
    }
    return p;
}

#define DEFINE_PROPERTY_TYPE_FUNCS(T1, T2, T3, T4)\
rbusProperty_t rbusProperty_Init##T1(char const* name, T2 data)\
{\
    rbusValue_t v = rbusValue_Init##T1(data);\
    rbusProperty_t p = rbusProperty_Init(NULL, name, v);\
    rbusValue_Release(v);\
    return p;\
}\
T2 rbusProperty_Get##T1(rbusProperty_t property)\
{\
    return rbusValue_Get##T1(property->value);\
}\
rbusValueError_t rbusProperty_Get##T1##Ex(rbusProperty_t property, T2* data)\
{\
    if(property->value)\
        return rbusValue_Get##T1##Ex(property->value, data);\
    return RBUS_VALUE_ERROR_NULL;\
}\
void rbusProperty_Set##T1(rbusProperty_t property, T2 data)\
{\
    if(property->value)\
        rbusValue_Release(property->value);\
    property->value = rbusValue_Init##T1(data);\
}\
rbusProperty_t rbusProperty_Append##T1(rbusProperty_t property, char const* name, T2 data)\
{\
    rbusProperty_t p = rbusProperty_Init##T1(name, data);\
    if(property)\
    {\
        rbusProperty_Append(property, p);\
        rbusProperty_Release(p);\
    }\
    return p;\
}

DEFINE_PROPERTY_TYPE_FUNCS(Boolean, bool, RBUS_BOOLEAN, b)
DEFINE_PROPERTY_TYPE_FUNCS(Char, char, RBUS_CHAR, c)
DEFINE_PROPERTY_TYPE_FUNCS(Byte, unsigned char, RBUS_BYTE, u)
DEFINE_PROPERTY_TYPE_FUNCS(Int8, int8_t, RBUS_INT8, i8)
DEFINE_PROPERTY_TYPE_FUNCS(UInt8, uint8_t, RBUS_UINT8, u8)
DEFINE_PROPERTY_TYPE_FUNCS(Int16, int16_t, RBUS_INT16, i16)
DEFINE_PROPERTY_TYPE_FUNCS(UInt16, uint16_t, RBUS_UINT16, u16)
DEFINE_PROPERTY_TYPE_FUNCS(Int32, int32_t, RBUS_INT32, i32)
DEFINE_PROPERTY_TYPE_FUNCS(UInt32, uint32_t, RBUS_UINT32, u32)
DEFINE_PROPERTY_TYPE_FUNCS(Int64, int64_t, RBUS_INT64, i64)
DEFINE_PROPERTY_TYPE_FUNCS(UInt64, uint64_t, RBUS_UINT64, u64)
DEFINE_PROPERTY_TYPE_FUNCS(Single, float, RBUS_SINGLE, f32)
DEFINE_PROPERTY_TYPE_FUNCS(Double, double, RBUS_DOUBLE, f64)
DEFINE_PROPERTY_TYPE_FUNCS(Time,rbusDateTime_t const*, RBUS_DATETIME, tv);
DEFINE_PROPERTY_TYPE_FUNCS(Property,struct _rbusProperty*, RBUS_PROPERTY, property);
DEFINE_PROPERTY_TYPE_FUNCS(Object,struct _rbusObject*, RBUS_OBJECT, object);
