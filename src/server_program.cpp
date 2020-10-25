#include <server.hpp>
#include <boost/asio.hpp>
#include <glog/logging.h>

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        LOG(ERROR) << "Usage: server <port>\n";
        return 1;
    }
    boost::asio::io_context ioContext;
    boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v4(), std::atoi(argv[1]));
    Server server(ioContext, endpoint);
    ioContext.run();
    return 0;
}
