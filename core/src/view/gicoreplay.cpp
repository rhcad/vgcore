//! \file gicoreplay.cpp
//! \brief 实现内核视图类 GiCoreView 的录制和播放功能
// Copyright (c) 2012-2013, https://github.com/rhcad/touchvg

#include "gicoreview.h"
#include "gicoreviewimpl.h"

long GiCoreView::getRecordTick(bool forUndo, long curTick)
{
    MgRecordShapes* recorder = impl->recorder[forUndo ? 0 : 1];
    if (!recorder)
        return 0;
    long tick = recorder->getCurrentTick(curTick);
    long pauseTick = curTick - impl->startPauseTick;
    return isPaused() && tick > pauseTick ? tick - pauseTick : tick;
}

bool GiCoreView::startRecord(const char* path, long doc, bool forUndo, long curTick)
{
    MgRecordShapes*& recorder = impl->recorder[forUndo ? 0 : 1];
    if (recorder || !path)
        return false;
    
    recorder = new MgRecordShapes(path, MgShapeDoc::fromHandle(doc), forUndo, curTick);
    return (isPlaying() || forUndo
            || saveToFile(doc, recorder->getFileName().c_str(), VG_PRETTY));
}

void GiCoreView::stopRecord(GiView* view, bool forUndo)
{
    MgRecordShapes*& recorder = impl->recorder[forUndo ? 0 : 1];
    
    if (recorder) {
        delete recorder;
        recorder = NULL;
    }
    if (!forUndo && (impl->dynShapesPlay || impl->docPlay)) {
        MgObject::release_pointer(impl->dynShapesPlay);
        MgObject::release_pointer(impl->docPlay);
        if (view) {
            submitBackDoc(view);
            submitDynamicShapes(view);
        }
    }
}

bool GiCoreView::recordShapes(bool forUndo, long tick, long doc, long shapes)
{
    return recordShapes(forUndo, tick, doc, shapes, NULL);
}

bool GiCoreView::recordShapes(bool forUndo, long tick, long doc,
                              long shapes, const mgvector<int>* exts)
{
    MgRecordShapes* recorder = impl->recorder[forUndo ? 0 : 1];
    bool ret = false;
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
                                   MgShapes::fromHandle(shapes), arr);
    } else {
        releaseDoc(doc);
        releaseShapes(shapes);
    }
    for (i = 0; i < (int)arr.size(); i++) {
        MgObject::release_pointer(arr[i]);
    }
    
    return ret;
}

