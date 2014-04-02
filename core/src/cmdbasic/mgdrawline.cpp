// mgdrawlines.cpp
// License: LGPL, https://github.com/rhcad/touchvg

#include "mgdrawline.h"
#include "mgshapet.h"
#include "mgbasicsp.h"

bool MgCmdDrawLine::initialize(const MgMotion* sender, MgStorage*)
{
    return _initialize(MgShapeT<MgLine>::create, sender);
}

bool MgCmdDrawLine::backStep(const MgMotion* sender)
{
    return MgCommandDraw::backStep(sender);
}

bool MgCmdDrawLine::touchBegan(const MgMotion* sender)
{
    m_step = 1;

    Point2d pnt(snapPoint(sender, true));
    dynshape()->shape()->setPoint(0, pnt);
    dynshape()->shape()->setPoint(1, pnt);
    dynshape()->shape()->update();

    return MgCommandDraw::touchBegan(sender);
}

bool MgCmdDrawLine::touchMoved(const MgMotion* sender)
{
    dynshape()->shape()->setPoint(1, snapPoint(sender));
    dynshape()->shape()->update();

    return MgCommandDraw::touchMoved(sender);
}

bool MgCmdDrawLine::touchEnded(const MgMotion* sender)
{
    dynshape()->shape()->setPoint(1, snapPoint(sender));
    dynshape()->shape()->update();

    if ( ((MgLine*)dynshape()->shape())->length() > sender->displayMmToModel(2.f)) {
        addShape(sender);
    }
    m_step = 0;

    return MgCommandDraw::touchEnded(sender);
}

// MgCmdDrawDot
//

bool MgCmdDrawDot::initialize(const MgMotion* sender, MgStorage*)
{
    return _initialize(MgShapeT<MgDot>::create, sender);
}

bool MgCmdDrawDot::click(const MgMotion* sender)
{
    return touchBegan(sender) && touchEnded(sender);
}

bool MgCmdDrawDot::touchBegan(const MgMotion* sender)
{
    m_step = 1;
    dynshape()->shape()->setPoint(0, snapPoint(sender, true));
    dynshape()->shape()->update();
    
    return MgCommandDraw::touchBegan(sender);
}

bool MgCmdDrawDot::touchMoved(const MgMotion* sender)
{
    dynshape()->shape()->setPoint(0, snapPoint(sender));
    dynshape()->shape()->update();
    
    return MgCommandDraw::touchMoved(sender);
}

bool MgCmdDrawDot::touchEnded(const MgMotion* sender)
{
    dynshape()->shape()->setPoint(0, snapPoint(sender));
    dynshape()->shape()->update();
    
    addShape(sender);
    m_step = 0;
    
    return MgCommandDraw::touchEnded(sender);
}
