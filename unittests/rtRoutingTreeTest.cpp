#include "gtest/gtest.h"
#include "rtRoutingTree.h"
#include <string>

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
    std::string invalid[] = {std::string(512, 'a'), std::string(4096, 'a'), deep, "root..valid"};

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

