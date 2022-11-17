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

#define kElementName "Device.Foo"

void run_client();
void run_server();

rbusError_t get_handler(rbusHandle_t rbus, rbusProperty_t prop, rbusGetHandlerOptions_t* opts);
rbusError_t set_handler(rbusHandle_t rbus, rbusProperty_t prop, rbusSetHandlerOptions_t* opts);
rbusError_t method_handler(rbusHandle_t rbus, char const* methodName, rbusObject_t inParams,
  rbusObject_t outParams, rbusMethodAsyncHandle_t asyncHandle);

int main(int argc, char *argv[])
{
  (void) argv;

  if (argc > 1)
      run_server();
  else
      run_client();
  return 0;
}


void run_client()
{
  char          buff[256];
  rbusValue_t   val;
  rbusHandle_t  rbus;

  rbus_open(&rbus, "test-client");

  // sample get
  rbusHandle_SetTraceContextFromString(rbus, "traceparent: 00-foo-bar-01",
    "tracestate: congo=t61rcWkgMzE");
  rbus_get(rbus, kElementName, &val);
  rbusValue_ToDebugString(val, buff, sizeof(buff));
  printf("%s:\n", kElementName);
  printf("  value       : %s\n", buff);


  // sample set
  rbusHandle_SetTraceContextFromString(rbus, "traceparent: 00-foo-bar-02",
    "tracestate: congo=t61rcWkgMzE");
  rbusValue_SetString(val, "new-value");
  rbus_set(rbus, kElementName, val, NULL);

  // sample invoke
  rbusHandle_SetTraceContextFromString(rbus, "traceparent: 00-foo-bar-03",
    "tracestate: congo=t61rcWkgMzE");
  rbusObject_t in, out;
  rbusObject_Init(&in, NULL);
  rbusMethod_Invoke(rbus, kElementName, in, &out);


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
  while (1) {
    sleep(5);
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
