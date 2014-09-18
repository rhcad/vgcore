# Copyright (c) 2013, Zhang Yungui, https://github.com/rhcad/touchvg

LOCAL_PATH := $(call my-dir)
cflags     := -frtti -Wall -Wextra -Wno-unused-parameter

core_inc   := $(call my-dir)/../../../core/include
shape_incs := $(core_inc) \
              $(core_inc)/geom \
              $(core_inc)/gshape \
              $(core_inc)/storage

core_src   := ../../../core/src

geom_files := $(core_src)/geom/mgbase.cpp \
              $(core_src)/geom/mgbox.cpp \
              $(core_src)/geom/mgcurv.cpp \
              $(core_src)/geom/mglnrel.cpp \
              $(core_src)/geom/mgmat.cpp \
              $(core_src)/geom/mgnear.cpp \
              $(core_src)/geom/mgnearbz.cpp \
              $(core_src)/geom/fitcurves.cpp \
              $(core_src)/geom/mgvec.cpp \
              $(core_src)/geom/mgpnt.cpp \
              $(core_src)/geom/mgpath.cpp \
              $(core_src)/geom/nanosvg.cpp

gshape_files := $(core_src)/gshape/mgarc.cpp \
              $(core_src)/gshape/mgbasesp.cpp \
              $(core_src)/gshape/mgcshapes.cpp \
              $(core_src)/gshape/mgdiamond.cpp \
              $(core_src)/gshape/mgdot.cpp \
              $(core_src)/gshape/mgellipse.cpp \
              $(core_src)/gshape/mggrid.cpp \
              $(core_src)/gshape/mgline.cpp \
              $(core_src)/gshape/mglines.cpp \
              $(core_src)/gshape/mgparallel.cpp \
              $(core_src)/gshape/mgpathsp.cpp \
              $(core_src)/gshape/mgrdrect.cpp \
              $(core_src)/gshape/mgrect.cpp \
              $(core_src)/gshape/mgsplines.cpp \
              $(core_src)/gshape/mgarccross.cpp

include $(CLEAR_VARS)
LOCAL_MODULE     := libVGShape
LOCAL_CFLAGS     := $(cflags)

ifeq ($(TARGET_ARCH),x86)
# For SWIG, http://stackoverflow.com/questions/6753241
LOCAL_CFLAGS += -fno-strict-aliasing
endif

LOCAL_C_INCLUDES := $(shape_incs)
LOCAL_SRC_FILES  := $(geom_files) $(gshape_files)
include $(BUILD_STATIC_LIBRARY)
