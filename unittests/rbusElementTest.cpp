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

#include <rbus.h>
#include "rbus_element.h"
#include "rbus_filter.h"
#include "rbus_handle.h"
#include <cerrno>
#include <cstdlib>
#include <unistd.h>
#include "gtest/gtest.h"

static int isElementValid(elementNode* el, const char* expectedFullName)
{
    int pass = 0;

    if(el && expectedFullName)
        pass = !strcmp(el->fullName, expectedFullName);
    else
        pass = (expectedFullName) ? 0 : 1 ;

    return pass;
}

static int testRetrieveElement(elementNode* root, const char* query, const char* expectedFullName)
{
    return isElementValid(retrieveElement(root, query),expectedFullName);
}

static int testRetrieveInstanceElement(elementNode* root, const char* query, const char* expectedFullName)
{
    return isElementValid(retrieveInstanceElement(root, query),expectedFullName);
}

static void insertElem(elementNode* root, const char* path, rbusElementType_t type)
{
    rbusDataElement_t elem = {(char *)path, type, {NULL}};
    elementNode* ret=insertElement(root, &elem);
    EXPECT_NE(nullptr,ret);
}

static void removeElem(elementNode* root, const char* path)
{
    elementNode* node = retrieveElement(root, path);
    removeElement(node);
    EXPECT_EQ(testRetrieveElement(root, path, NULL),1);
}

static void addRow(elementNode* root, const char* path, uint32_t insNum, const char* alias)
{
    elementNode* node = retrieveInstanceElement(root, path);
    elementNode* ret=instantiateTableRow(node, insNum, alias);
    EXPECT_NE(nullptr,ret);
}

static void delRow(elementNode* root, const char* path)
{
    elementNode* node = retrieveInstanceElement(root, path);
    deleteTableRow(node);
    EXPECT_EQ(testRetrieveInstanceElement(root, path, NULL),1);
}

class rbusIntervalRowReplayTest : public ::testing::TestWithParam<const char*>
{
protected:
    char directory[64] = "/tmp/rbus-row-replay-XXXXXX";
    struct _rbusHandle handle = {};
    rbusSubscriptions_t subscriptions = NULL;

    void SetUp() override
    {
        ASSERT_NE(mkdtemp(directory), nullptr);
        handle.elementRoot = getEmptyElementNode();
        handle.elementRoot->name = strdup("root");
        handle.elementRoot->fullName = strdup("root");
        insertElem(handle.elementRoot, "Device.Replay.Table.{i}.", RBUS_ELEMENT_TYPE_TABLE);
        insertElem(handle.elementRoot, "Device.Replay.Table.{i}.Value", RBUS_ELEMENT_TYPE_PROPERTY);
        rbusSubscriptions_create(&subscriptions, &handle, "RowReplayTest",
                                 handle.elementRoot, directory);
        handle.subscriptions = subscriptions;
    }

    void TearDown() override
    {
        if(subscriptions)
            rbusSubscriptions_destroy(subscriptions);
        if(handle.elementRoot)
            freeElementNode(handle.elementRoot);
        char cache[128];
        snprintf(cache, sizeof(cache), "%s/rbus_subs_RowReplayTest", directory);
        if(unlink(cache) != 0)
            EXPECT_EQ(errno, ENOENT);
        if(rmdir(directory) != 0)
            EXPECT_EQ(errno, ENOENT);
    }
};

