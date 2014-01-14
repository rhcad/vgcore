//! \file gicoreview.cpp
//! \brief 实现内核视图类 GiCoreView
// Copyright (c) 2012-2013, https://github.com/rhcad/touchvg

#include "gicoreview.h"
#include "GcShapeDoc.h"
#include "GcMagnifierView.h"
#include "mgcmdmgr.h"
#include "mgcmdmgrfactory.h"
#include "mgbasicspreg.h"
#include "RandomShape.h"
#include "mgjsonstorage.h"
#include "mglayer.h"
#include "cmdsubject.h"
#include "mgselect.h"
#include "mglog.h"
#include "mgshapet.h"
#include "girecordcanvas.h"
#include "girecordshape.h"
#include "cmdbasic.h"
#include <map>
#include "../corever.h"

#define CALL_VIEW(func) if (curview) curview->func
#define CALL_VIEW2(func, v) curview ? curview->func : v

static int _dpi = 96;
static float _factor = 1.0f;

//! 供Java等语言用的 MgShape 实现类
class MgShapeExt : public MgShape
{
public:
    MgBaseShape* _shape;
    GiContext   _context;
    int         _id;
    MgShapes*   _parent;
    int         _tag;
    volatile long _refcount;
    
    MgShapeExt(MgBaseShape* shape)
        : _shape(shape), _id(0), _parent(NULL), _tag(0), _refcount(1) {
    }
    virtual ~MgShapeExt() { _shape->release(); }

    const GiContext& context() const { return _context; }
    void setContext(const GiContext& ctx, int mask) { _context.copy(ctx, mask); }
    MgBaseShape* shape() { return _shape; }
    const MgBaseShape* shapec() const { return _shape; }
    int getType() const { return 0x20000 | _shape->getType(); }
    void release() { if (giInterlockedDecrement(&_refcount) == 0) delete this; }
    void addRef() { giInterlockedIncrement(&_refcount); }
    int getTag() const { return _tag; }
    void setTag(int tag) { _tag = tag; }
    int getID() const { return _id; }
    MgShapes* getParent() const { return _parent; }
    
    MgObject* clone() const {
        MgShapeExt *p = new MgShapeExt(_shape->cloneShape());
        p->copy(*this);
        return p;
    }
    void setParent(MgShapes* p, int sid) {
        _parent = p;
        _id = sid;
        shape()->setOwner(this);
    }
};

