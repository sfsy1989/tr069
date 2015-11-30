LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
include $(SDK_DIR)/Android.def
ANDROID_BUILD_TOP := $(SDK_DIR)/../../../../
LOCAL_MODULE := sample_config
ALL_DEFAULT_INSTALLED_MODULES += $(LOCAL_MODULE)

LOCAL_MODULE_TAGS := optional

LOCAL_CFLAGS := $(CFG_HI_CFLAGS) $(CFG_HI_BOARD_CONFIGS)
LOCAL_CFLAGS += -DLOG_TAG=\"$(LOCAL_MODULE)\"

LOCAL_SRC_FILES := main.c

LOCAL_C_INCLUDES := $(COMMON_UNF_INCLUDE)
LOCAL_C_INCLUDES += $(COMMON_DRV_INCLUDE)
LOCAL_C_INCLUDES += $(COMMON_API_INCLUDE)
LOCAL_C_INCLUDES += $(MSP_UNF_INCLUDE)
LOCAL_C_INCLUDES += $(MSP_DRV_INCLUDE)
LOCAL_C_INCLUDES += $(MSP_API_INCLUDE)
LOCAL_C_INCLUDES += $(SAMPLE_DIR)/common
LOCAL_C_INCLUDES += $(LOCAL_PATH)/libtr069/config
#LOCAL_C_INCLUDES += $(LOCAL_PATH)/libtr069/httpd
#LOCAL_C_INCLUDES += $(LOCAL_PATH)/libtr069/tr069

FV_INCLUDE := $(ANDROID_BUILD_TOP)/external/curl/include $(ANDROID_BUILD_TOP)/external/syslib/flib/include $(ANDROID_BUILD_TOP)/external/syslib/nlib/include $(ANDROID_BUILD_TOP)/external/syslib/olib/include \
              $(ANDROID_BUILD_TOP)/external/syslib/opensource/iniparser

LOCAL_C_INCLUDES += $(FV_INCLUDE)

LOCAL_STATIC_LIBRARIES := \
	libconfig \
	libxml2
	
LOCAL_SHARED_LIBRARIES := libssl libcrypto libicuuc libiniparser libnlib libolib libiop libinput libcurl libflib

include $(BUILD_EXECUTABLE)
