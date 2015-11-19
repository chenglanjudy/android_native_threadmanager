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


#include "Looper.h"
#include "MessageQueue.h"
#include "logging.h"

namespace ThreadManager{

static pthread_once_t gTLSLooperOnce = PTHREAD_ONCE_INIT;
static pthread_key_t gTLSLooperKey = 0;

Looper::Looper(){
	init();
}

Looper:: Looper(LooperPolicyInterface* policy){
	mPolicy = policy;
	init();
}

Looper::~Looper(){
	delete mQueue;
	mQueue = NULL;
}

void Looper::init(){
	mQueue = new MessageQueue();
}

int Looper::loopOnce(){
	if(mQueue == NULL){
		ALOGE("No MessageQueue; Looper.prepare() wasn't called on this thread.");
		return BAD_VALUE;
	}

	//Attach this thread to Main Thread.
	mPolicy->attachJavaThread();
	
	for (;;) {
        Message* msg = static_cast<Message*>(mQueue->next()); // block or not?
        if ( NULL == msg) {
            // No message indicates that the message queue is quitting.
            ALOGE("FUNCTION=%s line=%d",__FUNCTION__,__LINE__);
            return BAD_VALUE;
        }
		
        msg->getTarget()->dispatchMessage(msg);
		
        msg->recycle();

		ALOGE("FUNCTION=%s line=%d",__FUNCTION__,__LINE__);
    }
	//Detach Thread.
	mPolicy->detachJavaThread();
	return OK;
}

LooperInterface* Looper::prepare(){
	LooperInterface* looper = new Looper();
	return looper;
}

void Looper::setForThread(LooperInterface* looper){
	LooperInterface* old = getForThread(); // also has side-effect of initializing TLS
	if( old == NULL ){ 
		ALOGE("function = %s line =%d looper=%p",__FUNCTION__,__LINE__,looper);
		return;
	}
	pthread_setspecific(gTLSLooperKey, looper);
}

void Looper::initTLSKey() {
    int result = pthread_key_create(& gTLSLooperKey, NULL);
}

LooperInterface* Looper::getForThread(){
	int result = pthread_once(& gTLSLooperOnce, initTLSKey);
	//LOG_IF_ERRNO(result != 0,"Could not start Anthor thread due to error %d.", result);
    return (LooperInterface*)pthread_getspecific(gTLSLooperKey);
}


LooperInterface* Looper::myLooper(){
	LooperInterface* looper = Looper::getForThread();
	return looper;
}

MessageQueueInterface* Looper::getQueue(){
	return mQueue;
}

void Looper::getThread(Thread* thread){}

void Looper::quit(){}


void* Looper::getPointer(){
	return mData;
}


}//namespace ThreadManager