//! GiCoreView实现类
class GiCoreViewImpl
    : public MgView
    , private MgShapeFactory
{
public:
    GcShapeDoc*     _doc;
    MgCmdManager*   _cmds;
    GcBaseView*     curview;
    long            refcount;
    MgMotion        _motion;
    int             gestureHandler;
    MgJsonStorage   defaultStorage;
    
    long            regenPending;
    long            appendPending;
    long            redrawPending;
    volatile long   changeCount;
    volatile long   drawCount;
    
    std::map<int, MgShape* (*)()>   _shapeCreators;
    MgShapes*       dynShapes;
    MgShapes*       dynShapesBack;

public:
    GiCoreViewImpl() : curview(NULL), refcount(1), gestureHandler(0)
        , regenPending(-1), appendPending(-1), redrawPending(-1)
        , changeCount(0), drawCount(0), dynShapes(NULL), dynShapesBack(NULL)
    {
        _motion.view = this;
        _motion.gestureType = 0;
        _motion.gestureState = kMgGesturePossible;
        _doc = new GcShapeDoc();
        _cmds = MgCmdManagerFactory::create();

        MgBasicShapes::registerShapes(this);
        MgBasicCommands::registerCmds(this);
        MgShapeT<MgRecordShape>::registerCreator(this);
    }

    ~GiCoreViewImpl() {
        MgObject::release_pointer(dynShapes);
        MgObject::release_pointer(dynShapesBack);
        _cmds->release();
        delete _doc;
    }

    MgMotion* motion() { return &_motion; }
    MgCmdManager* cmds() const { return _cmds; }
    GcShapeDoc* document() const { return _doc; }
    MgShapeDoc* doc() const { return _doc->backDoc(); }
    MgShapes* shapes() const { return doc()->getCurrentShapes(); }
    GiContext* context() const { return doc()->context(); }
    GiTransform* xform() const { return CALL_VIEW2(xform(), NULL); }
    Matrix2d& modelTransform() const { return doc()->modelTransform(); }

    int getNewShapeID() { return _cmds->getNewShapeID(); }
    void setNewShapeID(int sid) { _cmds->setNewShapeID(sid); }
    CmdSubject* getCmdSubject() { return cmds()->getCmdSubject(); }
    MgSelection* getSelection() { return cmds()->getSelection(); }
    MgShapeFactory* getShapeFactory() { return this; }
    MgSnap* getSnap() { return _cmds->getSnap(); }
    MgActionDispatcher* getAction() {
        return _cmds->getActionDispatcher(); }

    bool registerCommand(const char* name, MgCommand* (*creator)()) {
        return _cmds->registerCommand(name, creator); }
    bool toSelectCommand() { return _cmds->setCommand(&_motion, "select", NULL); }
    const char* getCommandName() { return _cmds->getCommandName(); }
    MgCommand* getCommand() { return _cmds->getCommand(); }
    MgCommand* findCommand(const char* name) {
        return _cmds->findCommand(name); }
    bool setCommand(const char* name) { return _cmds->setCommand(&_motion, name, NULL); }
    bool setCurrentShapes(MgShapes* shapes) {
        return doc()->setCurrentShapes(shapes); }
    bool isReadOnly() const { return doc()->isReadOnly() || doc()->getCurrentLayer()->isLocked(); }

    bool shapeWillAdded(MgShape* shape) {
        return getCmdSubject()->onShapeWillAdded(motion(), shape); }
    bool shapeWillDeleted(const MgShape* shape) {
        return getCmdSubject()->onShapeWillDeleted(motion(), shape); }
    bool shapeCanRotated(const MgShape* shape) {
        return getCmdSubject()->onShapeCanRotated(motion(), shape); }
    bool shapeCanTransform(const MgShape* shape) {
        return getCmdSubject()->onShapeCanTransform(motion(), shape); }
    bool shapeCanUnlock(const MgShape* shape) {
        return getCmdSubject()->onShapeCanUnlock(motion(), shape); }
    bool shapeCanUngroup(const MgShape* shape) {
        return getCmdSubject()->onShapeCanUngroup(motion(), shape); }
    void shapeMoved(MgShape* shape, int segment) {
        getCmdSubject()->onShapeMoved(motion(), shape, segment); }

    void commandChanged() {
        CALL_VIEW(deviceView()->commandChanged());
    }
    void selectionChanged() {
        CALL_VIEW(deviceView()->selectionChanged());
    }
    void dynamicChanged() {
        CALL_VIEW(deviceView()->dynamicChanged());
    }

    bool removeShape(const MgShape* shape) {
        showContextActions(0, NULL, Box2d::kIdentity(), NULL);
        bool ret = (shape && shape->getParent()
                    && shape->getParent()->findShape(shape->getID()) == shape);
        if (ret) {
            getCmdSubject()->onShapeDeleted(motion(), shape);
            shape->getParent()->removeShape(shape->getID());
        }
        return ret;
    }

    bool useFinger() {
        return CALL_VIEW2(deviceView()->useFinger(), true);
    }

    bool isContextActionsVisible() {
        return CALL_VIEW2(deviceView()->isContextActionsVisible(), false);
    }

    bool showContextActions(int /*selState*/, const int* actions, 
        const Box2d& selbox, const MgShape*)
    {
        int n = 0;
        for (; actions && actions[n] > 0; n++) {}

        if (n > 0 && motion()->pressDrag && isContextActionsVisible()) {
            return false;
        }
        mgvector<int> arr(actions, n);
        mgvector<float> pos(2 * n);
        calcContextButtonPosition(pos, n, selbox);
        return CALL_VIEW2(deviceView()->showContextActions(arr, pos,
            selbox.xmin, selbox.ymin, selbox.width(), selbox.height()), false);
    }

    void shapeAdded(MgShape* sp) {
        regenAppend(sp->getID());
        getCmdSubject()->onShapeAdded(motion(), sp);
    }

    void redraw() {
        if (redrawPending >= 0) {
            redrawPending++;
        }
        else {
            giInterlockedIncrement(&drawCount);
            CALL_VIEW(deviceView()->redraw());  // 将调用dynDraw
        }
    }

    void regenAll(bool changed) {
        if (regenPending >= 0) {
            regenPending += changed ? 100 : 1;
        }
        else {  // 将调用drawAll
            giInterlockedIncrement(&drawCount);
            if (changed) {
                giInterlockedIncrement(&changeCount);
                for (int i = 0; i < _doc->getViewCount(); i++) {
                    _doc->getView(i)->deviceView()->regenAll(changed);
                }
                CALL_VIEW(deviceView()->contentChanged());
            }
            else {
                CALL_VIEW(deviceView()->regenAll(changed));
                for (int i = 0; i < _doc->getViewCount(); i++) {
                    if (_doc->getView(i) != curview)
                        _doc->getView(i)->deviceView()->redraw();
                }
            }
        }
    }

    void regenAppend(int sid) {
        if (appendPending == 0) {
            appendPending = sid;
        }
        else if (appendPending > 0) {
            if (appendPending != sid) {
                regenAll(true);
            }
        }
        else {  // 将调用drawAppend
            giInterlockedIncrement(&drawCount);
            giInterlockedIncrement(&changeCount);
            for (int i = 0; i < _doc->getViewCount(); i++) {
                _doc->getView(i)->deviceView()->regenAppend(sid);
            }
            CALL_VIEW(deviceView()->contentChanged());
        }
    }

    bool setView(GcBaseView* view) {
        if (curview != view) {
            GcBaseView* oldview = curview;
            curview = view;
            CALL_VIEW(deviceView()->viewChanged(oldview ? oldview->deviceView() : NULL));
        }
        return !!view;
    }

    bool gestureToCommand();

private:
    void registerShape(int type, MgShape* (*creator)()) {
        type = type & 0xFFFF;
        if (creator) {
            _shapeCreators[type] = creator;
        }
        else {
            _shapeCreators.erase(type);
        }
    }
    MgShape* createShape(int type) {
        std::map<int, MgShape* (*)()>::const_iterator it = _shapeCreators.find(type & 0xFFFF);
        if (it != _shapeCreators.end()) {
            return (it->second)();
        }

        MgBaseShape* sp = getCmdSubject()->createShape(motion(), type);
        if (sp) {
            return new MgShapeExt(sp);
        }
        
        return NULL;
    }

private:
    void calcContextButtonPosition(mgvector<float>& pos, int n, const Box2d& box)
    {
        Box2d selbox(box);

        selbox.inflate(12 * _factor, 18 * _factor);
        if (box.height() < (n < 7 ? 40 : 80) * _factor) {
            selbox.deflate(0, (box.height() - (n < 7 ? 40 : 80) * _factor) / 2);
        }
        if (box.width() < (n == 3 || n > 4 ? 120 : 40) * _factor) {
            selbox.deflate((box.width() - (n==3||n>4 ? 120 : 40) * _factor) / 2, 0);
        }

        Box2d rect(calcButtonPosition(pos, n, selbox));
        Vector2d off(moveActionsInView(rect));

        for (int i = 0; i < n; i++) {
            pos.set(2 * i, pos.get(2 * i) + off.x, pos.get(2 * i + 1) + off.y);
        }
    }

    Box2d calcButtonPosition(mgvector<float>& pos, int n, const Box2d& selbox)
    {
        Box2d rect;

        for (int i = 0; i < n; i++) {
            switch (i)
            {
            case 0:
                if (n == 1) {
                    pos.set(2 * i, selbox.center().x, selbox.ymin); // MT
                } else {
                    pos.set(2 * i, selbox.xmin, selbox.ymin);       // LT
                }
                break;
            case 1:
                if (n == 3) {
                    pos.set(2 * i, selbox.center().x, selbox.ymin); // MT
                } else {
                    pos.set(2 * i, selbox.xmax, selbox.ymin);       // RT
                }
                break;
            case 2:
                if (n == 3) {
                    pos.set(2 * i, selbox.xmax, selbox.ymin);       // RT
                } else {
                    pos.set(2 * i, selbox.xmax, selbox.ymax);       // RB
                }
                break;
            case 3:
                pos.set(2 * i, selbox.xmin, selbox.ymax);           // LB
                break;
            case 4:
                pos.set(2 * i, selbox.center().x, selbox.ymin);     // MT
                break;
            case 5:
                pos.set(2 * i, selbox.center().x, selbox.ymax);     // MB
                break;
            case 6:
                pos.set(2 * i, selbox.xmax, selbox.center().y);     // RM
                break;
            case 7:
                pos.set(2 * i, selbox.xmin, selbox.center().y);     // LM
                break;
            default:
                return rect;
            }
            rect.unionWith(Box2d(Point2d(pos.get(2 * i), pos.get(2 * i + 1)),
                                 32 * _factor, 32 * _factor));
        }

        return rect;
    }

    Vector2d moveActionsInView(Box2d& rect)
    {
        Vector2d off;
        Box2d viewrect(xform()->getWndRect());

        if (!rect.isEmpty() && !viewrect.contains(rect)) {
            if (rect.xmin < 0) {
                off.x = -rect.xmin;
            }
            else if (rect.xmax > viewrect.xmax) {
                off.x = viewrect.xmax - rect.xmax;
            }

            if (rect.ymin < 0) {
                off.y = -rect.ymin;
            }
            else if (rect.ymax > viewrect.ymax) {
                off.y = viewrect.ymax - rect.ymax;
            }
        }

        return off;
    }
};

