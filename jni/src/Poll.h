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


#ifndef _LIBS_POLL_H
#define _LIBS_POLL_H

#include "Timers.h"
#include "Message.h"
#include "MessageHandler.h"
#include "Mutex.h"


#include <android/looper.h>
#include <sys/epoll.h>



namespace ThreadManager {
	
typedef int (*ALooper_callbackFunc)(int fd, int events, void* data);

class Message;
class MessageHandler;

enum {
    /**
     * Option for ALooper_prepare: this looper will accept calls to
     * ALooper_addFd() that do not have a callback (that is provide NULL
     * for the callback).  In this case the caller of ALooper_pollOnce()
     * or ALooper_pollAll() MUST check the return from these functions to
     * discover when data is available on such fds and process it.
     */
    ALOOPER_PREPARE_ALLOW_NON_CALLBACKS = 1<<0
};


enum {
    /**
     * Result from ALooper_pollOnce() and ALooper_pollAll():
     * The poll was awoken using wake() before the timeout expired
     * and no callbacks were executed and no other file descriptors were ready.
     */
    ALOOPER_POLL_WAKE = -1,

    /**
     * Result from ALooper_pollOnce() and ALooper_pollAll():
     * One or more callbacks were executed.
     */
    ALOOPER_POLL_CALLBACK = -2,

    /**
     * Result from ALooper_pollOnce() and ALooper_pollAll():
     * The timeout expired.
     */
    ALOOPER_POLL_TIMEOUT = -3,

    /**
     * Result from ALooper_pollOnce() and ALooper_pollAll():
     * An error occurred.
     */
    ALOOPER_POLL_ERROR = -4,
};

/**
 * Flags for file descriptor events that a looper can monitor.
 *
 * These flag bits can be combined to monitor multiple events at once.
 */
enum {
    /**
     * The file descriptor is available for read operations.
     */
    ALOOPER_EVENT_INPUT = 1 << 0,

    /**
     * The file descriptor is available for write operations.
     */
    ALOOPER_EVENT_OUTPUT = 1 << 1,

    /**
     * The file descriptor has encountered an error condition.
     *
     * The looper always sends notifications about errors; it is not necessary
     * to specify this event flag in the requested event set.
     */
    ALOOPER_EVENT_ERROR = 1 << 2,

    /**
     * The file descriptor was hung up.
     * For example, indicates that the remote end of a pipe or socket was closed.
     *
     * The looper always sends notifications about hangups; it is not necessary
     * to specify this event flag in the requested event set.
     */
    ALOOPER_EVENT_HANGUP = 1 << 3,

    /**
     * The file descriptor is invalid.
     * For example, the file descriptor was closed prematurely.
     *
     * The looper always sends notifications about invalid file descriptors; it is not necessary
     * to specify this event flag in the requested event set.
     */
    ALOOPER_EVENT_INVALID = 1 << 4,
};



	
/*
* Declare a concrete type for the NDK's looper forward declaration.
*/

struct APoller {
};


/**
 * A message that can be posted to a Looper.
 */
/*struct Message {
    Message() : what(0) { }
    Message(int what) : what(what) { }

    /* The message type. (interpretation is left up to the handler) */
   /* int what;
};*/


/**
 * Interface for a Looper message handler.
 *
 * The Looper holds a strong reference to the message handler whenever it has
 * a message to deliver to it.  Make sure to call Looper::removeMessages
 * to remove any pending messages destined for the handler so that the handler
 * can be destroyed.
 */
/*class MessageHandler : public virtual RefBase {
protected:
    virtual ~MessageHandler() { }

public:
    /**
     * Handles a message.
     */
   /* virtual void handleMessage(const Message& message) = 0;
};*/


/**
 * A simple proxy that holds a weak reference to a message handler.
 */
class WeakMessageHandler {//: public MessageHandler {
protected:
    virtual ~WeakMessageHandler();

public:
    WeakMessageHandler(MessageHandler* handler);
    virtual void handleMessage(const Message& message);

private:
    MessageHandler* mHandler;
};


/**
 * A looper callback.
 */
class PollerCallback {//: public virtual RefBase {
protected:
    virtual ~PollerCallback() { }

public:
    /**
     * Handles a poll event for the given file descriptor.
     */
    virtual int handleEvent(int fd, int events, void* data) = 0;
};


/**
 * Wraps a ALooper_callbackFunc function pointer.
 */
class SimplePollerCallback : public PollerCallback {
protected:
    virtual ~SimplePollerCallback();

public:
    SimplePollerCallback(ALooper_callbackFunc callback);
    virtual int handleEvent(int fd, int events, void* data);

private:
    ALooper_callbackFunc mCallback;
};


/**
 * A poller can be associated with a thread although there is no requirement that it must be.
 */
class Poll : public APoller{
public:

