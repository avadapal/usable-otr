#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <iostream>
#include <string>
#include <atomic>
#include <include/server.hpp>
#include <include/client.hpp>

using tcp = boost::asio::ip::tcp;

int main() 
{

    boost::asio::io_context io_context;
    std::string address, port;
    boost::filesystem::create_directories("../lib/logs");   // Create a directory to hold the message DBs

    std::cout << "Enter host IP address: ";
    std::getline(std::cin, address);
    std::cout << "Enter host port: ";
    std::getline(std::cin, port);

    std::thread server_thread([&io_context, address, port]() {      // Server thread
        server(io_context, address, port);
    });
    server_thread.join();

    return 0;
}
