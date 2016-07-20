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


// Debugs poll and wake interactions.
#define DEBUG_POLL_AND_WAKE 1

// Debugs callback registration and invocation.
#define DEBUG_CALLBACKS 0

#include "logging.h"
#include "Poll.h"
#include "Timers.h"

#include <unistd.h>
#include <fcntl.h>
#include <limits.h>


namespace ThreadManager {

WeakMessageHandler::WeakMessageHandler(MessageHandler* handler) :
        mHandler(handler) {
}

WeakMessageHandler::~WeakMessageHandler() {
}

void WeakMessageHandler::handleMessage(const Message& message) {
    MessageHandler* handler = mHandler;
    //if (handler != NULL) {
    //    handler->handleMessage(message);
    //}
}


// --- SimplePollerCallback ---

SimplePollerCallback::SimplePollerCallback(ALooper_callbackFunc callback) :
        mCallback(callback) {
}

SimplePollerCallback::~SimplePollerCallback() {
}

int SimplePollerCallback::handleEvent(int fd, int events, void* data) {
    return mCallback(fd, events, data);
}


// --- Poll ---

// Hint for number of file descriptors to be associated with the epoll instance.
static const int EPOLL_SIZE_HINT = 8;

// Maximum number of file descriptors for which to retrieve poll events each iteration.
static const int EPOLL_MAX_EVENTS = 8;

static pthread_once_t gTLSOnce = PTHREAD_ONCE_INIT;
static pthread_key_t gTLSKey = 0;

Poll::Poll(bool allowNonCallbacks) :
        mAllowNonCallbacks(allowNonCallbacks), mSendingMessage(false),
        mResponseIndex(0), mNextMessageUptime(LLONG_MAX) {
    int wakeFds[2];
    int result = pipe(wakeFds);
	
#if DEBUG_POLL_AND_WAKE
	LOG_IF_ERRNO(result!=0,"Could not create wake pipe.  errno=%d", errno);
#endif

    mWakeReadPipeFd = wakeFds[0];
    mWakeWritePipeFd = wakeFds[1];

    result = fcntl(mWakeReadPipeFd, F_SETFL, O_NONBLOCK);
	
#if DEBUG_POLL_AND_WAKE
	LOG_IF_ERRNO(result!=0,"Could not make wake read pipe non-blocking.  errno=%d",
            errno);
#endif


    result = fcntl(mWakeWritePipeFd, F_SETFL, O_NONBLOCK);
#if DEBUG_POLL_AND_WAKE
	LOG_IF_ERRNO(result!=0,"Could not make wake write pipe non-blocking.  errno=%d",
            errno);
#endif

    // Allocate the epoll instance and register the wake pipe.
    mEpollFd = epoll_create(EPOLL_SIZE_HINT);
#if DEBUG_POLL_AND_WAKE
	LOG_IF_ERRNO(result!=0,"Could not create epoll instance.  errno=%d mEpollFd=%d", errno,mEpollFd);
#endif

    struct epoll_event eventItem;
    memset(& eventItem, 0, sizeof(epoll_event)); // zero out unused members of data field union
    eventItem.events = EPOLLIN;
    eventItem.data.fd = mWakeReadPipeFd;
    result = epoll_ctl(mEpollFd, EPOLL_CTL_ADD, mWakeReadPipeFd, & eventItem);
#if DEBUG_POLL_AND_WAKE
	LOG_IF_ERRNO(result!=0,"Could not add wake read pipe to epoll instance.  errno=%d",
            errno);
#endif
}

Poll::~Poll() {
    close(mWakeReadPipeFd);
    close(mWakeWritePipeFd);
    close(mEpollFd);
}

void Poll::initTLSKey() {
    int result = pthread_key_create(& gTLSKey, threadDestructor);
#if DEBUG_POLL_AND_WAKE
	LOG_IF_ERRNO(result!=0,"Could not allocate TLS key. The value of result = %d",result);
#endif
}

void Poll::threadDestructor(void *st) {
    Poll* const self = static_cast<Poll*>(st);
    delete self;
}

void Poll::setForThread(const Poll& poller) {
    Poll* old = getForThread(); // also has side-effect of initializing TLS
    if(old == &poller){
		return;
	}
    pthread_setspecific(gTLSKey, &poller);
}

Poll* Poll::getForThread() {
    int result = pthread_once(& gTLSOnce, initTLSKey);
#if DEBUG_POLL_AND_WAKE
	LOG_IF_ERRNO(result!=0,"pthread_once failed. The value of result = %d",result);
#endif
    return (Poll*)pthread_getspecific(gTLSKey);
}

Poll* Poll::prepare(int opts) {
    bool allowNonCallbacks = opts & ALOOPER_PREPARE_ALLOW_NON_CALLBACKS;
    Poll* poller = Poll::getForThread();
    if (NULL == poller) {
        poller = new Poll(allowNonCallbacks);
        Poll::setForThread(poller);
    }
    if (poller->getAllowNonCallbacks() != allowNonCallbacks) {
        ALOGW("Poll already prepared for this thread with a different value for the "
                "ALOOPER_PREPARE_ALLOW_NON_CALLBACKS option.");
    }
    return poller;
}

bool Poll::getAllowNonCallbacks() const {
    return mAllowNonCallbacks;
}

int Poll::pollOnce(int timeoutMillis, int* outFd, int* outEvents, void** outData) {
    int result = 0;
	
    for (;;) {
       result = pollInner(timeoutMillis);
	   if(ALOOPER_POLL_WAKE == result){
	   		break;
	   }
    }
}

int Poll::pollInner(int timeoutMillis) {

    // Adjust the timeout based on when the next message is due.
    if (timeoutMillis != 0 && mNextMessageUptime != LLONG_MAX) {
        nsecs_t now = systemTime(SYSTEM_TIME_MONOTONIC);
        int messageTimeoutMillis = toMillisecondTimeoutDelay(now, mNextMessageUptime);
        if (messageTimeoutMillis >= 0
                && (timeoutMillis < 0 || messageTimeoutMillis < timeoutMillis)) {
            timeoutMillis = messageTimeoutMillis;
        }
#if DEBUG_POLL_AND_WAKE
        ALOGD("%p ~ pollOnce - next message in %lldns, adjusted timeout: timeoutMillis=%d",
                this, mNextMessageUptime - now, timeoutMillis);
#endif
    }

    int result = ALOOPER_POLL_WAKE;

    struct epoll_event eventItems[EPOLL_MAX_EVENTS];
#if DEBUG_POLL_AND_WAKE
    ALOGD("1 %p ~ pollOnce - waiting: timeoutMillis=%d", this, timeoutMillis);
#endif

    int eventCount = epoll_wait(mEpollFd, eventItems, EPOLL_MAX_EVENTS, timeoutMillis);
#if DEBUG_POLL_AND_WAKE
	ALOGD("2 %p ~ pollOnce - waiting: timeoutMillis=%d", this, timeoutMillis);
#endif


    // Acquire lock.
    mLock.lock();

    // Check for poll error.
    if (eventCount < 0) {
        if (errno == EINTR) {
            goto Done;
        }
        ALOGW("Poll failed with an unexpected error, errno=%d", errno);
        result = ALOOPER_POLL_ERROR;
        goto Done;
    }

    // Check for poll timeout.
    if (eventCount == 0) {
#if DEBUG_POLL_AND_WAKE
        ALOGD("%p ~ pollOnce - timeout", this);
#endif
        result = ALOOPER_POLL_TIMEOUT;
        goto Done;
    }

    // Handle all events.
#if DEBUG_POLL_AND_WAKE
    ALOGD("%p ~ pollOnce - handling events from %d fds", this, eventCount);
#endif

    for (int i = 0; i < eventCount; i++) {
        int fd = eventItems[i].data.fd;
        uint32_t epollEvents = eventItems[i].events;
        if (fd == mWakeReadPipeFd) {
            if (epollEvents & EPOLLIN) {
                awoken();
            } else {
                ALOGW("Ignoring unexpected epoll events 0x%x on wake read pipe.", epollEvents);
            }
        } else {
          	ALOGW("Another read fd for epoll wait!");
        }
    }
Done: ;

    // Invoke pending message callbacks.
    mNextMessageUptime = LLONG_MAX;
	ALOGI("FUNCTION=%s line=%d",__FUNCTION__,__LINE__);
    // Release lock.
    mLock.unlock();

    // Invoke all response callbacks.
    // Wait for a moment. HAHA
    return result;
}

int Poll::pollAll(int timeoutMillis, int* outFd, int* outEvents, void** outData) {
    if (timeoutMillis <= 0) {
        int result;
        do {
            result = pollOnce(timeoutMillis, outFd, outEvents, outData);
        } while (result == ALOOPER_POLL_CALLBACK);
        return result;
    } else {
        nsecs_t endTime = systemTime(SYSTEM_TIME_MONOTONIC)
                + milliseconds_to_nanoseconds(timeoutMillis);

        for (;;) {
            int result = pollOnce(timeoutMillis, outFd, outEvents, outData);
            if (result != ALOOPER_POLL_CALLBACK) {
                return result;
            }

            nsecs_t now = systemTime(SYSTEM_TIME_MONOTONIC);
            timeoutMillis = toMillisecondTimeoutDelay(now, endTime);
            if (timeoutMillis == 0) {
                return ALOOPER_POLL_TIMEOUT;
            }
        }
    }
}

void Poll::wake() {
#if DEBUG_POLL_AND_WAKE
    ALOGD("%p ~ wake", this);
#endif

    ssize_t nWrite;
    do {
        nWrite = write(mWakeWritePipeFd, "W", 1);
    } while (nWrite == -1 && errno == EINTR);

    if (nWrite != 1) {
        if (errno != EAGAIN) {
            ALOGW("Could not write wake signal, errno=%d", errno);
        }
    }
}

void Poll::awoken() {
#if DEBUG_POLL_AND_WAKE
    ALOGD("%p ~ awoken", this);
#endif

    char buffer[16];
	//memset(buffer,0,sizeof(buffer));
    ssize_t nRead;
    do {
        nRead = read(mWakeReadPipeFd, buffer, sizeof(buffer));
    } while ((nRead == -1 && errno == EINTR) || nRead == sizeof(buffer));
}

int Poll::addFd(int fd, int ident, int events, ALooper_callbackFunc callback, void* data) {
    return addFd(fd, ident, events, callback ? new SimplePollerCallback(callback) : NULL, data);
}

int Poll::addFd(int fd, int ident, int events, const PollerCallback* callback, void* data) {
#if DEBUG_CALLBACKS
    ALOGD("%p ~ addFd - fd=%d, ident=%d, events=0x%x, callback=%p, data=%p", this, fd, ident,
            events, callback.get(), data);
#endif

    if (!callback) {
        if (! mAllowNonCallbacks) {
            ALOGE("Invalid attempt to set NULL callback but not allowed for this looper.");
            return -1;
        }

        if (ident < 0) {
            ALOGE("Invalid attempt to set NULL callback with ident < 0.");
            return -1;
        }
    } else {
        ident = ALOOPER_POLL_CALLBACK;
    }

    int epollEvents = 0;
    if (events & ALOOPER_EVENT_INPUT) epollEvents |= EPOLLIN;
    if (events & ALOOPER_EVENT_OUTPUT) epollEvents |= EPOLLOUT;

    { // acquire lock
        AutoMutex _l(mLock);

        struct epoll_event eventItem;
        memset(& eventItem, 0, sizeof(epoll_event)); // zero out unused members of data field union
        eventItem.events = epollEvents;
        eventItem.data.fd = fd;

        ssize_t requestIndex = -1;
        if (requestIndex < 0) {
            int epollResult = epoll_ctl(mEpollFd, EPOLL_CTL_ADD, fd, & eventItem);
            if (epollResult < 0) {
                ALOGE("Error adding epoll events for fd %d, errno=%d", fd, errno);
                return -1;
            }
        } else {
            int epollResult = epoll_ctl(mEpollFd, EPOLL_CTL_MOD, fd, & eventItem);
            if (epollResult < 0) {
                ALOGE("Error modifying epoll events for fd %d, errno=%d", fd, errno);
                return -1;
            }
        }
    } // release lock
    return 1;
}

int Poll::removeFd(int fd) {
#if DEBUG_CALLBACKS
    ALOGD("%p ~ removeFd - fd=%d", this, fd);
#endif

    { // acquire lock
        AutoMutex _l(mLock);

        int epollResult = epoll_ctl(mEpollFd, EPOLL_CTL_DEL, fd, NULL);
        if (epollResult < 0) {
            ALOGE("Error removing epoll events for fd %d, errno=%d", fd, errno);
            return -1;
        }
		
    } // release lock
    return 1;
}

void Poll::sendMessage(const MessageHandler& handler, const Message& message) {
    nsecs_t now = systemTime(SYSTEM_TIME_MONOTONIC);
    sendMessageAtTime(now, handler, message);
}

void Poll::sendMessageDelayed(nsecs_t uptimeDelay, const MessageHandler& handler,
        const Message& message) {
    nsecs_t now = systemTime(SYSTEM_TIME_MONOTONIC);
    sendMessageAtTime(now + uptimeDelay, handler, message);
}

void Poll::sendMessageAtTime(nsecs_t uptime, const MessageHandler& handler,
        const Message& message) {
#if DEBUG_CALLBACKS
    ALOGD("%p ~ sendMessageAtTime - uptime=%lld, handler=%p, what=%d",
            this, uptime, handler.get(), message.what);
#endif

    size_t i = 0;
    { // acquire lock
        AutoMutex _l(mLock);
        //more ....
    } // release lock

    // Wake the poll loop only when we enqueue a new message at the head.
    if (i == 0) {
        wake();
    }
}

void Poll::removeMessages(const MessageHandler& handler) {
#if DEBUG_CALLBACKS
    ALOGD("%p ~ removeMessages - handler=%p", this, handler.get());
#endif

    { // acquire lock
        AutoMutex _l(mLock);
		//more ....
    } // release lock
}

void Poll::removeMessages(const MessageHandler& handler, int what) {
#if DEBUG_CALLBACKS
    ALOGD("%p ~ removeMessages - handler=%p, what=%d", this, handler.get(), what);
#endif

    { // acquire lock
        AutoMutex _l(mLock);
		//more ....
    } // release lock
}

} // namespace ThreadManager
