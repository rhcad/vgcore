// mgshapes.cpp
// Copyright (c) 2004-2013, Zhang Yungui
// License: LGPL, https://github.com/rhcad/touchvg

#include "mgstorage.h"
#include "mgspfactory.h"
#include "mglog.h"
#include "mgcomposite.h"
#include <list>
#include <map>

struct MgShapes::I
{
    typedef std::list<MgShape*> Container;
    typedef Container::const_iterator citerator;
    typedef Container::iterator iterator;
    typedef std::map<int, MgShape*>  ID2SHAPE;
    
    Container   shapes;
    ID2SHAPE    id2shape;
    MgObject*   owner;
    int         index;
    int         newShapeID;
    volatile long refcount;
    
    MgShape* findShape(int sid) const;
    int getNewID(int sid);
    
    iterator findPosition(int sid) {
        iterator it = shapes.begin();
        for (; it != shapes.end() && (*it)->getID() != sid; ++it) ;
        return it;
    }
};

MgShapes* MgShapes::create(MgObject* owner, int index)
{
    return new MgShapes(owner, owner ? index : -1);
}

//static volatile long _n = 0;

MgShapes::MgShapes(MgObject* owner, int index)
{
    //LOGD("+MgShapes %ld", giAtomicIncrement(&_n));
    im = new I();
    im->owner = owner;
    im->index = index;
    im->newShapeID = 1;
    im->refcount = 1;
}

MgShapes::~MgShapes()
{
    clear();
    delete im;
    //LOGD("-MgShapes %ld", giAtomicDecrement(&_n));
}

void MgShapes::release()
{
    if (giAtomicDecrement(&im->refcount) == 0)
        delete this;
}

void MgShapes::addRef()
{
    giAtomicIncrement(&im->refcount);
}

MgObject* MgShapes::clone() const
{
    MgShapes *p = new MgShapes(im->owner, im->index);
    p->copy(*this);
    return p;
}

void MgShapes::copy(const MgObject&)
{
}

MgShapes* MgShapes::shallowCopy() const
{
    MgShapes *p = new MgShapes(im->owner, im->index);
    p->copyShapes(this, false);
    return p;
}

int MgShapes::copyShapes(const MgShapes* src, bool deeply, bool needClear)
{
    if (needClear)
        clear();
    
    int ret = 0;
    MgShapeIterator it(src);
    
    while (MgShape* sp = const_cast<MgShape*>(it.getNext())) {
        if (deeply) {
            ret += addShape(*sp) ? 1 : 0;
        } else {
            sp->addRef();
            im->shapes.push_back(sp);
            im->id2shape[sp->getID()] = sp;
            ret++;
        }
    }
    
    return ret;
}

bool MgShapes::equals(const MgObject& src) const
{
    bool ret = false;
    
    if (src.isKindOf(Type())) {
        const MgShapes& _src = (const MgShapes&)src;
        ret = (im->shapes == _src.im->shapes);
    }
    
    return ret;
}

void MgShapes::clear()
{
    for (I::iterator it = im->shapes.begin(); it != im->shapes.end(); ++it) {
        (*it)->release();
    }
    im->shapes.clear();
    im->id2shape.clear();
}

void MgShapes::clearCachedData()
{
    for (I::iterator it = im->shapes.begin(); it != im->shapes.end(); ++it) {
        (*it)->shape()->clearCachedData();
    }
}

MgObject* MgShapes::getOwner() const
{
    return this ? im->owner : NULL;
}

int MgShapes::getIndex() const
{
    return im->index;
}

bool MgShapes::updateShape(MgShape* shape, bool force)
{
    if (shape && (force || !shape->getParent() || shape->getParent() == this)) {
        I::iterator it = im->findPosition(shape->getID());
        if (it != im->shapes.end()) {
            shape->shape()->resetChangeCount((*it)->shapec()->getChangeCount() + 1);
            (*it)->release();
            *it = shape;
            shape->setParent(this, shape->getID());
            im->id2shape[shape->getID()] = shape;
            return true;
        }
    }
    return false;
}

void MgShapes::transform(const Matrix2d& mat)
{
    for (I::iterator it = im->shapes.begin(); it != im->shapes.end(); ++it) {
        MgShape* newsp = (*it)->cloneShape();
        newsp->shape()->transform(mat);
        if (!updateShape(newsp, true))
            MgObject::release_pointer(newsp);
    }
}

MgShape* MgShapes::cloneShape(int sid) const
{
    const MgShape* p = im->findShape(sid);
    return p ? p->cloneShape() : NULL;
}

MgShape* MgShapes::addShape(const MgShape& src)
{
    MgShape* p = src.cloneShape();
    if (p) {
        p->setParent(this, im->getNewID(src.getID()));
        im->shapes.push_back(p);
        im->id2shape[p->getID()] = p;
    }
    return p;
}

bool MgShapes::addShapeDirect(MgShape* shape, bool force)
{
    if (shape && (force || !shape->getParent() || shape->getParent() == this)) {
        shape->setParent(this, im->getNewID(0));
        im->shapes.push_back(shape);
        im->id2shape[shape->getID()] = shape;
        return true;
    }
    return false;
}

