// recordshapes.h
// Copyright (c) 2013-2014, Zhang Yungui
// License: LGPL, https://github.com/rhcad/touchvg

#ifndef TOUCHVG_RECORD_SHAPES_H_
#define TOUCHVG_RECORD_SHAPES_H_

#include <string>
#include <vector>

class MgShapeDoc;
class MgShapes;
class MgShape;
struct MgShapeFactory;

class MgRecordShapes
{
public:
    enum { ADD = 1, EDIT = 2, DEL = 4, DYN = 8,
        DOC_CHANGED = 1, SHAPE_APPEND = 2, DYN_CHANGED = 4 };
    MgRecordShapes(const char* path, MgShapeDoc* doc, bool forUndo, long curTick);
    ~MgRecordShapes();
    
    long getCurrentTick(long curTick) const;
    bool recordStep(long tick, long changeCount, MgShapeDoc* doc,
                    MgShapes* dynShapes, const std::vector<MgShapes*>& extShapes);
    std::string getFileName(bool back = false, int index = -1) const;
    std::string getPath() const;
    bool isLoading() const;
    void setLoading(bool loading);
    bool onResume(long ticks);
    void restore(int index, int count, int tick, long curTick);
    void stopRecordIndex();
    
    bool canUndo() const;
    bool canRedo() const;
    bool undo(MgShapeFactory *factory, MgShapeDoc* doc, long* changeCount);
    bool redo(MgShapeFactory *factory, MgShapeDoc* doc, long* changeCount);
    void resetDoc(MgShapeDoc* doc);
    int getMaxFileCount() const;
    
    bool isPlaying() const;
    int getFileTick() const;
    int getFileFlags() const;
    int getFileCount() const;
    bool applyFirstFile(MgShapeFactory *factory, MgShapeDoc* doc);
    bool applyFirstFile(MgShapeFactory *factory, MgShapeDoc* doc, const char* filename);
    int applyRedoFile(int& newID, MgShapeFactory *f, MgShapeDoc* doc, MgShapes* dyns, int index);
    int applyUndoFile(int& newID, MgShapeFactory *f, MgShapeDoc* doc, MgShapes* dyns, int index, long curTick);
    static bool loadFrameIndex(std::string path, std::vector<int>& arr);
    
    static int applyFile(int& tick, int* newId, MgShapeFactory *f,
                         MgShapeDoc* doc, MgShapes* dyns, const char* fn,
                         long* changeCount = NULL, MgShape* lastShape = NULL);
    
private:
    struct Impl;
    Impl* _im;
};

//#ifdef DEBUG
//#define VG_PRETTY true
//#else
#define VG_PRETTY false
//#endif

#endif // TOUCHVG_RECORD_SHAPES_H_
