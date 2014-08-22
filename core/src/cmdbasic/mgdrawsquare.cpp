// mgdrawsquare.cpp: 实现正方形绘图命令类 MgCmdDrawSquare
// Copyright (c) 2004-2013, Zhang Yungui
// License: LGPL, https://github.com/rhcad/touchvg

#include "mgdrawsquare.h"
#include "mgbasicsps.h"

bool MgCmdDrawSquare::initialize(const MgMotion* sender, MgStorage* s)
{
    bool ret = _initialize(MgRect::Type(), sender, s);
    
    MgBaseRect* rect = (MgBaseRect*)dynshape()->shape();
    rect->setSquare(true);
    
    return ret;
}
