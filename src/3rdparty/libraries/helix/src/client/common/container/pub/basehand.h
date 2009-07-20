/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: basehand.h,v 1.18 2007/04/17 23:44:43 milko Exp $
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

#include "hxphand.h"
#include "unkimp.h"
#include "hxslist.h"
#include "hxmap.h"

// Plugin Types.
#define	PLUGIN_FILESYSTEM_TYPE	    "PLUGIN_FILE_SYSTEM"
#define	PLUGIN_FILEFORMAT_TYPE	    "PLUGIN_FILE_FORMAT"
#define	PLUGIN_FILEWRITER_TYPE	    "PLUGIN_FILE_WRITER"
#define	PLUGIN_METAFILEFORMAT_TYPE  "PLUGIN_METAFILE_FORMAT"
#define	PLUGIN_RENDERER_TYPE	    "PLUGIN_RENDERER"
#define PLUGIN_DEPACKER_TYPE        "PLUGIN_DEPACKER"
#define PLUGIN_STREAM_DESC_TYPE	    "PLUGIN_STREAM_DESC"
#define PLUGIN_CLASS_FACTORY_TYPE   "PLUGIN_CLASS_FACT"
#define PLUGIN_PAC_TYPE		    "PLUGIN_PAC"

#define	PLUGIN_CLASS		    "PluginType"
#define PLUGIN_FILENAME		    "PluginFilename"
#define PLUGIN_REGKEY_ROOT	    "PluginHandlerData"
#define PLUGIN_PLUGININFO	    "PluginInfo"
#define PLUGIN_GUIDINFO		    "GUIDInfo"
#define PLUGIN_NONHXINFO	    "NonHXDLLs"
#define PLUGIN_DESCRIPTION2	    "Description"
#define PLUGIN_FILE_HASH	    "FileHash"
#define PLUGIN_INDEX		    "IndexNumber"
#define PLUGIN_FILENAMES	    "FileInfo"
#define PLUGIN_COPYRIGHT2	    "Copyright"
#define PLUGIN_LOADMULTIPLE	    "LoadMultiple"
#define PLUGIN_VERSION		    "Version"
#define PLUGIN_FILESYSTEMSHORT	    "FileShort"
#define PLUGIN_FILESYSTEMPROTOCOL   "FileProtocol"
#define PLUGIN_FILEMIMETYPES	    "FileMime"
#define PLUGIN_FILEEXTENSIONS	    "FileExtensions"
#define PLUGIN_FILEOPENNAMES	    "FileOpenNames"
#define PLUGIN_RENDERER_MIME	    "RendererMime"
#define PLUGIN_RENDERER_GRANULARITY "Renderer_Granularity"
#define PLUGIN_DEPACKER_MIME	    "DepackerMime"
#define PLUGIN_STREAMDESCRIPTION    "StreamDescription"

#define PLUGIN_NUM_PLUGINS	    "NumPlugins"
#define PLUGIN_FILE_CHECKSUM	    "DLLCheckSum"
#define PLUGIN_DLL_SIZE		    "DLLSize"
#define PLUGIN_HAS_FACTORY	    "DLLHasFactory"

typedef HX_RESULT (HXEXPORT_PTR FPCREATEINSTANCE) (IUnknown** /*OUT*/ ppIUnknown);
typedef HX_RESULT (HXEXPORT_PTR FPSHUTDOWN) ();
typedef HX_RESULT (HXEXPORT_PTR FPCANUNLOAD2) ();

class DLLAccess;
class CFindFile;