bool GiCoreView::restoreRecord(int type, const char* path, long doc, long changeCount,
                               int index, int count, int tick, long curTick)
{
    MgRecordShapes*& recorder = impl->recorder[type > 0 ? 1 : 0];
    if (recorder || !path)
        return false;
    
    recorder = new MgRecordShapes(path, MgShapeDoc::fromHandle(doc), type == 0, curTick);
    recorder->restore(index, count, tick, curTick);
    if (type == 0) {
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
    long changeCount;
    
    if (recorder) {
        recorder->setLoading(true);
        ret = recorder->undo(impl->getShapeFactory(), impl->doc(), &changeCount);
        if (ret) {
            giAtomicCompareAndSwap(&impl->changeCount, changeCount, impl->changeCount);
            submitBackDoc(view);
            submitDynamicShapes(view);
            recorder->resetDoc(MgShapeDoc::fromHandle(acquireFrontDoc()));
            impl->regenAll(true);
            impl->showContextActions(0, NULL, Box2d::kIdentity(), NULL);
        }
        recorder->setLoading(false);
    }
    
    return ret;
}

bool GiCoreView::redo(GiView* view)
{
    MgRecordShapes* recorder = impl->recorder[0];
    bool ret = false;
    long changeCount;
    
    if (recorder) {
        recorder->setLoading(true);
        ret = recorder->redo(impl->getShapeFactory(), impl->doc(), &changeCount);
        if (ret) {
            giAtomicCompareAndSwap(&impl->changeCount, changeCount, impl->changeCount);
            submitBackDoc(view);
            submitDynamicShapes(view);
            recorder->resetDoc(MgShapeDoc::fromHandle(acquireFrontDoc()));
            impl->regenAll(true);
            impl->showContextActions(0, NULL, Box2d::kIdentity(), NULL);
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
    long tick = impl->recorder[1] ? (long)impl->recorder[1]->getFileTick() : 0;
    return tick;
}

long GiCoreView::getPlayingDocForEdit()
{
    if (!impl->docPlay) {
        impl->docPlay = impl->doc()->cloneDoc();
    }
    return impl->docPlay->toHandle();
}

long GiCoreView::getDynamicShapesForEdit()
{
    MgObject::release_pointer(impl->dynShapesPlay);
    impl->dynShapesPlay = MgShapes::create();
    return impl->dynShapesPlay->toHandle();
}

bool GiCoreView::loadFrameIndex(const char* path, mgvector<int>& arr)
{
    std::vector<int> v;
    
    if (MgRecordShapes::loadFrameIndex(path, v)) {
        arr.setSize((int)v.size());
        for (int i = 0; i < arr.count(); i++) {
            arr.set(i, v[i]);
        }
    }
    return !v.empty() && arr.count() > 0;
}

int GiCoreView::loadFirstFrame()
{
    if (!isPlaying() || !getPlayingDocForEdit())
        return 0;
    
    if (!impl->recorder[1]->applyFirstFile(impl->getShapeFactory(), impl->docPlay))
        return 0;
    
    impl->docPlay->setReadOnly(true);
    return DOC_CHANGED;
}

int GiCoreView::loadFirstFrame(const char* filename)
{
    if (!isPlaying() || !getPlayingDocForEdit())
        return 0;
    
    if (!impl->recorder[1]->applyFirstFile(impl->getShapeFactory(), impl->docPlay, filename))
        return 0;
    
    impl->docPlay->setReadOnly(true);
    return DOC_CHANGED;
}

int GiCoreView::skipExpireFrame(const mgvector<int>& head, int index, long curTick)
{
    int from = index;
    int tickNow = (int)getPlayingTick(curTick);
    
    for ( ; index <= head.count() / 3; index++) {
        int tick = head.get(index * 3 - 2);
        int flags = head.get(index * 3 - 1);
        if (flags != MgRecordShapes::DYN || tick + 200 > tickNow)
            break;
    }
    if (index > from) {
        LOGD("Skip %d frames from #%d, tick=%d, now=%d",
             index - from, from, head.get(from * 3 - 2), tickNow);
    }
    return index;
}

bool GiCoreView::frameNeedWait(long curTick)
{
    return impl->startPauseTick || getFrameTick() - 100 > getPlayingTick(curTick);
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

int GiCoreView::loadNextFrame(const mgvector<int>& head, long curTick)
{
    return loadNextFrame(skipExpireFrame(head, getFrameIndex(), curTick));
}

int GiCoreView::loadNextFrame(int index)
{
    if (!isPlaying() || !getPlayingDocForEdit() || !getDynamicShapesForEdit()) {
        return 0;
    }
    
    int newID = 0;
    int flags = impl->recorder[1]->applyRedoFile(newID, impl->getShapeFactory(),
                                                 impl->docPlay, impl->dynShapesPlay, index);
    return flags;
}

int GiCoreView::loadPrevFrame(int index, long curTick)
{
    if (!isPlaying() || !getPlayingDocForEdit() || !getDynamicShapesForEdit()) {
        return 0;
    }
    
    int newID = 0;
    int flags = impl->recorder[1]->applyUndoFile(newID, impl->getShapeFactory(),
                                                 impl->docPlay, impl->dynShapesPlay, index, curTick);
    return flags;
}

void GiCoreView::applyFrame(int flags)
{
    if (flags & (DOC_CHANGED | SHAPE_APPEND)) {
        impl->_gcdoc->submitBackDoc(impl->docPlay);
        if (impl->curview) {
            impl->curview->xform()->setModelTransform(impl->docPlay->modelTransform());
            impl->curview->xform()->zoomTo(impl->docPlay->getPageRectW());
        }
    }
    if (flags & DYN_CHANGED) {
        MgObject::release_pointer(impl->dynShapesFront);
        impl->dynShapesFront = impl->dynShapesPlay;
        impl->dynShapesFront->addRef();
    }
    if (impl->curview) {
        impl->curview->submitBackXform();
    }
    
    if (flags & DOC_CHANGED) {
        impl->regenAll(true);
    }
    else if (flags & SHAPE_APPEND) {
        impl->regenAppend(impl->docPlay->getCurrentLayer()->getLastShape()->getID());
    }
    else if (flags & DYN_CHANGED) {
        impl->redraw();
    }
}

// MgPlaying
//

struct MgPlaying::Impl {
    MgShapes*   front;
    MgShapes*   back;
    int         tag;
    volatile long stopping;
    
    Impl(int tag) : front(NULL), back(NULL), tag(tag), stopping(0) {}
    ~Impl() {
        MgObject::release_pointer(front);
        MgObject::release_pointer(back);
    }
};

MgPlaying::MgPlaying(int tag) : impl(new Impl(tag))
{
}

MgPlaying::~MgPlaying()
{
    delete impl;
}

int MgPlaying::getTag() const
{
    return impl->tag;
}

void MgPlaying::stop()
{
    giAtomicIncrement(&impl->stopping);
}

bool MgPlaying::isStopping() const
{
    return impl->stopping > 0;
}

long MgPlaying::acquireShapes()
{
    if (!impl->front)
        return 0;
    impl->front->addRef();
    return impl->front->toHandle();
}

void MgPlaying::releaseShapes(long shapes)
{
    MgShapes* p = MgShapes::fromHandle(shapes);
    MgObject::release_pointer(p);
}

long MgPlaying::getShapesForEdit(bool needClear)
{
    if (needClear || !impl->back) {
        MgObject::release_pointer(impl->back);
        impl->back = MgShapes::create();
    } else {
        MgShapes* old = impl->back;
        impl->back = old->shallowCopy();
        MgObject::release_pointer(old);
    }
    return impl->back->toHandle();
}

void MgPlaying::submitShapes()
{
    MgObject::release_pointer(impl->front);
    impl->front = impl->back;
    impl->front->addRef();
}
