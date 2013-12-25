// Copyright (c) 2013, https://github.com/rhcad/touchvg

#include "girecordcanvas.h"
#include "mgstorage.h"
#include <sstream>
#include <vector>

MgObject* MgRecordShape::clone() const
{
    MgRecordShape* p = new MgRecordShape();
    p->copy(*this);
    return p;
}

void MgRecordShape::copy(const MgObject& src)
{
    if (src.isKindOf(Type())) {
        items = ((MgRecordShape&)src).items;
    }
    MgBaseShape::copy(src);
}

bool MgRecordShape::equals(const MgObject& src) const
{
    if (src.isKindOf(Type())) {
        if (items != ((MgRecordShape&)src).items)
            return false;
    }
    return MgBaseShape::equals(src);
}

void MgRecordShape::clear()
{
    _clear();
    items.clear();
}

bool MgRecordShape::save(MgStorage* s) const
{
    int i = 0;
    for (ITEMS::const_iterator it = items.begin(); it != items.end(); ++it) {
        std::stringstream ss;
        ss << "p" << i++;
        s->writeString(ss.str().c_str(), it->c_str());
    }
    return _save(s);
}

bool MgRecordShape::load(MgShapeFactory* factory, MgStorage* s)
{
    char item[100];
    
    items.clear();
    
    for (int i = 0; ; i++) {
        std::stringstream ss;
        ss << "p" << i++;
        if (s->readString(ss.str().c_str(), item, sizeof(item)) < 1)
            break;
        items.push_back(item);
    }
    return _load(factory, s);
}

bool MgRecordShape::draw(int mode, GiGraphics& gs, const GiContext& ctx, int segment) const
{
    int n = 0;
    std::string line;
    
    for (ITEMS::const_iterator it = items.begin(); it != items.end(); ++it) {
        std::stringstream ss(*it);
        std::vector<std::string> tokens;
        
        while (std::getline(ss, line, ',')) {
            tokens.push_back(line);
        }
    }
    return _draw(mode, gs, ctx, segment) || n > 0;
}

// GiRecordCanvas
//

void GiRecordCanvas::setPen(int argb, float width, int style, float phase)
{
    std::stringstream ss;
    ss << "setPen," << argb << "," << width << "," << style << "," << phase;
    shape.items.push_back(ss.str());
}

void GiRecordCanvas::setBrush(int argb, int style)
{
    std::stringstream ss;
    ss << "setBrush," << argb << "," << style;
    shape.items.push_back(ss.str());
}

void GiRecordCanvas::clearRect(float x, float y, float w, float h)
{
    std::stringstream ss;
    ss << "clearRect," << x << "," << y << "," << w << "," << h;
    shape.items.push_back(ss.str());
}

void GiRecordCanvas::drawRect(float x, float y, float w, float h, bool stroke, bool fill)
{
    std::stringstream ss;
    ss << "drawRect," << x << "," << y << "," << w << "," << h << "," << stroke << "," << fill;
    shape.items.push_back(ss.str());
}

void GiRecordCanvas::drawLine(float x1, float y1, float x2, float y2)
{
    std::stringstream ss;
    ss << "drawLine," << x1 << "," << y1 << "," << x2 << "," << y2;
    shape.items.push_back(ss.str());
}

void GiRecordCanvas::drawEllipse(float x, float y, float w, float h, bool stroke, bool fill)
{
    std::stringstream ss;
    ss << "drawEllipse," << x << "," << y << "," << w << "," << h << "," << stroke << "," << fill;
    shape.items.push_back(ss.str());
}

void GiRecordCanvas::beginPath()
{
    std::stringstream ss;
    ss << "beginPath";
    shape.items.push_back(ss.str());
}

void GiRecordCanvas::moveTo(float x, float y)
{
    std::stringstream ss;
    ss << "moveTo," << x << "," << y;
    shape.items.push_back(ss.str());
}

void GiRecordCanvas::lineTo(float x, float y)
{
    std::stringstream ss;
    ss << "lineTo," << x << "," << y;
    shape.items.push_back(ss.str());
}

void GiRecordCanvas::bezierTo(float c1x, float c1y, float c2x, float c2y, float x, float y)
{
    std::stringstream ss;
    ss << "bezierTo," << c1x << "," << c1y << "," << c2x << "," << c2y << "," << x << "," << y;
    shape.items.push_back(ss.str());
}

void GiRecordCanvas::quadTo(float cpx, float cpy, float x, float y)
{
    std::stringstream ss;
    ss << "quadTo," << cpx << "," << cpy << "," << x << "," << y;
    shape.items.push_back(ss.str());
}

void GiRecordCanvas::closePath()
{
    std::stringstream ss;
    ss << "closePath";
    shape.items.push_back(ss.str());
}

void GiRecordCanvas::drawPath(bool stroke, bool fill)
{
    std::stringstream ss;
    ss << "drawPath," << stroke << "," << fill;
    shape.items.push_back(ss.str());
}

void GiRecordCanvas::saveClip()
{
}

void GiRecordCanvas::restoreClip()
{
}

bool GiRecordCanvas::clipRect(float, float, float, float)
{
    return false;
}

bool GiRecordCanvas::clipPath()
{
    return false;
}

void GiRecordCanvas::drawHandle(float x, float y, int type)
{
    std::stringstream ss;
    ss << "drawHandle," << x << "," << y << "," << type;
    shape.items.push_back(ss.str());
}

void GiRecordCanvas::drawBitmap(const char* name, float xc, float yc,
                                float w, float h, float angle)
{
    std::stringstream ss;
    ss << "drawBitmap," << name << "," << xc << "," << yc << "," << w << "," << h << "," << angle;
    shape.items.push_back(ss.str());
}

float GiRecordCanvas::drawTextAt(const char* text, float x, float y, float h, int align)
{
    std::stringstream ss;
    ss << "drawTextAt," << text << "," << x << "," << y << "," << h << "," << align;
    shape.items.push_back(ss.str());
    return h;
}
