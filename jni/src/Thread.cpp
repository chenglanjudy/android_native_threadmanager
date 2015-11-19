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

#include "Thread.h"
#include "logging.h"
#include "AndroidThreads.h"
#include "Errors.h"
#include "Looper.h"
#include "sched_policy.h"

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>
#include <jni.h>

#if defined(HAVE_PTHREADS)
# include <pthread.h>
# include <sched.h>
# include <sys/resource.h>
#ifdef HAVE_ANDROID_OS
# include "bionic_pthread.h"
#endif
#endif

#if defined(HAVE_PRCTL)
#include <sys/prctl.h>
#endif


using namespace ThreadManager;

// ----------------------------------------------------------------------------
#if defined(HAVE_PTHREADS)
// ----------------------------------------------------------------------------

/*
 * Create and run a new thread.
 *
 * We create it "detached", so it cleans up after itself.
 */

typedef void* (*android_pthread_entry)(void*);

static pthread_once_t gDoSchedulingGroupOnce = PTHREAD_ONCE_INIT;
static bool gDoSchedulingGroup = true;

static void checkDoSchedulingGroup(void) {
    char buf[PROPERTY_VALUE_MAX];
    int len = 0;//property_get("debug.sys.noschedgroups", buf, "");
    if (len > 0) {
        int temp;
        if (sscanf(buf, "%d", &temp) == 1) {
            gDoSchedulingGroup = temp == 0;
        }
    }
}

struct thread_data_t {
    thread_func_t   entryFunction;
    void*           userData;
    int             priority;
    char *          threadName;

    // we use this trampoline when we need to set the priority with
    // nice/setpriority, and name with prctl.
    static int trampoline(const thread_data_t* t) {
        thread_func_t f = t->entryFunction;
        void* u = t->userData;
        int prio = t->priority;
        char * name = t->threadName;
        delete t;
        setpriority(PRIO_PROCESS, 0, prio);
        pthread_once(&gDoSchedulingGroupOnce, checkDoSchedulingGroup);
        if (gDoSchedulingGroup) {
            if (prio >= ANDROID_PRIORITY_BACKGROUND) {
                set_sched_policy(androidGetTid(), SP_BACKGROUND);
            } else if (prio > ANDROID_PRIORITY_AUDIO) {
                set_sched_policy(androidGetTid(), SP_FOREGROUND);
            } else {
                // defaults to that of parent, or as set by requestPriority()
            }
        }
        
        if (name) {
#if defined(HAVE_PRCTL)
            // Mac OS doesn't have this, and we build libutil for the host too
            int hasAt = 0;
            int hasDot = 0;
            char *s = name;
            while (*s) {
                if (*s == '.') hasDot = 1;
                else if (*s == '@') hasAt = 1;
                s++;
            }
            int len = s - name;
            if (len < 15 || hasAt || !hasDot) {
                s = name;
            } else {
                s = name + len - 15;
            }
            prctl(PR_SET_NAME, (unsigned long) s, 0, 0, 0);
#endif
            free(name);
        }
        return f(u);
    }
};

int androidCreateRawThreadEtc(android_thread_func_t entryFunction,
                               void *userData,
                               const char* threadName,
                               int32_t threadPriority,
                               size_t threadStackSize,
                               android_thread_id_t *threadId)
{
    pthread_attr_t attr; 
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

#ifdef HAVE_ANDROID_OS  /* valgrind is rejecting RT-priority create reqs */
    if (threadPriority != PRIORITY_DEFAULT || threadName != NULL) {
        // Now that the pthread_t has a method to find the associated
        // android_thread_id_t (pid) from pthread_t, it would be possible to avoid
        // this trampoline in some cases as the parent could set the properties
        // for the child.  However, there would be a race condition because the
        // child becomes ready immediately, and it doesn't work for the name.
        // prctl(PR_SET_NAME) only works for self; prctl(PR_SET_THREAD_NAME) was
        // proposed but not yet accepted.
        thread_data_t* t = new thread_data_t;
        t->priority = threadPriority;
        t->threadName = threadName ? strdup(threadName) : NULL;
        t->entryFunction = entryFunction;
        t->userData = userData;
        entryFunction = (android_thread_func_t)&thread_data_t::trampoline;
        userData = t;            
    }
#endif

    if (threadStackSize) {
        pthread_attr_setstacksize(&attr, threadStackSize);
    }
    
    errno = 0;
    pthread_t thread;
    int result = pthread_create(&thread, &attr,
                    (android_pthread_entry)entryFunction, userData);
    pthread_attr_destroy(&attr);
    if (result != 0) {
        ALOGE("androidCreateRawThreadEtc failed (entry=%p, res=%d, errno=%d)\n"
             "(android threadPriority=%d)",
            entryFunction, result, errno, threadPriority);
        return 0;
    }

    // Note that *threadID is directly available to the parent only, as it is
    // assigned after the child starts.  Use memory barrier / lock if the child
    // or other threads also need access.
    if (threadId != NULL) {
        *threadId = (android_thread_id_t)thread; // XXX: this is not portable
    }
    return 1;
}

