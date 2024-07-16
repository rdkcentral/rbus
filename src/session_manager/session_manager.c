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
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include "rbus.h"
#include "rbuscore.h"
#include "rtLog.h"
#include "rbus_session_mgr.h"
#define CCSP_CURRENT_SESSION_ID_SIGNAL      "currentSessionIDSignal"

static int g_counter;
static int g_current_session_id;
rbusHandle_t   g_busHandle = 0;

/* MethodHandler */
static rbusError_t sessionManager_methodHandler(rbusHandle_t handle,
        char const* methodName, rbusObject_t inParams,
        rbusObject_t outParams, rbusMethodAsyncHandle_t asyncHandle)
{
    (void)handle;
    (void)asyncHandle;
    int rc = RBUS_ERROR_SUCCESS;

    /* Get Current Session id*/
    if (strcmp(methodName, RBUS_SMGR_METHOD_GET_CURRENT_SESSION_ID) == 0)
    {
        printf("Current session id is %d\n", g_current_session_id);
        rbusValue_t return_value, sessionid_value;
        rbusValue_Init(&return_value);
        rbusValue_Init(&sessionid_value);
        rbusValue_SetInt32(return_value, RBUSCORE_SUCCESS);
        rbusValue_SetInt32(sessionid_value, g_current_session_id);
        rbusObject_SetValue(outParams, "return_value", return_value);
        rbusObject_SetValue(outParams, "sessionid", sessionid_value);
        rbusValue_Release(return_value);
        rbusValue_Release(sessionid_value);
        return RBUS_ERROR_SUCCESS;
    }
    /* Request for session id*/
    else if (strcmp(methodName, RBUS_SMGR_METHOD_REQUEST_SESSION_ID) == 0)
    {
        if (0 == g_current_session_id)
        {
            g_current_session_id = 1;
            ++g_counter;
            printf("Creating new session %d\n", g_current_session_id);
            rbusValue_t return_value, sessionid_value;
            rbusValue_Init(&return_value);
            rbusValue_Init(&sessionid_value);
            rbusValue_SetInt32(return_value, RBUSCORE_SUCCESS);
            rbusValue_SetInt32(sessionid_value, g_current_session_id);
            rbusObject_SetValue(outParams, "return_value", return_value);
            rbusObject_SetValue(outParams, "sessionid", sessionid_value);
            rbusValue_Release(return_value);
            rbusValue_Release(sessionid_value);
        }
        else
        {
            printf("Cannot create new session when session %d is active.\n", g_current_session_id);
            rbusValue_t return_value;
            rbusValue_Init(&return_value);
            rbusValue_SetInt32(return_value, RBUSCORE_ERROR_INVALID_STATE);
            rbusObject_SetValue(outParams, "return_value", return_value);
            rbusValue_Release(return_value);
        }
        rbusEvent_t event = {0};
        rbusObject_t data;
        rbusValue_t return_value, sessionid_value;
        rbusValue_Init(&return_value);
        rbusValue_Init(&sessionid_value);
        rbusObject_Init(&data, NULL);
        rbusValue_SetInt32(return_value, RBUSCORE_SUCCESS);
        rbusValue_SetInt32(sessionid_value, g_current_session_id);
        rbusObject_SetValue(data, "return_value", return_value);
        rbusObject_SetValue(data, "sessionid", sessionid_value);

        event.name = CCSP_CURRENT_SESSION_ID_SIGNAL;
        event.data = data;
        event.type = RBUS_EVENT_GENERAL;

        rc = rbusEvent_Publish(handle, &event);

        if (rc != RBUS_ERROR_SUCCESS)
            printf("provider: rbusEvent_Publish Event failed: %d\n", rc);

        rbusValue_Release(return_value);
        rbusValue_Release(sessionid_value);
        rbusObject_Release(data);
    }
    /* End of the session */
    else if (strcmp(methodName, RBUS_SMGR_METHOD_END_SESSION) == 0)
    {
        rbusProperty_t prop = NULL;
        int sessionid = 0;
        int result = 0;
        prop = rbusObject_GetProperties(inParams);
        if ((prop) && (sessionid = rbusValue_GetInt32(rbusProperty_GetValue(prop))))
        {
            if(sessionid == g_current_session_id)
            {
                printf("End of session %d\n", g_current_session_id);
                g_current_session_id = 0;
                result = RBUS_ERROR_SUCCESS;
            }
            else
            {
                printf("Cannot end session %d. It doesn't match active session, which is %d.\n", sessionid, g_current_session_id);
                result = RBUS_ERROR_SESSION_ALREADY_EXIST;
            }
        }
        else
        {
            printf("Session id not found. Cannot process end of session.\n");
            result = RBUS_ERROR_INVALID_INPUT;
        }

        rbusValue_t resultValue;
        rbusValue_Init(&resultValue);
        rbusValue_SetInt32(resultValue, result);
        rbusObject_SetValue(outParams, "result", resultValue);
        rbusValue_Release(resultValue);

        rbusEvent_t event = {0};
        rbusObject_t data;
        rbusValue_t return_value, sessionid_value;

        rbusValue_Init(&return_value);
        rbusValue_Init(&sessionid_value);
        rbusObject_Init(&data, NULL);

        rbusValue_SetInt32(return_value, RBUS_ERROR_SUCCESS);
        rbusValue_SetInt32(sessionid_value, g_current_session_id);
        rbusObject_SetValue(data, "return_value", return_value);
        rbusObject_SetValue(data, "sessionid", sessionid_value);

        event.name = CCSP_CURRENT_SESSION_ID_SIGNAL;
        event.data = data;
        event.type = RBUS_EVENT_GENERAL;

        rc = rbusEvent_Publish(handle, &event);

        rbusValue_Release(return_value);
        rbusValue_Release(sessionid_value);
        rbusObject_Release(data);

        if (rc != RBUS_ERROR_SUCCESS)
            printf("provider: rbusEvent_Publish Event failed: %d\n", rc);
    }
    else
    {
        return RBUS_ERROR_BUS_ERROR;
    }
    return RBUS_ERROR_SUCCESS;
}

