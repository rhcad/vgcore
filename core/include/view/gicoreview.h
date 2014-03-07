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
    static GiCoreView* createView(GiView* view, int type = 1);      //!< 创建内核视图
    static GiCoreView* createMagnifierView(GiView* newview, GiCoreView* mainView,
                                           GiView* mainDevView);    //!< 创建放大镜视图
    void destoryView(GiView* view);                                 //!< 销毁内核视图
    void release() { delete this; }                                 //!< 销毁自身
    
    long acquireGraphics(GiView* view);                             //!< 获取前端 GiGraphics 的句柄
    void releaseGraphics(long gs);                                  //!< 释放 GiGraphics 句柄
    
    int drawAll(long doc, long gs, GiCanvas* canvas);               //!< 显示所有图形
    int drawAppend(long doc, long gs, GiCanvas* canvas, int sid);   //!< 显示新图形
    int dynDraw(long shapes, long gs, GiCanvas* canvas);            //!< 显示动态图形
    int dynDraw(long shapes, long gs, GiCanvas* canvas, const mgvector<int>& exts); //!< 显示动态图形
    
    int drawAll(GiView* view, GiCanvas* canvas);                    //!< 显示所有图形，主线程中用
    int drawAppend(GiView* view, GiCanvas* canvas, int sid);        //!< 显示新图形，主线程中用
    int dynDraw(GiView* view, GiCanvas* canvas);                    //!< 显示动态图形，主线程中用
    
    int setBkColor(GiView* view, int argb);                         //!< 设置背景颜色
    static void setScreenDpi(int dpi, float factor = 1.f);          //!< 设置屏幕的点密度和UI放缩系数
    void onSize(GiView* view, int w, int h);                        //!< 设置视图的宽高
    void setPenWidthRange(GiView* view, float minw, float maxw);    //!< 设置画笔宽度范围
    
    //! 传递单指触摸手势消息
    bool onGesture(GiView* view, GiGestureType type,
            GiGestureState state, float x, float y, bool switchGesture = false);

    //! 传递双指移动手势(可放缩旋转)
    bool twoFingersMove(GiView* view, GiGestureState state,
            float x1, float y1, float x2, float y2, bool switchGesture = false);
    
    bool submitBackDoc(GiView* view);           //!< 提交静态图形到前端，在UI的regen回调中用
    bool submitDynamicShapes(GiView* view);     //!< 提交动态图形到前端，需要并发保护
    
    float calcPenWidth(GiView* view, float lineWidth);              //!< 计算画笔的像素宽度
    GiGestureType getGestureType();                                 //!< 得到当前手势类型
    GiGestureState getGestureState();                               //!< 得到当前手势状态
    static int getVersion();                                        //!< 得到内核版本号
    
    int exportSVG(long doc, long gs, const char* filename);         //!< 导出图形到SVG文件
    int exportSVG(GiView* view, const char* filename);              //!< 导出图形到SVG文件，主线程中用
    bool startRecord(const char* path, long doc,
                     bool forUndo, long curTick);                   //!< 开始录制图形，自动释放，在主线程用
    void stopRecord(GiView* view, bool forUndo);                    //!< 停止录制图形
    bool recordShapes(bool forUndo, long tick, long doc, long shapes); //!< 录制图形，自动释放
    bool recordShapes(bool forUndo, long tick, long doc,
                      long shapes, const mgvector<int>& exts);      //!< 录制图形，自动释放
    bool undo(GiView* view);                                        //!< 撤销, 需要并发访问保护
    bool redo(GiView* view);                                        //!< 重做, 需要并发访问保护
    static bool loadFrameIndex(const char* path, mgvector<int>& arr);   //!< 加载帧索引{index,tick,flags}
    int loadNextFrame(const mgvector<int>& head, long curTick);     //!< 加载下一帧，跳过过时的帧
    int skipExpireFrame(const mgvector<int>& head, int index, long curTick);    //!< 跳过过时的帧
    bool frameNeedWait(long curTick);                               //!< 当前帧是否等待显示
    bool onPause(long curTick);                                     //!< 暂停
    bool onResume(long curTick);                                    //!< 继续
    bool restoreRecord(int type, const char* path, long doc, long changeCount,
                       int index, int count, int tick, long curTick);   //!< 恢复录制
    
