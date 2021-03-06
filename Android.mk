LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	main.cpp \
	SkiWinView.cpp \
	SkiWinEventListener.cpp \
	SkiWin.cpp \
	SkiWinURLResource.cpp

LOCAL_CFLAGS += -DGL_GLEXT_PROTOTYPES -DEGL_EGLEXT_PROTOTYPES

LOCAL_CFLAGS += -std=gnu++0x -Wno-non-virtual-dtor

LOCAL_SHARED_LIBRARIES := \
	libcutils \
	libandroidfw \
	libutils \
	libbinder \
    	libui \
	libskia \
	libandroid_runtime \
	libEGL \
	libGLESv2 \
	libGLESv1_CM \
	libgui \
	libinput \
	libandroid_servers \
	libstlport \
	libcurl \
	libchromium_net \
	libwebcore
	

LOCAL_STATIC_LIBRARIES := \
    	libskiagpu 


LOCAL_C_INCLUDES := \
	$(call include-path-for, corecg graphics) \
	external/skia/include/core \
	external/skia/include/config \
	external/skia/include/effects \
	external/skia/include/images \
	external/skia/include/utils \
	external/skia/include/utils/android \
	external/skia/include/views \
	external/skia/include/pdf \
	external/skia/include/pipe \
	external/skia/include/xml \
	external/skia/include/gpu \
	external/skia/src/core \
	external/skia/gpu/include \
	external/skia/include/animator \
	frameworks/base/native/include/android \
	frameworks/native/services/surfaceflinger \
	frameworks/base/services\
	SkiaSamples \
	external/chromium/googleurl/src \
	external/chromium \
	external/chromium/android \
	external/stlport/stlport \
	external/curl/include \
	bionic/libstdc++/include \
	bionic 

LOCAL_MODULE:= SkiWin

include external/skia/src/views/views_files.mk
LOCAL_SRC_FILES += $(addprefix ../../external/skia/src/views/, $(SOURCE))

include external/skia/src/pipe/pipe_files.mk
LOCAL_SRC_FILES += $(addprefix ../../external/skia/src/pipe/, $(SOURCE))

include external/skia/src/pdf/pdf_files.mk
LOCAL_SRC_FILES += $(addprefix ../../external/skia/src/pdf/, $(SOURCE))

include external/skia/src/xml/xml_files.mk
LOCAL_SRC_FILES += $(addprefix ../../external/skia/src/xml/, $(SOURCE))

include $(LOCAL_PATH)/SkiaSamples/samplecode_files.mk
LOCAL_SRC_FILES += $(addprefix SkiaSamples/, $(SOURCE))

include $(BUILD_EXECUTABLE)