// DrawLocker
//

class DrawLocker
{
    GiCoreViewImpl* _impl;
public:
    DrawLocker(GiCoreViewImpl* impl) : _impl(NULL) {
        if (impl->regenPending < 0
            && impl->appendPending < 0
            && impl->redrawPending < 0)
        {
            _impl = impl;
            _impl->regenPending = 0;
            _impl->appendPending = 0;
            _impl->redrawPending = 0;
        }
    }

    ~DrawLocker() {
        if (!_impl) {
            return;
        }
        long regenPending = _impl->regenPending;
        long appendPending = _impl->appendPending;
        long redrawPending = _impl->redrawPending;

        _impl->regenPending = -1;
        _impl->appendPending = -1;
        _impl->redrawPending = -1;

        if (regenPending > 0) {
            _impl->regenAll(regenPending >= 100);
        }
        else if (appendPending > 0) {
            _impl->regenAppend((int)appendPending);
        }
        else if (redrawPending > 0) {
            _impl->redraw();
        }
    }
};

// GcBaseView
//

GcBaseView::GcBaseView(MgView* mgview, GiView *view)
    : _mgview(mgview), _view(view), _gsFront(&_xfFront), _gsBack(&_xfBack)
{
    for (unsigned i = 0; i < sizeof(_gsBuf)/sizeof(_gsBuf[0]); i++) {
        _gsBuf[i] = NULL;
        _gsUsed[i] = 0;
    }
    mgview->document()->addView(this);
}

