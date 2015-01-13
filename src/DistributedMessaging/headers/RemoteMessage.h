//
//  Copyright 2012 Alin Dobra and Christopher Jermaine
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
#ifndef _REMOTE_MESSAGE_H_
#define _REMOTE_MESSAGE_H_

#include "Message.h"
#include "Swap.h"
#include "RemoteAddress.h"
#include "Errors.h"

#include <string>
#include <cinttypes>

//remote message type definition
#define REMOTE_MESSAGE_TYPE 1

/**
 *  The RemoteMessage class is used as a container for messages to be sent
 *  out from the system to a remote actor.
 */

class RemoteMessage : public Message {
    private:
        // The source address of the message
        HostAddress srcAddr;

        // The destination address of the message
        MailboxAddress destAddr;

        // The serialized payload of the message
        Json::Value payload;

        // The type of the message
        off_t payloadType;

    public:
        RemoteMessage( HostAddress & _source, MailboxAddress & _dest, off_t _type, Json::Value & _payload ) :
            srcAddr(_source),
            destAddr(_dest),
            payloadType(_type)
        {
            payload.swap(_payload);
        }

        RemoteMessage( const Json::Value & val, void * dummy1, void * dummy2, void * dummy3 ) {
            FATAL("RemoteMessages should never be deserialized! They can only be sent!");
        }

        virtual ~RemoteMessage() {}

        // Every new message type should redefine this
        static const off_t type = REMOTE_MESSAGE_TYPE;

        virtual off_t Type(void) const OVERRIDE_SPEC { return REMOTE_MESSAGE_TYPE; }
        virtual const char* TypeName(void) const OVERRIDE_SPEC { return "RemoteMessage"; }

        // Methods to get the various parts of the message
        HostAddress getSource(void) const { return srcAddr; }
        MailboxAddress getDest(void) const { return destAddr; }
        Json::Value & getPayload(void) { return payload; }
        off_t getPayloadType(void) const { return payloadType; }

        // RemoteMessage should be serialized only, so only overload the ToJson method.
        void ToJson( Json::Value & dest ) const {
            dest = Json::Value(Json::objectValue);

            ::ToJson(srcAddr, dest["source"]);
            ::ToJson(destAddr, dest["dest"]);
            dest["type"] = (Json::Int64) payloadType;
            dest["payload"] = payload;
        }

        friend void RemoteMessage_Factory(EventProcessor & dest,
                HostAddress & srcAddr, MailboxAddress & destAddr, off_t type,
                Json::Value & payload );
};

inline void RemoteMessage_Factory( EventProcessor & dest, HostAddress & srcAddr,
        MailboxAddress & destAddr, off_t type, Json::Value & payload ) {
    Message * msg = new RemoteMessage(srcAddr, destAddr, type, payload);
    dest.ProcessMessage(*msg);
}

/**
  Container class for all the messages that are recieved.

  This message is used as a container for the serialized message.
  The content is used to unpack the actual message.

  */
class RemoteMessageContainer : public Message {
    private:
        // The source of the message
        HostAddress source;

        // The destination of the message
        MailboxAddress dest;

        // The message payload, which will be converted to the correct type by
        // the handler of the message.
        Json::Value payload;

        // The type of the message that we will masquerade as.
        off_t type;

    public:
        // Constructor from partially processed data from the network
        RemoteMessageContainer( HostAddress & _source, MailboxAddress & _dest, off_t _type, Json::Value & _payload ) :
            source(_source),
            dest(_dest),
            type(_type)
        {
            payload.swap(_payload);
        }

        RemoteMessageContainer( const Json::Value & src, void * d1, void * d2, void * d3 ) {
            FromJson(src);
        }

        virtual ~RemoteMessageContainer(void){}

        virtual off_t Type(void) const OVERRIDE_SPEC { return type; }

        virtual bool Serialized(void) const OVERRIDE_SPEC { return true; }

        // this interface is only for extracting the real message from the container
        // this function gives up the string after this call
        virtual Json::Value & GetSerializedContent(void) OVERRIDE_SPEC { return payload; }

        // Returns the address of the source of the message
        HostAddress GetSource(void) { return source; }

        void FromJson( const Json::Value & src ) {
            ::FromJson(src["source"], source);
            ::FromJson(src["dest"], dest);
            ::FromJson(src["type"], type);
            payload = src["payload"];
        }

        friend void RemoteMessageContainer_Factory(EventProcessor & dest,
                HostAddress & srcAddr, MailboxAddress & destAddr, off_t type,
                Json::Value & payload );
};

inline void RemoteMessageContainer_Factory( EventProcessor & dest, HostAddress & srcAddr,
        MailboxAddress & destAddr, off_t type, Json::Value & payload ) {
    Message * msg = new RemoteMessageContainer(srcAddr, destAddr, type, payload);
    dest.ProcessMessage(*msg);
}

#endif // _REMOTE_MESSAGE_H_
