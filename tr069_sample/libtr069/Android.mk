LOCAL_PATH := $(call my-dir)

# tr069 includes 3 modules: httpd, config and tr069
include $(LOCAL_PATH)/httpd.mk
include $(LOCAL_PATH)/tr069.mk

