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

#ifndef _LIBS_MESSAGEHANDLER_H
#define _LIBS_MESSAGEHANDLER_H

#include "Mutex.h"
#include "Looper.h"
#include "Message.h"

namespace ThreadManager{

class MessageConsumer {//: public PollerCallback{
public:
	MessageConsumer();
	virtual ~MessageConsumer();

	//
	virtual int32_t consumeMessage(Message* msg,int32_t when
					 , MessageHandlerInterface* handler) const;

	//
	virtual int handleEvent(int fd, int events, void* data); 
private:
	MessageHandlerInterface* mHandler;
};//MessageConsumer

class MessageHandlerPolicyInterface{
protected:
    MessageHandlerPolicyInterface() { }
    virtual ~MessageHandlerPolicyInterface() { }

public:
     
    virtual void notifyMessage() = 0;
};


class MessageHandlerInterface{
public:
	MessageHandlerInterface(){}
	virtual ~MessageHandlerInterface(){}

	/**
     * Subclasses must implement this to receive messages.
     */
	virtual bool handleMessage(const Message* const mMessage) const =0;

	/**
     * Pushes a message onto the end of the message queue after all pending messages
     * before the current time. It will be received in #handleMessage,
     * in the thread attached to this handler.
     *  
     * Returns true if the message was successfully placed in to the 
     * message queue.  Returns false on failure, usually because the
     * looper processing the message queue is exiting.
     */
	virtual bool sendMessage(const Message& mMessage,int32_t when)=0;

	/**
     * Dispatch the message to Message Consumer.
     * Return true on dipatch success.
     */
	virtual bool dispatchMessage(Message* mMessage)=0;

	/**
     * Obtain a Message.
     * Return message object.
     */
	virtual Message* obtainMessage()=0;
};//class MessageHandlerInterface

class MessagePublisher;

class MessageHandler : public MessageHandlerInterface{
public:
	
	virtual bool handleMessage(const Message* const mMessage)const;
	
	virtual bool sendMessage(const Message& mMessage,int32_t when);
	
	virtual bool dispatchMessage(Message* mMessage);
	
	virtual Message* obtainMessage();
	
	class Callback {
	public:
		virtual ~Callback(){}
		virtual bool enqueueMessage(const Message& msg,long when)=0;
		virtual void removeMessages(MessageHandlerInterface* mHandler,Message& msg)=0;
	};//Callback

	MessageHandler();
	
	MessageHandler(LooperInterface* looper,MessageHandler::Callback* queue
					,MessageHandlerPolicyInterface* policy);
	virtual ~MessageHandler();

private:
	//The controller for message interact.
	LooperInterface* mLooper;

	//The Message Consumer.
	MessageConsumer mConsumer;

	//The Message Publisher
	MessagePublisher* mPublisher;

	MessageHandlerPolicyInterface* mPolicy;
	
};//class Messagehandler

}//namespace ThreadManager

#endif //_LIBS_MESSAGEHANDLER_H
