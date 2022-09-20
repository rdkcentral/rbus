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

#include <stdlib.h>
#include <rtRetainable.h>
#include <rtMemory.h>
#include "rbus_log.h"
#include <assert.h>
#include "rbus_filter.h"
#include "rbus_buffer.h"

#define VERIFY_NULL(T) if(NULL == T){ return; }
typedef struct _rbusFilter_LogicExpression* rbusFilter_LogicExpression_t;

struct _rbusFilter_RelationExpression
{
    rbusFilter_RelationOperator_t op;
    rbusValue_t value;
};

struct _rbusFilter_LogicExpression
{
    rbusFilter_LogicOperator_t op;
    rbusFilter_t left;
    rbusFilter_t right;
};

struct _rbusFilter
{
    rtRetainable retainable;
    union
    {
        struct _rbusFilter_RelationExpression relation;
        struct _rbusFilter_LogicExpression logic;
    } e;
    rbusFilter_ExpressionType_t type;
};

void rbusFilter_InitRelation(rbusFilter_t* filter, rbusFilter_RelationOperator_t op, rbusValue_t value)
{
    (*filter) = rt_malloc(sizeof(struct _rbusFilter));

    (*filter)->type = RBUS_FILTER_EXPRESSION_RELATION;
    (*filter)->e.relation.op = op;
    (*filter)->e.relation.value = value;
    (*filter)->retainable.refCount = 1;
    if((*filter)->e.relation.value)
        rbusValue_Retain((*filter)->e.relation.value);
}

void rbusFilter_InitLogic(rbusFilter_t* filter, rbusFilter_LogicOperator_t op, rbusFilter_t left, rbusFilter_t right)
{
    (*filter) = rt_malloc(sizeof(struct _rbusFilter));

    (*filter)->type = RBUS_FILTER_EXPRESSION_LOGIC;
    (*filter)->e.logic.op = op;
    (*filter)->e.logic.left = left;
    (*filter)->e.logic.right = right;
    (*filter)->retainable.refCount = 1;

    if((*filter)->e.logic.left)
        rbusFilter_Retain((*filter)->e.logic.left);

    if((*filter)->e.logic.right)
        rbusFilter_Retain((*filter)->e.logic.right);
}

void rbusFilter_Destroy(rtRetainable* r)
{
    rbusFilter_t filter = (rbusFilter_t)r;
    VERIFY_NULL(filter);

    if(filter->type == RBUS_FILTER_EXPRESSION_RELATION)
    {
        if(filter->e.relation.value)
            rbusValue_Release(filter->e.relation.value);
    }
    else if(filter->type == RBUS_FILTER_EXPRESSION_LOGIC)
    {
        if(filter->e.logic.left)
            rbusFilter_Release(filter->e.logic.left);
        if(filter->e.logic.right)
            rbusFilter_Release(filter->e.logic.right);
    }
    free(filter);
}

void rbusFilter_Retain(rbusFilter_t filter)
{
    VERIFY_NULL(filter);
    rtRetainable_retain(filter);
}

void rbusFilter_Release(rbusFilter_t filter)
{
    VERIFY_NULL(filter);
    rtRetainable_release(filter, rbusFilter_Destroy);
}

bool rbusFilter_RelationApply(struct _rbusFilter_RelationExpression* ex, rbusValue_t value)
{
    if(!ex)
        return false;
    int c = rbusValue_Compare(value, ex->value);
    switch(ex->op)
    {
    case RBUS_FILTER_OPERATOR_GREATER_THAN:
        return c > 0;
    case RBUS_FILTER_OPERATOR_GREATER_THAN_OR_EQUAL:
        return c >= 0;
    case RBUS_FILTER_OPERATOR_LESS_THAN:
        return c < 0;
    case RBUS_FILTER_OPERATOR_LESS_THAN_OR_EQUAL:
        return c <= 0;
    case RBUS_FILTER_OPERATOR_EQUAL:
        return c == 0;
    case RBUS_FILTER_OPERATOR_NOT_EQUAL:
        return c != 0;
    default:
        return false;
    }
}

