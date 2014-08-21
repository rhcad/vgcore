//! \file gicoreview.cpp
//! \brief 实现内核视图类 GiCoreView
// Copyright (c) 2012-2013, https://github.com/rhcad/touchvg

#include "gicoreview.h"
#include "gicoreviewimpl.h"
#include "RandomShape.h"
#include "mgselect.h"
#include "girecordcanvas.h"
#include "mgbasicspreg.h"
#include "mgbasicsps.h"
#include "svgcanvas.h"
#include "../corever.h"
#include "mgimagesp.h"
#include "mglocal.h"
#include <sstream>

static volatile long _viewCount = 0;    // 总视图数
static int _dpi = 96;                   // 屏幕分辨率，在 GiCoreView::onSize() 中应用到新视图中
float GiCoreViewImpl::_factor = 1.0f;   // 屏幕放大系数，Android高清屏可用

// GcBaseView
//

GcBaseView::GcBaseView(MgView* mgview, GiView *view)
    : _mgview(mgview), _view(view), _zooming(false), _zoomEnabled(true)
{
    mgview->document()->addView(this);
    LOGD("View %p created", this);
}

// GiCoreViewImpl
//

GiCoreViewImpl::GiCoreViewImpl(GiCoreView* owner, bool useCmds)
    : _cmds(NULL), curview(NULL), refcount(1)
    , gestureHandler(0), regenPending(-1), appendPending(-1), redrawPending(-1)
    , changeCount(0), drawCount(0), stopping(0)
{
    memset(&gsBuf, 0, sizeof(gsBuf));
    memset((void*)&gsUsed, 0, sizeof(gsUsed));
    
    drawing = GiPlaying::create(NULL, GiPlaying::kDrawingTag, useCmds);
    backDoc = drawing->getBackDoc();
    addPlaying(drawing);
    
    play.playing = GiPlaying::create(NULL, GiPlaying::kPlayingTag, useCmds);
    addPlaying(play.playing);
    
    _motion.view = this;
    _motion.gestureType = 0;
    _motion.gestureState = kMgGesturePossible;
    _gcdoc = new GcShapeDoc();
    
    MgBasicShapes::registerShapes(this);
    if (useCmds) {
        _cmds = MgCmdManagerFactory::create();
        MgBasicCommands::registerCmds(this);
        MgShapeT<MgRecordShape>::registerCreator(this);
    }
}

GiCoreViewImpl::~GiCoreViewImpl()
{
    for (unsigned i = 0; i < sizeof(gsBuf)/sizeof(gsBuf[0]); i++) {
        delete gsBuf[i];
    }
    MgObject::release_pointer(_cmds);
    delete _gcdoc;
}

void GiCoreViewImpl::showMessage(const char* text)
{
    std::string str;

    if (*text == '@') {
        str = MgLocalized::getString(this, text + 1);
        text = str.c_str();
    }
    CALL_VIEW(deviceView()->showMessage(text));
}

void GiCoreViewImpl::calcContextButtonPosition(mgvector<float>& pos, int n, const Box2d& box)
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
    Vector2d off(moveActionsInView(rect, 16 * _factor));
    
    for (int i = 0; i < n; i++) {
        pos.set(2 * i, pos.get(2 * i) + off.x, pos.get(2 * i + 1) + off.y);
    }
}

Box2d GiCoreViewImpl::calcButtonPosition(mgvector<float>& pos, int n, const Box2d& selbox)
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

Vector2d GiCoreViewImpl::moveActionsInView(Box2d& rect, float btnHalfW)
{
    Vector2d off;
    Box2d viewrect(xform()->getWndRect());
    
    if (!rect.isEmpty() && !viewrect.contains(rect)) {
        if (rect.xmin < btnHalfW) {
            off.x = btnHalfW - rect.xmin;
        }
        else if (rect.xmax > viewrect.xmax - btnHalfW) {
            off.x = viewrect.xmax - btnHalfW - rect.xmax;
        }
        
        if (rect.ymin < btnHalfW) {
            off.y = btnHalfW - rect.ymin;
        }
        else if (rect.ymax > viewrect.ymax - btnHalfW) {
            off.y = viewrect.ymax - btnHalfW - rect.ymax;
        }
    }
    
    return off;
}

// GiCoreView
//

int GiCoreView::getVersion() { return COREVERSION; }

GiCoreView::GiCoreView(GiCoreView* mainView) : refcount(1)
{
    if (mainView) {
        impl = mainView->impl;
        impl->refcount++;
    }
    else {
        impl = new GiCoreViewImpl(this);
    }
    LOGD("GiCoreView %p created, refcount=%ld, n=#%ld",
         this, impl->refcount, giAtomicIncrement(&_viewCount));
}

