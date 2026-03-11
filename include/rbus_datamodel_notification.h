/*
 * If not stated otherwise in this file or this component's Licenses.txt file
 * the following copyright and licenses apply:
 *
 * Copyright 2026
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

#ifndef RBUS_DATAMODEL_NOTIFICATION_H
#define RBUS_DATAMODEL_NOTIFICATION_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

/**
 * @brief Global RBUS event topic for dynamic data model discovery signals.
 * Providers emit to _RBUS.DML.SIGNAL.DISCOVERY
 * NotifyDML manager subscribes to this topic for reactive discovery.
 */
#ifdef __cplusplus
extern "C" {
#endif

/* 
 * This header defines the INTERNAL/MANAGER API for NotifyDML.
 * The PUBLIC API and common types (rbusDataModelNotificationRequest_t etc.)
 * are defined in rbus.h.
 *
 * rbus.h includes this file at the end to expose these functions.
 */

typedef struct _rbusDataModelNotificationManager* rbusDataModelNotificationManager_t;

rbusError_t rbusDataModelNotificationManager_Create(
    rbusHandle_t handle,
    rbusDataModelNotificationManager_t* outMgr);

void rbusDataModelNotificationManager_Destroy(rbusDataModelNotificationManager_t mgr);

rbusError_t rbusDataModelNotificationManager_Subscribe(
    rbusDataModelNotificationManager_t mgr,
    rbusDataModelNotificationRequest_t const* req,
    rbusDataModelNotificationHandle_t* outHandle);

rbusError_t rbusDataModelNotificationManager_Unsubscribe(
    rbusDataModelNotificationManager_t mgr,
    rbusDataModelNotificationHandle_t subscriptionHandle);

rbusError_t rbusDataModelNotificationManager_List(
    rbusDataModelNotificationManager_t mgr,
    rbusDataModelNotificationList_t* outList);

void rbusDataModelNotificationManager_FreeList(rbusDataModelNotificationList_t* list);

rbusError_t rbusDataModelNotificationManager_GetStats(
    rbusDataModelNotificationManager_t mgr,
    rbusDataModelNotificationStats_t* outStats);

void rbusDataModelNotificationManager_OnProviderModelChanged(
    rbusHandle_t providerHandle,
    char const* changedPath,
    bool isRegistrationEvent);

#ifdef __cplusplus
}
#endif

#endif
