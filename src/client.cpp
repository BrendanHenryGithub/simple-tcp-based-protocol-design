#include "protocol.hpp"
#include "client.hpp"
#include <iostream>
#include <thread>
#include <boost/asio.hpp>
#include <queue>
#include <unordered_map>
#include <sstream>
#include <glog/logging.h>
#include <limits>

Client::Client(boost::asio::io_context &ioCtx,
               const boost::asio::ip::tcp::resolver::results_type &endpoints)
    : ioContext(ioCtx),
      socket(ioCtx)
{
    connect(endpoints);
}

void Client::write(const Protocol::Package &pkg)
{
    boost::asio::post(ioContext, [pkg, this]() {
        bool actionFlag = !pkgQueue.empty(); // 当队列非空时说明还在执行写操作。
        pkgQueue.push(pkg);
        if (!actionFlag)
        {
            execWriteAction();
        }
    });
}

void Client::close()
{
    boost::asio::post(ioContext, [this]() { socket.close(); });
}

void Client::connect(const boost::asio::ip::tcp::resolver::results_type &endpoints)
{
    boost::asio::async_connect(socket, endpoints,
                               [this](std::error_code ec, boost::asio::ip::tcp::endpoint) {
                                   if (!ec)
                                   {
                                       readHeader();
                                   }else{
                                       LOG(ERROR) << ec.message();
                                       close();
                                   }
                               });
}

void Client::readHeader()
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
                                }else{
                                    LOG(ERROR) << ec.message();
                                    close();
                                }
                            });
}

void Client::readBody(std::shared_ptr<Protocol::Package> pkg)
{
    boost::asio::async_read(socket, boost::asio::buffer(pkg->body, pkg->length),
                            [pkg, this](std::error_code ec, std::size_t len) {
                                if (!ec)
                                {
                                    try
                                    {
                                        auto type = Protocol::decodePackage(*pkg);
                                        std::string output;
                                        switch (type)
                                        {
                                        case Protocol::Type::MESSAGE:
                                            output = "<------- " + std::string(pkg->body.begin(), pkg->body.end());
                                            break;

                                        default:
                                            output = "[" + std::string(pkg->body.begin(), pkg->body.end()) + "]";
                                            break;
                                        }
                                        std::cout << "\n" << output << "\n> " << std::flush;
                                    }
                                    catch (const std::exception &e)
                                    {
                                        // std::cout << "[" << e.what() << "]" << std::endl;
                                        LOG(ERROR) << e.what();
                                    }
                                    readHeader();
                                }else{
                                    LOG(ERROR) << ec.message();
                                    close();
                                }
                            });
}

void Client::execWriteAction()
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
                                 }else{
                                     LOG(ERROR) << ec.message();
                                 }
                             });
}
