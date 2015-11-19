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


#ifndef _BIONIC_PTHREAD_H
#define _BIONIC_PTHREAD_H

#include <sys/cdefs.h>

__BEGIN_DECLS

/* Internal, not an NDK API */
extern pid_t __pthread_gettid(pthread_t thid);
extern int __pthread_settid(pthread_t thid, pid_t tid);

__END_DECLS

#endif /* _BIONIC_PTHREAD_H */