	virtual ~Poll();

    /**
     * Creates a Poller.
     */
    Poll(bool allowNonCallbacks);

    /**
     * Returns whether this looper instance allows the registration of file descriptors
     * using identifiers instead of callbacks.
     */
    bool getAllowNonCallbacks() const;

    /**
     * Waits for events to be available, with optional timeout in milliseconds.
     * Invokes callbacks for all file descriptors on which an event occurred.
     */
    int pollOnce(int timeoutMillis, int* outFd, int* outEvents, void** outData);
    inline int pollOnce(int timeoutMillis) {
        return pollOnce(timeoutMillis, NULL, NULL, NULL);
    }

    /**
     * Like pollOnce(), but performs all pending callbacks until all
     * data has been consumed or a file descriptor is available with no callback.
     * This function will never return ALOOPER_POLL_CALLBACK.
     */
    int pollAll(int timeoutMillis, int* outFd, int* outEvents, void** outData);
    inline int pollAll(int timeoutMillis) {
        return pollAll(timeoutMillis, NULL, NULL, NULL);
    }

    /**
     * Wakes the poll asynchronously.
     *
     * This method can be called on any thread.
     */
    void wake();

    /**
     * Adds a new file descriptor to be polled by the looper.
     * If the same file descriptor was previously added, it is replaced.
     */
    int addFd(int fd, int ident, int events, ALooper_callbackFunc callback, void* data);
    int addFd(int fd, int ident, int events, const PollerCallback* callback, void* data);

    /**
     * Removes a previously added file descriptor from the poller.
     */
    int removeFd(int fd);

    /**
     * Enqueues a message to be processed by the specified handler.
     */
    void sendMessage(const MessageHandler& handler, const Message& message);

    /**
     * Enqueues a message to be processed by the specified handler after all pending messages
     * after the specified delay.
     */
    void sendMessageDelayed(nsecs_t uptimeDelay, const MessageHandler& handler,
            const Message& message);

    /**
     * Enqueues a message to be processed by the specified handler after all pending messages
     * at the specified time.
     */
    void sendMessageAtTime(nsecs_t uptime, const MessageHandler& handler,
            const Message& message);

    /**
     * Removes all messages for the specified handler from the queue.
     *
     * The handler must not be null.
     * This method can be called on any thread.
     */
    void removeMessages(const MessageHandler& handler);

    /**
     * Removes all messages of a particular type for the specified handler from the queue.
     *
     * The handler must not be null.
     * This method can be called on any thread.
     */
    void removeMessages(const MessageHandler& handler, int what);

    /**
     * Prepares a looper associated with the calling thread, and returns it.
     * If the thread already has a looper, it is returned.  Otherwise, a new
     * one is created, associated with the thread, and returned.
     *
     * The opts may be ALOOPER_PREPARE_ALLOW_NON_CALLBACKS or 0.
     */
    static Poll* prepare(int opts);

    /**
     * Sets the given looper to be associated with the calling thread.
     */
    static void setForThread(const Poll& looper);

    /**
     * Returns the poller associated with the calling thread, or NULL if
     * there is not one.
     */
    static Poll* getForThread();

private:

    const bool mAllowNonCallbacks; // immutable

    int mWakeReadPipeFd;  // immutable
    int mWakeWritePipeFd; // immutable
    Mutex mLock;

   // std::vector<MessageEnvelope> mMessageEnvelopes; // guarded by mLock
    bool mSendingMessage; // guarded by mLock

    int mEpollFd; // immutable

    // This state is only used privately by pollOnce and does not require a lock since
    // it runs on a single thread.
    //std::vetor<Response> mResponses;
    size_t mResponseIndex;
    nsecs_t mNextMessageUptime; // set to LLONG_MAX when none

    int pollInner(int timeoutMillis);
    void awoken();
    //void pushResponse(int events, const Request& request);

    static void initTLSKey();
    static void threadDestructor(void *st);
};

} // namespace ThreadManager

#endif // UTILS_LOOPER_H
