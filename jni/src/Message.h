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


#ifndef _LIBS_MESSAGE_H
#define _LIBS_MESSAGE_H

#include "Timers.h"

namespace ThreadManager{

class MessageHandlerInterface;

template <typename T>
struct Link {
	T* next;
	T* prev;
protected:
	inline Link() : next(NULL), prev(NULL) { }
};


class MessageInterface {
public:
	virtual ~MessageInterface(){}

	virtual MessageHandlerInterface* getTarget()const=0;
	virtual bool setTarget(MessageHandlerInterface* target)=0;
	virtual void sendToTarget()=0;
	virtual void setData(void* mData)=0;
	virtual nsecs_t getWhen()const=0;
	virtual void recycle()=0;
	virtual void markInUse()=0;
	virtual int32_t getType()const=0;

};

class Message : public Link<Message>, public MessageInterface{
public:
	enum{
		TYPE_HAVE_CALLBACK,
		TYPE_NO_RECEIVER
	};

	enum{
		FLAG_IN_USE = 1<<0,
		FLAG_FREE   = 1<<1
	};
	Message(){}
	virtual ~Message(){}
	virtual bool setTarget(MessageHandlerInterface* target);
	virtual void sendToTarget();
	virtual void setData(void* mData);
	virtual void recycle();
	virtual MessageHandlerInterface* getTarget()const;
	static Message* createMessage(MessageHandlerInterface* target);
	virtual nsecs_t getWhen()const;
	virtual void markInUse();
	virtual int32_t getType()const;

private:
	nsecs_t when;
	MessageHandlerInterface* mTarget;
	int32_t flags;
	int32_t mSize;
	void* mData;
	int32_t type;
};


}//namespace ThreadManager

#endif

