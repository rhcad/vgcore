//! \file gicorerecord.cpp
//! \brief 实现内核视图类 GiCoreView 的录制功能
// Copyright (c) 2012-2013, https://github.com/rhcad/touchvg

#include "gicoreview.h"
#include "gicoreviewimpl.h"
#include <algorithm>

long GiCoreView::getRecordTick(bool forUndo, long curTick)
{
    MgRecordShapes* recorder = impl->recorder[forUndo ? 0 : 1];
    if (!recorder)
        return 0;
    long tick = recorder->getCurrentTick(curTick);
    long pauseTick = curTick - impl->startPauseTick;
    return isPaused() && tick > pauseTick ? tick - pauseTick : tick;
}

bool GiCoreView::startRecord(const char* path, long doc, bool forUndo,
                             long curTick, MgStringCallback* c)
{
    MgRecordShapes*& recorder = impl->recorder[forUndo ? 0 : 1];
    if (recorder || !path)
        return false;
    
    recorder = new MgRecordShapes(path, MgShapeDoc::fromHandle(doc), forUndo, curTick);
    if (isPlaying() || forUndo) {
        return true;
    }
    
    if (!saveToFile(doc, recorder->getFileName().c_str(), VG_PRETTY)) {
        return false;
    }
    
    if (c) {
        c->onGetString(recorder->getFileName().c_str());
    }
    return true;
}

void GiCoreView::stopRecord(bool forUndo)
{
    MgRecordShapes*& recorder = impl->recorder[forUndo ? 0 : 1];
    
    if (recorder) {
        delete recorder;
        recorder = NULL;
    }
    if (!forUndo && impl->play.playing) {
        impl->play.playing->clear();
    }
}

bool GiCoreView::recordShapes(bool forUndo, long tick, long doc, long shapes)
{
    return recordShapes(forUndo, tick, doc, shapes, NULL);
}

bool GiCoreView::recordShapes(bool forUndo, long tick, long doc,
                              long shapes, const mgvector<int>* exts, MgStringCallback* c)
{
    MgRecordShapes* recorder = impl->recorder[forUndo ? 0 : 1];
    int ret = 0;
    std::vector<MgShapes*> arr;
    int i;
    
    for (i = 0; i < (exts ? exts->count() : 0); i++) {
        MgShapes* p = MgShapes::fromHandle(exts->get(i));
        if (p) {
            arr.push_back(p);
        }
    }
    
    if (recorder && !recorder->isLoading() && !recorder->isPlaying()) {
        ret = recorder->recordStep(tick, impl->changeCount, MgShapeDoc::fromHandle(doc),
                                   MgShapes::fromHandle(shapes), arr) ? 2 : 1;
        if (ret > 1 && c) {
            c->onGetString(recorder->getFileName(false, recorder->getFileCount() - 1).c_str());
        }
    } else {
        GiPlaying::releaseDoc(doc);
        GiPlaying::releaseShapes(shapes);
    }
    for (i = 0; i < (int)arr.size(); i++) {
        MgObject::release_pointer(arr[i]);
    }
    
    return ret > 0;
}

bool GiCoreView::restoreRecord(int type, const char* path, long doc, long changeCount,
                               int index, int count, int tick, long curTick)
{
    MgRecordShapes*& recorder = impl->recorder[type > 0 ? 1 : 0];
    if (recorder || !path)
        return false;
    
    recorder = new MgRecordShapes(path, MgShapeDoc::fromHandle(doc), type == 0, curTick);
    recorder->restore(index, count, tick, curTick);
    if (type == 0 && changeCount != 0) {
        giAtomicCompareAndSwap(&impl->changeCount, changeCount, impl->changeCount);
    }
    
    return true;
}

bool GiCoreView::isUndoLoading() const
{
    return impl->recorder[0] && impl->recorder[0]->isLoading();
}

bool GiCoreView::canUndo() const
{
    return impl->recorder[0] && impl->recorder[0]->canUndo();
}

bool GiCoreView::canRedo() const
{
    return impl->recorder[0] && impl->recorder[0]->canRedo();
}

int GiCoreView::getRedoIndex() const
{
    return impl->recorder[0] ? impl->recorder[0]->getFileCount() : 0;
}

int GiCoreView::getRedoCount() const
{
    return impl->recorder[0] ? impl->recorder[0]->getMaxFileCount() : 0;
}

bool GiCoreView::undo(GiView* view)
{
    MgRecordShapes* recorder = impl->recorder[0];
    bool ret = false;
    long changeCount = impl->changeCount;
    
    if (recorder) {
        recorder->setLoading(true);
        ret = recorder->undo(impl->getShapeFactory(), impl->doc(), &changeCount);
        if (ret) {
            giAtomicCompareAndSwap(&impl->changeCount, changeCount, impl->changeCount);
            submitBackDoc(view);
            submitDynamicShapes(view);
            recorder->resetDoc(MgShapeDoc::fromHandle(acquireFrontDoc()));
            impl->regenAll(true);
            impl->hideContextActions();
        }
        recorder->setLoading(false);
    }
    
    return ret;
}

