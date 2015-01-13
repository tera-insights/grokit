
/*
 * Copyright 2013 Tera Insights, LLC. All Rights Reserved.
 *
 * This file has been automatically generated.
 *
 * Source:          Test_DistMsg/php/TestMessages.h.php
 * Generated on:    2013-08-14T10:37:11-0400
 *
 * Do not make modifications to this file, they will be wiped out on the next
 * compilation. Please make modifications to the source file instead.
 */#include "Swap.h"
#include "Message.h"
#include "SerializeJson.h"

#ifndef _TEST_MESSAGES_H_
#define _TEST_MESSAGES_H_


class TestMessage : public Message {
public:
    // members
    int id;
    Json::Value msg;

private:
    // constructor
    TestMessage ( int id, Json::Value & msg ) :
        Message()
        // Copy constructed members
        , id( id )
    {
        // swapped members
        (this->msg).swap(msg);
    }

    // Default constructor
    TestMessage ( void ) : Message() { }

public:
    // Destructor
    virtual ~TestMessage() {}

    // type
    static const off_t type = 0x08ff08d3b2981eb6ULL;
    virtual off_t Type(void) const OVERRIDE_SPEC { return 0x08ff08d3b2981eb6ULL; }
    virtual const char * TypeName(void) const OVERRIDE_SPEC { return "TestMessage"; }

    // To/From Json
    virtual void ToJson( Json::Value & dest ) const OVERRIDE_SPEC {
        dest = Json::Value(Json::objectValue);

        ::ToJson(id, dest["id"]);
        ::ToJson(msg, dest["msg"]);
    }

    virtual void FromJson ( const Json::Value & src ) OVERRIDE_SPEC {
        if( ! src.isObject() ) {
            throw new std::invalid_argument("Tried to construct TestMessage message from non-object JSON");
        }

        if( ! src.isMember("id") )
            throw new std::invalid_argument("JSON for message TestMessage has no member for attribute id");
        ::FromJson(src["id"], id);
        if( ! src.isMember("msg") )
            throw new std::invalid_argument("JSON for message TestMessage has no member for attribute msg");
        ::FromJson(src["msg"], msg);
    }


    // Constructor from JSON
    // This constructor has a bizarre signature on purpose as not to conflict
    // with messages that contain exactly 1 JSON value as their payload.
    // It is our hope that no sane individual would store 3 void pointers in a
    // message.
    TestMessage( const Json::Value & src, void * dummy1, void * dummy2, void * dummy3 ) {
        FromJson(src);
    }

    // friend delcarations
    friend void TestMessage_Factory( EventProcessor & __dest, int id, Json::Value & msg );
}; // End of class TestMessage

// Factory function to build TestMessage objects
inline
void TestMessage_Factory( EventProcessor & __dest, int id, Json::Value & msg ) {
    Message * __msg = (Message *) new TestMessage( id, msg );
    __dest.ProcessMessage(*__msg);
}


#endif // _TEST_MESSAGES_H_
