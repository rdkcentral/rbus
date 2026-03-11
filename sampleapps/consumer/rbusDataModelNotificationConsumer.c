/*
 * Minimal NotifyDML sample consumer.
 *
 * Demonstrates:
 * - wildcard subscription
 * - future-path activation (via provider model-change events + RBUS retries)
 * - bulk delivery (batching + coalescing threshold)
 * - handle-based unsubscribe and list/stats
 */

#include <rbus.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static void onBatch(rbusHandle_t handle, rbusDataModelNotificationEventBatch_t const* batch, void* userData)
{
    (void)handle;
    (void)userData;
    printf("BATCH count=%zu\n", batch->count);
    for(size_t i=0;i<batch->count;i++)
    {
        rbusDataModelNotificationEvent_t const* ev = &batch->events[i];
        char const* t = "Unknown";
        if(ev->type == RBUS_DMLNOTIFY_VALUE_CHANGE) t = "ValueChange";
        else if(ev->type == RBUS_DMLNOTIFY_OBJECT_CREATION) t = "ObjectCreation";
        else if(ev->type == RBUS_DMLNOTIFY_OBJECT_DELETION) t = "ObjectDeletion";
        else if(ev->type == RBUS_DMLNOTIFY_STRUCTURAL_UPDATE) t = "StructuralUpdate";

        printf("  %s path=%s by=%s\n", t, ev->path ? ev->path : "(null)", ev->sourceComponent ? ev->sourceComponent : "(unknown)");
    }
}

int main(int argc, char** argv)
{
    (void)argc; (void)argv;
    rbusHandle_t handle;
    if(rbus_open(&handle, "dmlNotifyConsumer") != RBUS_ERROR_SUCCESS)
    {
        printf("rbus_open failed\n");
        return 1;
    }

    rbusDataModelNotificationRequest_t req;
    memset(&req, 0, sizeof(req));
    req.pattern = "Device.Test.Table.*.*"; /* Test deeper wildcard matching */
    req.scope = RBUS_DMLNOTIFY_SCOPE_EXACT;
    req.eventMask = RBUS_DMLNOTIFY_MASK_VALUE_CHANGE | RBUS_DMLNOTIFY_MASK_OBJECT_CREATION | RBUS_DMLNOTIFY_MASK_OBJECT_DELETION;
    req.initialState = true;
    req.batching.batchWindowMs = 500;
    req.batching.maxBatchSize = 100;
    req.batching.rateLimitPerSec = 1000;
    req.batching.coalesceThreshold = 10;
    req.batchHandler = onBatch;

    rbusDataModelNotificationHandle_t subHandle = 0;
    if(rbusDataModelNotification_Subscribe(handle, &req, &subHandle) != RBUS_ERROR_SUCCESS)
    {
        printf("Subscribe failed\n");
        rbus_close(handle);
        return 2;
    }

    printf("Subscribed handle=%llu\n", (unsigned long long)subHandle);

    for(int i=0;i<10;i++)
    {
        rbusDataModelNotificationStats_t stats;
        memset(&stats, 0, sizeof(stats));
        rbusDataModelNotification_GetStats(handle, &stats);
        printf("Stats active=%u delivered=%llu batched=%llu dropped=%llu\n",
               stats.activeSubscriptions,
               (unsigned long long)stats.notificationsDelivered,
               (unsigned long long)stats.notificationsBatched,
               (unsigned long long)stats.notificationsDropped);
        sleep(1);
    }

    rbusDataModelNotification_Unsubscribe(handle, subHandle);
    rbus_close(handle);
    return 0;
}

