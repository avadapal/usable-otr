#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <iostream>
#include <string>
#include <atomic>
#include <include/server.hpp>
#include <include/client.hpp>

using tcp = boost::asio::ip::tcp;

void setup_mode()
{
    std::cout<<"Welcome to DenIM!\n";
    std::cout<<"DenIM offers two modes of security\n";
    std::cout<<"1. Deniable Mode\n";
    std::cout<<"2. Ultra-Deniable Mode\n";
    
    std::string mode;
    
    while(true)
    {
        std::cout<<"Enter your preferred mode of operation: (Type man to know more about the modes)\n";
        std::getline(std::cin,mode);
        if(mode == "1")
        {
            return;
        } else if(mode == "2")
        {
            edit_enabled = true;
            return;
        } else if(mode == "man")
        {
            std::cout<<"Deniable mode offers superficial deniability only on the basis of key exchange\n";
            std::cout<<"Ultra-deniable mode offers an extra layer of deniability by providing a feature to edit/delete the sent and received messages\n";
        } else 
            std::cout<<"Invalid Choice! Please enter again\n";
    }
}

int main() 
{
    setup_mode();

    boost::asio::io_context io_context;
    std::string address, port;
    boost::filesystem::create_directories("../lib/logs");   // Creates a directory to hold the message DBs

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
