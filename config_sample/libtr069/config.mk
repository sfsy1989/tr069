LOCAL_PATH := $(call my-dir)
LIB_PATH := $(IPTV_APK_PATH)/libs
INCLUDE_PATH := $(IPTV_APK_PATH)/include
ANDROID_BUILD_TOP := $(SDK_DIR)/../../../../

include $(CLEAR_VARS)

LOCAL_MODULE := libconfig

LOCAL_SRC_FILES := \
	config/cfg.c \
	config/cfg_dispatch_fvipc.c \
	config/cfg_file.c \
	config/cfg_main.c \
	config/cfg_service.c

LOCAL_SHARED_LIBRARIES := \
	libssl \
	libcrypto \
	libicuuc

LOCAL_C_INCLUDES := \
	$(ANDROID_BUILD_TOP)/external/openssl/include \
	$(ANDROID_BUILD_TOP)/external/libxml2/include \
	$(ANDROID_BUILD_TOP)/external/icu4c/common

LOCAL_CFLAGS := \
	-Wno-error=format-security \
	-DPREFIX="\"/ipstb"\" \
	-DFTM_USE_FVIPC=1

FV_INCLUDE := -I$(ANDROID_BUILD_TOP)/external/curl/include -I$(ANDROID_BUILD_TOP)/external/syslib/flib/include -I$(ANDROID_BUILD_TOP)/external/syslib/nlib/include -I$(ANDROID_BUILD_TOP)/external/syslib/olib/include \
              -I$(ANDROID_BUILD_TOP)/external/syslib/opensource/iniparser
FV_LFLAGS := -L$(LIB_PATH) -liniparser -liop -lcurl -lflib -lnlib -lolib

LOCAL_CFLAGS += $(FV_INCLUDE)
LOCAL_LDFLAGS := $(FV_LFLAGS)

include $(BUILD_STATIC_LIBRARY)

# test
include $(CLEAR_VARS)

LOCAL_MODULE := configtest

LOCAL_SRC_FILES := \
	config/cfg_test.c

LOCAL_C_INCLUDES := \
	$(ANDROID_BUILD_TOP)/external/openssl/include \
	$(ANDROID_BUILD_TOP)/external/libxml2/include \
	$(ANDROID_BUILD_TOP)/external/icu4c/common

LOCAL_CFLAGS := \
	-Wno-error=format-security \
	-DPREFIX="\"/ipstb"\" \
	-DFTM_USE_FVIPC=1

FV_INCLUDE := -I$(ANDROID_BUILD_TOP)/external/curl/include -I$(ANDROID_BUILD_TOP)/external/syslib/flib/include -I$(ANDROID_BUILD_TOP)/external/syslib/nlib/include -I$(ANDROID_BUILD_TOP)/external/syslib/olib/include \
              -I$(ANDROID_BUILD_TOP)/external/syslib/opensource/iniparser
FV_LFLAGS := -L$(LIB_PATH) -liniparser -liop -lcurl -lflib -lnlib -lolib

LOCAL_CFLAGS += $(FV_INCLUDE)
LOCAL_LDFLAGS := $(FV_LFLAGS)


include $(BUILD_EXECUTABLE)
