#include "protocol.hpp"
#include <stdexcept>
#include <glog/logging.h>

using namespace Protocol;

Package Protocol::encodePackage(const std::string &msg)
{
    return encodePackage(Type::MESSAGE, msg);
}

Package Protocol::encodePackage(Type type, const std::string &content)
{
    Package pkg;
    if (!checkType(type))
    {
        throw invalid_type();
    }
    pkg.type = static_cast<std::uint16_t>(type);
    if (content.size() > BODY_MAX_LENGTH)
    {
        throw invalid_length();
    }
    pkg.length = static_cast<std::uint16_t>(content.size());
    pkg.body = std::vector<std::uint8_t>(content.begin(), content.end());
    return pkg;
}

Type Protocol::decodePackage(const Package &pkg)
{
    if (pkg.length != pkg.body.size())
    {
        LOG(ERROR) << "package.length = " << pkg.length << ", pakcage.body.length = " << pkg.body.size();
        LOG(ERROR) << "pakcage.body = " << std::string(pkg.body.begin(), pkg.body.end());
        throw invalid_length();
    }
    Type type = static_cast<Type>(pkg.type);
    if (!checkType(type))
    {
        throw invalid_type();
    }
    return type;
}
