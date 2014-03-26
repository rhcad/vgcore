// mgcmdmgr_.cpp: 实现命令管理器类
// Copyright (c) 2004-2013, Zhang Yungui
// License: LGPL, https://github.com/rhcad/touchvg

#include "mgcmdmgr_.h"
#include "mgcmdmgrfactory.h"
#include "mgcmdselect.h"
#include "cmdsubject.h"
#include "mglog.h"

MgCmdManager* MgCmdManagerFactory::create()
{
    return new MgCmdManagerImpl();
}

bool MgCmdManagerImpl::registerCommand(const char* name, MgCommand* (*creator)())
{
    bool ret = (name && _factories[name] != creator);
    
    if (ret && !creator) {
        _factories.erase(name);
    }
    else if (ret) {
        _factories[name] = creator;
        LOGD("registerCommand %d: %s", (int)_factories.size(), name);
        return true;
    }
    
    return ret;
}

MgCmdManagerImpl::MgCmdManagerImpl() : _newShapeID(0), _snapShapeId(0)
{
    _snapType[0] = _snapType[1] = 0;
}

MgCmdManagerImpl::~MgCmdManagerImpl()
{
    unloadCommands();
}

void MgCmdManagerImpl::unloadCommands()
{
    for (CMDS::iterator it = _cmds.begin(); it != _cmds.end(); ++it)
        it->second->release();
    _cmds.clear();
    _cmdname = "";
    getCmdSubject()->onUnloadCommands(this);
}

const char* MgCmdManagerImpl::getCommandName()
{
    MgCommand* cmd = getCommand();
    return cmd ? cmd->getName() : "";
}

const char* MgCmdManagerImpl::getCommandName(int index)
{
    Factories::iterator it = _factories.begin();
    for (; it != _factories.end() && index-- > 0; ++it) ;
    return it != _factories.end() ? it->first.c_str() : "";
}

bool MgCmdManagerImpl::switchCommand(const MgMotion* sender)
{
    const char* name;
    
    for (int i = 0; *(name = getCommandName(i)); i++) {
        if (_cmdname == name) {
            name = getCommandName(i + 1);
            name = *name ? name : getCommandName(0);
            return setCommand(sender, name, NULL);
        }
    }
    return setCommand(sender, getCommandName(0), NULL);
}

MgCommand* MgCmdManagerImpl::getCommand()
{
    CMDS::iterator it = _cmds.find(_cmdname);
    return (it != _cmds.end()) ? it->second : NULL;
}

MgCommand* MgCmdManagerImpl::findCommand(const char* name)
{
    if (!name) {
        return NULL;
    }
    
    CMDS::iterator it = _cmds.find(name);
    
    if (it == _cmds.end() && *name)
    {
        MgCommand* cmd = NULL;
        Factories::iterator itf = _factories.find(name);
        
        if (itf != _factories.end() && itf->second) {
            cmd = (itf->second)();
        }
        if (!cmd && strcmp(name, "select") == 0) {
            cmd = MgCmdSelect::Create();
        }
        if (cmd) {
            _cmds[name] = cmd;
            it = _cmds.find(name);
        }
    }
    
    return it != _cmds.end() ? it->second : NULL;
}

bool MgCmdManagerImpl::setCommand(const MgMotion* sender,
                                  const char* name, MgStorage* s)
{
    if (!name) {
        return cancel(sender);
    }
    if (strcmp(name, "@draw") == 0) {   // 将 @draw 换成上一次绘图命令名
        name = _drawcmd.empty() ? "splines" : _drawcmd.c_str();
    }

    MgCommand* cmd = findCommand(name);
    if (!cmd) {
        cmd = sender->view->getCmdSubject()->createCommand(sender, name);
        if (cmd) {
            _cmds[name] = cmd;
            LOGD("createCommand %d: %s", (int)_cmds.size(), name);
        }
    }
    
    if (strcmp(name, "erase") == 0 && _cmdname == "select") {   // 在选择命令中点橡皮擦
        MgSelection *sel = getSelection();
        if (sel && sel->deleteSelection(sender)) {      // 直接删除选中的图形
            return false;                               // 不切换到橡皮擦命令
        }
    }
    
    cancel(sender);
    
    bool ret = false;
    std::string oldname(_cmdname);
    
    if (cmd) {
        _cmdname = cmd->getName();
        
        ret = cmd->initialize(sender, s);
        if (!ret) {
            _cmdname = oldname;
        }
        else if (cmd->isDrawingCommand()) {
            _drawcmd = _cmdname;
        }
    }
    else {
        if (strcmp(name, "erasewnd") == 0) {
            eraseWnd(sender);
        }
        else {
            _cmdname = "select";
            cmd = findCommand(_cmdname.c_str());
            cmd->initialize(sender, s);
        }
    }
    
    if (MgBaseShape::minTol().equalPoint() < 1e-5) {
        MgBaseShape::minTol().setEqualPoint(sender->view->xform()->displayToModel(1.f, true));
    }
    if (oldname != _cmdname) {
        sender->view->commandChanged();
    }
    sender->view->redraw(false);
    
    return ret;
}

bool MgCmdManagerImpl::cancel(const MgMotion* sender)
{
    clearSnap(sender);
    
    int actions[] = { 0 };
    sender->view->showContextActions(0, actions, Box2d(), NULL);

    CMDS::iterator it = _cmds.find(_cmdname);
    if (it != _cmds.end()) {
        return it->second->cancel(sender);
    }
    return false;
}

int MgCmdManagerImpl::getSelection(MgView* view, int count, const MgShape** shapes)
{
    if (_cmdname == MgCmdSelect::Name() && view) {
        MgCmdSelect* sel = (MgCmdSelect*)getCommand();
        return sel ? sel->getSelection(view, count, shapes) : 0;
    }
    return 0;
}

int MgCmdManagerImpl::getSelectionForChange(MgView* view, int count, MgShape** shapes)
{
    if (_cmdname == MgCmdSelect::Name() && view) {
        MgCmdSelect* sel = (MgCmdSelect*)getCommand();
        return sel ? sel->getSelectionForChange(view, count, shapes) : 0;
    }
    return 0;
}

bool MgCmdManagerImpl::dynamicChangeEnded(MgView* view, bool apply)
{
    bool changed = false;
    if (_cmdname == MgCmdSelect::Name() && view) {
        MgCmdSelect* sel = (MgCmdSelect*)getCommand();
        changed = sel && sel->dynamicChangeEnded(view, apply);
    }
    return changed;
}

MgSelection* MgCmdManagerImpl::getSelection()
{
    return (MgCmdSelect*)findCommand(MgCmdSelect::Name());
}

MgActionDispatcher* MgCmdManagerImpl::getActionDispatcher()
{
    return this;
}

bool MgCmdManagerImpl::doContextAction(const MgMotion* sender, int action)
{
    return action != 0 && doAction(sender, action);
}

MgSnap* MgCmdManagerImpl::getSnap()
{
    return this;
}
