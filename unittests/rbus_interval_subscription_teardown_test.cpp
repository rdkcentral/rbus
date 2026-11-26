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

/*
 * Regression tests for subscription teardown races introduced by publisher-initiated
 * and external-removal ownership paths in rbusInterval_RemoveSubscriptionRecord.
 * These tests verify that subscription and record cleanup occurs cleanly
 * even when overlapping with duration completion or concurrent removal.
 * 
 * Tests are validated with:
 * - No deadlock (timeout would fail).
 * - No crashes or memory corruption.
 * - ASAN/valgrind will catch double-free or use-after-free.
 */

#include "gtest/gtest.h"
#include <unistd.h>
#include <pthread.h>
#include <rbus.h>
#include "rbusProviderConsumer.h"

class IntervalSubscriptionTeardownTest : public ::testing::Test {
protected:
    rbusHandle_t provider_handle = NULL;
    rbusHandle_t consumer_handle = NULL;

    static void SetUpTestCase() {
        printf("IntervalSubscriptionTeardownTest: Set up\n");
    }

    static void TearDownTestCase() {
        printf("IntervalSubscriptionTeardownTest: Tear down\n");
    }

    virtual void SetUp() {
        ASSERT_EQ(rbus_open(&provider_handle, "TeardownTestProvider"), RBUS_ERROR_SUCCESS);
        ASSERT_EQ(rbus_open(&consumer_handle, "TeardownTestConsumer"), RBUS_ERROR_SUCCESS);
    }

    virtual void TearDown() {
        if (provider_handle)
            rbus_close(provider_handle);
        if (consumer_handle)
            rbus_close(consumer_handle);
    }
};

/*
 * Test: External removal during active publishing.
 * Subscribe to an interval-based property with SubscribeEx, let the publisher run, 
 * then unsubscribe with UnsubscribeEx.
 * Verifies:
 * - No deadlock (would timeout).
 * - Unsubscribe returns success.
 * - No use-after-free or double-free (detected by ASAN/valgrind).
 */
TEST_F(IntervalSubscriptionTeardownTest, ExternalRemovalDuringPublishing) {
    rbusSubscription_t sub = NULL;
    rbusProperty_t prop = NULL;

    prop = rbusProperty_InitString("Device.TestComponent.AutoPublishProperty", "initial");
    ASSERT_NE(prop, nullptr);

    sub = rbusSubscription_Init(&sub, "Device.TestComponent.AutoPublishProperty");
    ASSERT_NE(sub, nullptr);

    /* Subscribe with interval (2 seconds) and no duration limit */
    EXPECT_EQ(rbusEvent_SubscribeEx(consumer_handle, sub, 2, 0), RBUS_ERROR_SUCCESS);

    /* Allow publisher thread to start */
    sleep(1);

    /* External removal: unsubscribe while thread is running.
       This should not deadlock or crash. */
    EXPECT_EQ(rbusEvent_UnsubscribeEx(consumer_handle, sub, 1), RBUS_ERROR_SUCCESS);

    rbusSubscription_Release(sub);
    rbusProperty_Release(prop);
}

/*
 * Test: Overlapping external removal and duration completion.
 * Subscribe with interval and duration, allowing the publisher to complete
 * naturally while external removal races.
 * Verifies:
 * - No deadlock (would timeout).
 * - Cleanup completes cleanly.
 * - No double-free or use-after-free (ASAN/valgrind).
 */
TEST_F(IntervalSubscriptionTeardownTest, DurationCompletionWithConcurrentRemoval) {
    rbusSubscription_t sub = NULL;
    rbusProperty_t prop = NULL;

    prop = rbusProperty_InitString("Device.TestComponent.AutoPublishProperty", "initial");
    ASSERT_NE(prop, nullptr);

    sub = rbusSubscription_Init(&sub, "Device.TestComponent.AutoPublishProperty");
    ASSERT_NE(sub, nullptr);

    /* Subscribe with interval (1 second) and duration (2 seconds total) */
    EXPECT_EQ(rbusEvent_SubscribeEx(consumer_handle, sub, 1, 2), RBUS_ERROR_SUCCESS);

    /* Wait for duration to complete (publisher self-cleanup races with external removal) */
    sleep(3);

    /* Attempt external removal: may be a no-op if publisher already cleaned up,
       or may clean up if still pending. Either way should not deadlock. */
    rbusEvent_UnsubscribeEx(consumer_handle, sub, 1);

    rbusSubscription_Release(sub);
    rbusProperty_Release(prop);
}

/*
 * Test: Rapid subscribe/unsubscribe cycles.
 * Stress the ownership handoff between publisher and external remover
 * by cycling subscriptions multiple times.
 * Verifies:
 * - No deadlock in handle_subsMutex or gInternalSubMutex.
 * - No crashes during rapid transitions.
 * - ASAN/valgrind detects any leaks or corruption.
 */
TEST_F(IntervalSubscriptionTeardownTest, RapidCycleUnderContention) {
    rbusSubscription_t sub = NULL;
    rbusProperty_t prop = NULL;
    const int cycles = 5;

    for (int i = 0; i < cycles; ++i) {
        prop = rbusProperty_InitString("Device.TestComponent.AutoPublishProperty", "value");
        ASSERT_NE(prop, nullptr);

        sub = rbusSubscription_Init(&sub, "Device.TestComponent.AutoPublishProperty");
        ASSERT_NE(sub, nullptr);

        EXPECT_EQ(rbusEvent_SubscribeEx(consumer_handle, sub, 1, 0), RBUS_ERROR_SUCCESS);
        
        /* Brief overlap to stress lock contention */
        usleep(500 * 1000);  /* 500ms */

        EXPECT_EQ(rbusEvent_UnsubscribeEx(consumer_handle, sub, 1), RBUS_ERROR_SUCCESS);

        rbusSubscription_Release(sub);
        rbusProperty_Release(prop);

        /* Allow cleanup to settle before next cycle */
        usleep(100 * 1000);  /* 100ms */
    }
}

/*
 * Test: Verify no deadlock in mutex release ordering.
 * Subscribe and immediately unsubscribe to stress the mutex release path
 * where handle_subsMutex must be released before pthread_join.
 * Verifies:
 * - No deadlock (would timeout).
 * - Mutex ordering is correct.
 */
TEST_F(IntervalSubscriptionTeardownTest, NoMutexDeadlock) {
    rbusSubscription_t sub = NULL;
    rbusProperty_t prop = NULL;

    prop = rbusProperty_InitString("Device.TestComponent.AutoPublishProperty", "initial");
    ASSERT_NE(prop, nullptr);

    sub = rbusSubscription_Init(&sub, "Device.TestComponent.AutoPublishProperty");
    ASSERT_NE(sub, nullptr);

    EXPECT_EQ(rbusEvent_SubscribeEx(consumer_handle, sub, 1, 0), RBUS_ERROR_SUCCESS);
    sleep(1);

    /* This unsubscribe should not deadlock even if publisher is in a callback.
       If mutex ordering is wrong (join before releasing handle_subsMutex),
       this will hang and timeout. */
    EXPECT_EQ(rbusEvent_UnsubscribeEx(consumer_handle, sub, 1), RBUS_ERROR_SUCCESS);

    rbusSubscription_Release(sub);
    rbusProperty_Release(prop);
}
