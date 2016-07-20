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


/*
 *  Encapsulate a mutex for thread synchronization.
 */

#include "Mutex.h"
#include "logging.h"
#include <errno.h>

namespace ThreadManager{
/*******************************************************************************
**
** Function:        Mutex
**
** Description:     Initialize member variables.
**
** Returns:         None.
**
*******************************************************************************/
Mutex::Mutex()
{
    memset (&mMutex, 0, sizeof(mMutex));
    int res = pthread_mutex_init (&mMutex, NULL);
    if (res != 0)
    {
        ALOGE ("Mutex::Mutex: fail init; error=0x%X", res);
    }
}


/*******************************************************************************
**
** Function:        ~Mutex
**
** Description:     Cleanup all resources.
**
** Returns:         None.
**
*******************************************************************************/
Mutex::~Mutex()
{
    int res = pthread_mutex_destroy (&mMutex);
    if (res != 0)
    {
        ALOGE ("Mutex::~Mutex: fail destroy; error=0x%X", res);
    }
}


/*******************************************************************************
**
** Function:        lock
**
** Description:     Block the thread and try lock the mutex.
**
** Returns:         None.
**
*******************************************************************************/
void Mutex::lock()
{
    int res = pthread_mutex_lock (&mMutex);
    if (res != 0)
    {
        ALOGE ("Mutex::lock: fail lock; error=0x%X", res);
    }
}


/*******************************************************************************
**
** Function:        unlock
**
** Description:     Unlock a mutex to unblock a thread.
**
** Returns:         None.
**
*******************************************************************************/
void Mutex::unlock()
{
    int res = pthread_mutex_unlock (&mMutex);
    if (res != 0)
    {
        ALOGE ("Mutex::unlock: fail unlock; error=0x%X", res);
    }
}


/*******************************************************************************
**
** Function:        tryLock
**
** Description:     Try to lock the mutex.
**
** Returns:         True if the mutex is locked.
**
*******************************************************************************/
bool Mutex::tryLock()
{
    int res = pthread_mutex_trylock (&mMutex);
    if ((res != 0) && (res != EBUSY))
    {
        ALOGE ("Mutex::tryLock: error=0x%X", res);
    }
    return res == 0;
}


/*******************************************************************************
**
** Function:        nativeHandle
**
** Description:     Get the handle of the mutex.
**
** Returns:         Handle of the mutex.
**
*******************************************************************************/
pthread_mutex_t* Mutex::nativeHandle()
{
    return &mMutex;
}

}

