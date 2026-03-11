#define _GNU_SOURCE 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <assert.h>
#include <pthread.h>
#include <unistd.h>

/* Forward declarations of RBUS/RT headers before including .c */
#include "rbus.h"
#include "rbus_datamodel_notification.h"
#include "rbus_log.h"
#include <rtVector.h>
#include <rtTime.h>
#include <rtMemory.h>

/* Mock rtTime_Now to control time in tests */
uint64_t g_now_ms = 1000;
#define rtTime_Now mock_rtTime_Now
void mock_rtTime_Now(rtTime_t* t)
{
    t->tv_sec = g_now_ms / 1000;
    t->tv_nsec = (g_now_ms % 1000) * 1000000;
}

/* Internal property mock structure */
typedef struct _mockProp {
    char* name;
    struct _mockProp* next;
} mockProp_t;

/* Mock rbus core APIs */
#undef rbusEvent_Subscribe
#undef rbusEvent_Unsubscribe
#undef rbus_discoverComponentName
#undef rbus_getExt
#undef rbusProperty_GetName
#undef rbusProperty_GetValue
#undef rbusProperty_GetNext
#undef rbusProperty_Release
#undef rbusValue_Retain
#undef rbusValue_Release
#undef rbusValue_GetType
#undef rbusValue_GetString
#undef rbusFilter_Retain
#undef rbusFilter_Release
#undef rbusFilter_Apply
#undef rbusObject_GetValue
#undef rbusObject_Release

rbusError_t rbusEvent_Subscribe(rbusHandle_t handle, char const* eventName, rbusEventHandler_t handler, void* userData, int timeout)
{ (void)handle; (void)eventName; (void)handler; (void)userData; (void)timeout; return RBUS_ERROR_SUCCESS; }

rbusError_t rbusEvent_Unsubscribe(rbusHandle_t handle, char const* eventName)
{ (void)handle; (void)eventName; return RBUS_ERROR_SUCCESS; }

rbusError_t rbus_discoverComponentName(rbusHandle_t handle, int numElements, char const** elements, int* numComponents, char*** components)
{
    (void)handle; (void)numElements; (void)elements;
    if(strcmp(elements[0], "Device.Test") == 0) {
        *numComponents = 1;
        *components = malloc(sizeof(char*));
        (*components)[0] = strdup("SampleProvider");
        return RBUS_ERROR_SUCCESS;
    }
    *numComponents = 0;
    *components = NULL;
    return RBUS_ERROR_SUCCESS;
}

rbusError_t rbus_getExt(rbusHandle_t handle, int numValues, char const** paths, int* numProps, rbusProperty_t* props)
{
    (void)handle; (void)numValues; (void)paths;
    if(numValues > 0 && strcmp(paths[0], "Device.Test.*") == 0) {
        *numProps = 2;
        mockProp_t* p1 = calloc(1, sizeof(mockProp_t));
        p1->name = strdup("Device.Test.P1");
        mockProp_t* p2 = calloc(1, sizeof(mockProp_t));
        p2->name = strdup("Device.Test.P2");
        p1->next = p2;
        *props = (rbusProperty_t)p1;
        return RBUS_ERROR_SUCCESS;
    }
    *numProps = 0;
    *props = NULL;
    return RBUS_ERROR_SUCCESS;
}

char const* rbusProperty_GetName(rbusProperty_t p) { return ((mockProp_t*)p)->name; }
rbusValue_t rbusProperty_GetValue(rbusProperty_t p) { (void)p; return (rbusValue_t)0x5555; }
rbusProperty_t rbusProperty_GetNext(rbusProperty_t p) { return (rbusProperty_t)((mockProp_t*)p)->next; }
void rbusProperty_Release(rbusProperty_t p) {
    mockProp_t* curr = (mockProp_t*)p;
    while(curr) {
        mockProp_t* next = curr->next;
        free(curr->name);
        free(curr);
        curr = next;
    }
}
void rbusValue_Retain(rbusValue_t v) { (void)v; }
void rbusValue_Release(rbusValue_t v) { (void)v; }
rbusValueType_t rbusValue_GetType(rbusValue_t v) { (void)v; return RBUS_STRING; }
char const* rbusValue_GetString(rbusValue_t v, int* len) { (void)v; if(len) *len=0; return ""; }
void rbusFilter_Retain(rbusFilter_t f) { (void)f; }
void rbusFilter_Release(rbusFilter_t f) { (void)f; }
bool rbusFilter_Apply(rbusFilter_t f, rbusValue_t v) { (void)f; (void)v; return true; }
rbusValue_t rbusObject_GetValue(rbusObject_t o, char const* name) {
    (void)o;
    if(strcmp(name, "path") == 0) return (rbusValue_t)0x6666;
    return NULL;
}
void rbusObject_Release(rbusObject_t o) { (void)o; }