MgShapeDoc* GcBaseView::backDoc()
{
    return cmdView()->document()->backDoc();
}

MgShapeDoc* GcBaseView::frontDoc()
{
    return cmdView()->document()->frontDoc();
}

MgShapes* GcBaseView::backShapes()
{
    return backDoc()->getCurrentShapes();
}

// GiCoreView
//

int GiCoreView::getVersion() { return COREVERSION; }

GiCoreView::GiCoreView(GiCoreView* mainView)
{
    if (mainView) {
        impl = mainView->impl;
        impl->refcount++;
    }
    else {
        impl = new GiCoreViewImpl;
    }
}

GiCoreView::~GiCoreView()
{
    if (--impl->refcount == 0) {
        delete impl;
    }
}

long GiCoreView::viewAdapterHandle()
{
    return impl->toHandle();
}

long GiCoreView::backDoc()
{
    return impl->doc()->toHandle();
}

long GiCoreView::backShapes()
{
    return impl->shapes()->toHandle();
}

long GiCoreView::acquireFrontDoc()
{
    impl->_doc->frontDoc()->addRef();
    return impl->_doc->frontDoc()->toHandle();
}

void GiCoreView::releaseDoc(long hDoc)
{
    MgShapeDoc* doc = MgShapeDoc::fromHandle(hDoc);
    MgObject::release_pointer(doc);
}

void GiCoreView::submitBackDoc()
{
    if (impl->curview) {
        impl->doc()->saveAll(NULL, impl->curview->xform());  // save viewport
        impl->curview->submitBackXform();
    }
    impl->_doc->submitBackDoc();
}

long GiCoreView::acquireDynamicShapes()
{
    if (impl->dynShapes) {
        impl->dynShapes->addRef();
        return impl->dynShapes->toHandle();
    }
    return 0;
}

void GiCoreView::releaseShapes(long hShapes)
{
    MgShapes* p = MgShapes::fromHandle(hShapes);
    MgObject::release_pointer(p);
}

void GiCoreView::createView(GiView* view, int type)
{
    if (view && !impl->_doc->findView(view)) {
        impl->curview = new GcGraphView(impl, view);
        if (type == 0) {
            setCommand(view, "splines");
        }
    }
}

void GiCoreView::createMagnifierView(GiView* newview, GiView* mainView)
{
    GcGraphView* refview = dynamic_cast<GcGraphView *>(impl->_doc->findView(mainView));

    if (refview && newview && !impl->_doc->findView(newview)) {
        new GcMagnifierView(impl, newview, refview);
    }
}

void GiCoreView::destoryView(GiView* view)
{
    GcBaseView* aview = this ? impl->_doc->findView(view) : NULL;

    if (aview && impl->_doc->removeView(aview)) {
        if (impl->curview == aview) {
            impl->curview = impl->_doc->firstView();
        }
        //delete aview;
    }
}

int GiCoreView::setBkColor(GiView* view, int argb)
{
    GcBaseView* aview = impl->_doc->findView(view);
    return aview ? aview->graph()->setBkColor(GiColor(argb)).getARGB() : 0;
}

void GiCoreView::setScreenDpi(int dpi, float factor)
{
    if (_dpi != dpi && dpi > 0) {
        _dpi = dpi;
    }
    if (factor != _factor && factor > 0.1f) {
        _factor = factor;
        GiGraphics::setPenWidthFactor(factor);
    }
}

bool GiCoreView::isDrawing(GiView* view)
{
    GcBaseView* aview = impl->_doc->findView(view);
    return aview && aview->isDrawing();
}

