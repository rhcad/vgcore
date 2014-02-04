// recordshapes.cpp
// Copyright (c) 2013-2014, Zhang Yungui
// License: LGPL, https://github.com/rhcad/touchvg

#include "recordshapes.h"
#include "mgshapedoc.h"
#include "mglayer.h"
#include "mgjsonstorage.h"
#include "mgstorage.h"
#include "mglog.h"
#include <sstream>
#include <map>
#include <vector>

struct MgRecordShapes::Impl
{
    std::string     path;
    bool            forUndo;
    std::map<int, long>  id2ver;
    volatile int    fileCount;
    volatile int    maxCount;
    volatile long   loading;
    MgShapeDoc      *lastDoc;
    long            startTick;
    int             tick;
    int             flags[2];
    int             shapeCount;
    MgJsonStorage   *js[3];
    MgStorage       *s[3];
    
    Impl() : fileCount(0), maxCount(0), loading(0), startTick(GetTickCount()), tick(0) {
        memset(js, 0, sizeof(js));
        memset(s, 0, sizeof(s));
    }
    ~Impl() { MgObject::release_pointer(lastDoc); }
    
    void beginJsonFile();
    bool saveJsonFile();
    std::string getFileName(bool back, int index = -1) const;
    void resetVersion(const MgShapes* shapes);
    void startRecord(const MgShapes* shapes);
    void stopRecord();
    void saveIndexFile(bool ended);
    void recordShapes(const MgShapes* shapes);
};

MgRecordShapes::MgRecordShapes(const char* path, MgShapeDoc* doc, bool forUndo)
{
    _im = new Impl();
    _im->path = path;
    if (*_im->path.rbegin() != '/' && *_im->path.rbegin() != '\\') {
        _im->path += '/';
    }
    _im->forUndo = forUndo;
    _im->lastDoc = doc;
    if (doc) {
        _im->startRecord(doc->getCurrentLayer());
    }
}

MgRecordShapes::~MgRecordShapes()
{
    _im->stopRecord();
    delete _im;
}

bool MgRecordShapes::isPlaying() const
{
    return !_im->lastDoc;
}

int MgRecordShapes::getFileTick() const
{
    return _im->tick;
}

int MgRecordShapes::getFileCount() const
{
    return _im->fileCount;
}

long MgRecordShapes::getCurrentTick() const
{
    return GetTickCount() - _im->startTick;
}

bool MgRecordShapes::recordStep(long tick, long changeCount, MgShapeDoc* doc, MgShapes* dynShapes)
{
    _im->beginJsonFile();
    _im->tick = (int)tick;
    
    bool needDyn = _im->lastDoc && !_im->forUndo;
    if (doc) {
        if (_im->lastDoc) {     // undo() set lastDoc as null
            _im->recordShapes(doc->getCurrentLayer());
            MgObject::release_pointer(_im->lastDoc);
        }
        _im->lastDoc = doc;
    }
    if (needDyn && dynShapes && dynShapes->getShapeCount() > 0) {
        _im->flags[0] |= DYN;
        _im->s[0]->writeNode("dynamic", -1, false);
        dynShapes->save(_im->s[0]);
        _im->s[0]->writeNode("dynamic", -1, true);
    }
    MgObject::release_pointer(dynShapes);
    _im->s[0]->writeInt("flags", _im->flags[0]);
    _im->s[0]->writeInt("changeCount", (int)changeCount);
    _im->s[1]->writeInt("changeCount", (int)changeCount);
    
    bool ret = _im->saveJsonFile();
    
    if (ret && _im->s[2]) {
        _im->s[2]->writeNode("r", _im->fileCount - 2, false);
        _im->s[2]->writeInt("tick", _im->tick);
        _im->s[2]->writeInt("flags", _im->flags[0]);
        _im->s[2]->writeNode("r", _im->fileCount - 2, true);
        
        if (_im->fileCount % 10 == 0 && _im->flags[0] != MgRecordShapes::DYN) {
            _im->saveIndexFile(false);
        }
    }
    
    return ret;
}

std::string MgRecordShapes::getFileName(bool back) const
{
    return _im->getFileName(back);
}

std::string MgRecordShapes::getPath() const
{
    return _im->path;
}

bool MgRecordShapes::isLoading() const
{
    return _im->loading > 0;
}

bool MgRecordShapes::canUndo() const
{
    return _im->fileCount > 1 && !_im->loading;
}

bool MgRecordShapes::canRedo() const
{
    return _im->fileCount < _im->maxCount && !_im->loading;
}

void MgRecordShapes::setLoading(bool loading)
{
    if (loading)
        giInterlockedIncrement(&_im->loading);
    else
        giInterlockedDecrement(&_im->loading);
}

