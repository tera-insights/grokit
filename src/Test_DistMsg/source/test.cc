#include "TestMessages.h"
#include "DistMsgTest.h"
#include "CommManager.h"
#include "CommunicationFramework.h"
#include "Errors.h"
#include "Logging.h"
#include "MessageMacros.h"

#include <ctime>
#include <iostream>

using namespace std;

TestSenderImp :: TestSenderImp( MailboxAddress & addr ) :
    id(0)
{
    CommManager & commMan = CommManager::GetManager();

    int ret = commMan.FindRemoteEventProcessor(addr, target);

    FATALIF( ret != 0, "Failed to get RemoteEventProcessor for our address");
}

int TestSenderImp :: ProduceMessage(void) {
    timespec to_sleep = { 5, 0 };
    timespec remaining;

    // Sleep for DURATION
    while( nanosleep( &to_sleep, &remaining) != 0 ) {
        to_sleep = remaining;
    }

    // Send a message
    Json::Value msg = "I'm a message!";
    cerr << "Sending message: [" << id << "] " << msg.asString() << endl;;
    TestMessage_Factory(target, id, msg);

    ++ id;

    return id < 10 ? 0 : 1;
}

MESSAGE_HANDLER_DEFINITION_BEGIN ( TestReceiverImp, ProcessTestMessage, TestMessage) {
    int id = msg.id;
    auto message = msg.msg;

    cout << "Received: [" << id << "]: " << message.asString() << endl;

} MESSAGE_HANDLER_DEFINITION_END

int main( void ) {
    StartLogging();
    StartCommunicationFramework();

    HostAddress hostAddr("localhost", 9701);
    MailboxAddress mailAddr(hostAddr, "test");

    TestReceiver tReceiver("test");
    tReceiver.ForkAndSpin();

    TestSender tSender(mailAddr);
    tSender.Run();

    for( int i = 0; i < 15; ++i ) {
        timespec to_sleep = { 5, 0 };
        timespec remaining;

        while( nanosleep( &to_sleep, &remaining ) != 0 ) {
            to_sleep = remaining;
        }
    }

    tSender.Kill();
    KillEvProc(tReceiver);
    tReceiver.WaitForProcessorDeath();

    StopCommunicationFramework();
    StopLogging();
}
