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


#include "Message.h"
#include "MessageHandler.h"
#include "logging.h"

namespace ThreadManager{


bool Message::setTarget(MessageHandlerInterface* target){
	mTarget = target;
	return 0;
}

void Message::sendToTarget(){

}

void Message::setData(void* mData){

}

void Message::recycle(){
	delete this;
}

inline nsecs_t Message::getWhen()const{return when;}


inline MessageHandlerInterface* Message::getTarget()const{
	return mTarget;
}

Message* Message::createMessage(MessageHandlerInterface* target){
	Message* msg = new Message();
	if(NULL == msg){
		return NULL;
	}
	msg->flags = FLAG_FREE;
	msg->mTarget = target;
	int i = 5;
	msg->mData = reinterpret_cast<int*>(&i);
	msg->when = systemTime(SYSTEM_TIME_MONOTONIC);
	msg->type = TYPE_HAVE_CALLBACK;
	return msg;
}

inline void Message::markInUse(){
	flags |= FLAG_IN_USE;
}

inline int32_t Message::getType()const{
	return type;
}



}//namespace ThreadManager
