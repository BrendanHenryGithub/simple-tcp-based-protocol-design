#include <boost/asio.hpp>
#include <iostream>
#include <unordered_map>
#include "client.hpp"
#include <thread>
#include <string>

const std::unordered_map<std::string, Protocol::Type> Command{
    {"create", Protocol::Type::CREATE_CHANNEL},
    {"list", Protocol::Type::LIST_ALL_CHANNELS},
    {"join", Protocol::Type::JOIN_IN_CHANNEL},
    {"leave", Protocol::Type::LEAVE_CHANNEL},
};

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        std::cerr << "Usage: client <host> <port>\n";
        return 1;
    }
    boost::asio::io_context ioContext;
    boost::asio::ip::tcp::resolver resolver(ioContext);
    auto endpoints = resolver.resolve(argv[1], argv[2]);
    Client client(ioContext, endpoints);
    std::thread thread([&ioContext]() { ioContext.run(); });
    std::string line;
    line.resize(Protocol::BODY_MAX_LENGTH);
    while (true)
    {
        std::cout << "> ";
        std::getline(std::cin, line);
        if (line.front() == '!') // 以"!"作为命令标志
        {                        // 执行操作如： ！join XXX  (加入指定的channel)
            std::istringstream ss(line);
            ss.get();
            std::string cmdStr;
            ss >> cmdStr;
            auto cmd = Command.find(cmdStr);
            if (cmd == Command.end())
            {
                std::cout << "[Invalid Command]" << std::endl;
                continue;
            }
            auto type = cmd->second;
            std::string argument;
            if (type == Protocol::Type::CREATE_CHANNEL || type == Protocol::Type::JOIN_IN_CHANNEL)
            {
                ss >> argument;
                if (argument.empty())
                {
                    std::cout << "[Invalid Argument]" << std::endl;
                    continue;
                }
            }
            try
            {
                client.write(Protocol::encodePackage(type, argument));
            }
            catch (const std::exception &e)
            {
                std::cout << "[" << e.what() << "]" << std::endl;
                continue;
            }
        }
        else
        { // 发送消息
            try
            {
                client.write(Protocol::encodePackage(line));
            }
            catch (const std::exception &e)
            {
                std::cout << "[" << e.what() << "]" << std::endl;
                continue;
            }
        }
    }

    client.close();
    thread.join();
    return 0;
}