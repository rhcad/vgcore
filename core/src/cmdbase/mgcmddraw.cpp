// mgcmddraw.cpp: 实现绘图命令基类
// Copyright (c) 2004-2013, Zhang Yungui
// License: LGPL, https://github.com/rhcad/touchvg

#include "mgcmddraw.h"
#include "mgsnap.h"
#include "mgaction.h"
#include "cmdsubject.h"
#include <string.h>
#include "mglog.h"
#include "mgstorage.h"

MgCommandDraw::MgCommandDraw(const char* name)
    : MgCommand(name), m_step(0), m_shape(NULL)
{
}

MgCommandDraw::~MgCommandDraw()
{
    MgObject::release_pointer(m_shape);
}

bool MgCommandDraw::cancel(const MgMotion* sender)
{
    if (m_step > 0) {
        m_step = 0;
        m_shape->shape()->clear();
        sender->view->getSnap()->clearSnap(sender);
        sender->view->redraw();
        return true;
    }
    return false;
}

bool MgCommandDraw::_initialize(int shapeType, const MgMotion* sender, MgStorage* s)
{
    if (!m_shape) {
        m_shape = sender->view->createShapeCtx(shapeType);
        if (!m_shape || !m_shape->shape())
            return false;
        m_shape->setParent(sender->view->shapes(), 0);
    }
    sender->view->setNewShapeID(0);
    m_step = 0;
    m_shape->shape()->clear();
    m_shape->setContext(*sender->view->context());
    m_oneShapeEnd = !!sender->view->getOptionInt(getName(), "oneShape", 0);
    
    int n = s ? s->readFloatArray("points", NULL, 0) : 0;
    if (n > 1) {
        MgMotion tmpmotion(*sender);
        Point2d buf[20];
        
        n = s->readFloatArray("points", &buf[0].x, mgMin(n, 100*2)) / 2;
        for (int i = 0; i < n; i += 2) {
            tmpmotion.pointM = buf[i];
            touchBegan(&tmpmotion);
            tmpmotion.pointM = buf[i + 1 < n ? i + 1 : i];
            touchEnded(&tmpmotion);
        }
    }
    
    return true;
}

MgShape* MgCommandDraw::addShape(const MgMotion* sender, MgShape* shape)
{
    shape = shape ? shape : m_shape;
    MgShape* newsp = NULL;
    
    if (sender->view->shapeWillAdded(shape)) {
        newsp = sender->view->shapes()->addShape(*shape);
        if (shape == m_shape) {
            m_shape->shape()->clear();
            sender->view->shapeAdded(newsp);
        } else {
            sender->view->getCmdSubject()->onShapeAdded(sender, newsp);
        }
        if (strcmp(getName(), "splines") != 0) {
            sender->view->setNewShapeID(newsp->getID());
        }
    }
    if (m_shape && sender->view->context()) {
        m_shape->setContext(*sender->view->context());
    }
    if (m_oneShapeEnd) {
        sender->view->toSelectCommand();
    }
    
    return newsp;
}

bool MgCommandDraw::backStep(const MgMotion* sender)
{
    if (m_step > 1) {
        m_step--;
        sender->view->redraw();
        return true;
    }
    return false;
}

bool MgCommandDraw::draw(const MgMotion* sender, GiGraphics* gs)
{
    bool ret = m_step > 0 && m_shape->draw(0, *gs, NULL, -1);
    return sender->view->getSnap()->drawSnap(sender, gs) || ret;
}

bool MgCommandDraw::gatherShapes(const MgMotion* /*sender*/, MgShapes* shapes)
{
    if (m_step > 0 && m_shape && m_shape->shapec()->getPointCount() > 0) {
        shapes->addShape(*m_shape);
    }
    return false; // gather more shapes via draw()
}

bool MgCommandDraw::click(const MgMotion* sender)
{
    return (m_step == 0 ? _click(sender)
            : touchBegan(sender) && touchEnded(sender));
}

bool MgCommandDraw::_click(const MgMotion* sender)
{
    Box2d limits(sender->displayMmToModelBox("select", "hitTestTol", 10.f));
    MgHitResult res;
    const MgShape* shape = sender->view->shapes()->hitTest(limits, res);
    
    if (shape) {
        sender->view->setNewShapeID(shape->getID());
        sender->view->toSelectCommand();
        LOGD("Command (%s) cancelled after the shape #%d clicked.", getName(), shape->getID());
    }
    
    return shape || (sender->view->useFinger() && longPress(sender));
}