bool GiCoreView::stopDrawing(GiView* view)
{
    GcBaseView* aview = impl->_doc->findView(view);
    return aview && aview->stopDrawing() > 0;
}

long GiCoreView::acquireGraphics(GiView* view)
{
    GcBaseView* aview = impl->_doc->findView(view);
    return aview ? aview->createFrontGraph()->toHandle() : 0;
}

void GiCoreView::releaseGraphics(GiView* view, long hGs)
{
    GcBaseView* aview = impl->_doc->findView(view);
    if (aview && hGs) {
        aview->releaseFrontGraph(GiGraphics::fromHandle(hGs));
    }
}

int GiCoreView::drawAll(GiView* view, GiCanvas* canvas) {
    long hDoc = acquireFrontDoc();
    long hGs = acquireGraphics(view);
    int n = drawAll(hDoc, hGs, canvas);
    releaseDoc(hDoc);
    releaseGraphics(view, hGs);
    return n;
}

int GiCoreView::drawAppend(GiView* view, GiCanvas* canvas, int sid) {
    long hDoc = acquireFrontDoc();
    long hGs = acquireGraphics(view);
    int n = drawAppend(hDoc, hGs, canvas, sid);
    releaseDoc(hDoc);
    releaseGraphics(view, hGs);
    return n;
}

int GiCoreView::dynDraw(GiView* view, GiCanvas* canvas){
    long hShapes = acquireDynamicShapes();
    long hGs = acquireGraphics(view);
    int n = dynDraw(hShapes, hGs, canvas);
    releaseShapes(hShapes);
    releaseGraphics(view, hGs);
    return n;
}

int GiCoreView::drawAll(long hDoc, long hGs, GiCanvas* canvas)
{
    int n = -1;
    GiGraphics* gs = GiGraphics::fromHandle(hGs);
    
    if (hDoc && gs && gs->beginPaint(canvas)) {
        n = MgShapeDoc::fromHandle(hDoc)->draw(*gs);
        gs->endPaint();
    }

    return n;
}

int GiCoreView::drawAppend(long hDoc, long hGs, GiCanvas* canvas, int sid)
{
    int n = -1;
    GiGraphics* gs = GiGraphics::fromHandle(hGs);
    
    if (hDoc && gs && sid && gs->beginPaint(canvas)) {
        const MgShapes* sps = MgShapeDoc::fromHandle(hDoc)->getCurrentShapes();
        const MgShape* sp = sps->findShape(sid);
        n = (sp && sp->draw(0, *gs, NULL, -1)) ? 1 : 0;
        gs->endPaint();
    }
    
    return n;
}

int GiCoreView::dynDraw(long hShapes, long hGs, GiCanvas* canvas)
{
    int n = -1;
    GiGraphics* gs = GiGraphics::fromHandle(hGs);

    if (hShapes && gs && gs->beginPaint(canvas)) {
        mgCopy(impl->motion()->d2mgs, impl->cmds()->displayMmToModel(1, gs));
        n = MgShapes::fromHandle(hShapes)->draw(*gs);
        gs->endPaint();
    }
    
    return n;
}

void GiCoreView::onSize(GiView* view, int w, int h)
{
    GcBaseView* aview = impl->_doc->findView(view);
    if (aview) {
        aview->onSize(_dpi, w, h);
    }
}

bool GiCoreView::onGesture(GiView* view, GiGestureType type,
                           GiGestureState state, float x, float y, bool switchGesture)
{
    DrawLocker locker(impl);
    GcBaseView* aview = impl->_doc->findView(view);
    bool ret = false;

    if (impl->setView(aview)) {
        impl->motion()->gestureType = type;
        impl->motion()->gestureState = (MgGestureState)state;
        impl->motion()->pressDrag = (type == kGiGesturePress && state < kGiGestureEnded);
        impl->motion()->switchGesture = switchGesture;
        impl->motion()->point.set(x, y);
        impl->motion()->pointM = impl->motion()->point * aview->xform()->displayToModel();
        impl->motion()->point2 = impl->motion()->point;
        impl->motion()->point2M = impl->motion()->pointM;
        impl->motion()->d2m = impl->cmds()->displayMmToModel(1, impl->motion());

        if (state <= kGiGestureBegan) {
            impl->motion()->startPt = impl->motion()->point;
            impl->motion()->startPtM = impl->motion()->pointM;
            impl->motion()->lastPt = impl->motion()->point;
            impl->motion()->lastPtM = impl->motion()->pointM;
            impl->motion()->startPt2 = impl->motion()->point;
            impl->motion()->startPt2M = impl->motion()->pointM;
            impl->gestureHandler = (impl->gestureToCommand() ? 1
                                    : (aview->onGesture(impl->_motion) ? 2 : 0));
        }
        else if (impl->gestureHandler == 1) {
            impl->gestureToCommand();
        }
        else if (impl->gestureHandler == 2) {
            aview->onGesture(impl->_motion);
        }
        ret = (impl->gestureHandler > 0);

        impl->motion()->lastPt = impl->motion()->point;
        impl->motion()->lastPtM = impl->motion()->pointM;
    }

    return ret;
}

