#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <rbus.h>
#include <unistd.h>
#include <rtLog.h>
#include <string.h>

#define NUM_THREADS 4

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
    int thread_id = *(int *)arg;
    int rc;
    rbusHandle_t handle;
    char consumername[128] = {0};
    char datamodel[128] = {0};
    printf("Thread %d is running\n", thread_id);
    while(1)
    {
        snprintf(consumername, sizeof(consumername), "EventConsumerThread%d", thread_id);
        printf("consumername = %s\n", consumername);
        rc = rbus_open(&handle, consumername);
        if(rc != RBUS_ERROR_SUCCESS)
        {
            printf("consumer: rbus_open failed: %d\n", rc);
            return NULL;
        }
        snprintf(datamodel, sizeof(datamodel), "Device.Provider1.Event%d!", thread_id); 
        printf("datamodel = %s\n", datamodel);
        rc = rbusEvent_Subscribe(
                handle,
                datamodel,
                generalEvent1Handler,
                "UserTest",
                0);
        if(rc != RBUS_ERROR_SUCCESS)
        {
            printf("consumer: rbusEvent_Subscribe failed: %d\n", rc);
            return NULL;
        }
        sleep(1);
        rbusEvent_Unsubscribe(handle, datamodel);
    }
    return NULL;
}

int main() {
    pthread_t threads[NUM_THREADS];
    int thread_ids[NUM_THREADS];

    for (int i = 1; i <= NUM_THREADS; i++) {
        thread_ids[i] = i;
        int result = pthread_create(&threads[i], NULL, threadFunction, &thread_ids[i]);
        if (result != 0) {
            fprintf(stderr, "Error creating thread %d\n", i);
            return -1;
        }
        sleep(1);
    }

    for (int i = 1; i <= NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("All threads have finished\n");

    return 0;
}
