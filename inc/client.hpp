#include "protocol.hpp"
#include <boost/asio.hpp>
#include <memory>
#include <queue>

class Client
{
private:
    boost::asio::io_context &ioContext;
    boost::asio::ip::tcp::socket socket;
    std::queue<Protocol::Package> pkgQueue;

public:
    Client(boost::asio::io_context &ioCtx,
           const boost::asio::ip::tcp::resolver::results_type &endpoints);

    void write(const Protocol::Package &pkg);

    void close();

private:
    void connect(const boost::asio::ip::tcp::resolver::results_type &endpoints);

    void readHeader();

    void readBody(std::shared_ptr<Protocol::Package> pkg);

    void execWriteAction();
};
