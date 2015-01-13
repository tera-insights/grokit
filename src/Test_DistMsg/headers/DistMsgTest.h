
// Copyright 2013 Tera Insights, LLC. All Rights Reserved.

#ifndef _DIST_MSG_TEST_H_
#define _DIST_MSG_TEST_H_

#include "EventGenerator.h"
#include "EventGeneratorImp.h"
#include "EventProcessorImp.h"
#include "EventProcessor.h"
#include "RemoteAddress.h"
#include "ProxyEventProcessor.h"
#include "MessageMacros.h"
#include "CommunicationFramework.h"

class TestSenderImp : public EventGeneratorImp {
    private:
        // Proxy to send the test messages
        ProxyEventProcessor target;

        // Current message ID
        int id;

    public:
        // Constructors / Destructor
        TestSenderImp() {}
        TestSenderImp( MailboxAddress & addr );

        virtual ~TestSenderImp() { }

        virtual int ProduceMessage(void);
};

class TestSender : public EventGenerator {
    public:
        TestSender() { }
        TestSender(MailboxAddress & addr ) {
            evGen = new TestSenderImp(addr);
        }

        virtual ~TestSender() { }

};

class TestReceiverImp : public EventProcessorImp {
    private:

    public:
        // Constructors / Destructor
        TestReceiverImp() {
            RegisterMessageProcessor(TestMessage::type, &ProcessTestMessage);
        }

        virtual ~TestReceiverImp() { }

        MESSAGE_HANDLER_DECLARATION(ProcessTestMessage);
};

class TestReceiver : public EventProcessor {
    public:
        TestReceiver() {
        }

        TestReceiver( std::string mailbox ) {
            evProc = new TestReceiverImp();

            RegisterAsRemoteEventProcessor(*this, mailbox);
        }

        virtual ~TestReceiver() { }
};

#endif // _DIST_MSG_TEST_H_
