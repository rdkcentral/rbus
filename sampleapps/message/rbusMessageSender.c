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
#include <rbus.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

int main()
{
    rbusError_t err;
    rbusHandle_t rbus;
    char topic[] = "A.B.C";
    char buff[256];
    rbusMessage_t msg;

    err = rbus_open(&rbus, "rbus_send");
    if (err) {
        fprintf(stderr, "rbus_open:%s\n", rbusError_ToString(err));
        return 0;
    }

    msg.topic = topic;
    msg.data = (uint8_t*)buff;
    
    msg.length = snprintf(buff, sizeof(buff), "%ld: Hello!", time(NULL));
    
    err = rbusMessage_Send(rbus, &msg, RBUS_MESSAGE_CONFIRM_RECEIPT);
    if (err)
    {
        fprintf(stderr, "rbusMessage_Send:%s\n", rbusError_ToString(err));
        goto cleanup;
    }

    sleep(1);
 
    msg.length = snprintf(buff, sizeof(buff), "%ld: Goodbye!", time(NULL));

    err = rbusMessage_Send(rbus, &msg, RBUS_MESSAGE_CONFIRM_RECEIPT);
    if (err)
    {
        fprintf(stderr, "rbusMessage_Send:%s\n", rbusError_ToString(err));
        goto cleanup;
    }

    cleanup:
    rbus_close(rbus);

    return 0;
}