bool rbusFilter_LogicApply(struct _rbusFilter_LogicExpression* ex, rbusValue_t value)
{
    bool left = false, right = false;

    if(!ex)
        return false;
    left = rbusFilter_Apply(ex->left, value);

    if(ex->op != RBUS_FILTER_OPERATOR_NOT)
        right = rbusFilter_Apply(ex->right, value);

    if(ex->op == RBUS_FILTER_OPERATOR_AND)
    {
        return left && right;
    }
    else if(ex->op == RBUS_FILTER_OPERATOR_OR)
    {
        return left || right;
    }
    else if(ex->op == RBUS_FILTER_OPERATOR_NOT)
    {
        return !left;
    }
    return false;
}

bool rbusFilter_Apply(rbusFilter_t filter, rbusValue_t value)
{
    if(!filter)
        return false;
    if(filter->type == RBUS_FILTER_EXPRESSION_RELATION)
        return rbusFilter_RelationApply(&filter->e.relation, value);
    else if(filter->type == RBUS_FILTER_EXPRESSION_LOGIC)
        return rbusFilter_LogicApply(&filter->e.logic, value);
    else
        return false;
}

rbusFilter_ExpressionType_t rbusFilter_GetType(rbusFilter_t filter)
{
    if(!filter)
	return -1;
    return filter->type;
}

rbusFilter_RelationOperator_t rbusFilter_GetRelationOperator(rbusFilter_t filter)
{
    if(!filter)
	return -1;
    return filter->e.relation.op;
}

rbusValue_t rbusFilter_GetRelationValue(rbusFilter_t filter)
{
    if(!filter)
        return NULL;
    return filter->e.relation.value;
}

rbusFilter_LogicOperator_t rbusFilter_GetLogicOperator(rbusFilter_t filter)
{
    if(!filter)
	return -1;
    return filter->e.logic.op;
}

rbusFilter_t rbusFilter_GetLogicLeft(rbusFilter_t filter)
{
    if(!filter)
        return NULL;
    return filter->e.logic.left;
}

rbusFilter_t rbusFilter_GetLogicRight(rbusFilter_t filter)
{
    if(!filter)
        return NULL;
    return filter->e.logic.right;
}

void rbusFilter_Encode(rbusFilter_t filter, rbusBuffer_t buff)
{
    VERIFY_NULL(filter);
    rbusBuffer_WriteUInt32TLV(buff, filter->type);
    if(filter->type == RBUS_FILTER_EXPRESSION_RELATION)
    {
        rbusBuffer_WriteUInt32TLV(buff, filter->e.relation.op);
        rbusValue_Encode(filter->e.relation.value, buff);
    }
    else if(filter->type == RBUS_FILTER_EXPRESSION_LOGIC)
    {
        rbusBuffer_WriteUInt32TLV(buff, filter->e.logic.op);
        rbusFilter_Encode(filter->e.logic.left, buff);
        rbusFilter_Encode(filter->e.logic.right, buff);
    }
}

int rbusFilter_Decode(rbusFilter_t* filterOut, rbusBuffer_t const buff)
{
    uint16_t    type;
    uint16_t    length;
    rbusFilter_t filter;
    rbusFilter_ExpressionType_t filterType;
    rbusFilter_RelationOperator_t relOp;
    rbusFilter_LogicOperator_t logOp;
    rbusValue_t val;
    rbusFilter_t left, right;

    if(rbusBuffer_ReadUInt16(buff, &type) < 0)
        return -1;
    if(rbusBuffer_ReadUInt16(buff, &length) < 0)
        return -1;
    if(!(type == RBUS_UINT32 && length == 4))
    {
        RBUSLOG_WARN("rbusFilter_Decode failed");
        return -1;
    }
    if(rbusBuffer_ReadUInt32(buff, &filterType) < 0)
        return -1;

    if(filterType == RBUS_FILTER_EXPRESSION_RELATION)
    {
        if(rbusBuffer_ReadUInt16(buff, &type) < 0)
            return -1;
        if(rbusBuffer_ReadUInt16(buff, &length) < 0)
            return -1;
        if(!(type == RBUS_UINT32 && length == 4))
        {
            RBUSLOG_WARN("rbusFilter_Decode failed");
            return -1;
        }
        if(rbusBuffer_ReadUInt32(buff, &relOp) < 0)
            return -1;
        if(rbusValue_Decode(&val, buff) < 0)
            return -1;

        rbusFilter_InitRelation(&filter, relOp, val);
        rbusValue_Release(val);
    }
    else if(filterType == RBUS_FILTER_EXPRESSION_LOGIC)
    {
        if(rbusBuffer_ReadUInt16(buff, &type) < 0)
            return -1;
        if(rbusBuffer_ReadUInt16(buff, &length) < 0)
            return -1;
        if(!(type == RBUS_UINT32 && length == 4))
        {
            RBUSLOG_WARN("rbusFilter_Decode failed");
            return -1;
        }
        if(rbusBuffer_ReadUInt32(buff, &logOp) < 0)
            return -1;
        if(rbusFilter_Decode(&left, buff) < 0)
            return -1;
        if(rbusFilter_Decode(&right, buff) < 0)
            return -1;

        rbusFilter_InitLogic(&filter, logOp, left, right);
        rbusFilter_Release(left);
        rbusFilter_Release(right);
    }
    *filterOut = filter;
    return 0;
}

