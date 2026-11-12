#include "gtest/gtest.h"
#include "rtRoutingTree.h"
#include <string>

class rtRoutingTreeTest : public ::testing::Test
{
protected:
  rtRoutingTree tree = NULL;
  rtList topics = NULL;
  int routeData = 0;

  void SetUp() override
  {
    rtRoutingTree_Create(&tree);
  }

  void TearDown() override
  {
    if(topics)
      rtList_Destroy(topics, NULL);
    rtRoutingTree_Destroy(tree);
  }

  void expectRoute(const std::string& expression)
  {
    rtList routes = NULL;
    rtRoutingTree_GetTopicRoutes(tree, expression.c_str(), &routes);
    ASSERT_NE(routes, nullptr);
    size_t count = 0;
    ASSERT_EQ(rtList_GetSize(routes, &count), RT_OK);
    ASSERT_EQ(count, 1u);
    rtListItem item = NULL;
    void* data = NULL;
    ASSERT_EQ(rtList_GetFront(routes, &item), RT_OK);
    ASSERT_EQ(rtListItem_GetData(item, &data), RT_OK);
    ASSERT_NE(data, nullptr);
    EXPECT_EQ(static_cast<rtTreeRoute*>(data)->route, &routeData);
  }

  void expectRejected(const std::string& expression)
  {
    const std::string valid = "Device.Test.Value";
    ASSERT_EQ(rtRoutingTree_AddTopicRoute(tree, valid.c_str(), &routeData, 0), RT_OK);
    expectRoute(valid);

    EXPECT_EQ(rtRoutingTree_AddTopicRoute(tree, expression.c_str(), &routeData, 0),
              RT_ERROR_INVALID_EXPRESSION);
    rtList routes = NULL;
    rtRoutingTree_GetTopicRoutes(tree, expression.c_str(), &routes);
    EXPECT_EQ(routes, nullptr);

    rtRoutingTree_RemoveTopic(tree, expression.c_str());
    expectRoute(valid);

    ASSERT_EQ(rtList_Create(&topics), RT_OK);
    rtRoutingTree_ResolvePartialPath(tree, expression.c_str(), topics);
    size_t count = 0;
    EXPECT_EQ(rtList_GetSize(topics, &count), RT_OK);
    EXPECT_EQ(count, 0u);
    expectRoute(valid);
  }

  std::string makeTokens(size_t count)
  {
    std::string expression = "A";
    for(size_t i = 1; i < count; ++i)
      expression += ".A";
    return expression;
  }
};

TEST_F(rtRoutingTreeTest, accepts_511_character_token)
{
  const std::string expression(511, 'A');
  ASSERT_EQ(rtRoutingTree_AddTopicRoute(tree, expression.c_str(), &routeData, 0), RT_OK);
  expectRoute(expression);
}

TEST_F(rtRoutingTreeTest, rejects_512_character_token)
{
  expectRejected(std::string(512, 'A'));
}

TEST_F(rtRoutingTreeTest, accepts_32_tokens)
{
  const std::string expression = makeTokens(32);
  ASSERT_EQ(rtRoutingTree_AddTopicRoute(tree, expression.c_str(), &routeData, 0), RT_OK);
  expectRoute(expression);
}

TEST_F(rtRoutingTreeTest, rejects_33_tokens)
{
  expectRejected(makeTokens(33));
}

TEST_F(rtRoutingTreeTest, accepts_exact_buffer_capacity_with_trailing_dot)
{
  const std::string expression = std::string(511, 'A') + ".";
  ASSERT_EQ(rtRoutingTree_AddTopicRoute(tree, expression.c_str(), &routeData, 0), RT_OK);
  expectRoute(expression.substr(0, expression.size() - 1));

  // A trailing-dot lookup returns descendants, not the node's own route.
  rtList routes = NULL;
  rtRoutingTree_GetTopicRoutes(tree, expression.c_str(), &routes);
  EXPECT_EQ(routes, nullptr);
}

TEST_F(rtRoutingTreeTest, rejects_extra_token_after_full_buffer)
{
  expectRejected(std::string(511, 'A') + ".B");
}

TEST_F(rtRoutingTreeTest, rejects_4999_character_expression)
{
  expectRejected(std::string(4999, 'A'));
}

TEST_F(rtRoutingTreeTest, removing_unowned_alias_preserves_routes)
{
  int otherRoute = 0;
  ASSERT_EQ(rtRoutingTree_AddTopicRoute(tree, "Device.Owner", &routeData, 0), RT_OK);
  ASSERT_EQ(rtRoutingTree_AddTopicRoute(tree, "Device.Other", &otherRoute, 0), RT_OK);

  EXPECT_EQ(rtRoutingTree_RemoveTopicRoute(tree, "Device.Owner", &otherRoute),
            RT_ERROR_INVALID_ARG);
  expectRoute("Device.Owner");
  EXPECT_EQ(rtRoutingTree_RemoveTopicRoute(tree, "Device", &otherRoute),
            RT_ERROR_INVALID_ARG);
  expectRoute("Device.Owner");
}

