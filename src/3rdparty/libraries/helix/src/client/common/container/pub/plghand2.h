/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: plghand2.h,v 1.15 2006/08/16 18:50:47 gwright Exp $
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

#ifndef _PLGNHAND_H_
#define _PLGNHAND_H_

#include "hxslist.h"
#include "hxplugn.h"
#include "hxstring.h"
#include "hxguidmap.h"
#include "hxprefs.h"
#include "hxengin.h"
#include "hxcomm.h" 
#include "hxerror.h"
#include "hxphand.h"

#include "dbindex.h"

#include "unkimp.h"

typedef HX_RESULT (HXEXPORT_PTR FPCREATEINSTANCE) (IUnknown** /*OUT*/ ppIUnknown);
typedef HX_RESULT (HXEXPORT_PTR FPSHUTDOWN) ();
typedef HX_RESULT (HXEXPORT_PTR FPCANUNLOAD2) ();

typedef ULONG32	    (STDMETHODCALLTYPE *FPWatchRef)(void*);
typedef HX_RESULT   (STDMETHODCALLTYPE *FPQueryInterfaceWatch)(void*, REFIID riid,void** ppvObj);

// This is the new and improved plugin handler. 
// What is better in this version 
//  (1)	Unified Plugins No longer do you have to modify code in seven places when a bug 
//	appears. Simply modify the Plugin Class. It is the heart of the plugin Handler.
//  (2)	The plugin handler now has rudimentary ref counting of the DLLs which it loads.
//	This means that we cannot (almost) safely shutdown the player. Further, we can
//	ALMOST unload DLLs when they are not required anymore. 

class Plugin2Handler;
class DLLAccess;
class CPluginEnumerator;
class CPluginInfoWriter;

// you want GUIDS? WE have GUIDS!
DEFINE_GUID(IID_IHXPluginWatcherResponse, 0x00000C08, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59);

DECLARE_INTERFACE_(IHXPluginWatcherResponse, IUnknown)
{
    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /************************************************************************
     *	Method:
     *	    IHXPluginWatcherResponse::AllObjectsDeleted
     *	Purpose:
     *	    This function will be called all objects of the given class are
     *	    destroyed. 
     */
    STDMETHOD(AllObjectsDeleted)		(THIS_ void*) PURE;
};