bool MgCommandDraw::longPress(const MgMotion* sender)
{
    return sender->view->getAction()->showInDrawing(sender, m_shape);
}

bool MgCommandDraw::touchBegan(const MgMotion* sender)
{
    m_shape->setContext(*sender->view->context());
    sender->view->redraw();
    return true;
}

bool MgCommandDraw::touchMoved(const MgMotion* sender)
{
    sender->view->redraw();
    return true;
}

bool MgCommandDraw::touchEnded(const MgMotion* sender)
{
    sender->view->getSnap()->clearSnap(sender);
    sender->view->redraw();
    return true;
}

bool MgCommandDraw::mouseHover(const MgMotion* sender)
{
    return m_step > 0 && touchMoved(sender);
}

Point2d MgCommandDraw::snapPoint(const MgMotion* sender, bool firstStep)
{
    return snapPoint(sender, firstStep ? sender->startPtM : sender->pointM, firstStep);
}

Point2d MgCommandDraw::snapPoint(const MgMotion* sender, 
                                 const Point2d& orignPt, bool firstStep)
{
    MgSnap *snap = sender->view->getSnap();
    Point2d pt(snap->snapPoint(sender, orignPt,
                               firstStep ? NULL : m_shape, m_step));
    
    if ( (firstStep || !sender->dragging())
        && snap->getSnappedType() >= kMgSnapPoint) {
        sender->view->getCmdSubject()->onPointSnapped(sender, dynshape());
    }
    
    return pt;
}

int MgCommandDraw::getSnappedType(const MgMotion* sender) const
{
    return sender->view->getSnap()->getSnappedType();
}

void MgCommandDraw::setStepPoint(int step, const Point2d& pt)
{
    if (step > 0) {
        dynshape()->shape()->setHandlePoint(step, pt, 0);
    }
}

bool MgCommandDraw::touchBeganStep(const MgMotion* sender)
{
    if (0 == m_step) {
        m_step = 1;
        Point2d pnt(snapPoint(sender, true));
        for (int i = dynshape()->shape()->getPointCount() - 1; i >= 0; i--) {
            dynshape()->shape()->setPoint(i, pnt);
        }
        setStepPoint(0, pnt);
    }
    else {
        setStepPoint(m_step, snapPoint(sender));
    }
    dynshape()->shape()->update();

    return MgCommandDraw::touchBegan(sender);
}

bool MgCommandDraw::touchMovedStep(const MgMotion* sender)
{
    if (sender->dragging()) {
        setStepPoint(m_step, snapPoint(sender));
        dynshape()->shape()->update();
    }
    return MgCommandDraw::touchMoved(sender);
}

bool MgCommandDraw::touchEndedStep(const MgMotion* sender)
{
    Point2d pnt(snapPoint(sender));
    Tol tol(sender->displayMmToModel(2.f));
    
    setStepPoint(m_step, pnt);
    dynshape()->shape()->update();
    
    if (!pnt.isEqualTo(dynshape()->shape()->getPoint(m_step - 1), tol)) {
        m_step++;
        if (m_step >= getMaxStep()) {
            m_step = 0;
            if (!dynshape()->shape()->getExtent().isEmpty(tol, false)) {
                addShape(sender);
            }
        }
    }

    return MgCommandDraw::touchEnded(sender);
}

// MgLocalized
//

#include "mglocal.h"
#include "mgcoreview.h"

struct MgStringCallback1 : MgStringCallback {
    std::string result;
    virtual void onGetString(const char* text) { if (text) result = text; }
};

std::string MgLocalized::getString(MgView* view, const char* name)
{
    MgStringCallback1 c;
    view->getLocalizedString(name, &c);
    return c.result.empty() ? name : c.result;
}

int MgLocalized::formatString(char *buffer, size_t size, MgView* view, const char *format, ...)
{
    std::string str;
    va_list arglist;

    va_start(arglist, format);
    if (*format == '@') {
        str = getString(view, format + 1);
        format = str.c_str();
    }
#if defined(_MSC_VER) && _MSC_VER >= 1400
    return vsprintf_s(buffer, size, format, arglist);
#else
    return vsprintf(buffer, format, arglist);
#endif
}