TEST_F(rtRoutingTreeTest, removing_alias_preserves_other_owner_and_descendants)
{
  int otherRoute = 0;
  ASSERT_EQ(rtRoutingTree_AddTopicRoute(tree, "Device.Alias", &routeData, 0), RT_OK);
  ASSERT_EQ(rtRoutingTree_AddTopicRoute(tree, "Device.Alias", &otherRoute, 0), RT_OK);
  ASSERT_EQ(rtRoutingTree_AddTopicRoute(tree, "Device.Alias.Child", &routeData, 0), RT_OK);

  EXPECT_EQ(rtRoutingTree_RemoveTopicRoute(tree, "Device.Alias", &otherRoute), RT_OK);
  expectRoute("Device.Alias");
  expectRoute("Device.Alias.Child");
  expectRoute("Device.Alias.");
  EXPECT_EQ(rtRoutingTree_RemoveTopicRoute(tree, "Device.Alias", &otherRoute),
            RT_ERROR_INVALID_ARG);
}

TEST_F(rtRoutingTreeTest, removing_alias_updates_descendant_caches)
{
  ASSERT_EQ(rtRoutingTree_AddTopicRoute(tree, "Device.Alias.First", &routeData, 0), RT_OK);
  ASSERT_EQ(rtRoutingTree_AddTopicRoute(tree, "Device.Alias.Second", &routeData, 0), RT_OK);

  EXPECT_EQ(rtRoutingTree_RemoveTopicRoute(tree, "Device.Alias.First", &routeData), RT_OK);
  expectRoute("Device.Alias.Second");
  expectRoute("Device.Alias.");
  EXPECT_EQ(rtRoutingTree_RemoveTopicRoute(tree, "Device.Alias.Second", &routeData), RT_OK);

  rtList routes = NULL;
  rtRoutingTree_GetTopicRoutes(tree, "Device.Alias.", &routes);
  size_t count = 0;
  if(routes)
    ASSERT_EQ(rtList_GetSize(routes, &count), RT_OK);
  EXPECT_EQ(count, 0u);
}

TEST_F(rtRoutingTreeTest, removing_last_alias_allows_immediate_registration)
{
  int otherRoute = 0;
  ASSERT_EQ(rtRoutingTree_AddTopicRoute(tree, "Device.rbusProvider.Param1", &routeData, 1),
            RT_OK);
  ASSERT_EQ(rtRoutingTree_AddTopicRoute(tree, "Device.rbusProvider.Param1.Child", &routeData, 1),
            RT_OK);
  ASSERT_EQ(rtRoutingTree_RemoveTopicRoute(tree, "Device.rbusProvider.Param1", &routeData),
            RT_OK);

  rtList routes = NULL;
  rtRoutingTree_GetTopicRoutes(tree, "Device.rbusProvider.Param1", &routes);
  EXPECT_EQ(routes, nullptr);
  EXPECT_EQ(rtRoutingTree_AddTopicRoute(tree, "Device.rbusProvider.Param1", &otherRoute, 1),
            RT_OK);
  EXPECT_EQ(rtRoutingTree_AddTopicRoute(tree, "Device.rbusProvider.Param1", &routeData, 1),
            RT_ERROR_DUPLICATE_ENTRY);
  expectRoute("Device.rbusProvider.Param1.Child");
}

TEST_F(rtRoutingTreeTest, removing_subtree_clears_freed_routes_from_ancestor_caches)
{
  int otherRoute = 0;
  ASSERT_EQ(rtRoutingTree_AddTopicRoute(tree, "Device.A.B", &routeData, 0), RT_OK);
  ASSERT_EQ(rtRoutingTree_AddTopicRoute(tree, "Device.A.B.Child", &otherRoute, 0), RT_OK);

  rtRoutingTree_RemoveTopic(tree, "Device.A.B");
  rtRoutingTree_RemoveTopic(tree, "Device.Absent");

  rtList routes = NULL;
  rtRoutingTree_GetTopicRoutes(tree, "Device.A.", &routes);
  size_t count = 0;
  if(routes)
    ASSERT_EQ(rtList_GetSize(routes, &count), RT_OK);
  EXPECT_EQ(count, 0u);

  ASSERT_EQ(rtList_Create(&topics), RT_OK);
  rtRoutingTree_ResolvePartialPath(tree, "Device.A", topics);
  ASSERT_EQ(rtList_GetSize(topics, &count), RT_OK);
  EXPECT_EQ(count, 0u);
}

TEST_F(rtRoutingTreeTest, removing_subtree_preserves_shared_route_elsewhere)
{
  ASSERT_EQ(rtRoutingTree_AddTopicRoute(tree, "Device.A.B.Child", &routeData, 0), RT_OK);
  ASSERT_EQ(rtRoutingTree_AddTopicRoute(tree, "Device.X.Y", &routeData, 0), RT_OK);

  rtRoutingTree_RemoveTopic(tree, "Device.A.B");

  expectRoute("Device.X.Y");
  expectRoute("Device.X.");
  expectRoute("Device.");
  rtList routes = NULL;
  rtRoutingTree_GetTopicRoutes(tree, "Device.A.", &routes);
  size_t count = 0;
  if(routes)
    ASSERT_EQ(rtList_GetSize(routes, &count), RT_OK);
  EXPECT_EQ(count, 0u);
}
