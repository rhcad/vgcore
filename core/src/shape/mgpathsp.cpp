// mgpathsp.cpp: 实现路径图形 MgPathShape
// Copyright (c) 2004-2014, Zhang Yungui
// License: LGPL, https://github.com/touchvg/vgcore

#include "mgpathsp.h"
#include "mgshape_.h"

MG_IMPLEMENT_CREATE(MgPathShape)

MgPathShape::MgPathShape()
{
}

MgPathShape::~MgPathShape()
{
}

int MgPathShape::_getPointCount() const
{
    return _path.getCount();
}

Point2d MgPathShape::_getPoint(int index) const
{
    return _path.getPoint(index);
}

void MgPathShape::_setPoint(int index, const Point2d& pt)
{
    _path.setPoint(index, pt);
}

void MgPathShape::_copy(const MgPathShape& src)
{
    _path.copy(src._path);
    __super::_copy(src);
}

bool MgPathShape::_equals(const MgPathShape& src) const
{
    if (_path.getCount() != src._path.getCount() || !__super::_equals(src)) {
        return false;
    }
    for (int i = 0; i < _path.getCount(); i++) {
        if (_path.getPoint(i) != src._path.getPoint(i) ||
            _path.getNodeType(i) != src._path.getNodeType(i)) {
            return false;
        }
    }
    return true;
}

void MgPathShape::_update()
{
    _extent.set(_path.getCount(), _path.getPoints());
    __super::_update();
}

void MgPathShape::_transform(const Matrix2d& mat)
{
    for (int i = 0; i < _path.getCount(); i++) {
        _path.setPoint(i, _path.getPoint(i) * mat);
    }
    __super::_transform(mat);
}

void MgPathShape::_clear()
{
    _path.clear();
    __super::_clear();
}

bool MgPathShape::_isClosed() const
{
    return !!(_path.getNodeType(_path.getCount() - 1) & kGiCloseFigure);
}

bool MgPathShape::isCurve() const
{
    for (int i = 0; i < _path.getCount(); i++) {
        if (_path.getNodeType(i) & (kGiBezierTo | kGiQuadTo))
            return true;
    }
    return false;
}

float MgPathShape::_hitTest(const Point2d& pt, float tol, MgHitResult& res) const
{
    int n = _path.getCount();
    const Point2d* pts = _path.getPoints();
    const char* types = _path.getTypes();
    Point2d pos, ends, bz[7], nearpt;
    bool err = false;
    const Box2d rect (pt, 2 * tol, 2 * tol);
    
    res.dist = _FLT_MAX - tol;
    for (int i = 0; i < n && !err; i++) {
        float dist = _FLT_MAX;
        pos = ends;
        
        switch (types[i] & ~kGiCloseFigure) {
            case kGiMoveTo:
                ends = pts[i];
                break;
                
            case kGiLineTo:
                ends = pts[i];
                if (rect.isIntersect(Box2d(pos, ends))) {
                    dist = mglnrel::ptToLine(pos, ends, pt, nearpt);
                }
                break;
                
            case kGiBezierTo:
                if (i + 2 >= n) {
                    err = true;
                    break;
                }
                bz[0] = pos;
                bz[1] = pts[i];
                bz[2] = pts[i+1];
                bz[3] = pts[i+2];
                ends = bz[3];
                
                if (rect.isIntersect(mgnear::bezierBox1(bz))) {
                    dist = mgnear::nearestOnBezier(pt, bz, nearpt);
                }
                i += 2;
                break;
                
            case kGiQuadTo:
                if (i + 1 >= n) {
                    err = true;
                    break;
                }
                bz[0] = pos;
                bz[1] = pts[i];
                bz[2] = pts[i+1];
                ends = bz[2];
                
                mgcurv::quadBezierToCubic(bz, bz + 3);
                if (rect.isIntersect(mgnear::bezierBox1(bz + 3))) {
                    dist = mgnear::nearestOnBezier(pt, bz, nearpt);
                }
                i++;
                break;
                
            default:
                err = true;
                break;
        }
        if (res.dist > dist) {
            res.dist = dist;
            res.segment = i;
        }
    }
    
    return res.dist;
}

bool MgPathShape::_hitTestBox(const Box2d& rect) const
{
    if (!__super::_hitTestBox(rect))
        return false;
    
    int n = _path.getCount();
    const Point2d* pts = _path.getPoints();
    const char* types = _path.getTypes();
    Point2d pos, ends, bz[7];
    int ret = 0;
    
    for (int i = 0; i < n && !ret; i++) {
        pos = ends;
        
        switch (types[i] & ~kGiCloseFigure) {
            case kGiMoveTo:
                ends = pts[i];
                break;
                
            case kGiLineTo:
                ends = pts[i];
                ret = rect.isIntersect(Box2d(pos, ends));
                break;
                
            case kGiBezierTo:
                if (i + 2 >= n) {
                    ret |= 2;
                    break;
                }
                bz[0] = pos;
                bz[1] = pts[i];
                bz[2] = pts[i+1];
                bz[3] = pts[i+2];
                ends = bz[3];
                ret |= rect.isIntersect(mgnear::bezierBox1(bz)) ? 1 : 0;
                i += 2;
                break;
                
            case kGiQuadTo:
                if (i + 1 >= n) {
                    ret |= 2;
                    break;
                }
                bz[0] = pos;
                bz[1] = pts[i];
                bz[2] = pts[i+1];
                ends = bz[2];
                mgcurv::quadBezierToCubic(bz, bz + 3);
                ret |= rect.isIntersect(mgnear::bezierBox1(bz + 3)) ? 1 : 0;
                i++;
                break;
                
            default:
                ret |= 2;
                break;
        }
    }
    
    return ret == 1;
}

bool MgPathShape::_draw(int mode, GiGraphics& gs, const GiContext& ctx, int segment) const
{
    bool ret = gs.drawPath(&ctx, _path, mode == 0);
    return __super::_draw(mode, gs, ctx, segment) || ret;
}

bool MgPathShape::_save(MgStorage* s) const
{
    bool ret = __super::_save(s);
    
    s->writeInt("count", _path.getCount());
    s->writeFloatArray("points", &(_path.getPoints()[0].x), 2 * _path.getCount());
    
    int *types = new int[1 + _path.getCount()];
    for (int i = 0; i < _path.getCount(); i++)
        types[i] = _path.getNodeType(i);
    s->writeIntArray("types", types, _path.getCount());
    delete[] types;
    
    return ret;
}

bool MgPathShape::_load(MgShapeFactory* factory, MgStorage* s)
{
    bool ret = __super::_load(factory, s);
    int n = mgMin(s->readInt("count", _path.getCount()), 10000);
    Point2d *pts = new Point2d[n + 1];
    int *types = new int[n + 1];
    
    ret = (ret && s->readFloatArray("points", &pts[0].x, 2 * n) == 2 * n
           && s->readIntArray("types", types, n) == n);
    if (ret) {
        _path.setPath(n, pts, types);
    }
    
    delete[] pts;
    delete[] types;
    
    return ret;
}
