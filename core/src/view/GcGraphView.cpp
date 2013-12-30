//! \file GcGraphView.cpp
//! \brief 实现主绘图视图类 GcGraphView
// Copyright (c) 2012-2013, https://github.com/rhcad/touchvg

#include "GcGraphView.h"
#include "mglog.h"

// GcBaseView
//

GcBaseView::~GcBaseView()
{
    for (int i = 0; i < sizeof(_gsBuf)/sizeof(_gsBuf[0]); i++) {
        delete _gsBuf[i];
    }
}

GiGraphics* GcBaseView::createFrontGraph()
{
    GiGraphics* gs = (GiGraphics*)0;
    int i = sizeof(_gsBuf)/sizeof(_gsBuf[0]);
    
    while (--i >= 0) {
        if (!_gsUsed[i] && _gsBuf[i]) {
            if (giInterlockedIncrement(&_gsUsed[i]) == 1) {
                gs = _gsBuf[i];
                gs->copy(_gsFront);
                gs->_xf().copy(_xfFront);
                break;
            } else {
                giInterlockedDecrement(&_gsUsed[i]);
            }
        }
    }
    if (!gs) {
        gs = new GiGraphics(new GiTransform(_xfFront), true);
        gs->copy(_gsFront);
        for (i = 0; i < sizeof(_gsBuf)/sizeof(_gsBuf[0]); i++) {
            if (!_gsBuf[i]) {
                if (giInterlockedIncrement(&_gsUsed[i]) == 1) {
                    _gsBuf[i] = gs;
                    break;
                } else {
                    giInterlockedDecrement(&_gsUsed[i]);
                }
            }
        }
    }
    return gs;
}

void GcBaseView::releaseFrontGraph(GiGraphics* gs)
{
    for (int i = 0; i < sizeof(_gsBuf)/sizeof(_gsBuf[0]); i++) {
        if (_gsBuf[i] == gs) {
            giInterlockedDecrement(&_gsUsed[i]);
            return;
        }
    }
    delete gs;
}

bool GcBaseView::isDrawing()
{
    for (int i = 0; i < sizeof(_gsBuf)/sizeof(_gsBuf[0]); i++) {
        if (_gsUsed[i] && _gsBuf[i] && _gsBuf[i]->isDrawing())
            return true;
    }
    return false;
}

int GcBaseView::stopDrawing()
{
    int n = 0;
    for (int i = 0; i < sizeof(_gsBuf)/sizeof(_gsBuf[0]); i++) {
        if (_gsUsed[i] && _gsBuf[i] && _gsBuf[i]->isDrawing()) {
            _gsBuf[i]->stopDrawing();
            n++;
        }
    }
    return n;
}

void GcBaseView::onSize(int dpi, int w, int h)
{
    xform()->setResolution((float)dpi);
    xform()->setWndSize(w, h);
}

bool GcBaseView::onGesture(const MgMotion& motion)
{
    if (motion.gestureType != kGiGesturePan){
        return false;
    }
    if (motion.gestureState == kMgGestureBegan) {
        _lastScale = xform()->getZoomValue(_lastCenter);
    }
    if (motion.gestureState == kMgGestureMoved) {
        Vector2d vec(motion.point - motion.startPt);
        xform()->zoom(_lastCenter, _lastScale);     // 先恢复
        xform()->zoomPan(vec.x, vec.y);             // 平移到当前点
        
        cmdView()->regenAll(false);
    }
    
    return true;
}

bool GcBaseView::twoFingersMove(const MgMotion& motion)
{
    if (motion.gestureState == kMgGestureBegan) {
        _lastScale = xform()->getZoomValue(_lastCenter);
    }
    if (motion.gestureState == kMgGestureMoved
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
        
        cmdView()->regenAll(false);
    }
    
    return true;
}

// GcGraphView
//

GcGraphView::GcGraphView(MgView* mgview, GiView *view) : GcBaseView(mgview, view)
{
}

GcGraphView::~GcGraphView()
{
}
