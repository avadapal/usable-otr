#ifndef MESSAGE_HPP
#define MESSAGE_HPP

#include <iostream>
#include <sstream>
#include <boost/archive/binary_iarchive.hpp>    
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/string.hpp>

// Structure of the message being sent and received

class Message
{
    friend class boost::serialization::access;
    std::string enc_msg;
    std::string mac_tag;

    template <class Archive>
    void serialize(Archive& archive, const unsigned int version)
    {
        archive & enc_msg;
        archive & mac_tag;
    }

public:
    Message() {}

    Message(const std::string& enc_msg, const std::string& mac_tag)
    {
        this->enc_msg = enc_msg;
        this->mac_tag = mac_tag;
    }

    std::string get_enc_msg() const
    {
        return enc_msg;
    }

    std::string get_mac_tag() const
    {
        return mac_tag;
    }

    void set_enc_msg(const std::string& enc_msg)
    {
        this->enc_msg = enc_msg;
    }

    void set_mac_tag(const std::string& mac_tag)
    {
        this->mac_tag = mac_tag;
    }

    ~Message() {}
};

BOOST_CLASS_VERSION(Message, 1)

#endif
