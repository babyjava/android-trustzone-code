# Copyright (C) 2021 The Android Open Source Project
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

TARGET = f888888817d366889b01c992d56d78bf.ta

SRC_C := $(wildcard ./src/*.c) $(wildcard ./platform/isee/*.c)
MY_CFLAGS +=-DTEE_ISEE

MY_CFLAGS += -mfloat-abi=softfp \
	-I$(UT_SDK_HOME)/sampleta/inc \
	$(CFLAGS_PF_LOG) \
	$(CFLAGS_SYS_BASE) \
	$(CFLAGS_PF_CRYPTO) \
	$(CFLAGS_PF_FP) \
	$(CFLAGS_PF_KM) \
	$(CFLAGS_PF_SPI) \
	$(CFLAGS_PF_TIME) \
	$(CFLAGS_PF_TS) \
	$(CFLAGS_PF_FP_API) \
	$(CFLAGS_PF_GP_NATIVE) \
	$(CFLAGS_PF_UT_BTA)

MY_LDFLAGS += $(LDFLAGS_PF_LOG) \
	$(LDFLAGS_PF_CRYPTO) \
	$(LDFLAGS_PF_FP) \
	$(LDFLAGS_PF_KM) \
	$(LDFLAGS_PF_SPI) \
	$(LDFLAGS_PF_TIME) \
	$(LDFLAGS_PF_TS) \
	$(LDFLAGS_PF_UT_BTA) \
	$(LDFLAGS_SYS_BASE) \
	$(LDFLAGS_SYS_IO) \
	$(LDFLAGS_SYS_IRQ)
#	-L$(UT_SDK_HOME)/sampleta/lib -lm \

MODE = shared

include $(UT_SDK_HOME)/build/prog.mk

install:
	adb root && adb remount && adb push obj/$(TARGET) /system/vendor/thh/ta
	adb shell chmod 666 /dev/teei_client && adb shell chmod 666 /dev/tee0