TEST_P(rbusIntervalRowReplayTest, preserves_active_subscription_on_row_recreation)
{
    elementNode* table = retrieveInstanceElement(handle.elementRoot, "Device.Replay.Table.");
    ASSERT_NE(table, nullptr);
    elementNode* row = instantiateTableRow(table, 1, "row");
    ASSERT_NE(row, nullptr);
    elementNode* instance = retrieveInstanceElement(handle.elementRoot, "Device.Replay.Table.1.Value");
    ASSERT_NE(instance, nullptr);
    rbusSubscription_t* sub = rbusSubscriptions_addSubscription(
        subscriptions, "RowReplayListener", GetParam(), 1, NULL, 1, 60, true, instance, false);
    if(sub == nullptr)
    {
        ADD_FAILURE() << "Failed to create the row replay subscription";
        return;
    }
    const uint32_t id = sub->subscriptionId;
    EXPECT_EQ(sub->element, retrieveElement(handle.elementRoot, GetParam()));

    for(int iteration = 0; iteration < 3; ++iteration)
    {
        rbusSubscriptions_onTableRowRemoved(subscriptions, row);
        deleteTableRow(row);
        size_t count = 0;
        ASSERT_EQ(rtList_GetSize(sub->instances, &count), RT_OK);
        EXPECT_EQ(count, 0u);

        row = instantiateTableRow(table, 1, "row");
        ASSERT_NE(row, nullptr);
        rbusSubscriptions_onTableRowAdded(subscriptions, row);
        rbusSubscriptions_resubscribeRowElementCache(&handle, subscriptions, row);
        EXPECT_EQ(rbusSubscriptions_getSubscription(
            subscriptions, "RowReplayListener", GetParam(), 1, NULL, 1, 60, false), sub);
        EXPECT_EQ(sub->subscriptionId, id);
        ASSERT_EQ(rtList_GetSize(sub->instances, &count), RT_OK);
        EXPECT_EQ(count, 1u);
        instance = retrieveInstanceElement(handle.elementRoot, "Device.Replay.Table.1.Value");
        ASSERT_NE(instance, nullptr);
        ASSERT_EQ(rtList_GetSize(instance->subscriptions, &count), RT_OK);
        EXPECT_EQ(count, 1u);
    }
}

INSTANTIATE_TEST_SUITE_P(IntervalSubscriptions, rbusIntervalRowReplayTest,
    ::testing::Values("Device.Replay.Table.1.Value", "Device.Replay.Table.[row].Value",
                      "Device.Replay.Table.*.Value"));

static void printRow(elementNode* root)
{
    char* buffer = NULL;
    size_t buffer_size = 0;
    FILE* fp = NULL;

    fp = open_memstream(&buffer, &buffer_size);
    if (fp){
        fprintRegisteredElements(fp, root, 0);
        fflush(fp);
        fclose(fp);
        free(buffer);
    }
}

