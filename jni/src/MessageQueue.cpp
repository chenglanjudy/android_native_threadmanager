/*
 * Copyright (C) ThreadManager Module Project.
 * @Author chenglan@ucweb.com 
 * @Date 2014-12-30
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#include "MessageQueue.h"
#include "Mutex.h"
#include "logging.h"

namespace ThreadManager{

template<typename T>
inline static T min(const T& a, const T& b) {
    return a < b ? a : b;
}

//-------- MesagePublisher -------

MessagePublisher::MessagePublisher(MessageHandler::Callback* queue)
					:mQueue(queue)
{
	ALOGD("Thread number %ld function= %s line=%d\n", pthread_self(),__FUNCTION__,__LINE__);
}

MessagePublisher::~MessagePublisher(){
}

bool MessagePublisher::publishMessage(const Message& msg,nsecs_t when){
	return mQueue->enqueueMessage(msg,when);
}

bool MessagePublisher::publishRawData(const void * data){
return 0;
}

//--------- MessageQueue ----------
MessageQueue::MessageQueue(){
	this->init();
}

MessageQueue::~MessageQueue(){
	delete mPoll;
	mPoll = NULL;
}

void MessageQueue::pollOnce(int timeoutMillis){
    mPoll->pollOnce(timeoutMillis);
}

void MessageQueue::wake(){
	mPoll->wake();
}

void MessageQueue::init(){
	mPoll = Poll::getForThread();
    if (NULL == mPoll) {
        mPoll = new Poll(false);
        Poll::setForThread(mPoll);
    }
}

bool MessageQueue::enqueueMessage(const Message& msg,long when){
	if(NULL == msg.getTarget()){
		ALOGE("Can't get the MessageQueue! Funtion =%s Line=%d ",__FUNCTION__,__LINE__);
		return false;
	}
	bool needWake;
	{//acquire lock
		AutoMutex _l(mLock);
		
		needWake = mInboundQueue.isEmpty();
		
		mInboundQueue.enqueueAtTimeOut(const_cast<Message*>(&msg), when);
		
	}//release lock

	if(needWake){
		mPoll->wake();
	}
	return 0;
}

void MessageQueue::removeMessages(MessageHandlerInterface* mHandler,Message& msg){

}

MessageInterface* MessageQueue::next(){
	int32_t nextPollTimeoutMillis = -1;
	for(;;){
		this->pollOnce(nextPollTimeoutMillis);
		ALOGI("FUNCTION=%s line=%d",__FUNCTION__,__LINE__);
		{//acquire lock
			AutoMutex _l(mLock);
			
			// Try to retrieve the next message.  Return if found.
            nsecs_t now = systemTime(SYSTEM_TIME_MONOTONIC);
            
			if(mInboundQueue.isEmpty()){
				nextPollTimeoutMillis = -1;
				ALOGE("FUNCTION=%s line=%d",__FUNCTION__,__LINE__);
			}else{
				Message* msg = mInboundQueue.dequeueAtHead();
            	if (NULL != msg && msg->getTarget() == NULL) {
                	// Stalled by a barrier.  Find the next asynchronous message in the queue.
					ALOGE("The msg can't set the callbacker.");
            	}
				
            	if (NULL != msg) {
					if (now < msg->getWhen()) {
						// Next message is not ready.  Set a timeout to wake up when it is ready.
                    	nextPollTimeoutMillis = min((int32_t)(msg->getWhen() - now), MAX_VALUE);
                	} else {
                    	// Got a message.
                    	mBlock = false;              
                    	msg->next = NULL;
#if 1
						ALOGV("MessageQueue,Returning message: %p" , msg);
#endif
                    	msg->markInUse();
                    	return msg;
                	}
            	} else {
                	// No more messages.
                	ALOGE("FUNCTION=%s line=%d",__FUNCTION__,__LINE__);
                	nextPollTimeoutMillis = -1;
            	}
			}
		}//release lock
	}
	return NULL;
}

bool MessageQueue::quit(){
	return 0;
}

}//namespace ThreadManager