bool MgRecordShapes::undo(MgShapeFactory *factory, MgShapeDoc* doc, long* changeCount)
{
    if (_im->loading > 1 || !_im->lastDoc)
        return false;
    
    giInterlockedIncrement(&_im->loading);
    
    std::string fn(_im->getFileName(true, _im->fileCount - 1));
    int ret = applyFile(_im->tick, NULL, factory, doc, NULL, fn.c_str(), changeCount);
    
    if (ret) {
        _im->fileCount--;
        _im->resetVersion(doc->getCurrentLayer());
        MgObject::release_pointer(_im->lastDoc);
        LOGD("Undo %d", _im->fileCount);
    }
    giInterlockedDecrement(&_im->loading);
    
    return ret != 0;
}

bool MgRecordShapes::redo(MgShapeFactory *factory, MgShapeDoc* doc, long* changeCount)
{
    if (_im->loading > 1)
        return false;
    
    giInterlockedIncrement(&_im->loading);
    
    std::string fn(_im->getFileName(false, _im->fileCount));
    int ret = applyFile(_im->tick, NULL, factory, doc, NULL, fn.c_str(), changeCount);
    
    if (ret) {
        _im->fileCount++;
        _im->resetVersion(doc->getCurrentLayer());
        MgObject::release_pointer(_im->lastDoc);
        LOGD("Redo %d", _im->fileCount);
    }
    giInterlockedDecrement(&_im->loading);
    
    return ret != 0;
}

void MgRecordShapes::Impl::recordShapes(const MgShapes* shapes)
{
    MgShapeIterator it(shapes);
    std::map<int, long> tmpids(id2ver);
    std::map<int, long>::iterator i;
    int i2 = 0;
    int sid;
    std::vector<int> newids;
    
    s[0]->writeNode("shapes", shapes->getIndex(), false);
    s[1]->writeNode("shapes", shapes->getIndex(), false);
    
    while (const MgShape* sp = it.getNext()) {
        sid = sp->getID();
        i = id2ver.find(sid);                                   // 查找是否之前已存在
        
        if (i == id2ver.end()) {                                // 是新增的图形
            newids.push_back(sid);
            id2ver[sid] = sp->shapec()->getChangeCount();       // 增加记录版本
            shapes->saveShape(s[0], sp, shapeCount++);          // 写图形节点
            flags[0] |= flags[0] ? EDIT : ADD;
        } else {
            tmpids.erase(tmpids.find(sid));                     // 标记是已有图形
            if (i->second != sp->shapec()->getChangeCount()) {  // 改变的图形
                i->second = sp->shapec()->getChangeCount();
                id2ver[sid] = sp->shapec()->getChangeCount();   // 更新版本
                shapes->saveShape(s[0], sp, shapeCount++);
                flags[0] |= EDIT;
                i2 += shapes->saveShape(s[1], lastDoc->findShape(sid), i2) ? 1 : 0;
                flags[1] |= EDIT;
            }
        }
    }
    s[0]->writeNode("shapes", shapes->getIndex(), true);
    s[0]->writeInt("count", shapeCount += (int)tmpids.size());
    
    if (!tmpids.empty()) {                                      // 之前存在，现在已删除
        flags[0] |= DEL;
        s[0]->writeNode("delete", -1, false);
        int j = 0;
        for (i = tmpids.begin(); i != tmpids.end(); ++i) {
            sid = i->first;
            id2ver.erase(id2ver.find(sid));
            
            std::stringstream ss;
            ss << "d" << j++;
            s[0]->writeInt(ss.str().c_str(), sid);              // 记下删除的图形的ID
            flags[1] |= ADD;
            i2 += shapes->saveShape(s[1], lastDoc->findShape(sid), i2) ? 1 : 0;
        }
        s[0]->writeNode("delete", -1, true);
    }
    
    s[1]->writeNode("shapes", shapes->getIndex(), true);
    
    if (!newids.empty()) {
        flags[1] |= DEL;
        s[1]->writeNode("delete", -1, false);
        for (unsigned j = 0; j < newids.size(); j++) {
            std::stringstream ss;
            ss << "d" << j;
            s[1]->writeInt(ss.str().c_str(), newids[j]);
        }
        s[1]->writeNode("delete", -1, true);
    }
    s[1]->writeInt("flags", flags[1]);
    s[1]->writeInt("count", i2 + (int)newids.size());
}

void MgRecordShapes::Impl::resetVersion(const MgShapes* shapes)
{
    MgShapeIterator it(shapes);
    
    id2ver.clear();
    while (const MgShape* sp = it.getNext()) {
        id2ver[sp->getID()] = sp->shapec()->getChangeCount();
    }
}

void MgRecordShapes::Impl::startRecord(const MgShapes* shapes)
{
    resetVersion(shapes);
    if (!forUndo) {
        js[2] = new MgJsonStorage();
        s[2] = js[2]->storageForWrite();
        s[2]->writeNode("records", -1, false);
    }
}