// MgCoreView
#ifndef SWIG
public:
    bool isPressDragging();
    bool isDrawingCommand();
    long viewAdapterHandle();
    long backDoc();
    long backShapes();
    long acquireFrontDoc();
    long acquireDynamicShapes();
    bool isDrawing();
    bool isStopping();
    int stopDrawing(bool stop = true);
    bool isUndoRecording() const;
    bool isRecording() const;
    bool isPlaying() const;
    bool isPaused() const;
    long getRecordTick(bool forUndo, long curTick);
    bool isUndoLoading() const;
    bool canUndo() const;
    bool canRedo() const;
    int getRedoIndex() const;
    int getRedoCount() const;
    int loadFirstFrame();
    int loadFirstFrame(const char* file);
    int loadNextFrame(int index);
    int loadPrevFrame(int index, long curTick);
    long getFrameTick();
    int getFrameIndex() const;
    void applyFrame(int flags);
    long getPlayingDocForEdit();
    long getDynamicShapesForEdit();
    const char* getCommand() const;
    bool setCommand(const char* name, const char* params = "");
    bool doContextAction(int action);
    void clearCachedData();
    int addShapesForTest();
    int getShapeCount();
    int getShapeCount(long doc);
    long getChangeCount();
    long getDrawCount() const;
    int getSelectedShapeCount();
    int getSelectedShapeType();
    int getSelectedShapeID();
    void clear();
    bool loadFromFile(const char* vgfile, bool readOnly = false);
    bool saveToFile(long doc, const char* vgfile, bool pretty = true);
    bool loadShapes(MgStorage* s, bool readOnly = false);
    bool saveShapes(long doc, MgStorage* s);
    const char* getContent(long doc);
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
    bool hasImageShape(long doc);
    int findShapeByImageID(long doc, const char* name);
    int traverseImageShapes(long doc, MgFindImageCallback* c);
    bool getDisplayExtent(mgvector<float>& box);
    bool getDisplayExtent(long doc, long gs, mgvector<float>& box);
    bool getBoundingBox(mgvector<float>& box);
    bool getBoundingBox(mgvector<float>& box, int shapeId);
    bool getBoundingBox(long doc, long gs, mgvector<float>& box, int shapeId);
#endif // SWIG

private:
    GiCoreView(GiCoreView* mainView = (GiCoreView*)0);
    virtual ~GiCoreView();
    void createView_(GiView* view, int type = 1);                   //!< 创建内核视图
    void createMagnifierView_(GiView* newview, GiView* mainView);   //!< 创建放大镜视图
    
    GiCoreViewImpl* impl;
};

//! 图形播放项
/*!
    \ingroup CORE_VIEW
 */
class MgPlaying
{
public:
    MgPlaying(int tag);
    ~MgPlaying();
    
    int getTag() const;                         //!< 得到标识号
    long acquireShapes();                       //!< 得到显示用的图形列表句柄，需要并发保护
    static void releaseShapes(long shapes);     //!< 释放 acquireShapes() 句柄
    
    long getShapesForEdit(bool needClear);      //!< 得到修改图形用的图形列表句柄
    void submitShapes();                        //!< 提交图形列表结果，需要并发保护
    void stop();                                //!< 标记需要停止
    bool isStopping() const;                    //!< 返回是否待停止
    
private:
    struct Impl;
    Impl* impl;
};

#ifndef DOXYGEN
#include "gicontxt.h"
#endif

#endif // TOUCHVG_CORE_VIEWDISPATCHER_H
