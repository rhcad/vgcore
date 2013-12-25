//! \file girecordcanvas.h
//! \brief Define the canvas adapter class to record drawing: GiRecordCanvas.
// Copyright (c) 2013, https://github.com/rhcad/touchvg

#ifndef TOUCHVG_CORE_GIRECORDCANVAS_H
#define TOUCHVG_CORE_GIRECORDCANVAS_H

#include "gicanvas.h"
#include "mgshape.h"
#include <string>
#include <list>

//! The shape class to record drawing.
/*! \ingroup CORE_SHAPE
 */
class MgRecordShape : public MgBaseShape
{
public:
    MgRecordShape() {}
    
    typedef std::list<std::string>  ITEMS;
    ITEMS   items;
    
    static MgRecordShape* create() { return new MgRecordShape(); }
    static int Type() { return 30; }
    
    virtual MgObject* clone() const;
    virtual void copy(const MgObject& src);
    virtual void release() { delete this; }
    virtual bool equals(const MgObject& src) const;
    virtual int getType() const { return Type(); }
    virtual bool isKindOf(int type) const { return type == Type(); }
    
    virtual void clear();
    virtual bool draw(int mode, GiGraphics& gs, const GiContext& ctx, int segment) const;
    virtual bool save(MgStorage* s) const;
    virtual bool load(MgShapeFactory* factory, MgStorage* s);
    
    virtual bool isCurve() const { return true; }
    virtual int getPointCount() const { return 0; }
    virtual Point2d getPoint(int) const { return Point2d(); }
    virtual void setPoint(int, const Point2d&) {}
    virtual float hitTest(const Point2d&, float, MgHitResult&) const { return _FLT_MAX; }
};

//! The canvas adapter class to record drawing.
/*!
    \ingroup CORE_VIEW
 */
class GiRecordCanvas : public GiCanvas
{
public:
    GiRecordCanvas() {}
    
    MgRecordShape   shape;
    
private:
    virtual void setPen(int argb, float width, int style, float phase);
    virtual void setBrush(int argb, int style);
    virtual void clearRect(float x, float y, float w, float h);
    virtual void drawRect(float x, float y, float w, float h, bool stroke, bool fill);
    virtual void drawLine(float x1, float y1, float x2, float y2);
    virtual void drawEllipse(float x, float y, float w, float h, bool stroke, bool fill);
    virtual void beginPath();
    virtual void moveTo(float x, float y);
    virtual void lineTo(float x, float y);
    virtual void bezierTo(float c1x, float c1y, float c2x, float c2y, float x, float y);
    virtual void quadTo(float cpx, float cpy, float x, float y);
    virtual void closePath();
    virtual void drawPath(bool stroke, bool fill);
    virtual void saveClip();
    virtual void restoreClip();
    virtual bool clipRect(float x, float y, float w, float h);
    virtual bool clipPath();
    virtual void drawHandle(float x, float y, int type);
    virtual void drawBitmap(const char* name, float xc, float yc, 
                            float w, float h, float angle);
    virtual float drawTextAt(const char* text, float x, float y, float h, int align);
};

#endif // TOUCHVG_CORE_GIRECORDCANVAS_H
