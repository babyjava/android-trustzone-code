LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := tee_test
LOCAL_PROPRIETARY_MODULE := true
LOCAL_SRC_FILES := tee_test.c ./platform/gp.c ./platform/isee.c \
		./platform/kinibi.c ./platform/qsee.c ./platform/trusty.c
LOCAL_C_INCLUDES += $(LOCAL_PATH)/inc
LOCAL_HEADER_LIBRARIES := libhardware_headers libutils_headers libsystem_headers
LOCAL_SHARED_LIBRARIES := liblog libcutils libutils libhardware
# include $(BUILD_STATIC_LIBRARY)
include $(BUILD_EXECUTABLE)
#########################################################################