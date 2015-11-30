LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
ANDROID_BUILD_TOP := $(LOCAL_PATH)/../../../../../../../
LOCAL_MODULE := libtr069

LOCAL_SRC_FILES := \
	tr069/cu_usb_config.c \
	tr069/tr069.c \
	tr069/tr069_control.c \
	tr069/tr069_cookie.c \
	tr069/tr069_dispatch_fvipc.c \
	tr069/tr069_interface.c \
	tr069/tr069_main.c \
	tr069/tr069_parameter.c \
	tr069/tr069_ping.c \
	tr069/tr069_rpc.c \
	tr069/tr069_service.c \
	tr069/tr069_session.c \
	tr069/tr069_speed_test.c \
	tr069/tr069_statistics.c \
	tr069/tr069_timer.c \
	tr069/tr069_traceroute.c \
	tr069/tr111_control.c

LOCAL_SHARED_LIBRARIES := \
	libssl \
	libcrypto \
	libicuuc


LOCAL_C_INCLUDES := \
	$(ANDROID_BUILD_TOP)/external/openssl/include \
	$(ANDROID_BUILD_TOP)/external/libxml2/include \
	$(ANDROID_BUILD_TOP)/external/icu4c/common

FV_INCLUDE := -I$(ANDROID_BUILD_TOP)/external/curl/include -I$(ANDROID_BUILD_TOP)/external/syslib/flib/include -I$(ANDROID_BUILD_TOP)/external/syslib/nlib/include -I$(ANDROID_BUILD_TOP)/external/syslib/olib/include \
              -I$(ANDROID_BUILD_TOP)/external/syslib/opensource/iniparser
FV_LFLAGS := -L$(LIB_PATH) -liniparser -liop -lcurl -lflib -lnlib -lolib

LOCAL_CFLAGS := \
	-Wno-error=format-security \
	-DPREFIX="\"/ipstb"\" \
	-DCPE_VER=1 -DCPE_BUILD_TIME="\"1\"" \
	-DFTM_USE_FVIPC=1

LOCAL_CFLAGS += $(FV_INCLUDE)
LOCAL_LDFLAGS := $(FV_LFLAGS)


include $(BUILD_STATIC_LIBRARY)

