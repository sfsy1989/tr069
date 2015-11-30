LOCAL_PATH := $(call my-dir)
#LIB_PATH := $(IPTV_APK_PATH)/libs
#INCLUDE_PATH := $(IPTV_APK_PATH)/include
ANDROID_BUILD_TOP := $(SDK_DIR)/../../../../
include $(CLEAR_VARS)


LOCAL_MODULE := fv_httpd

LOCAL_SRC_FILES := \
	httpd/fv_httpd.c \
	httpd/mongoose.c

LOCAL_SHARED_LIBRARIES := \
	libssl \
	libcrypto \
	libicuuc

LOCAL_STATIC_LIBRARIES := \
	libxml2

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
	-DFTM_USE_FVIPC=1 \
	-D_POSIX_SOURCE -D_BSD_SOURCE -D_FILE_OFFSET_BITS=64 \
	-D_LARGEFILE_SOURCE -W -Wall -std=c99 -pedantic -Os -fomit-frame-pointer

LOCAL_CFLAGS += $(FV_INCLUDE)

LOCAL_LDFLAGS := \
	-ldl

LOCAL_LDFLAGS += $(FV_LFLAGS)

include $(BUILD_EXECUTABLE)

#tr069.cgi
include $(CLEAR_VARS)

LOCAL_MODULE := tr069.cgi

LOCAL_SRC_FILES := \
	httpd/cgic.c \
	httpd/tr069cgi.c

LOCAL_SHARED_LIBRARIES := \
	libssl \
	libcrypto \
	libicuuc

LOCAL_STATIC_LIBRARIES := \
	libxml2

LOCAL_C_INCLUDES := \
	$(ANDROID_BUILD_TOP)/external/openssl/include \
	$(ANDROID_BUILD_TOP)/external/libxml2/include \
	$(ANDROID_BUILD_TOP)/external/icu4c/common

LOCAL_CFLAGS := \
	-Wno-error=format-security \
	-DPREFIX="\"/ipstb"\" \
	-DCPE_VER=1 -DCPE_BUILD_TIME="\"1\"" \
	-DFTM_USE_FVIPC=1

FV_INCLUDE := -I$(ANDROID_BUILD_TOP)/external/curl/include -I$(ANDROID_BUILD_TOP)/external/syslib/flib/include -I$(ANDROID_BUILD_TOP)/external/syslib/nlib/include -I$(ANDROID_BUILD_TOP)/external/syslib/olib/include \
              -I$(ANDROID_BUILD_TOP)/external/syslib/opensource/iniparser

FV_LFLAGS := -L$(LIB_PATH) -liniparser -liop -lcurl -lflib -lnlib -lolib

LOCAL_CFLAGS += $(FV_INCLUDE)

LOCAL_LDFLAGS := $(FV_LFLAGS)

include $(BUILD_EXECUTABLE)