class BaseHandler :	public CUnknownIMP,
			public IHXPluginEnumerator,
			public IHXPlugin2Handler,
		    	public IHXPluginHandler3
{

    class PluginMountPoint;
    class PluginDLL;
    class OtherDLL;
    
public:
    class Plugin;

    friend class CPluginEnumerator;
    friend class PluginDLL;
    friend class Plugin;
    friend class OtherDLL;

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


    DECLARE_UNKNOWN_NOCREATE( BaseHandler )
    DECLARE_COM_CREATE_FUNCS( BaseHandler )

    BaseHandler();
    ~BaseHandler();

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
     *	IHXPlugin2Handler
     */

     /************************************************************************
     *	Method:
     *	    IHXPlugin2Handler::Init
     *
     *	Purpose:
     *	    Specifies the context and sets the BaseHandler in motion.
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
						REF(IUnknown*) pIUnknownInstance,
						IUnknown* pIUnkOuter, IUnknown* pContext );

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


    //------------------------------------ Class Methods

    Errors		    Stat(const char* pszFilename, struct stat* pStatBuffer);
    IHXBuffer*		    ChecksumFile(char* pszFileName, IHXBuffer* pPathBuffer);
    IHXBuffer*		    ConvertToAsciiString(char* pBuffer, UINT32 nBuffLen);

    HX_RESULT               FindGroupOfPluginsUsingBuffers(char* PropName1,
                                                           void* PropVal1,
						           REF(IHXPluginSearchEnumerator*) pIEnumerator);

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

    HX_RESULT ReloadPluginsNoPropagate();
    HX_RESULT ReloadPluginsNoPropagateWithFindFile(
               PluginMountPoint* pMountPoint, CFindFile* pFileFinder,
               IHXBuffer* pPathBuffer, char* pszPluginDir);
    HX_RESULT ReloadPluginsNoPropagate( PluginMountPoint* pMountPoint );

    class Plugin : public IUnknown
    {
	public:
	STDMETHOD(QueryInterface)	(THIS_
					REFIID riid,
					void** ppvObj);

	STDMETHOD_(ULONG32,AddRef)	(THIS);

	STDMETHOD_(ULONG32,Release)	(THIS);

	Plugin(IUnknown* pContext);

	~Plugin();

	// Exposed to the BaseHandler
	void                        SetPluginProperty(const char * pszPluginType);
	HXBOOL			    DoesMatch(IHXValues* pValues);

	Errors			    GetValuesFromDLL(IHXPlugin* pHXPlugin);
	// Creates an instance of the top-level plugin object
	Errors			    GetPlugin( REF(IUnknown*) ppUnknown );
	// Checks to see if this is a component plugin and does the appropriate indirection
	Errors			    GetInstance(REF(IUnknown*) ppUnknown, IUnknown* pIUnkOuter = NULL );
	HX_RESULT		    GetPluginInfo(REF(IHXValues*));
	IHXBuffer*		    GetFileName();
	HXBOOL			    IsLoaded();

	void			    SetInfoNeedsRefresh(HXBOOL bRefresh) { m_bInfoNeedsRefresh = bRefresh;}

	void			    SetDLL(PluginDLL * pPluginDll);
	PluginDLL*		    GetDLL() {return m_pPluginDLL;}
	void			    SetIndex(UINT16 nIndex);
	UINT16			    GetIndex() {return m_nPluginIndex;}

	void			    SetPropertyULONG32(char* , char*);
	void			    SetPropertyCString(char*, char*);
	void			    SetPropertyBuffer(char*, BYTE*, UINT32);

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
	IHXCommonClassFactory*	    m_pClassFactory;


	// Methods to retreive from the DLL
	Errors	    GetBasicValues(IHXPlugin* pHXPlugin);
	Errors	    GetExtendedValues(IHXPlugin* pHXPlugin);

	// Support Functions

	HXBOOL	    AreBufferEqual(IHXBuffer* pBigBuff,
				   IHXBuffer* pSmallBuff);
	Errors	    CreateWatcher(IUnknown* pUnknown);
    };

private:

    void UnloadDeadDLLs(void);

    Errors		    LoadDLL(char* pszDllName, PluginMountPoint* pMountPoint);
    void		    LoadPluginsFromComponentDLL( BaseHandler::PluginDLL* pPluginDll,
							IHXComponentPlugin* pIIterator );

    // Methods to determine out of data DLLs
    HXBOOL		    FindPlugin(const char* pFileName, UINT32 nDLLIndex, REF(UINT32) nIndex);
    HX_RESULT AddToValues(IHXValues*, char* pPropName, void* pPropVal, eValueTypes eValueType);

    HX_RESULT		    RefreshPluginInfo( PluginMountPoint* pMountPoint );

    CHXMapStringToOb	    m_MountPoints;
    CHXSimpleList	    m_PluginDLLList;
    CHXSimpleList	    m_PluginList;
    CHXSimpleList	    m_MiscDLLList;
    CHXMapStringToOb	    m_GUIDtoSupportList;

    IUnknown*		    m_pContext;
    HXBOOL                  m_bLoadStaticLinkedPlugins;

    class PluginMountPoint
    {
    public:
	PluginMountPoint( IUnknown* pContext, BaseHandler* pHandler,
			  const char* pName,
			  UINT32 majorVersion, UINT32 minorVersion,
			  IHXBuffer* pPath );
	~PluginMountPoint();

	STDMETHOD_(ULONG32,AddRef)( THIS );
	STDMETHOD_(ULONG32,Release)( THIS );

#if !defined(_STATICALLY_LINKED) || defined(HELIX_CONFIG_CONSOLIDATED_CORE)
	IHXBuffer* Path();
#endif

    private:
#if !defined(_STATICALLY_LINKED) || defined(HELIX_CONFIG_CONSOLIDATED_CORE)
	IHXBuffer* m_pPath;
#endif
	INT32 m_lRefCount;
    };

    class PluginDLL : public IUnknown
    {
    public:
	PluginDLL(const char* pszFileName, PluginMountPoint* pMountPoint,
		    BaseHandler* pBaseHandler);

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
	BaseHandler*		m_pBaseHandler;
    };

    class OtherDLL
    {
    public:
	    CHXString			m_filename;
	    CHXString			m_fileChecksum;
	    PluginMountPoint*		m_pMountPoint;
    };

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
    void Add(BaseHandler::Plugin* pPlugin);

    // FIX This is for backwards compatibility, and should be removed
    HX_RESULT GetNext(REF(IUnknown*) pRetUnk);

protected:

private:
    CHXSimpleList   m_ListOfPlugins;
    UINT32	    m_nIndex;

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
    void Add(BaseHandler::Plugin* pPlugin);

private:
    CHXSimpleList   m_plugins;
    UINT32	    m_nIndex;

};
#endif /* _BASEHAND_H_ */
