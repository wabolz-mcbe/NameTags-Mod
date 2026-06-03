LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE    := NameTags
LOCAL_SRC_FILES := src/main.cpp
LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/include \
    $(LOCAL_PATH)/extern/includes \
    $(LOCAL_PATH)/extern/includes/beatsaber-hook/shared \
    $(LOCAL_PATH)/extern/includes/gorilla-utils/shared \
    $(LOCAL_PATH)/extern/includes/codegen/include

LOCAL_CPPFLAGS  := -std=c++20 -O2 -frtti -fexceptions \
                   -DVERSION='"1.0.0"' \
                   -DID='"NameTags"'

LOCAL_LDLIBS    := -llog -landroid
LOCAL_SHARED_LIBRARIES := beatsaber-hook gorilla-utils

include $(BUILD_SHARED_LIBRARY)