TEST(rbusElementTest, testElement1)
{
    elementNode* root = getEmptyElementNode();
    root->name = strdup("root");
    root->fullName = strdup("root");

    //add row/insert elements
    insertElem(root, "Device.Foo.Table1.{i}.", RBUS_ELEMENT_TYPE_TABLE);
    insertElem(root, "Device.Foo.Prop1", RBUS_ELEMENT_TYPE_PROPERTY);
    insertElem(root, "Device.Foo.Table1.{i}.Prop1", RBUS_ELEMENT_TYPE_PROPERTY);
    addRow(root, "Device.Foo.Table1.", 1, NULL);
    addRow(root, "Device.Foo.Table1.", 2, NULL);
    printRow(root);

    //retrieve the added elements/rows
    EXPECT_EQ(testRetrieveElement(root, "Device", "Device"),1);
    EXPECT_EQ(testRetrieveElement(root, "Device.", "Device"),1);
    EXPECT_EQ(testRetrieveElement(root, "Device.Foo", "Device.Foo"),1);
    EXPECT_EQ(testRetrieveElement(root, "Device.Foo.Prop1", "Device.Foo.Prop1"),1);
    EXPECT_EQ(testRetrieveElement(root, "Device.Foo.Table1", "Device.Foo.Table1"),1);
    EXPECT_EQ(testRetrieveElement(root, "Device.Foo.Table1.", "Device.Foo.Table1"),1);
    EXPECT_EQ(testRetrieveElement(root, "Device.Foo.Table1.{i}", "Device.Foo.Table1.{i}"),1);
    EXPECT_EQ(testRetrieveElement(root, "Device.Foo.Table1.{i}.", "Device.Foo.Table1.{i}"),1);
    EXPECT_EQ(testRetrieveElement(root, "Device.Foo.Table1.{i}.Prop1", "Device.Foo.Table1.{i}.Prop1"),1);
    EXPECT_EQ(testRetrieveElement(root, "Device.Foo.Table1.1.Prop1", "Device.Foo.Table1.{i}.Prop1"),1);
    EXPECT_EQ(testRetrieveElement(root, "Device.Foo.Table1.2.Prop1", "Device.Foo.Table1.{i}.Prop1"),1);
    EXPECT_EQ(testRetrieveElement(root, "Device.Foo.Table1.3.Prop1", "Device.Foo.Table1.{i}.Prop1"),1);
    EXPECT_EQ(testRetrieveElement(root, "Device.Foo.Table1.4.Prop1", "Device.Foo.Table1.{i}.Prop1"),1);

    EXPECT_EQ(testRetrieveInstanceElement(root, "Device", "Device"),1);
    EXPECT_EQ(testRetrieveInstanceElement(root, "Device.", "Device"),1);
    EXPECT_EQ(testRetrieveInstanceElement(root, "Device.Foo", "Device.Foo"),1);
    EXPECT_EQ(testRetrieveInstanceElement(root, "Device.Foo.Prop1", "Device.Foo.Prop1"),1);
    EXPECT_EQ(testRetrieveInstanceElement(root, "Device.Foo.Table1", "Device.Foo.Table1"),1);
    EXPECT_EQ(testRetrieveInstanceElement(root, "Device.Foo.Table1.", "Device.Foo.Table1"),1);
    EXPECT_EQ(testRetrieveInstanceElement(root, "Device.Foo.Table1.1.Prop1", "Device.Foo.Table1.1.Prop1"),1);
    EXPECT_EQ(testRetrieveInstanceElement(root, "Device.Foo.Table1.2.Prop1", "Device.Foo.Table1.2.Prop1"),1);

    //deleting elements/row
    removeElem(root, "Device.Foo.Table1.{i}.Prop1");
    removeElem(root, "Device.Foo.Table1.{i}.");
    removeElem(root, "Device.Foo.Prop1");

    freeElementNode(root);
}

TEST(rbusElementTest, testElement2)
{
    elementNode* root = getEmptyElementNode();
    root->name = strdup("root");
    root->fullName = strdup("root");

    //add row/insert elements
    insertElem(root, "Device.Foo.Table1.{i}.", RBUS_ELEMENT_TYPE_TABLE);
    insertElem(root, "Device.Foo.Prop1", RBUS_ELEMENT_TYPE_PROPERTY);
    insertElem(root, "Device.Foo.Table1.{i}.Prop1", RBUS_ELEMENT_TYPE_PROPERTY);
    addRow(root, "Device.Foo.Table1.", 1, NULL);
    addRow(root, "Device.Foo.Table1.", 2, NULL);

    insertElem(root, "Device.Foo.Table1.{i}.Obj.Prop1", RBUS_ELEMENT_TYPE_PROPERTY);
    insertElem(root, "Device.Foo.Table1.{i}.Obj.Table2.{i}.", RBUS_ELEMENT_TYPE_TABLE);
    addRow(root, "Device.Foo.Table1.", 3, NULL);
    addRow(root, "Device.Foo.Table1.1.Obj.Table2.", 1, NULL);
    addRow(root, "Device.Foo.Table1.1.Obj.Table2.", 2, NULL);
    printRow(root);

    //retrieve the added elements/rows
    EXPECT_EQ(testRetrieveInstanceElement(root, "Device.Foo.Table1.1.Obj", "Device.Foo.Table1.1.Obj"),1);
    EXPECT_EQ(testRetrieveInstanceElement(root, "Device.Foo.Table1.2.Obj", "Device.Foo.Table1.2.Obj"),1);
    EXPECT_EQ(testRetrieveInstanceElement(root, "Device.Foo.Table1.1.Obj.Table2", "Device.Foo.Table1.1.Obj.Table2"),1);
    EXPECT_EQ(testRetrieveInstanceElement(root, "Device.Foo.Table1.1.Obj.Table2.", "Device.Foo.Table1.1.Obj.Table2"),1);
    EXPECT_EQ(testRetrieveInstanceElement(root, "Device.Foo.Table1.2.Obj.Table2", "Device.Foo.Table1.2.Obj.Table2"),1);

    delRow(root,"Device.Foo.Table1.1.Obj.Table2.2" );
    delRow(root,"Device.Foo.Table1.1.Obj.Table2.1" );
    delRow(root,"Device.Foo.Table1.3" );
    removeElem(root, "Device.Foo.Table1.{i}.Obj.Prop1");
    removeElem(root, "Device.Foo.Table1.{i}.Obj.Table2.{i}.");

    //deleting elements/row
    removeElem(root, "Device.Foo.Table1.{i}.Prop1");
    removeElem(root, "Device.Foo.Table1.{i}.");
    removeElem(root, "Device.Foo.Prop1");

    freeElementNode(root);

}

