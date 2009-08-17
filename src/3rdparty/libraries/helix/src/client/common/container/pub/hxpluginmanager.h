/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxpluginmanager.h,v 1.5 2006/10/30 22:00:24 gwright Exp $
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
 * terms of the GNU General Public License Version 2 or later (the
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

#ifndef _BASEHAND_H_
#define _BASEHAND_H_

#include "unkimp.h"
#include "hxslist.h"
#include "hxmap.h"
#include "hxphand.h" //IHXPluginEnumerator
#include "hxplugn.h" //IHXPlugin2Handler, IHXPluginHandler3, IHX4PluginHandler, IHXPlugin5Handler1

typedef HX_RESULT (HXEXPORT_PTR FPCREATEINSTANCE) (IUnknown** /*OUT*/ ppIUnknown);
typedef HX_RESULT (HXEXPORT_PTR FPSHUTDOWN) ();
typedef HX_RESULT (HXEXPORT_PTR FPCANUNLOAD2) ();

class CFindFile;
class HXPluginArchiveReader;
class HXPluginEnumerator;
class HXPlugin;


// class HXPluginManager
class HXPluginManager 
: public CUnknownIMP
, public IHXPluginEnumerator
, public IHXPlugin2Handler
, public IHXPluginHandler3
{
    class HXPluginMountPoint;
    class PluginDLL;
    
    friend class HXPluginEnumerator;
    friend class HXPluginDLL;
    friend class HXPlugin;

public:
   
    enum eValueTypes
    {
	eString,
	eBuffer,
	eInt
    };


    DECLARE_UNKNOWN_NOCREATE( HXPluginManager )
    DECLARE_COM_CREATE_FUNCS( HXPluginManager )

    HXPluginManager();
    ~HXPluginManager();

    //
    //	IHXPluginEnumerator
    //
    STDMETHOD_(ULONG32,GetNumOfPlugins)	(THIS);
    STDMETHOD(GetPlugin)   (THIS_
			    ULONG32	    /*IN*/  ulIndex,
			    REF(IUnknown*)  /*OUT*/ pPlugin);

    //
    //	IHXPlugin2Handler
    //
    STDMETHOD(Init)    (THIS_ IUnknown* pContext);
    STDMETHOD_(ULONG32,GetNumOfPlugins2)    (THIS);
    STDMETHOD(GetPluginInfo)	(THIS_
				UINT32 unIndex,
				REF(IHXValues*) /*OUT*/ Values) ;
    STDMETHOD(FlushCache)	(THIS);
    STDMETHOD(SetCacheSize)	(THIS_ ULONG32 nSizeKB);
    STDMETHOD(ReadFromRegistry)(THIS);
    STDMETHOD(GetInstance) (THIS_ UINT32 index, REF(IUnknown*) ppUnknown);
    STDMETHOD(FindIndexUsingValues)		    (THIS_ IHXValues*,
						    REF(UINT32) unIndex);
    STDMETHOD(FindPluginUsingValues)(THIS_ IHXValues*, REF(IUnknown*) pUnk);
    STDMETHOD(FindIndexUsingStrings)		    (THIS_ char* PropName1,
						    char* PropVal1,
						    char* PropName2,
						    char* PropVal2,
						    char* PropName3,
						    char* PropVal3,
						    REF(UINT32) unIndex);

    STDMETHOD(FindPluginUsingStrings)		    (THIS_ char* PropName1,
						    char* PropVal1,
						    char* PropName2,
						    char* PropVal2,
						    char* PropName3,
						    char* PropVal3,
						    REF(IUnknown*) pUnk);

    STDMETHOD(FindImplementationFromClassID)(REFGUID GUIDClassID,REF(IUnknown*) pIUnknownInstance);
    STDMETHOD(Close)		(THIS);
    STDMETHOD(SetRequiredPlugins) (THIS_ const char** ppszRequiredPlugins);

    //
    // IHXPluginHandler3
    //
    STDMETHOD( RegisterContext )( THIS_ IUnknown* pContext );
    STDMETHOD( AddPluginMountPoint )( THIS_ const char* pName, UINT32 majorVersion,
					    UINT32 minorVersion, IHXBuffer* pPath );
    STDMETHOD( RefreshPluginMountPoint )( THIS_ const char* pName );
    STDMETHOD( RemovePluginMountPoint )( THIS_ const char* pName );
    STDMETHOD( FindImplementationFromClassID )( THIS_ REFGUID GUIDClassID,
						REF(IUnknown*) pIUnknownInstance,
						IUnknown* pIUnkOuter, IUnknown* pContext );
    STDMETHOD( FindCLSIDFromName )( THIS_ const char* pName, REF(IHXBuffer*) pCLSID );
    STDMETHOD(FindGroupOfPluginsUsingValues)(THIS_ IHXValues* pValues,
				    REF(IHXPluginSearchEnumerator*) pIEnumerator);

    STDMETHOD(FindGroupOfPluginsUsingStrings)(THIS_ char* PropName1,
				    char* PropVal1,
				    char* PropName2,
				    char* PropVal2,
				    char* PropName3,
				    char* PropVal3,
				    REF(IHXPluginSearchEnumerator*) pIEnumerator);
    STDMETHOD(GetPlugin)(THIS_ ULONG32 ulIndex,
				    REF(IUnknown*) pIUnkResult,
				    IUnknown* pIUnkOuter );
    STDMETHOD(FindPluginUsingValues)(THIS_ IHXValues*,
				    REF(IUnknown*) pIUnkResult,
				    IUnknown* pIUnkOuter );

    STDMETHOD(FindPluginUsingStrings)(THIS_ char* PropName1,
				    char* PropVal1,
				    char* PropName2,
				    char* PropVal2,
				    char* PropName3,
				    char* PropVal3,
				    REF(IUnknown*) pIUnkResult,
				    IUnknown* pIUnkOuter );

    STDMETHOD( UnloadPluginFromClassID )( THIS_ REFGUID GUIDClassID );
    STDMETHOD (UnloadPackageByName) (const char* pName);


public:
 
    HX_RESULT		    Stat(const char* pszFilename, struct stat* pStatBuffer);

    HX_RESULT               FindGroupOfPluginsUsingBuffers(char* PropName1,
                                                           void* PropVal1,
						           REF(IHXPluginSearchEnumerator*) pIEnumerator);

    HX_RESULT		    FindGroupOfPluginsUsingStrings(char* PropName1,
						    char* PropVal1,
						    char* PropName2,
						    char* PropVal2,
						    char* PropName3,
						    char* PropVal3,
						    REF(HXPluginEnumerator*) pEnumerator);

    HX_RESULT		    FindGroupOfPluginsUsingValues(IHXValues* pValues,
						    REF(HXPluginEnumerator*) pEnumerator);

    HX_RESULT ReloadPluginsNoPropagate();
    HX_RESULT ReloadPlugins();
    HX_RESULT ReloadPlugins(const char* pMountPoint);

private:
     // implementation
    HX_RESULT ReloadPluginsWithFindFile(
               const char* pMountPoint, CFindFile* pFileFinder,
               IHXBuffer* pPathBuffer, const char* pszPluginDir);

    HX_RESULT SaveToArchive(const char* pszArchiveFile);
    HX_RESULT LoadFromArchive(const char* pszArchiveFile, const char* pszMountPoint);
    HX_RESULT LoadPluginDLLFromArchive(const char* pszMountPoint, HXPluginArchiveReader& ar);
    HX_RESULT LoadOtherDLLFromArchive(const char* pszMountPoint, HXPluginArchiveReader& ar);
    bool DoesDLLExist(const char* pszName, const char* pszMountPoint);    
#ifdef HELIX_CONFIG_UNLOAD_DEAD_DLLS
    void UnloadDeadDLLs();
#endif
    void RebuildPluginList();
    HX_RESULT AddToValues(IHXValues*, char* pPropName, void* pPropVal, eValueTypes eValueType);

private:
    CHXMapStringToString    m_mountPoints;
    CHXSimpleList	    m_pluginDlls;
    CHXSimpleList	    m_otherDlls;
    CHXSimpleList	    m_plugins;

    IUnknown*		    m_pContext;
    IHXCommonClassFactory*  m_pClassFactory;
    
};



// class HXPluginEnumerator
class HXPluginEnumerator 
: public CUnknownIMP
, public IHXPluginSearchEnumerator
{
public: 
    DECLARE_UNKNOWN_NOCREATE( HXPluginEnumerator )
public:
    HXPluginEnumerator();
    virtual ~HXPluginEnumerator();

    //
    // IHXPluginSearchEnumerator
    //
    STDMETHOD_(UINT32, GetNumPlugins)(THIS);

    STDMETHOD_(void, GoHead)(THIS);
    STDMETHOD(GetNextPlugin)( THIS_ REF(IUnknown*) pIUnkResult, 
				    IUnknown* pIUnkOuter );

    STDMETHOD(GetNextPluginInfo)( THIS_ REF(IHXValues*) pRetValues );
    
    STDMETHOD(GetPluginAt)( THIS_ UINT32 index, 
				    REF(IUnknown*) pIUnkResult, 
				    IUnknown* pIUnkOuter );

    STDMETHOD(GetPluginInfoAt)( THIS_ UINT32 index, 
				    REF(IHXValues*) pRetValues );

public:
    void Add(HXPlugin* pPlugin);

private:
    CHXSimpleList   m_plugins;
    UINT32	    m_nIndex;

};


#endif /* _BASEHAND_H_ */
