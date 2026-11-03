#include "gtest/gtest.h"
#include "rtSocket.h"
#include <string>
#include <cstring>
#include <sys/un.h>

TEST(rtSocketTest, rejects_oversized_tcp_host_without_modifying_endpoint)
{
    size_t lengths[] = {128, 129, 512, 4096};
    for(size_t length : lengths)
    {
        struct sockaddr_storage endpoint = {};
        endpoint.ss_family = AF_UNIX;
        struct sockaddr_storage original = endpoint;
        std::string address = "tcp://" + std::string(length, 'a') + ":6667";
        EXPECT_EQ(RT_ERROR_INVALID_ARG, rtSocketStorage_FromString(&endpoint, address.c_str()));
        EXPECT_EQ(0, std::memcmp(&original, &endpoint, sizeof(endpoint)));
    }
}

TEST(rtSocketTest, rejects_empty_host_and_null_arguments)
{
    struct sockaddr_storage endpoint = {};
    EXPECT_EQ(RT_ERROR_INVALID_ARG, rtSocketStorage_FromString(&endpoint, "tcp://:6667"));
    EXPECT_EQ(RT_ERROR_INVALID_ARG, rtSocketStorage_FromString(&endpoint, NULL));
    EXPECT_EQ(RT_ERROR_INVALID_ARG, rtSocketStorage_FromString(NULL, "tcp://127.0.0.1:6667"));
}

TEST(rtSocketTest, preserves_ipv4_and_unix_endpoints)
{
    struct sockaddr_storage endpoint = {};
    ASSERT_EQ(RT_OK, rtSocketStorage_FromString(&endpoint, "tcp://127.0.0.1:6667"));
    EXPECT_EQ(AF_INET, endpoint.ss_family);
    struct sockaddr_in* ipv4 = reinterpret_cast<struct sockaddr_in*>(&endpoint);
    EXPECT_EQ(6667, ntohs(ipv4->sin_port));
    EXPECT_EQ(INADDR_LOOPBACK, ntohl(ipv4->sin_addr.s_addr));

    ASSERT_EQ(RT_OK, rtSocketStorage_FromString(&endpoint, "unix:///tmp/rtrouted"));
    EXPECT_EQ(AF_UNIX, endpoint.ss_family);
    struct sockaddr_un* unixEndpoint = reinterpret_cast<struct sockaddr_un*>(&endpoint);
    EXPECT_STREQ("/tmp/rtrouted", unixEndpoint->sun_path);
}

