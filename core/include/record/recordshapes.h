// recordshapes.h
// Copyright (c) 2013-2014, Zhang Yungui
// License: LGPL, https://github.com/rhcad/touchvg

#ifndef TOUCHVG_RECORD_SHAPES_H_
#define TOUCHVG_RECORD_SHAPES_H_

#include <string>

class MgShapeDoc;
class MgShapes;
struct MgShapeFactory;

class MgRecordShapes
{
public:
    enum { ADD = 1, EDIT = 2, DEL = 4, DYN = 8,
        STD_CHANGED = 1, APPEND = 2, DYN_CHANGED = 4 };
    MgRecordShapes(const char* path, MgShapeDoc* doc, bool forUndo);
    ~MgRecordShapes();
    
    long getCurrentTick() const;
    bool recordStep(long tick, long changeCount, MgShapeDoc* doc, MgShapes* dynShapes);
    std::string getFileName(bool back = false) const;
    std::string getPath() const;
    bool isLoading() const;
    void setLoading(bool loading);
    
    bool canUndo() const;
    bool canRedo() const;
    bool undo(MgShapeFactory *factory, MgShapeDoc* doc, long* changeCount);
    bool redo(MgShapeFactory *factory, MgShapeDoc* doc, long* changeCount);
    
    bool isPlaying() const;
    int getFileTick() const;
    int getFileCount() const;
    
    static int applyFile(int& tick, int* newId, MgShapeFactory *f, MgShapeDoc* doc,
                         MgShapes* dyns, const char* fn, long* changeCount = NULL);
    
private:
    struct Impl;
    Impl* _im;
};

#ifdef DEBUG
#define VG_PRETTY true
#else
#define VG_PRETTY false
#endif

#ifndef WIN32
#include <sys/time.h>
inline long GetTickCount() {
    struct timeval current;
    gettimeofday(&current, NULL);
    return current.tv_sec * 1000 + current.tv_usec / 1000;
}
#endif

#endif // TOUCHVG_RECORD_SHAPES_H_
