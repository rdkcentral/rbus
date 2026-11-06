#include "gtest/gtest.h"
#include "rtSocket.h"
#include <cstring>
#include <string>
#include <sys/un.h>

class rtSocketHostLengthTest : public ::testing::TestWithParam<size_t>
{
};

TEST_P(rtSocketHostLengthTest, rejects_oversized_host_without_changing_endpoint)
{
  struct sockaddr_storage endpoint;
  std::memset(&endpoint, 0x5A, sizeof(endpoint));
  unsigned char original[sizeof(endpoint)];
  std::memcpy(original, &endpoint, sizeof(endpoint));
  const std::string address = "tcp://" + std::string(GetParam(), 'A') + ":12345";

  EXPECT_EQ(rtSocketStorage_FromString(&endpoint, address.c_str()), RT_ERROR_INVALID_ARG);
  EXPECT_EQ(std::memcmp(&endpoint, original, sizeof(endpoint)), 0);

  ASSERT_EQ(rtSocketStorage_FromString(&endpoint, "tcp://127.0.0.1:12345"), RT_OK);
  EXPECT_EQ(endpoint.ss_family, AF_INET);
}

INSTANTIATE_TEST_SUITE_P(Critical008, rtSocketHostLengthTest,
                        ::testing::Values(size_t(128), size_t(129), size_t(4096)));

TEST(rtSocketTest, parses_valid_ipv4_address)
{
  struct sockaddr_storage endpoint = {};
  ASSERT_EQ(rtSocketStorage_FromString(&endpoint, "tcp://127.0.0.1:12345"), RT_OK);
  ASSERT_EQ(endpoint.ss_family, AF_INET);

  const struct sockaddr_in* ipv4 = reinterpret_cast<const struct sockaddr_in*>(&endpoint);
  EXPECT_EQ(ntohs(ipv4->sin_port), 12345);
  EXPECT_EQ(ntohl(ipv4->sin_addr.s_addr), 0x7F000001u);
}

TEST(rtSocketTest, preserves_unix_address_parsing)
{
  struct sockaddr_storage endpoint = {};
  ASSERT_EQ(rtSocketStorage_FromString(&endpoint, "unix:///tmp/rbus-socket-test"), RT_OK);
  ASSERT_EQ(endpoint.ss_family, AF_UNIX);

  const struct sockaddr_un* unixEndpoint = reinterpret_cast<const struct sockaddr_un*>(&endpoint);
  EXPECT_STREQ(unixEndpoint->sun_path, "/tmp/rbus-socket-test");
}
