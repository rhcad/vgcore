//! \file mgsnap.h
//! \brief 定义图形特征点捕捉器接口 MgSnap
// Copyright (c) 2004-2013, Zhang Yungui
// License: LGPL, https://github.com/rhcad/touchvg

#ifndef TOUCHVG_MGSNAP_H_
#define TOUCHVG_MGSNAP_H_

class MgMotion;
class Point2d;
class GiGraphics;
class MgShape;

//! 特征点类型
typedef enum {
    kMgSnapNone,        //!< 无
    kMgSnapSameX,       //!< X方向参考点
    kMgSnapSameY,       //!< Y方向参考点
    kMgSnapGridX,       //!< X方向网格线
    kMgSnapGridY,       //!< Y方向网格线
    kMgSnapGrid,        //!< 网格点
    kMgSnapPoint,       //!< 顶点
    kMgSnapCenter,      //!< 圆心
    kMgSnapMidPoint,    //!< 中点
    kMgSnapQuadrant,    //!< 象限点
    kMgSnapOutPoint,    //!< 线外点
    kMgSnapReserved,
    kMgSnapIntersect,   //!< 交点
    kMgSnapPerp,        //!< 垂足
    kMgSnapPerpNear,    //!< 一端为垂足，另一端为线上最近点
    kMgSnapNearPt,      //!< 线上最近点
} MgSnapType;

//! 图形特征点捕捉器接口
/*! \ingroup CORE_COMMAND
    \interface MgSnap
*/
struct MgSnap {
    //! 清除捕捉结果
    virtual void clearSnap(const MgMotion* sender) = 0;
    
    //! 显示捕捉提示线
    virtual bool drawSnap(const MgMotion* sender, GiGraphics* gs) const = 0;
    
    //! 返回捕捉到的特征点类型, >=kMgSnapGrid
    virtual int getSnappedType() const = 0;
    
    //! 得到捕捉到的特征点坐标和原始参考坐标、捕捉坐标
    virtual int getSnappedPoint(Point2d& fromPt, Point2d& toPt) const = 0;
    
    //! 根据当前点捕捉新的坐标
    virtual Point2d snapPoint(const MgMotion* sender, const Point2d& orignPt) {
        return snapPoint(sender, orignPt, NULL, -1);
    }
    
#ifndef SWIG
    //! 根据当前点捕捉新的坐标
    virtual Point2d snapPoint(const MgMotion* sender, 
                              const Point2d& orignPt, const MgShape* shape,
                              int hotHandle, int ignoreHandle = -1, const int* ignoreids = NULL) = 0;
    
    //! 得到捕捉到的图形、控制点序号、源图形上匹配的控制点序号
    virtual bool getSnappedHandle(int& shapeid, int& handleIndex, int& handleIndexSrc) const = 0;
#endif
};

#endif // TOUCHVG_MGSNAP_H_
