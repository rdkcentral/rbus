#ifndef RBUS_OTEL_BRIDGE_H
#define RBUS_OTEL_BRIDGE_H

#ifdef __cplusplus
extern "C" {
#endif

#if defined(RBUS_OTEL_BRIDGE_IMPORT_WEAK) && defined(__GNUC__)
#define RBUS_OTEL_BRIDGE_OPTIONAL __attribute__((weak))
#else
#define RBUS_OTEL_BRIDGE_OPTIONAL
#endif

RBUS_OTEL_BRIDGE_OPTIONAL void rbus_otel_event_publish(
    const char* event_name,
    const char** trace_parent,
    const char** trace_state);

RBUS_OTEL_BRIDGE_OPTIONAL void* rbus_otel_event_receive_begin(
    const char* event_name,
    const char* trace_parent,
    const char* trace_state);

RBUS_OTEL_BRIDGE_OPTIONAL void rbus_otel_event_receive_end(void* receive_context);

#undef RBUS_OTEL_BRIDGE_OPTIONAL

#ifdef __cplusplus
}
#endif

#endif

