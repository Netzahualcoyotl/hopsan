/*-----------------------------------------------------------------------------
 This source file is part of Hopsan NG

 Copyright (c) 2011
    Mikael Axin, Robert Braun, Alessandro Dell'Amico, Björn Eriksson,
    Peter Nordin, Karl Pettersson, Petter Krus, Ingo Staack

 This file is provided "as is", with no guarantee or warranty for the
 functionality or reliability of the contents. All contents in this file is
 the original work of the copyright holders at the Division of Fluid and
 Mechatronic Systems (Flumes) at Linköping University. Modifying, using or
 redistributing any part of this file is prohibited without explicit
 permission from the copyright holders.
-----------------------------------------------------------------------------*/

//!
//! @file   LoadExternal.h
//! @author <peter.nordin@liu.se>
//! @date   2009-12-22
//!
//! @brief Contains the ExternalLoader class
//!
//$Id$

#ifndef LOADEXTERNAL_H
#define LOADEXTERNAL_H

#include "win32dll.h"
#include "Component.h"

namespace hopsan {

//Forward Declaration
class HopsanCoreMessageHandler;

class LoadedLibInfo
{
public:
    void* mpLib;
    HString mLibName;
    std::vector<HString> mRegistredComponents;
    std::vector<HString> mRegistredNodes;
};

//! @brief This class handles loading and unloading of external component and node libs
class LoadExternal
{
private:
    ComponentFactory *mpComponentFactory;
    NodeFactory *mpNodeFactory;
    HopsanCoreMessageHandler *mpMessageHandler;

    typedef std::map<HString, LoadedLibInfo> LoadedExtLibsMapT;
    LoadedExtLibsMapT mLoadedExtLibsMap;

public:
    LoadExternal(ComponentFactory* pComponentFactory, NodeFactory* pNodefactory, HopsanCoreMessageHandler *pMessenger);
    bool load(const HString &rLibpath);
    bool unLoad(const HString &rLibpath);
    void setFactory();
    void getLoadedLibNames(std::vector<HString> &rLibNames);
    void getLibContents(const HString &rLibpath, std::vector<HString> &rComponents, std::vector<HString> &rNodes);
    void getLibPathByTypeName(const HString &rTypeName, HString &rLibPath);
};
}

#endif // LOADEXTERNAL_H