MgShape* MgShapes::addShapeByType(MgShapeFactory* factory, int type)
{
    MgShape* p = factory->createShape(type);
    if (p) {
        p->setParent(this, im->getNewID(0));
        im->shapes.push_back(p);
        im->id2shape[p->getID()] = p;
    }
    return p;
}

bool MgShapes::removeShape(int sid)
{
    I::iterator it = im->findPosition(sid);
    
    if (it != im->shapes.end()) {
        MgShape* shape = *it;
        im->shapes.erase(it);
        im->id2shape.erase(shape->getID());
        shape->release();
        return true;
    }
    
    return false;
}

bool MgShapes::moveShapeTo(int sid, MgShapes* dest)
{
    I::iterator it = im->findPosition(sid);
    
    if (dest && dest != this && it != im->shapes.end()) {
        MgShape* newsp = (*it)->cloneShape();
        newsp->setParent(dest, dest->im->getNewID(newsp->getID()));
        dest->im->shapes.push_back(newsp);
        dest->im->id2shape[newsp->getID()] = newsp;
        
        return removeShape(sid);
    }
    
    return false;
}

void MgShapes::copyShapesTo(MgShapes* dest) const
{
    if (dest && dest != this) {
        for (I::iterator it = im->shapes.begin(); it != im->shapes.end(); ++it) {
            MgShape* newsp = (*it)->cloneShape();
            newsp->setParent(dest, dest->im->getNewID(newsp->getID()));
            dest->im->shapes.push_back(newsp);
            dest->im->id2shape[newsp->getID()] = newsp;
        }
    }
}

bool MgShapes::bringToFront(int sid)
{
    I::iterator it = im->findPosition(sid);
    
    if (it != im->shapes.end()) {
        MgShape* shape = *it;
        im->shapes.erase(it);
        im->shapes.push_back(shape);
        return true;
    }
    
    return false;
}

int MgShapes::getShapeCount() const
{
    return this ? (int)im->shapes.size() : 0;
}

void MgShapes::freeIterator(void*& it) const
{
    if (it) {
        delete (I::citerator*)it;
        it = NULL;
    }
}

const MgShape* MgShapes::getFirstShape(void*& it) const
{
    if (!this || im->shapes.empty()) {
        it = NULL;
        return NULL;
    }
    it = (void*)(new I::citerator(im->shapes.begin()));
    return im->shapes.empty() ? NULL : im->shapes.front();
}

const MgShape* MgShapes::getNextShape(void*& it) const
{
    I::citerator* pit = (I::citerator*)it;
    if (pit && *pit != im->shapes.end()) {
        ++(*pit);
        if (*pit != im->shapes.end())
            return *(*pit);
    }
    return NULL;
}

const MgShape* MgShapes::getHeadShape() const
{
    return (!this || im->shapes.empty()) ? NULL : im->shapes.front();
}

const MgShape* MgShapes::getLastShape() const
{
    return (!this || im->shapes.empty()) ? NULL : im->shapes.back();
}

const MgShape* MgShapes::findShape(int sid) const
{
    return im->findShape(sid);
}

const MgShape* MgShapes::findShapeByTag(int tag) const
{
    if (!this || 0 == tag)
        return NULL;
    for (I::citerator it = im->shapes.begin(); it != im->shapes.end(); ++it) {
        if ((*it)->getTag() == tag)
            return *it;
    }
    return NULL;
}

const MgShape* MgShapes::findShapeByType(int type) const
{
    if (!this || 0 == type)
        return NULL;
    for (I::citerator it = im->shapes.begin(); it != im->shapes.end(); ++it) {
        if ((*it)->shapec()->getType() == type)
            return *it;
    }
    return NULL;
}

int MgShapes::traverseByType(int type, void (*c)(const MgShape*, void*), void* d)
{
    int count = 0;
    
    for (I::citerator it = im->shapes.begin(); it != im->shapes.end(); ++it) {
        const MgBaseShape* shape = (*it)->shapec();
        if (shape->isKindOf(type)) {
            (*c)(*it, d);
            count++;
        } else if (shape->isKindOf(MgComposite::Type())) {
            const MgComposite *composite = (const MgComposite *)shape;
            count += composite->shapes()->traverseByType(type, c, d);
        }
    }
    
    return count;
}

const MgShape* MgShapes::getParentShape(const MgShape* shape)
{
    const MgComposite *composite = NULL;
    
    if (shape && shape->getParent()
        && shape->getParent()->getOwner()->isKindOf(MgComposite::Type())) {
        composite = (const MgComposite *)(shape->getParent()->getOwner());
    }
    return composite ? composite->getOwnerShape() : NULL;
}

Box2d MgShapes::getExtent() const
{
    Box2d extent;
    for (I::citerator it = im->shapes.begin(); it != im->shapes.end(); ++it) {
        extent.unionWith((*it)->shapec()->getExtent());
    }
    
    return extent;
}

