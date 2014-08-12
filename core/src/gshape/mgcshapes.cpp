// mgcshapes.cpp: 实现基本图形的工厂类 MgCoreShapeFactory
// Copyright (c) 2004-2014, Zhang Yungui
// License: LGPL, https://github.com/rhcad/touchvg

#include "mgcshapes.h"
#include "mgshapetype.h"
#include "mgarc.h"
#include "mgdiamond.h"
#include "mgdot.h"
#include "mgellipse.h"
#include "mggrid.h"
#include "mgline.h"
#include "mglines.h"
#include "mgparallel.h"
#include "mgpathsp.h"
#include "mgrdrect.h"
#include "mgrect.h"
#include "mgsplines.h"

MgBaseShape* MgCoreShapeFactory::createShape(int type)
{
    switch (type) {
        case kMgShapeGrid:
            return MgGrid::create();
        case kMgShapeDot:
            return MgDot::create();
        case kMgShapeLine:
            return MgLine::create();
        case kMgShapeRect:
            return MgRect::create();
        case kMgShapeEllipse:
            return MgEllipse::create();
        case kMgShapeRoundRect:
            return MgRoundRect::create();
        case kMgShapeDiamond:
            return MgDiamond::create();
        case kMgShapeParallel:
            return MgParallel::create();
        case kMgShapeLines:
            return MgLines::create();
        case kMgShapeSplines:
            return MgSplines::create();
        case kMgShapeArc:
            return MgArc::create();
        case kMgShapePath:
            return MgPathShape::create();
        default:
            return (MgBaseShape*)0;
    }
}
