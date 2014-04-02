//! \file mgdrawline.h
//! \brief 定义直线段绘图命令
//! \author pengjun, 2012.6.4
// License: LGPL, https://github.com/rhcad/touchvg

#ifndef TOUCHVG_CMD_DRAW_LINE_H_
#define TOUCHVG_CMD_DRAW_LINE_H_

#include "mgcmddraw.h"

//! 直线段绘图命令
/*! \ingroup CORE_COMMAND
    \see MgLine
*/
class MgCmdDrawLine : public MgCommandDraw
{
public:
    static const char* Name() { return "line"; }
    static MgCommand* Create() { return new MgCmdDrawLine; }
    
private:
    MgCmdDrawLine() : MgCommandDraw(Name()) {}
    virtual void release() { delete this; }
    virtual bool initialize(const MgMotion* sender, MgStorage* s);
    virtual bool backStep(const MgMotion* sender);
    virtual bool touchBegan(const MgMotion* sender);
    virtual bool touchMoved(const MgMotion* sender);
    virtual bool touchEnded(const MgMotion* sender);
};

//! 点绘图命令
/*! \ingroup CORE_COMMAND
    \see MgDot
*/
class MgCmdDrawDot : public MgCommandDraw
{
public:
    static const char* Name() { return "dot"; }
    static MgCommand* Create() { return new MgCmdDrawDot; }

private:
    MgCmdDrawDot() : MgCommandDraw(Name()) {}
    virtual void release() { delete this; }
    virtual bool initialize(const MgMotion* sender, MgStorage* s);
    virtual bool click(const MgMotion* sender);
    virtual bool touchBegan(const MgMotion* sender);
    virtual bool touchMoved(const MgMotion* sender);
    virtual bool touchEnded(const MgMotion* sender);
};

#endif // TOUCHVG_CMD_DRAW_LINE_H_
