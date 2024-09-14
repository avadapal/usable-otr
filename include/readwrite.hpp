#ifndef READWRITE_HPP
#define READWRITE_HPP

#include <boost/asio.hpp>
#include <boost/archive/text_oarchive.hpp>  // boost::archive::text_oarchive
#include <boost/archive/text_iarchive.hpp>  // boost::archive::text_iarchive
#include <iostream>
#include <string>
#include <chrono>
#include <atomic>
#include "messageops.hpp"
// #include "keyex.hpp"
#include "tdh.hpp"
#include "message.hpp"

using clk = std::chrono::system_clock;
using tcp = boost::asio::ip::tcp;
using msg = std::vector<std::pair<std::chrono::time_point<clk>, std::string>>;
namespace asio = boost::asio;

std::atomic<bool> ds_enabled = false;

void read_data_packet(tcp::socket& socket, Message& msg)
{   
    // Read the size of the buffer vector in the socket
    uint32_t data_size;
    boost::asio::read(socket, boost::asio::buffer(&data_size, sizeof(data_size)));  
    data_size = ntohl(data_size);   // Convert data size from network byte back to normal byte order

    // Read the serialized string from the socket
    std::vector<char> serialized_data(data_size);
    boost::asio::read(socket, boost::asio::buffer(serialized_data.data(), data_size));

    // Deserialize the received string back to the message object
    std::istringstream archive_stream(std::string(serialized_data.begin(), serialized_data.end()));
    boost::archive::text_iarchive archive(archive_stream);
    archive >> msg;
}

void send_data_packet(tcp::socket& socket, const Message& msg)
{   
    // Serialize the message object to a message string to be sent over the socket
    std::ostringstream archive_stream;
    boost::archive::text_oarchive archive(archive_stream);
    archive << msg;
    std::string serialized_data = archive_stream.str();     // Serialized string containing the object

    uint32_t data_size = static_cast<uint32_t>(serialized_data.size());
    data_size = htonl(data_size);   // Convert the data size to a network byte

    // Push the serialized string and data size to buffers
    std::vector<boost::asio::const_buffer> buffers;
    buffers.push_back(boost::asio::buffer(&data_size, sizeof(data_size)));  // Buffer to hold the data size
    buffers.push_back(boost::asio::buffer(serialized_data));    // Buffer to hold the serialized data

    // Write buffers to the socket
    boost::asio::write(socket, buffers);
}

void read_from_socket(tcp::socket& socket, const std::string& dbname, const Botan::secure_vector<uint8_t>& key, const std::string& ds_pass) {
    try 
    {
        // Create a DB pointer and load the message DB for logging 
        sqlite3* DB;
        sqlite3_open(dbname.c_str(), &DB);

        char data[2048];
        boost::system::error_code error;
        if (error == boost::asio::error::eof) {
            return; // Connection closed by peer
        } else if (error) {
            throw boost::system::system_error(error); // Some other error
        }

        // Read message packet from the socket
        Message msg_pkt;
        read_data_packet(socket, msg_pkt);

        std::string message = crypto::decrypt_message(key, msg_pkt.get_enc_msg());  // Decrypt the message using shared key

        // Compute MAC tag and verify
        std::string hmac_tag = crypto::compute_mac(message,key);
        if(hmac_tag != msg_pkt.get_mac_tag())
        {
            std::cout<<"MAC TAGS MISMATCH!"<<std::endl<<"TERMINATING.."<<std::endl;
            exit(0);
        }

        if(ds_enabled)
        {
            // Verify the digital signature
            auto ds_key = Botan::PKCS8::load_key(msg_pkt.get_serial_pk_key(), ds_pass);
            Botan::PK_Verifier verifier(*ds_key, "SHA-256");
            verifier.update(message);
            if(!verifier.check_signature(msg_pkt.get_signature()))
            {
                std::cout<<"SIGNATURE VERIFICATION FAILED!"<<std::endl<<"TERMINATING.."<<std::endl;
                exit(0);
            }
        }

        // Log the message if MAC tag is verified and signature is verified
        std::time_t timestamp = clk::to_time_t(clk::now());
        if (!(message == ":e" || message == ":v" || message == ":h" || message == ":d" || message == ":q"))
        {
            std::string person = "USER";
            std::string time_str = std::ctime(&timestamp);
            time_str.pop_back();

            insert_message(DB, person, message, time_str);

            std::cout << "Message received: " << message << "\n";
        }
        
        sqlite3_close(DB);
    } catch (std::exception& e) {
        std::cerr << "READ ERROR: " << e.what() << "\n";
    }
}

void write_to_socket(tcp::socket& socket, const std::string& dbname, const Botan::secure_vector<uint8_t>& key, const std::string& ds_pass) {
    try 
    {
        sqlite3* DB;
        sqlite3_open(dbname.c_str(), &DB);

        // Take the user message input
        std::string message;
        std::cout << "Enter the message: (Enter :h for help)\n";
        std::getline(std::cin, message);

        // Set the command entered status as false so that key exchange in next iteration is not disturbed after handling the command operation
        command_entered = false;

        if (!(message == ":e" || message == ":v" || message == ":h" || message == ":d" || message == ":q")) // No commands have been entered
        {
            // Encrypt the message with the key before sending and compute MAC tag
            std::string enc_msg = crypto::encrypt_message(key, message);
            std::string mac_tag = crypto::compute_mac(message,key);
            Message msg_pkt(enc_msg,mac_tag);
            if(ds_enabled)
            {
                // Sign using ECDSA Private Key
                Botan::AutoSeeded_RNG rng;
                Botan::ECDSA_PrivateKey ds_key(rng, Botan::EC_Group("secp521r1"));
                Botan::PK_Signer signer(ds_key, rng, "SHA-256");
                signer.update(message);
                std::vector<uint8_t> signature = signer.signature(rng);
                std::vector<uint8_t> serial_pk_key = Botan::PKCS8::BER_encode(ds_key, rng, ds_pass);

                // Bind the encrypted message and MAC tag along with the signature in a packet and send it over the socket
                Message msg_pkt_ds(enc_msg, mac_tag, signature, serial_pk_key);
                msg_pkt = msg_pkt_ds;
            }
            send_data_packet(socket, msg_pkt);
        } else command_entered = true;


        // Log the sent message
        std::time_t timestamp = clk::to_time_t(clk::now());
        if (!executeCommands(message, dbname))       // Execute the function for respective command (if entered)
        {        
            std::string person = "YOU";
            std::string time_str = std::ctime(&timestamp);
            time_str.pop_back(); // remove the newline character

            insert_message(DB, person, message, time_str);

            std::cout << "Message Sent!\n";
            std::cout << "-----------------\n";
        }

        sqlite3_close(DB);

    } catch (std::exception& e) {
        std::cerr << "Write exception: " << e.what() << "\n";
    }
}


#endif
