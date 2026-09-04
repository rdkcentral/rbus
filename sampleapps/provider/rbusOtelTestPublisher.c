/*
 * rbus_otel_test_publisher.c
 *
 * Experiment: no-bridge design for the RBUS event-path OTel trace propagation.
 *
 * This publisher creates a real OTel span on the publishing thread via
 * librdk_otlp, then explicitly sets that span's W3C traceparent into rbus's
 * per-thread trace context with rbusHandle_SetTraceContextFromString() before
 * calling rbusEvent_Publish(). rbus_publishSubscriberEvent() just reads
 * whatever is in that context and serializes it into the event metadata -
 * there is no weak-hook/bridge indirection (librbus_otel_bridge.so) involved
 * and librbuscore.so never links against an OTEL library. Run:
 *
 *   ./rbus_otel_test_publisher
 *   ./rbus_otel_test_subscriber
 *
 * The traceparent printed here should match what the subscriber receives, and a
 * parent/child span pair should appear in Jaeger (if the collector is running).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>

#include <rbus.h>
#include "rdk_otlp_instrumentation.h"

#define EVENT_ELEMENT "Device.OtelTest.Event!"

static volatile int g_running = 1;
static int g_subscribed = 0;

static void on_signal(int sig) { (void)sig; g_running = 0; }

static rbusError_t sub_handler(rbusHandle_t handle, rbusEventSubAction_t action,
    const char* eventName, rbusFilter_t filter, int32_t interval, bool* autoPublish)
{
    (void)handle; (void)eventName; (void)filter; (void)interval;
    *autoPublish = false;
    g_subscribed = (action == RBUS_EVENT_ACTION_SUBSCRIBE);
    printf(">>> subscriber %s\n", g_subscribed ? "connected" : "disconnected");
    fflush(stdout);
    return RBUS_ERROR_SUCCESS;
}

int main(void)
{
    signal(SIGINT, on_signal);
    signal(SIGTERM, on_signal);

    rdk_otlp_init("rbus-otel-test-publisher", "1.0.0");

    rbusHandle_t rbus;
    if (rbus_open(&rbus, "OtelTestPublisher") != RBUS_ERROR_SUCCESS)
    {
        printf("ERROR: rbus_open failed\n");
        return 1;
    }

    rbusDataElement_t el = { EVENT_ELEMENT, RBUS_ELEMENT_TYPE_EVENT,
        { NULL, NULL, NULL, NULL, sub_handler, NULL } };
    if (rbus_regDataElements(rbus, 1, &el) != RBUS_ERROR_SUCCESS)
    {
        printf("ERROR: rbus_regDataElements failed\n");
        rbus_close(rbus);
        return 1;
    }

    printf("Publisher ready on %s. Waiting for subscriber...\n", EVENT_ELEMENT);
    fflush(stdout);

    int n = 0;
    while (g_running)
    {
        sleep(1);
        if (!g_subscribed)
            continue;

        /* Create an active span on THIS thread, then hand its traceparent to
         * rbus directly - no bridge/wrapper involved. */
        rdk_otlp_start_distributed_trace(EVENT_ELEMENT, "publish");

        const char* tp = rdk_otlp_get_current_traceparent();
        printf("Publishing #%d  traceparent=%s\n", ++n, tp ? tp : "(none - is tracing enabled?)");
        fflush(stdout);

        rbusHandle_SetTraceContextFromString(rbus, tp, NULL);

        rbusEvent_t event = {0};
        rbusObject_t data;
        rbusValue_t value;

        rbusValue_Init(&value);
        rbusValue_SetInt32(value, n);
        rbusObject_Init(&data, NULL);
        rbusObject_SetValue(data, "counter", value);

        event.name = EVENT_ELEMENT;
        event.data = data;
        event.type = RBUS_EVENT_GENERAL;

        rbusEvent_Publish(rbus, &event);   /* rbus reads traceparent straight from TLS */

        rbusValue_Release(value);
        rbusObject_Release(data);

        rbusHandle_ClearTraceContext(rbus);
        rdk_otlp_finish_distributed_trace();
    }

    printf("\nShutting down publisher...\n");
    rbus_unregDataElements(rbus, 1, &el);
    rbus_close(rbus);
    rdk_otlp_shutdown();
    return 0;
}

