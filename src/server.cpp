#include "protocol.hpp"
#include "server.hpp"
#include <boost/asio.hpp>
#include <unordered_map>
#include <set>
#include <queue>
#include <sstream>
#include <algorithm>
#include <glog/logging.h>

// ---------------- Class Server ------------------------------

std::unordered_map<std::string, std::shared_ptr<Channel>> Server::channels{};
std::set<std::shared_ptr<Participant>> Server::members{};

Server::Server(boost::asio::io_context &ioCtx, const boost::asio::ip::tcp::endpoint &endpoint)
    : acceptor(ioCtx, endpoint)
{
    LOG(INFO) << "server listen in " << endpoint.address().to_string() << ":" << endpoint.port();
    accept();
}

void Server::accept()
{
    acceptor.async_accept([this](std::error_code ec, boost::asio::ip::tcp::socket socket) {
        if (!ec)
        {
            LOG(INFO) << "accept one connection, " << socket.remote_endpoint().address().to_string() << ":" << socket.remote_endpoint().port();
            auto mem = std::make_shared<Participant>(std::move(socket));
            members.insert(mem->getPtr());
            mem->run();
            accept();
        }
    });
}

// ---------------- Class Channel ------------------------------

Channel::Channel(std::string channelName)
    : name(channelName),
      MAX_CONNECTION_NUM(2)
{
}

Channel::Channel(std::string channelName, int num)
    : name(channelName),
      MAX_CONNECTION_NUM(num)
{
}

Channel::~Channel()
{
    LOG(INFO) << "destruct this channel: " << name;
}

bool Channel::ifFull()
{
    return connections.size() == MAX_CONNECTION_NUM;
}

unsigned int Channel::count()
{
    return connections.size();
}

std::string Channel::getName()
{
    return name;
}

std::shared_ptr<Channel> Channel::getPtr()
{
    return shared_from_this();
}

bool Channel::join(std::shared_ptr<Participant> con)
{
    if (ifFull())
    {
        return false;
    }
    connections.insert(con);
    return true;
}

bool Channel::send(std::shared_ptr<Participant> self, const Protocol::Package &pkg)
{
    bool sendAtLeastOneTime = false;
    for (auto &item : connections)
    {
        if (item != self)
        {
            item->write(pkg);
            sendAtLeastOneTime = true;
        }
    }
    return sendAtLeastOneTime;
}

void Channel::leave(std::shared_ptr<Participant> self)
{
    connections.erase(self);
    if (connections.empty())
    {
        Server::channels.erase(name);
    }
}

// ---------------- Class Participant ------------------------------

Participant::Participant(boost::asio::ip::tcp::socket socket_)
    : socket(std::move(socket_))
{
    channel.reset();
}

Participant::~Participant()
{
    LOG(INFO) << "destruct participant. remote address is " << socket.remote_endpoint().address().to_string() << ":" << socket.remote_endpoint().port();
}

void Participant::run()
{
    readHeader();
}

std::shared_ptr<Participant> Participant::getPtr()
{
    return shared_from_this();
}

void Participant::exit()
{
    // 退出channel
    if (channel)
    {
        channel->leave(shared_from_this());
        channel.reset();
    }

    // socket.close();
    Server::members.erase(shared_from_this());
}

void Participant::write(const Protocol::Package &pkg)
{
    bool actionFlag = !pkgQueue.empty(); // 当队列非空时说明还在执行写操作。
    pkgQueue.push(pkg);
    if (!actionFlag)
    {
        execWriteAction();
    }
}

void Participant::readHeader()
{
    auto pkg = std::make_shared<Protocol::Package>();
    boost::asio::async_read(socket,
                            std::vector<boost::asio::mutable_buffer>{
                                boost::asio::buffer(&pkg->type, sizeof(Protocol::Package::type)),
                                boost::asio::buffer(&pkg->length, sizeof(Protocol::Package::length)),
                            },
                            [pkg, this](std::error_code ec, std::size_t len) {
                                if (!ec)
                                {
                                    pkg->body.resize(pkg->length);
                                    readBody(pkg);
                                }
                                else
                                {
                                    LOG(ERROR) << "operation failed";
                                    exit();
                                }
                            });
}

void Participant::readBody(std::shared_ptr<Protocol::Package> pkg)
{
    boost::asio::async_read(socket, boost::asio::buffer(pkg->body, pkg->length),
                            [pkg, this](std::error_code ec, std::size_t len) {
                                if (!ec)
                                {
                                    LOG(INFO) << "async read length = "<< len;
                                    handle(*pkg);
                                    readHeader();
                                }
                                else
                                {
                                    LOG(ERROR) << "operation failed";
                                    exit();
                                }
                            });
}

void Participant::execWriteAction()
{
    boost::asio::async_write(socket,
                             std::vector<boost::asio::const_buffer>{
                                 boost::asio::buffer(&pkgQueue.front().type, sizeof(Protocol::Package::type)),
                                 boost::asio::buffer(&pkgQueue.front().length, sizeof(Protocol::Package::length)),
                                 boost::asio::buffer(pkgQueue.front().body)},
                             [this](std::error_code ec, std::size_t len) {
                                 if (!ec)
                                 {
                                     pkgQueue.pop();
                                     if (!pkgQueue.empty())
                                     {
                                         execWriteAction();
                                     }
                                 }
                                 else
                                 {
                                     LOG(ERROR) << "operation failed";
                                     exit();
                                 }
                             });
}

