//! \file gicoreview.cpp
//! \brief 实现内核视图类 GiCoreView
// Copyright (c) 2012-2013, https://github.com/rhcad/touchvg

#include "gicoreview.h"
#include "gicoreviewimpl.h"
#include "RandomShape.h"
#include "mgselect.h"
#include "mgbasicsp.h"
#include "girecordcanvas.h"
#include "svgcanvas.h"
#include "../corever.h"

static int _dpi = 96;
float GiCoreViewImpl::_factor = 1.0f;

// GcBaseView
//

GcBaseView::GcBaseView(MgView* mgview, GiView *view)
    : _mgview(mgview), _view(view)
{
    mgview->document()->addView(this);
    LOGD("View %p created", this);
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
    LOGD("GiCoreView %p created, refcount=%ld", this, impl->refcount);
}

GiCoreView::~GiCoreView()
{
    LOGD("GiCoreView %p destroyed, %ld", this, impl->refcount);
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
    if (!impl->_gcdoc->frontDoc())
        return 0;
    impl->_gcdoc->frontDoc()->addRef();
    return impl->_gcdoc->frontDoc()->toHandle();
}

void MgCoreView::releaseDoc(long hDoc)
{
    MgShapeDoc* doc = MgShapeDoc::fromHandle(hDoc);
    MgObject::release_pointer(doc);
}

bool GiCoreView::submitBackDoc(GiView* view)
{
    GcBaseView* aview = impl->_gcdoc->findView(view);
    bool ret = aview && aview == impl->curview && !isPlaying();
    
    if (ret) {
        impl->doc()->saveAll(NULL, aview->xform());  // set viewport from view
        impl->_gcdoc->submitBackDoc(impl->doc());
        giAtomicIncrement(&impl->changeCount);
    }
    if (aview) {
        aview->submitBackXform();
    }
    
    return ret;
}

long GiCoreView::acquireDynamicShapes()
{
    if (!impl->dynShapesFront)
        return 0;
    impl->dynShapesFront->addRef();
    return impl->dynShapesFront->toHandle();
}

void MgCoreView::releaseShapes(long hShapes)
{
    MgShapes* p = MgShapes::fromHandle(hShapes);
    MgObject::release_pointer(p);
}

GiCoreView* GiCoreView::createView(GiView* view, int type)
{
    GiCoreView* ret = new GiCoreView();
    ret->createView_(view, type);
    return ret;
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
    long hDoc = acquireFrontDoc();
    long hGs = acquireGraphics(view);
    int n = drawAll(hDoc, hGs, canvas);
    releaseDoc(hDoc);
    releaseGraphics(hGs);
    return n;
}

int GiCoreView::drawAppend(GiView* view, GiCanvas* canvas, int sid) {
    long hDoc = acquireFrontDoc();
    long hGs = acquireGraphics(view);
    int n = drawAppend(hDoc, hGs, canvas, sid);
    releaseDoc(hDoc);
    releaseGraphics(hGs);
    return n;
}

