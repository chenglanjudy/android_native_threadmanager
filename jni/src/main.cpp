#include "logging.h"
#include "Thread.h"
#include "MessageHandler.h"
#include "Looper.h"
#include "Message.h"
#include "MessageQueue.h"
#include "ThreadDefs.h"

using namespace ThreadManager;

int main(){

	ALOGD("Thread number %ld\n", pthread_self());

	HandleThread* ht = new HandleThread();
	int result = ht->run("Anthor Thread", PRIORITY_URGENT_DISPLAY);
	LOG_IF_ERRNO(result!=0,"Could not start Anthor thread due to error %d.", result);
	
	MessageHandlerInterface* mh = new MessageHandler(ht->getLooper(),ht->getLooper()->getQueue());
	mh->sendMessage(*(mh->obtainMessage()),systemTime(SYSTEM_TIME_MONOTONIC));

	LooperInterface* lis = new Looper();//Looper::myLooper();
	ALOGD("The main thread loop message.");
	lis->loopOnce();

	delete lis;
	delete ht;
	delete mh;
}