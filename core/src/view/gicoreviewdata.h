//! \file gicoreviewdata.h
//! \brief 定义GiCoreView内部数据类 GiCoreViewData
// Copyright (c) 2014, https://github.com/rhcad/touchvg

#ifndef TOUCHVG_CORE_VIEWDATA_H
#define TOUCHVG_CORE_VIEWDATA_H

#include "recordshapes.h"
#include "giplaying.h"
#include "mgspfactory.h"
#include "mgview.h"

struct GiPlayShapes
{
    GiPlaying*      playing;
    MgRecordShapes* player;
    GiPlayShapes() : playing(NULL), player(NULL) {}
};

//! GiCoreView内部数据类
class GiCoreViewData : public MgView, public MgShapeFactory
{
public:
    volatile long   startPauseTick;
    MgRecordShapes* recorder[2];
    std::vector<GiPlaying*> playings;
    GiPlaying*      drawing;
    MgShapeDoc*     backDoc;
    GiPlayShapes    play;
    
    GiCoreViewData() : startPauseTick(0) {}
    
    virtual void submitBackXform() = 0;         //!< 应用后端坐标系对象到前端
};

#endif // TOUCHVG_CORE_VIEWDATA_H
