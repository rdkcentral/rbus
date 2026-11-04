#include "gtest/gtest.h"
#include "rtMessageHeader.h"
#include "rtEncoder.h"
#include <cstring>
#include <vector>

TEST(rtMessageHeaderTest, rejects_short_headers_before_reading_fixed_fields)
{
    for(uint16_t length = 6; length < 32; ++length)
    {
        std::vector<uint8_t> bytes(length, 0);
        uint8_t* ptr = bytes.data();
        rtEncoder_EncodeUInt16(&ptr, RTMSG_HEADER_MARKER);
        rtEncoder_EncodeUInt16(&ptr, RTMSG_HEADER_VERSION);
        rtEncoder_EncodeUInt16(&ptr, length);
        rtMessageHeader header;
        rtMessageHeader_Init(&header);
        EXPECT_EQ(RT_ERROR, rtMessageHeader_Decode(&header, bytes.data()));
    }
}

TEST(rtMessageHeaderTest, rejects_strings_crossing_declared_header_boundary)
{
    for(int field = 0; field < 2; ++field)
    {
        std::vector<uint8_t> bytes(32, 0);
        uint8_t* ptr = bytes.data();
        rtEncoder_EncodeUInt16(&ptr, RTMSG_HEADER_MARKER);
        rtEncoder_EncodeUInt16(&ptr, RTMSG_HEADER_VERSION);
        rtEncoder_EncodeUInt16(&ptr, 32);
        for(int i = 0; i < 4; ++i)
            rtEncoder_EncodeUInt32(&ptr, 0);
        rtEncoder_EncodeUInt32(&ptr, field == 0 ? 1 : 0);
        rtEncoder_EncodeUInt32(&ptr, field == 1 ? 1 : 0);
        rtEncoder_EncodeUInt16(&ptr, RTMSG_HEADER_MARKER);
        rtMessageHeader header;
        rtMessageHeader_Init(&header);
        EXPECT_EQ(RT_ERROR, rtMessageHeader_Decode(&header, bytes.data()));
    }
}

TEST(rtMessageHeaderTest, preserves_roundtrip_and_terminates_reused_topic_buffers)
{
    uint8_t bytes[1024] = {};
    rtMessageHeader source, decoded;
    rtMessageHeader_Init(&source);
    std::strcpy(source.topic, "topic");
    std::strcpy(source.reply_topic, "reply");
    source.payload_length = 123;
    ASSERT_EQ(RT_OK, rtMessageHeader_Encode(&source, bytes));
    rtMessageHeader_Init(&decoded);
    std::memset(decoded.topic, 'x', sizeof(decoded.topic));
    std::memset(decoded.reply_topic, 'x', sizeof(decoded.reply_topic));
    ASSERT_EQ(RT_OK, rtMessageHeader_Decode(&decoded, bytes));
    EXPECT_STREQ("topic", decoded.topic);
    EXPECT_STREQ("reply", decoded.reply_topic);
    EXPECT_EQ(123U, decoded.payload_length);

    source.topic[0] = source.reply_topic[0] = '\0';
    ASSERT_EQ(RT_OK, rtMessageHeader_Encode(&source, bytes));
    ASSERT_EQ(RT_OK, rtMessageHeader_Decode(&decoded, bytes));
    EXPECT_STREQ("", decoded.topic);
    EXPECT_STREQ("", decoded.reply_topic);
}
