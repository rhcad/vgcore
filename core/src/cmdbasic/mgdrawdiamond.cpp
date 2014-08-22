// mgdrawdiamond.cpp
// Copyright (c) 2004-2013, Zhang Yungui
// License: LGPL, https://github.com/rhcad/touchvg

#include "mgdrawdiamond.h"
#include "mgshapet.h"
#include "mgbasicsps.h"

bool MgCmdDrawDiamond::initialize(const MgMotion* sender, MgStorage*)
{
    return _initialize(MgDiamond::Type(), sender);
}