TEST(rbusElementTest, negtestElement1)
{
    elementNode* root = getEmptyElementNode();
    root->name = strdup("root");
    root->fullName = strdup("root");

    //add row/insert elements
    insertElem(root, "Device.Foo.Table1.{i}.", RBUS_ELEMENT_TYPE_TABLE);
    insertElem(root, "Device.Foo.Prop1", RBUS_ELEMENT_TYPE_PROPERTY);
    insertElem(root, "Device.Foo.Table1.{i}.Prop1", RBUS_ELEMENT_TYPE_PROPERTY);
    addRow(root, "Device.Foo.Table1.", 1, NULL);
    addRow(root, "Device.Foo.Table1.", 2, NULL);
    printRow(root);

    //retrieve random elements
    EXPECT_EQ(testRetrieveElement(root, "Device.", "Device"),1);
    EXPECT_EQ(testRetrieveElement(root, "Device.Prop2", "Device.Prop2"),0);
    EXPECT_EQ(testRetrieveElement(root, "Device.Foo.Prop2", "Device.Foo.Prop2"),0);
    EXPECT_EQ(testRetrieveElement(root, "Device.Foo.Table1", "Device.Foo.Table1"),1);
    EXPECT_EQ(testRetrieveElement(root, "Device.Foo.Table1.{i}.Prop2", "Device.Foo.Table1.{i}.Prop2"),0);

    EXPECT_EQ(testRetrieveInstanceElement(root, "Device.Prop2", "Device.Prop2"),0);
    EXPECT_EQ(testRetrieveInstanceElement(root, "Device.Foo.Table1.1.Prop2", "Device.Foo.Table1.1.Prop2"),0);
    EXPECT_EQ(testRetrieveInstanceElement(root, "Device.Foo.Table1.2.Prop2", "Device.Foo.Table1.2.Prop2"),0);
    EXPECT_EQ(testRetrieveInstanceElement(root, "Device.Foo.Table1.3.Prop2", "Device.Foo.Table1.3.Prop2"),0);
    EXPECT_EQ(testRetrieveInstanceElement(root, "Device.Foo.Table1.4.Prop2", "Device.Foo.Table1.4.Prop2"),0);

    //deleting elements/row
    removeElem(root, "Device.Foo.Table1.{i}.Prop1");
    removeElem(root, "Device.Foo.Table1.{i}.");
    removeElem(root, "Device.Foo.Prop1");

    freeElementNode(root);
}