void MgRecordShapes::Impl::beginJsonFile()
{
    if (maxCount == 0) {
        maxCount = fileCount = 1;
    }
    flags[0] = 0;
    flags[1] = 0;
    shapeCount = 0;
    
    for (int i = 0; i < 2; i++) {
        js[i] = new MgJsonStorage();
        s[i] = js[i]->storageForWrite();
        s[i]->writeNode("record", -1, false);
        s[i]->writeInt("tick", tick);
    }
}

std::string MgRecordShapes::Impl::getFileName(bool back, int index) const
{
    std::stringstream ss;
    if (index < 0)
        index = fileCount;
    ss << path << index << (index > 0 ? (back ? ".vgu" : ".vgr") : ".vg");
    return ss.str();
}

bool MgRecordShapes::Impl::saveJsonFile()
{
    bool ret = false;
    std::string filename;
    
    for (int i = 0; i < 2; i++) {
        if (lastDoc) {
            s[i]->writeFloatArray("transform", &lastDoc->modelTransform().m11, 6);
            s[i]->writeFloatArray("pageExtent", &lastDoc->getPageRectW().xmin, 4);
            s[i]->writeFloat("viewScale", lastDoc->getViewScale());
        }
        if (flags[i] != 0) {
            filename = getFileName(i > 0);
            FILE *fp = mgopenfile(filename.c_str(), "wt");
            
            if (!fp) {
                LOGE("Fail to save file: %s", filename.c_str());
            } else {
                ret = s[i]->writeNode("record", -1, true) && js[i]->save(fp, VG_PRETTY);
                fclose(fp);
                if (!ret) {
                    LOGE("Fail to record shapes: %s", filename.c_str());
                }
            }
        }
        delete js[i];
        js[i] = NULL;
        s[i] = NULL;
    }
    if (ret) {
        if (flags[0] != DYN || shapeCount > 1) {
            LOGD("Record %03d: tick=%d, flags=%d, count=%d", fileCount, tick, flags[0], shapeCount);
        }
        maxCount = ++fileCount;
    }
    
    return ret;
}

void MgRecordShapes::Impl::saveIndexFile(bool ended)
{
    std::string filename(path + "records.json");
    FILE *fp = mgopenfile(filename.c_str(), "wt");
    
    if (!fp) {
        LOGE("Fail to save file: %s", filename.c_str());
    } else {
        if (ended) {
            s[2]->writeNode("records", -1, true);
        }
        if (js[2]->save(fp, VG_PRETTY)) {
            LOGD("Save records: %s", filename.c_str());
        } else {
            LOGE("Fail to save records: %s", filename.c_str());
        }
        fclose(fp);
    }
}

void MgRecordShapes::Impl::stopRecord()
{
    if (js[2]) {
        saveIndexFile(true);
        delete js[2];
        js[2] = NULL;
        s[2] = NULL;
    }
}

int MgRecordShapes::applyFile(int& tick, int* newId, MgShapeFactory *f, MgShapeDoc* doc,
                              MgShapes* dyns, const char* fn, long* changeCount)
{
    FILE *fp = mgopenfile(fn, "rt");
    if (!fp) {
        if (!newId) {
            LOGE("Fail to read file: %s", fn);
        }
        return 0;
    }
    
    MgJsonStorage js;
    MgStorage* s = js.storageForRead(fp);
    int ret = 0;
    
    fclose(fp);
    if (s->readNode("record", -1, false)) {
        if (doc) {
            if (s->readFloatArray("transform", &doc->modelTransform().m11, 6) == 6) {
                Box2d rect(doc->getPageRectW());
                s->readFloatArray("pageExtent", &rect.xmin, 4);
                float viewScale = s->readFloat("viewScale", doc->getViewScale());
                doc->setPageRectW(rect, viewScale);
            }
            
            MgShapes* stds = doc->getCurrentLayer();
            int flags = s->readInt("flags", 0);
            
            if ((flags & (ADD|EDIT)) && stds->load(f, s, true) > 0) {
                ret |= (flags == ADD) ? APPEND : STD_CHANGED;
                if (newId && ret == APPEND)
                    *newId = stds->getLastShape()->getID();
            }
            
            if (s->readNode("delete", -1, false)) {
                for (int i = 0; ; i++) {
                    std::stringstream ss;
                    ss << "d" << i;
                    int sid = s->readInt(ss.str().c_str(), 0);
                    if (sid == 0)
                        break;
                    if (stds->removeShape(sid))
                        ret |= STD_CHANGED;
                }
                s->readNode("delete", -1, true);
            }
        }
        if (dyns && s->readNode("dynamic", -1, false)) {
            if (dyns->load(f, s) >= 0)
                ret |= DYN_CHANGED;
            s->readNode("dynamic", -1, true);
        }
        if (ret) {
            tick = s->readInt("tick", tick);
        }
        if (ret && changeCount) {
            *changeCount = s->readInt("changeCount", 0);
        }
        
        s->readNode("record", -1, true);
    }
    
    return ret;
}
