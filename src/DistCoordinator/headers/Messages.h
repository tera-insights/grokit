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

#ifndef _MESSAGES_H_
#define _MESSAGES_H_

#include "RemoteMessage.h"
#include "EventProcessor.h"

#include <string>
#include <vector>

/**
  Message sent by a node when it connects to the coordinator.
Arguments:
*/

class RegisterMessage : public RemoteMessage {
    public:
        ARCHIVER_ACCESS_DECLARATION;

        //members


    private:
        //constructor
        RegisterMessage(void )
        {
            // swap members
        }

        //constructor with the extra parameter typeIndicator
        RegisterMessage(bool _typeIndicator ) :  RemoteMessage(_typeIndicator)
    {
        // swap members
    }

    public:


        virtual ~RegisterMessage() {}

        // type
        static const off_t type=0x9519b9664f067fbbULL
            ;
        virtual off_t Type(void){ if (typeIndicator == true) return 0x9519b9664f067fbbULL
            ; else return RemoteMessage::Type();}
        virtual const char* TypeName(void){ return "RegisterMessage"; }

        // archiver
        ARCHIVER(ar){
            ar & ARCHIVER_BASE_CLASS(RemoteMessage) ;
        }

        // friend declarations
        friend void RegisterMessage_Factory(EventProcessor& dest );

};

//export class for serializer
ARCHIVER_CLASS_EXPORT(RegisterMessage)

    // Factory function to build RegisterMessage
    inline void RegisterMessage_Factory(EventProcessor& dest ){
        Message* msg = new RegisterMessage( false );
        dest.ProcessMessage(*msg);
    }


/**
  Message sent by a node when it disconnects from the coordinator.
Arguments:
*/

class UnregisterMessage : public RemoteMessage {
    public:
        ARCHIVER_ACCESS_DECLARATION;

        //members


    private:
        //constructor
        UnregisterMessage(void )
        {
            // swap members
        }

        //constructor with the extra parameter typeIndicator
        UnregisterMessage(bool _typeIndicator ) :  RemoteMessage(_typeIndicator)
    {
        // swap members
    }

    public:


        virtual ~UnregisterMessage() {}

        // type
        static const off_t type=0x8304f8ecfce1f659ULL
            ;
        virtual off_t Type(void){ if (typeIndicator == true) return 0x8304f8ecfce1f659ULL
            ; else return RemoteMessage::Type();}
        virtual const char* TypeName(void){ return "UnregisterMessage"; }

        // archiver
        ARCHIVER(ar){
            ar & ARCHIVER_BASE_CLASS(RemoteMessage) ;
        }

        // friend declarations
        friend void UnregisterMessage_Factory(EventProcessor& dest );

};

//export class for serializer
ARCHIVER_CLASS_EXPORT(UnregisterMessage)

    // Factory function to build UnregisterMessage
    inline void UnregisterMessage_Factory(EventProcessor& dest ){
        Message* msg = new UnregisterMessage( false );
        dest.ProcessMessage(*msg);
    }


/**
  Message sent by a node to the coordinator to show that it is alive.
  Message sent by the coordinator to a node to ask if it is still alive.
Arguments:
*/

class HeartbeatMessage : public RemoteMessage {
    public:
        ARCHIVER_ACCESS_DECLARATION;

        //members


    private:
        //constructor
        HeartbeatMessage(void )
        {
            // swap members
        }

        //constructor with the extra parameter typeIndicator
        HeartbeatMessage(bool _typeIndicator ) :  RemoteMessage(_typeIndicator)
    {
        // swap members
    }

    public:


        virtual ~HeartbeatMessage() {}

        // type
        static const off_t type=0x6c64ed892124aa6eULL
            ;
        virtual off_t Type(void){ if (typeIndicator == true) return 0x6c64ed892124aa6eULL
            ; else return RemoteMessage::Type();}
        virtual const char* TypeName(void){ return "HeartbeatMessage"; }

        // archiver
        ARCHIVER(ar){
            ar & ARCHIVER_BASE_CLASS(RemoteMessage) ;
        }

        // friend declarations
        friend void HeartbeatMessage_Factory(EventProcessor& dest );

};

//export class for serializer
ARCHIVER_CLASS_EXPORT(HeartbeatMessage)

    // Factory function to build HeartbeatMessage
    inline void HeartbeatMessage_Factory(EventProcessor& dest ){
        Message* msg = new HeartbeatMessage( false );
        dest.ProcessMessage(*msg);
    }


/**
  Message sent by query input generator to query coordinator when a new job is
  required to be performed.
Arguments:
*/

class NewJobMessage : public Message {
    public:
        //members


    private:
        //constructor
        NewJobMessage(void )
        {
            // swap members
        }


    public:
        //destructor
        virtual ~NewJobMessage() {}

        // type
        static const off_t type=0x9782629e2ea65e19ULL
            ;
        virtual off_t Type(void){ return 0x9782629e2ea65e19ULL
            ; }
        virtual const char* TypeName(void){ return "NewJobMessage"; }

        // friend declarations
        friend void NewJobMessage_Factory(EventProcessor& dest );

};

// Factory function to build NewJobMessage
inline void NewJobMessage_Factory(EventProcessor& dest ){
    Message* msg = new NewJobMessage();
    dest.ProcessMessage(*msg);
}


/**
  Message sent by query coordinator to the execution nodes with the structure of
  the execution tree.
Arguments:
- parent node as string
- children as vector<string>
*/

class NetworkConfigurationMessage : public RemoteMessage {
    public:
        ARCHIVER_ACCESS_DECLARATION;

        //members

        std::string parent;
        std::vector<std::string> children;

    private:
        //constructor
        NetworkConfigurationMessage(std::string _parent, std::vector<std::string> _children ):
            // copy constructed members
            parent(_parent), children(_children)
    {
        // swap members
    }

        //constructor with the extra parameter typeIndicator
        NetworkConfigurationMessage(std::string _parent, std::vector<std::string> _children, bool _typeIndicator ) : // copy constructed members
            parent(_parent), children(_children),  RemoteMessage(_typeIndicator)
    {
        // swap members
    }

    public:

        //empty constructor required for deserialization
        NetworkConfigurationMessage()    : RemoteMessage()  { }

        virtual ~NetworkConfigurationMessage() {}

        // type
        static const off_t type=0x8429612614339119ULL
            ;
        virtual off_t Type(void){ if (typeIndicator == true) return 0x8429612614339119ULL
            ; else return RemoteMessage::Type();}
        virtual const char* TypeName(void){ return "NetworkConfigurationMessage"; }

        // archiver
        ARCHIVER(ar){
            ar & ARCHIVER_BASE_CLASS(RemoteMessage)  & parent  & children ;
        }

        // friend declarations
        friend void NetworkConfigurationMessage_Factory(EventProcessor& dest ,std::string _parent, std::vector<std::string> _children);

};

//export class for serializer
ARCHIVER_CLASS_EXPORT(NetworkConfigurationMessage)

    // Factory function to build NetworkConfigurationMessage
    inline void NetworkConfigurationMessage_Factory(EventProcessor& dest ,std::string _parent, std::vector<std::string> _children){
        Message* msg = new NetworkConfigurationMessage(_parent, _children,  false );
        dest.ProcessMessage(*msg);
    }


#endif // _MESSAGES_H_