bool GiCoreView::redo(GiView* view)
{
    MgRecordShapes* recorder = impl->recorder[0];
    bool ret = false;
    long changeCount = impl->changeCount;
    
    if (recorder) {
        recorder->setLoading(true);
        ret = recorder->redo(impl->getShapeFactory(), impl->doc(), &changeCount);
        if (ret) {
            giAtomicCompareAndSwap(&impl->changeCount, changeCount, impl->changeCount);
            submitBackDoc(view);
            submitDynamicShapes(view);
            recorder->resetDoc(MgShapeDoc::fromHandle(acquireFrontDoc()));
            impl->regenAll(true);
            impl->hideContextActions();
        }
        recorder->setLoading(false);
    }
    
    return ret;
}

bool GiCoreView::isUndoRecording() const
{
    return !!impl->recorder[0];
}

bool GiCoreView::isRecording() const
{
    return impl->recorder[1] && !impl->recorder[1]->isPlaying();
}

bool GiCoreView::isPlaying() const
{
    return impl->recorder[1] && impl->recorder[1]->isPlaying();
}

int GiCoreView::getFrameIndex() const
{
    return impl->recorder[1] ? impl->recorder[1]->getFileCount() : -1;
}

long GiCoreView::getFrameTick()
{
    return impl->recorder[1] ? (long)impl->recorder[1]->getFileTick() : 0;
}

int GiCoreView::getFrameFlags()
{
    return impl->recorder[1] ? impl->recorder[1]->getFileFlags() : 0;
}

bool GiCoreView::isPaused() const
{
    return impl->startPauseTick != 0;
}

bool GiCoreView::onPause(long curTick)
{
    return giAtomicCompareAndSwap(&impl->startPauseTick, curTick, 0);
}

bool GiCoreView::onResume(long curTick)
{
    long startPauseTick = impl->startPauseTick;
    
    if (startPauseTick != 0
        && giAtomicCompareAndSwap(&impl->startPauseTick, 0, startPauseTick)) {
        long ticks = curTick - startPauseTick;
        if (impl->recorder[0] && !impl->recorder[0]->onResume(ticks)) {
            LOGE("recorder[0]->onResume(%ld) fail", ticks);
            return false;
        }
        if (impl->recorder[1] && !impl->recorder[1]->onResume(ticks)) {
            LOGE("recorder[1]->onResume(%ld) fail", ticks);
            return false;
        }
        return true;
    }
    
    return false;
}

// GiPlaying
//

struct GiPlaying::Impl {
    MgShapeDoc* frontDoc;
    MgShapeDoc* backDoc;
    MgShapes*   front;
    MgShapes*   back;
    int         tag;
    volatile long stopping;
    
    Impl(int tag) : frontDoc(NULL), backDoc(NULL), front(NULL), back(NULL)
        , tag(tag), stopping(0) {}
};

GiPlaying* GiPlaying::create(MgCoreView* v, int tag)
{
    GiPlaying* p = new GiPlaying(tag);
    if (v && tag >= 0) {
        GiCoreViewData::fromHandle(v->viewDataHandle())->playings.push_back(p);
    }
    return p;
}

void GiPlaying::release(MgCoreView* v)
{
    if (v) {
        std::vector<GiPlaying*>& s = GiCoreViewData::fromHandle(v->viewDataHandle())->playings;
        s.erase(std::find(s.begin(), s.end(), this));
    }
    delete this;
}

GiPlaying::GiPlaying(int tag) : impl(new Impl(tag))
{
}

GiPlaying::~GiPlaying()
{
    clear();
    delete impl;
}

void GiPlaying::clear()
{
    MgObject::release_pointer(impl->frontDoc);
    MgObject::release_pointer(impl->backDoc);
    MgObject::release_pointer(impl->front);
    MgObject::release_pointer(impl->back);
}

int GiPlaying::getTag() const
{
    return impl->tag;
}

void GiPlaying::stop()
{
    giAtomicIncrement(&impl->stopping);
}

bool GiPlaying::isStopping() const
{
    return impl->stopping > 0;
}

long GiPlaying::acquireFrontDoc()
{
    if (!this || !impl->frontDoc)
        return 0;
    impl->frontDoc->addRef();
    return impl->frontDoc->toHandle();
}

void GiPlaying::releaseDoc(long doc)
{
    MgShapeDoc* p = MgShapeDoc::fromHandle(doc);
    MgObject::release_pointer(p);
}

MgShapeDoc* GiPlaying::getBackDoc()
{
    if (!impl->backDoc) {
        impl->backDoc = MgShapeDoc::createDoc();
    }
    return impl->backDoc;
}

void GiPlaying::submitBackDoc()
{
    MgObject::release_pointer(impl->frontDoc);
    if (impl->backDoc) {
        impl->frontDoc = impl->backDoc->shallowCopy();
    }
}

long GiPlaying::acquireFrontShapes()
{
    if (!this || !impl->front)
        return 0;
    impl->front->addRef();
    return impl->front->toHandle();
}

void GiPlaying::releaseShapes(long shapes)
{
    MgShapes* p = MgShapes::fromHandle(shapes);
    MgObject::release_pointer(p);
}

long GiPlaying::getBackShapesHandle(bool needClear)
{
    return getBackShapes(needClear)->toHandle();
}

MgShapes* GiPlaying::getBackShapes(bool needClear)
{
    if (needClear || !impl->back) {
        MgObject::release_pointer(impl->back);
        impl->back = MgShapes::create();
    } else {
        MgShapes* old = impl->back;
        impl->back = old->shallowCopy();
        MgObject::release_pointer(old);
    }
    return impl->back;
}

void GiPlaying::submitBackShapes()
{
    MgObject::release_pointer(impl->front);
    impl->front = impl->back;
    impl->front->addRef();
}