class Plugin2Handler :	public CUnknownIMP,
			public IHXPluginEnumerator, 
			public IHXPluginReloader,
			public IHXPlugin2Handler,
			public IHXPlugin2HandlerEnumeratorInterface,
		    	public IHXPluginHandler3,
		    	public IHXPluginDatabase,
		    	public IHXCallback
{
    // forward declarations?
    // see note below   class PluginMonitor;	    

    class PreferenceEnumerator;
    class PluginMountPoint;
    class PluginDLL;
    class Plugin;
    class OtherDLL;
    class PluginSupportingGUID;
    
    friend class CPluginEnumerator;
    friend class PreferenceEnumerator;
    friend class PluginDLL;
    friend class Plugin;
    friend class PluginSupportingGUID;
    friend class OtherDLL;

public:
    enum    Errors
    {
	NO_ERRORS = 0,
	PLUGIN_NOT_FOUND,
	MEMORY_ERROR,
	CANT_OPEN_DLL,
	BAD_DLL,
	CREATE_INSTANCHXR_FAILURE,
	CANT_DETERMINE_PLUGIN_DIR,
	CANT_OPEN_PLUGIN_DIR,
	BAD_PLUGIN,
	INVALID_CONTEXT,
	CANT_GET_FILE_FORMAT_INFO,
	CANT_GET_RENDERER_INFO,
	CANT_GET_FILE_SYSTEM_INFO,
	CANT_LOAD_INTERFACE,
	SHORT_NAME_NOT_FOUND,
	PLUGIN_ALREADY_HAS_MOUNT_POINT,
	INVALID_SHORT_NAME,
	BAD_REGISTRY_HANDLE,
	PLUGIN_DIR_NOT_SAME,
	REQUIRED_PLUGIN_NOT_LOADED,
	NO_HX_CREATE_INSTANCE,
	AGGREGATION_NOT_SUPPORTED
    };

    enum eValueTypes
    {
	eString,
	eBuffer,
	eInt
    };


    DECLARE_UNKNOWN_NOCREATE( Plugin2Handler )
    DECLARE_COM_CREATE_FUNCS( Plugin2Handler )

    Plugin2Handler();
    ~Plugin2Handler();

    /*
     *	IHXCallback methods
     */
    STDMETHOD ( Func ) ( THIS );

    /*
     *	IHXPluginEnumerator methods
     */

    /************************************************************************
     *	Method:
     *	    IHXPluginEnumerator::GetNumOfPlugins
     *
     *	Purpose:    
     *	    return the number of plugins available
     *
     */
    STDMETHOD_(ULONG32,GetNumOfPlugins)	(THIS);

    /************************************************************************
     *	Method:
     *	    IHXPluginEnumerator::GetPlugin
     *	Purpose:
     *	    return an instance(IUnknown) of the plugin
     *
     */
    STDMETHOD(GetPlugin)   (THIS_
			    ULONG32	    /*IN*/  ulIndex,
			    REF(IUnknown*)  /*OUT*/ pPlugin);

    /*
     *	IHXPluginReloader methods
     */

    /************************************************************************
     *	Method:
     *	    IHXPluginReloader::ReloadPlugins
     *
     *	Purpose:    
     *	    reloads plugins 
     *
     */
    STDMETHOD(ReloadPlugins)	(THIS);


    /*
     *	IHXPlugin2Handler
     */

     /************************************************************************
     *	Method:
     *	    IHXPlugin2Handler::Init
     *
     *	Purpose:    
     *	    Specifies the context and sets the Plugin2Handler in motion.
     *
     */
    STDMETHOD(Init)    (THIS_ IUnknown* pContext);

    /************************************************************************
     *	Method:
     *	    IHXPlugin2Handler::GetNumPlugins2
     *
     *	Purpose:    
     *	    Gets the info of a particular plugin.
     *
     */
    STDMETHOD_(ULONG32,GetNumOfPlugins2)    (THIS);
    
    /************************************************************************
     *	Method:
     *	    IHXPlugin2Handler::GetPluginInfo
     *
     *	Purpose:    
     *	    Gets the info of a particular plugin.
     *
     */

    STDMETHOD(GetPluginInfo)	(THIS_ 
				UINT32 unIndex, 
				REF(IHXValues*) /*OUT*/ Values) ;

    /************************************************************************
     *	Method:
     *	    IHXPlugin2Handler::FlushCache()
     *
     *	Purpose:    
     *	    Flushes the LRU cache -- Unloads all DLLs from memory 
     *	    which currenltly have a refcount of 0.
     */

    STDMETHOD(FlushCache)	(THIS);

    /************************************************************************
     *	Method:
     *	    IHXPlugin2Handler::SetCacheSize
     *
     *	Purpose:    
     *	    This function sets the size of the Cache. The cache is 
     *	    initally set to 1000KB. To disable the cache simply set
     *		the size to 0.If the cache is disabled a DLL will be 
     *	    unloaded whenever it's refcount becomes zero. Which MAY
     *		cause performance problems.
     */

    STDMETHOD(SetCacheSize)	(THIS_ ULONG32 nSizeKB);

    /************************************************************************
     *	Method:
     *	    IHXPlugin2Handler::ReadFromRegistry
     *
     *	Purpose:    
     *	    Reload the DLL information from the registery. It will also 
     *	    Check to see if this information is valid, and if 
     */
    STDMETHOD(ReadFromRegistry)(THIS);

    /************************************************************************
     *	Method:
     *	    IHXPlugin2Handler::GetInstance
     *
     *	Purpose:    
     *	    
     *	    This function will return a plugin instance given a plugin index.
     *		
     */
    
    STDMETHOD(GetInstance) (THIS_ UINT32 index, REF(IUnknown*) ppUnknown); 
    
    /************************************************************************
     *	Method:
     *	    IHXPlugin2Handler::FindReturnIndexUsingValues
     *
     *	Purpose:    
     *	    Finds a plugin  which matches the set of values given. An index
     *	    is returned which can be used to either get the values (using 
     *	    GetPluginInfo) or an instance can be created using GetPluing(). 	
     * 
     */

    STDMETHOD(FindIndexUsingValues)		    (THIS_ IHXValues*, 
						    REF(UINT32) unIndex);

    /************************************************************************
     *	Method:
     *	    IHXPlugin2Handler::FindReturnPluginUsingValues
     *
     *	Purpose:    
     *	    Finds a plugin  which matches the set of values given. A Plugin
     *	    instance is returned. 
     *	    
     */

    STDMETHOD(FindPluginUsingValues)		    (THIS_ IHXValues*, 
						    REF(IUnknown*) pUnk);
    
    /************************************************************************
     *	Method:
     *	    IHXPlugin2Handler::FindReturnIndexUsingStrings
     *
     *	Purpose:    
     *	    Finds a plugin  which matches the set of values given. An index
     *	    is returned which can be used to either get the values (using 
     *	    GetPluginInfo) or an instance can be created using GetPluing(). 	
     *	    NOTE: that a max of two values may be given.
     */
    STDMETHOD(FindIndexUsingStrings)		    (THIS_ char* PropName1, 
						    char* PropVal1, 
						    char* PropName2, 
						    char* PropVal2, 
						    char* PropName3, 
						    char* PropVal3, 
						    REF(UINT32) unIndex);

    /************************************************************************
     *	Method:
     *	    IHXPlugin2Handler::FindReturnPluginUsingStrings
     *
     *	Purpose:    
     *	    Finds a plugin  which matches the set of values given. A Plugin
     *	    instance is returned. 
     *	    NOTE: that a max of two values may be given.
     */

    STDMETHOD(FindPluginUsingStrings)		    (THIS_ char* PropName1, 
						    char* PropVal1, 
						    char* PropName2, 
						    char* PropVal2, 
						    char* PropName3, 
						    char* PropVal3, 
						    REF(IUnknown*) pUnk);

    /************************************************************************
     *	Method:
     *	    IHXPlugin2Handler::FindImplementationFromClassID
     *
     *	Purpose:    
     *	    Finds a CommonClassFactory plugin which supports the 
     *	    ClassID given. An instance of the Class is returned. 
     */

    STDMETHOD(FindImplementationFromClassID)
    (
	REFGUID GUIDClassID, 
	REF(IUnknown*) pIUnknownInstance
    );

    /************************************************************************
     *	Method:
     *	    IHXPlugin2Handler::Close
     *
     *	Purpose:    
     *	    A function which performs all of the functions of delete.
     *	    
     *
     */
    
    STDMETHOD(Close)		(THIS); 

    /************************************************************************
     *	Method:
     *	    IHXPlugin2Handler::SetRequiredPlugins
     *
     *	Purpose:    
     *	    This function sets the required plugin list
     *	    
     *
     */

    STDMETHOD(SetRequiredPlugins) (THIS_ const char** ppszRequiredPlugins);

    /************************************************************************
     *	Method:
     *	    IHXPlugin2HandlerEnumeratorInterface::GetNumPluginsSupporting
     *
     *	Purpose:    Required for the plugin enumerator.
     *	    
     *	    
     *
     */

    STDMETHOD(GetNumPluginsSupporting) (THIS_ REFIID iid, REF(UINT32) nNumPlugins);

    /************************************************************************
     *	Method:
     *	    IHXPlugin2HandlerEnumeratorInterface::GetPluginIndexSupportingIID
     *
     *	Purpose:    Required for the plugin enumerator.
     *	    
     *	    
     *
     */
    STDMETHOD(GetPluginIndexSupportingIID) (THIS_ REFIID iid, UINT32 nPluginIndex, REF(UINT32) nIndexOut);

    /************************************************************************
     *	Method:
     *	    IHXPlugin2HandlerEnumeratorInterface::AddSupportedIID
     *
     *	Purpose:    Required for the plugin enumerator.
     *	    
     *	    
     *
     */
    STDMETHOD(AddSupportedIID) (THIS_ REFIID iid);



    /************************************************************************
     *	Method:
     *	    IHXPluginHandler3::RegisterContext
     *
     *	Purpose:    
     *	    Sets up the context without loading any plugin info
     *
     */
    STDMETHOD( RegisterContext )( THIS_ IUnknown* pContext );

    /************************************************************************
     *	Method:
     *	    IHXPluginHandler3::AddPluginMountPoint
     *
     *	Purpose:    
     *
     */
    STDMETHOD( AddPluginMountPoint )( THIS_ const char* pName, UINT32 majorVersion,
					    UINT32 minorVersion, IHXBuffer* pPath );


    /************************************************************************
     *	Method:
     *	    IHXPluginHandler3::RefreshPluginMountPoint
     *
     *	Purpose:    
     *
     */
    STDMETHOD( RefreshPluginMountPoint )( THIS_ const char* pName );


    /************************************************************************
     *	Method:
     *	    IHXPluginHandler3::RemovePluginMountPoint
     *
     *	Purpose:    
     *
     */
    STDMETHOD( RemovePluginMountPoint )( THIS_ const char* pName );


    /************************************************************************
     *	Method:
     *	    IHXPluginHandler3::FindImplementationFromClassID
     *
     *	Purpose:    
     *	    Finds a CommonClassFactory plugin which supports the 
     *	    ClassID given. An instance of the Class is returned. 
     *	    The plugin instance is initialized with the specified 
     *	    context
     */

    STDMETHOD( FindImplementationFromClassID )( THIS_ REFGUID GUIDClassID, 
		    REF(IUnknown*) pIUnknownInstance, IUnknown* pIUnkOuter, IUnknown* pContext );

    /************************************************************************
     *	Method:
     *	    IHXPluginHandler3::FindCLSIDFromName
     *
     *	Purpose:    
     *
     *	    Maps a text name to a CLSID based on information from 
     *	    component plugins
     */
    STDMETHOD( FindCLSIDFromName )( THIS_ const char* pName, REF(IHXBuffer*) pCLSID );


    /************************************************************************
     *	Method:
     *	    IHXPluginHandler3::FindGroupOfPluginsUsingValues
     *
     *	Purpose:    
     *	    Builds a collection of plugins that match the criteria
     *
     */
    STDMETHOD(FindGroupOfPluginsUsingValues)(THIS_ IHXValues* pValues, 
				    REF(IHXPluginSearchEnumerator*) pIEnumerator);

    /************************************************************************
     *	Method:
     *	    IHXPluginHandler3::FindGroupOfPluginsUsingStrings
     *
     *	Purpose:    
     *	    Builds a collection of plugins that match the criteria
     *
     */
    STDMETHOD(FindGroupOfPluginsUsingStrings)(THIS_ char* PropName1, 
				    char* PropVal1, 
				    char* PropName2, 
				    char* PropVal2, 
				    char* PropName3, 
				    char* PropVal3, 
				    REF(IHXPluginSearchEnumerator*) pIEnumerator);


    /************************************************************************
     *	Method:
     *	    IHXPluginHandler3::GetPlugin
     *
     *	Purpose:    
     *	    Allocates a plugin based on index.  Supports aggregation
     *
     */
    STDMETHOD(GetPlugin)(THIS_ ULONG32 ulIndex,
				    REF(IUnknown*) pIUnkResult,
				    IUnknown* pIUnkOuter );

    /************************************************************************
     *	Method:
     *	    IHXPluginHandler3::FindPluginUsingValues
     *
     *	Purpose:    
     *	    Allocates a plugin based on criteria.  Supports aggregation
     *
     */
    STDMETHOD(FindPluginUsingValues)(THIS_ IHXValues*, 
				    REF(IUnknown*) pIUnkResult, 
				    IUnknown* pIUnkOuter );


    /************************************************************************
     *	Method:
     *	    IHXPluginHandler3::FindPluginUsingStrings
     *
     *	Purpose:    
     *	    Allocates a plugin based on criteria.  Supports aggregation
     *
     */
    STDMETHOD(FindPluginUsingStrings)(THIS_ char* PropName1, 
				    char* PropVal1, 
				    char* PropName2, 
				    char* PropVal2, 
				    char* PropName3, 
				    char* PropVal3, 
				    REF(IUnknown*) pIUnkResult, 
				    IUnknown* pIUnkOuter );

    /************************************************************************
     *	Method:
     *	    IHXPluginHandler3::UnloadPluginFromClassID
     *
     *	Purpose:    
     *	    Finds a plugin from the classID and unloads it if it supports CanUnload2
	 *		and returns TRUE in response to query
     */

    STDMETHOD( UnloadPluginFromClassID )( THIS_ REFGUID GUIDClassID );

    /************************************************************************
     *	Method:
     *	    IHXPluginHandler3::UnloadPackageByName
     *
     *	Purpose:    
     *	    finds a package from the name passed in and attempts to unload it.
     */
    STDMETHOD (UnloadPackageByName) (const char* pName);


    //------------------------------------ IHXPluginDatabase interface methods
    STDMETHOD( AddPluginIndex ) ( THIS_ const char* pKeyName, EPluginIndexType indexType, HXBOOL bScanExisting );
    STDMETHOD( RemovePluginIndex )( THIS_ const char* pKeyName );
    STDMETHOD( FindPluginInfoViaIndex )( THIS_ const char* pKeyName, const void* pValue, IHXValues** ppIInfo );
    STDMETHOD( FindPluginSetViaIndex )( THIS_ const char* pKeyName, const void* pValue, IHXPluginSearchEnumerator** ppIEnumerator );
    STDMETHOD( CreatePluginViaIndex )( THIS_ const char* pKeyName, const void* pValue, IUnknown** ppIUnkPlugin, IUnknown* pIUnkOuter );


    //------------------------------------ Class Methods

    Errors		    Stat(const char* pszFilename, struct stat* pStatBuffer);
    IHXBuffer*		    ChecksumFile(char* pszFileName, IHXBuffer* pPathBuffer);
    IHXBuffer*		    ConvertToAsciiString(char* pBuffer, UINT32 nBuffLen);

    IHXBuffer*		    GetPluginDir();
    IHXPreferences*	    GetPreferences();
    CHXSimpleList&	    GetCanUnload2DllList() { return( m_CanUnload2DllList ); }

    HX_RESULT		    FindGroupOfPluginsUsingStrings(char* PropName1, 
						    char* PropVal1, 
						    char* PropName2, 
						    char* PropVal2, 
						    char* PropName3, 
						    char* PropVal3, 
						    REF(CPluginEnumerator*) pEnumerator);

    HX_RESULT		    FindGroupOfPluginsUsingValues(IHXValues* pValues, 
						    REF(CPluginEnumerator*) pEnumerator);

    void		    ReportError( UINT8 severity, const char* pDLLName, const char* pDesc );

    static const char* const	zm_pszFileExtension;
    static const char* const	zm_pszDirectorySeperator;
    static const char* const	zm_pszValueSeperator;
    static const char* const	zm_pszListStart;
    static const char* const	zm_pszListEnd;
    static const char* const	zm_pszValueSeperator2;
    static const char   	zm_cDirectorySeperator;
    static const char* const	zm_pszKeyNameRegKey;
    static const char* const	zm_pszRegKeySeperator;
#if !defined(HELIX_CONFIG_NOSTATICS)
    static HXBOOL 		zm_bFasterPrefs;
#else
    static const HXBOOL 		zm_bFasterPrefs;
#endif

    HX_RESULT AddtoLRU(Plugin2Handler::PluginDLL* pDLL);
    HX_RESULT RemoveFromLRU(Plugin2Handler::PluginDLL* pDLL);

    HX_RESULT UpdateCache();
    HX_RESULT ReloadPluginsNoPropagate();
    HX_RESULT ReloadPluginsNoPropagate( PluginMountPoint* pMountPoint );
    void UnloadDeadDLLs(void);

private: 
    static HX_RESULT VerifyChecksum_ (const char*);

    IHXScheduler*  m_pIScheduler;
    CallbackHandle m_hScheduler;
    HXBOOL         m_bStatDllsOnStartup;

    enum
    {
//	kPingDuration = 1000	// USE ONLY FOR TESTING
	kPingDuration = 60000	// 60000ms = 1 minute
    };

    IHXBuffer*		    GetDefaultPluginDir();
    void		    WriteSupportedGUIDs();

    HX_RESULT		    ConnectPluginToDLL(Plugin2Handler::Plugin * pPlugin);
    Errors		    LoadDLL(char* pszDllName, PluginMountPoint* pMountPoint);
    void		    LoadPluginsFromComponentDLL( Plugin2Handler::PluginDLL* pPluginDll, 
							IHXComponentPlugin* pIIterator );

    // Methods to determine out of data DLLs
    HXBOOL		    FindPlugin(const char* pFileName, UINT32 nDLLIndex, REF(UINT32) nIndex);
    HX_RESULT		    AddToValues(IHXValues*, char* pPropName, char* pPropVal, eValueTypes eValueType);
    UINT32		    GetNumSupportedGUIDs();
    HX_RESULT		    RemoveDLLFromGUIDSupportLists(const char* pszFileName);
    HX_RESULT		    GetGUIDForIndex(UINT32 nIndex, REF(CHXString) sGUID);
    HX_RESULT		    AddSupportForGUID(const char* pszGUID, PluginDLL* pDLL, UINT32 nIndexInDLL);

    HXBOOL		    GetPluginFileInfo( REF(char*) pszCurrentPos, 
					       REF(char*) pszName, 
					       REF(char*) pszCheckSum, 
					       REF(HXBOOL) bFactory, 
					       REF(int) nDLLSize, 
					       REF(int) nNumberPlugins);

    HXBOOL		    GetNameValuePair(REF(char*) pszCurrentPos, REF(char*) pszName, REF(char*) pszValue);
    HXBOOL		    GetPluginFileInfo(REF(char*) pszCurrentPos, REF(Plugin2Handler::Plugin*) pPlugin);
    HXBOOL		    GetNextSupportingFile(REF(char*) pszCurrentPos, REF(char*) pszFileName, REF(UINT32) index);
    HXBOOL		    GetGUIDInfo(REF(char*) pszCurrentPos, PluginMountPoint* pMountPoint, REF(char*) pszGUID, REF(CHXSimpleList*) pList);
    void		    ReconnectDLL(const char* pszDLLName, Plugin2Handler::PluginDLL* pNewDLL);
    HXBOOL		    GetNonHXInfo(REF(char*) pszCurrentPos, PluginMountPoint* pMountPoint, REF(Plugin2Handler::OtherDLL*) pOtherData);
    void		    WriteHugePref( IHXPreferences* pPrefs, const char* pszKeyName, IHXBuffer* pBigBuffer);
    HX_RESULT		    ReadHugePref( IHXPreferences* pPrefs, const char* pszKeyName, REF(IHXBuffer*) pBigBuffer);
    void		    DeleteHugePref_ (IHXPreferences*, IHXPreferences3*, const char*);

    // Internal shared code
    HX_RESULT 		    FindImplementationFromClassIDInternal( 
					    REFGUID GUIDClassID, 
					    REF(IUnknown*) pIUnknownInstance, 
					    IUnknown* pContext );


    // New plugin info loading methods
    HX_RESULT ClearMountPoint_ (PluginMountPoint*);

    HX_RESULT RefreshPluginInfo( PluginMountPoint* pMountPoint );
    HX_RESULT WritePluginInfo( PluginMountPoint* pMountPoint );

    HX_RESULT ReadPluginInfoFast( PluginMountPoint* pMountPoint );
    HX_RESULT WritePluginInfoFast( PluginMountPoint* pMountPoint );

    HX_RESULT ReadPluginInfoSlow( PluginMountPoint* pMountPoint );
    HX_RESULT WritePluginInfoSlow( PluginMountPoint* pMountPoint );

    CPluginDatabaseIndex* FindDBIndex( const char* pKeyName );
    void AddPluginToIndices( Plugin2Handler::Plugin* pPlugin );
    void RemovePluginFromIndices( Plugin2Handler::Plugin* pPlugin );
    
    HX_RESULT FindIndexWithDescription(
                        CHXSimpleList* pPossibleValues,
                        CHXSimpleList* pPossibleIndexes,
                        const char* pDescription,
                        REF(UINT32) outPossibleIndex);
    HX_RESULT FindPluginWithDescription(
                        CHXSimpleList* pPossibleValues,
                        const char* pDescription,
                        REF(IUnknown*) pIUnkResult,
                        IUnknown* pIUnkOuter);


    CHXMapStringToOb	    m_MountPoints;
    CHXSimpleList	    m_PluginDLLList;
    CHXSimpleList	    m_PluginList;
    CHXSimpleList	    m_MiscDLLList;
    CHXSimpleList	    m_CanUnload2DllList;
    CHXMapStringToOb	    m_GUIDtoSupportList;
    CHXMapStringToOb	    m_dbIndices;

    CHXMapStringToOb	    m_GUIDSupportListIsValid;
    CHXMapStringToOb	    m_FileNameMap;
    
    IHXBuffer*		    m_pPluginDir;
    IHXPreferences*	    m_pPreferences;
    IHXErrorMessages*	    m_pErrorMessages;
    IUnknown*		    m_pContext;
    INT32		    m_nCacheSizeBites;
    CHXSimpleList	    m_DLL_LRUList;

    HX_RESULT		    ConstructRegKeyPath(char* pszPath, const char* pszNode0, 
						const char* pszNode1, const char* pszNode2,
						const char* pszNode3);

    class PluginMountPoint
    {
    public:
	PluginMountPoint( IUnknown* pContext, Plugin2Handler* pHandler,
			  const char* pName, 
			  UINT32 majorVersion, UINT32 minorVersion, 
			  IHXBuffer* pPath );
	~PluginMountPoint();
    
	STDMETHOD_(ULONG32,AddRef)( THIS );
	STDMETHOD_(ULONG32,Release)( THIS );
    
	void AddClient();
	INT32 RemoveClient();

	// XXXND  This is for backward compatibility
	HXBOOL IsHXCompliant();

	IHXPreferences* Prefs();
	IHXBuffer* Path();
	IHXBuffer* Name();
	    
    private:
	INT32 m_lRefCount;
	INT32 m_lClientCount;
    
	// XXXND  This is for backward compatibility
	HXBOOL m_bHXCompliant;

	IHXPreferences* m_pIPrefs;
	IHXBuffer* m_pIPath;
	IHXBuffer* m_pName;
    };


    class PluginDLL : public IUnknown
    {
    public:
	PluginDLL(const char* pszFileName, PluginMountPoint* pMountPoint, 
		    Plugin2Handler* pPlugin2Handler);

	~PluginDLL();

	/*
	 *	IUnknown methods
	 */
	STDMETHOD(QueryInterface)(THIS_ REFIID riid, void** ppvObj);
	STDMETHOD_(ULONG32,AddRef)	(THIS);
	STDMETHOD_(ULONG32,Release)	(THIS);


	Errors			Load(IUnknown* pContext);
	HX_RESULT		Unload(HXBOOL safe = TRUE);
	HXBOOL			IsLoaded();

	Errors			CreateInstance( IUnknown** ppUnk, UINT32 uIndex );

	UINT32			AddDLLReference();
	UINT32			ReleaseDLLReference();

	void			SetPref( int nNumberPlugins, char* pszCheckSum, int nDLLSize, HXBOOL bFactory );
	HX_RESULT		WritePref2( REF(CPluginInfoWriter) piw);
	HX_RESULT		WritePref( PreferenceEnumerator* pPrefEnum );
	HX_RESULT		ReadPref( PreferenceEnumerator* pPrefs );

	// Accessors
	void			SetHash(char* phash) {m_hash = phash;}
	void			SetFileSize(INT32 nSize) {m_nSizeBites = nSize;}

	PluginMountPoint*	GetMountPoint() { return m_pMountPoint; }
	IHXBuffer*		GetFileName();
	INT32			GetFileSize() { return m_nSizeBites; }
	void			SetNamespace(IHXBuffer* pNamespace);
	IHXBuffer*		GetNamespace();
	const char*		GetHash() { return (const char*) m_hash; }
	CHXString const&	GetPackageName () const { return m_packageName; }

	UINT32			GetNumPlugins() { return m_NumOfPlugins; }

	HXBOOL			DoesExist() { return m_bDoesExist; }

    private:
	FPCREATEINSTANCE	m_fpCreateInstance;
	FPSHUTDOWN		m_fpShutdown;
	FPCANUNLOAD2		m_fCanUnload;

	PluginMountPoint*	m_pMountPoint;
	IHXBuffer*		m_pFileName;
	IHXBuffer*		m_pNamespace;

	CHXString		m_packageName;
	CHXString		m_hash;
	LONG32			m_nSizeBites;
	INT32			m_lRefCount;
	UINT16			m_NumOfPlugins : 16;
	HX_BITFIELD		m_bHas_factory : 1;
	HX_BITFIELD		m_bLoaded : 1;
	HXBOOL			m_bDoesExist;

	DLLAccess*		m_pDLLAccess;
	UINT32			m_nActiveReferences; 
	Plugin2Handler*		m_pPlugin2Handler;
    };

    class Plugin :  public IHXPluginWatcherResponse
    {
	public:
	STDMETHOD(QueryInterface)	(THIS_
					REFIID riid,
					void** ppvObj);

	STDMETHOD_(ULONG32,AddRef)	(THIS);

	STDMETHOD_(ULONG32,Release)	(THIS);
	STDMETHOD(AllObjectsDeleted)	(THIS_ void*);


	Plugin(IUnknown* pContext);

	~Plugin();  

	// Exposed to the Plugin2Handler
	HXBOOL			    DoesMatch(IHXValues* pValues);
	HX_RESULT		    WritePref2(REF(CPluginInfoWriter) piw);

	HX_RESULT		    WritePref(PreferenceEnumerator* pPrefEnum);
	HX_RESULT		    ReadPref(PreferenceEnumerator* pPrefs);

	Errors			    GetValuesFromDLL(IHXPlugin* pHXPlugin);
	// Creates an instance of the top-level plugin object
	Errors			    GetPlugin( REF(IUnknown*) ppUnknown );
	// Checks to see if this is a component plugin and does the appropriate indirection
	Errors			    GetInstance(REF(IUnknown*) ppUnknown, IUnknown* pIUnkOuter = NULL );
	HX_RESULT		    GetPluginInfo(REF(IHXValues*));
	IHXBuffer*		    GetFileName();
	HXBOOL			    IsLoaded();
	HXBOOL			    DoesInfoNeedsRefresh() {return m_bInfoNeedsRefresh;}
	void			    SetInfoNeedsRefresh(HXBOOL bRefresh) { m_bInfoNeedsRefresh = bRefresh;}
	void			    SetDLL(PluginDLL * pPluginDll);
	PluginDLL*		    GetDLL() {return m_pPluginDLL;}
	void			    SetIndex(UINT16 nIndex);
	UINT16			    GetIndex() {return m_nPluginIndex;}

	void			    SetPropertyULONG32(char* , char*);
	void			    SetPropertyCString(char*, char*);
	void			    SetPropertyBuffer(char*, BYTE*, UINT32);

	// this causes an init plugins should be handled with care.
	HX_RESULT		    GetBandwidthInfo();			

	// FIX  This is to support the initialization of component plugins
	void InitializeComponentPlugin( IHXPlugin* pIPlugin, IHXValues* pIValues );

	// void* because the client doesn't have these
	// XXXAH not supported currently...  They were not in the old plugin handler either...
	// must be some code in the server that talks directly to this (BLEECH!)
	void*			    m_process;  

    private:
	LONG32			    m_lRefCount;
	UINT16			    m_nPluginIndex;		
	PluginDLL*		    m_pPluginDLL;		
	IHXValues*		    m_pValues;
	HX_BITFIELD		    m_bCanUnload : 1;
	HX_BITFIELD		    m_bInfoNeedsRefresh : 1;
	IUnknown*		    m_pContext;


	// Methods to retreive from the DLL
	Errors	    GetBasicValues(IHXPlugin* pHXPlugin);
	Errors	    GetExtendedValues(IHXPlugin* pHXPlugin);

	// Support Functions
	HX_RESULT   CatPropertiesULONG32(   REF(IHXBuffer*) pBuffer, 
					    const char* pPropName, 
					    ULONG32 nValue);
	HX_RESULT   CatPropertiesCString(   REF(IHXBuffer*) pBuffer, 
					    const char* pPropName, 
					    IHXBuffer* pValue);
	HXBOOL	    GetNextValueFromString( REF(char*) pszValues, 
					    REF(UINT32) nType, 
					    REF(IHXBuffer*) pValueNameBuffer, 
					    REF(IHXBuffer*) pValueBuffer, 
					    REF(ULONG32) nValue);
	HXBOOL	    AreBufferEqual(	    IHXBuffer* pBigBuff, 
					    IHXBuffer* pSmallBuff);
	Errors	    CreateWatcher(IUnknown* pUnknown);
    };

    class PluginSupportingGUID
    {
    public:
	    CHXString			m_filename;
	    UINT32			m_nIndexInDLL;
	    PluginMountPoint*		m_pMountPoint;
    };

    class OtherDLL 
    {
    public:
	    CHXString			m_filename;
	    CHXString			m_fileChecksum;
	    PluginMountPoint*		m_pMountPoint;
    };

    class PreferenceEnumerator
    {
	public:
	    HX_RESULT		    BeginSubPref(const char* pszSubPref);
	    HX_RESULT		    EndSubPref();
	    HX_RESULT		    WriteSubPref(const char* pszSubName, IHXBuffer* pBuffer);
	    HX_RESULT		    ReadPref(const char* pszSubName, REF(IHXBuffer*) /*OUT*/ pBuffer);
	    HX_RESULT		    GetPrefKey(UINT32 nIndex, IHXBuffer*& pBuffer);
	    HX_RESULT		    ResetPropNameList();

	    PreferenceEnumerator(IUnknown* pContext);
	    ~PreferenceEnumerator();


	private:

	    IUnknown*			m_pContext;
	    CHXString			m_RegKey;
	    CHXSimpleList		m_ListofProps;
	    IHXPreferenceEnumerator*	m_pPrefEnum;
	    IHXPreferences*		m_pPreferences;
    };

    HXBOOL DoChecksumsMatch(const char* pszOrigMD5,
                            char* pszFilename,
                            IHXBuffer* pPath);
    
    
};