/* Include source */
#include "../../src/rbus/rbus_datamodel_notification.c"

/* --- Test Cases --- */

static int g_batch_count = 0;
static void my_batch_handler(rbusHandle_t h, rbusDataModelNotificationEventBatch_t const* batch, void* userData)
{
    (void)h; (void)userData;
    printf("Received batch with %zu events\n", batch->count);
    g_batch_count += (int)batch->count;
}

void test_full_lifecycle()
{
    printf("Running test_full_lifecycle...\n");
    rbusDataModelNotificationManager_t mgr;
    void* fakeRbusHandle = (void*)0x1234;
    assert(rbusDataModelNotificationManager_Create(fakeRbusHandle, &mgr) == RBUS_ERROR_SUCCESS);
    
    // Test 1: Subscribe with Initial State
    rbusDataModelNotificationRequest_t req = {0};
    req.pattern = "Device.Test.*";
    req.initialState = true;
    req.batching.batchWindowMs = 50;
    req.batchHandler = my_batch_handler;
    
    rbusDataModelNotificationHandle_t h;
    g_batch_count = 0;
    assert(rbusDataModelNotificationManager_Subscribe(mgr, &req, &h) == RBUS_ERROR_SUCCESS);
    // Initial state should have queued 2 events (P1, P2)
    dmSub_t* sub = dmFindSubByHandle(mgr, h);
    assert(sub != NULL);
    assert(rtVector_Size(sub->queue) == 2);

    // Test 2: List and stats
    rbusDataModelNotificationList_t list;
    assert(rbusDataModelNotificationManager_List(mgr, &list) == RBUS_ERROR_SUCCESS);
    assert(list.count == 1);
    rbusDataModelNotificationManager_FreeList(&list);

    // Test 3: Event dispatcher (dmHandleEvent)
    rbusEvent_t evData = {0};
    evData.name = "SampleProvider._RBUS.DML!";
    evData.data = (rbusObject_t)0x8888; // dummy
    // We need to trigger a match. dmHandleEvent calls rbusObject_GetValue("path")
    // which our mock returns dummy string.
    // Actually our mock of rbusValue_GetString returns empty string.
    // Let's make it more realistic.
    
    // Check coverage of dmRbusEventHandler2
    rbusEventSubscription_t subData = {0};
    subData.userData = sub;
    dmRbusEventHandler2(fakeRbusHandle, &evData, &subData);

    // Test 4: Expiration Cleanup
    req.pattern = "Device.Expire.*";
    req.expirationSeconds = 1; // 1 second
    req.initialState = false;
    rbusDataModelNotificationHandle_t h2;
    assert(rbusDataModelNotificationManager_Subscribe(mgr, &req, &h2) == RBUS_ERROR_SUCCESS);
    assert(mgr->stats.activeSubscriptions == 2);
    
    // Warp time forward 2 seconds
    g_now_ms += 2000;
    // Wake up thread or manually call cleanup part
    // Since we want coverage, let's signal the thread
    pthread_cond_signal(&mgr->cond);
    usleep(100000); // Wait for thread to process
    
    // h2 should be gone
    assert(mgr->stats.activeSubscriptions == 1);

    // Test 5: Provider model changed hook
    rbusDataModelNotificationManager_OnProviderModelChanged(fakeRbusHandle, "Device.Test", true);

    assert(rbusDataModelNotificationManager_Unsubscribe(mgr, h) == RBUS_ERROR_SUCCESS);
    rbusDataModelNotificationManager_Destroy(mgr);
    printf("test_full_lifecycle passed!\n");
}

int main()
{
    test_full_lifecycle();
    printf("\nALL UNIT TESTS PASSED!\n");
    return 0;
}
