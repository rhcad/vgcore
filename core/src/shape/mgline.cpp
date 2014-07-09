// mgline.cpp
// Copyright (c) 2004-2013, Zhang Yungui
// License: LGPL, https://github.com/rhcad/touchvg

#include "mgbasicsp.h"
#include "mgshape_.h"

MG_IMPLEMENT_CREATE(MgDot)

MgDot::MgDot() : _type(0)
{
}

MgDot::~MgDot()
{
}

int MgDot::_getPointCount() const
{
    return 1;
}

Point2d MgDot::_getPoint(int) const
{
    return _point;
}

void MgDot::_setPoint(int, const Point2d& pt)
{
    _point = pt;
}

void MgDot::_copy(const MgDot& src)
{
    _point = src._point;
    _type = src._type;
    __super::_copy(src);
}

bool MgDot::_equals(const MgDot& src) const
{
    return (_point == src._point && _type == src._type && __super::_equals(src));
}

void MgDot::_update()
{
    _extent.set(_point, minTol().equalPoint(), 0.f);
    __super::_update();
}

void MgDot::_transform(const Matrix2d& mat)
{
    _point *= mat;
    __super::_transform(mat);
}

void MgDot::_clear()
{
    _point = Point2d::kOrigin();
    __super::_clear();
}

float MgDot::_hitTest(const Point2d& pt, float, MgHitResult& res) const
{
    res.nearpt = _point;
    return pt.distanceTo(_point);
}

bool MgDot::_draw(int mode, GiGraphics& gs, const GiContext& ctx, int segment) const
{
    bool ret = false;
    
    if (_type <= 0) {
        GiContext ctx2(0, GiColor::Invalid(), GiContext::kNullLine,
                       ctx.hasFillColor() ? ctx.getFillColor() : ctx.getLineColor());
        float w = gs.calcPenWidth(ctx.getLineWidth(), false);
        
        ret = gs.drawCircle(&ctx2, _point, gs.xf().displayToModel(w));
    } else {
        ret = gs.drawHandle(_point, _type - 1);
    }
    
    return __super::_draw(mode, gs, ctx, segment) || ret;
}

void MgDot::_output(GiPath&) const
{
}

bool MgDot::_save(MgStorage* s) const
{
    bool ret = __super::_save(s);
    s->writeInt("ptype", _type);
    s->writeFloat("x", _point.x);
    s->writeFloat("y", _point.y);
    return ret;
}

bool MgDot::_load(MgShapeFactory* factory, MgStorage* s)
{
    bool ret = __super::_load(factory, s);
    _type = s->readInt("ptype", _type);
    _point.set(s->readFloat("x", _point.x), s->readFloat("y", _point.y));
    return ret;
}

// MgLine
//

MG_IMPLEMENT_CREATE(MgLine)

MgLine::MgLine()
{
}

MgLine::~MgLine()
{
}

int MgLine::_getPointCount() const
{
    return 2;
}

Point2d MgLine::_getPoint(int index) const
{
    return _points[index ? 1 : 0];
}

void MgLine::_setPoint(int index, const Point2d& pt)
{
    _points[index ? 1 : 0] = pt;
}

int MgLine::_getHandleCount() const
{
    return 3;
}

Point2d MgLine::_getHandlePoint(int index) const
{
    return index < 2 ? _getPoint(index) : center();
}

bool MgLine::_setHandlePoint(int index, const Point2d& pt, float tol)
{
    return index < 2 && __super::_setHandlePoint(index, pt, tol);
}

bool MgLine::_isHandleFixed(int index) const
{
    return index >= 2;
}

int MgLine::_getHandleType(int index) const
{
    return index >= 2 ? kMgHandleMidPoint : __super::_getHandleType(index);
}

void MgLine::_copy(const MgLine& src)
{
    _points[0] = src._points[0];
    _points[1] = src._points[1];
    __super::_copy(src);
}

bool MgLine::_equals(const MgLine& src) const
{
    return (_points[0] == src._points[0]
            && _points[1] == src._points[1]
            && __super::_equals(src));
}

void MgLine::_update()
{
    _extent.set(_points[0], _points[1]);
    __super::_update();
}

void MgLine::_transform(const Matrix2d& mat)
{
    _points[0] *= mat;
    _points[1] *= mat;
    __super::_transform(mat);
}

void MgLine::_clear()
{
    _points[1] = _points[0];
    __super::_clear();
}

float MgLine::_hitTest(const Point2d& pt, float, MgHitResult& res) const
{
    return mglnrel::ptToLine(_points[0], _points[1], pt, res.nearpt);
}

bool MgLine::_hitTestBox(const Box2d& rect) const
{
    if (!__super::_hitTestBox(rect))
        return false;
    Point2d pts[2] = { _points[0], _points[1] };
    return mglnrel::clipLine(pts[0], pts[1], rect);
}

