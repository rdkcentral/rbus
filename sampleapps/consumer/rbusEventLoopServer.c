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

#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#include <rbus.h>
#include <rtList.h>

#include <ev.h>

static int32_t device_foo = 0;
static pthread_t main_thread_id;

rbusError_t get_handler(rbusHandle_t rbus, rbusProperty_t prop, rbusGetHandlerOptions_t* opts);
rbusError_t set_handler(rbusHandle_t rbus, rbusProperty_t prop, rbusSetHandlerOptions_t* opts);
rbusError_t method_handler(rbusHandle_t rbus, char const* methodName, rbusObject_t inParams,
  rbusObject_t outParams, rbusMethodAsyncHandle_t asyncHandle);
rbusError_t event_sub_handler(rbusHandle_t rbus, rbusEventSubAction_t action, const char* event_name, 
  rbusFilter_t filter, int32_t interval, bool* auto_publish);

static void my_libev_dispatcher(EV_P_ ev_io *w, __attribute__((unused)) int revents)
{
  rbusHandle_t rbus = (rbusHandle_t) w->data;
  rbusHandle_RunOne(rbus, 0);

  // or you want to drain queue
  // while (rbusHandle_RunOne(rbus, 0) == RBUS_ERROR_SUCCESS)
  //   ;
}

int main(int argc, char* argv[])
{

  (void) argc;
  (void) argv;

  rbusHandle_t rbus;
  rbusDataElement_t dataElements[2] = {
    {
      "Examples.Property1", RBUS_ELEMENT_TYPE_PROPERTY, {
        get_handler,        // get handler
        set_handler,        // set handler
        NULL,               // add row
        NULL,               // delete row
        event_sub_handler,  // event subscription notification
        NULL                // method handler
      }
    }, {
      "Device.Methods.SimpleMethod()", RBUS_ELEMENT_TYPE_METHOD, {
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        method_handler
      }
    }
  };

  main_thread_id = pthread_self();

  rbusOptions_t opts;
  opts.use_event_loop = true;
  opts.component_name = "event-loop-example";

  rbusHandle_New(&rbus, &opts);
  rbus_regDataElements(rbus, 2, dataElements);
  
  struct ev_loop *loop = EV_DEFAULT; 
   
  ev_io rbus_watcher;
  ev_io_init(&rbus_watcher, &my_libev_dispatcher, rbusHandle_GetEventFD(rbus), EV_READ);
  rbus_watcher.data = rbus;
  ev_io_start(loop, &rbus_watcher);


  while (true)
    ev_run(loop, 0);

  rbus_unregDataElements(rbus, 1, dataElements);
  rbus_close(rbus);
}


rbusError_t get_handler(rbusHandle_t rbus, rbusProperty_t prop, rbusGetHandlerOptions_t* opts)
{
  (void) rbus;
  (void) opts;

  assert( pthread_self() == main_thread_id );

  // update by 2 everytime someone calls get. This triggers any change-notify callbacks
  device_foo += 2;
  rbusProperty_SetInt32(prop, device_foo);

  printf("GET: %s == %d\n", rbusProperty_GetName(prop), device_foo);

  return RBUS_ERROR_SUCCESS;
}


rbusError_t set_handler(rbusHandle_t rbus, rbusProperty_t prop, rbusSetHandlerOptions_t* opts)
{
  (void) opts;
  (void) prop;
  (void) rbus;

  assert( pthread_self() == main_thread_id );

  printf("SET: %s == %d\n", rbusProperty_GetName(prop), device_foo);

  device_foo = rbusProperty_GetInt32(prop);

  return RBUS_ERROR_SUCCESS;
}

rbusError_t method_handler(rbusHandle_t rbus, char const* method_name, rbusObject_t argv,
  rbusObject_t args_out, rbusMethodAsyncHandle_t finished)
{
  (void) rbus;
  (void) method_name;
  (void) argv;
  (void) finished;

  assert( pthread_self() == main_thread_id );

  printf("INVOKE: %s\n", method_name);

  rbusValue_t v;
  rbusValue_Init(&v);
  rbusValue_SetString(v, "Hello World");
  rbusObject_SetValue(args_out, "value", v);
  rbusValue_Release(v);

  return RBUS_ERROR_SUCCESS;
}

rbusError_t event_sub_handler(rbusHandle_t rbus, rbusEventSubAction_t action, const char* event_name, 
  rbusFilter_t filter, int32_t interval, bool* auto_publish)
{
  (void) rbus;
  (void) action;
  (void) filter;
  (void) interval;

  assert( pthread_self() == main_thread_id );

  printf("EVENT_SUB:%s\n", event_name);

  *auto_publish = true;

  return RBUS_ERROR_SUCCESS;
}
