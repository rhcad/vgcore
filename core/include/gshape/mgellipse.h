//! \file mgellipse.h
//! \brief 定义椭圆图形类 MgEllipse
// Copyright (c) 2004-2014, Zhang Yungui
// License: LGPL, https://github.com/touchvg/vgcore

#ifndef TOUCHVG_ELLIPSE_SHAPE_H_
#define TOUCHVG_ELLIPSE_SHAPE_H_

#include "mgrect.h"

//! 椭圆图形类
/*! \ingroup CORE_SHAPE
 */
class MgEllipse : public MgBaseRect
{
    MG_INHERIT_CREATE(MgEllipse, MgBaseRect, 12)
public:
    //! 返回X半轴长度
    float getRadiusX() const;
    
    //! 返回Y半轴长度
    float getRadiusY() const;
    
    //! 设置半轴长度
    void setRadius(float rx, float ry = 0.0);
    
#ifndef SWIG
    //! 返回Bezier顶点，13个点
    const Point2d* getBeziers() const { return _bzpts; }
    
    virtual bool isCurve() const { return true; }
#endif

protected:
    void _update();
    void _transform(const Matrix2d& mat);
    int _getHandleCount() const;
    Point2d _getHandlePoint(int index) const;
    int _getHandleType(int index) const;
    bool _setHandlePoint(int index, const Point2d& pt, float tol);
    float _hitTest(const Point2d& pt, float tol, MgHitResult& res) const;
    bool _hitTestBox(const Box2d& rect) const;
    void _output(MgPath& path) const;
    
protected:
    Point2d     _bzpts[13];
};

#endif // TOUCHVG_ELLIPSE_SHAPE_H_
