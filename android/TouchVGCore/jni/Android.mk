# Copyright (c) 2013, Zhang Yungui, https://github.com/rhcad/touchvg

LOCAL_PATH := $(call my-dir)
cflags     := -frtti -Wall -Wextra -Wno-unused-parameter

core_inc   := $(call my-dir)/../../../core/include
shape_incs := $(core_inc) \
              $(core_inc)/canvas \
              $(core_inc)/geom \
              $(core_inc)/graph \
              $(core_inc)/jsonstorage \
              $(core_inc)/shape \
              $(core_inc)/storage \
              $(core_inc)/shapedoc \
              $(core_inc)/test

cmd_incs   := $(core_inc)/cmd \
              $(core_inc)/cmdbase \
              $(core_inc)/cmdobserver \
              $(core_inc)/cmdbasic \
              $(core_inc)/cmdmgr
              
view_incs  := $(core_inc)/view \
              $(core_inc)/export \
              $(core_inc)/record

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
              $(core_src)/geom/mgpnt.cpp

graph_files := $(core_src)/graph/gigraph.cpp \
              $(core_src)/graph/gipath.cpp \
              $(core_src)/graph/gixform.cpp

json_files := $(core_src)/jsonstorage/mgjsonstorage.cpp

shape_files := $(core_src)/shape/mgcomposite.cpp \
              $(core_src)/shape/mgellipse.cpp \
              $(core_src)/shape/mggrid.cpp \
              $(core_src)/shape/mgline.cpp \
              $(core_src)/shape/mglines.cpp \
              $(core_src)/shape/mgrdrect.cpp \
              $(core_src)/shape/mgrect.cpp \
              $(core_src)/shape/mgshape.cpp \
              $(core_src)/shape/mgshapes.cpp \
              $(core_src)/shape/mgsplines.cpp \
              $(core_src)/shape/mgbasicspreg.cpp

doc_files  := $(core_src)/shapedoc/mgshapedoc.cpp \
              $(core_src)/shapedoc/mglayer.cpp \
              $(core_src)/shapedoc/spfactoryimpl.cpp

test_files := $(core_src)/test/testcanvas.cpp \
              $(core_src)/test/RandomShape.cpp

base_files := $(core_src)/cmdbase/mgcmddraw.cpp \
              $(core_src)/cmdbase/mgdrawarc.cpp \
              $(core_src)/cmdbase/mgdrawrect.cpp

basic_files := $(core_src)/cmdbasic/cmdbasic.cpp \
              $(core_src)/cmdbasic/mgcmderase.cpp \
              $(core_src)/cmdbasic/mgdrawcircle.cpp \
              $(core_src)/cmdbasic/mgdrawdiamond.cpp \
              $(core_src)/cmdbasic/mgdrawellipse.cpp \
              $(core_src)/cmdbasic/mgdrawfreelines.cpp \
              $(core_src)/cmdbasic/mgdrawgrid.cpp \
              $(core_src)/cmdbasic/mgdrawline.cpp \
              $(core_src)/cmdbasic/mgdrawlines.cpp \
              $(core_src)/cmdbasic/mgdrawparallel.cpp \
              $(core_src)/cmdbasic/mgdrawpolygon.cpp \
              $(core_src)/cmdbasic/mgdrawsplines.cpp \
              $(core_src)/cmdbasic/mgdrawsquare.cpp \
              $(core_src)/cmdbasic/mgdrawtriang.cpp

mgr_files  := $(core_src)/cmdmgr/cmdsubject.cpp \
              $(core_src)/cmdmgr/mgactions.cpp \
              $(core_src)/cmdmgr/mgcmdmgr_.cpp \
              $(core_src)/cmdmgr/mgcmdmgr2.cpp \
              $(core_src)/cmdmgr/mgcmdselect.cpp \
              $(core_src)/cmdmgr/mgsnapimpl.cpp

view_files := $(core_src)/view/GcGraphView.cpp \
              $(core_src)/view/GcMagnifierView.cpp \
              $(core_src)/view/GcShapeDoc.cpp \
              $(core_src)/view/gicoreview.cpp \
              $(core_src)/view/gicoreplay.cpp \
              $(core_src)/export/svgcanvas.cpp \
              $(core_src)/export/girecordcanvas.cpp \
              $(core_src)/record/recordshapes.cpp

include $(CLEAR_VARS)
LOCAL_MODULE     := libTouchVGCore
LOCAL_CFLAGS     := $(cflags)
LOCAL_C_INCLUDES := $(shape_incs) $(cmd_incs) $(view_incs)
LOCAL_SRC_FILES  := $(geom_files) $(graph_files) $(shape_files) \
                    $(json_files) $(test_files) $(doc_files) \
                    $(base_files) $(basic_files) $(mgr_files) \
                    $(view_files)
include $(BUILD_STATIC_LIBRARY)