GiCoreView::GiCoreView(GiView* view, int type) : refcount(1)
{
    impl = new GiCoreViewImpl(this, !!view && type > kNoCmdType);
    LOGD("GiCoreView %p created, type=%d, n=%ld",
         this, type, giAtomicIncrement(&_viewCount));
    createView_(view, type);
}

GiCoreView::~GiCoreView()
{
    LOGD("GiCoreView %p destroyed, refcount=%ld, n=%ld",
         this, impl->refcount, giAtomicDecrement(&_viewCount));
    if (--impl->refcount == 0) {
        delete impl;
    }
}

void GiCoreView::release()
{
    if (giAtomicDecrement(&refcount) == 0) {
        delete this;
    }
}

void GiCoreView::addRef()
{
    giAtomicIncrement(&refcount);
}

long GiCoreView::viewDataHandle()
{
    long h;
    *(MgView**)&h = (GiCoreViewData*)impl;
    return h;
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
    return impl->drawing->acquireFrontDoc();
}

long GiCoreView::acquireFrontDoc(long playh)
{
    GiPlaying* p = GiPlaying::fromHandle(playh);
    return (p ? p : impl->drawing)->acquireFrontDoc();
}

void MgCoreView::releaseDoc(long doc)
{
    MgShapeDoc* p = MgShapeDoc::fromHandle(doc);
    MgObject::release_pointer(p);
}

long GiCoreView::acquireDynamicShapes()
{
    return impl->drawing->acquireFrontShapes();
}

void MgCoreView::releaseShapes(long shapes)
{
    MgShapes* p = MgShapes::fromHandle(shapes);
    MgObject::release_pointer(p);
}

int GiCoreView::acquireFrontDocs(mgvector<long>& docs)
{
    int n = 0;
    
    docs.setSize(1 + impl->getPlayingCount());
    for (int i = 0; i < docs.count() - 1; i++) {
        if (i == 0 && isPlaying())      // 播放时隐藏主文档图形
            continue;
        long doc = impl->acquireFrontDoc(i);
        if (doc) {
            docs.set(n++, doc);
        }
    }
    
    return n;
}

void GiCoreView::releaseDocs(const mgvector<long>& docs)
{
    for (int i = 0; i < docs.count(); i++) {
        MgShapeDoc* doc = MgShapeDoc::fromHandle(docs.get(i));
        MgObject::release_pointer(doc);
    }
}

int GiCoreView::acquireDynamicShapesArray(mgvector<long>& shapes)
{
    int n = 0;
    
    shapes.setSize(1 + impl->getPlayingCount());
    for (int i = 1; i < shapes.count() - 1; i++) {
        if (i == 0 && isPlaying())
            continue;
        long s = impl->acquireFrontShapes(i);
        if (s) {
            shapes.set(n++, s);
        }
    }
    if (!isPlaying()) {     // Show shapes of the current command at top of playings.
        shapes.set(n++, impl->acquireFrontShapes(0));
    }
    
    return n;
}

void GiCoreView::releaseShapesArray(const mgvector<long>& shapes)
{
    for (int i = 0; i < shapes.count(); i++) {
        MgShapes* s = MgShapes::fromHandle(shapes.get(i));
        MgObject::release_pointer(s);
    }
}

bool GiCoreView::submitBackDoc(GiView* view, bool changed)
{
    GcBaseView* aview = impl->_gcdoc->findView(view);
    bool ret = !aview || aview == impl->curview;
    
    if (ret) {
        if (aview) {    // set viewport from view
            impl->doc()->saveAll(NULL, aview->xform());
        }
        impl->drawing->submitBackDoc();
        if (changed) {
            static long n = 0;
            if (!giAtomicCompareAndSwap(&impl->changeCount, ++n, impl->changeCount)) {
                LOGE("Fail to set changeCount via giAtomicCompareAndSwap");
            }
        }
    }
    if (aview) {
        aview->submitBackXform();
    }
    
    return ret;
}

GiCoreView* GiCoreView::createView(GiView* view, int type)
{
    return new GiCoreView(view, type);
}

GiCoreView* GiCoreView::createMagnifierView(GiView* newview, GiCoreView* mainView,
                                            GiView* mainDevView)
{
    GiCoreView* ret = new GiCoreView(mainView);
    ret->createMagnifierView_(newview, mainDevView);
    return ret;
}

