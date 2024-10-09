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
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>

#define VERIFY_NULL(T)    if(NULL == T){ return; }

struct _rbusObject
{
    rtRetainable retainable;
    char* name;
    rbusObjectType_t type;      /*single or multi-instance*/
    rbusProperty_t properties;  /*the list of properties(tr181 parameters) on this object*/
    struct _rbusObject* parent;  /*the object's parent (NULL for root object)*/
    struct _rbusObject* children;/*the object's child objects which could be a mix of single or multi instance tables (not rows)*/
    struct _rbusObject* next;    /*this object's siblings where the type of sibling is based on parent type
                                  1) if parent is RBUS_OBJECT_SINGLE_INSTANCE | RBUS_OBJECT_MULTI_INSTANCE_ROW: 
                                        list of RBUS_OBJECT_SINGLE_INSTANCE and/or RBUS_OBJECT_MULTI_INSTANCE_TABLE
                                  2) if parent RBUS_OBJECT_MULTI_INSTANCE_TABLE: next is in a list of RBUS_OBJECT_MULTI_INSTANCE_ROW*/
};

rbusObject_t rbusObject_Init(rbusObject_t* pobject, char const* name)
{
    rbusObject_t object;
    object = rt_calloc(1, sizeof(struct _rbusObject));
    object->type = RBUS_OBJECT_SINGLE_INSTANCE;
    object->retainable.refCount = 1;
    if(name)
        object->name = strdup(name);
    if(pobject)
        *pobject = object;
    return object;
}

void rbusObject_InitMultiInstance(rbusObject_t* pobject, char const* name)
{
    rbusObject_Init(pobject, name);
    (*pobject)->type = RBUS_OBJECT_MULTI_INSTANCE;
}

void rbusObject_Destroy(rtRetainable* r)
{
    rbusObject_t object = (rbusObject_t)r;
    VERIFY_NULL(object);
    if(object->name)
    {
        free(object->name);
        object->name = NULL;
    }
    if(object->properties)
    {
        rbusProperty_Release(object->properties);
    }

    rbusObject_SetChildren(object, NULL);
    rbusObject_SetNext(object, NULL);
    rbusObject_SetParent(object, NULL);

    free(object);
}

void rbusObject_Retain(rbusObject_t object)
{
    VERIFY_NULL(object);
    rtRetainable_retain(object);
}

void rbusObject_Release(rbusObject_t object)
{
    VERIFY_NULL(object);
    rtRetainable_release(object, rbusObject_Destroy);
}

void rbusObject_Releases(int count, ...)
{
    int i;
    va_list vl;
    va_start(vl, count);
    for(i = 0; i < count; ++i)
    {
        rbusObject_t obj = va_arg(vl, rbusObject_t);
        if(obj)
            rtRetainable_release(obj, rbusObject_Destroy);
    }
    va_end(vl);
}

