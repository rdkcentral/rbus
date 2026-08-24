/*
 * rbus_otel_test_publisher.c
 *
 * Standalone test for the PRODUCTION rbus <-> OTel bridge (librbus_otel_bridge.so).
 *
 * Unlike rbusOpenTelemetry.c (which is uninstrumented), this publisher creates a
 * real OTel span on the publishing thread via librdk_otlp. The preloaded bridge
 * then reads that span in rbus_otel_event_publish() and injects its W3C
 * traceparent into the event metadata. Run:
 *
 *   LD_PRELOAD=./librbus_otel_bridge.so ./rbus_otel_test_publisher
 *   LD_PRELOAD=./librbus_otel_bridge.so ./rbus_otel_test_subscriber
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

    /* librdk_otlp gates span creation on the RFC tracing flag; set it so this
       self-contained test can create spans without the full RFC path. */
    FILE* flag = fopen("/tmp/rdk_distributed_tracing_enabled", "w");
    if (flag) fclose(flag);

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

        /* Create an active span on THIS thread; the bridge reads it at publish. */
        rdk_otlp_start_distributed_trace(EVENT_ELEMENT, "publish");

        const char* tp = rdk_otlp_get_current_traceparent();
        printf("Publishing #%d  traceparent=%s\n", ++n, tp ? tp : "(none - is tracing enabled?)");
        fflush(stdout);

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

        rbusEvent_Publish(rbus, &event);   /* bridge injects traceparent here */

        rbusValue_Release(value);
        rbusObject_Release(data);

        rdk_otlp_finish_distributed_trace();
    }

    printf("\nShutting down publisher...\n");
    rbus_unregDataElements(rbus, 1, &el);
    rbus_close(rbus);
    rdk_otlp_shutdown();
    return 0;
}

