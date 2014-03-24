// mgshape.cpp
// Copyright (c) 2004-2013, Zhang Yungui
// License: LGPL, https://github.com/rhcad/touchvg

#include "mgshape.h"
#include "mgstorage.h"
#include "mglog.h"

//static volatile long _n = 0;

MgBaseShape::MgBaseShape() : _flags(0), _changeCount(0) {
    //LOGD("+MgBaseShape %ld", giAtomicIncrement(&_n));
}
MgBaseShape::~MgBaseShape() {
    //LOGD("-MgBaseShape %ld", giAtomicDecrement(&_n));
}

void MgBaseShape::copy(const MgObject& src) {
    if (src.isKindOf(Type()))
        _copy((const MgBaseShape&)src);
}
bool MgBaseShape::equals(const MgObject& src) const {
    return src.isKindOf(Type()) && _equals((const MgBaseShape&)src);
}
bool MgBaseShape::isKindOf(int type) const {
    return type == Type();
}
Box2d MgBaseShape::getExtent() const { return _getExtent(); }
long MgBaseShape::getChangeCount() const { return _changeCount; }
void MgBaseShape::resetChangeCount(long count) { update(); _changeCount = count; }
void MgBaseShape::afterChanged() { _changeCount++; }
void MgBaseShape::update() { _update(); }
void MgBaseShape::transform(const Matrix2d& mat) { _transform(mat); }
void MgBaseShape::clear() { _clear(); }
void MgBaseShape::clearCachedData() { _clearCachedData(); }

bool MgBaseShape::isClosed() const {
    return _isClosed();
}
bool MgBaseShape::hitTestBox(const Box2d& rect) const {
    return _hitTestBox(rect);
}
bool MgBaseShape::draw(int mode, GiGraphics& gs, 
                       const GiContext& ctx, int segment) const {
    return _draw(mode, gs, ctx, segment);
}
bool MgBaseShape::save(MgStorage* s) const {
    return _save(s);
}
bool MgBaseShape::load(MgShapeFactory* factory, MgStorage* s) {
    return _load(factory, s);
}
int MgBaseShape::getHandleCount() const {
    return _getHandleCount();
}
Point2d MgBaseShape::getHandlePoint(int index) const {
    return _getHandlePoint(index);
}
bool MgBaseShape::setHandlePoint(int index, const Point2d& pt, float tol) {
    return _rotateHandlePoint(index, pt) || _setHandlePoint(index, pt, tol);
}
bool MgBaseShape::isHandleFixed(int index) const {
    return _isHandleFixed(index);
}
int MgBaseShape::getHandleType(int index) const {
    return _getHandleType(index);
}
bool MgBaseShape::offset(const Vector2d& vec, int segment) {
    return _offset(vec, segment);
}

void MgBaseShape::_copy(const MgBaseShape& src)
{
    _extent = src._extent;
    _flags = src._flags;
    _changeCount = src._changeCount;
}

bool MgBaseShape::_equals(const MgBaseShape& src) const
{
    return _flags == src._flags;
}

Box2d MgBaseShape::_getExtent() const
{
    if (_extent.isNull() || !_extent.isEmpty(minTol())) {
        return _extent;
    }
    
    Box2d rect(_extent);
    
    if (rect.width() < minTol().equalPoint() && getPointCount() > 0) {
        rect.inflate(minTol().equalPoint() / 2.f, 0);
    }
    if (rect.height() < minTol().equalPoint() && getPointCount() > 0) {
        rect.inflate(0, minTol().equalPoint() / 2.f);
    }
    
    return rect;
}

void MgBaseShape::_update()
{
    _extent = _getExtent();
    afterChanged();
}

void MgBaseShape::_transform(const Matrix2d& mat)
{
    _extent *= mat;
}

void MgBaseShape::_clear()
{
    _extent.empty();
}

bool MgBaseShape::_draw(int, GiGraphics&, const GiContext&, int) const
{
    return false;
}

int MgBaseShape::_getHandleCount() const
{
    return getPointCount();
}

Point2d MgBaseShape::_getHandlePoint(int index) const
{
    return getPoint(index);
}

bool MgBaseShape::_rotateHandlePoint(int index, const Point2d& pt)
{
    if (getFlag(kMgFixedLength)) {
        if (getFlag(kMgRotateDisnable)) {
            offset(pt - getHandlePoint(index), -1);
        }
        else {
            Point2d basept(_extent.center());
            if (!getFlag(kMgSquare)) {
                int baseindex = index > 0 ? index - 1 : getHandleCount() - 1;
                while (baseindex > 0 && isHandleFixed(baseindex))
                    baseindex--;
                basept = (getHandlePoint(baseindex));
            }
            float a1 = (pt - basept).angle2();
            float a2 = (getHandlePoint(index) - basept).angle2();
            
            transform(Matrix2d::rotation(a1 - a2, basept));
        }
        return true;
    }

    return false;
}

bool MgBaseShape::_setHandlePoint(int index, const Point2d& pt, float)
{
    setPoint(index, pt);
    update();
    return true;
}

