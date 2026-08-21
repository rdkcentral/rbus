/**
 * Production rbus <-> OpenTelemetry bridge  (librbus_otel_bridge.so)
 *
 * Implements the three hooks that rbus core (librbuscore.so) declares as weak
 * symbols and calls on every event publish/receive. This library provides the
 * STRONG definitions that override those weak stubs, so rbus core itself stays
 * completely free of any OpenTelemetry dependency.
 *
 * All OTel work is delegated to librdk_otlp.so (the shared RDK tracer, the same
 * one Thunder JSON-RPC propagation uses), so spans created here share the same
 * tracer/exporter/collector configuration as the rest of the component.
 *
 *   publish : read the current thread's active span as a W3C traceparent and
 *             hand it back to rbus, which serializes it into the event metadata.
 *   receive : start a child span from the propagated traceparent, then end it
 *             after the subscriber callback returns.
 *
 * Deploy by preloading into any rbus component:
 *     LD_PRELOAD=/usr/lib/librbus_otel_bridge.so <component>
 * or by linking the component against it.
 */

#include <cstdlib>
#include <mutex>
#include <string>

#include "rdk_otlp_instrumentation.h"

/*
 * Canonical rbus hook contract (shipped by rbus). Included WITHOUT defining
 * RBUS_OTEL_BRIDGE_IMPORT_WEAK, so the declarations are plain (strong) and the
 * definitions below override rbuscore's weak stubs at load time.
 */
#include "rbus_otel_bridge.h"

namespace {

// Initialise the shared tracer exactly once per process. If the host component
// already called rdk_otlp_init(), set RBUS_OTEL_BRIDGE_NO_AUTOINIT=1 to skip.
std::once_flag g_init_once;

void ensure_tracer_initialised()
{
    std::call_once(g_init_once, [] {
        const char* skip = ::getenv("RBUS_OTEL_BRIDGE_NO_AUTOINIT");
        if (skip && skip[0] == '1')
            return;
        const char* name = ::getenv("OTEL_SERVICE_NAME");
        rdk_otlp_init((name && name[0]) ? name : "rbus", "1.0.0");
    });
}

// rbus uses the returned char* immediately (rbusMessage_SetString), so a
// thread_local buffer keeps it valid across the hook return without heap churn.
thread_local std::string g_publish_traceparent;

} // namespace

extern "C" void rbus_otel_event_publish(const char* event_name,
                                        const char** trace_parent,
                                        const char** trace_state)
{
    (void)event_name;
    (void)trace_state; // librdk_otlp models context via traceparent only;
                       // leave whatever rbus already had in TLS untouched.
    ensure_tracer_initialised();

    const char* tp = rdk_otlp_get_current_traceparent();
    if (tp && tp[0])
    {
        g_publish_traceparent.assign(tp);
        *trace_parent = g_publish_traceparent.c_str();
    }
    // else: no active span on this thread -> leave *trace_parent as the
    // existing TLS value so any explicitly-set context still propagates.
}

extern "C" void* rbus_otel_event_receive_begin(const char* event_name,
                                               const char* trace_parent,
                                               const char* trace_state)
{
    (void)trace_state;
    ensure_tracer_initialised();

    if (!trace_parent || !trace_parent[0])
        return nullptr; // nothing propagated -> no consumer span to start

    const char* span_name = (event_name && event_name[0]) ? event_name
                                                          : "rbus.event.receive";
    rdk_otlp_start_child_from_traceparent(trace_parent, span_name);

    // Non-null marker signals receive_end that a span was started on this thread.
    return reinterpret_cast<void*>(1);
}

extern "C" void rbus_otel_event_receive_end(void* receive_context)
{
    if (!receive_context)
        return;
    rdk_otlp_finish_child_span();
}

