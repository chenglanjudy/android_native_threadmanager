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


#ifndef _LIBS_THREAD_H
#define _LIBS_THREAD_H

#include <stdint.h>
#include <sys/types.h>
#include <time.h>
#include <jni.h>

#if defined(HAVE_PTHREADS)
# include <pthread.h>
#endif

#include "Errors.h"
#include "Mutex.h"
#include "util.h"
#include "Timers.h"
#include "ThreadDefs.h"
#include "Condition.h"
//#include "Looper.h"


#define PROPERTY_VALUE_MAX  92

#define PR_SET_NAME		15               /* Set process name */

// ---------------------------------------------------------------------------
namespace ThreadManager {
// ---------------------------------------------------------------------------

class LooperInterface;
class LooperPolicyInterface;

class Thread
{
public:
    // Create a Thread object, but doesn't create or start the associated
    // thread. See the run() method.
                        Thread(bool canCallJava = true);
    virtual             ~Thread();

    // Start the thread in threadLoop() which needs to be implemented.
    virtual status_t    run(    const char* name = 0,
                                int32_t priority = PRIORITY_DEFAULT,
                                size_t stack = 0);
    
    // Ask this object's thread to exit. This function is asynchronous, when the
    // function returns the thread might still be running. Of course, this
    // function can be called from a different thread.
    virtual void        requestExit();

    // Good place to do one-time initializations
    virtual status_t    readyToRun();
    
    // Call requestExit() and wait until this object's thread exits.
    // BE VERY CAREFUL of deadlocks. In particular, it would be silly to call
    // this function from this object's thread. Will return WOULD_BLOCK in
    // that case.
            status_t    requestExitAndWait();

    // Wait until this object's thread exits. Returns immediately if not yet running.
    // Do not call from this object's thread; will return WOULD_BLOCK in that case.
            status_t    join();

#ifdef HAVE_ANDROID_OS
    // Return the thread's kernel ID, same as the thread itself calling gettid() or
    // androidGetTid(), or -1 if the thread is not running.
            int32_t     getTid() const;
#endif

protected:
    // exitPending() returns true if requestExit() has been called.
            bool        exitPending() const;
			
private:
    // loop: if threadLoop() returns true, it will be called again if
    //          requestExit() wasn't called
    virtual bool        threadLoop() = 0;

private:
    Thread& operator=(const Thread&);
    static  int             _threadLoop(void* user);
    const   bool            mCanCallJava;
	mutable Mutex           mLock;
    // always hold mLock when reading or writing
            thread_id_t     mThread;
    
	  		Condition       mThreadExitedCondition;
            status_t        mStatus;
    // note that all accesses of mExitPending and mRunning need to hold mLock
    volatile bool           mExitPending;
    volatile bool           mRunning;
            Thread*			mHoldSelf;
#ifdef HAVE_ANDROID_OS
    // legacy for debugging, not used by getTid() as it is set by the child thread
    // and so is not initialized until the child reaches that point
            int32_t         mTid;
#endif
};

//----------------------------------------

class PP{
public:
		JavaVM *mJVM;
		JNIEnv *mJNIEnv;
		jobject g_obj;
	};


class HandleThread : public Thread{
public:
	HandleThread(LooperPolicyInterface*  policy);
	virtual ~HandleThread();
	virtual LooperInterface* getLooper();
	
private:
	virtual bool  			threadLoop();
	
private:	
    LooperInterface* 		mLooper;
	Mutex           		lock;
	Condition       		mThreadCondition;
	LooperPolicyInterface*  mPolicy;
};

}// namespace ThreadManager

// ---------------------------------------------------------------------------
#endif // _LIBS_UTILS_THREAD_H
// ---------------------------------------------------------------------------
