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
#ifndef _REMOTE_MESSAGE_MACROS_
#define _REMOTE_MESSAGE_MACROS_

// macro to create a message from a remote message. Same meaning as the above
#define REMOTE_MESSAGE_HANDLER_DEFINITION_BEGIN(ClassName, MessageHandler, MessageType)\
void ClassName::MessageHandler(EventProcessorImp &_obj, Message &_msg){\
	RemoteMessageContainer &rMsg = dynamic_cast<RemoteMessageContainer&>(_msg);\
	\
	std::istringstream ss(std::ios::binary);\
	ss.str(rMsg.GetSerializedContent());\
	\
	BINARY_ARCHIVE_INPUT ia(ss);\
	\
	RemoteMessage *tMsg = NULL;\
	ia >> tMsg;\
	MessageType &msg = dynamic_cast<MessageType&>(*tMsg);\
	\
	ClassName &evProc = dynamic_cast<ClassName&>(_obj);

// Use of this macro is crucial -- memory leaks otherwise
#define REMOTE_MESSAGE_HANDLER_DEFINITION_END \
	delete tMsg; \
}


#endif // _REMOTE_MESSAGE_MACROS_
