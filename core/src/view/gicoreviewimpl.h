//! \file gicoreviewimpl.h
//! \brief 定义GiCoreView实现类 GiCoreViewImpl
// Copyright (c) 2012-2013, https://github.com/rhcad/touchvg

#include "GcShapeDoc.h"
#include "GcMagnifierView.h"
#include "mgcmdmgr.h"
#include "mgcmdmgrfactory.h"
#include "cmdsubject.h"
#include "mgbasicspreg.h"
#include "mgjsonstorage.h"
#include "mgstorage.h"
#include "recordshapes.h"
#include "girecordshape.h"
#include "mgshapet.h"
#include "cmdbasic.h"
#include "mglayer.h"
#include "mglog.h"
#include <map>

#define CALL_VIEW(func) if (curview) curview->func
#define CALL_VIEW2(func, v) curview ? curview->func : v

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
    void release() { if (giAtomicDecrement(&_refcount) == 0) delete this; }
    void addRef() { giAtomicIncrement(&_refcount); }
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
    static float    _factor;
    GcShapeDoc*     _gcdoc;
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
    MgShapes*       dynShapesFront;
    MgShapes*       dynShapesPlay;
    MgShapes*       dynShapesPlayFront;
    MgShapeDoc*     docPlay;
    volatile long   startPauseTick;
    MgRecordShapes* recorder[2];
    
    GiGraphics*     gsBuf[20];
    volatile long   gsUsed[20];
    volatile long   stopping;
    
public:
    GiCoreViewImpl() : curview(NULL), refcount(1), gestureHandler(0)
        , regenPending(-1), appendPending(-1), redrawPending(-1)
        , changeCount(0), drawCount(0), dynShapesFront(NULL)
        , dynShapesPlay(NULL), dynShapesPlayFront(NULL), docPlay(NULL)
        , startPauseTick(0), stopping(0)
    {
        memset(&gsBuf, 0, sizeof(gsBuf));
        memset((void*)&gsUsed, 0, sizeof(gsUsed));
        
        _motion.view = this;
        _motion.gestureType = 0;
        _motion.gestureState = kMgGesturePossible;
        recorder[0] = recorder[1] = NULL;
        _gcdoc = new GcShapeDoc();
        _cmds = MgCmdManagerFactory::create();
        
        MgBasicShapes::registerShapes(this);
        MgBasicCommands::registerCmds(this);
        MgShapeT<MgRecordShape>::registerCreator(this);
    }
    
    ~GiCoreViewImpl() {
        for (unsigned i = 0; i < sizeof(gsBuf)/sizeof(gsBuf[0]); i++) {
            delete gsBuf[i];
        }
        MgObject::release_pointer(dynShapesFront);
        MgObject::release_pointer(dynShapesPlay);
        MgObject::release_pointer(dynShapesPlayFront);
        MgObject::release_pointer(docPlay);
        _cmds->release();
        delete _gcdoc;
    }
    
    MgMotion* motion() { return &_motion; }
    MgCmdManager* cmds() const { return _cmds; }
    GcShapeDoc* document() const { return _gcdoc; }
    MgShapeDoc* doc() const { return _gcdoc->backDoc(); }
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
    bool isCommand(const char* name) { return name && strcmp(getCommandName(), name) == 0; }
    
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
                    && shape->getParent()->findShape(shape->getID()) == shape
                    && !shape->shapec()->getFlag(kMgShapeLocked));
        if (ret) {
            getCmdSubject()->onShapeDeleted(motion(), shape);
            ret = shape->getParent()->removeShape(shape->getID());
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
        
        if (!curview || (n > 0 && motion()->pressDrag && isContextActionsVisible())) {
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
    
    void redraw(bool changed = true) {
        if (redrawPending >= 0) {
            redrawPending += changed ? 100 : 1;
        }
        else {
            CALL_VIEW(deviceView()->redraw(changed));
        }
    }
    
    void regenAll(bool changed) {
        bool apply = regenPending != 0;
        
        if (regenPending >= 0) {
            regenPending += changed ? 100 : 1;
        }
        if (apply) {
            CALL_VIEW(deviceView()->regenAll(changed));
            if (changed) {
                for (int i = 0; i < _gcdoc->getViewCount(); i++) {
                    if (_gcdoc->getView(i) != curview)
                        _gcdoc->getView(i)->deviceView()->regenAll(changed);
                }
                CALL_VIEW(deviceView()->contentChanged());
            }
            else {
                for (int i = 0; i < _gcdoc->getViewCount(); i++) {
                    if (_gcdoc->getView(i) != curview)
                        _gcdoc->getView(i)->deviceView()->redraw(changed);
                }
            }
        }
    }
    
    void regenAppend(int sid) {
        bool apply = appendPending != 0;
        
        if (appendPending >= 0 && sid) {
            if (appendPending == 0 || appendPending == sid) {
                appendPending = sid;
            }
            else if (appendPending > 0 && appendPending != sid) {
                regenAll(true);
            }
        }
        if (apply) {
            sid = sid ? sid : (int)appendPending;
            
            CALL_VIEW(deviceView()->regenAppend(sid));
            for (int i = 0; i < _gcdoc->getViewCount(); i++) {
                if (_gcdoc->getView(i) != curview)
                    _gcdoc->getView(i)->deviceView()->regenAppend(sid);
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
    void submitDynamicShapes(GcBaseView* v);
    
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
        Vector2d off(moveActionsInView(rect, 16 * _factor));
        
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
    
    Vector2d moveActionsInView(Box2d& rect, float btnHalfW)
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
            _impl->redraw(redrawPending >= 100);
        }
    }
};
