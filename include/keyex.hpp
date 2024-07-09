#ifndef KEYEX_HPP
#define KEYEX_HPP

#define KEYEX_INIT std::string("INIT_DHKE")              // Request to initiate key exchange
#define KEYEX_INIT_ACK std::string("INIT_DHKE_ACK")      // Acknowledgement to the request indicating the server is ready for key exchange

#include <iostream>
#include <boost/asio.hpp>
#include <atomic>
#include <thread>
#include "crypt.hpp"

using tcp = boost::asio::ip::tcp;

// Global atomic variables for updating the status of the key exchange 
std::atomic<bool> init_dhke_flag = false;
std::atomic<bool> init_dhke_ack_flag = false;
std::atomic<bool> client_pk_sent = false;
std::atomic<bool> server_pk_sent = false;
std::atomic<bool> key_exchange_status_server = false; // False = Key Exchange Not done && True = Done
std::atomic<bool> key_exchange_status_client = false;

void key_exchange_client(tcp::socket& keyex_socket, Botan::secure_vector<uint8_t>& shared_key)
{
    char data[2048];
    boost::system::error_code error;

    // Send key-exchange initiation request
    boost::asio::write(keyex_socket, boost::asio::buffer(KEYEX_INIT), error);
    if (error) {
        std::cerr << "Client: Error sending INIT_DHKE: " << error.message() << "\n";
        return;
    }
    init_dhke_flag = true;
    

    size_t length = keyex_socket.read_some(boost::asio::buffer(data), error);
    if (error) {
        std::cerr << "Client: Error reading INIT_DHKE_ACK: " << error.message() << "\n";
        return;
    }

    std::string recv_data(data,length);
    if(recv_data == KEYEX_INIT_ACK)
    {
        init_dhke_ack_flag = false;

        // Compute Client's DH key pair after receiving INIT_DHKE_ACK from the server
        Botan::AutoSeeded_RNG rng;
        Botan::DL_Group domain("modp/ietf/1536");
        const std::string kdf = "KDF2(SHA-256)";
        Botan::DH_PrivateKey client_private_key(rng, domain);

        // Send the public key to the server
        const auto client_public_key = client_private_key.public_value();
        crypto::send_pubkey(keyex_socket, Botan::hex_encode(client_public_key));
        client_pk_sent = true;

        // Receive the server's public key
        const auto server_public_key = Botan::hex_decode(crypto::receive_pubkey(keyex_socket));
        server_pk_sent = false;     // Set this flag back to false after receiving the key

        // Compute the 256-bit shared key
        Botan::PK_Key_Agreement client_key(client_private_key, rng, kdf);
        shared_key = client_key.derive_key(32, server_public_key).bits_of();
        key_exchange_status_client = true;
    }
}   

void key_exchange_server(tcp::socket& keyex_socket, Botan::secure_vector<uint8_t>& shared_key)
{
    char data[2048];
    boost::system::error_code error;
    
    // Wait until the client sends INIT_DHKE and then read
    keyex_socket.wait(tcp::socket::wait_type::wait_read);
    size_t length = keyex_socket.read_some(boost::asio::buffer(data), error);
    if (error) {
        std::cerr << "Server: Error reading INIT_DHKE: " << error.message() << "\n";
        return;
    }
    std::string recv_data(data,length);
    
    // Send INIT_DHKE_ACK back to the client to initiate key exchange
    if(recv_data == KEYEX_INIT)
    {
        boost::asio::write(keyex_socket, boost::asio::buffer(KEYEX_INIT_ACK), error);
        if (error) {
            std::cerr << "Server: Error sending INIT_DHKE_ACK: " << error.message() << "\n";
            return;
        }
        init_dhke_ack_flag = true;  
        init_dhke_flag = false;     // Set this flag back to false
    } else return;

    // Compute server side's public key 
    Botan::AutoSeeded_RNG rng;
    Botan::DL_Group domain("modp/ietf/1536");
    const std::string kdf = "KDF2(SHA-256)";
    Botan::DH_PrivateKey server_private_key(rng, domain);
    const auto server_public_key = server_private_key.public_value();    

    // Receive the client's public key and send the computed server's public key
    client_pk_sent = false; 
    const auto client_public_key = Botan::hex_decode(crypto::receive_pubkey(keyex_socket));
    crypto::send_pubkey(keyex_socket, Botan::hex_encode(server_public_key));
    server_pk_sent = true;

    // Compute the 256-bit shared key 
    Botan::PK_Key_Agreement server_key(server_private_key, rng, kdf);
    shared_key = server_key.derive_key(32, client_public_key).bits_of();
    key_exchange_status_server = true;
}

#endif