int rbusObject_Compare(rbusObject_t object1, rbusObject_t object2, bool recursive)
{
    int rc;
    int match;
    rbusProperty_t prop1;
    rbusProperty_t prop2;
    char const* prop1Name;
    char const* prop2Name;

    if(object1 == object2)
        return 0;

    if((object1==NULL)&&(object2!=NULL))
        return -1;
    if((object1!=NULL)&&(object2==NULL))
        return 1;

    rc = strcmp(object1->name, object2->name);
    if(rc != 0)
        return rc;

    /*verify each property in object1 has a matching property in object2*/
    prop1 = object1->properties;
    while(prop1)
    {
        prop1Name = rbusProperty_GetName(prop1);
        prop2 = object2->properties;
        match = 0;
        while(prop2)
        {
            prop2Name = rbusProperty_GetName(prop2);
            if(strcmp(prop1Name, prop2Name)==0)
            {
                rc = rbusProperty_Compare(prop1, prop2);
                if(rc != 0)
                    return rc;
                match = 1;
                break;
            }
            prop2 = rbusProperty_GetNext(prop2);
        }
        if(!match)
            return 1; /*TODO: 1 implies object1 > object2 but its unclear why that should be the case*/
        prop1 = rbusProperty_GetNext(prop1);
    }

    /*verify there are no additional properties in object2 that do not exist in object1*/
    prop2 = object2->properties;
    while(prop2)
    {
        prop2Name = rbusProperty_GetName(prop2);
        prop1 = object1->properties;
        match = 0;
        while(prop1)
        {
            prop1Name = rbusProperty_GetName(prop1);

            if(strcmp(prop2Name, prop1Name)==0)
            {
                match = 1;
                break;
            }
            prop1 = rbusProperty_GetNext(prop1);
        }
        if(!match)
            return -1; /*TODO: -1 implies object1 < object2 but its unclear why that should be the case*/
        prop2 = rbusProperty_GetNext(prop2);
    }

    /*compare children, recursively*/
    if(recursive)
    {
        int rc;
        int match;
        rbusObject_t obj1;
        rbusObject_t obj2;
        char const* obj1Name;
        char const* obj2Name;
        /*verify each property in object1 has a matching property in object2*/
        obj1 = object1->children;
        while(obj1)
        {
            obj1Name = rbusObject_GetName(obj1);
            obj2 = object2->children;
            match = 0;
            while(obj2)
            {
                obj2Name = rbusObject_GetName(obj2);
                if(strcmp(obj1Name, obj2Name)==0)
                {
                    rc = rbusObject_Compare(obj1, obj2, true);
                    if(rc != 0)
                        return rc;
                    match = 1;
                    break;
                }
                obj2 = rbusObject_GetNext(obj2);
            }
            if(!match)
                return 1;
            obj1 = rbusObject_GetNext(obj1);
        }
        /*verify there are no additional properties in object2 that do not exist in object1*/
        obj2 = object2->children;
        while(obj2)
        {
            obj2Name = rbusObject_GetName(obj2);
            obj1 = object1->children;
            match = 0;
            while(obj1)
            {
                obj1Name = rbusObject_GetName(obj1);
                if(strcmp(obj2Name, obj1Name)==0)
                {
                    match = 1;
                    break;
                }
                obj1 = rbusObject_GetNext(obj1);
            }
            if(!match)
                return -1;
            obj2 = rbusObject_GetNext(obj2);
        }
    }
    return 0; /*everything is equal*/
}

char const* rbusObject_GetName(rbusObject_t object)
{
    return object->name;
}

void rbusObject_SetName(rbusObject_t object, char const* name)
{
    VERIFY_NULL(object);
    if(object->name)
        free(object->name);
    if(name)
        object->name = strdup(name);
    else
        object->name = NULL;
}

rbusProperty_t rbusObject_GetProperties(rbusObject_t object)
{
    if(!object)
        return NULL;
    return object->properties;
}

void rbusObject_SetProperties(rbusObject_t object, rbusProperty_t properties)
{
    VERIFY_NULL(object);
    if(object->properties)
        rbusProperty_Release(object->properties);
    object->properties = properties;
    if(object->properties)
        rbusProperty_Retain(object->properties);
}

rbusProperty_t rbusObject_GetProperty(rbusObject_t object, char const* name)
{
    if(!object)
        return NULL;
    rbusProperty_t prop = object->properties;
    while(prop && strcmp(rbusProperty_GetName(prop), name))
        prop = rbusProperty_GetNext(prop);
    return prop;
}

