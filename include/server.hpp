#ifndef SERVER_HPP
#define SERVER_HPP

#include <boost/asio.hpp>
#include <iostream>
#include <thread>
#include <atomic>
#include <string>
#include <memory>
#include "readwrite.hpp"
#include "client.hpp"

std::atomic<bool> client_accepted = false;
std::atomic<bool> keyex_socket_est = false;     // False == Key exchange socket not established yet and vice-versa
Botan::secure_vector<uint8_t> shared_key;

void start_keyex(std::shared_ptr<tcp::socket> keyex_socket, Botan::secure_vector<uint8_t>& shared_key)
{
    key_exchange_server(*keyex_socket,shared_key);
}

void keyex_socket_accept(tcp::acceptor& keyex_acceptor, std::shared_ptr<tcp::socket> keyex_socket)
{   
    while(!client_accepted){            // Sleep (do not execute) until a connection is accepted
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    keyex_acceptor.accept(*keyex_socket);
    keyex_socket_est = true;
}

void start_server_accept(tcp::acceptor& acceptor, std::shared_ptr<boost::asio::ip::tcp::socket> socket) {
    // Start accepting incoming connections from a user
    acceptor.async_accept(*socket, [&](const boost::system::error_code& error) {
        if (!error) {
            std::cout << "Connection accepted." << std::endl;
            client_accepted = true;
            stop_client_mode = true;
        } else {
            std::cerr << "Accept error: " << error.message() << std::endl;
        }
    });
}

void handle_connection_signal(boost::asio::io_context& io_context) {

    // Ask whether user wants to connect to a user -> Switches to client mode
    std::string connection_signal;
    std::cout << "Enter c to connect to a client: ";
    std::getline(std::cin, connection_signal);
    while (connection_signal != "c" && !client_accepted) {
        std::cout << "Invalid response! Enter again: ";
        std::getline(std::cin, connection_signal);
    }

    //  Return if a connection is accepted meanwhile -> and handle that connection
    if(client_accepted)
        return;

    // If "c" is entered -> switch to client mode asking the recipient details
    if (connection_signal == "c") {
        std::thread client_thread([&io_context]() {
            client(io_context);
        });
        client_thread.detach();
    }
}

void handle_client(std::shared_ptr<tcp::socket> socket, const std::string& dbname, const Botan::secure_vector<uint8_t>& key) {
    try {
        
        std::thread write_thread(write_to_socket, std::ref(*socket), dbname, std::ref(key));
        std::thread read_thread(read_from_socket, std::ref(*socket), dbname, std::ref(key));
        
        write_thread.join();
        read_thread.detach();
    } catch (std::exception& e) {
        std::cerr << "Client handling exception: " << e.what() << "\n";
    }
}

void server(boost::asio::io_context& io_context, const std::string& address, const std::string& port) {
    try {
        // Communication socket
        tcp::acceptor acceptor(io_context, tcp::endpoint(boost::asio::ip::make_address(address), std::stoi(port)));
        auto socket = std::make_shared<tcp::socket>(io_context);

        // Key Exchange socket
        tcp::acceptor keyex_acceptor(io_context, tcp::endpoint(boost::asio::ip::make_address(address), std::stoi(port) + 1));
        auto keyex_socket = std::make_shared<tcp::socket>(io_context);

        std::thread start_server_thread([&]() {     // Thread to start accepting connections
            start_server_accept(acceptor, socket);
            io_context.run();
        });

        std::thread handle_connection_signal_thread([&io_context]() {   // Thread for the function of asking the user whether to connect to another user
            handle_connection_signal(io_context);
        });

        start_server_thread.detach();
        handle_connection_signal_thread.detach();

        // Sleep until a connection is accepted while keeping the handle_conc_signal thread running 
        while (!client_accepted) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        std::cout << "CONNECTED TO A CLIENT!" << std::endl;     // client_accepted == true at this point -> control out of (sleep_for) loop
        std::string clientIP = socket->remote_endpoint().address().to_string();
        unsigned short clientPort = socket->remote_endpoint().port();
        std::cout << "CLIENT IP: " << clientIP << " CLIENT PORT: " << clientPort << "\n";

        // Check and create DB
        sqlite3* DB;
        const std::string path = "../lib/logs/";
        const std::string dbname = path + "msghist_" + clientIP + ".db";
        sqlite3_open(dbname.c_str(), &DB);

        // Create table if not exists
        std::string create_table = "CREATE TABLE IF NOT EXISTS MSG_LOGS("
                                   "ID INTEGER PRIMARY KEY AUTOINCREMENT,"
                                   "PERSON TEXT NOT NULL,"
                                   "MESSAGE TEXT NOT NULL,"
                                   "TIME TEXT NOT NULL);";
        execute_sql(DB, create_table);
        sqlite3_close(DB);

        // Start the key exchange socket thread and sleep until it is established
        std::thread keyex_socket_thread([&](){
            keyex_socket_accept(keyex_acceptor, keyex_socket);
        });
        keyex_socket_thread.detach();

        while(!keyex_socket_est){
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        //  Key exchange and messaging until user terminates
        while(true)
        {   
            if(!command_entered)
            {
                std::thread keyex_thread([&](){
                    start_keyex(keyex_socket, shared_key);
                });
                keyex_thread.detach();

                while(!key_exchange_status_server) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }

                key_exchange_status_server = false;
            }

            handle_client(socket, dbname, shared_key);
        }
    } catch (std::exception& e) {
        std::cerr << "Server exception: " << e.what() << "\n";
    }
}

#endif