#if 0
static int callback(const char * destination, const char * method, rbusMessage message, void * user_data, rbusMessage *response, const rtMessageHeader* hdr)
{
    (void) user_data;
    (void) response;
    (void) destination;
    (void) method;
    (void) hdr;
    printf("Received message in base callback.\n");
    char* buff = NULL;
    uint32_t buff_length = 0;

    rbusMessage_ToDebugString(message, &buff, &buff_length);
    printf("%s\n", buff);
    free(buff);

    return 0;
}
#endif
/*Signal handler for closing broker connection*/
static void handle_signal(int sig)
{
    (void) sig;
    if (g_busHandle)
    {
        rbus_close(g_busHandle);
        g_busHandle = 0;
    }
    printf("rbus session manager exiting.\n");
    exit(0);
}

int main(int argc, char *argv[])
{
    (void) argc;
    (void) argv;
    printf("rbus session manager launching.\n");
    rtLog_SetLevel(RT_LOG_INFO);
    int rc = RBUS_ERROR_SUCCESS;
    char componentName[] = RBUS_SMGR_DESTINATION_NAME;
    if (argc == 2)
    {
        if (-1 == daemon(0 /*chdir to "/"*/, 1 /*redirect stdout/stderr to /dev/null*/ ))
        {
            rtLog_Fatal("failed to fork off daemon. %s", rtStrError(errno));
            exit(1);
        }
        signal(SIGTERM, handle_signal);
    }

    rbusDataElement_t dataElements[4] = {
        {CCSP_CURRENT_SESSION_ID_SIGNAL, RBUS_ELEMENT_TYPE_EVENT, {NULL, NULL, NULL, NULL, NULL, NULL}},
        {RBUS_SMGR_METHOD_GET_CURRENT_SESSION_ID, RBUS_ELEMENT_TYPE_METHOD, {NULL, NULL, NULL, NULL, NULL, sessionManager_methodHandler}},
        {RBUS_SMGR_METHOD_REQUEST_SESSION_ID, RBUS_ELEMENT_TYPE_METHOD, {NULL, NULL, NULL, NULL, NULL, sessionManager_methodHandler}},
        {RBUS_SMGR_METHOD_END_SESSION, RBUS_ELEMENT_TYPE_METHOD, {NULL, NULL, NULL, NULL, NULL, sessionManager_methodHandler}}
    };

    while(1)
    {
        if ((g_busHandle == NULL) && ((rc = rbus_open(&g_busHandle, componentName) == RBUS_ERROR_SUCCESS)))
        {
            printf("Successfully connected to bus.\n");
            break;
        }
        else
        {
            printf("Error in connecting to the bus.\n");
            sleep(10);
        }
    }

#if 0
    if ((rc = rbus_unregisterObj(RBUS_SMGR_DESTINATION_NAME)) == RBUSCORE_SUCCESS)
    {
        printf("Successfully Unregistered object.\n");
    }
    if ((rc = rbus_registerObj(RBUS_SMGR_DESTINATION_NAME, callback, NULL)) == RBUSCORE_SUCCESS)
    {
        printf("Successfully registered object.\n");
    }
#endif
    if ((rc = rbus_regDataElements(g_busHandle, 4, dataElements)) == RBUS_ERROR_SUCCESS)
        printf("Successfully registered Event.\n");

    pause();

    rbus_unregDataElements(g_busHandle, 4, dataElements);
    rbus_close(g_busHandle);
    g_busHandle = NULL;

    printf("rbus session manager exiting.\n");
    return 0;
}