int GiCoreView::dynDraw(GiView* view, GiCanvas* canvas){
    long hShapes = acquireDynamicShapes();
    long hGs = acquireGraphics(view);
    int n = dynDraw(hShapes, hGs, canvas, 0);
    releaseShapes(hShapes);
    releaseGraphics(hGs);
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

int GiCoreView::dynDraw(long hShapes, long hGs, GiCanvas* canvas, const mgvector<int>& exts)
{
    int n = -1;
    GiGraphics* gs = GiGraphics::fromHandle(hGs);
    
    if (hShapes && gs && gs->beginPaint(canvas)) {
        mgCopy(impl->motion()->d2mgs, impl->cmds()->displayMmToModel(1, gs));
        n = MgShapes::fromHandle(hShapes)->draw(*gs);
        for (int i = 0; i < exts.count(); i++) {
            MgShapes* extShapes = MgShapes::fromHandle(exts.get(i));
            n += extShapes ? extShapes->draw(*gs) : 0;
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

void GiCoreView::setPenWidthRange(GiView* view, float minw, float maxw)
{
    GcBaseView* aview = impl->_gcdoc->findView(view);
    if (aview) {
        aview->graph()->setMaxPenWidth(maxw, minw);
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

int GiCoreView::getShapeCount(long hDoc)
{
    const MgShapeDoc* doc = MgShapeDoc::fromHandle(hDoc);
    return doc ? doc->getShapeCount() : 0;
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

bool GiCoreView::submitDynamicShapes(GiView* view)
{
    GcBaseView* aview = impl->_gcdoc->findView(view);
    bool ret = (aview && aview == impl->curview && !isPlaying());
    
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
    
    MgObject::release_pointer(dynShapesFront);
    if (cmd) {
        dynShapesFront = MgShapes::create();
        if (!cmd->gatherShapes(motion(), dynShapesFront)) {
            GiRecordCanvas canvas(dynShapesFront, v->xform(),
                                  cmd->isDrawingCommand() ? 0 : -1);
            if (v->frontGraph()->beginPaint(&canvas)) {
                mgCopy(motion()->d2mgs, cmds()->displayMmToModel(1, v->frontGraph()));
                cmd->draw(motion(), v->frontGraph());
                if (cmd->isDrawingCommand()) {
                    getCmdSubject()->drawInShapeCommand(motion(), cmd, v->frontGraph());
                }
                v->frontGraph()->endPaint();
            }
        }
        giAtomicIncrement(&drawCount);
    }
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
    if (!fp) {
        LOGE("Fail to open file: %s", vgfile);
        return loadShapes(NULL, readOnly) && fp;
    }
    
    MgJsonStorage s;
    bool ret = loadShapes(s.storageForRead(fp), readOnly);

    if (fp) {
        fclose(fp);
        LOGD("loadFromFile: %d, %s", ret, vgfile);
    }

    return ret;
}

bool GiCoreView::saveToFile(long hDoc, const char* vgfile, bool pretty)
{
    FILE *fp = hDoc ? mgopenfile(vgfile, "wt") : NULL;
    MgJsonStorage s;
    bool ret = (fp != NULL
                && saveShapes(hDoc, s.storageForWrite())
                && s.save(fp, pretty));
    
    if (fp) {
        fclose(fp);
        LOGD("saveToFile: %s", vgfile);
    } else {
        LOGE("Fail to open file: %s", vgfile);
    }
    
    return ret;
}

int GiCoreView::exportSVG(long hDoc, long hGs, const char* filename)
{
    GiSvgCanvas canvas;
    int n = -1;
    
    if (hDoc && impl->curview
        && canvas.open(filename, impl->curview->xform()->getWidth(),
                       impl->curview->xform()->getHeight()))
    {
        n = drawAll(hDoc, hGs, &canvas);
    }
    
    return canvas.close() ? n : -1;
}

int GiCoreView::exportSVG(GiView* view, const char* filename) {
    long hDoc = acquireFrontDoc();
    long hGs = acquireGraphics(view);
    int n = exportSVG(hDoc, hGs, filename);
    releaseDoc(hDoc);
    releaseGraphics(hGs);
    return n;
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

int GiCoreView::addImageShape(const char* name, float xc, float yc, float w, float h)
{
    DrawLocker locker(impl);
    MgShape* shape = impl->_cmds->addImageShape(impl->motion(), name, xc, yc, w, h);
    return shape ? shape->getID() : 0;
}

bool GiCoreView::hasImageShape(long hDoc)
{
    const MgShapeDoc* doc = MgShapeDoc::fromHandle(hDoc);
    return doc && doc->getCurrentLayer()->findShapeByType(MgImageShape::Type());
}

int GiCoreView::findShapeByImageID(long hDoc, const char* name)
{
    const MgShapeDoc* doc = MgShapeDoc::fromHandle(hDoc);
    const MgShape* sp = doc ? MgImageShape::findShapeByImageID(doc->getCurrentLayer(), name) : NULL;
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

int GiCoreView::traverseImageShapes(long hDoc, MgFindImageCallback* c)
{
    const MgShapeDoc* doc = MgShapeDoc::fromHandle(hDoc);
    return doc ? doc->getCurrentLayer()->traverseByType(MgImageShape::Type(), traverseImage, c) : 0;
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

bool GiCoreViewImpl::gestureToCommand()
{
    MgCommand* cmd = _cmds->getCommand();
    bool ret = false;
    
    if (!cmd || _motion.gestureState == kMgGestureCancel) {
        return cmd && cmd->cancel(&_motion);
    }
    if (recorder[1] && recorder[1]->isPlaying()
        && _motion.gestureState > kMgGesturePossible) {
        return true;
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
