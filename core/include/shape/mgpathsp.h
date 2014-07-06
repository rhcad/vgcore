//! \file mgpathsp.h
//! \brief 定义路径图形 MgPathShape
// Copyright (c) 2004-2014, Zhang Yungui
// License: LGPL, https://github.com/touchvg/vgcore

#ifndef TOUCHVG_PATH_SHAPE_H_
#define TOUCHVG_PATH_SHAPE_H_

#include "mgbasicsp.h"

//! 路径图形类
/*! \ingroup CORE_SHAPE
*/
class MgPathShape : public MgBaseShape
{
    MG_DECLARE_CREATE(MgPathShape, MgBaseShape, 32)
public:
    const GiPath& pathc() const { return _path; }
    GiPath& path() { return _path; }
    
    virtual bool isCurve() const;
    
protected:
    bool _isClosed() const;
    bool _hitTestBox(const Box2d& rect) const;
    bool _save(MgStorage* s) const;
    bool _load(MgShapeFactory* factory, MgStorage* s);
    
private:
    GiPath _path;
};

#endif // TOUCHVG_PATH_SHAPE_H_
