/*
 * If not stated otherwise in this file or this component's Licenses.txt file the
 * following copyright and licenses apply:
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

#include <stdint.h>
#include <stdbool.h>

/*sample hal*/

#define HAL_MAX_STR_LEN 100

typedef struct _HalSampleRecord
{
    bool boolVal;
    int32_t intVal;
    uint32_t ulongVal;
    char stringVal[HAL_MAX_STR_LEN];
} HalData, HalStaticRecord, HalWriteableRecord, HalDynamicRecord;

void Hal_Init();
void Hal_Unload();

void Hal_GetData(HalData* data);
void Hal_SetData(HalData* data);

int Hal_GetStaticRecordCount();
void Hal_GetStaticRecord(uint32_t id, HalStaticRecord* data);
void Hal_SetStaticRecord(uint32_t id, HalStaticRecord* data);

int Hal_GetWritableRecordCount();
void Hal_GetWritableRecord(uint32_t id, HalWriteableRecord* data);
void Hal_SetWritableRecord(uint32_t id, HalWriteableRecord* data);
uint32_t Hal_AddWritableRecord();
void Hal_DelWritableRecord(uint32_t id);

bool Hal_IsUpdatedDynamicRecords();
int Hal_GetDynamicRecordCount();
void Hal_GetDynamicRecord(uint32_t id, HalDynamicRecord* data);
