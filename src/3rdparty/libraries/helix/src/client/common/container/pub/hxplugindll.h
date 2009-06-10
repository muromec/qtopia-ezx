/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxplugindll.h,v 1.5 2007/07/06 21:57:57 jfinnecy Exp $
 * 
 * Portions Copyright (c) 1995-2004 RealNetworks, Inc. All Rights Reserved.
 * 
 * The contents of this file, and the files included with this file,
 * are subject to the current version of the RealNetworks Public
 * Source License (the "RPSL") available at
 * http://www.helixcommunity.org/content/rpsl unless you have licensed
 * the file under the current version of the RealNetworks Community
 * Source License (the "RCSL") available at
 * http://www.helixcommunity.org/content/rcsl, in which case the RCSL
 * will apply. You may also obtain the license terms directly from
 * RealNetworks.  You may not use this file except in compliance with
 * the RPSL or, if you have a valid RCSL with RealNetworks applicable
 * to this file, the RCSL.  Please see the applicable RPSL or RCSL for
 * the rights, obligations and limitations governing use of the
 * contents of the file.
 * 
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU General Public License Version 2 (the
 * "GPL") in which case the provisions of the GPL are applicable
 * instead of those above. If you wish to allow use of your version of
 * this file only under the terms of the GPL, and not to allow others
 * to use your version of this file under the terms of either the RPSL
 * or RCSL, indicate your decision by deleting the provisions above
 * and replace them with the notice and other provisions required by
 * the GPL. If you do not delete the provisions above, a recipient may
 * use your version of this file under the terms of any one of the
 * RPSL, the RCSL or the GPL.
 * 
 * This file is part of the Helix DNA Technology. RealNetworks is the
 * developer of the Original Code and owns the copyrights in the
 * portions it created.
 * 
 * This file, and the files included with this file, is distributed
 * and made available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY
 * KIND, EITHER EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS
 * ALL SUCH WARRANTIES, INCLUDING WITHOUT LIMITATION, ANY WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET
 * ENJOYMENT OR NON-INFRINGEMENT.
 * 
 * Technology Compatibility Kit Test Suite(s) Location:
 *    http://www.helixcommunity.org/content/tck
 * 
 * Contributor(s):
 * 
 * ***** END LICENSE BLOCK ***** */

#ifndef HXPLUGINDLL_H__
#define HXPLUGINDLL_H__

#include "hxstring.h"
#include "hxtypes.h"
#include "hxcom.h"
#include "unkimp.h"
#include "hxslist.h"

class HXPluginManager;
class HXPluginArchiveReader;
class HXPluginArchiveWriter;

typedef HX_RESULT (HXEXPORT_PTR FPCREATEINSTANCE) (IUnknown** /*OUT*/ ppIUnknown);
typedef HX_RESULT (HXEXPORT_PTR FPSHUTDOWN) ();
typedef HX_RESULT (HXEXPORT_PTR FPCANUNLOAD2) ();


//
// used to identify plugin type when plugin object is written to/read from archive
//
enum
{
    ARCHIVE_ID_OTHER_DLL = 1,
    ARCHIVE_ID_PLUGIN_DLL
};



// class HXOtherDLL
class HXOtherDLL
: public CUnknownIMP
{
public:
    DECLARE_UNKNOWN_NOCREATE(HXPluginDLL)
public:
    HXOtherDLL(const char* pszFileName, const char* pszMountPoint);
    HXOtherDLL(const char* pszMountPoint, HXPluginArchiveReader& ar);

    const CHXString& GetMountPoint() const;
    const CHXString& GetFileName() const;

    void Archive(HXPluginArchiveWriter& ar);

private:
    CHXString m_strFileName;
    CHXString m_strMountPoint;
};

inline 
const CHXString& HXOtherDLL::GetMountPoint() const { return m_strMountPoint; }
inline
const CHXString& HXOtherDLL::GetFileName() const { return m_strFileName; }








// class HXPluginDLL
class HXPluginDLL 
: public CUnknownIMP
{
public:
    DECLARE_UNKNOWN_NOCREATE(HXPluginDLL)
public:
    HXPluginDLL(IUnknown* pContext, const char* pszFileName, const char* pszMountPoint);
    HXPluginDLL(IUnknown* pContext, const char* pszMountPoint, HXPluginArchiveReader& ar);
    ~HXPluginDLL();
    void Archive(HXPluginArchiveWriter& ar);

    HX_RESULT Load();
    HX_RESULT Unload(bool bForce = false);
    HXBOOL IsLoaded();

    HX_RESULT CreateInstance( IUnknown** ppUnk, UINT32 uIndex );

    const CHXString& GetMountPoint() const;
    const CHXString& GetFileName() const;
    UINT32 GetNumPlugins();
    void AddPlugins(CHXSimpleList& list);
    

private:
// implementation
    void Init(IUnknown* pContext);
    HX_RESULT CreatePlugins();
    void CreateComponentPlugins(IHXComponentPlugin* pComponentPlugin);

private:
    FPCREATEINSTANCE	m_fpCreateInstance;
    FPSHUTDOWN		m_fpShutdown;
    FPCANUNLOAD2	m_fCanUnload;

    CHXString           m_strMountPoint;
    CHXString           m_strFileName;
 
    UINT16		m_pluginCount;
    bool		m_bHasFactory;
    bool		m_bLoaded;

    DLLAccess*		m_pDLLAccess;

    CHXSimpleList	m_plugins;
    IUnknown*           m_pContext;
    IHXCommonClassFactory*  m_pClassFactory;

};

inline
const CHXString& HXPluginDLL::GetMountPoint() const 
{ 
    return m_strMountPoint; 
}

inline
const CHXString& HXPluginDLL::GetFileName() const 
{ 
    return m_strFileName; 
}

inline
UINT32 HXPluginDLL::GetNumPlugins() 
{ 
    return m_pluginCount; 
}




#endif /* HXPLUGINDLL_H__ */
