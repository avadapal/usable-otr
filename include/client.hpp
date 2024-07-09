#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <boost/asio.hpp>
#include <iostream>
#include <thread>
#include <atomic>
#include "readwrite.hpp"

std::atomic<bool> stop_client_mode = false;
std::atomic<bool> keyex_socket_connected = false;

void keyex_socket_connect(boost::asio::io_context& io_context, std::string& address, std::string& port, tcp::socket& keyex_socket)
{
    boost::system::error_code error;
    tcp::resolver resolver(io_context);
    auto keyex_endpoints = resolver.resolve(address, std::to_string(std::stoi(port) + 1));
    
    boost::asio::connect(keyex_socket, keyex_endpoints, error);
    if(!error)
    {
        keyex_socket_connected = true;
    } else {
        std::cerr << "Client: Error connecting: " << error.message() << "\n";
        return;
    }
}

void client(boost::asio::io_context& io_context) {
    try 
    {
        std::string address, port;
        std::cout << "Enter the recipient IP address: ";
        std::getline(std::cin, address);
        std::cout << "Enter the recipient port: ";
        std::getline(std::cin, port);

        tcp::resolver resolver(io_context);
        auto endpoints = resolver.resolve(address, port);
        tcp::socket socket(io_context);

        tcp::socket keyex_socket(io_context);

        // Connect to the server
        boost::asio::connect(socket, endpoints);

        // Check and create db
        sqlite3* DB;
        const std::string path = "../lib/logs/";
        const std::string dbname = path + "msghist_" + address + ".db";
        sqlite3_open(dbname.c_str(), &DB);

        // Create table if not exists
        std::string create_table = "CREATE TABLE IF NOT EXISTS MSG_LOGS("
                                   "ID INTEGER PRIMARY KEY AUTOINCREMENT,"
                                   "PERSON TEXT NOT NULL,"
                                   "MESSAGE TEXT NOT NULL,"
                                   "TIME TEXT NOT NULL);";
        execute_sql(DB, create_table);
        sqlite3_close(DB);

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        Botan::secure_vector<uint8_t> shared_key;
        
        std::thread keyex_socket_connect_thread([&](){
            keyex_socket_connect(std::ref(io_context), std::ref(address), std::ref(port), std::ref(keyex_socket));
        });
        keyex_socket_connect_thread.detach();

        while(!keyex_socket_connected){
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        // Launch read and write threads
        while(true)
        {
        // Avoid key exchange if a command for viewing/editing has been entered
        if(!command_entered)
        {
            std::thread keyex_thread([&](){
                key_exchange_client(std::ref(keyex_socket), shared_key);
            });
            keyex_thread.detach();

            while(!key_exchange_status_client){
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            key_exchange_status_client = false;
        }

        std::thread write_thread(write_to_socket, std::ref(socket), dbname, std::ref(shared_key));
        std::thread read_thread(read_from_socket, std::ref(socket), dbname, std::ref(shared_key));
    
        // Keep the client (main thread) running
        write_thread.join();
        read_thread.detach();
        }
    } catch (std::exception& e) {
        std::cerr << "Client exception: " << e.what() << "\n";
    }
}

#endif
