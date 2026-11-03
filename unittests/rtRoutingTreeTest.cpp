#include "gtest/gtest.h"
#include "rtRoutingTree.h"
#include <string>
#include <utility>

static bool hasRoute(rtRoutingTree tree, const char* topic, const void* expected)
{
    rtList routes = NULL;
    rtListItem item = NULL;
    rtRoutingTree_GetTopicRoutes(tree, topic, &routes);
    rtList_GetFront(routes, &item);
    while(item)
    {
        rtTreeRoute* route;
        rtListItem_GetData(item, reinterpret_cast<void**>(&route));
        if(route->route == expected)
            return true;
        rtListItem_GetNext(item, &item);
    }
    return false;
}

TEST(rtRoutingTreeTest, alias_removal_requires_exact_route_ownership)
{
    rtRoutingTree tree;
    rtRoutingTree_Create(&tree);
    int owner = 0, other = 0;
    ASSERT_EQ(RT_OK, rtRoutingTree_AddTopicRoute(tree, "owner.inbox", &owner, 0));
    ASSERT_EQ(RT_OK, rtRoutingTree_AddTopicRoute(tree, "shared.alias", &owner, 0));
    ASSERT_EQ(RT_OK, rtRoutingTree_AddTopicRoute(tree, "other.inbox", &other, 0));
    ASSERT_EQ(RT_OK, rtRoutingTree_AddTopicRoute(tree, "shared.alias.child", &other, 0));

    EXPECT_EQ(RT_ERROR_INVALID_ARG, rtRoutingTree_RemoveTopicRoute(tree, "shared.alias", &other));
    EXPECT_EQ(RT_ERROR_INVALID_ARG, rtRoutingTree_RemoveTopicRoute(tree, "shared", &owner));
    EXPECT_TRUE(hasRoute(tree, "shared.alias", &owner));
    EXPECT_EQ(RT_OK, rtRoutingTree_RemoveTopicRoute(tree, "shared.alias", &owner));
    EXPECT_FALSE(hasRoute(tree, "shared", &owner));
    EXPECT_TRUE(hasRoute(tree, "shared.alias.child", &other));
    EXPECT_TRUE(hasRoute(tree, "owner.inbox", &owner));
    EXPECT_TRUE(hasRoute(tree, "other.inbox", &other));
    EXPECT_EQ(RT_ERROR_INVALID_ARG, rtRoutingTree_RemoveTopicRoute(tree, "shared.alias", &owner));
    rtRoutingTree_Destroy(tree);
}

TEST(rtRoutingTreeTest, removing_one_registration_preserves_shared_routes_and_cache)
{
    rtRoutingTree tree;
    rtRoutingTree_Create(&tree);
    int owner = 0, other = 0;
    ASSERT_EQ(RT_OK, rtRoutingTree_AddTopicRoute(tree, "shared.alias", &owner, 0));
    ASSERT_EQ(RT_OK, rtRoutingTree_AddTopicRoute(tree, "shared.alias", &other, 0));
    ASSERT_EQ(RT_OK, rtRoutingTree_AddTopicRoute(tree, "shared.sibling", &owner, 0));
    EXPECT_EQ(RT_OK, rtRoutingTree_RemoveTopicRoute(tree, "shared.alias", &owner));
    EXPECT_FALSE(hasRoute(tree, "shared.alias", &owner));
    EXPECT_TRUE(hasRoute(tree, "shared.alias", &other));
    EXPECT_TRUE(hasRoute(tree, "shared.", &owner));
    EXPECT_EQ(RT_OK, rtRoutingTree_RemoveTopicRoute(tree, "shared.sibling", &owner));
    EXPECT_FALSE(hasRoute(tree, "shared.", &owner));
    EXPECT_TRUE(hasRoute(tree, "shared.", &other));
    EXPECT_EQ(RT_OK, rtRoutingTree_AddTopicRoute(tree, "shared.sibling", &owner, 1));
    rtRoutingTree_RemoveRoute(tree, &owner);
    EXPECT_TRUE(hasRoute(tree, "shared.alias", &other));
    rtRoutingTree_Destroy(tree);
}

TEST(rtRoutingTreeTest, accepts_exact_length_and_token_limits)
{
    rtRoutingTree tree;
    rtRoutingTree_Create(&tree);
    int route = 0;
    std::string path(511, 'a');
    EXPECT_EQ(RT_OK, rtRoutingTree_AddTopicRoute(tree, path.c_str(), &route, 0));
    rtList routes = NULL;
    rtRoutingTree_GetTopicRoutes(tree, path.c_str(), &routes);
    EXPECT_NE(nullptr, routes);

    path = "root";
    for(int i = 1; i < 32; ++i)
        path += ".a";
    EXPECT_EQ(RT_OK, rtRoutingTree_AddTopicRoute(tree, path.c_str(), &route, 0));
    rtRoutingTree_GetTopicRoutes(tree, path.c_str(), &routes);
    EXPECT_NE(nullptr, routes);
    EXPECT_EQ(RT_OK, rtRoutingTree_AddTopicRoute(tree, (path + ".").c_str(), &route, 0));
    rtRoutingTree_Destroy(tree);
}

TEST(rtRoutingTreeTest, rejects_invalid_expressions_on_every_tokenizer_path)
{
    rtRoutingTree tree;
    rtRoutingTree_Create(&tree);
    int route = 0;
    ASSERT_EQ(RT_OK, rtRoutingTree_AddTopicRoute(tree, "root.valid", &route, 0));
    std::string deep = "root";
    for(int i = 1; i < 33; ++i)
        deep += ".a";
    std::string invalid[] = {std::string(512, 'a'), std::string(4096, 'a'), std::move(deep), "root..valid"};

    rtList topics;
    rtList_Create(&topics);
    for(std::string const& path : invalid)
    {
        EXPECT_EQ(RT_ERROR_INVALID_EXPRESSION,
                  rtRoutingTree_AddTopicRoute(tree, path.c_str(), &route, 0));
        rtList routes = NULL;
        rtRoutingTree_GetTopicRoutes(tree, path.c_str(), &routes);
        EXPECT_EQ(nullptr, routes);
        rtRoutingTree_ResolvePartialPath(tree, path.c_str(), topics);
        size_t count = 0;
        rtList_GetSize(topics, &count);
        EXPECT_EQ(0U, count);
        rtRoutingTree_RemoveTopic(tree, path.c_str());
    }
    rtList routes = NULL;
    rtRoutingTree_GetTopicRoutes(tree, "root.valid", &routes);
    EXPECT_NE(nullptr, routes);
    rtList_Destroy(topics, NULL);
    rtRoutingTree_Destroy(tree);
}
