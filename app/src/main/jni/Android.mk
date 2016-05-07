LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

#override OPENCV_INSTALL_MODULES:=on
override OPENCV_CAMERA_MODULES:=on

ifdef OPENCV_ANDROID_SDK
  ifneq ("","$(wildcard $(OPENCV_ANDROID_SDK)/OpenCV.mk)")
    include ${OPENCV_ANDROID_SDK}/OpenCV.mk
  else
    include ${OPENCV_ANDROID_SDK}/sdk/native/jni/OpenCV.mk
  endif
else
  include C:\OpenCV\OpenCV-android-sdk\sdk\native\jni\OpenCV.mk
endif

LOCAL_MODULE    := imageproc
LOCAL_SRC_FILES := detection_and_drawing.cpp aruco.cpp dictionary.cpp
LOCAL_C_INCLUDES += $(LOCAL_PATH)
LOCAL_LDLIBS +=  -llog -ldl -march=armv7-a -Wl,--fix-cortex-a8

include $(BUILD_SHARED_LIBRARY)
