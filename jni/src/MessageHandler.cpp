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


#include "MessageHandler.h"
#include "logging.h"
#include "MessageQueue.h"
#include "jni.h"
//------
namespace ThreadManager{

//-------- MessageConsumer -------

MessageConsumer::MessageConsumer()
{
	ALOGD("Thread number %ld function= %s line=%d\n", pthread_self(),__FUNCTION__,__LINE__);
}

MessageConsumer::~MessageConsumer(){
}

int32_t MessageConsumer::consumeMessage(Message* msg,int32_t when
	,MessageHandlerInterface* handler)const{
	nsecs_t now = systemTime(SYSTEM_TIME_MONOTONIC);
	if(msg->getWhen()< now){
		handler->handleMessage(msg);
	}else{
		ALOGI("start timer!");
	}
	return NO_ERROR;
}

int32_t MessageConsumer::handleEvent(int fd, int events, void* data){
	return 0;
}

//-------- MessageHandler -------

MessageHandler::MessageHandler(){}


MessageHandler::MessageHandler(LooperInterface* looper
		, MessageHandler::Callback* queue
		,MessageHandlerPolicyInterface* policy)
		: mLooper(looper),mConsumer(),mPolicy(policy){
	mPublisher = new MessagePublisher(queue);
	ALOGD("Thread number %ld function= %s line=%d\n", pthread_self(),__FUNCTION__,__LINE__);
}

MessageHandler::~MessageHandler(){
	delete mPublisher;
	mPublisher = NULL;
}

bool MessageHandler::handleMessage(const Message* const mMessage)const{
	switch(mMessage->getType()){
		case 0:{
			ALOGI("Get value:%lld mPolicy=%p",mMessage->getWhen(),mPolicy);
			mPolicy->notifyMessage();
			break;
		}
		default:{
			ALOGI("Can't get the Value!");
		}
	}
	return OK;
}

bool MessageHandler::sendMessage(const Message& mMessage,int32_t when){
	MessagePublisher* publiser = mPublisher;
    if (NULL == publiser) { 
         ALOGE("Can't get the MessageQueue! Funtion =%s Line=%d ",__FUNCTION__,__LINE__);
         return false;
    }
	ALOGE("get the MessageQueue! Funtion =%s Line=%d ",__FUNCTION__,__LINE__);
	return publiser->publishMessage(mMessage,when);
}

bool MessageHandler::dispatchMessage(Message* mMessage){
	mConsumer.consumeMessage(mMessage,0,this);
	return OK;
}

Message* MessageHandler::obtainMessage(){
	return Message::createMessage(this);
}


}//namespace ThreadManager


