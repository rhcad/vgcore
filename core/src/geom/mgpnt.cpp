// mgpnt.cpp
// Copyright (c) 2004-2013, Zhang Yungui
// License: LGPL, https://github.com/rhcad/touchvg

#include "mgpnt.h"

Point2d Point2d::rulerPoint(const Point2d& dir, float yoff) const
{
    float len = distanceTo(dir);
    if (len < _MGZERO) {
        return Point2d(x, y + yoff);
    }
    yoff /= len;
    return Point2d(x - (dir.y - y) * yoff, y + (dir.x - x) * yoff);
}

Point2d Point2d::rulerPoint(const Point2d& dir, float xoff, float yoff) const
{
    float len = distanceTo(dir);
    if (len < _MGZERO)
        return Point2d(x + xoff, y + yoff);
    float dcos = (dir.x - x) / len;
    float dsin = (dir.y - y) / len;
    return Point2d(x + xoff * dcos - yoff * dsin,
                   y + xoff * dsin + yoff * dcos);
}
