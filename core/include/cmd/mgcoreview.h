//! \file mgcoreview.h
//! \brief 定义内核视图接口 MgCoreView
// Copyright (c) 2012-2013, https://github.com/rhcad/touchvg

#ifndef TOUCHVG_CORE_VIEW_INTERFACE_H
#define TOUCHVG_CORE_VIEW_INTERFACE_H

#include "mgvector.h"

class GiContext;
struct MgStorage;

//! 内核视图接口
/*! \ingroup CORE_VIEW
    \interface MgCoreView
 */
struct MgCoreView
{
    virtual ~MgCoreView() {}
    static MgCoreView* fromHandle(long h) { MgCoreView* p; *(long*)&p = h; return p; } //!< 转为对象
    long toHandle() { long h; *(MgCoreView**)&h = this; return h; }   //!< 得到句柄，用于跨库转换
    
    virtual long viewAdapterHandle() = 0;           //!< 命令视图回调适配器的句柄, 可转换为 MgView 指针
    virtual long backDoc() = 0;                     //!< 图形文档的句柄, 用 MgShapeDoc::fromHandle() 转换
    virtual long backShapes() = 0;                  //!< 当前图形列表的句柄, 用 MgShapes::fromHandle() 转换
    
    virtual long acquireFrontDoc() = 0;             //!< 获取前端图形文档的句柄, 需要并发访问保护
    virtual void releaseDoc(long hDoc) = 0;         //!< 释放 acquireFrontDoc() 得到的文档句柄
    virtual long acquireDynamicShapes() = 0;        //!< 获取动态图形列表的句柄, 需要并发访问保护
    virtual void releaseShapes(long hShapes) = 0;   //!< 释放 acquireDynamicShapes() 得到的图形列表句柄

    virtual bool loadDynamicShapes(MgStorage* s) = 0; //!< 从数据源中加载临时图形，s为空则清除, 可异步加载
    virtual void applyDynamicShapes() = 0;          //!< 提交 loadDynamicShapes() 结果，需要并发访问保护
    
    virtual bool isPressDragging() = 0;             //!< 是否按下并拖动
    virtual const char* getCommand() const = 0;     //!< 返回当前命令名称
    virtual bool setCommand(const char* name, const char* params = "") = 0; //!< 启动命令
    virtual bool doContextAction(int action) = 0;   //!< 执行上下文动作
    
    virtual void clearCachedData() = 0;             //!< 释放临时数据内存，未做线程保护
    virtual int addShapesForTest() = 0;             //!< 添加测试图形
    
    virtual int getShapeCount() = 0;                //!< 返回后端文档的图形总数
    virtual long getChangeCount() = 0;              //!< 返回静态图形改变次数，可用于检查是否需要保存
    virtual long getDrawCount() const = 0;          //!< 返回已绘制次数，可用于录屏
    virtual int getSelectedShapeCount() = 0;        //!< 返回选中的图形个数
    virtual int getSelectedShapeType() = 0;         //!< 返回选中的图形的类型, MgShapeType
    virtual int getSelectedShapeID() = 0;           //!< 返回当前选中的图形的ID，选中多个时只取第一个

    virtual void clear() = 0;                       //!< 删除所有图形，包括锁定的图形
    virtual bool loadFromFile(const char* vgfile, bool readOnly = false) = 0;       //!< 从文件中加载
    virtual bool saveToFile(long hDoc, const char* vgfile, bool pretty = true) = 0; //!< 保存图形
    bool saveToFile(const char* vgfile, bool pretty = true);            //!< 保存图形，主线程中用
    
    virtual bool loadShapes(MgStorage* s, bool readOnly = false) = 0;   //!< 从数据源中加载图形
    virtual bool saveShapes(long hDoc, MgStorage* s) = 0;               //!< 保存图形到数据源
    bool saveShapes(MgStorage* s);                      //!< 保存图形到数据源，主线程中用

    virtual const char* getContent(long hDoc) = 0;      //!< 得到图形的JSON内容，需要调用 freeContent()
    virtual void freeContent() = 0;                     //!< 释放 getContent() 产生的缓冲资源
    virtual bool setContent(const char* content) = 0;   //!< 从JSON内容中加载图形
    const char* getContent();                           //!< 得到图形内容，需调用 freeContent()，主线程中用

    virtual bool zoomToExtent() = 0;                    //!< 放缩显示全部内容到视图区域
    virtual bool zoomToModel(float x, float y, float w, float h) = 0;   //!< 放缩显示指定范围到视图区域
    
    virtual GiContext& getContext(bool forChange) = 0;  //!< 当前绘图属性，可用 calcPenWidth() 计算线宽
    virtual void setContext(int mask) = 0;              //!< 绘图属性改变后提交更新

    //! 绘图属性改变后提交更新
    /*! 在 getContext(true) 后调用本函数。
        \param ctx 绘图属性
        \param mask 需要应用哪些属性(GiContextBits)，-1表示全部属性，0则不修改，按位组合值见 GiContextBits
        \param apply 0表示还要继续动态修改属性，1表示结束动态修改并提交，-1表示结束动态修改并放弃改动
     */
    virtual void setContext(const GiContext& ctx, int mask, int apply) = 0;

    //! 设置线条属性是否正在动态修改. getContext(false)将重置为未动态修改.
    virtual void setContextEditing(bool editing) = 0;
    
    //! 添加一个容纳图像的矩形图形
    /*!
        \param name 图像的标识名称，例如可用无路径的文件名
        \param width 图像矩形的宽度，正数，单位为点
        \param height 图像矩形的高度，正数，单位为点
        \return 新图形的ID，0表示失败
     */
    virtual int addImageShape(const char* name, float width, float height) = 0;
    
    //! 添加一个容纳图像的矩形图形
    virtual int addImageShape(const char* name, float xc, float yc, float w, float h) = 0;

    //! 返回选择包络框，四个点坐标(left, top, right, bottom)
    virtual bool getBoundingBox(mgvector<float>& box) = 0;
    
    //! 返回指定ID的图形的包络框，四个点坐标(left, top, right, bottom)
    virtual bool getBoundingBox(mgvector<float>& box, int shapeId) = 0;
};

inline bool MgCoreView::saveToFile(const char* vgfile, bool pretty) {
    long hDoc = acquireFrontDoc();
    bool ret = saveToFile(hDoc, vgfile, pretty);
    releaseDoc(hDoc);
    return ret;
}
inline bool MgCoreView::saveShapes(MgStorage* s) {
    long hDoc = acquireFrontDoc();
    bool ret = saveShapes(hDoc, s);
    releaseDoc(hDoc);
    return ret;
}
inline const char* MgCoreView::getContent() {
    long hDoc = acquireFrontDoc();
    const char* ret = getContent(hDoc);
    releaseDoc(hDoc);
    return ret;
}

#endif // TOUCHVG_CORE_VIEW_INTERFACE_H
