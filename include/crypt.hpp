    #ifndef CRYPT_HPP
    #define CRYPT_HPP

    #include <botan/auto_rng.h>
    #include <botan/ecdh.h>
    #include <botan/hex.h>
    #include <botan/pubkey.h>
    #include <botan/dl_group.h>
    #include <botan/dh.h>
    #include <botan/cipher_mode.h>
    #include <botan/filters.h>
    #include <boost/asio.hpp>

    using tcp = boost::asio::ip::tcp;

    namespace crypto
    {
        static std::string compute_mac(const std::string& msg, const Botan::secure_vector<uint8_t>& key)
        {
            auto hmac = Botan::MessageAuthenticationCode::create_or_throw("HMAC(SHA-256)");
            hmac->set_key(key);
            hmac->update(msg);

            return Botan::hex_encode(hmac->final());
        }

        void send_pubkey(tcp::socket& socket, const std::string& key_pub)
        {
            boost::asio::write(socket, boost::asio::buffer(key_pub));
        }

        std::string receive_pubkey(tcp::socket& socket)
        {
            char data[2048];
            boost::system::error_code error;
            size_t length = socket.read_some(boost::asio::buffer(data), error);
            if (error && error != boost::asio::error::eof) {
                throw boost::system::system_error(error); // Some other error
            }

            std::string key_raw(data, length);
            return key_raw;
        }

        std::string encrypt_message(const Botan::secure_vector<uint8_t>& key, const std::string& message)
        {
            Botan::AutoSeeded_RNG rng;

            const auto enc = Botan::Cipher_Mode::create_or_throw("AES-256/CBC/PKCS7", Botan::Cipher_Dir::Encryption);
            enc->set_key(key);

            auto iv = rng.random_vec(enc->default_nonce_length());

            Botan::secure_vector<uint8_t> pt(message.data(), message.data() + message.size());
            enc->start(iv);
            enc->finish(pt);

            std::string encrypted_message = Botan::hex_encode(iv) + Botan::hex_encode(pt);
            return encrypted_message;
        }

        std::string decrypt_message(const Botan::secure_vector<uint8_t>& key, const std::string& encrypted_message)
        {
            Botan::AutoSeeded_RNG rng;

            const auto dec = Botan::Cipher_Mode::create_or_throw("AES-256/CBC/PKCS7", Botan::Cipher_Dir::Decryption);
            dec->set_key(key);

            size_t iv_size = 2 * dec->default_nonce_length();
            auto iv = Botan::hex_decode(encrypted_message.substr(0,iv_size));
            auto ciphertext = Botan::hex_decode(encrypted_message.substr(iv_size));

            dec->start(iv);
            dec->finish(ciphertext);

            return std::string(ciphertext.begin(), ciphertext.end());
        }
    }


    #endif