void GiCoreView::createView_(GiView* view, int type)
{
    if (view && !impl->_gcdoc->findView(view)) {
        impl->curview = new GcGraphView(impl, view);
        if (type == 0) {
            setCommand("splines");
        }
    }
}

void GiCoreView::createMagnifierView_(GiView* newview, GiView* mainView)
{
    GcGraphView* refview = dynamic_cast<GcGraphView *>(impl->_gcdoc->findView(mainView));

    if (refview && newview && !impl->_gcdoc->findView(newview)) {
        new GcMagnifierView(impl, newview, refview);
    }
}

void GiCoreView::destoryView(GiView* view)
{
    GcBaseView* aview = this ? impl->_gcdoc->findView(view) : NULL;

    if (aview && impl->_gcdoc->removeView(aview)) {
        if (impl->curview == aview) {
            impl->curview = impl->_gcdoc->firstView();
        }
        delete aview;
    }
}

int GiCoreView::setBkColor(GiView* view, int argb)
{
    GcBaseView* aview = impl->_gcdoc->findView(view);
    return aview ? aview->graph()->setBkColor(GiColor(argb)).getARGB() : 0;
}

void GiCoreView::setScreenDpi(int dpi, float factor)
{
    if (_dpi != dpi && dpi > 0) {
        _dpi = dpi;
    }
    if (GiCoreViewImpl::_factor != factor && factor > 0.1f) {
        GiCoreViewImpl::_factor = factor;
        GiGraphics::setPenWidthFactor(factor);
    }
}

bool GiCoreView::isDrawing()
{
    for (unsigned i = 0; i < sizeof(impl->gsBuf)/sizeof(impl->gsBuf[0]); i++) {
        if (impl->gsUsed[i] && impl->gsBuf[i] && impl->gsBuf[i]->isDrawing())
            return true;
    }
    return false;
}

bool GiCoreView::isZooming()
{
    return impl->curview && impl->curview->isZooming();
}

bool GiCoreView::isStopping()
{
    if (!this || !impl || impl->stopping) {
        return true;
    }
    for (unsigned i = 0; i < sizeof(impl->gsBuf)/sizeof(impl->gsBuf[0]); i++) {
        if (impl->gsUsed[i] && impl->gsBuf[i] && impl->gsBuf[i]->isStopping())
            return true;
    }
    return false;
}

int GiCoreView::stopDrawing(bool stop)
{
    int n = 0;
    
    if (impl->stopping == 0 && stop)
        giAtomicIncrement(&impl->stopping);
    else while (impl->stopping > 0 && !stop)
        giAtomicDecrement(&impl->stopping);
    
    for (unsigned i = 0; i < sizeof(impl->gsBuf)/sizeof(impl->gsBuf[0]); i++) {
        if (impl->gsBuf[i]) {
            impl->gsBuf[i]->stopDrawing(stop);
            n++;
        }
    }
    return n;
}

long GiCoreView::acquireGraphics(GiView* view)
{
    GcBaseView* aview = impl->_gcdoc->findView(view);
    if (!aview)
        return 0;
    
    GiGraphics* gs = (GiGraphics*)0;
    int i = sizeof(impl->gsBuf)/sizeof(impl->gsBuf[0]);
    
    while (--i >= 0) {
        if (!impl->gsUsed[i] && impl->gsBuf[i]) {
            if (giAtomicIncrement(&impl->gsUsed[i]) == 1) {
                gs = impl->gsBuf[i];
                aview->copyGs(gs);
                break;
            } else {
                giAtomicDecrement(&impl->gsUsed[i]);
            }
        }
    }
    if (!gs) {
        gs = new GiGraphics();
        aview->copyGs(gs);
        for (i = 0; i < (int)(sizeof(impl->gsBuf)/sizeof(impl->gsBuf[0])); i++) {
            if (!impl->gsBuf[i]) {
                if (giAtomicIncrement(&impl->gsUsed[i]) == 1) {
                    impl->gsBuf[i] = gs;
                    break;
                } else {
                    giAtomicDecrement(&impl->gsUsed[i]);
                }
            }
        }
    }
    
    return gs->toHandle();
}

void GiCoreView::releaseGraphics(long hGs)
{
    GiGraphics* gs = GiGraphics::fromHandle(hGs);
    
    if (!gs) {
        return;
    }
    for (unsigned i = 0; i < sizeof(impl->gsBuf)/sizeof(impl->gsBuf[0]); i++) {
        if (impl->gsBuf[i] == gs) {
            giAtomicDecrement(&impl->gsUsed[i]);
            return;
        }
    }
    delete gs;
}

