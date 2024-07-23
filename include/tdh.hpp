#ifndef TDH_HPP
#define TDH_HPP

#define KEYEX_INIT std::string("INIT_DHKE")              // Request to initiate key exchange
#define KEYEX_INIT_ACK std::string("INIT_DHKE_ACK")      // Acknowledgement to the request indicating the server is ready for key exchange

#include <iostream>
#include <boost/asio.hpp>
#include <atomic>
#include <thread>
#include <botan/kdf.h>
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
        const std::string kdf = "SP800-56A(SHA-256)";
        
       // Client generates first DH key pair
        Botan::DH_PrivateKey client_private_key1(rng, domain); // a
        auto client_public_key1 = client_private_key1.public_value();  // A = g^a 
        crypto::send_pubkey(keyex_socket, Botan::hex_encode(client_public_key1));
        client_pk_sent = true;

        // Receive the server's public key
        auto server_public_key1 = Botan::hex_decode(crypto::receive_pubkey(keyex_socket)); // B = g^b
        server_pk_sent = false;     // Set this flag back to false after receiving the key

        // Client generates second DH key pair
        Botan::DH_PrivateKey client_private_key2(rng, domain); // x
        auto client_public_key2 = client_private_key2.public_value(); // X = g^x
        crypto::send_pubkey(keyex_socket, Botan::hex_encode(client_public_key2));
        auto server_public_key2 = Botan::hex_decode(crypto::receive_pubkey(keyex_socket)); // Y = g^y

        // Key agreement for S1, S2, S3
        Botan::PK_Key_Agreement client_key1(client_private_key1, rng, kdf);
        Botan::PK_Key_Agreement client_key2(client_private_key2, rng, kdf);

        auto shared_key1 = client_key1.derive_key(32, server_public_key2).bits_of(); // S1 = Y^a
        auto shared_key2 = client_key2.derive_key(32, server_public_key1).bits_of(); // S2 = B^x
        auto shared_key3 = client_key2.derive_key(32, server_public_key2).bits_of(); // S3 = Y^x

        // Concatenate shared keys and hash
        Botan::secure_vector<uint8_t> key;
        key.insert(key.end(), shared_key1.begin(), shared_key1.end());
        key.insert(key.end(), shared_key2.begin(), shared_key2.end());
        key.insert(key.end(), shared_key3.begin(), shared_key3.end());

        auto hash = Botan::HashFunction::create_or_throw("SHA-512");
        hash->update(key.data(), key.size());
        auto key_hash = hash->final();

        auto kdf2 = Botan::KDF::create_or_throw("SP800-56A(SHA-256)");
        shared_key = kdf2->derive_key(32,key_hash);

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
    const std::string kdf = "SP800-56A(SHA-256)";

    // Server generates first DH key pair
    Botan::DH_PrivateKey server_private_key1(rng, domain);   // b
    auto server_public_key1 = server_private_key1.public_value();   // B = g^b

    // Receive the client's public key and send the computed server's public key
    client_pk_sent = false; 
     auto client_public_key1 = Botan::hex_decode(crypto::receive_pubkey(keyex_socket)); // A = g^a
    crypto::send_pubkey(keyex_socket, Botan::hex_encode(server_public_key1));
    server_pk_sent = true;

    // Server generates second DH key pair
    Botan::DH_PrivateKey server_private_key2(rng, domain);   // y
    auto server_public_key2 = server_private_key2.public_value(); // Y = g^y
    auto client_public_key2 = Botan::hex_decode(crypto::receive_pubkey(keyex_socket)); // X = g^x
    crypto::send_pubkey(keyex_socket, Botan::hex_encode(server_public_key2));

    // Key agreement for S1, S2, S3
    Botan::PK_Key_Agreement server_key1(server_private_key1, rng, kdf);
    Botan::PK_Key_Agreement server_key2(server_private_key2, rng, kdf);

    auto shared_key1 = server_key2.derive_key(32, client_public_key1).bits_of();  // S1 = A^y
    auto shared_key2 = server_key1.derive_key(32, client_public_key2).bits_of();  // S2 = X^b
    auto shared_key3 = server_key2.derive_key(32, client_public_key2).bits_of();  // S3 = X^y

    // Concatenate shared keys and hash
    Botan::secure_vector<uint8_t> key;
    key.insert(key.end(), shared_key1.begin(), shared_key1.end());
    key.insert(key.end(), shared_key2.begin(), shared_key2.end());
    key.insert(key.end(), shared_key3.begin(), shared_key3.end());

    auto hash = Botan::HashFunction::create_or_throw("SHA-512");
    hash->update(key.data(), key.size());
    auto key_hash = hash->final();

    auto kdf2 = Botan::KDF::create_or_throw("SP800-56A(SHA-256)");
    shared_key = kdf2->derive_key(32,key_hash);
    key_exchange_status_server = true;
}


#endif
