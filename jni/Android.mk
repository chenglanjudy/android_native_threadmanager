# Copyright (C) 2010 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
LOCAL_PATH := $(call my-dir)

#####################################################
#####################################################
include $(CLEAR_VARS)

LOCAL_MODULE    := JNIThreads

# find CPP files.
define all-cpp-files-under
$(patsubst ./%,%, \
  $(shell cd $(LOCAL_PATH) ; \
          find $(1) -name "include" -prune -o -name "*.cpp" -and -not -name ".*") \
 )
endef

define all-subdir-cpp-files
$(call all-cpp-files-under,.)
endef

# find C files.
define all-c-files-under  
$(patsubst ./%,%, $(shell find $(LOCAL_PATH) -name "include" -prune -o -name "*.c" -and -not -name ".*"))  
endef  
  
define all-subdir-c-files  
$(call all-c-files-under,.)  
endef

CPP_FILES := $(call all-subdir-cpp-files)

C_FILES   := $(call all-subdir-c-files)

LOCAL_SRC_FILES := $(CPP_FILES:$(LOCAL_PATH)/%=%) \
				   $(C_FILES:$(LOCAL_PATH)/%=%)
	
LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/include 
	#$(LOCAL_PATH)/../../../../frameworks/native/include \
	#$(LOCAL_PATH)/../../../../system/core/include
	 
# for logging
LOCAL_LDLIBS    += -llog
LOCAL_CFLAGS += -fno-rtti -fno-exceptions -g
# for native asset manager
LOCAL_LDLIBS    += -landroid

include $(BUILD_SHARED_LIBRARY)
#include $(BUILD_EXECUTABLE)


include $(call all-makefiles-under,$(LOCAL_PATH))

#####################################################
#####################################################