int GiCoreView::drawAll(GiView* view, GiCanvas* canvas) {
    long doc = acquireFrontDoc();
    long hGs = acquireGraphics(view);
    int n = drawAll(doc, hGs, canvas);
    releaseDoc(doc);
    releaseGraphics(hGs);
    return n;
}

int GiCoreView::drawAppend(GiView* view, GiCanvas* canvas, int sid) {
    long doc = acquireFrontDoc();
    long hGs = acquireGraphics(view);
    int n = drawAppend(doc, hGs, canvas, sid);
    releaseDoc(doc);
    releaseGraphics(hGs);
    return n;
}

int GiCoreView::dynDraw(GiView* view, GiCanvas* canvas){
    long hShapes = acquireDynamicShapes();
    long hGs = acquireGraphics(view);
    int n = dynDraw(hShapes, hGs, canvas);
    releaseShapes(hShapes);
    releaseGraphics(hGs);
    return n;
}

int GiCoreView::drawAll(long doc, long hGs, GiCanvas* canvas)
{
    int n = -1;
    GiGraphics* gs = GiGraphics::fromHandle(hGs);
    
    if (doc && gs && gs->beginPaint(canvas)) {
        n = MgShapeDoc::fromHandle(doc)->dyndraw(isZooming() ? 2 : 0, *gs);
        gs->endPaint();
    }

    return n;
}

int GiCoreView::drawAll(const mgvector<long>& docs, long hGs, GiCanvas* canvas)
{
    int n = -1;
    GiGraphics* gs = GiGraphics::fromHandle(hGs);
    
    if (gs && gs->beginPaint(canvas)) {
        n = 0;
        for (int i = 0; i < docs.count(); i++) {
            MgShapeDoc* doc = MgShapeDoc::fromHandle(docs.get(i));
            n += doc ? doc->dyndraw(isZooming() ? 2 : 0, *gs) : 0;
        }
        gs->endPaint();
    }
    
    return n;
}

int GiCoreView::drawAppend(long doc, long hGs, GiCanvas* canvas, int sid)
{
    int n = -1;
    GiGraphics* gs = GiGraphics::fromHandle(hGs);
    
    if (doc && gs && sid && gs->beginPaint(canvas)) {
        const MgShapes* sps = MgShapeDoc::fromHandle(doc)->getCurrentShapes();
        const MgShape* sp = sps->findShape(sid);
        n = (sp && sp->draw(isZooming() ? 2 : 0, *gs, NULL, -1)) ? 1 : 0;
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
        n = MgShapes::fromHandle(hShapes)->dyndraw(isZooming() ? 2 : 0, *gs, NULL, -1);
        gs->endPaint();
    }
    
    return n;
}

int GiCoreView::dynDraw(const mgvector<long>& shapes, long hGs, GiCanvas* canvas)
{
    int n = -1;
    GiGraphics* gs = GiGraphics::fromHandle(hGs);
    
    if (gs && gs->beginPaint(canvas)) {
        n = 0;
        mgCopy(impl->motion()->d2mgs, impl->cmds()->displayMmToModel(1, gs));
        for (int i = 0; i < shapes.count(); i++) {
            MgShapes* sp = MgShapes::fromHandle(shapes.get(i));
            n += sp ? sp->dyndraw(isZooming() ? 2 : 0, *gs, NULL, -1) : 0;
        }
        gs->endPaint();
    }
    
    return n;
}

void GiCoreView::onSize(GiView* view, int w, int h)
{
    GcBaseView* aview = impl->_gcdoc->findView(view);
    if (aview) {
        aview->onSize(_dpi, w, h);
    }
}

void GiCoreView::setViewScaleRange(GiView* view, float minScale, float maxScale)
{
    GcBaseView* aview = impl->_gcdoc->findView(view);
    if (aview) {
        aview->xform()->setViewScaleRange(minScale, maxScale);
    }
}

void GiCoreView::setPenWidthRange(GiView* view, float minw, float maxw)
{
    GcBaseView* aview = impl->_gcdoc->findView(view);
    if (aview) {
        aview->graph()->setMaxPenWidth(maxw, minw);
    }
}

void GiCoreView::setGestureVelocity(GiView* view, float vx, float vy)
{
    GcBaseView* aview = impl->_gcdoc->findView(view);
    if (impl->setView(aview)) {
        impl->motion()->velocity.set(vx, vy);
    }
}

static void movePointInView(Point2d& pt, const Box2d& rect)
{
    if (!rect.contains(pt)) {
        if (pt.x < rect.xmin) {
            pt.x = rect.xmin;
        }
        else if (pt.x > rect.xmax) {
            pt.x = rect.xmax;
        }
        
        if (pt.y < rect.ymin) {
            pt.y = rect.ymin;
        }
        else if (pt.y > rect.ymax) {
            pt.y = rect.ymax;
        }
    }
}