void Participant::handle(const Protocol::Package &pkg)
{
    try
    {
        auto type = Protocol::decodePackage(pkg);
        switch (type)
        {
        case Protocol::Type::MESSAGE: //转发给同一channel的接收方
            transmit(pkg);
            break;

        case Protocol::Type::LEAVE_CHANNEL:
            leaveChannel();
            break;

        case Protocol::Type::LIST_ALL_CHANNELS:
            listAllChannels();
            break;

        case Protocol::Type::CREATE_CHANNEL:
            createChannel(std::string(pkg.body.begin(), pkg.body.end()));
            break;

        case Protocol::Type::JOIN_IN_CHANNEL:
            joinInChannel(std::string(pkg.body.begin(), pkg.body.end()));
            break;

        default:
            LOG(ERROR) << "unknow type: " << static_cast<std::int16_t>(type);
            break;
        }
    }
    catch (const std::exception &e)
    {
        LOG(ERROR) << "[" << e.what() << "]";
    }
}

void Participant::transmit(const Protocol::Package &inputPkg)
{
    Protocol::Package pkg;
    if (!channel)
    {
        pkg = Protocol::encodePackage(Protocol::Type::OTHER_ERROR,
                                      "Error: did not join any channel");
    }
    else if (channel->count() <= 1)
    {
        pkg = Protocol::encodePackage(Protocol::Type::OTHER_ERROR,
                                      "Error: no other members in this channel");
    }
    else
    {
        if (!channel->send(shared_from_this(), inputPkg))
        {
            pkg = Protocol::encodePackage(Protocol::Type::OTHER_ERROR,
                                          "Error: transmit package failed");
        }
        else
        {
            return;
        }
    }
    write(pkg);
}

void Participant::leaveChannel()
{
    if (channel)
    {
        channel->leave(shared_from_this());
        channel.reset();
        Protocol::Package pkg;
        pkg = Protocol::encodePackage(Protocol::Type::SUCCEED_IN_LEAVE_CHANNEL,
                                      "leaved channel");
        write(pkg);
    }
}

void Participant::listAllChannels()
{
    std::stringstream ss;
    std::string separator = ", ";
    auto it = Server::channels.cbegin();
    if (it != Server::channels.cend())
    {
        ss << it->first;
        it++;
    }
    for (; it != Server::channels.cend(); it++)
    {
        ss << separator << it->first;
    }
    Protocol::Package pkg = Protocol::encodePackage(Protocol::Type::CHANNEL_LIST, ss.str());
    write(pkg);
}

void Participant::createChannel(std::string channelName)
{
    Protocol::Package pkg;
    if (channelName.empty() || channelName.find(',') != std::string::npos)
    {
        pkg = Protocol::encodePackage(Protocol::Type::FAIL_IN_CREATE_CHANNEL,
                                      "Error: invalid channel name");
    }
    else if (channel)
    {
        pkg = Protocol::encodePackage(Protocol::Type::FAIL_IN_CREATE_CHANNEL,
                                      "Error: already in one channel");
    }
    else
    {
        auto channelPtr = std::make_shared<Channel>(channelName);
        auto rlt = Server::channels.emplace(channelName, channelPtr);
        if (rlt.second)
        {
            if (rlt.first->second->join(shared_from_this()))
            {
                channel = channelPtr->getPtr();
                pkg = Protocol::encodePackage(Protocol::Type::SUCCEED_IN_CREATE_CHANNEL,
                                              "create and join in channel: " + channelName);
            }
            else
            {
                pkg = Protocol::encodePackage(Protocol::Type::FAIL_IN_CREATE_CHANNEL,
                                              "Error: failed to join in created channel");
                Server::channels.erase(rlt.first);
            }
        }
        else
        {
            pkg = Protocol::encodePackage(Protocol::Type::FAIL_IN_CREATE_CHANNEL,
                                          "Error: channel already exist");
        }
    }
    write(pkg);
}

void Participant::joinInChannel(std::string channelName)
{
    Protocol::Package pkg;
    if (channel)
    {
        pkg = Protocol::encodePackage(Protocol::Type::FAIL_IN_JOIN_IN_CHANNEL,
                                      "already in channel: " + channel->getName());
    }
    else
    {
        auto it = Server::channels.find(channelName);
        if (it != Server::channels.end())
        {
            if (it->second->join(shared_from_this()))
            {
                channel = it->second->getPtr();
                pkg = Protocol::encodePackage(Protocol::Type::SUCCEED_IN_JOIN_IN_CHANNEL,
                                              "join in channel: " + channelName);
            }
            else if (it->second->ifFull())
            {
                pkg = Protocol::encodePackage(Protocol::Type::FAIL_IN_JOIN_IN_CHANNEL,
                                              "Error: selected channel is full");
            }
            else
            {
                pkg = Protocol::encodePackage(Protocol::Type::FAIL_IN_JOIN_IN_CHANNEL,
                                              "Error: fail to join in selected channel");
            }
        }
        else
        {
            pkg = Protocol::encodePackage(Protocol::Type::FAIL_IN_JOIN_IN_CHANNEL,
                                          "Error: selected channel not exist");
        }
    }

    write(pkg);
}
