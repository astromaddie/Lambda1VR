LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := openxr_loader

# The Steam Frame build passes the Khronos loader it fetched
ifneq ($(L1VR_OPENXR_LOADER),)
LOCAL_SRC_FILES := $(L1VR_OPENXR_LOADER)
LOADER_FILE := $(L1VR_OPENXR_LOADER)
else
LOCAL_SRC_FILES := lib$(LOCAL_MODULE).so
LOADER_FILE := $(LOCAL_PATH)/$(LOCAL_SRC_FILES)
endif

# NOTE: This check is added to prevent the following error when running a "make clean" where
# the prebuilt lib may have been deleted: "LOCAL_SRC_FILES points to a missing file"
ifneq (,$(wildcard $(LOADER_FILE)))
  include $(PREBUILT_SHARED_LIBRARY)
endif