bool GiCoreView::onGesture(GiView* view, GiGestureType type,
                           GiGestureState state, float x, float y, bool switchGesture)
{
    DrawLocker locker(impl);
    GcBaseView* aview = impl->_gcdoc->findView(view);
    bool ret = false;

    if (impl->setView(aview)) {
        impl->motion()->gestureType = type;
        impl->motion()->gestureState = (MgGestureState)state;
        impl->motion()->pressDrag = (type == kGiGesturePress && state < kGiGestureEnded);
        impl->motion()->switchGesture = switchGesture;
        impl->motion()->d2m = impl->cmds()->displayMmToModel(1, impl->motion());
        
        impl->motion()->point.set(x, y);
        float margin = impl->motion()->displayMmToModel(2);
        movePointInView(impl->motion()->point,
                        aview->xform()->getWndRect().deflate(margin));
        
        impl->motion()->pointM = impl->motion()->point * aview->xform()->displayToModel();
        impl->motion()->point2 = impl->motion()->point;
        impl->motion()->point2M = impl->motion()->pointM;

        if (state <= kGiGestureBegan) {
            impl->motion()->velocity.set(0, 0);
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
    GcBaseView* aview = impl->_gcdoc->findView(view);
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
            impl->motion()->velocity.set(0, 0);
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

bool GiCoreView::isDrawingCommand()
{
    MgCommand* cmd = impl->_cmds->getCommand();
    return cmd && cmd->isDrawingCommand();
}

bool GiCoreView::isCommand(const char* name)
{
    return impl->isCommand(name);
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
    MgJsonStorage js;
    MgStorage* s = js.storageForRead(params);
    return impl->curview && impl->_cmds->setCommand(impl->motion(), name, s);
}

bool GiCoreView::switchCommand()
{
    DrawLocker locker(impl);
    return impl->curview && impl->_cmds->switchCommand(impl->motion());
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

int GiCoreView::addShapesForTest(int n)
{
    n = RandomParam(n).addShapes(impl->shapes());
    impl->regenAll(true);
    LOGD("Add %d shapes for test", n);
    return n;
}

int GiCoreView::getShapeCount()
{
    return impl->doc()->getShapeCount();
}

int GiCoreView::getShapeCount(long doc)
{
    const MgShapeDoc* p = MgShapeDoc::fromHandle(doc);
    return p ? p->getShapeCount() : 0;
}

static void getUnlockedShapeCount_(const MgShape* sp, void* d)
{
    if (!sp->shapec()->getFlag(kMgLocked)) {
        int* n = (int*)d;
        *n = *n + 1;
    }
}

int GiCoreView::getUnlockedShapeCount()
{
    int n = 0;
    impl->shapes()->traverseByType(0, getUnlockedShapeCount_, &n);
    return n;
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
    
    impl->hideContextActions();

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
    if (impl->curview && impl->cmds()) {
        impl->getCmdSubject()->onDocLoaded(impl->motion());
    }

    return ret;
}

bool GiCoreView::saveShapes(long doc, MgStorage* s)
{
    const MgShapeDoc* p = MgShapeDoc::fromHandle(doc);
    return p && p->save(s, 0);
}

bool GiCoreView::submitDynamicShapes(GiView* view)
{
    GcBaseView* aview = impl->_gcdoc->findView(view);
    bool ret = aview && aview == impl->curview;
    
    if (ret) {
        impl->submitDynamicShapes(aview);
    }
    if (aview) {
        aview->submitBackXform();
    }
    return ret;
}

void GiCoreViewImpl::submitDynamicShapes(GcBaseView* v)
{
    MgCommand* cmd = getCommand();
    MgShapes* shapes = drawing->getBackShapes(true);
    
    if (cmd) {
        if (!cmd->gatherShapes(motion(), shapes)) {
            GiRecordCanvas canvas(shapes, v->xform(), cmd->isDrawingCommand() ? 0 : -1);
            if (v->frontGraph()->beginPaint(&canvas)) {
                mgCopy(motion()->d2mgs, cmds()->displayMmToModel(1, v->frontGraph()));
                cmd->draw(motion(), v->frontGraph());
                if (cmd->isDrawingCommand()) {
                    getCmdSubject()->drawInShapeCommand(motion(), cmd, v->frontGraph());
                }
                v->frontGraph()->endPaint();
            }
        }
        drawing->submitBackShapes();
        giAtomicIncrement(&drawCount);
    }
}

void GiCoreView::clear()
{
    loadShapes((MgStorage*)0);
}

const char* GiCoreView::getContent(long doc)
{
    const char* content = "";
    if (saveShapes(doc, impl->defaultStorage.storageForWrite())) {
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
    if (!fp) {
        LOGE("Fail to open file: %s", vgfile);
        return loadShapes(NULL, readOnly) && fp;
    }
    
    MgJsonStorage s;
    bool ret = loadShapes(s.storageForRead(fp), readOnly);

    fclose(fp);
    LOGD("loadFromFile: %d, %s", ret, vgfile);

    return ret;
}

bool GiCoreView::saveToFile(long doc, const char* vgfile, bool pretty)
{
    FILE *fp = doc ? mgopenfile(vgfile, "wt") : NULL;
    MgJsonStorage s;
    bool ret = (fp != NULL
                && saveShapes(doc, s.storageForWrite())
                && s.save(fp, pretty));
    
    if (fp) {
        fclose(fp);
        LOGD("saveToFile: %s, %d shapes", vgfile, MgShapeDoc::fromHandle(doc)->getShapeCount());
    } else {
        LOGE("Fail to open file: %s", vgfile);
    }
    
    return ret;
}

int GiCoreView::exportSVG(long doc, long hGs, const char* filename)
{
    GiSvgCanvas canvas;
    int n = -1;
    
    if (doc && impl->curview
        && canvas.open(filename, impl->curview->xform()->getWidth(),
                       impl->curview->xform()->getHeight()))
    {
        n = drawAll(doc, hGs, &canvas);
    }
    
    return canvas.close() ? n : -1;
}

int GiCoreView::exportSVG(GiView* view, const char* filename) {
    long doc = acquireFrontDoc();
    long hGs = acquireGraphics(view);
    int n = exportSVG(doc, hGs, filename);
    releaseDoc(doc);
    releaseGraphics(hGs);
    return n;
}

bool GiCoreView::zoomToInitial()
{
    bool ret = impl->doc()->zoomToInitial(impl->xform());
    if (ret) {
        impl->regenAll(false);
    }
    return ret;
}

bool GiCoreView::zoomToExtent(float margin)
{
    Box2d rect(impl->doc()->getExtent() * impl->xform()->modelToWorld());
    RECT_2D to;
    
    Box2d(impl->xform()->getWndRect()).deflate(margin).get(to);
    bool ret = impl->xform()->zoomTo(rect, &to);
    
    if (ret) {
        impl->regenAll(false);
    }
    return ret;
}

bool GiCoreView::zoomToModel(float x, float y, float w, float h, float margin)
{
    Box2d rect(Box2d(x, y, x + w, y + h) * impl->xform()->modelToWorld());
    RECT_2D to;
    
    Box2d(impl->xform()->getWndRect()).deflate(margin).get(to);
    bool ret = impl->xform()->zoomTo(rect, &to);
    
    if (ret) {
        impl->regenAll(false);
    }
    return ret;
}

bool GiCoreView::zoomPan(float dxPixel, float dyPixel, bool adjust)
{
    bool res = impl->xform()->zoomPan(dxPixel, dyPixel, adjust);
    if (res) {
        impl->regenAll(false);
    }
    return res;
}

bool GiCoreView::isZoomEnabled(GiView* view)
{
    GcBaseView* aview = impl->_gcdoc->findView(view);
    return aview && aview->isZoomEnabled();
}

void GiCoreView::setZoomEnabled(GiView* view, bool enabled)
{
    GcBaseView* aview = impl->_gcdoc->findView(view);
    if (aview) {
        aview->setZoomEnabled(enabled);
    }
}

float GiCoreView::calcPenWidth(GiView* view, float lineWidth)
{
    GcBaseView* aview = impl->_gcdoc->findView(view);
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
    DrawLocker locker(impl);
    MgShape* shape = impl->_cmds->addImageShape(impl->motion(), name, width, height);
    return shape ? shape->getID() : 0;
}

int GiCoreView::addImageShape(const char* name, float xc, float yc, float w, float h, int tag)
{
    DrawLocker locker(impl);
    MgShape* shape = impl->_cmds->addImageShape(impl->motion(), name, xc, yc, w, h, tag);
    return shape ? shape->getID() : 0;
}

bool GiCoreView::getImageSize(mgvector<float>& info, int sid)
{
    const MgShape* shape = impl->doc()->findShape(sid);
    bool ret = (info.count() >= 5 && shape && impl->curview
                && shape->shapec()->isKindOf(MgImageShape::Type()));
    
    if (ret) {
        const MgImageShape *image = (const MgImageShape*)shape->shapec();
        Box2d rect(image->getRect() * impl->curview->xform()->modelToDisplay());
        Vector2d v;
        
        v.setAngleLength(image->getAngle(), 1);
        info.set(0, image->getImageSize().x, image->getImageSize().y);
        info.set(2, rect.width(), rect.height());
        info.set(4, (v * impl->curview->xform()->modelToDisplay()).angle2());
    }
    
    return ret;
}

bool GiCoreView::hasImageShape(long doc)
{
    const MgShapeDoc* p = MgShapeDoc::fromHandle(doc);
    return p && p->getCurrentLayer()->findShapeByType(MgImageShape::Type());
}

int GiCoreView::findShapeByImageID(long doc, const char* name)
{
    const MgShapeDoc* p = MgShapeDoc::fromHandle(doc);
    const MgShape* sp = p ? MgImageShape::findShapeByImageID(p->getCurrentLayer(), name) : NULL;
    return sp ? sp->getID() : 0;
}

int GiCoreView::findShapeByTag(long doc, int tag)
{
    const MgShapeDoc* p = MgShapeDoc::fromHandle(doc);
    const MgShape* sp = p ? p->getCurrentLayer()->findShapeByTag(tag) : NULL;
    return sp ? sp->getID() : 0;
}

static void traverseImage(const MgShape* sp, void* d) {
    MgFindImageCallback* c = (MgFindImageCallback*)d;
    const MgImageShape* shape = (const MgImageShape*)sp->shapec();
    const MgShape* sp2 = sp;
    
    for (; sp2; sp2 = MgShapes::getParentShape(sp))
        sp = sp2;
    c->onFindImage(sp->getID(), shape->getName());
}

int GiCoreView::traverseImageShapes(long doc, MgFindImageCallback* c)
{
    const MgShapeDoc* p = MgShapeDoc::fromHandle(doc);
    return p ? p->getCurrentLayer()->traverseByType(MgImageShape::Type(), traverseImage, c) : 0;
}

int GiCoreView::importSVGPath(long shapes, int sid, const char* d)
{
    MgShapes* splist = MgShapes::fromHandle(shapes);
    const MgShape* oldsp = splist ? splist->findShape(sid) : NULL;
    int ret = 0;
    
    if (oldsp && oldsp->shapec()->isKindOf(MgPathShape::Type()) && d) {
        MgShape* newsp = oldsp->cloneShape();
        if (((MgPathShape*)newsp->shape())->importSVGPath(d)) {
            splist->updateShape(oldsp, newsp);
            ret = sid;
        } else {
            newsp->release();
        }
    } else if (splist) {
        MgShapeT<MgPathShape> sp;
        if (sp._shape.importSVGPath(d)) {
            ret = splist->addShape(sp)->getID();
        }
    }
    
    return ret;
}

int GiCoreView::exportSVGPath(long shapes, int sid, char* buf, int size)
{
    MgShapes* splist = MgShapes::fromHandle(shapes);
    const MgShape* sp = splist ? splist->findShape(sid) : NULL;
    int ret = 0;
    
    if (sp && sp->shapec()->isKindOf(MgPathShape::Type())) {
        ret = ((const MgPathShape*)sp->shapec())->exportSVGPath(buf, size);
    } else if (sp) {
        MgPath path;
        sp->shapec()->output(path);
        ret = MgPathShape::exportSVGPath(path, buf, size);
    }
    
    return ret;
}

bool GiCoreView::getDisplayExtent(mgvector<float>& box)
{
    bool ret = box.count() == 4 && impl->curview;
    if (ret) {
        Box2d rect(impl->doc()->getExtent());
        rect *= impl->curview->xform()->modelToDisplay();
        box.set(0, rect.xmin, rect.ymin);
        box.set(2, rect.xmax, rect.ymax);
    }
    return ret;
}

bool GiCoreView::getDisplayExtent(long hDoc, long hGs, mgvector<float>& box)
{
    const MgShapeDoc* doc = MgShapeDoc::fromHandle(hDoc);
    const GiGraphics* gs = GiGraphics::fromHandle(hGs);
    bool ret = box.count() == 4 && doc && gs;
    
    if (ret) {
        Box2d rect(doc->getExtent());
        rect *= gs->xf().modelToDisplay();
        box.set(0, rect.xmin, rect.ymin);
        box.set(2, rect.xmax, rect.ymax);
    }
    return ret;
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
    const MgShape* shape = impl->doc()->findShape(shapeId);
    bool ret = box.count() == 4 && shape && impl->curview;
    
    if (ret) {
        Box2d rect(shape->shapec()->getExtent());
        rect *= impl->curview->xform()->modelToDisplay();
        box.set(0, rect.xmin, rect.ymin);
        box.set(2, rect.xmax, rect.ymax);
    }
    return ret;
}

bool GiCoreView::getBoundingBox(long hDoc, long hGs, mgvector<float>& box, int shapeId)
{
    const MgShapeDoc* doc = MgShapeDoc::fromHandle(hDoc);
    const GiGraphics* gs = GiGraphics::fromHandle(hGs);
    const MgShape* shape = doc ? doc->findShape(shapeId) : NULL;
    bool ret = box.count() == 4 && shape && gs;
    
    if (ret) {
        Box2d rect(shape->shapec()->getExtent());
        rect *= gs->xf().modelToDisplay();
        box.set(0, rect.xmin, rect.ymin);
        box.set(2, rect.xmax, rect.ymax);
    }
    return ret;
}

bool GiCoreView::displayToModel(mgvector<float>& d)
{
    bool ret = impl->curview && (d.count() == 2 || d.count() == 4);
    
    if (ret) {
        if (d.count() == 2) {
            Point2d pt(d.get(0), d.get(1));
            pt *= impl->curview->xform()->displayToModel();
            d.set(0, pt.x, pt.y);
        }
        else {
            Box2d rect(d.get(0), d.get(1), d.get(2), d.get(3));
            rect *= impl->curview->xform()->displayToModel();
            d.set(0, rect.xmin, rect.ymin);
            d.set(2, rect.xmax, rect.ymax);
        }
    }
    return ret;
}

bool GiCoreViewImpl::gestureToCommand()
{
    MgCommand* cmd = _cmds->getCommand();
    bool ret = false;
    
    if (!cmd || _motion.gestureState == kMgGestureCancel) {
        return cmd && cmd->cancel(&_motion);
    }
    if (recorder(false) && recorder(false)->isPlaying()) {
        return true;
    }
    if (_motion.gestureState == kMgGesturePossible
        && _motion.gestureType != kGiTwoFingersMove) {
        return true;
    }
    
    if (!getCmdSubject()->onPreGesture(&_motion)) {
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
        ret = _motion.gestureState > kMgGestureBegan || cmd->longPress(&_motion);
        break;
    }

    getCmdSubject()->onPostGesture(&_motion);
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

int GiCoreViewImpl::getOptionInt(const char* group, const char* name, int defValue)
{
    int ret = defValue;
    OPT_GROUP::const_iterator it = options.find(group);
    
    if (it != options.end()) {
        OPT_KEY_VALUE::const_iterator kv = it->second.find(name);
        
        if (kv != it->second.end()
            && MgJsonStorage::parseInt(kv->second.c_str(), defValue)) {
            ret = defValue;
        }
    }
    
    return ret;
}

float GiCoreViewImpl::getOptionFloat(const char* group, const char* name, float defValue)
{
    int ret = defValue;
    OPT_GROUP::const_iterator it = options.find(group);
    
    if (it != options.end()) {
        OPT_KEY_VALUE::const_iterator kv = it->second.find(name);
        
        if (kv != it->second.end()
            && MgJsonStorage::parseFloat(kv->second.c_str(), defValue)) {
            ret = defValue;
        }
    }
    
    return ret;
}

void GiCoreViewImpl::setOptionInt(const char* group, const char* name, int value)
{
    std::stringstream ss;
    ss << value;
    options[group][name] = ss.str();
}

void GiCoreViewImpl::setOptionFloat(const char* group, const char* name, float value)
{
    std::stringstream ss;
    ss << value;
    options[group][name] = ss.str();
}

void GiCoreView::setOption(const char* group, const char* name, const char* text)
{
    if (!group) {
        impl->getOptions().clear();
    } else if (!name) {
        impl->getOptions()[group].clear();
    } else if (text) {
        impl->getOptions()[group][name] = text;
    } else {
        impl->getOptions()[group].erase(name);
    }
}

int GiCoreView::traverseOptions(MgOptionCallback* c)
{
    int ret = 0;
    GiCoreViewImpl::OPT_GROUP::const_iterator it = impl->getOptions().begin();
    
    for (; it != impl->getOptions().end(); ++it) {
        for (GiCoreViewImpl::OPT_KEY_VALUE::const_iterator kv = it->second.begin();
             kv != it->second.end(); ++kv, ++ret) {
            c->onGetOption(it->first.c_str(), kv->first.c_str(), kv->second.c_str());
        }
    }
    
    return ret;
}
