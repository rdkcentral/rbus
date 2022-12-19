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

#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#include <rbus.h>
#include <rtList.h>

#include <ev.h>

static pthread_t main_thread_id;

static void my_libev_dispatcher(EV_P_ ev_io *w, __attribute__((unused)) int revents)
{
  rbusHandle_t rbus = (rbusHandle_t) w->data;
  rbusHandle_RunOne(rbus, 0);

  // or you want to drain queue
  // while (rbusHandle_RunOne(rbus, 0) == RBUS_ERROR_SUCCESS)
  //   ;
}

void set_callback(rbusHandle_t rbus, rbusError_t err, rbusProperty_t prop, void* argp);
void get_callback(rbusHandle_t rbus, rbusError_t err, rbusProperty_t value, void* argp);
void on_value_changed(rbusHandle_t rbus, const rbusEvent_t* e, rbusEventSubscription_t* sub);

static void on_timeout(EV_P_ ev_timer* w, __attribute__((unused)) int revents)
{
  rbusHandle_t rbus = (rbusHandle_t) w->data;
  rbusProperty_t prop = rbusProperty_InitInt32("Examples.Property1", 1);
  rbusError_t err = rbusProperty_GetAsync(rbus, prop, -1, get_callback,  NULL);
  if (err)
    abort();
  rbusProperty_Release(prop);
}

int main(int argc, char* argv[])
{
  rbusOptions_t opts;
  rbusHandle_t  rbus;
  
  (void) argc;
  (void) argv;

  main_thread_id = pthread_self();

  opts.use_event_loop = false;
  opts.component_name = "event-loop-example";

  rbusHandle_New(&rbus, &opts);

  struct ev_loop *loop = EV_DEFAULT;

  ev_io rbus_watcher;
  ev_io_init(&rbus_watcher, &my_libev_dispatcher, rbusHandle_GetEventFD(rbus), EV_READ);
  rbus_watcher.data = rbus;
  ev_io_start(loop, &rbus_watcher);

  ev_timer timeout_watcher;
  ev_timer_init(&timeout_watcher, on_timeout, 1.0, 1.0);
  timeout_watcher.data = rbus;
  ev_timer_start(loop, &timeout_watcher);

  // rbusEvent_Subscribe(rbus, "Device.Provider1.Param1", on_value_changed, NULL, 0);
  // rbusProperty_SetAsync(rbus, prop, NULL, -1, set_callback, NULL);

  while (true)
    ev_run(loop, 0);

  rbus_close(rbus);

  return 0;
}


void get_callback(rbusHandle_t rbus, rbusError_t err, rbusProperty_t prop, void* argp)
{
  (void) argp;

  main_thread_id = pthread_self();

  if (err == RBUS_ERROR_SUCCESS) {
    printf("GET[%s] %s == %d\n", rbusError_ToString(err), rbusProperty_GetName(prop),
      rbusProperty_GetInt32(prop));
    prop = rbusProperty_InitInt32( rbusProperty_GetName(prop), rbusProperty_GetInt32(prop) + 1 );
  }
  else {
    printf("GET[%s]\n", rbusError_ToString(err));
  }

  rbusSetOptions_t set_opts = { false, 0 };
  rbusProperty_SetAsync(rbus, prop, &set_opts, -1, &set_callback, NULL);
  rbusProperty_Release(prop);
}


void set_callback(rbusHandle_t rbus, rbusError_t err, rbusProperty_t prop, void* argp)
{
  (void) argp;

  main_thread_id = pthread_self();

  printf("SET[%s] %s == %d\n", rbusError_ToString(err), rbusProperty_GetName(prop),
    rbusProperty_GetInt32(prop));

  rbusProperty_GetAsync(rbus, prop, -1, &get_callback, NULL);
}

void on_value_changed(rbusHandle_t rbus, const rbusEvent_t* e, rbusEventSubscription_t* sub)
{
  (void) rbus;
  (void) sub;

  main_thread_id = pthread_self();

  printf("EVENT:%s\n", e->name);
  rbusObject_fwrite(e->data, 2, stdout);
  printf("\n");
  printf("\n");
}
