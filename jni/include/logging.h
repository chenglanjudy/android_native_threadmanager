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


#define LOG_TAG "THREAD_MANAGER"

#include "util.h"
#include <android/log.h>


//namespace ThreadManager{

#ifdef ANDROID // set to 0 to print directly to console

#define LOG_NDDEBUG		0	// ALOGD
#define LOG_NDEBUG		0	// ALOGV
#define LOG_NIDEBUG		0	// ALOGI

#define __XLOG(lvl, ...) \
  __android_log_print(lvl, LOG_TAG, __VA_ARGS__)
#else
#include <stdio.h>
#define __XLOG(lvl, ...) \
  printf(__VA_ARGS__); printf("\n");
#endif

#define ALOGE(...) \
	__XLOG(ANDROID_LOG_ERROR, __VA_ARGS__)
	
#define ALOGW(...) \
	__XLOG(ANDROID_LOG_WARN, __VA_ARGS__)
	
#define ALOGI(...) \
	__XLOG(ANDROID_LOG_INFO, __VA_ARGS__)

#define ALOGV(...) \
	__XLOG(ANDROID_LOG_VERBOSE, __VA_ARGS__)

#define ALOGD(...) \
	__XLOG(ANDROID_LOG_DEBUG, __VA_ARGS__)

//}//namespace ThreadManager