#ifdef HAVE_ANDROID_OS
static pthread_t android_thread_id_t_to_pthread(android_thread_id_t thread)
{
    return (pthread_t) thread;
}
#endif

android_thread_id_t androidGetThreadId()
{
    return (android_thread_id_t)pthread_self();
}

#else
#error "Threads not supported"
#endif

// ----------------------------------------------------------------------------

int androidCreateThread(android_thread_func_t fn, void* arg)
{
    return createThreadEtc(fn, arg);
}

int androidCreateThreadGetID(android_thread_func_t fn, void *arg, android_thread_id_t *id)
{
    return createThreadEtc(fn, arg, "android:unnamed_thread",
                           PRIORITY_DEFAULT, 0, id);
}

static android_create_thread_fn gCreateThreadFn = androidCreateRawThreadEtc;

int androidCreateThreadEtc(android_thread_func_t entryFunction,
                            void *userData,
                            const char* threadName,
                            int32_t threadPriority,
                            size_t threadStackSize,
                            android_thread_id_t *threadId)
{
    return gCreateThreadFn(entryFunction, userData, threadName,
        threadPriority, threadStackSize, threadId);
}

void androidSetCreateThreadFunc(android_create_thread_fn func)
{
    gCreateThreadFn = func;
}

int32_t androidGetTid()
{
#ifdef HAVE_GETTID
    return gettid();
#else
    return getpid();
#endif
}

#ifdef HAVE_ANDROID_OS
int androidSetThreadPriority(int32_t tid, int pri)
{
    int rc = 0;
    
#if defined(HAVE_PTHREADS)
    int lasterr = 0;

    pthread_once(&gDoSchedulingGroupOnce, checkDoSchedulingGroup);
    if (gDoSchedulingGroup) {
        // set_sched_policy does not support tid == 0
        int policy_tid;
        if (tid == 0) {
            policy_tid = androidGetTid();
        } else {
            policy_tid = tid;
        }
        if (pri >= ANDROID_PRIORITY_BACKGROUND) {
            rc = set_sched_policy(policy_tid, SP_BACKGROUND);
        } else if (getpriority(PRIO_PROCESS, tid) >= ANDROID_PRIORITY_BACKGROUND) {
            rc = set_sched_policy(policy_tid, SP_FOREGROUND);
        }
    }

    if (rc) {
        lasterr = errno;
    }

    if (setpriority(PRIO_PROCESS, tid, pri) < 0) {
        rc = INVALID_OPERATION;
    } else {
        errno = lasterr;
    }
#endif
    
    return rc;
}

int androidGetThreadPriority(int32_t tid) {
#if defined(HAVE_PTHREADS)
    return getpriority(PRIO_PROCESS, tid);
#else
    return ANDROID_PRIORITY_NORMAL;
#endif
}

#endif