int rbusFilter_Compare(rbusFilter_t filter1, rbusFilter_t filter2)
{
    if(filter1 && filter2)
    {
        if(filter1->type == filter2->type)
        {
            if(filter1->type == RBUS_FILTER_EXPRESSION_RELATION)
            {
                if(filter1->e.relation.op == filter2->e.relation.op)
                    return rbusValue_Compare(filter1->e.relation.value, filter2->e.relation.value);
                else if(filter1->e.relation.op < filter2->e.relation.op)
                    return 1;
                else
                    return -1;
            }
            else /*RBUS_FILTER_EXPRESSION_LOGIC*/
            {
                if(filter1->e.logic.op == filter2->e.logic.op)
                {
                    int rc = rbusFilter_Compare(filter1->e.logic.left, filter2->e.logic.left);
                    if(rc != 0)
                        return rc;
                    return rbusFilter_Compare(filter1->e.logic.right, filter2->e.logic.right);
                }
                else if(filter1->e.logic.op < filter2->e.logic.op)
                    return 1;
                else
                    return -1;
            }
        }
        else if(filter1->type < filter2->type)
            return 1;
        else
            return -1;
    }
    else if(filter1)
        return 1;
    else if(filter2)
        return -1;
    else
        return 0;
}

void rbusFilter_fwrite(rbusFilter_t filter, FILE* fout, rbusValue_t value)
{
    VERIFY_NULL(filter);
    VERIFY_NULL(fout);
    if(filter->type == RBUS_FILTER_EXPRESSION_RELATION)
    {
        if(value)
            rbusValue_fwrite(value, 0, fout);
        else
            fwrite("X", 1, 1, fout);

        switch(filter->e.relation.op)
        {
        case RBUS_FILTER_OPERATOR_GREATER_THAN: fwrite(">",1,1,fout); break;
        case RBUS_FILTER_OPERATOR_GREATER_THAN_OR_EQUAL: fwrite(">=",1,2,fout); break;
        case RBUS_FILTER_OPERATOR_LESS_THAN: fwrite("<",1,1,fout); break;
        case RBUS_FILTER_OPERATOR_LESS_THAN_OR_EQUAL: fwrite("<=",1,2,fout); break;
        case RBUS_FILTER_OPERATOR_EQUAL: fwrite("==",1,2,fout); break;
        case RBUS_FILTER_OPERATOR_NOT_EQUAL: fwrite("!=",1,2,fout); break;
        };

        rbusValue_fwrite(filter->e.relation.value, 0, fout);
    }
    else if(filter->type == RBUS_FILTER_EXPRESSION_LOGIC)
    {
        if(filter->e.logic.op == RBUS_FILTER_OPERATOR_NOT)
        {
            fwrite("!", 1, 1, fout);
        }

        fwrite("(", 1, 1, fout);

        rbusFilter_fwrite(filter->e.logic.left, fout, value);

        if(filter->e.logic.op == RBUS_FILTER_OPERATOR_AND)
        {
            fwrite(" && ", 1, 4, fout);
        }
        else if(filter->e.logic.op == RBUS_FILTER_OPERATOR_OR)
        {
            fwrite(" || ", 1, 4, fout);
        }

        if(filter->e.logic.op != RBUS_FILTER_OPERATOR_NOT)
        {
            rbusFilter_fwrite(filter->e.logic.right, fout, value);
        }

        fwrite(")", 1, 1, fout);
    }
}

