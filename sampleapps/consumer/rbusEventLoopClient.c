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

#if 0
static void my_libev_dispatcher(EV_P_ ev_io *w, __attribute__((unused)) int revents)
{
  rbusHandle_t rbus = (rbusHandle_t) w->data;
  rbusHandle_RunOne(rbus, 0);

  // or you want to drain queue
  // while (rbusHandle_RunOne(rbus, 0) == RBUS_ERROR_SUCCESS)
  //   ;
}
#endif

static void set_callback(rbusHandle_t rbus, rbusError_t err, rbusProperty_t prop, void* argp);
static void get_callback(rbusHandle_t rbus, rbusError_t err, rbusProperty_t value, void* argp);

int main(int argc, char* argv[])
{
  rbusOptions_t opts;
  rbusHandle_t  rbus;
  
  (void) argc;
  (void) argv;

  opts.use_event_loop = false;
  opts.component_name = "event-loop-example";

  rbusHandle_New(&rbus, &opts);

  #if 0
  struct ev_loop *loop = EV_DEFAULT;
  ev_io rbus_watcher;
  ev_io_init(&rbus_watcher, &my_libev_dispatcher, rbusHandle_GetEventFD(rbus), EV_READ);
  rbus_watcher.data = rbus;
  ev_io_start(loop, &rbus_watcher);

  // send get request to kick-start

  rbusProperty_t prop = rbusProperty_InitInt32("Device.Foo", 0);
  rbusProperty_GetAsync(rbus, prop, -1, &get_callback, NULL);
  rbusProperty_Release(prop);

  while (true)
    ev_run(loop, 0);
  #endif

  rbusValue_t val;
  rbusValue_Init(&val);

  while (true) {
    rbus_get(rbus, "Device.Foo", &val);

    printf("GET:%s == %d\n", "Devce.Foo", rbusValue_GetInt32(val));

    sleep(1);

    rbusValue_SetInt32(val, rbusValue_GetInt32(val) + 1);

    rbusSetOptions_t opts = { false, 0 };
    rbus_set(rbus, "Device.Foo", val, &opts);
  }

  rbus_close(rbus);

  return 0;
}


void get_callback(rbusHandle_t rbus, rbusError_t err, rbusProperty_t prop, void* argp)
{
  (void) argp;

  printf("GET[%s] %s == %d\n", rbusError_ToString(err), rbusProperty_GetName(prop),
    rbusProperty_GetInt32(prop));

  prop = rbusProperty_InitInt32( rbusProperty_GetName(prop), rbusProperty_GetInt32(prop) + 1 );

  rbusSetOptions_t set_opts = { false, 0 };
  rbusProperty_SetAsync(rbus, prop, &set_opts, -1, &set_callback, NULL);
  rbusProperty_Release(prop);
}


void set_callback(rbusHandle_t rbus, rbusError_t err, rbusProperty_t prop, void* argp)
{
  (void) argp;

  printf("SET[%s] %s == %d\n", rbusError_ToString(err), rbusProperty_GetName(prop),
    rbusProperty_GetInt32(prop));

  rbusProperty_GetAsync(rbus, prop, -1, &get_callback, NULL);
}
