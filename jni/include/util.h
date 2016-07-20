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


#pragma once

#include <errno.h>
#include <stdint.h>
#include "util.h"

#define ANDROID 1 // Used in Android OS.

#define HAVE_PTHREADS  1 // Support linux pthread.

#define HAVE_ANDROID_OS  1 // The function same as ANDROID.

#define HAVE_PRCTL 1 // For a specific operation process.

#define MAX_VALUE  0x7FFFFFFF

typedef int int32_t;
typedef unsigned char Boolean;
typedef unsigned short int uint16_t;
typedef unsigned int uint32_t;
typedef int32_t     status_t;


#define HANDLE_ERRNO(expr, ...) \
	{ \
		int _x = expr; \
		if (_x < 0) { \
			ALOGE(__VA_ARGS__); \
			return -errno; \
		} \
	}

#define LOG_IF_ERRNO(expr, ...) \
	{ \
		int _x = expr; \
		if (_x) { \
			ALOGE(__VA_ARGS__); \
		} \
	}

#define LOG_IF_ERRNO_RETURN(expr,value, ...) \
	{ \
		int _x = expr; \
		if (_x) { \
			ALOGE(__VA_ARGS__); \
			return value; \
		} \
	}




#define HANDLE_VOID(expr, ...) \
	{ \
		int _x = expr; \
		if (_x < 0) { \
			ALOGE(__VA_ARGS__); \
			return; \
		} \
	}

#define HANDLE_STATUS(expr, ...) \
	{ \
		int _x_ = expr; \
		if (_x_ < 0) { \
			ALOGE(__VA_ARGS__); \
			return _x_; \
		} \
	}

/* Returns the time in seconds */
uint32_t getTimestamp();

/* Returns the time in seconds */
uint64_t getTimestampNs();