TEST(rbusElementTest, negtestElement2)
{
    elementNode* root = getEmptyElementNode();
    root->name = strdup("root");
    root->fullName = strdup("root");

    //add row/insert elements
    insertElem(root, "Device.Foo.Table1.{i}.", RBUS_ELEMENT_TYPE_TABLE);
    insertElem(root, "Device.Foo.Prop1", RBUS_ELEMENT_TYPE_PROPERTY);
    insertElem(root, "Device.Foo.Table1.{i}.Prop1", RBUS_ELEMENT_TYPE_PROPERTY);
    addRow(root, "Device.Foo.Table1.", 1, NULL);
    addRow(root, "Device.Foo.Table1.", 2, NULL);
    printRow(root);

    //retrieving elements that is not created
    EXPECT_EQ(testRetrieveInstanceElement(root, "Device2", NULL),1);
    EXPECT_EQ(testRetrieveInstanceElement(root, "Device.Prop3", NULL),1);
    EXPECT_EQ(testRetrieveInstanceElement(root, "Device.Foo2", NULL),1);
    EXPECT_EQ(testRetrieveInstanceElement(root, "Device.Foo.Table1.5", NULL),1);
    EXPECT_EQ(testRetrieveInstanceElement(root, "Device.Foo.Table1.5.Prop1", NULL),1);
    EXPECT_EQ(testRetrieveInstanceElement(root, "Device.Foo.Table1.1.Prop4", NULL),1);

    removeElem(root, "Device.Foo.Table1.{i}.Prop1");
    removeElem(root, "Device.Foo.Table1.{i}.");
    removeElem(root, "Device.Foo.Prop1");

    freeElementNode(root);
}

