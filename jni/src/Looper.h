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


#ifndef _LIBS_LOOPER_H
#define _LIBS_LOOPER_H

#include "Mutex.h"
#include "Thread.h"

//------------------------------------
namespace ThreadManager{
//------------------------------------


class Thread;
class MessageInterface;
class MessageQueueInterface;


class LooperPolicyInterface {
protected:
    LooperPolicyInterface() { }
    virtual ~LooperPolicyInterface() { }

public:
   	virtual void attachJavaThread()=0;
	
	virtual void detachJavaThread()=0;

};


class LooperInterface{	
public:
	LooperInterface(){}
	virtual ~LooperInterface(){}

	//class Callback defination.
	class Callback {
	public:
		virtual ~Callback(){}
		virtual MessageInterface* next()=0;
		virtual bool quit()=0;
	};//end 
	
	/* Run the message queue in this thread. Be sure to call
     *
     * This method may be called on any thread (usually by the waitting work thread). */
	virtual int32_t loopOnce()=0;
	
	/**
     * Returns the Thread object associated with the calling thread, or NULL if
     * there is not one.
     */
	virtual void getThread(Thread* thread)=0;

	/**
     * Returns the MessageQueue associated with the calling thread, or NULL if
     * there is not one.
     */
	virtual MessageQueueInterface* getQueue()=0;

	/**
     * Exit this Message Looper.It's means the thread over run.
     */
	virtual void quit()=0;

	virtual void* getPointer()=0;

private:
	
	/**
     * Initialize the Message Queue.
     */
	virtual void init()=0;

};

class Looper : public LooperInterface {
public:
	virtual int32_t loopOnce();

	static LooperInterface* prepare();
	/**
     * Sets the given looper to be associated with the calling thread.
     * If another looper is already associated with the thread, it is replaced.
     *
     * If "looper" is NULL, removes the currently associated looper.
     */
    static void setForThread(LooperInterface* looper);

    /**
     * Returns the looper pointer associated with the calling thread, or NULL if
     * there is not one.
     */
    static LooperInterface* getForThread();

	//
	static LooperInterface* myLooper();

	virtual MessageQueueInterface* getQueue();

	virtual void getThread(Thread* thread);

	virtual void quit();

	virtual void* getPointer();

	Looper();
	Looper(LooperPolicyInterface* policy);
	virtual ~Looper();

private:
	/**
     * Initialize the thread's TLS key.
     */
	static void initTLSKey();

	virtual void init();
	
private:

	//
	MessageQueueInterface* mQueue;

	//
	Thread* mThread;

	void* mData;

	LooperPolicyInterface* mPolicy;
};

}// namespace ThreadManager

#endif //_LIBS_LOOPER_H 
