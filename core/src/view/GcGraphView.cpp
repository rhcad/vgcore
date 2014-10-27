//! \file GcGraphView.cpp
//! \brief 实现主绘图视图类 GcGraphView
// Copyright (c) 2012-2013, https://github.com/rhcad/touchvg

#include "GcGraphView.h"
#include "mglog.h"

// GcBaseView
//

GcBaseView::~GcBaseView()
{
    LOGD("View %p destroyed", this);
}

void GcBaseView::onSize(int dpi, int w, int h)
{
    xform()->setResolution((float)dpi);
    xform()->setWndSize(w, h);
}

bool GcBaseView::onGesture(const MgMotion& motion)
{
    if (motion.gestureType != kGiGesturePan || !_zoomEnabled){
        return false;
    }
    if (motion.gestureState <= kMgGestureBegan) {
        _lastScale = xform()->getZoomValue(_lastCenter);
    }
    else if (motion.gestureState == kMgGestureMoved) {
        Vector2d vec(motion.point - motion.startPt);
        xform()->zoom(_lastCenter, _lastScale);     // 先恢复
        xform()->zoomPan(vec.x, vec.y);             // 平移到当前点
        _zooming = true;
        cmdView()->regenAll(false);
    }
    else if (_zooming) {
        _zooming = false;
        cmdView()->regenAll(false);
    }
    
    return true;
}

bool GcBaseView::twoFingersMove(const MgMotion& motion)
{
    if (!_zoomEnabled) {
        return false;
    }
    if (motion.gestureState <= kMgGestureBegan) {
        _lastScale = xform()->getZoomValue(_lastCenter);
    }
    else if (motion.gestureState == kMgGestureMoved
        && motion.startPt != motion.startPt2
        && motion.point != motion.point2) {         // 双指变单指则忽略移动
        Point2d at((motion.startPt + motion.startPt2) / 2);
        Point2d pt((motion.point + motion.point2) / 2);
        float d1 = motion.point.distanceTo(motion.point2);
        float d0 = motion.startPt.distanceTo(motion.startPt2);
        float scale = d1 / d0;
        
        xform()->zoom(_lastCenter, _lastScale);     // 先恢复
        xform()->zoomByFactor(scale - 1, &at);      // 以起始点为中心放缩显示
        xform()->zoomPan(pt.x - at.x, pt.y - at.y); // 平移到当前点
        
        _zooming = true;
        cmdView()->regenAll(false);
    }
    else if (_zooming) {
        _zooming = false;
        cmdView()->regenAll(false);
    }
    
    return true;
}

void GcBaseView::dyndraw(GiGraphics& gs)
{
    GiContext ctx(0, GiColor(255, 0, 0, 24), GiContext::kDashLine);
    gs.drawRect(&ctx, gs.xf().getWorldLimits(), false);
}

// GcGraphView
//

GcGraphView::GcGraphView(MgView* mgview, GiView *view) : GcBaseView(mgview, view)
{
}

GcGraphView::~GcGraphView()
{
}
