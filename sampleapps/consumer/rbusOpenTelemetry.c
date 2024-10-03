/*
 * If not stated otherwise in this file or this component's Licenses.txt file
 * the following copyright and licenses apply:
 *
 * Copyright 2019 RDK Management
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
#include <rbus.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#define kElementName "Device.Foo"
#define RBUS_OPEN_TELEMETRY_DATA_MAX 512
void run_client();
void run_manual_client();
void run_server();

rbusError_t get_handler(rbusHandle_t rbus, rbusProperty_t prop, rbusGetHandlerOptions_t* opts);
rbusError_t set_handler(rbusHandle_t rbus, rbusProperty_t prop, rbusSetHandlerOptions_t* opts);
rbusError_t method_handler(rbusHandle_t rbus, char const* methodName, rbusObject_t inParams,
  rbusObject_t outParams, rbusMethodAsyncHandle_t asyncHandle);

int runtime = 20;
int main(int argc, char *argv[])
{
  (void) argv;

  if (argc > 1)
  {
      if (strcmp(argv[1], "manual") == 0)
      {
          run_manual_client();
      }
      else
      {
          run_client();
      }
  }
  else
      run_server();
  return 0;
}

void GetTraceContextFromUser(char *traceparent, char *tracestate)
{
    char buff[256];

    memset(buff, '\0', 256);
    printf("Enter traceparent:");
    if (scanf("%s", buff) == 1)
        snprintf(traceparent, RBUS_OPEN_TELEMETRY_DATA_MAX, "traceparent:%s", buff);
    else
        memset(traceparent, '\0', RBUS_OPEN_TELEMETRY_DATA_MAX);

    memset(buff, '\0', 256);
    printf("Enter tracestate:");
    if (scanf("%s", buff) == 1)
        snprintf(tracestate, RBUS_OPEN_TELEMETRY_DATA_MAX, "tracestate:%s", buff);
    else
        memset(tracestate, '\0', RBUS_OPEN_TELEMETRY_DATA_MAX);
}

void run_client()
{
  char          buff[256];
  rbusValue_t   val;
  rbusHandle_t  rbus;

  rbus_open(&rbus, "test-client");

  // sample get
  rbusHandle_SetTraceContextFromString(rbus, "traceparent: 00-foo-bar-012345",
    "tracestate: congo=t61rcWkgMzE");
  rbus_get(rbus, kElementName, &val);
  rbusValue_ToDebugString(val, buff, sizeof(buff));
  printf("%s:\n", kElementName);
  printf("  value       : %s\n", buff);


  // sample set
  rbusHandle_SetTraceContextFromString(rbus, "traceparent: 00-foo-bar-0123",
    "tracestate: congo=t61rcWkgMzE");
  rbusValue_SetString(val, "new-value");
  rbus_set(rbus, kElementName, val, NULL);

  rbusHandle_ClearTraceContext(rbus);

  // sample invoke
  rbusHandle_SetTraceContextFromString(rbus, "traceparent: 00-foo-bar-012345678",
    "tracestate: congo=t61rcWkgMzE");
  rbusObject_t in, out;
  rbusObject_Init(&in, NULL);
  rbusMethod_Invoke(rbus, kElementName, in, &out);

  rbusValue_Release(val);
  rbusObject_Release(in);
  rbusObject_Release(out);

  rbusHandle_ClearTraceContext(rbus);
  rbus_close(rbus);
}

void run_manual_client()
{
  char          buff[256];
  char traceparent[RBUS_OPEN_TELEMETRY_DATA_MAX];
  char tracestate[RBUS_OPEN_TELEMETRY_DATA_MAX];
  rbusValue_t   val;
  rbusHandle_t  rbus;

  rbus_open(&rbus, "test-client");

  // sample get
  printf("\n ======================================================== \n");
  printf(" Enter test inputs for 'rbus_get' method\n");
  printf(" ======================================================== \n");
  GetTraceContextFromUser(traceparent, tracestate);
  rbusHandle_SetTraceContextFromString(rbus, traceparent, tracestate);
  rbus_get(rbus, kElementName, &val);
  rbusValue_ToDebugString(val, buff, sizeof(buff));
  printf("%s:\n", kElementName);
  printf("  value       : %s\n", buff);

 // sample set
  printf("\n ======================================================== \n");
  printf(" Enter test inputs for 'rbus_set' method\n");
  printf(" ======================================================== \n");
  GetTraceContextFromUser(traceparent, tracestate);
  rbusHandle_SetTraceContextFromString(rbus, traceparent, tracestate);
  rbusValue_SetString(val, "new-value");
  rbus_set(rbus, kElementName, val, NULL);

  rbusHandle_ClearTraceContext(rbus);

  // sample invoke
  printf("\n ======================================================== \n");
  printf(" Enter test inputs for 'rbusMethod_Invoke' method\n");
  printf(" ======================================================== \n");
  GetTraceContextFromUser(traceparent, tracestate);
  rbusHandle_SetTraceContextFromString(rbus, traceparent, tracestate);
  rbusObject_t in, out;
  rbusObject_Init(&in, NULL);
  rbusMethod_Invoke(rbus, kElementName, in, &out);

  rbusValue_Release(val);
  rbusObject_Release(in);
  rbusObject_Release(out);

  rbusHandle_ClearTraceContext(rbus);
  rbus_close(rbus);
}
void run_server()
{
  rbusHandle_t rbus;
  rbusDataElement_t dataElements[1] = {
    {kElementName, RBUS_ELEMENT_TYPE_PROPERTY, {
      get_handler,
      set_handler,
      NULL, NULL, NULL,
      method_handler}}
  };

  rbus_open(&rbus, "test-server");
  rbus_regDataElements(rbus, 1, dataElements);

  // XXX: wasted thread
  while (runtime != 0) {
    sleep(1);
    runtime --;
  }

  rbus_unregDataElements(rbus, 1, dataElements);
  rbus_close(rbus);
}

rbusError_t get_handler(rbusHandle_t rbus, rbusProperty_t prop, rbusGetHandlerOptions_t* opts)
{
  (void) opts;
  rbusValue_t val = rbusValue_InitString("dummy-value");
  rbusProperty_SetValue(prop, val);
  rbusValue_Release(val);

  char traceParent[512];
  char traceState[512];

  rbusHandle_GetTraceContextAsString(rbus, traceParent, sizeof(traceParent),
    traceState, sizeof(traceState));

  printf("GET\n");
  printf("  traceParent : %s\n", traceParent);
  printf("  traceState  : %s\n", traceState);

  return RBUS_ERROR_SUCCESS;
}


rbusError_t set_handler(rbusHandle_t rbus, rbusProperty_t prop, rbusSetHandlerOptions_t* opts)
{
  (void) opts;
  (void) prop;

  char traceParent[512];
  char traceState[512];

  rbusHandle_GetTraceContextAsString(rbus, traceParent, sizeof(traceParent),
    traceState, sizeof(traceState));

  printf("SET\n");
  printf("  traceParent : %s\n", traceParent);
  printf("  traceState  : %s\n", traceState);

  return RBUS_ERROR_SUCCESS;
}

rbusError_t method_handler(rbusHandle_t rbus, char const* methodName, rbusObject_t inParams,
  rbusObject_t outParams, rbusMethodAsyncHandle_t asyncHandle)
{
  char traceParent[512];
  char traceState[512];

  (void) methodName;
  (void) inParams;
  (void) asyncHandle;

  rbusHandle_GetTraceContextAsString(rbus, traceParent, sizeof(traceParent),
    traceState, sizeof(traceState));

  printf("INVOKE\n");
  printf("  traceParent : %s\n", traceParent);
  printf("  traceState  : %s\n", traceState);

  rbusValue_t v;
  rbusValue_Init(&v);
  rbusValue_SetString(v, "Hello World");
  rbusObject_SetValue(outParams, "value", v);
  rbusValue_Release(v);

  return RBUS_ERROR_SUCCESS;
}
