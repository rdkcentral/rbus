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
#include <rbus_session_mgr.h>
#include "rbus.h"
#include "rtLog.h"

static int g_current_session_id = 0;
rbusHandle_t g_handle;
void create_session()
{
    rbusObject_t outParams = NULL;
    if (RBUS_ERROR_SUCCESS == rbusMethod_Invoke(g_handle, RBUS_SMGR_METHOD_REQUEST_SESSION_ID, NULL, &outParams))
    {
        rbusProperty_t prop = NULL;
        prop = rbusObject_GetProperties(outParams);
        int result = rbusValue_GetInt32(rbusProperty_GetValue(prop));
        if(RBUS_ERROR_SUCCESS != result)
        {
            printf("Session manager reports internal error %d.\n", result);
            return;
        }
        prop = rbusProperty_GetNext(prop);
        if (prop)
        {
            g_current_session_id = rbusValue_GetInt32(rbusProperty_GetValue(prop));
            printf("Got new session id %d\n", g_current_session_id);
        }
        else
            printf("Malformed response from session manager.\n");
    }
    else
        printf("RPC with session manager failed.\n");
}

void print_current_session_id()
{
    rbusObject_t outParams = NULL;
    if (RBUS_ERROR_SUCCESS == rbusMethod_Invoke(g_handle, RBUS_SMGR_METHOD_GET_CURRENT_SESSION_ID, NULL, &outParams))
    {
        rbusProperty_t prop = NULL;
        prop = rbusObject_GetProperties(outParams);
        int result = rbusValue_GetInt32(rbusProperty_GetValue(prop));
        if(RBUS_ERROR_SUCCESS != result)
        {
            printf("Session manager reports internal error %d.\n", result);
            return;
        }
        prop = rbusProperty_GetNext(prop);
        /* Get current session id*/
        if (prop)
        {
            g_current_session_id = rbusValue_GetInt32(rbusProperty_GetValue(prop));
            printf("Current session id %d\n", g_current_session_id);
        }
        else
            printf("Malformed response from session manager.\n");
    }
    else
        printf("RPC with session manager failed.\n");
}

void end_session(int session)
{
    rbusObject_t inParams = NULL, outParams = NULL;
    rbusObject_Init(&inParams, NULL);
    rbusValue_t sessionValue;
    rbusValue_Init(&sessionValue);
    rbusValue_SetInt32(sessionValue, session);
    rbusObject_SetValue(inParams, "session", sessionValue);
    if (RBUS_ERROR_SUCCESS == rbusMethod_Invoke(g_handle, RBUS_SMGR_METHOD_END_SESSION, inParams, &outParams))
    {
        rbusProperty_t prop = NULL;
        prop = rbusObject_GetProperties(outParams);
        int result = rbusValue_GetInt32(rbusProperty_GetValue(prop));
        if(RBUS_ERROR_SUCCESS != result)
        {
            printf("Session manager reports internal error %d.\n", result);
            return;
        }
        else
            printf("Successfully ended session %d.\n", session);
    }
    else
        printf("RPC with session manager failed.\n");
}

int main(int argc, char *argv[])
{
    (void) argc;
    (void) argv;
    rtLog_SetLevel(RT_LOG_INFO);
    int err = RBUS_ERROR_SUCCESS;

    if ((err = rbus_open(&g_handle, "rbus_smgr_client")) == RBUS_ERROR_SUCCESS)
    {
        printf("Successfully connected to bus.\n");
    }

    create_session();
    print_current_session_id();
    create_session();//Negative test case.
    end_session(g_current_session_id + 1); //Negative test case.
    end_session(g_current_session_id);
    create_session();
    print_current_session_id();
    end_session(g_current_session_id);

    rbus_close(g_handle);
    return 0;
}