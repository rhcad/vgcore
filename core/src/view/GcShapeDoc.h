//! \file GcShapeDoc.h
//! \brief 定义图形文档类 GcShapeDoc
// Copyright (c) 2012-2013, https://github.com/rhcad/touchvg

#ifndef TOUCHVG_CORE_SHAPEDOC_H
#define TOUCHVG_CORE_SHAPEDOC_H

#include <vector>

class GcBaseView;
class GiView;
class MgShapeDoc;

//! 图形文档类
/*! \ingroup CORE_VIEW
 */
class GcShapeDoc
{
public:
    GcShapeDoc();
    ~GcShapeDoc();
    
    bool addView(GcBaseView* view);
    bool removeView(GcBaseView* view);
    GcBaseView* findView(GiView* view) const;
    GcBaseView* getView(int index) const;
    int getViewCount() const;
    GcBaseView* firstView() const;
    
    MgShapeDoc* frontDoc() const { return _frontDoc; }
    MgShapeDoc* backDoc() const { return _backDoc; }
    void submitBackDoc(MgShapeDoc* doc);
    
private:
    std::vector<GcBaseView*>    _views;
    MgShapeDoc*     _frontDoc;
    MgShapeDoc*     _backDoc;
};

#endif // TOUCHVG_CORE_SHAPEDOC_H