bool MgBaseShape::_setHandlePoint2(int index, const Point2d& pt, float tol, int&)
{
    return setHandlePoint(index, pt, tol);
}

bool MgBaseShape::_offset(const Vector2d& vec, int)
{
    transform(Matrix2d::translation(vec));
    return true;
}

bool MgBaseShape::_hitTestBox(const Box2d& rect) const
{
    return getExtent().isIntersect(rect);
}

bool MgBaseShape::_save(MgStorage* s) const
{
    s->writeUInt("flags", _flags);
    return true;
}

bool MgBaseShape::_load(MgShapeFactory*, MgStorage* s)
{
    _flags = s->readInt("flags", _flags);
    return true;
}

bool MgBaseShape::getFlag(MgShapeBit bit) const
{
    return (_flags & (1 << bit)) != 0;
}

void MgBaseShape::setFlag(MgShapeBit bit, bool on)
{
    long value = on ? _flags | (1 << bit) : _flags & ~(1 << bit);
    if (_flags != value) {
        giAtomicCompareAndSwap((volatile long *)&_flags, value, _flags);
    }
}

// MgShape
//

bool MgShape::hasFillColor() const
{
    return context().hasFillColor() && shapec()->isClosed();
}

bool MgShape::draw(int mode, GiGraphics& gs, const GiContext *ctx, int segment) const
{
    GiContext tmpctx(context());

    if (shapec()->isKindOf(6)) { // MgComposite
        tmpctx = ctx ? *ctx : GiContext(0, GiColor(), GiContext::kNullLine);
    }
    else {
        if (ctx) {
            float addw = ctx->getLineWidth();

            if (addw < -0.1f) {
                tmpctx.setExtraWidth(-addw);
            } else if (addw > 0.1f) {                               // 传入正数表示像素宽度
                tmpctx.setLineWidth(-addw, ctx->isAutoScale());     // 换成新的像素宽度
            }
        }

        if (ctx && ctx->getLineColor().a > 0) {
            tmpctx.setLineColor(ctx->getLineColor());
        }
        if (ctx && !ctx->isNullLine()) {
            tmpctx.setLineStyle(ctx->getLineStyle());
        }
        if (ctx && ctx->hasFillColor()) {
            tmpctx.setFillColor(ctx->getFillColor());
        }
    }

    bool ret = false;
    Box2d rect(shapec()->getExtent() * gs.xf().modelToDisplay());

    rect.inflate(1 + gs.calcPenWidth(tmpctx.getLineWidth(), tmpctx.isAutoScale()) / 2);

    if (gs.beginShape(shapec()->getType(), getID(),
                      (int)shapec()->getChangeCount(),
                      rect.xmin, rect.ymin, rect.width(), rect.height())) {
        ret = shapec()->draw(mode, gs, tmpctx, segment);
        gs.endShape(shapec()->getType(), getID(), rect.xmin, rect.ymin);
    }
    return ret;
}

void MgShape::copy(const MgObject& src)
{
    if (src.isKindOf(Type())) {
        const MgShape& _src = (const MgShape&)src;
        shape()->copy(*_src.shapec());
        setContext(_src.context(), -1);
        setTag(_src.getTag());
        if (!getParent() && 0 == getID()) {
            setParent(_src.getParent(), _src.getID());
        }
    }
    else if (src.isKindOf(MgBaseShape::Type())) {
        shape()->copy(src);
    }
    shape()->update();
}

bool MgShape::isKindOf(int type) const
{
    return type == Type() || type == shapec()->getType();
}

bool MgShape::equals(const MgObject& src) const
{
    bool ret = false;

    if (src.isKindOf(Type())) {
        const MgShape& _src = (const MgShape&)src;
        ret = shapec()->equals(*_src.shapec())
            && context().equals(_src.context())
            && getTag() == _src.getTag();
    }

    return ret;
}

bool MgShape::save(MgStorage* s) const
{
    GiColor c;

    s->writeInt("tag", getTag());
    s->writeInt("lineStyle", (unsigned char)context().getLineStyle());
    s->writeFloat("lineWidth", context().getLineWidth());

    c = context().getLineColor();
    s->writeUInt("lineColor", c.b | (c.g << 8) | (c.r << 16) | (c.a << 24));
    c = context().getFillColor();
    s->writeUInt("fillColor", c.b | (c.g << 8) | (c.r << 16) | (c.a << 24));

    return shapec()->save(s);
}

bool MgShape::load(MgShapeFactory* factory, MgStorage* s)
{
    setTag(s->readInt("tag", getTag()));

    GiContext ctx;
    ctx.setLineStyle(s->readInt("lineStyle", 0));
    ctx.setLineWidth(s->readFloat("lineWidth", 0), true);
    ctx.setLineColor(GiColor(s->readInt("lineColor", 0xFF000000), true));
    ctx.setFillColor(GiColor(s->readInt("fillColor", 0), true));
    setContext(ctx, -1);

    bool ret = shape()->load(factory, s);
    if (ret) {
        shape()->update();
    }

    return ret;
}