const MgShape* MgShapes::hitTest(const Box2d& limits, MgHitResult& res, Filter filter) const
{
    const MgShape* retshape = NULL;
    
    res.dist = _FLT_MAX;
    for (I::citerator it = im->shapes.begin(); it != im->shapes.end(); ++it) {
        const MgBaseShape* shape = (*it)->shapec();
        Box2d extent(shape->getExtent());
        
        if (!shape->getFlag(kMgShapeLocked)
            && extent.isIntersect(limits)
            && (!filter || filter(*it))) {
            MgHitResult tmpRes;
            float  tol = (!(*it)->hasFillColor() ? limits.width() / 2
                          : mgMax(extent.width(), extent.height()));
            float  dist = shape->hitTest(limits.center(), tol, tmpRes);
            
            if (res.dist > dist - _MGZERO) {     // 让末尾图形优先选中
                res = tmpRes;
                res.dist = dist;
                retshape = *it;
            }
        }
    }
    
    return retshape;
}

int MgShapes::draw(GiGraphics& gs, const GiContext *ctx) const
{
    return dyndraw(0, gs, ctx, -1);
}

int MgShapes::dyndraw(int mode, GiGraphics& gs, const GiContext *ctx, int segment) const
{
    Box2d clip(gs.getClipModel());
    int count = 0;
    
    for (I::citerator it = im->shapes.begin(); it != im->shapes.end() && !gs.isStopping(); ++it) {
        const MgShape* sp = *it;
        if (sp->shapec()->getExtent().isIntersect(clip)) {
            if (sp->draw(mode, gs, ctx, segment))
                count++;
        }
    }
    
    return count;
}

bool MgShapes::save(MgStorage* s, int startIndex) const
{
    bool ret = false;
    Box2d rect;
    int index = 0;
    
    if (s && s->writeNode("shapes", im->index, false)) {
        ret = saveExtra(s);
        rect = getExtent();
        s->writeFloatArray("extent", &rect.xmin, 4);
        s->writeInt("count", (int)im->shapes.size() - startIndex);
        
        for (I::citerator it = im->shapes.begin();
             ret && it != im->shapes.end(); ++it, ++index)
        {
            if (index < startIndex)
                continue;
            ret = saveShape(s, *it, index - startIndex);
        }
        s->writeNode("shapes", im->index, true);
    }
    
    return ret;
}

bool MgShapes::saveShape(MgStorage* s, const MgShape* shape, int index) const
{
    bool ret = shape && s->writeNode("shape", index, false);
    
    if (ret) {
        s->writeInt("type", shape->getType() & 0xFFFF);
        s->writeInt("id", shape->getID());
        
        Box2d rect(shape->shapec()->getExtent());
        s->writeFloatArray("extent", &rect.xmin, 4);
        
        ret = shape->save(s);
        s->writeNode("shape", index, true);
    }
    
    return ret;
}

int MgShapes::load(MgShapeFactory* factory, MgStorage* s, bool addOnly)
{
    Box2d rect;
    int index = 0, count = 0;
    bool ret = s && s->readNode("shapes", im->index, false);
    
    if (ret) {
        if (!addOnly)
            clear();
        
        ret = loadExtra(s);
        s->readFloatArray("extent", &rect.xmin, 4, false);
        int n = s->readInt("count", 0);
        
        for (; ret && s->readNode("shape", index, false); n--) {
            const int type = s->readInt("type", 0);
            const int sid = s->readInt("id", 0);
            s->readFloatArray("extent", &rect.xmin, 4, false);
            
            const MgShape* oldsp = addOnly && sid ? findShape(sid) : NULL;
            MgShape* newsp = factory->createShape(type);
            
            if (oldsp && oldsp->shapec()->getType() != type) {
                oldsp = NULL;
            }
            if (newsp) {
                newsp->setParent(this, oldsp ? sid : im->getNewID(sid));
                newsp->shape()->setExtent(rect);
                ret = newsp->load(factory, s);
                if (ret) {
                    count++;
                    newsp->shape()->setFlag(kMgClosed, newsp->shape()->isClosed());
                    im->id2shape[newsp->getID()] = newsp;
                    if (oldsp) {
                        updateShape(newsp);
                    }
                    else {
                        im->shapes.push_back(newsp);
                    }
                }
                else {
                    newsp->release();
                    LOGE("Fail to load shape (id=%d, type=%d)", sid, type);
                }
            }
            s->readNode("shape", index++, true);
        }
        s->readNode("shapes", im->index, true);
    }
    else if (s && im->index == 0) {
        s->setError("No shapes node.");
    }
    
    return ret ? count : (count > 0 ? -count : -1);
}

void MgShapes::setNewShapeID(int sid)
{
    im->newShapeID = sid;
}

MgShape* MgShapes::I::findShape(int sid) const
{
    if (!this || 0 == sid)
        return NULL;
    ID2SHAPE::const_iterator it = id2shape.find(sid);
    return it != id2shape.end() ? it->second : NULL;
}

int MgShapes::I::getNewID(int sid)
{
    if (0 == sid || findShape(sid)) {
        while (findShape(newShapeID))
            newShapeID++;
        sid = newShapeID++;
    }
    return sid;
}