TEST(rbusElementTest, negtestElement3)
{
    elementNode* root = getEmptyElementNode();
    root->name = strdup("root");
    root->fullName = strdup("root");

    //add row/insert elements
    insertElem(root, "Device.Foo.Table1.{i}.", RBUS_ELEMENT_TYPE_TABLE);
    insertElem(root, "Device.Foo.Prop1", RBUS_ELEMENT_TYPE_PROPERTY);
    insertElem(root, "Device.Foo.Table1.{i}.Prop1", RBUS_ELEMENT_TYPE_PROPERTY);
    addRow(root, "Device.Foo.Table1.", 1, NULL);
    addRow(root, "Device.Foo.Table1.", 2, NULL);
    printRow(root);

    //retrieve the added elements/rows
    EXPECT_EQ(testRetrieveElement(root, "Device", "Device"),1);
    EXPECT_EQ(testRetrieveElement(root, "Device.", "Device"),1);
    EXPECT_EQ(testRetrieveElement(root, "Device.Foo", "Device.Foo"),1);
    EXPECT_EQ(testRetrieveElement(root, "Device.Foo.Prop1", "Device.Foo.Prop1"),1);
    EXPECT_EQ(testRetrieveElement(root, "Device.Foo.Table1", "Device.Foo.Table1"),1);
    EXPECT_EQ(testRetrieveElement(root, "Device.Foo.Table1.", "Device.Foo.Table1"),1);
    EXPECT_EQ(testRetrieveElement(root, "Device.Foo.Table1.{i}", "Device.Foo.Table1.{i}"),1);
    EXPECT_EQ(testRetrieveElement(root, "Device.Foo.Table1.{i}.", "Device.Foo.Table1.{i}"),1);
    EXPECT_EQ(testRetrieveElement(root, "Device.Foo.Table1.{i}.Prop1", "Device.Foo.Table1.{i}.Prop1"),1);
    EXPECT_EQ(testRetrieveElement(root, "Device.Foo.Table1.1.Prop1", "Device.Foo.Table1.{i}.Prop1"),1);
    EXPECT_EQ(testRetrieveElement(root, "Device.Foo.Table1.2.Prop1", "Device.Foo.Table1.{i}.Prop1"),1);

    EXPECT_EQ(testRetrieveInstanceElement(root, "Device", "Device"),1);
    EXPECT_EQ(testRetrieveInstanceElement(root, "Device.", "Device"),1);
    EXPECT_EQ(testRetrieveInstanceElement(root, "Device.Foo", "Device.Foo"),1);
    EXPECT_EQ(testRetrieveInstanceElement(root, "Device.Foo.Prop1", "Device.Foo.Prop1"),1);
    EXPECT_EQ(testRetrieveInstanceElement(root, "Device.Foo.Table1", "Device.Foo.Table1"),1);
    EXPECT_EQ(testRetrieveInstanceElement(root, "Device.Foo.Table1.", "Device.Foo.Table1"),1);
    EXPECT_EQ(testRetrieveInstanceElement(root, "Device.Foo.Table1.1.Prop1", "Device.Foo.Table1.1.Prop1"),1);
    EXPECT_EQ(testRetrieveInstanceElement(root, "Device.Foo.Table1.2.Prop1", "Device.Foo.Table1.2.Prop1"),1);

    //deleting elements/row
    removeElem(root, "Device.Foo.Table1.{i}.Prop1");
    removeElem(root, "Device.Foo.Table1.{i}.");
    removeElem(root, "Device.Foo.Prop1");


    //Trying to retrieve after deleting
    EXPECT_EQ(testRetrieveElement(root, "Device", "Device"),0);
    EXPECT_EQ(testRetrieveElement(root, "Device.", "Device"),0);
    EXPECT_EQ(testRetrieveElement(root, "Device.Foo", "Device.Foo"),0);
    EXPECT_EQ(testRetrieveElement(root, "Device.Foo.Prop1", "Device.Foo.Prop1"),0);
    EXPECT_EQ(testRetrieveElement(root, "Device.Foo.Table1", "Device.Foo.Table1"),0);
    EXPECT_EQ(testRetrieveElement(root, "Device.Foo.Table1.", "Device.Foo.Table1"),0);
    EXPECT_EQ(testRetrieveElement(root, "Device.Foo.Table1.{i}", "Device.Foo.Table1.{i}"),0);
    EXPECT_EQ(testRetrieveElement(root, "Device.Foo.Table1.{i}.", "Device.Foo.Table1.{i}"),0);
    EXPECT_EQ(testRetrieveElement(root, "Device.Foo.Table1.{i}.Prop1", "Device.Foo.Table1.{i}.Prop1"),0);
    EXPECT_EQ(testRetrieveElement(root, "Device.Foo.Table1.1.Prop1", "Device.Foo.Table1.{i}.Prop1"),0);
    EXPECT_EQ(testRetrieveElement(root, "Device.Foo.Table1.2.Prop1", "Device.Foo.Table1.{i}.Prop1"),0);

    EXPECT_EQ(testRetrieveInstanceElement(root, "Device", "Device"),0);
    EXPECT_EQ(testRetrieveInstanceElement(root, "Device.", "Device"),0);
    EXPECT_EQ(testRetrieveInstanceElement(root, "Device.Foo", "Device.Foo"),0);
    EXPECT_EQ(testRetrieveInstanceElement(root, "Device.Foo.Prop1", "Device.Foo.Prop1"),0);
    EXPECT_EQ(testRetrieveInstanceElement(root, "Device.Foo.Table1", "Device.Foo.Table1"),0);
    EXPECT_EQ(testRetrieveInstanceElement(root, "Device.Foo.Table1.", "Device.Foo.Table1"),0);
    EXPECT_EQ(testRetrieveInstanceElement(root, "Device.Foo.Table1.1.Prop1", "Device.Foo.Table1.1.Prop1"),0);
    EXPECT_EQ(testRetrieveInstanceElement(root, "Device.Foo.Table1.2.Prop1", "Device.Foo.Table1.2.Prop1"),0);

    freeElementNode(root);
}