class CPluginEnumerator :
    public CUnknownIMP,
    public IHXPluginSearchEnumerator
{
public: 
    CPluginEnumerator();
    virtual ~CPluginEnumerator();

    DECLARE_UNKNOWN_NOCREATE( CPluginEnumerator )

    /*
     * IHXPluginSearchEnumerator
     */
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

    //---------------- Class methods
    void Add(Plugin2Handler::Plugin* pPlugin);

    // FIX This is for backwards compatibility, and should be removed
    HX_RESULT GetNext(REF(IUnknown*) pRetUnk);

protected:

private:
    CHXSimpleList   m_ListOfPlugins;
    UINT32	    m_nIndex;

};

// Helper class used to write plugin info to registry...
class CPluginInfoWriter
{
public:
    CPluginInfoWriter();
    ~CPluginInfoWriter();
    HX_RESULT Init(IHXPreferences* pPrefs, const char* pBaseKeyName, IHXBuffer* pIHXBuffer);
    HX_RESULT Write(IHXBuffer *pBuffer);
    HX_RESULT Write(const char *pInfo);
    HX_RESULT Write(const char *pInfo, UINT32 len);
    HX_RESULT Flush();

private:
    void TerminateBuffer();
    void WriteToRegistry();
    static HXBOOL IsAscii7Compliant (const char*, const UINT32 len);

    IHXBuffer* m_pIHXBuffer; // Contains a pointer to internal buffer
    IHXPreferences* m_pIHXPreferences; // Used to write to registry
    UINT32 m_NumWrites;  // Keeps track of how many times we have written to registry during lifetime of class
    UINT32 m_BufUsed;    // Keeps track of how much we have written to internal buffer between writes to registry
    UINT32 m_BufSize;    // Size of internal buffer; Used to determine when to write info to registry
    UINT32 m_Checksum;	 // Total size of the data written to the registry across the lifetime of the object.

    CHXString m_BaseKeyName; // Combined with m_NumWrite to generate the name of the registry key to which we write
};

#endif /* _PLGNHAND_H_ */