bool GiCoreView::twoFingersMove(GiView* view, GiGestureState state,
                                float x1, float y1, float x2, float y2, bool switchGesture)
{
    DrawLocker locker(impl);
    GcBaseView* aview = impl->_doc->findView(view);
    bool ret = false;

    if (impl->setView(aview)) {
        impl->motion()->gestureType = kGiTwoFingersMove;
        impl->motion()->gestureState = (MgGestureState)state;
        impl->motion()->pressDrag = false;
        impl->motion()->switchGesture = switchGesture;
        impl->motion()->point.set(x1, y1);
        impl->motion()->pointM = impl->motion()->point * aview->xform()->displayToModel();
        impl->motion()->point2.set(x2, y2);
        impl->motion()->point2M = impl->motion()->point2 * aview->xform()->displayToModel();
        impl->motion()->d2m = impl->cmds()->displayMmToModel(1, impl->motion());

        if (state <= kGiGestureBegan) {
            impl->motion()->startPt = impl->motion()->point;
            impl->motion()->startPtM = impl->motion()->pointM;
            impl->motion()->lastPt = impl->motion()->point;
            impl->motion()->lastPtM = impl->motion()->pointM;
            impl->motion()->startPt2 = impl->motion()->point2;
            impl->motion()->startPt2M = impl->motion()->point2M;
            impl->gestureHandler = (impl->gestureToCommand() ? 1
                                    : (aview->twoFingersMove(impl->_motion) ? 2 : 0));
        }
        else if (impl->gestureHandler == 1) {
            impl->gestureToCommand();
        }
        else if (impl->gestureHandler == 2) {
            aview->twoFingersMove(impl->_motion);
        }
        ret = (impl->gestureHandler > 0);

        impl->motion()->lastPt = impl->motion()->point;
        impl->motion()->lastPtM = impl->motion()->pointM;
    }

    return ret;
}

bool GiCoreView::isPressDragging()
{
    return impl->motion()->pressDrag;
}

GiGestureType GiCoreView::getGestureType()
{
    return (GiGestureType)impl->motion()->gestureType;
}

GiGestureState GiCoreView::getGestureState()
{
    return (GiGestureState)impl->motion()->gestureState;
}

const char* GiCoreView::getCommand() const
{
    return impl->_cmds->getCommandName();
}

bool GiCoreView::setCommand(const char* name, const char* params)
{
    DrawLocker locker(impl);
    MgJsonStorage s;
    return impl->curview && impl->_cmds->setCommand(impl->motion(), name, s.storageForRead(params));
}

bool GiCoreView::setCommand(GiView* view, const char* name, const char* params)
{
    GcBaseView* aview = impl->_doc->findView(view);
    return impl->setView(aview) && setCommand(name, params);
}

bool GiCoreView::doContextAction(int action)
{
    DrawLocker locker(impl);
    return impl->_cmds->doContextAction(impl->motion(), action);
}

void GiCoreView::clearCachedData()
{
    impl->doc()->clearCachedData();
}

int GiCoreView::addShapesForTest()
{
    int n = RandomParam().addShapes(impl->shapes());
    impl->regenAll(true);
    return n;
}

int GiCoreView::getShapeCount()
{
    return impl->doc()->getShapeCount();
}

long GiCoreView::getChangeCount()
{
    return impl->changeCount;
}

long GiCoreView::getDrawCount() const
{
    return impl->drawCount;
}

int GiCoreView::getSelectedShapeCount()
{
    return impl->cmds()->getSelection(impl, 0, NULL);
}

int GiCoreView::getSelectedShapeID()
{
    const MgShape* shape = NULL;
    impl->cmds()->getSelection(impl, 1, &shape);
    return shape ? shape->getID() : 0;
}

int GiCoreView::getSelectedShapeType()
{
    MgSelection* sel = impl->cmds()->getSelection();
    return sel ? sel->getSelectType(impl) : 0;
}