bool MgLine::_draw(int mode, GiGraphics& gs, const GiContext& ctx, int segment) const
{
    bool ret = gs.drawLine(&ctx, _points[0], _points[1]);
    return __super::_draw(mode, gs, ctx, segment) || ret;
}

void MgLine::_output(GiPath& path) const
{
    path.moveTo(_points[0]);
    path.lineTo(_points[1]);
}

bool MgLine::_save(MgStorage* s) const
{
    bool ret = __super::_save(s);
    s->writeFloatArray("points", &(_points[0].x), 4);
    return ret;
}

bool MgLine::_load(MgShapeFactory* factory, MgStorage* s)
{
    bool ret = __super::_load(factory, s);
    return s->readFloatArray("points", &(_points[0].x), 4) == 4 && ret;
}

// MgParallel
//

MG_IMPLEMENT_CREATE(MgParallel)

MgParallel::MgParallel()
{
}

MgParallel::~MgParallel()
{
}

int MgParallel::_getPointCount() const
{
    return 4;
}

Point2d MgParallel::_getPoint(int index) const
{
    return _points[index];
}

void MgParallel::_setPoint(int index, const Point2d& pt)
{
    _points[index] = pt;
}

void MgParallel::_copy(const MgParallel& src)
{
    for (int i = 0; i < 4; i++)
        _points[i] = src._points[i];
    __super::_copy(src);
}

bool MgParallel::_equals(const MgParallel& src) const
{
    for (int i = 0; i < 4; i++) {
        if (_points[i] != src._points[i])
            return false;
    }
    return __super::_equals(src);
}

void MgParallel::_update()
{
    _points[3] = _points[0] + _points[2] + (- _points[1]);
    _extent.set(4, _points);
    if (_extent.isEmpty())
        _extent.set(_points[0], 2 * Tol::gTol().equalPoint(), 0);
    __super::_update();
}

void MgParallel::_transform(const Matrix2d& mat)
{
    for (int i = 0; i < 4; i++)
        _points[i] *= mat;
    __super::_transform(mat);
}

void MgParallel::_clear()
{
    for (int i = 1; i < 4; i++)
        _points[i] = _points[0];
    __super::_clear();
}

int MgParallel::_getHandleCount() const
{
    return 8;
}

Point2d MgParallel::_getHandlePoint(int index) const
{
    return (index < 4 ? __super::_getHandlePoint(index) :
            (_points[index % 4] + _points[(index + 1) % 4]) / 2);
}

int MgParallel::_getHandleType(int index) const
{
    return index < 4 ? __super::_getHandleType(index) : kMgHandleMidPoint;
}

bool MgParallel::_isHandleFixed(int index) const
{
    return index >= 4;
}

bool MgParallel::_setHandlePoint(int index, const Point2d& pt, float)
{
    index = index % 4;
    if (getFlag(kMgFixedLength)) {
        Point2d& basept = _points[(index - 1) % 4];
        _points[index] = basept.rulerPoint(pt, _points[index].distanceTo(basept), 0);
    }
    else {
        _points[index] = pt;
    }
    _points[(index + 1) % 4] = (_points[index] + _points[(index + 2) % 4]
                                + (- _points[(index + 3) % 4]));
    update();
    return true;
}

bool MgParallel::_offset(const Vector2d& vec, int segment)
{
    if (segment < 0 || getFlag(kMgFixedSize))
        return __super::_offset(vec, segment);
    return _setHandlePoint(segment, _points[segment] + vec, 0);
}

bool MgParallel::_rotateHandlePoint(int index, const Point2d& pt)
{
    return getFlag(kMgFixedSize) && __super::_rotateHandlePoint(index, pt);
}

float MgParallel::_hitTest(const Point2d& pt, float tol, MgHitResult& res) const
{
    return mgnear::linesHit(4, _points, true, pt, tol, 
        res.nearpt, res.segment, &res.inside);
}

bool MgParallel::_hitTestBox(const Box2d& rect) const
{
    if (!__super::_hitTestBox(rect))
        return false;

    for (int i = 0; i < 3; i++) {
        if (Box2d(_points[i], _points[i + 1]).isIntersect(rect))
            return true;
    }

    return false;
}

bool MgParallel::_draw(int mode, GiGraphics& gs, const GiContext& ctx, int segment) const
{
    bool ret = gs.drawPolygon(&ctx, 4, _points);
    return __super::_draw(mode, gs, ctx, segment) || ret;
}

void MgParallel::_output(GiPath& path) const
{
    path.moveTo(_points[0]);
    path.linesTo(3, _points + 1);
    path.closeFigure();
}

bool MgParallel::_save(MgStorage* s) const
{
    bool ret = __super::_save(s);
    s->writeFloatArray("points", &(_points[0].x), 8);
    return ret;
}

bool MgParallel::_load(MgShapeFactory* factory, MgStorage* s)
{
    bool ret = __super::_load(factory, s);
    return s->readFloatArray("points", &(_points[0].x), 8) == 8 && ret;
}
