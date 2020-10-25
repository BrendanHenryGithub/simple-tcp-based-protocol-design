#pragma once

#include "protocol.hpp"
#include <boost/asio.hpp>
#include <unordered_map>
#include <set>
#include <queue>
#include <memory>

class Channel;
class Participant;

// ---------------- Class Server ------------------------------

class Server
{
private:
    boost::asio::ip::tcp::acceptor acceptor;

public:
    // 所有channel的集合
    static std::unordered_map<std::string, std::shared_ptr<Channel>> channels;
    // 所有成员集合
    static std::set<std::shared_ptr<Participant>> members;

public:
    Server(boost::asio::io_context &ioCtx, const boost::asio::ip::tcp::endpoint &endpoint);

private:
    void accept();
};

// ---------------- Class Channel ------------------------------

class Channel : public std::enable_shared_from_this<Channel>
{
private:
    unsigned int MAX_CONNECTION_NUM;
    std::string name;
    std::set<std::shared_ptr<Participant>> connections;

public:
    explicit Channel(std::string channelName);

    Channel(std::string channelName, int num);
    ~Channel();

    bool ifFull();

    unsigned int count();

    std::string getName();

    std::shared_ptr<Channel> getPtr();

    bool join(std::shared_ptr<Participant> con);

    bool send(std::shared_ptr<Participant> self, const Protocol::Package &pkg);

    void leave(std::shared_ptr<Participant> self);
};

// ---------------- Class Participant ------------------------------

class Participant : public std::enable_shared_from_this<Participant>
{
private:
    boost::asio::ip::tcp::socket socket;
    std::shared_ptr<Channel> channel;
    std::queue<Protocol::Package> pkgQueue;

public:
    Participant(boost::asio::ip::tcp::socket socket_);
    ~Participant();

    void run();

    void write(const Protocol::Package &pkg);

    std::shared_ptr<Participant> getPtr();

    void exit();

private:
    void readHeader();

    void readBody(std::shared_ptr<Protocol::Package> pkg);

    void handle(const Protocol::Package &pkg);

    void transmit(const Protocol::Package &inputPkg);

    void leaveChannel();

    void listAllChannels();

    void createChannel(std::string channelName);

    void joinInChannel(std::string channelName);

    void execWriteAction();
};
