#pragma once

#include <string>
#include <vector>
#include <iostream>

// 简单的报文协议
namespace Protocol
{
    // 描述协议报文的一个完整包的具体结构
    struct Package
    {
        std::uint16_t type;             // 标识该报文的类型
        std::uint16_t length;           // 标识后续body的长度
        std::vector<std::uint8_t> body; // 负载
    };

    constexpr unsigned int HEADER_LENGTH = 2 + 2;
    constexpr std::uint16_t BODY_MAX_LENGTH = UINT16_MAX; // 2个byte所能标识的最大无符号数：65535
    constexpr unsigned int PACKAGE_MAX_LENGTH = BODY_MAX_LENGTH + HEADER_LENGTH;

    enum class Type : std::uint16_t
    {
        MESSAGE = 0,

        CREATE_CHANNEL = 1,
        SUCCEED_IN_CREATE_CHANNEL = 2,
        FAIL_IN_CREATE_CHANNEL = 3,

        LIST_ALL_CHANNELS = 4,
        CHANNEL_LIST = 5,

        JOIN_IN_CHANNEL = 6,
        SUCCEED_IN_JOIN_IN_CHANNEL = 7,
        FAIL_IN_JOIN_IN_CHANNEL = 8,

        LEAVE_CHANNEL = 9,
        SUCCEED_IN_LEAVE_CHANNEL = 10,

        OTHER_ERROR = 11,
    };

    inline bool checkType(const Type &type)
    {
        switch (type)
        {
        case Type::MESSAGE:
        case Type::CREATE_CHANNEL:
        case Type::SUCCEED_IN_CREATE_CHANNEL:
        case Type::FAIL_IN_CREATE_CHANNEL:
        case Type::LIST_ALL_CHANNELS:
        case Type::CHANNEL_LIST:
        case Type::JOIN_IN_CHANNEL:
        case Type::SUCCEED_IN_JOIN_IN_CHANNEL:
        case Type::FAIL_IN_JOIN_IN_CHANNEL:
        case Type::LEAVE_CHANNEL:
        case Type::SUCCEED_IN_LEAVE_CHANNEL:
        case Type::OTHER_ERROR:
            return true;
        }
        return false;
    }

    class invalid_type : public std::exception
    {
    public:
        const char *what() const noexcept override
        {
            return "Invalid Type";
        }
    };

    class invalid_length : public std::exception
    {
    public:
        const char *what() const noexcept override
        {
            return "Invalid legnth";
        }
    };

    /**
     * 将给定的信息封装成一个package
     * 当长度超出限制时抛出异常：invalid_length
     */
    Package encodePackage(const std::string &);
    Package encodePackage(Type, const std::string &);

    /**
     * 返回pakckage的类型信息
     * 当type值不在所定义的范围内时抛出异常: invalid_type
     * 当length值与实际负载长度不一致时抛出异常：invalid_length
     */
    Type decodePackage(const Package &);
}; // namespace Protocol