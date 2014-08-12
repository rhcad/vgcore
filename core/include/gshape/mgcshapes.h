//! \file mgcshapes.h
//! \brief 定义基本图形的工厂类 MgCoreShapeFactory
// Copyright (c) 2004-2014, Zhang Yungui
// License: LGPL, https://github.com/rhcad/touchvg

#ifndef TOUCHVG_CORESHAPE_FACTORY_H_
#define TOUCHVG_CORESHAPE_FACTORY_H_

class MgBaseShape;

//! 基本图形的工厂类
/*! \ingroup CORE_SHAPE
*/
class MgCoreShapeFactory
{
public:
    //! 根据指定的图形类型创建图形对象
    static MgBaseShape* createShape(int type);
};

#endif // TOUCHVG_CORESHAPE_FACTORY_H_
