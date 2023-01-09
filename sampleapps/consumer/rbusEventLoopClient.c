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

static pthread_t main_thread_id;
static uint32_t property1_value = 100;

static void my_libev_dispatcher(EV_P_ ev_io *w, __attribute__((unused)) int revents)
{
  rbusHandle_t rbus = (rbusHandle_t) w->data;
  rbusHandle_Run(rbus);
}

void set_callback(rbusHandle_t rbus, rbusAsyncResponse_t res);
void get_callback(rbusHandle_t rbus, rbusAsyncResponse_t res);
void method_callback(rbusHandle_t rbus, rbusAsyncResponse_t res);
void on_value_changed(rbusHandle_t rbus, const rbusEvent_t* e, rbusEventSubscription_t* sub);

void on_timeout_get(EV_P_ ev_timer* w, __attribute__((unused)) int revents)
{
  rbusHandle_t rbus = (rbusHandle_t) w->data;
  rbusAutoPtr(rbusAsyncRequest) req = rbusAsyncRequest_New();
  rbusAsyncRequest_AddProperty(req, rbusProperty_InitInt32("Examples.Property1", 0));
  rbusAsyncRequest_SetCompletionHandler(req, get_callback);

  rbusError_t err = rbusProperty_GetAsync(rbus, req);
  if (err)
    abort();
}

void on_timeout_set(EV_P_ ev_timer* w, __attribute__((unused)) int revents)
{
  property1_value += 10;

  rbusHandle_t rbus = (rbusHandle_t) w->data;
  rbusAutoPtr(rbusAsyncRequest) req = rbusAsyncRequest_New();
  rbusAsyncRequest_AddProperty(req, rbusProperty_InitInt32("Examples.Property1", property1_value));
  rbusAsyncRequest_SetCompletionHandler(req, set_callback);

  rbusError_t err = rbusProperty_SetAsync(rbus, req);
  if (err)
    abort();
}

void on_timeout_invoke(EV_P_ ev_timer* w, __attribute__((unused)) int revents)
{
  int i;

  rbusHandle_t rbus = (rbusHandle_t) w->data;
  rbusAutoPtr(rbusAsyncRequest) req = rbusAsyncRequest_New();
  rbusAsyncRequest_SetCompletionHandler(req, method_callback);
  rbusAsyncRequest_SetMethodName(req, "org.rdk.Calculator.Sum()");

  rbusAutoPtr(rbusProperty) argv = rbusProperty_InitInt32("argv-000", 0);
  for (i = 1; i < 10; ++i) {
    char prop_name[64];
    snprintf(prop_name, sizeof(prop_name), "argv-%03d", i);
    rbusProperty_AppendInt32(argv, prop_name, i);
  }
  rbusAsyncRequest_SetMethodParameters(req, argv);

  rbusError_t err = rbusMethod_InvokeAsyncEx(rbus, req);
  if (err)
    abort();
}


int main(int argc, char* argv[])
{
  rbusOptions_t opts;
  rbusHandle_t  rbus;

  (void) argc;
  (void) argv;

  main_thread_id = pthread_self();

  opts.use_event_loop = true;
  opts.component_name = "event-loop-example";

  rbusHandle_New(&rbus, &opts);

  struct ev_loop *loop = EV_DEFAULT;

  ev_io rbus_watcher;
  ev_io_init(&rbus_watcher, &my_libev_dispatcher, rbusHandle_GetEventFD(rbus), EV_READ);
  rbus_watcher.data = rbus;
  ev_io_start(loop, &rbus_watcher);

  #if 1
  ev_timer timeout_get;
  ev_timer_init(&timeout_get, on_timeout_get, 1.0, 1.0);
  timeout_get.data = rbus;
  ev_timer_start(loop, &timeout_get);
  #endif

  #if 1
  ev_timer timeout_set;
  ev_timer_init(&timeout_set, on_timeout_set, .25, .25);
  timeout_set.data = rbus;
  ev_timer_start(loop, &timeout_set);
  #endif

  #if 1
  ev_timer timeout_invoke;
  ev_timer_init(&timeout_invoke, on_timeout_invoke, 1.0, 1.0);
  timeout_invoke.data = rbus;
  ev_timer_start(loop, &timeout_invoke);
  #endif

  ev_run(loop, 0);
  rbus_close(rbus);

  return 0;
}

void get_callback(rbusHandle_t rbus, rbusAsyncResponse_t res)
{
  (void) rbus;

  assert(main_thread_id == pthread_self());

  rbusError_t err = rbusAsyncResponse_GetStatus(res);
  if (err == RBUS_ERROR_SUCCESS) {
    rbusProperty_t prop = rbusAsyncResponse_GetProperty(res);
    printf("GET[%s] %s == %d\n", rbusError_ToString(err), rbusProperty_GetName(prop),
      rbusProperty_GetInt32(prop));
  }
  else {
    printf("GET[%s]\n", rbusError_ToString(err));
  }

  // ev_break(EV_DEFAULT, EVBREAK_ONE);
}


void set_callback(rbusHandle_t rbus, rbusAsyncResponse_t res)
{
  (void) rbus;

  assert(main_thread_id == pthread_self());

  rbusError_t err = rbusAsyncResponse_GetStatus(res);
  if (err == RBUS_ERROR_SUCCESS) {
    rbusProperty_t prop = rbusAsyncResponse_GetProperty(res);
    printf("SET[%s] %s == %d\n", rbusError_ToString(err), rbusProperty_GetName(prop),
      rbusProperty_GetInt32(prop));
  }
  else {
    printf("SET[%s]\n", rbusError_ToString(err));
  }

  // ev_break(EV_DEFAULT, EVBREAK_ONE);
}

void method_callback(rbusHandle_t rbus, rbusAsyncResponse_t res)
{
  (void) rbus;

  rbusError_t err = rbusAsyncResponse_GetStatus(res);
  if (err == RBUS_ERROR_SUCCESS) {
    rbusProperty_t sum = rbusAsyncResponse_GetProperty(res);
    printf("INVOKE[%s] %s == %d\n", rbusError_ToString(err), rbusProperty_GetName(sum),
      rbusProperty_GetInt32(sum));
  }
  else {
    printf("INVOKE[%s]\n", rbusError_ToString(err));
  }

  // ev_break(EV_DEFAULT, EVBREAK_ONE);
}

void on_value_changed(rbusHandle_t rbus, const rbusEvent_t* e, rbusEventSubscription_t* sub)
{
  (void) rbus;
  (void) sub;

  assert(main_thread_id == pthread_self());

  printf("EVENT:%s\n", e->name);
  rbusObject_fwrite(e->data, 2, stdout);
  printf("\n");
  printf("\n");
}