namespace ThreadManager{
	
/*
* This is our thread object!
*/
	
Thread::Thread(bool canCallJava)
	:	mCanCallJava(canCallJava),
		mThread(thread_id_t(-1)),
		mHoldSelf(NULL),
		mLock(),
		mStatus(NO_ERROR),
		mExitPending(false), mRunning(false)
#ifdef HAVE_ANDROID_OS
		, mTid(-1)
#endif
{
}
	
Thread::~Thread()
{
}
	
status_t Thread::readyToRun()
{
		return NO_ERROR;
}
	
status_t Thread::run(const char* name, int32_t priority, size_t stack)
{
	Mutex::Autolock _l(mLock);
	
	if (mRunning) {
		// thread already started
		return INVALID_OPERATION;
	}
	
	// reset status and exitPending to their default value, so we can
	// try again after an error happened (either below, or in readyToRun())
	mStatus = NO_ERROR;
	mExitPending = false;
	mThread = thread_id_t(-1);
		
	// hold a strong reference on ourself
	mHoldSelf = this;
	
	mRunning = true;
	
	bool res;
	if (mCanCallJava) {
		res = createThreadEtc(_threadLoop,
				this, name, priority, stack, &mThread);
	} else {
		res = androidCreateRawThreadEtc(_threadLoop,
				this, name, priority, stack, &mThread);
	}
		
	if (res == false) {
		mStatus = UNKNOWN_ERROR;   // something happened!
		mRunning = false;
		mThread = thread_id_t(-1);
		//mHoldSelf.clear();	// "this" may have gone away after this.
	
		return UNKNOWN_ERROR;
	}
		
	// Do not refer to mStatus here: The thread is already running (may, in fact
	// already have exited with a valid mStatus result). The NO_ERROR indication
	// here merely indicates successfully starting the thread and does not
	// imply successful termination/execution.
	return NO_ERROR;
	
	// Exiting scope of mLock is a memory barrier and allows new thread to run
}
	
int Thread::_threadLoop(void* user)
{
	Thread* const self = static_cast<Thread*>(user);
	
#ifdef HAVE_ANDROID_OS
	// this is very useful for debugging with gdb
	self->mTid = gettid();
#endif
	do{
		bool first = true;
		bool result;
		if (first) {
			first = false;
			self->mStatus = self->readyToRun();
			result = (self->mStatus == NO_ERROR);
	
			if (result && !self->exitPending()) {
				result = self->threadLoop();
			}
		} else {
			result = self->threadLoop();
		}
	
	
		{//acquire lock
			Mutex::Autolock _l(self->mLock);
			if (result == false || self->mExitPending) {
				self->mExitPending = true;
				self->mRunning = false;
				// clear thread ID so that requestExitAndWait() does not exit if
				// called by a new thread using the same thread ID as this one.
				self->mThread = thread_id_t(-1);
				// note that interested observers blocked in requestExitAndWait are
				// awoken by broadcast, but blocked on mLock until break exits scope
				self->mThreadExitedCondition.broadcast();
				break;
			}
		}//release lock
	
	}while(true);
		
	return 0;
}
	
void Thread::requestExit()
{
	Mutex::Autolock _l(mLock);
	mExitPending = true;
}
	
status_t Thread::requestExitAndWait()
{
	Mutex::Autolock _l(mLock);
	if (mThread == getThreadId()) {
		ALOGW(
			"Thread (this=%p): don't call waitForExit() from this "
			"Thread object's thread. It's a guaranteed deadlock!",
			this);
	
		return WOULD_BLOCK;
	}
		
	mExitPending = true;
	
	while (mRunning == true) {
		mThreadExitedCondition.wait(mLock);
	}
	// This next line is probably not needed any more, but is being left for
	// historical reference. Note that each interested party will clear flag.
	mExitPending = false;
	
	return mStatus;
}
	
status_t Thread::join()
{
	Mutex::Autolock _l(mLock);
	if (mThread == getThreadId()) {
			ALOGW(
			"Thread (this=%p): don't call join() from this "
			"Thread object's thread. It's a guaranteed deadlock!",
			this);
	
		return WOULD_BLOCK;
	}
	
	while (mRunning == true) {
		mThreadExitedCondition.wait(mLock);
	}
	
	return mStatus;
}
	
#ifdef HAVE_ANDROID_OS
int32_t Thread::getTid() const
{
	// mTid is not defined until the child initializes it, and the caller may need it earlier
	Mutex::Autolock _l(mLock);
	int32_t tid;
	if (mRunning) {
		pthread_t pthread = android_thread_id_t_to_pthread(mThread);
		tid = __pthread_gettid(pthread);
	} else {
		ALOGW("Thread (this=%p): getTid() is undefined before run()", this);
		tid = -1;
	}
	return tid;
}
#endif
	
bool Thread::exitPending() const
{
	Mutex::Autolock _l(mLock);
	return mExitPending;
}

//------------- HandleThread ------------
HandleThread::HandleThread(LooperPolicyInterface*  policy) :
        Thread(/*canCallJava*/ true),mLooper(NULL){
		mPolicy = policy;
}

HandleThread::~HandleThread() {
	delete mLooper;
	mLooper = NULL;
}

LooperInterface* HandleThread::getLooper(){
	Mutex::Autolock _l(lock);
	// If the thread has been started, Please wait until the looper has been created.
	if(NULL == mLooper){
		mThreadCondition.wait(lock);
	}
	ALOGD("Thread number %ld mlooper=%p function=%s line=%d\n", pthread_self(),mLooper,__FUNCTION__,__LINE__);
	return mLooper;
}

bool HandleThread::threadLoop() {
	
	ALOGD("Thread number %ld\n", pthread_self());
	{
		Mutex::Autolock _l(lock);
		mLooper = new Looper(mPolicy);
		if(mLooper != NULL){
			mThreadCondition.broadcast();
		}else{
			return false;
		}
		ALOGD("Thread number %ld function=%s line=%d\n", pthread_self(),__FUNCTION__,__LINE__);
	}
    mLooper->loopOnce();
	
    return true;
}//

}//namespace ThreadManager