void rbusObject_SetProperty(rbusObject_t object, rbusProperty_t newProp)
{
    VERIFY_NULL(object);
    if(object->properties == NULL)
    {
        rbusObject_SetProperties(object, newProp);
    }
    else
    {
        rbusProperty_t oldProp = object->properties;
        rbusProperty_t lastProp = NULL;
        /*search for property by name*/
        while(oldProp && strcmp(rbusProperty_GetName(oldProp), rbusProperty_GetName(newProp)))
        {
            if(rbusProperty_GetNext(oldProp))
            {
                lastProp = oldProp;
                oldProp = rbusProperty_GetNext(oldProp);
            }
            else
            {
                lastProp = oldProp;
                oldProp = NULL;
            }
        }
        if(oldProp)/*existing property found by name*/
        {   /*replace oldProp with newProp, preserving the rest of the property list*/

            /*append all the properties after oldProp to the newProp*/
            rbusProperty_t next = rbusProperty_GetNext(oldProp);
            if(next)
            {
                rbusProperty_Append(newProp, next);/*newProp will retain the list*/
            }
            
            /*link newProp to the tail of all the properties that came before oldProp*/
            if(lastProp) 
            {
                /*this will release oldProp(which will release oldProp's next)
                  and retain newPorp(which retained the oldProp's next above)*/
                rbusProperty_SetNext(lastProp, newProp);
            }
            else
            {
                rbusObject_SetProperties(object, newProp);/*this will release oldProp and retain newProp*/
            }
        }
        else/*no existing property with that name*/
        {
            rbusProperty_SetNext(lastProp, newProp);/*this will retain property*/
        }
    }        
}

rbusValue_t rbusObject_GetValue(rbusObject_t object, char const* name)
{
    return rbusObject_GetPropertyValue(object, name);
}

rbusValue_t rbusObject_GetPropertyValue(rbusObject_t object, char const* name)
{
    rbusProperty_t prop;
    if(!object)
        return NULL;
    if(name)
        prop = rbusObject_GetProperty(object, name);
    else
        prop = object->properties;
    if(prop)
        return rbusProperty_GetValue(prop);
    else
        return NULL;
}

void rbusObject_SetValue(rbusObject_t object, char const* name, rbusValue_t value)
{
    rbusObject_SetPropertyValue(object, name, value);
}

void rbusObject_SetPropertyValue(rbusObject_t object, char const* name, rbusValue_t value)
{
    VERIFY_NULL(object);
    VERIFY_NULL(value);
    VERIFY_NULL(name);
    rbusProperty_t prop = rbusObject_GetProperty(object, name);
    if(prop)
    {
        rbusProperty_SetValue(prop, value);
    }
    else
    {
        rbusProperty_Init(&prop, name, value);
        if(object->properties == NULL)
        {
            rbusObject_SetProperties(object, prop);
        }
        else
        {
            rbusProperty_Append(object->properties, prop);
        }
        rbusProperty_Release(prop);
    }
}


rbusObject_t rbusObject_GetParent(rbusObject_t object)
{
    return object->parent;
}

void rbusObject_SetParent(rbusObject_t object, rbusObject_t parent)
{
    VERIFY_NULL(object);
    object->parent = parent;
    /*
    if(object->parent)
        rbusObject_Release(object->parent);
    object->parent = parent;
    if(object->parent)
        rbusObject_Retain(object->parent);
    */
}

rbusObject_t rbusObject_GetChildren(rbusObject_t object)
{
    if(!object)
        return NULL;
    return object->children;
}

void rbusObject_SetChildren(rbusObject_t object, rbusObject_t children)
{
    VERIFY_NULL(object);
    if(object->children)
        rbusObject_Release(object->children);
    object->children = children;
    if(object->children)
        rbusObject_Retain(object->children);
}

rbusObject_t rbusObject_GetNext(rbusObject_t object)
{
    if(!object)
        return NULL;
    return object->next;
}

void rbusObject_SetNext(rbusObject_t object, rbusObject_t next)
{
    VERIFY_NULL(object);
    if(object->next)
        rbusObject_Release(object->next);
    object->next = next;
    if(object->next)
        rbusObject_Retain(object->next);
}

rbusObjectType_t rbusObject_GetType(rbusObject_t object)
{
    return object->type;
}

void rbusObject_fwrite(rbusObject_t obj, int depth, FILE* fout)
{
    int i;
    rbusObject_t child;
    rbusProperty_t prop;
    VERIFY_NULL(obj);
    VERIFY_NULL(fout);
    for(i=0; i<depth; ++i)
        fprintf(fout, " ");
    fprintf(fout, "rbusObject name=%s\r\n", rbusObject_GetName(obj));
    prop = rbusObject_GetProperties(obj);
    VERIFY_NULL(prop);
    rbusProperty_fwrite(prop, depth+1, fout);
    child = rbusObject_GetChildren(obj);
    while(child)
    {
        rbusObject_fwrite(child, depth+1, fout);
        child = rbusObject_GetNext(child);
    }
}

