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

#ifndef _LIBS_MESSAGEQUEUE_H
#define _LIBS_MESSAGEQUEUE_H

#include "MessageHandler.h"
#include "Message.h"
#include "Looper.h"
#include "Poll.h"

namespace ThreadManager{

class MessageQueueInterface;

class MessagePublisher{
public:
	MessagePublisher(MessageHandler::Callback* queue);
	~MessagePublisher();
	
	/* Publishes a Message event to the MessageQueue.
     *
     * Returns true on success.
     * Returns false on fail.
     */
	virtual bool publishMessage(const Message & msg,nsecs_t when);

	/* Publishes a raw Message event to the MessageQueue.
     *
     * Returns true on success.
     * Returns false on fail.
     */
	virtual bool publishRawData(const void * data);

private:
	//The Message Queue option interface.
	MessageHandler::Callback* mQueue;
};


class MessageQueueInterface
	: public MessageHandler::Callback 
	, public LooperInterface::Callback{
public:
	/* Thread poll once when time out.
     *
     * Returns void.
     */
	virtual void pollOnce(int timeoutMillis)=0;

	/* when Thread is sleep,then wake it.
     *
     * Returns void.
     */
	virtual void wake()=0;
	
	MessageQueueInterface(){}
	virtual ~MessageQueueInterface(){}
};

class MessageQueue : public MessageQueueInterface{
public:
	
	virtual void pollOnce(int timeoutMillis);
	
	virtual void wake();
	
	virtual bool enqueueMessage(const Message& msg,long when);
	
	virtual void removeMessages(MessageHandlerInterface* mHandler,Message& msg);
	
	virtual MessageInterface* next();
	
	virtual bool quit();

	MessageQueue();
	virtual ~MessageQueue();
	
protected:
	//Get the Poll object for loop message.
	inline Poll* getPoll(){
		if( NULL == mPoll){
			mPoll = new Poll(true);
		}
		return mPoll;
	}

private:
	/* Initialize the the MessageQueue.
     *
     * Returns void.
     */
	virtual void init();

private:
	// Generic message queue implementation.
    template <typename T>
    struct Queue {
        T* head;
        T* tail;

        inline Queue() : head(NULL), tail(NULL) {
        }

        inline bool isEmpty() const {
            return !head;
        }

        inline void enqueueAtTail(T* entry) {
            entry->prev = tail;
            if (tail) {
                tail->next = entry;
            } else {
                head = entry;
            }
            entry->next = NULL;
            tail = entry;
        }

        inline void enqueueAtHead(T* entry) {
            entry->next = head;
            if (head) {
                head->prev = entry;
            } else {
                tail = entry;
            }
            entry->prev = NULL;
            head = entry;
        }

		inline void enqueueAtTimeOut(T* entry,nsecs_t when) {
			if(NULL == head || 0 == when || when < head->getWhen()){
				enqueueAtHead(entry);
			}else{
				T* t;
				T* prev;
				t = head;
				while(true){
					prev = t;
					t = t->next;
					if(NULL == t){
						enqueueAtTail(entry);
					}else if(when < t->getWhen()){
						break;
					}
				}
				entry->next = t; // invariant: t == prev->next
                prev->next = entry;
				entry->prev = prev;
			}
        }

        inline void dequeue(T* entry) {
            if (entry->prev) {
                entry->prev->next = entry->next;
            } else {
                head = entry->next;
            }
            if (entry->next) {
                entry->next->prev = entry->prev;
            } else {
                tail = entry->prev;
            }
        }

        inline T* dequeueAtHead() {
            T* entry = head;
            head = entry->next;
            if (head) {
                head->prev = NULL;
            } else {
                tail = NULL;
            }
            return entry;
        }

        inline uint32_t count() const{
			uint32_t result = 0;
			for (const T* entry = head; entry; entry = entry->next) {
				result += 1;
			}
			return result;
		}
    };

	//The queue for message event.
	Queue<Message> mInboundQueue;
	
	//The Poll pointer.
	Poll* mPoll;
	
	//Message* msg;
	MessagePublisher* mPublisher;

	//The mutex object.
	Mutex mLock;

	//The block flag.
	bool mBlock;
	
};


}//namespace ThreadManager

#endif//_LIBS_MESSAGEQUEUE_H

