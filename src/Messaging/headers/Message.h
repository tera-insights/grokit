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
#ifndef _MESSAGE_H_
#define _MESSAGE_H_

/**
	This header file contains the definition of the Message hierachy

	The code contains a base class, a macro to create derived classes
	and the instantiations of the macro for all types of messages needed.

	To define a new type of message, the message creation macro has to be
	called in the following manner:

	M4_CREATE_MESSAGE_TYPE(TypeName, listCopyCostructed, listSwapped)

	where the each of the lists looks like:

	</</ (att_name, att_type), .../>/>

	The attributes in the CopyConstructed list are copied in using assignment,
	the attributes in the swapped list are swaped in using the swap() method.

	Hopefully only basic types are copied in, the rest are swapped in.

	The macro will create a class, called TypeName that has members
	called as the attributes specified that are directly accessible.
	A private constructor that insantiates all the attributes in the order
	specified is created.

	The method Type() is automatically created and will return the numeric type
	associated with the message. The static int TypeName::type is also defined
	as a public constant member that allows the numeric type to be retrieved.

	When a method is registered to receive messages of type TypeName, the numeric
	registration can be done using TypeName::type

	The function TypeNameFactory(EventProcessor*, arguments) is automatically
	produced as well. It creates a message of type TypeName and sends it
	to the specified EventProcessor.

	To check that the message class is correctly generated, do:
	m4 -I m4_dir Message.h

	The output should look like a hand generated C++ code

	NOTE: all the copy constructed members are passed by value and all
	the swapped members by reference (for efficiency).

	WARNING: The messages needed to run tests should be placed into the file
	Messages-local.h.m4 in the test directory. This goes for the includes they
	need as well. The file Messages-local.h.m4 is included at the end of
	this file. It is fine if it is missing
*/

////////////////////////////////////////////////
// INCLUDES OF DATATYPES USED BY THE MESSAGES
/**
	The following lines should be uncommented if a debug class is used
	the name of the class shold replace MessageDebug
	define(DEBUG_CLASS, MessageDebug)
*/


#include "Config.h"
#include "EventProcessorImp.h"
#include "DistributedCounter.h"
#include "MessageMacros.h"
#include "SerializeJson.h"

//abstract message type definition
#define ABSTRACT_MESSAGE_TYPE -1


/**
	Base message class for all the messages that can be manipulated by
	the MultiMessageQueue class.

	Provides no functionality, just a basic type for messages. Allows only
	determining the type as a integer constant to allow conversion to the
	real type.

	When functionality is added to deal with another type of message,
	this class should be inherited from. The derived type can have as
	much intrnal structure and as many methods as desired. They will
	be accessible when the conversion to the true type is performed
	using dynamic_cast.

	For each derived class the static member type and the function Type()
	give the type as an off_t.
*/
class Message {
    Json::Value dummy;

public:
	// constructor doing nothing
	Message() {}

	virtual ~Message() {}

	// Every new message type should redefine this
	virtual off_t Type(void) const { return ABSTRACT_MESSAGE_TYPE; }
	virtual const char* TypeName(void) const { return "Message"; }

    // Method to determine if a message is serialized.
    virtual bool Serialized(void) const { return false; }

    virtual Json::Value & GetSerializedContent( void ) {
        FATAL("Attempted to get serialized data from non-serialized message.");
        return dummy;
    }

    // To/From JSON
    virtual void ToJson( Json::Value & dest ) const {
    }

    virtual void FromJson( const Json::Value & src ) {
    }
};

//////////// DIE MESSAGE /////////////
// THIS IS A SYSTEM WIDE MESSAGE, DO NOT ERASE
/**
	Message given to an EventProcessor when it's time to finish.
	The threads inside the EventProcessor are killed.
	Any EventProcessor automatically processes this type of message.

	Arguments: NONE
*/
class DieMessage : public Message {
private:
	//constructor
	DieMessage(void) {}

public:
	virtual ~DieMessage() {}

	// type
	static const off_t type=0xdef359b5a9bda8b6ULL;
	virtual off_t Type(void) const OVERRIDE_SPEC { return 0xdef359b5a9bda8b6ULL; }
	virtual const char* TypeName(void) const OVERRIDE_SPEC { return "DieMessage"; }

    DieMessage( Json::Value & val, void * dummy1, void * dummy2, void * dummy3 ) {
        FromJson(val);
    }

	// friend declarations
	friend void DieMessage_Factory(EventProcessor& dest);
};

// Factory function to build DieMessage
inline void DieMessage_Factory(EventProcessor& dest) {
	Message* msg = new DieMessage();
	dest.ProcessMessage(*msg);
}

#endif //  _MESSAGE_H_