#define DEFINE_OBJECT_PROPERTY_VALUE(T1, T2)\
rbusValueError_t rbusObject_GetProperty##T1(rbusObject_t object, char const* name, T2* pdata)\
{\
    rbusValue_t v = NULL;\
    rbusProperty_t p = rbusObject_GetProperty(object, name);\
    if(!p)\
        return RBUS_VALUE_ERROR_NOT_FOUND;\
    v = rbusProperty_GetValue(p);\
    if(v)\
        return rbusValue_Get##T1##Ex(v, pdata);\
    return RBUS_VALUE_ERROR_NULL;\
}\
void rbusObject_SetProperty##T1(rbusObject_t object, char const* name, T2 data)\
{\
    rbusValue_t v = rbusValue_Init##T1(data);\
    rbusObject_SetPropertyValue(object, name, v);\
    rbusValue_Release(v);\
}

DEFINE_OBJECT_PROPERTY_VALUE(Boolean,bool);
DEFINE_OBJECT_PROPERTY_VALUE(Char,char);
DEFINE_OBJECT_PROPERTY_VALUE(Byte,unsigned char);
DEFINE_OBJECT_PROPERTY_VALUE(Int8,int8_t);
DEFINE_OBJECT_PROPERTY_VALUE(UInt8,uint8_t);
DEFINE_OBJECT_PROPERTY_VALUE(Int16,int16_t);
DEFINE_OBJECT_PROPERTY_VALUE(UInt16,uint16_t);
DEFINE_OBJECT_PROPERTY_VALUE(Int32,int32_t);
DEFINE_OBJECT_PROPERTY_VALUE(UInt32,uint32_t);
DEFINE_OBJECT_PROPERTY_VALUE(Int64,int64_t);
DEFINE_OBJECT_PROPERTY_VALUE(UInt64,uint64_t);
DEFINE_OBJECT_PROPERTY_VALUE(Single,float);
DEFINE_OBJECT_PROPERTY_VALUE(Double,double);
DEFINE_OBJECT_PROPERTY_VALUE(Time,rbusDateTime_t const*);
DEFINE_OBJECT_PROPERTY_VALUE(Property,struct _rbusProperty*);
DEFINE_OBJECT_PROPERTY_VALUE(Object,struct _rbusObject*);

rbusValueError_t rbusObject_GetPropertyString(rbusObject_t object, char const* name, char const** pdata, int* len)
{
    rbusValue_t v = NULL;
    rbusProperty_t p = rbusObject_GetProperty(object, name);
    if(!p)
        return RBUS_VALUE_ERROR_NOT_FOUND;
    v = rbusProperty_GetValue(p);
    if(v)
        return rbusValue_GetStringEx(v, pdata, len);
    return RBUS_VALUE_ERROR_NULL;
}
void rbusObject_SetPropertyString(rbusObject_t object, char const* name, char const* data)
{
    rbusValue_t v = rbusValue_InitString(data);
    VERIFY_NULL(v);
    rbusObject_SetPropertyValue(object, name, v);
    rbusValue_Release(v);
}

rbusValueError_t rbusObject_GetPropertyBytes(rbusObject_t object, char const* name, uint8_t const** pdata, int* len)
{
    rbusValue_t v = NULL;
    rbusProperty_t p = rbusObject_GetProperty(object, name);
    if(!p)
        return RBUS_VALUE_ERROR_NOT_FOUND;
    v = rbusProperty_GetValue(p);
    if(v)
        return rbusValue_GetBytesEx(v, pdata, len);
    return RBUS_VALUE_ERROR_NULL;
}
void rbusObject_SetPropertyBytes(rbusObject_t object, char const* name, uint8_t const* data, int len)
{
    rbusValue_t v = rbusValue_InitBytes(data, len);
    VERIFY_NULL(v);
    rbusObject_SetPropertyValue(object, name, v);
    rbusValue_Release(v);
}
