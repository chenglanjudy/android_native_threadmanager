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


#include "util.h"


#include <time.h>
#include <stdio.h>

uint32_t getTimestamp() {
	struct timespec tp;
	uint32_t result;

	clock_gettime(CLOCK_MONOTONIC, &tp);

	result = ((uint32_t) tp.tv_sec * 1000) + ((uint32_t) tp.tv_nsec / 1000000);
	return result;
}

uint64_t getTimestampNs() {
	struct timespec tp;
	int64_t result;

	clock_gettime(CLOCK_MONOTONIC, &tp);

	result = ((int64_t) tp.tv_sec * 1000000000LL) + ((int64_t) tp.tv_nsec);
	return result;
}
