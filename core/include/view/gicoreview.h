//! \file gicoreview.h
//! \brief 定义内核视图分发器类 GiCoreView
// Copyright (c) 2012-2013, https://github.com/rhcad/touchvg

#ifndef TOUCHVG_CORE_VIEWDISPATCHER_H
#define TOUCHVG_CORE_VIEWDISPATCHER_H

#include "gigesture.h"
#include "giview.h"
#include "mgcoreview.h"

class GiCanvas;
class GiCoreViewImpl;

//! 内核视图分发器类
/*! 本对象拥有图形文档对象，负责显示和手势动作的分发。
    \ingroup CORE_VIEW
 */
class GiCoreView : public MgCoreView
{
public:
    //! 构造函数，传入NULL构造主视图，传入主视图构造辅助视图
    GiCoreView(GiCoreView* mainView = (GiCoreView*)0);
    virtual ~GiCoreView();
    
    void createView(GiView* view, int type = 1);                    //!< 创建内核视图
    void createMagnifierView(GiView* newview, GiView* mainView);    //!< 创建放大镜视图
    void destoryView(GiView* view);                                 //!< 销毁内核视图
    
    bool isDrawing(GiView* view);                                   //!< 返回是否正在绘制静态图形
    bool stopDrawing(GiView* view);                                 //!< 标记需要停止绘图
    
    long acquireGraphics(GiView* view);                             //!< 获取前端 GiGraphics 的句柄
    void releaseGraphics(GiView* view, long hGs);                   //!< 释放 GiGraphics 句柄
    
    int drawAll(long hDoc, long hGs, GiCanvas* canvas);             //!< 显示所有图形
    int drawAppend(long hDoc, long hGs, GiCanvas* canvas, int sid); //!< 显示新图形
    int dynDraw(long hShapes, long hGs, GiCanvas* canvas);          //!< 显示动态图形
    
    int drawAll(GiView* view, GiCanvas* canvas);                    //!< 显示所有图形，单线程
    int drawAppend(GiView* view, GiCanvas* canvas, int sid);        //!< 显示新图形，单线程
    int dynDraw(GiView* view, GiCanvas* canvas);                    //!< 显示动态图形，单线程
    
    int setBkColor(GiView* view, int argb);                         //!< 设置背景颜色
    static void setScreenDpi(int dpi, float factor = 1.f);          //!< 设置屏幕的点密度和UI放缩系数
    void onSize(GiView* view, int w, int h);                        //!< 设置视图的宽高
    
    //! 传递单指触摸手势消息
    bool onGesture(GiView* view, GiGestureType type,
            GiGestureState state, float x, float y, bool switchGesture = false);

    //! 传递双指移动手势(可放缩旋转)
    bool twoFingersMove(GiView* view, GiGestureState state,
            float x1, float y1, float x2, float y2, bool switchGesture = false);
    
    void submitBackDoc();                       //!< 提交静态图形到前端，在UI的regen回调中用
    bool submitDynamicShapes(GiView* view);     //!< 提交动态图形到前端，需要并发保护
    
    float calcPenWidth(GiView* view, float lineWidth);              //!< 计算画笔的像素宽度
    GiGestureType getGestureType();                                 //!< 得到当前手势类型
    GiGestureState getGestureState();                               //!< 得到当前手势状态
    int getVersion();                                               //!< 得到内核版本号
    
// MgCoreView
public:
    bool isPressDragging();
    long viewAdapterHandle();
    long backDoc();
    long backShapes();
    long acquireFrontDoc();
    void releaseDoc(long hDoc);
    long acquireDynamicShapes();
    void releaseShapes(long hShapes);
    bool loadDynamicShapes(MgStorage* s);
    void applyDynamicShapes();
    const char* getCommand() const;
    bool setCommand(GiView* view, const char* name, const char* params = "");
    bool setCommand(const char* name, const char* params = "");
    bool doContextAction(int action);
    void clearCachedData();
    int addShapesForTest();
    int getShapeCount();
    long getChangeCount();
    long getDrawCount() const;
    int getSelectedShapeCount();
    int getSelectedShapeType();
    int getSelectedShapeID();
    void clear();
    bool loadFromFile(const char* vgfile, bool readOnly = false);
    bool saveToFile(long hDoc, const char* vgfile, bool pretty = true);
    bool loadShapes(MgStorage* s, bool readOnly = false);
    bool saveShapes(long hDoc, MgStorage* s);
    const char* getContent(long hDoc);
    void freeContent();
    bool setContent(const char* content);
    bool zoomToExtent();
    bool zoomToModel(float x, float y, float w, float h);
    GiContext& getContext(bool forChange);
    void setContext(const GiContext& ctx, int mask, int apply);
    void setContext(int mask);
    void setContextEditing(bool editing);
    int addImageShape(const char* name, float width, float height);
    int addImageShape(const char* name, float xc, float yc, float w, float h);
    bool getBoundingBox(mgvector<float>& box);
    bool getBoundingBox(mgvector<float>& box, int shapeId);

private:
    GiCoreViewImpl* impl;
};

#ifndef DOXYGEN
#include "gicontxt.h"
#endif

#endif // TOUCHVG_CORE_VIEWDISPATCHER_H