bool GiCoreView::loadShapes(MgStorage* s, bool readOnly)
{
    DrawLocker locker(impl);
    bool ret = false;

    MgCommand* cmd = impl->getCommand();
    if (cmd) cmd->cancel(impl->motion());
    
    impl->showContextActions(0, NULL, Box2d::kIdentity(), NULL);

    if (s) {
        ret = impl->doc()->loadAll(impl->getShapeFactory(), s, impl->xform());
        impl->doc()->setReadOnly(readOnly);
        LOGD("Load %d shapes and %d layers",
             impl->doc()->getShapeCount(), impl->doc()->getLayerCount());
    }
    else {
        impl->doc()->clear();
        ret = true;
    }
    impl->regenAll(true);
    impl->getCmdSubject()->onDocLoaded(impl->motion());

    return ret;
}

bool GiCoreView::saveShapes(long hDoc, MgStorage* s)
{
    const MgShapeDoc* doc = MgShapeDoc::fromHandle(hDoc);
    return doc && doc->save(s, 0);
}

bool GiCoreView::loadDynamicShapes(MgStorage* s)
{
    bool ret = false;
    
    if (s) {
        MgObject::release_pointer(impl->dynShapesBack);
        impl->dynShapesBack = MgShapes::create();
        ret = impl->dynShapesBack->load(impl->getShapeFactory(), s);
    }
    else if (impl->dynShapesBack) {
        MgObject::release_pointer(impl->dynShapesBack);
        ret = true;
    }
    
    return ret;
}

void GiCoreView::applyDynamicShapes()
{
    MgObject::release_pointer(impl->dynShapes);
    if (impl->dynShapesBack) {
        impl->dynShapes = impl->dynShapesBack;
        impl->dynShapes->addRef();
        impl->redraw();
    }
}

bool GiCoreView::submitDynamicShapes(GiView* view)
{
    GcBaseView* v = impl->_doc->findView(view);
    MgCommand* cmd = impl->getCommand();
    bool ret = false;

    if (v) {
        v->submitBackXform();
    }
    if (v == impl->curview && cmd && !impl->dynShapesBack) {
        MgObject::release_pointer(impl->dynShapes);
        impl->dynShapes = MgShapes::create();
        
        ret = cmd->gatherShapes(impl->motion(), impl->dynShapes);
        if (!ret) {
            GiRecordCanvas canvas(impl->dynShapes, v->xform(),
                    cmd->isDrawingCommand() ? 0 : -1);
            
            if (v->frontGraph()->beginPaint(&canvas)) {
                cmd->draw(impl->motion(), v->frontGraph());
                if (cmd->isDrawingCommand()) {
                    impl->getCmdSubject()->drawInShapeCommand(impl->motion(), cmd, v->frontGraph());
                }
                v->frontGraph()->endPaint();
            }
            ret = impl->dynShapes->getShapeCount() > 0;
        }
    }
    
    return ret;
}

void GiCoreView::clear()
{
    loadShapes((MgStorage*)0);
}

const char* GiCoreView::getContent(long hDoc)
{
    const char* content = "";
    if (saveShapes(hDoc, impl->defaultStorage.storageForWrite())) {
        content = impl->defaultStorage.stringify();
    }
    return content; // has't free defaultStorage's string buffer
}

void GiCoreView::freeContent()
{
    impl->defaultStorage.clear();
}

bool GiCoreView::setContent(const char* content)
{
    bool ret = loadShapes(impl->defaultStorage.storageForRead(content));
    impl->defaultStorage.clear();
    return ret;
}

bool GiCoreView::loadFromFile(const char* vgfile, bool readOnly)
{
    FILE *fp = mgopenfile(vgfile, "rt");
    MgJsonStorage s;
    bool ret = loadShapes(s.storageForRead(fp), readOnly);

    if (fp) {
        fclose(fp);
    } else {
        LOGE("Fail to open file: %s", vgfile);
    }

    return ret;
}

bool GiCoreView::saveToFile(long hDoc, const char* vgfile, bool pretty)
{
    FILE *fp = mgopenfile(vgfile, "wt");
    MgJsonStorage s;
    bool ret = (fp != NULL
                && saveShapes(hDoc, s.storageForWrite())
                && s.save(fp, pretty));
    
    if (fp) {
        fclose(fp);
    } else {
        LOGE("Fail to open file: %s", vgfile);
    }
    
    return ret;
}

bool GiCoreView::zoomToExtent()
{
    Box2d rect(impl->doc()->getExtent() * impl->xform()->modelToWorld());
    bool ret = impl->xform()->zoomTo(rect);
    if (ret) {
        impl->regenAll(false);
    }
    return ret;
}

bool GiCoreView::zoomToModel(float x, float y, float w, float h)
{
    Box2d rect(Box2d(x, y, x + w, y + h) * impl->xform()->modelToWorld());
    bool ret = impl->xform()->zoomTo(rect);
    if (ret) {
        impl->regenAll(false);
    }
    return ret;
}

float GiCoreView::calcPenWidth(GiView* view, float lineWidth)
{
    GcBaseView* aview = impl->_doc->findView(view);
    return aview ? aview->graph()->calcPenWidth(lineWidth, false) : 0.f;
}

