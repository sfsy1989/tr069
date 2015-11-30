TR069_PATH:= $(call my-dir)
SDK_DIR := $(TR069_PATH)/../../

include $(TR069_PATH)/libtr069/Android.mk
include $(TR069_PATH)/compile.mk
