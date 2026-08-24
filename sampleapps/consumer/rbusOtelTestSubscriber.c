/*
 * rbus_otel_test_subscriber.c
 *
 * Consumer side of the production-bridge test. Subscribes to the publisher's
 * event and prints the trace context that arrived in the event metadata. rbus
 * core restores that context into TLS (rbus_setOpenTelemetryContext) before the
 * callback, so GetTraceContextAsString returns it regardless of the bridge; the
 * preloaded bridge additionally starts a child span from it.
 *
 *   LD_PRELOAD=./librbus_otel_bridge.so ./rbus_otel_test_subscriber
 *
 * The traceparent printed here should match the publisher's.
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>

#include <rbus.h>

#define EVENT_ELEMENT "Device.OtelTest.Event!"
#define TRACE_MAX 512

static volatile int g_running = 1;

static void on_signal(int sig) { (void)sig; g_running = 0; }

static void event_handler(rbusHandle_t handle, rbusEvent_t const* event,
    rbusEventSubscription_t* subscription)
{
    (void)subscription;

    rbusValue_t counter = rbusObject_GetValue(event->data, "counter");

    char traceParent[TRACE_MAX] = {0};
    char traceState[TRACE_MAX] = {0};
    rbusHandle_GetTraceContextAsString(handle,
        traceParent, sizeof(traceParent),
        traceState, sizeof(traceState));

    printf("EVENT %s  counter=%d\n", event->name,
        counter ? rbusValue_GetInt32(counter) : -1);
    printf("  traceparent=%s\n", traceParent);
    fflush(stdout);
}

int main(void)
{
    signal(SIGINT, on_signal);
    signal(SIGTERM, on_signal);

    rbusHandle_t rbus;
    if (rbus_open(&rbus, "OtelTestSubscriber") != RBUS_ERROR_SUCCESS)
    {
        printf("ERROR: rbus_open failed\n");
        return 1;
    }

    if (rbusEvent_Subscribe(rbus, EVENT_ELEMENT, event_handler, NULL, 0) != RBUS_ERROR_SUCCESS)
    {
        printf("ERROR: subscribe to %s failed\n", EVENT_ELEMENT);
        rbus_close(rbus);
        return 1;
    }

    printf("Subscribed to %s. Waiting for events (Ctrl+C to stop)...\n", EVENT_ELEMENT);
    fflush(stdout);

    while (g_running)
        sleep(1);

    printf("\nShutting down subscriber...\n");
    rbusEvent_Unsubscribe(rbus, EVENT_ELEMENT);
    rbus_close(rbus);
    return 0;
}


