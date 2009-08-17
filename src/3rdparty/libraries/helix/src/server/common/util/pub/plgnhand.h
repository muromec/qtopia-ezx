/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: plgnhand.h,v 1.6 2004/08/23 19:07:04 jgordon Exp $ 
 *   
 * Portions Copyright (c) 1995-2003 RealNetworks, Inc. All Rights Reserved.  
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

#ifndef _DLLHAND_H_
#define _DLLHAND_H_

#include "hxslist.h"
#include "hxplugn.h"
#include "hxstring.h"
#ifdef _WINCE
#include <sys/stat.h>
#endif

struct IUnknown;
struct IHXBuffer;
struct IHXPreferences;
struct IHXPlugin;
struct IHXPluginEnumerator;
struct IHXFileSystemObject;
struct IHXFileFormatObject;
struct IHXRenderer;
struct IHXMetaFileFormatObject;
struct IHXValues;
struct IHXStreamDescription;
struct IHXErrorMessages;
struct IHXScheduler;

class DLLAccess;
class CHXMapStringToOb;
class CHXMountPointMap;
class CHXPluginInfoList;
class MountPointHandler;

typedef HX_RESULT (HXEXPORT_PTR FPCREATEINSTANCE) (IUnknown** /*OUT*/ ppIUnknown);
typedef HX_RESULT (HXEXPORT_PTR FPSHUTDOWN) (void);

#define PLUGINHANDLER_REGISTRY_LOAD_LIST "config.PluginLoadOrder"

#define REGISTRY_KEY_ALLOWANCE_STARTUP_OPTIMIZATION "config.AllowanceOptimizations.EnableURLBasedAllowanceQueries"
#define REGISTRY_KEY_ALLOWANCE_OPTIMIZATION_TRACE "config.AllowanceOptimizations.TraceAllowanceCalls"

#define MISC_PLUGINS			    20000
#define GENERAL_PLUGINS			    20001
#define ALLOWANCE_PLUGINS		    20002

///////////////////////////////////////////////
// File System List		Actual Stored as...
// { fs-name, proto, dll },	fs-name\0proto\0dll\0\0
// { fs-name, proto, dll },	fs-name\0proto\0dll\0\0
// ...				\0
// 
// Render List			Actual Stored as...
// { mime-type, dll },		mime-type\0dll\0\0
// { mime-type, dll },		mime-type\0dll\0\0
// ...				\0
// 
// File Format List		Actual Stored as...
// { ext, mime, open, dll },	ext\0mime\0open\0dll\0\0
// { ext, mime, open, dll },	ext\0mime\0open\0dll\0\0
// ...				\0
//
// Meta File Format List	Actual Stored as...
// { ext, mime, open, dll },	ext\0mime\0open\0dll\0\0
// { ext, mime, open, dll },	ext\0mime\0open\0dll\0\0
// ...				\0
//


