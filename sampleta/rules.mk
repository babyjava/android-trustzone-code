# Copyright (C) 2015 The Android Open Source Project
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

LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
    $(LOCAL_DIR)/platform/trusty/manifest.c \
    $(LOCAL_DIR)/platform/trusty/trusty.c \
    $(wildcard $(LOCAL_DIR)/src/*.c)

MODULE_CFLAGS := -DTEE_TRUSTY

MODULE_DEPS += \
        app/trusty \

MODULE_DEPS_STATIC += \
        libc \
        libc-trusty \
        libstdc++-trusty \
        storage \
        libm \
        rng \
        openssl \
        keymaster \

MODULE_INCLUDES += \
        $(LOCAL_DIR) \
        $(LOCAL_DIR)/inc \

include make/module-user_task.mk
include make/module.mk