static GiContext _tmpContext;
static bool _contextEditing = false;

GiContext& GiCoreView::getContext(bool forChange)
{
    if (!forChange) {
        _contextEditing = false;
    }

    const MgShape* shape = NULL;
    impl->_cmds->getSelection(impl, 1, &shape);
    _tmpContext = shape ? shape->context() : *impl->context();

    return _tmpContext;
}

void GiCoreView::setContext(int mask)
{
    setContext(_tmpContext, mask, _contextEditing ? 0 : 1);
}

void GiCoreView::setContextEditing(bool editing)
{
    if (_contextEditing != editing) {
        _contextEditing = editing;
        if (!editing) {
            DrawLocker locker(impl);
            impl->_cmds->dynamicChangeEnded(impl, true);
        }
    }
}

void GiCoreView::setContext(const GiContext& ctx, int mask, int apply)
{
    DrawLocker locker(impl);

    if (mask != 0) {
        int n = impl->_cmds->getSelectionForChange(impl, 0, NULL);
        std::vector<MgShape*> shapes(n, (MgShape*)0);

        if (n > 0 && impl->_cmds->getSelectionForChange(impl, n, &shapes.front()) > 0) {
            for (int i = 0; i < n; i++) {
                if (shapes[i]) {
                    shapes[i]->setContext(ctx, mask);
                    shapes[i]->shape()->afterChanged();
                }
            }
            impl->redraw();
        }
        else {
            impl->context()->copy(ctx, mask);
        }
    }

    if (apply != 0) {
        impl->_cmds->dynamicChangeEnded(impl, apply > 0);
    }
}

int GiCoreView::addImageShape(const char* name, float width, float height)
{
    MgShape* shape = impl->_cmds->addImageShape(impl->motion(), name, width, height);
    return shape ? shape->getID() : 0;
}

int GiCoreView::addImageShape(const char* name, float xc, float yc, float w, float h)
{
    MgShape* shape = impl->_cmds->addImageShape(impl->motion(), name, xc, yc, w, h);
    return shape ? shape->getID() : 0;
}

bool GiCoreView::getBoundingBox(mgvector<float>& box)
{
    bool ret = box.count() == 4 && impl->curview;
    if (ret) {
        Box2d rect;
        impl->_cmds->getBoundingBox(rect, impl->motion());
        box.set(0, rect.xmin, rect.ymin);
        box.set(2, rect.xmax, rect.ymax);
    }
    return ret;
}

bool GiCoreView::getBoundingBox(mgvector<float>& box, int shapeId)
{
    const MgShape* shape = impl->shapes()->findShape(shapeId);
    bool ret = box.count() == 4 && shape;
    
    if (ret) {
        Box2d rect(shape->shapec()->getExtent());
        impl->_cmds->getBoundingBox(rect, impl->motion());
        box.set(0, rect.xmin, rect.ymin);
        box.set(2, rect.xmax, rect.ymax);
    }
    return ret;
}

bool GiCoreViewImpl::gestureToCommand()
{
    MgCommand* cmd = _cmds->getCommand();
    bool ret = false;

    if (_motion.gestureState == kMgGestureCancel || !cmd) {
        return cmd && cmd->cancel(&_motion);
    }
    if (_motion.gestureState == kMgGesturePossible
        && _motion.gestureType != kGiTwoFingersMove) {
        return true;
    }

    switch (_motion.gestureType)
    {
    case kGiTwoFingersMove:
        ret = cmd->twoFingersMove(&_motion);
        break;
    case kGiGesturePan:
        switch (_motion.gestureState)
        {
        case kMgGestureBegan:
            ret = cmd->touchBegan(&_motion);
            break;
        case kMgGestureMoved:
            ret = cmd->touchMoved(&_motion);
            break;
        case kMgGestureEnded:
        default:
            ret = cmd->touchEnded(&_motion);
            break;
        }
        break;
    case kGiGestureTap:
        ret = cmd->click(&_motion);
        break;
    case kGiGestureDblTap:
        ret = cmd->doubleClick(&_motion);
        break;
    case kGiGesturePress:
        ret = cmd->longPress(&_motion);
        break;
    }

    if (!ret && !cmd->isDrawingCommand()) {
#ifndef NO_LOGD
        const char* const typeNames[] = { "?", "pan", "tap", "dbltap", "press", "twoFingersMove" };
        const char* const stateNames[] = { "possible", "began", "moved", "ended", "cancel" };
        LOGD("Gesture %s (%s) not supported (%s)",
             typeNames[_motion.gestureType], stateNames[_motion.gestureState], cmd->getName());
#endif
    }
    return ret;
}
