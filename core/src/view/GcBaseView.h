//! \file GcBaseView.h
//! \brief 定义内核视图基类 GcBaseView
// Copyright (c) 2012-2013, https://github.com/rhcad/touchvg

#ifndef TOUCHVG_CORE_BASEVIEW_H
#define TOUCHVG_CORE_BASEVIEW_H

#include "gigesture.h"
#include "mgcmd.h"
#include "mgshapedoc.h"

class GiView;

//! 内核视图基类
/*! \ingroup CORE_VIEW
 */
class GcBaseView
{
public:
    GcBaseView(MgView* mgview, GiView *view);
    virtual ~GcBaseView();
    
    GiView* deviceView() { return _view; }                          //!< 返回回调视图对象
    
    MgView* cmdView() { return _mgview; }
    MgShapeDoc* frontDoc();
    MgShapeDoc* backDoc();
    MgShapes* backShapes();
    
    GiGraphics* createFrontGraph();                                 //!< 克隆前端图形显示对象
    void releaseFrontGraph(GiGraphics* gs);                         //!< 释放前端图形显示对象
    bool isDrawing();                                               //!< 返回是否正在绘制静态图形
    int stopDrawing();                                              //!< 标记需要停止绘图
    
    //! 应用后端坐标系对象到前端
    void submitBackXform() { _xfFront = _xfBack; _gsFront.copy(_gsBack); }
    
    //! 得到前端图形显示对象
    GiGraphics* frontGraph() { return &_gsFront; }
    
    //! 得到后端坐标系对象
    GiTransform* xform() { return &_xfBack; }
    
    //! 得到后端图形显示对象
    GiGraphics* graph() { return &_gsBack; }

    //! 设置视图的宽高
    virtual void onSize(int dpi, int w, int h);
    
    //! 传递单指触摸手势消息
    virtual bool onGesture(const MgMotion& motion);

    //! 传递双指移动手势(可放缩旋转)
    virtual bool twoFingersMove(const MgMotion& motion);

private:
    MgView*     _mgview;
    GiView*     _view;
    GiTransform _xfFront;
    GiTransform _xfBack;
    GiGraphics  _gsFront;
    GiGraphics  _gsBack;
    GiGraphics* _gsBuf[10];
    volatile long _gsUsed[10];

    Point2d     _lastCenter;
    float       _lastScale;
};

#endif // TOUCHVG_CORE_BASEVIEW_H
