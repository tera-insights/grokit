// Copyright 2013 Tera Insights, LLC. All Rights Reserved.

#ifndef _REMOTE_ADDRESS_H_
#define _REMOTE_ADDRESS_H_

#include <string>
#include <cinttypes>
#include <sstream>

#include "HashFunctions.h"
#include "Swap.h"
#include "CommConfig.h"
#include "SerializeJson.h"

/*
 * This represents an address of a machine (hostname and port).
 *
 * By default, the port is the value that is set by the REMOTE_MESSAGING_PORT in
 * Constants.h
 */
class HostAddress {
    public:
        // The hostname of the machine
        std::string hostname;

        // The port used for communication by this machine.
        uint16_t port;

        HostAddress() :
            hostname(""),
            port(0)
        {
        }

        HostAddress( const std::string & _host, const uint16_t _port = comm::LISTEN_PORT_WS ) :
            hostname(_host),
            port(_port)
        {
        }

        // Copy constructor
        HostAddress( const HostAddress & other ) :
            hostname(other.hostname),
            port(other.port)
        {

        }

        // == and < operator for use in maps
        bool operator == (const HostAddress& other ) const {
            return (hostname == other.hostname) && (port == other.port);
        }

        bool operator < (const HostAddress & other ) const {
            int nameComp = hostname.compare(other.hostname);

            return (nameComp < 0) || (nameComp == 0 && port < other.port);
        }

        // IsEqual and LessThan for (In)EfficientMap
        bool IsEqual( const HostAddress & other ) const {
            return *this == other;
        }

        bool LessThan( const HostAddress & other ) const {
            return *this < other;
        }

        // swap
        void swap( HostAddress & other ) {
            SWAP_STD(hostname, other.hostname);
            SWAP_STD(port, other.port);
        }

        void ToJson( Json::Value & dest ) const {
            dest = Json::Value(Json::objectValue);
            dest["hostname"] = hostname;
            dest["port"] = (Json::UInt) port;
        }

        void FromJson( const Json::Value & src ) {
            hostname = src["hostname"].asString();
            port = src["port"].asUInt();
        }

        // String representation
        std::string str( void ) const {
            std::ostringstream ss;
            ss << hostname << ":" << port;

            return ss.str();
        }
};

inline
void ToJson(const HostAddress & src, Json::Value & dest){
  src.ToJson(dest);
}

inline
void FromJson(const Json::Value & src, HostAddress & dest){
  dest.FromJson(src);
}

// Overload of swap
void swap( HostAddress & a, HostAddress & b );

// inlined definition
inline
void swap( HostAddress & a, HostAddress & b ) {
    a.swap(b);
}

// Specialization of std::hash for HostAddress (for use in unordered_map/set)
namespace std {
    template<>
    struct hash<HostAddress>
    {
        public:
            std::size_t operator() (HostAddress const & s) const {
                return CongruentHash(std::hash<std::string>()(s.hostname), s.port);
            }
    };
}

class MailboxAddress {
    public:
        // The address of the host
        HostAddress host;

        // The name of the mailbox on the host that this address corresponds to.
        std::string mailbox;

        //constructors
        MailboxAddress() {
            host = HostAddress("");
            mailbox = "";
        }

        MailboxAddress( const HostAddress & _host, const std::string & _mailbox ) :
            host(_host), mailbox(_mailbox)
        {
        }

        // Copy constructor
        MailboxAddress( const MailboxAddress & other ) :
            host(other.host),
            mailbox(other.mailbox)
        {
        }

        // == and < operator for use in maps
        bool operator == (const MailboxAddress& other ) const {
            return (host == other.host) && (mailbox == other.mailbox);
        }

        bool operator < (const MailboxAddress & other ) const {
            return (host < other.host) || (host == other.host && mailbox < other.mailbox);
        }

        // IsEqual and LessThan for (In)EfficientMap
        // Just forwarded to the operators
        bool IsEqual( const MailboxAddress & other ) const {
            return *this == other;
        }

        bool LessThan( const MailboxAddress & other ) const {
            return *this < other;
        }

        // swap function so we can use swapping paradigm
        void swap(MailboxAddress& other){
            SWAP_STD(host, other.host);
            SWAP_STD(mailbox, other.mailbox);
        }

        void ToJson( Json::Value & dest ) const {
            dest = Json::Value(Json::objectValue);
            dest["mailbox"] = mailbox;
            host.ToJson(dest["host"]);
        }

        void FromJson( const Json::Value & src ) {
            host.FromJson(src["host"]);
            mailbox = src["mailbox"].asString();
        }

        // String representation
        std::string str( void ) const {
            std::ostringstream ss;

            ss << host.str() << ":" << mailbox;

            return ss.str();
        }
};

inline
void ToJson( const MailboxAddress& src, Json::Value & dest){
  src.ToJson(dest);
}

inline
void FromJson( const Json::Value & src, MailboxAddress& dest){
  dest.FromJson(src);
}

// Overload of swap
void swap( MailboxAddress & a, MailboxAddress & b );

// inlined definition
inline
void swap( MailboxAddress & a, MailboxAddress & b ) {
    a.swap(b);
}

// Specialization of std::hash for HostAddress (for use in unordered_map/set)
namespace std {
    template<>
    struct hash<MailboxAddress>
    {
        public:
            std::size_t operator() (MailboxAddress const & s) const {
                return CongruentHash(std::hash<HostAddress>()(s.host), std::hash<std::string>()(s.mailbox));
            }
    };
}

#endif // _REMOTE_ADDRESS_H_
