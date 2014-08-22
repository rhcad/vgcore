// mgdrawparallel.cpp: 实现平行四边形绘图命令类 MgCmdParallel
// License: LGPL, https://github.com/rhcad/touchvg

#include "mgdrawparallel.h"
#include "mgbasicsps.h"

bool MgCmdParallel::initialize(const MgMotion* sender, MgStorage* s)
{
    return _initialize(MgParallel::Type(), sender, s);
}
