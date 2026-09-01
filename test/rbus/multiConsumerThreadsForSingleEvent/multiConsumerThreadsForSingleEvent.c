#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <rbus.h>
#include <unistd.h>
#include <rtLog.h>
#include <string.h>

#define NUM_THREADS 8

static void generalEvent1Handler(
    rbusHandle_t handle,
    rbusEvent_t const* event,
    rbusEventSubscription_t* subscription)
{
    printf("Consumer receiver General event 1 %s\n", event->name);

    printf("  My user data: %s\n", (char*)subscription->userData);
    (void)handle;
}

void *threadFunction(void *arg) {
    int instance = *(int *)arg;
    int rc;
    rbusHandle_t handle;
    char consumername[128] = {0};
    snprintf(consumername, sizeof(consumername), "EventConsumerThread%d", instance);
    printf("consumername = %s\n", consumername);
    rc = rbus_open(&handle, consumername);
    if(rc != RBUS_ERROR_SUCCESS)
    {
        printf("consumer: rbus_open failed: %d\n", rc);
        return NULL;
    }

    while(1)
    {
        rc = rbusEvent_Subscribe(
                handle,
                "Device.Provider1.Event1!",
                generalEvent1Handler,
                "UserTest",
                0);
        if(rc != RBUS_ERROR_SUCCESS)
        {
            printf("consumer: rbusEvent_Subscribe failed: %d\n", rc);
            return NULL;
        }
        usleep(250000);
        rbusEvent_Unsubscribe(handle, "Device.Provider1.Event1!");
    }
    return NULL;
}

int main() {
    pthread_t threads[NUM_THREADS];
    int i;

    for (i = 1; i <= NUM_THREADS; i++) {
        int result = pthread_create(&threads[i], NULL, threadFunction, &i);
        if (result != 0) {
            fprintf(stderr, "Error creating thread %d\n", i);
            return -1;
        }
        usleep(250000);
    }

    for (i = 1; i <= NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("All threads have finished\n");

    return 0;
}
