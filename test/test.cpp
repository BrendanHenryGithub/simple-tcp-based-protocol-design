#include <gtest/gtest.h>
#include <iostream>
#include "protocol.hpp"

TEST(Protocol, encodePackage)
{
    const std::string msg = "Hello World";
    Protocol::Package rlt = Protocol::encodePackage(msg);
    EXPECT_EQ(rlt.type, static_cast<std::uint16_t>(Protocol::Type::MESSAGE));
    EXPECT_EQ(rlt.length, 11);
    EXPECT_EQ(rlt.body.size(), 11);
    EXPECT_STREQ(std::string(rlt.body.begin(), rlt.body.end()).c_str(), msg.c_str());

    const std::string bigMsg(65536, 'a');
    EXPECT_THROW(Protocol::encodePackage(bigMsg), Protocol::invalid_length);

    EXPECT_THROW(Protocol::encodePackage(static_cast<Protocol::Type>(0xFF), msg), Protocol::invalid_type);
}

TEST(Protocol, decodePackage)
{
    const std::string msg = "Hello World";
    Protocol::Package pkg{static_cast<std::uint16_t>(Protocol::Type::MESSAGE), 11, std::vector<std::uint8_t>(msg.begin(), msg.end())};
    EXPECT_EQ(Protocol::decodePackage(pkg), Protocol::Type::MESSAGE);

    Protocol::Package invalidPkg{static_cast<std::uint16_t>(Protocol::Type::MESSAGE), 1, std::vector<std::uint8_t>(msg.begin(), msg.end())};
    EXPECT_THROW(Protocol::decodePackage(invalidPkg), Protocol::invalid_length);

    Protocol::Package invalidPkg2{0xFF, 11, std::vector<std::uint8_t>(msg.begin(), msg.end())};
    EXPECT_THROW(Protocol::decodePackage(invalidPkg2), Protocol::invalid_type);
}

int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}