class PluginHandler : public IHXPluginEnumerator, 
		      public IHXPluginReloader,
		      public IHXPluginQuery,
                      public IHXPlugin2Handler
{
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
	REQUIRED_PLUGIN_NOT_LOADED
    };


    PluginHandler(const char* pPluginDir = 0);
    ~PluginHandler();

    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)	(THIS);

    STDMETHOD_(ULONG32,Release)	(THIS);

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

    /************************************************************************
     *	Method:
     *	    IHXPluginReloader::LoadNewPlugins
     *
     *	Purpose:    
     *	    Loads only those plugins which are new to the system
     *
     */
    STDMETHOD(LoadNewPlugins)() { return HXR_NOTIMPL; };

    /*
     *	IHXPluginQuery methods
     */

    /************************************************************************
     *	Method:
     *	    IHXPluginQuery::GetNumPluginsGivenGroup
     *
     *	Purpose:    
     *	    Gets the number of plugins associated with a particular class id.
     *
     */
    STDMETHOD(GetNumPluginsGivenGroup)	(THIS_ REFIID riid, 
					REF(UINT32) /*OUT*/ unNumPlugins) ;

    /************************************************************************
     *	Method:
     *	    IHXPluginQuery::GetPluginInfo
     *
     *	Purpose:    
     *	    Gets the info of a particular plugin.
     *
     */
    STDMETHOD(GetPluginInfo)	(THIS_ REFIID riid, 
				UINT32 unIndex, 
				REF(IHXValues*) /*OUT*/ Values) ;

    /*
     *	IHXPlugin2Handler Methods
     */

     /************************************************************************
     *	Method:
     *	    IHXPlugin2Handler::Init
     *
     *	Purpose:    
     *	    Specifies the context and sets the pluginhandler in motion.
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
				REF(IHXValues*) /*OUT*/ Values);

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
     *	    the size to 0.If the cache is disabled a DLL will be 
     *	    unloaded whenever it's refcount becomes zero. Which MAY
     *	    cause performance problems.
     */

    STDMETHOD(SetCacheSize)	(THIS_ ULONG32 nSizeKB);

    /************************************************************************
     *	Method:
     *	    IHXPlugin2Handler::GetInstance
     *
     *	Purpose:    
     *	    
     *	    This function will return a plugin instance given a plugin index.
     *		
     */

    STDMETHOD(GetInstance) (THIS_ UINT32 index, REF(IUnknown*) pUnknown); 

    /************************************************************************
     *	Method:
     *	    IHXPlugin2Handler::FindIndexUsingValues
     *
     *	Purpose:    
     *	    Finds a plugin  which matches the set of values given. An index
     *	    is returned which can be used to either get the values (using 
     *	    GetPluginInfo) or an instance can be created using GetPluing(). 	
     * 
     */

    STDMETHOD(FindIndexUsingValues)	    (THIS_ IHXValues*, 
						    REF(UINT32) unIndex);

    /************************************************************************
     *	Method:
     *	    IHXPlugin2Handler::FindPluginUsingValues
     *
     *	Purpose:    
     *	    Finds a plugin  which matches the set of values given. A Plugin
     *	    instance is returned. 
     *	    
     */

    STDMETHOD(FindPluginUsingValues)	    (THIS_ IHXValues*, 
						    REF(IUnknown*) pUnk);
    
    /************************************************************************
     *	Method:
     *	    IHXPlugin2Handler::FindIndexUsingStrings
     *
     *	Purpose:    
     *	    Finds a plugin  which matches the set of values given. An index
     *	    is returned which can be used to either get the values (using 
     *	    GetPluginInfo) or an instance can be created using GetPluing(). 	
     *	    NOTE: that a max of two values may be given.
     */

    STDMETHOD(FindIndexUsingStrings)	    (THIS_ char* PropName1, 
						    char* PropVal1, 
						    char* PropName2, 
						    char* PropVal2, 
						    char* PropName3, 
						    char* PropVal3, 
						    REF(UINT32) unIndex);

    /************************************************************************
     *	Method:
     *	    IHXPlugin2Handler::FindPluginUsingStrings
     *
     *	Purpose:    
     *	    Finds a plugin  which matches the set of values given. A Plugin
     *	    instance is returned. 
     *	    NOTE: that a max of two values may be given.
     */

    STDMETHOD(FindPluginUsingStrings)	    (THIS_ char* PropName1, 
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

    STDMETHOD(FindImplementationFromClassID)(THIS_ REFGUID GUIDClassID, 
	                                     REF(IUnknown*) pIUnknownInstance);

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


    BOOL			PluginIsInRegistery(char* pszDllName);

    Errors			Refresh(BOOL bShowOutput = TRUE);
    void			Clear();
    void			ClearRequiredPluginsList();
    Errors			StoreToRegistry();
    Errors			ReadFromRegistry();
    static void			ParseInfoArray(CHXSimpleList* pszStringList, const char** ppszInfoArray);
    static void			ClearStringList(CHXSimpleList* pszStringList);
    void                        FreeAllLibraries(void);

    class PluginDLL  
    {
    public:

    PluginDLL(const char* pszFileName, IHXErrorMessages* pEM = 0);
    ~PluginDLL();
    Errors		    Load();
    void		    Unload();
    ULONG32		    AddRef();
    ULONG32		    Release();

    char*		    m_pszFileName;
    LONG32		    m_lRefCount;
    UINT16		    m_NumOfPlugins;
    IHXPluginFactory*	    m_pPluginFactory;
    DLLAccess*		    m_pDllAccess;
    FPCREATEINSTANCE	    m_fpCreateInstance;
    FPSHUTDOWN		    m_fpShutdown;
    BOOL		    m_has_factory;
    IHXErrorMessages*	    m_pErrorMessages;  
    BOOL		    m_bLoaded;

    };

    
    class Plugin
    {
    public:

    Plugin(const char* pszDllName, BOOL do_unload,IHXErrorMessages* pEM = 0, PluginDLL* 
	pPluginDLL=0, UINT16 index=0);

    ~Plugin();

    ULONG32			    AddRef();
    ULONG32			    Release();

    Errors			    Init(IUnknown* pContext);
    Errors			    Load();
    void			    ReleaseInstance(); // Does nothing, left it in so as to keep references to it.
    Errors			    GetInstance(IUnknown** ppUnknown);
    void			    GetPluginInfo(char** ppszDllName,
						  char** ppszDescription,
						  char** ppszCopyright,
						  char** ppszMoreInfo,
						  BOOL*  pbMultiple);
	
    BOOL			    m_load_multiple;
    BOOL			    m_generic;
    BOOL			    m_single_instance;
    char*			    m_pszDescription;
    char*			    m_pszCopyright;
    char* 			    m_pszMoreInfoUrl;
    char*			    m_pszDllName;
    PluginDLL*			    m_pPluginDLL;		
    UINT16			    m_nPluginIndex;		
	
    // void* because the client doesn't have these
    void*               m_process;  

    private:    
    LONG32			    m_lRefCount;
    BOOL			    m_do_unload;
    IHXErrorMessages*		    m_pErrorMessages;
#ifdef _WIN32
    CRITICAL_SECTION		    m_critSec;
#endif
    };
    
    class DataConvert
    {
    public:
	DataConvert(PluginHandler* pparent);
	~DataConvert();
	
	class PluginInfo
	{
	    friend class		DataConvert;
	public:
	    Plugin*			m_pPlugin;
	    UINT32			m_ulID;
	    IHXValues*			m_options;
	private:
	    static UINT32		m_cNextID;
	    PluginInfo();
	    ~PluginInfo();
	    
	    Errors			Init(Plugin* pPlugin);
	    
	    IUnknown*			m_pInstance;
	    CHXString			m_mount_point;
	    CHXString			m_szShortName;
	};

	Errors		StoreToRegistry(IHXPreferences** ppRegistry);
	Errors		ReadFromRegistry(IHXPreferences* pRegistry);

	Errors		Add(Plugin* pPlugin, char* pszShortName = 0);
	void		Clear();
	Errors		AddMountPoint(const char* pszShortName,
					const char* pszMountPoint,
					IHXValues* pszOptions);
	Errors		Find(const char* pszFilePath, PluginInfo*& plugin);


    private:
	CHXMapStringToOb*	    m_pShortNameMap;
	CHXSimpleList*		    m_pPlugins;
	PluginHandler*		    m_pparent;
	MountPointHandler*	    m_pMountPointHandler;
    };

    class FileSystem
    {
    public:
	FileSystem(PluginHandler* pparent);
	~FileSystem();
	class PluginInfo
	{
	    friend class		FileSystem;
	public:
	    Plugin*		  	m_pPlugin;
	    UINT32			m_ulID;
	private:
	    static UINT32		m_cNextID;
	    PluginInfo();
	    ~PluginInfo();

	    Errors		    	Init(Plugin* pPlugin);

	    IUnknown*			m_pInstance;
	    CHXString			m_mount_point;
	    CHXString			m_szProtocol;
	    CHXString			m_szShortName;
	    IHXValues*                 m_options;
	};


	Errors			    StoreToRegistry(IHXPreferences** ppRegistry);
	Errors			    ReadFromRegistry(IHXPreferences* pRegistry);

	Errors			    Find(const char*	pszFilePath,
					 const char*	pszProtocol,
					 UINT32&	mount_point_len,
					 PluginInfo*&	plugin,
					 IHXValues*&   options);

	Errors			    FindAfter(const char*   pszFilePath,
					      UINT32&	    mount_point_len,
					      PluginInfo*&	    plugin,
					      IHXValues*&  options);

	Errors			    FindShort(const char*   pszShortName,
					      Plugin*&	    plugin);

	Errors			    Add(Plugin* pPlugin, char* pszShortName = 0, char* pszProtocol = 0);

	Errors			    AddMountPoint(const char* pszShortName,
						  const char* pszMountPoint,
						  IHXValues* pszOptions);

	void			    Clear();
	
	UINT32			    GetNumOfPlugins();
	void			    GetPluginInfo(UINT32 unIndex, 
						  char** ppszDllPath,
						  char** ppszDescription,
						  char** ppszCopyright,
						  char** ppszMoreInfo,
						  BOOL*	 pbMultiple,
						  char** ppszProtocol,
						  char** ppszShortName);

	BOOL XPlatformPathCmp(const char* pCharMountPoint, INT32 ulMountPointLength, const char* pCharFilePath);
	const char* GetNextPathElement(const char*& pCharPath);

    private:
	MountPointHandler*	    m_pMountPointHandler;
	CHXMapStringToOb*	    m_pProtocolMap;
	CHXMapStringToOb*	    m_pShortNameMap;
	CHXSimpleList*		    m_pPlugins;
	PluginHandler*		    m_pparent;
    };

    class Renderer
    {
    public:
	Renderer(PluginHandler* pparent);
	~Renderer();

	Errors			    StoreToRegistry(IHXPreferences** ppRegistry);
	Errors			    ReadFromRegistry(IHXPreferences* pRegistry);

	Errors			    Find(const char*	pszMimeType,
					 Plugin*&	plugin);
	Errors			    Add(Plugin* pPlugin, CHXSimpleList* pMimeTypesList=0);
	
	void			    Clear();
	
	UINT32			    GetNumOfPlugins();
	void			    GetPluginInfo(UINT32	    unIndex, 
						  char**	    ppszDllPath,
						  char**    	    ppszDescription,
						  char**	    ppszCopyright,
						  char**	    ppszMoreInfo,
						  BOOL*		    pbMultiple,
						  CHXSimpleList**   ppszMimeTypes);
	
	class PluginInfo
	{
	public:
	    Plugin*		    m_pPlugin;
	private:
	    friend class	    Renderer;
	    PluginInfo();
	    ~PluginInfo();

	    Errors		    Init(Plugin* pPlugin);

	    CHXSimpleList	    m_mimeTypes;
	};

	CHXSimpleList*		    m_pPlugins;
    private:
	PluginHandler*		    m_pparent;
	PluginInfo*		    FindPluginInfo(const char* pszMimeType);

	CHXMapStringToOb*	    m_pMimeMap;
    };

    class FileFormat
    {
    public:
	FileFormat(ULONG32 ulType, PluginHandler* pparent);
	~FileFormat();

	Errors			    StoreToRegistry(IHXPreferences** ppRegistry);
	Errors			    ReadFromRegistry(IHXPreferences* pRegistry);

	UINT32			    GetNumPlugins();
	Errors			    Find(const char*	pszMimeType, 
					 const char*	pszExtension, 
					 Plugin*&	plugin);
	
	Errors			    Add(Plugin* pPlugin, CHXSimpleList* pMimeTypesList=0, 
				    CHXSimpleList* pExtensionList=0, CHXSimpleList* pOpenNames=0);
	
	void			    Clear();
	
	UINT32			    GetNumOfPlugins();
	void			    GetPluginInfo(UINT32	    unIndex, 
						  char**	    ppszDllPath,
						  char**	    ppszDescription,
						  char**	    ppszCopyright,
						  char**	    ppszMoreInfo,
						  BOOL*		    pbMultiple,
						  CHXSimpleList**   ppszMimeTypes,
						  CHXSimpleList**   ppszExtensions, 
						  CHXSimpleList**    ppszOpenNames);

        Errors                      FindUsingValues(IHXValues* pValues, 
                                                    Plugin*& pPlugin);

	class PluginInfo
	{
	    friend class	    FileFormat;
	public:
	    PluginInfo();
	    ~PluginInfo();

	    Errors		    Init(Plugin* pPlugin);

	    CHXSimpleList	    m_mimeTypes;
	    CHXSimpleList	    m_extensions;
	    CHXSimpleList	    m_OpenNames;
	    Plugin*		    m_pPlugin;
	public:
	    PluginInfo*		    m_pNextPlugin;
	};

	Errors			    MapFromExtToMime(const char* pszExtension,
						     const char*& mime_type);
    public:
	ULONG32			    m_ulType;

	PluginInfo*		    FindPluginInfo(const char* pszMimeType, 
						   const char* pszExtension);
	UINT32			    GetPriority(PluginInfo* pPlugin);

	CHXMapStringToOb*	    m_pMimeMap;
	CHXMapStringToOb*	    m_pExtensionMap;
	CHXSimpleList*		    m_pPlugins;
	PluginHandler*		    m_pparent;
    };

    class BroadcastFormat
    {
    public:
	BroadcastFormat(PluginHandler* pparent);
	~BroadcastFormat();

	Errors			    StoreToRegistry(IHXPreferences** ppRegistry);
	Errors			    ReadFromRegistry(IHXPreferences* pRegistry);

	Errors			    Find(const char*	pszBroadcastType,
					 Plugin*&	plugin);
	Errors			    Add(Plugin* pPlugin, char* pszType=0);
	
	void			    Clear();
	
	UINT32			    GetNumOfPlugins();
	void			    GetPluginInfo(UINT32 unIndex, 
						  char** ppszDllPath,
						  char** ppszDescription,
						  char** ppszCopyright,
						  char** ppszMoreInfo,
						  BOOL*	 pbMultiple,
						  char** ppszType);						

	class PluginInfo
	{
	    friend class	    BroadcastFormat;
	public:
	    PluginInfo();
	    ~PluginInfo();

	    Errors		    Init(Plugin* pPlugin);

	    char*		    m_pszType;
	    Plugin*		    m_pPlugin;
	};

	CHXSimpleList*		    m_pPlugins;
    private:
	PluginHandler*		    m_pparent;
	PluginInfo*		    FindPluginInfo(const char* pszMimeType);
	CHXMapStringToOb*           m_pBroadcastMap;
    };

    class StreamDescription
    {
    public:
	StreamDescription(PluginHandler* pparent);
	~StreamDescription();

	Errors			    StoreToRegistry(IHXPreferences** ppRegistry);
	Errors			    ReadFromRegistry(IHXPreferences* pRegistry);

	Errors			    Find(const char*	pszMimeType,
					 Plugin*&	plugin);
	Errors			    Add(Plugin* pPlugin, char* pszMimeType = 0);
	
	void			    Clear();
	
	UINT32			    GetNumOfPlugins();
	void			    GetPluginInfo(UINT32	    unIndex, 
						  char**	    ppszDllPath,
						  char**    	    ppszDescription,
						  char**	    ppszCopyright,
						  char**	    ppszMoreInfo,
						  BOOL*		    pbMultiple,
						  char**  	    ppszMimeType);
	
        Errors                      FindUsingValues(IHXValues* pValues, 
                                                    Plugin*& pPlugin);
	class PluginInfo
	{
	public:
	    Plugin*		    m_pPlugin;
	private:
	    friend class	    StreamDescription;
	    PluginInfo();
	    ~PluginInfo();

	    Errors		    Init(Plugin* pPlugin);

	    char*		    m_pszMimeType;
	};

	CHXSimpleList*		    m_pPlugins;
    private:
	PluginHandler*		    m_pparent;
	PluginInfo*		    FindPluginInfo(const char* pszMimeType);
	CHXMapStringToOb*	    m_pMimeMap;
    };

    class Basic
    {
    public:
	Basic(ULONG32 ulType, PluginHandler* pparent);
	~Basic();

	Errors			    StoreToRegistry(IHXPreferences** ppRegistry);
	Errors			    ReadFromRegistry(IHXPreferences* pRegistry);
	Errors			    Add(Plugin* pPlugin);

	void			    Clear();
	
	UINT32			    GetNumOfPlugins();
	void			    GetPluginInfo(UINT32 unIndex, 
						  char** ppszDllPath,
						  char** ppszDescription,
						  char** ppszCopyright,
						  char** ppszMoreInfo,
						  BOOL*  pbMultiple);

	CHXSimpleList*		    m_pPlugins;

    private:
	PluginHandler*		    m_pparent;
	ULONG32			    m_ulType;
    };

    class AllowancePlugins
    {
    public:
        class PluginInfo
        {
        public:
            PluginInfo() : m_pPlugin(NULL)
                        , m_bMultiLoad(FALSE)
            {}
            ~PluginInfo();

            PluginHandler::Errors Init(Plugin* pPlugin);

            PluginHandler::Plugin*      m_pPlugin;
            BOOL                        m_bMultiLoad;
            CHXString                   m_szFileSysShortName;
            CHXString                   m_szFileSysProtocol;
            CHXString                   m_szFileSysMountPoint;

        friend class PluginHandler::AllowancePlugins;
        };

        AllowancePlugins(PluginHandler* pparent);
        ~AllowancePlugins();

        void Init(IHXPreferences* pPreferences);

        Errors StoreToRegistry(IHXPreferences** ppRegistry);
        Errors ReadFromRegistry(IHXPreferences* pRegistry);
        Errors Add(Plugin* pPlugin);

        void   Clear();
	
        UINT32 GetNumOfPlugins();
        void   GetPluginInfo(UINT32 unIndex, 
                        char** ppszDllPath,
                        char** ppszDescription,
                        char** ppszCopyright,
                        char** ppszMoreInfo,
                        BOOL*  pbMultiple);

        Errors AddMountPoint(IHXBuffer* pShortNameBuff,
                        const char* pszMountPoint,
                        IHXValues* pszOptions);

        Errors FindPluginFromMountPoint(const char* pszFilePath,
                        PluginInfo*& pPluginInfo);

        void   PrintDebugInfo();
        CHXSimpleList*          m_pPluginInfoList;
        MountPointHandler*      m_pNonMultiPluginsWithMountPoint;
	BOOL                    m_bClientStartupOptimization;
	BOOL                    m_bDebug;
    private:
	PluginHandler*  m_pparent;
    };

    class Factory
    {
    public:
	Factory(PluginHandler* pparent);
	~Factory();

	Errors		    StoreToRegistry(IHXPreferences** ppRegistry);
	Errors		    ReadFromRegistry(IHXPreferences* pRegistry);
	Errors		    Add(Plugin* pPlugin);

	void		    Clear();
	
	UINT32		    GetNumOfPlugins();
	void		    GetPluginInfo(UINT32 unIndex, 
					  char** ppszDllPath,
					  char** ppszDescription,
					  char** ppszCopyright,
					  char** ppszMoreInfo,
					  BOOL*  pbMultiple);

	CHXSimpleList*		    m_pPlugins;
	PluginHandler*		    m_pparent;
    };

    class PluginFactory
    {
    public:
	PluginFactory();
	~PluginFactory();

	Errors			    StoreToRegistry(IHXPreferences** ppRegistry);
	Errors			    ReadFromRegistry(IHXPreferences* pRegistry);
	Errors			    Add(PluginDLL* pPluginDll);
	void			    Clear();
	UINT32			    GetNumOfDlls();
	PluginDLL*			FindDLLFromName(char* pFilename);

	
	CHXSimpleList*		    m_pPluginDlls;
    };

    DataConvert*		m_data_convert_handler;
    FileSystem*			m_file_sys_handler;
    FileFormat*			m_file_format_handler;
    Renderer*			m_renderer_handler;
    FileFormat*			m_meta_format_handler;
    BroadcastFormat*		m_broadcast_handler;
    StreamDescription*		m_stream_description_handler;
    AllowancePlugins*		m_allowance_handler;
    Basic*			m_misc_handler;
    Basic*			m_general_handler;
    Factory*			m_factory_handler;
    PluginFactory*		m_PluginFactory;
    IUnknown*	   	 	m_pContext;

private:

    LONG32			m_lRefCount;

    void                        EchoToStartupLog(const char* path, const char* prefix, const char* string);
    void			InitMountPoints();
    char*			GetPluginDir();
    const char*			GetDefaultPluginDir();
    Errors			Stat(const char* pszFilename, struct stat* pStatBuffer);
    Errors			ChkSumDirectory(char * pszPluginDir, char* MD5Result);
    Errors			IsDirValid(IHXPreferences* pRegistry);
    Errors			SaveDirChkSum(IHXPreferences** pRegistry);

    BOOL			IsPluginRequired(const char* pszDescription, 
						 ULONG32& ulIndex);
    BOOL			ValidateRequiredPlugin(IHXPlugin* pHXPlugin, 
						       const char* pszDescription);
    void			ReportRequiredPluginError(UINT32 ulIndex);
    Errors                      RefreshOrderedPluginNames();
    void                        ClearOrderedPluginNames();
    HX_RESULT		        AddToValues(IHXValues*, char* pPropName, 
                                            char* pPropVal);

    static inline HX_RESULT     ConvertError(Errors);

    IHXPreferences*   		m_pPreferences;
    IHXErrorMessages*  	m_pErrorMessages;
    IHXScheduler*		m_pScheduler;
    char*			m_pszDefaultPluginDir;
    char*			m_pszPluginDir;
    CHXSimpleList*		m_pRequiredPlugins;
    char**                      m_pPluginName;
    char**                      m_pPluginPath;
    int                         m_nNumPluginNames;
};

inline HX_RESULT
PluginHandler::ConvertError(Errors err)
{
    /* convert error code to appropriate HX_RESULT */
    switch(err)
    {
        case NO_ERRORS:
            return HXR_OK;

        case PLUGIN_NOT_FOUND:
            return HXR_MISSING_COMPONENTS;
        case MEMORY_ERROR:
            return HXR_OUTOFMEMORY;
        case CANT_OPEN_DLL:
            return HXR_FILE_NOT_FOUND;
        case BAD_DLL:
        case BAD_PLUGIN:
            return HXR_INVALID_FILE;
        case CREATE_INSTANCHXR_FAILURE:
            return HXR_NOINTERFACE;
        case BAD_REGISTRY_HANDLE:
            return HXR_INVALID_PARAMETER;

        default:
            return HXR_FAIL;
    }
}

#endif /* _DLLHAND_H_ */
