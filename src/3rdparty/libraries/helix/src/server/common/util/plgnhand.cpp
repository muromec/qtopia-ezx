/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: plgnhand.cpp,v 1.15 2007/07/04 14:15:22 imakandar Exp $ 
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
/*
 *
 *  Plugin information are stored into the registry in the following format:
 *
 *	File Format Plugins: {dllpath;description;copyright;moreinfo;loadmultiple;mimetype1|mimetype2;extension1|extension2}{ ... }
 *			     ---------------------------------------------------------------------------------------------------
 *								One Plugin
 *
 *	File System Plugins: {dllpath;description;copyright;moreinfo;loadmultiple;protocol;shortname}{ ... }
 *			     ----------------------------------------------------------------------------
 *								One Plugin
 *
 *	Renderer Plugins:    {dllpath;description;copyright;moreinfo;loadmultiple;mimetype1|mimetype2}{ ... }
 *			     -----------------------------------------------------------------------------
 *								One Plugin
 *
 *	Broadcast Plugins:   {dllpath;description;copyright;moreinfo;loadmultiple;type}{ ... }
 *			     --------------------------------------------------------------
 *								One Plugin
 *
 *	Stream Description Plugins: {dllpath;description;copyright;moreinfo;loadmultiple;mimetype}{ ... }
 *			     ----------------------------------------------------------------------------
 *								One Plugin
 *	
 *	Allowance Plugins:   {dllpath;description;copyright;moreinfo;loadmultiple}{ ... }
 *                           ---------------------------------------------------------
 *								One Plugin
 *
 *	Misc. Plugins:	     {dllpath;description;copyright;moreinfo;loadmultiple}{ ... }
 *                           ---------------------------------------------------------
 *								One Plugin
 *
 *	Plugins:	     {dllpath;description;copyright;moreinfo;loadmultiple}{ ... }
 *                           ---------------------------------------------------------
 *								One Plugin
 *
 */

#include "hxtypes.h"

#ifdef _WINDOWS
#ifdef _WIN16
#include <stdlib.h>
#endif
#include "hlxclib/windows.h"
#include <ctype.h>
#endif
#ifdef _MACINTOSH
#include <ctype.h>
#endif

#if defined (__MWERKS__)
#include <stdlib.h>
#include "fullpathname.h"
#include "chxdataf.h"
#include <stat.h>
#include <fcntl.h>
#endif

#include <stdio.h>
#include <string.h>

#include "hxresult.h"
#include "hxassert.h"
#include "hxcom.h"
#include "ihxpckts.h"
#include "hxfiles.h"
#include "hxdtcvt.h"
#include "hxformt.h"
#include "hxrendr.h"
#include "hxprefs.h"
#include "hxplugn.h"
#include "hxmeta.h"
#include "hxsdesc.h"
#include "hxauth.h"
#include "hxallow.h"
#include "hxerror.h"
#include "hxcomm.h"
#include "hxengin.h"
#include "hxshtdn.h"
#include "chxpckts.h"
#include "hxstring.h"
#include "hxslist.h"
#include "hxmap.h"
#include "hxstrutl.h"
#include "findfile.h"
#include "dbcs.h"
#include "plgnhand.h"
#include "hxmon.h"
#include "md5.h"
#include "mphandle.h"
#include "defslice.h"
#ifdef _STATICALLY_LINKED
#include "staticff.h"
#endif


#include "dllacces.h"
#include "dllpath.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static char HX_THIS_FILE[] = __FILE__;
#endif

#ifdef _WINCE
#include <wincestr.h>
#endif

#include "activopt.h"

#if defined (_WINDOWS ) || defined (WIN32)
#define OS_SEPARATOR_CHAR	'\\'
#define OS_SEPARATOR_STRING	"\\"
#elif defined (_UNIX)
#define OS_SEPARATOR_CHAR	'/'
#define OS_SEPARATOR_STRING	"/"
#elif defined (__MWERKS__)
#define OS_SEPARATOR_CHAR	':'
#define OS_SEPARATOR_STRING	":"
#endif // defined (_WINDOWS ) || defined (WIN32)

#define PLUGIN_ENTRY_SEPARATOR		    ';'
#define PLUGIN_VALUE_SEPARATOR		    '|'

#define META_FILE_FORMAT		    10000
#define OTHER_FILE_FORMAT		    10001

#define REGKEY_FILEFORMAT_PLUGIN	    "FileFormatPluginInfo"
#define REGKEY_METAFORMAT_PLUGIN	    "MetaFormatPluginInfo"
#define REGKEY_FILESYSTEM_PLUGIN	    "FileSystemPluginInfo"
#define REGKEY_BROADCAST_PLUGIN		    "BroadcastPluginInfo"
#define REGKEY_RENDERER_PLUGIN		    "RendererPluginInfo"
#define REGKEY_STREAM_DESCRIPTION_PLUGIN    "StreamDescriptionPluginInfo"
#define REGKEY_MISC_PLUGIN		    "MiscPluginInfo"
#define REGKEY_GENERAL_PLUGIN		    "GeneralPluginInfo"
#define REGKEY_ALLOWANCE_PLUGIN		    "AllowancePluginInfo"
#define REGKEY_FACTORY_PLUGIN		    "FactoryPluginInfo"
#define REGKEY_DLLLIST			    "DLLListInfo"
#define REGKEY_PLUGINHASH		    "PluginHash"

//Special case: RPK Data converstion plugin
#define RPK_DATA_CONVERT_PLUGIN_NAME         "rpkconv"

#define FILEFORMAT_PLUGIN_RECORD_ENTRIES    9
#define FILESYSTEM_PLUGIN_RECORD_ENTRIES    8
#define BROADCAST_PLUGIN_RECORD_ENTRIES	    7
#define RENDERER_PLUGIN_RECORD_ENTRIES	    7
#define STREAM_DESCRIPTION_PLUGIN_RECORD_ENTRIES	    7
#define MISC_PLUGIN_RECORD_ENTRIES	    6
#define GENERAL_PLUGIN_RECORD_ENTRIES	    5
#define ALLOWANCE_PLUGIN_RECORD_ENTRIES	    5

// help functions
void AppendListToRecord(CHXSimpleList* /*IN*/ pList, char** /*IN OUT*/ ppszRecord)
{
    CHXSimpleList::Iterator i;
    char*   pszInfo = NULL;
    char*   pszData = NULL;
    char*   pszTemp = NULL;
    
    for (i = pList->Begin(); i != pList->End(); ++i)
    {
	pszData = (char*) (*i);

	// xxx
	if (pList->Begin() == i)
	{
	    pszTemp = new char[strlen(pszData)+1];
	    sprintf(pszTemp, "%s", pszData);
	}
	// xxx|xxx
	else
	{
	    pszTemp = new char[strlen(pszInfo)+strlen(pszData)+2];
	    sprintf(pszTemp, "%s%c%s", pszInfo, PLUGIN_VALUE_SEPARATOR, pszData);
	}

	if (pszInfo) delete[] pszInfo;
	pszInfo = pszTemp;
    }

    if (pszInfo)
    {
	pszTemp = new char[strlen(*ppszRecord)+strlen(pszInfo)+1];
	sprintf(pszTemp, "%s%s", *ppszRecord, pszInfo);

	delete *ppszRecord;
	*ppszRecord = pszTemp;
    }

    if (pszInfo)
    {
	delete[] pszInfo;
	pszInfo = NULL;
    }

    return;
}

void AppendStringToRecord(char* /*IN*/ pszString, char** /*IN OUT*/ ppszRecord)
{
    char*   pszTemp = NULL;
    
    if (pszString)
    {
	pszTemp = new char[strlen(*ppszRecord)+strlen(pszString)+1];
	sprintf(pszTemp, "%s%s", *ppszRecord, pszString);
	
	delete *ppszRecord;
	*ppszRecord = pszTemp;
    }

    return;
}

void RetrieveListFromRecord(char** /*IN OUT*/ ppszRecord, CHXSimpleList* /*OUT*/ pList)
{
    char*   pszData = NULL;
    char*   pszCursor = NULL;
    int	    nLength = 0;

    while (TRUE)
    {
	pszCursor = *ppszRecord;
	while (**ppszRecord != PLUGIN_VALUE_SEPARATOR	&&
	       **ppszRecord != PLUGIN_ENTRY_SEPARATOR	&&
	       **ppszRecord != '}')
	{
	    (*ppszRecord)++;
	    nLength++;
	}

	pszData = new char[nLength+1];

	strncpy(pszData, pszCursor, nLength);
	pszData[nLength] = 0;
	// add to the list
	pList->AddTail((void*)pszData);

//	delete pszData;  WTF?
	pszData = NULL;

	nLength = 0;

	if (**ppszRecord == PLUGIN_VALUE_SEPARATOR)
	{	
	    (*ppszRecord)++;
	}
	else
	{
	    break;
	}
    }
}

void RetrieveStringFromRecord(char** /*IN OUT*/ ppszRecord, char** /*OUT*/ ppszString)
{
    char*   pszCursor = NULL;
    int	    nLength = 0;
   
    pszCursor = *ppszRecord;
    while (**ppszRecord != PLUGIN_ENTRY_SEPARATOR &&
           **ppszRecord != '}')
    {
        (*ppszRecord)++;
        nLength++;
    }

    *ppszString = new char[nLength+1];
    memset(*ppszString, 0, nLength+1);

    strncpy(*ppszString, pszCursor, nLength);
    
    return;
}

void* GetDataFromList(CHXSimpleList* /* IN */ pList, UINT32 /* IN */ unPos)
{
    UINT32  unCount = 0;
    CHXSimpleList::Iterator i;

    for (unCount = 0, i = pList->Begin(); i != pList->End(); ++i, ++unCount)
    {
	if (unCount == unPos)
	{
	    return *i;
	}	
    }

    return NULL;
}
    
// The lifetime of the PluginInfo objects is managed 
// elsewhere, so we just store the reference.
class CHXInfoWrapper
{
public:
    PluginHandler::FileSystem::PluginInfo* pInfo;
};

#define LIST_NAME CHXPluginInfoList
#define VALUE_TYPE CHXInfoWrapper
#define DEFINE_LIST
#include "hxlist.h"

#define LIST_NAME CHXPluginInfoList
#define DEFINE_LIST_GLOBALS
#include "hxlist.h"

#define LIST_NAME CHXPluginInfoList
#define IMPLEMENT_LIST
#include "hxlist.h"

///////////////////////////////////////////////////////////////////////////////
//
// PluginHandler
//
///////////////////////////////////////////////////////////////////////////////
PluginHandler::PluginHandler(const char* pPluginDir)
    : m_pPluginName(NULL)
    , m_pPluginPath(NULL)
    , m_nNumPluginNames(0)
{
    m_pContext			= NULL;
    m_pScheduler		= NULL;
    m_pPreferences		= NULL;
    m_data_convert_handler	= new DataConvert(this);
    m_file_sys_handler		= new FileSystem(this);
    m_file_format_handler	= new FileFormat(OTHER_FILE_FORMAT,this);
    m_meta_format_handler	= new FileFormat(META_FILE_FORMAT,this);
    m_renderer_handler		= new Renderer(this);
    m_broadcast_handler		= new BroadcastFormat(this);
    m_stream_description_handler= new StreamDescription(this);
    m_allowance_handler         = new AllowancePlugins(this);
    m_misc_handler              = new Basic(MISC_PLUGINS,this);
    m_general_handler		= new Basic(GENERAL_PLUGINS,this);
    m_factory_handler		= new Factory(this);
    m_PluginFactory		= new PluginFactory();
    m_pErrorMessages		= NULL;
    m_pszDefaultPluginDir	= NULL;
    m_pRequiredPlugins		= NULL;
    m_lRefCount			= 0;
    m_pszPluginDir		= (pPluginDir) ?
				  new_string(pPluginDir) : 0;
}

PluginHandler::~PluginHandler()
{
    ClearOrderedPluginNames();
    Close();
}

STDMETHODIMP PluginHandler::Close()
{
    Clear();

    if (m_pPreferences)
    {
	HX_ASSERT(0xdddddddd != (UINT32)(PTR_INT)m_pPreferences);
	m_pPreferences->Release();
	m_pPreferences = NULL;
    }

    if (m_pErrorMessages)
    {
	m_pErrorMessages->Release();
	m_pErrorMessages = NULL;
    }

    if (m_pContext)
    {
	m_pContext->Release();
	m_pContext = NULL;
    }

    if (m_pScheduler)
    {
	m_pScheduler->Release();
	m_pScheduler = NULL;
    }

    if (m_pszDefaultPluginDir)
    {
	delete[] m_pszDefaultPluginDir;
	m_pszDefaultPluginDir = NULL;
    }

    if (m_pRequiredPlugins)
    {
	ClearRequiredPluginsList();
	delete m_pRequiredPlugins;
	m_pRequiredPlugins = NULL;
    }

    if (m_data_convert_handler)
    {
	delete m_data_convert_handler;
	m_data_convert_handler = NULL;
    }

    if (m_file_sys_handler)
    {
	delete m_file_sys_handler;
	m_file_sys_handler = NULL;
    }

    if (m_file_format_handler)
    {
	delete m_file_format_handler;
	m_file_format_handler = NULL;
    }

    if (m_meta_format_handler)
    {
	delete m_meta_format_handler;
	m_meta_format_handler = NULL;
    }

    if (m_renderer_handler)
    {
	delete m_renderer_handler;
	m_renderer_handler = NULL;
    }

    if (m_allowance_handler)
    {
	delete m_allowance_handler;
	m_allowance_handler = NULL;
    }

    if (m_misc_handler)
    {
	delete m_misc_handler;
	m_misc_handler = NULL;
    }

    if (m_general_handler)
    {
	delete m_general_handler;
	m_general_handler = NULL;
    }

    if (m_broadcast_handler)
    {
	delete m_broadcast_handler;
	m_broadcast_handler = NULL;
    }

    if (m_stream_description_handler)
    {
	delete m_stream_description_handler;
	m_stream_description_handler = NULL;
    }

    HX_DELETE(m_factory_handler);
    HX_DELETE(m_PluginFactory);
    HX_DELETE(m_pszPluginDir);

    return HXR_OK;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//      PluginHandler::QueryInterface
//  Purpose:
//      Implement this to export the interfaces supported by your
//      object.
//
STDMETHODIMP
PluginHandler::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IHXPluginEnumerator))
    {
        AddRef();
        *ppvObj = (IHXPluginEnumerator*)this;
        return HXR_OK;
    }
    else
    if (IsEqualIID(riid, IID_IHXPluginReloader))
    {
        AddRef();
        *ppvObj = (IHXPluginReloader*)this;
        return HXR_OK;
    }
    if (IsEqualIID(riid, IID_IHXPluginQuery))
    {
        AddRef();
        *ppvObj = (IHXPluginQuery*)this;
        return HXR_OK;
    }
    if (IsEqualIID(riid, IID_IHXPlugin2Handler))
    {
        AddRef();
        *ppvObj = (IHXPlugin2Handler*)this;
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}   

/////////////////////////////////////////////////////////////////////////
//  Method:
//      PluginHandler::AddRef
//  Purpose:
//      Everyone usually implements this the same... feel free to use
//      this implementation.
//
STDMETHODIMP_(ULONG32)
PluginHandler::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}   

/////////////////////////////////////////////////////////////////////////
//  Method:
//      PluginHandler::Release
//  Purpose:
//      Everyone usually implements this the same... feel free to use
//      this implementation.
//
STDMETHODIMP_(ULONG32)
PluginHandler::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }
    
    delete this;
    return 0;
}   

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
STDMETHODIMP_(ULONG32) PluginHandler::GetNumOfPlugins(void)
{
    if (m_general_handler)
    {
	return m_general_handler->GetNumOfPlugins();
    }

    return 0;
}

/************************************************************************
 *	Method:
 *	    IHXPluginEnumerator::GetPlugin
 *	Purpose:
 *	    return an instance(IUnknown) of the plugin
 *
 */
STDMETHODIMP PluginHandler::GetPlugin  
(
    ULONG32	    /*IN*/  ulIndex,
    REF(IUnknown*)  /*OUT*/ pInstance
)
{
    HX_RESULT	theErr = HXR_OK;
    Plugin*	pPlugin = NULL;

    pInstance = NULL;
 
    if (NULL == m_general_handler)
    {
	theErr = HXR_FAILED;
	goto cleanup;
    }

    if (!(pPlugin = (Plugin*) GetDataFromList(m_general_handler->m_pPlugins, ulIndex)))
    {
	theErr = HXR_FAILED;
	goto cleanup;
    }

    theErr = pPlugin->GetInstance(&pInstance);

cleanup:
    return theErr;
}

STDMETHODIMP
PluginHandler::ReloadPlugins()
{
    if(Refresh() == NO_ERRORS)
	return HXR_OK;
    return HXR_FAIL;
}


BOOL PluginHandler::PluginIsInRegistery(char* pszDllName)
{
    IHXBuffer*	    pBuffer	= NULL;
    char*	    pszInfo	= NULL;
    char*	    pszInfoCopy = NULL;
    char*	    pszDLLNameCopy = NULL;
    BOOL	    bRet	    = FALSE;


    if (!m_pPreferences)
    {
	return bRet;	
    }
    // Get the string which lists all DLLs in the registery 
    if (HXR_OK != m_pPreferences->ReadPref(REGKEY_DLLLIST, pBuffer))
    {
	return bRet;  //XXXAH ditto for above. 
    }
    pszInfo = (char*) pBuffer->GetBuffer(); 

    pszInfoCopy = new char[pBuffer->GetSize()+1];
    strcpy(pszInfoCopy, pszInfo);
    pszDLLNameCopy = new char[strlen(pszDllName)+1];
    strcpy(pszDLLNameCopy, pszDllName);

    strlwr(pszDLLNameCopy);
    strlwr(pszInfoCopy);

    // Should be case insensitive -- XXXAH
    
    if (strstr(pszInfoCopy, pszDLLNameCopy)) 
    {
	bRet = TRUE;
    }

    delete []  pszDLLNameCopy;
    delete []  pszInfoCopy; 
    HX_RELEASE(pBuffer);
    return bRet;
}

STDMETHODIMP
PluginHandler::GetNumPluginsGivenGroup(REFIID riid, REF(UINT32) /*OUT*/ unNumPlugins)
{
    if (IsEqualIID(riid, IID_IHXFileFormatObject))
    {
	if (m_meta_format_handler)
	{
	    unNumPlugins = m_file_format_handler->GetNumPlugins();
	    return HXR_OK;
	}
    } 
    else if (IsEqualIID(riid, IID_IHXMetaFileFormatObject))
    {
	if (m_meta_format_handler)
	{
	    unNumPlugins = m_meta_format_handler->GetNumPlugins();
	    return HXR_OK;
	}
    } 
    return HXR_FAIL;
}

STDMETHODIMP
PluginHandler::GetPluginInfo(REFIID riid, UINT32 unIndex, REF(IHXValues*) Values)
{
    Values = NULL;

    if ( (IsEqualIID(riid, IID_IHXMetaFileFormatObject) &&  m_meta_format_handler) 
	|| (IsEqualIID(riid, IID_IHXFileFormatObject) && m_file_format_handler) )
    {
	PluginHandler::FileFormat*	    pFormat;

	// funky. But it does work. 
	if (IsEqualIID(riid, IID_IHXMetaFileFormatObject))
	{
	    pFormat = m_meta_format_handler;
	}
	else
	{
	    pFormat = m_file_format_handler;
	}

	if (unIndex >= (UINT32) pFormat->GetNumPlugins())
	{
	    return HXR_FAILED;
	}

	char*		    pszDllPath;
	char*		    pszDescription;
	char*		    pszCopyright;
	char*		    pszMoreInfo;
	BOOL		    bMultiple;
	CHXSimpleList*	    pszMimeTypes;
	CHXSimpleList*	    pszExtensions;
	CHXSimpleList*	    pszOpenNames;
	CHXSimpleList::Iterator i;

	pFormat->GetPluginInfo(	unIndex,
				&pszDllPath,
				&pszDescription,
				&pszCopyright,
				&pszMoreInfo,
				&bMultiple,
				&pszMimeTypes,
				&pszExtensions, 
				&pszOpenNames);


        CHXHeader*  pHeader	= new CHXHeader;
	pHeader->QueryInterface(IID_IHXValues, (void**)&Values);
	    
	CHXBuffer*  pBuffer = new CHXBuffer;
	IHXBuffer*  pIHXBuffer = NULL;
	*pBuffer = pszDllPath;
	pBuffer->QueryInterface(IID_IHXBuffer, (void**) &pIHXBuffer);
	Values->SetPropertyCString(PLUGIN_PATH, pIHXBuffer);
	pBuffer->Release();

	pBuffer = new CHXBuffer;
	*pBuffer = pszDescription;
	pBuffer->QueryInterface(IID_IHXBuffer, (void**) &pIHXBuffer);
	Values->SetPropertyCString(PLUGIN_DESCRIPTION, pIHXBuffer);
	pBuffer->Release();

	pBuffer = new CHXBuffer;
	*pBuffer = pszCopyright;
	pBuffer->QueryInterface(IID_IHXBuffer, (void**) &pIHXBuffer);
	Values->SetPropertyCString(PLUGIN_COPYRIGHT, pIHXBuffer);
	pBuffer->Release();

	pBuffer = new CHXBuffer;
	*pBuffer = pszMoreInfo;
	pBuffer->QueryInterface(IID_IHXBuffer, (void**) &pIHXBuffer);
	Values->SetPropertyCString(PLUGIN_MOREINFO, pIHXBuffer);
	pBuffer->Release();

	CHXString RetString;
	for(i=pszMimeTypes->Begin(); i!=pszMimeTypes->End(); ++i)
	{
	    if (i!=pszMimeTypes->Begin())
	    {
		RetString += ", ";		
	    }
	    RetString += (char*) *i;
	}
	pBuffer = new CHXBuffer;
	*pBuffer = RetString;
	pBuffer->QueryInterface(IID_IHXBuffer, (void**) &pIHXBuffer);
	Values->SetPropertyCString(PLUGIN_MIMETYPES, pIHXBuffer);
	pBuffer->Release();

	RetString = "";
	for(i=pszExtensions->Begin(); i!=pszExtensions->End(); ++i)
	{
	    if (i!=pszExtensions->Begin())
	    {
		RetString += ", ";		
	    }
	    RetString += (char*) *i;
	}
	pBuffer = new CHXBuffer;
	*pBuffer = RetString;
	pBuffer->QueryInterface(IID_IHXBuffer, (void**) &pIHXBuffer);
	Values->SetPropertyCString(PLUGIN_EXTENSIONS, pIHXBuffer);
	pBuffer->Release();

	RetString = "";
	for(i=pszOpenNames->Begin(); i!=pszOpenNames->End(); ++i)
	{
	    if (i!=pszOpenNames->Begin())
	    {
		RetString += ", ";		
	    }
	    RetString += (char*) *i;
	}
	pBuffer = new CHXBuffer;
	*pBuffer = RetString;
	pBuffer->QueryInterface(IID_IHXBuffer, (void**) &pIHXBuffer);
	Values->SetPropertyCString(PLUGIN_OPENNAME, pIHXBuffer);
	pBuffer->Release();

	Values->SetPropertyULONG32(PLUGIN_MULTIPLE, bMultiple);

	return HXR_OK;		
    }
    return HXR_FAIL;
}


STDMETHODIMP
PluginHandler::Init(IUnknown* pContext)
{
    if (!pContext)
    {
	return HXR_INVALID_PARAMETER;
    }
    m_pContext = pContext;
    m_pContext->AddRef();

    HX_RESULT rc = m_pContext->QueryInterface(IID_IHXScheduler, 
					     (void**) &m_pScheduler);

    if (SUCCEEDED(rc))
    {
        rc = m_pContext->QueryInterface(IID_IHXPreferences, 
					     (void**) &m_pPreferences);
    }
    
    if (SUCCEEDED(rc))
    {
        /* We don't check errors because it's ok not to have this available. */
        m_pContext->QueryInterface(IID_IHXErrorMessages,
	    (void**) &m_pErrorMessages);
        
        m_allowance_handler->Init(m_pPreferences);
    }

    return rc;
}

STDMETHODIMP_(ULONG32)
PluginHandler::GetNumOfPlugins2(THIS)
{
    return GetNumOfPlugins();
}

STDMETHODIMP
PluginHandler::GetPluginInfo(THIS_ UINT32 ulIndex, REF(IHXValues*) pValues)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP
PluginHandler::FlushCache(THIS)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP
PluginHandler::SetCacheSize(THIS_ ULONG32 ulSizeKB)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP
PluginHandler::GetInstance(THIS_ UINT32 ulIndex, REF(IUnknown*) pUnknown)
{
    return GetPlugin(ulIndex, pUnknown);
}

STDMETHODIMP
PluginHandler::FindIndexUsingValues(THIS_ IHXValues*, REF(UINT32) ulIndex)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP
PluginHandler::FindPluginUsingValues(THIS_ IHXValues* pValues, 
                                     REF(IUnknown*) pInstance)
{
    pInstance = NULL;
    if (!pValues)
    {
        return HXR_INVALID_PARAMETER;
    }

    Errors rc = PLUGIN_NOT_FOUND;

    IHXBuffer* pTypeBuf;
    const char* szType;
    Plugin* pPlugin;

    if (SUCCEEDED(pValues->GetPropertyCString(PLUGIN_CLASS, pTypeBuf)))
    {
        szType = (const char*)pTypeBuf->GetBuffer();
        if (strcasecmp(szType, PLUGIN_STREAM_DESC_TYPE) == 0)
        {
            rc = m_stream_description_handler->FindUsingValues(pValues, 
                pPlugin);
        }
        else if (strcasecmp(szType, PLUGIN_FILEFORMAT_TYPE) == 0)
        {
            rc = m_file_format_handler->FindUsingValues(pValues, pPlugin);
        }
        else if (strcasecmp(szType, PLUGIN_METAFILEFORMAT_TYPE) == 0)
        {
            rc = m_meta_format_handler->FindUsingValues(pValues, pPlugin);
        }

        // Add other types here

        pTypeBuf->Release();
    }

    // if no type, there's currently nothing we can search for

    if (rc == NO_ERRORS)
    {
        rc = pPlugin->GetInstance(&pInstance);
    }

    return ConvertError(rc);
}

STDMETHODIMP
PluginHandler::FindIndexUsingStrings(THIS_ char* PropName1, 
				     char* PropVal1, 
				     char* PropName2, 
				     char* PropVal2, 
				     char* PropName3, 
				     char* PropVal3, 
				     REF(UINT32) unIndex)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP
PluginHandler::FindPluginUsingStrings(THIS_ char* PropName1, 
				      char* PropVal1, 
				      char* PropName2, 
				      char* PropVal2, 
				      char* PropName3, 
				      char* PropVal3, 
				      REF(IUnknown*) pUnk)
{
    // Initialize out params
    pUnk = NULL;

    IHXValues* pValues;
    HX_RESULT   retVal = HXR_FAIL;

    CHXHeader* pHeader = new CHXHeader();
    pHeader->QueryInterface(IID_IHXValues,  (void**)&pValues);

    AddToValues(pValues, PropName1, PropVal1);
    AddToValues(pValues, PropName2, PropVal2);
    AddToValues(pValues, PropName3, PropVal3);

    retVal = FindPluginUsingValues(pValues, pUnk);
    pValues->Release();

    return retVal;
}

STDMETHODIMP
PluginHandler::FindImplementationFromClassID(THIS_ REFGUID GUIDClassID, 
                                             REF(IUnknown*) pIUnknownInstance)
{
    return HXR_NOTIMPL;
}

PluginHandler::Errors
PluginHandler::StoreToRegistry()
{
    return NO_ERRORS;

    //XXX this code was disabled at some point in the past, not sure why

}


PluginHandler::Errors
PluginHandler::ReadFromRegistry()
{
    Errors  result = NO_ERRORS;

    if (!m_pPreferences)
    {
	result = INVALID_CONTEXT;
	goto cleanup;
    }

#if !defined (_SOLARIS) && !defined (_SUN)
    if (IsDirValid(m_pPreferences)!=NO_ERRORS)
#endif
    {
	return Refresh();
    }

    if (m_PluginFactory)
    {
	m_PluginFactory->ReadFromRegistry(m_pPreferences);
    }
    
    if (m_data_convert_handler)
    {
	m_data_convert_handler->ReadFromRegistry(m_pPreferences);
    }

    if (m_file_sys_handler)
    {
	m_file_sys_handler->ReadFromRegistry(m_pPreferences);
    }

    if (m_file_format_handler)
    {
	m_file_format_handler->ReadFromRegistry(m_pPreferences);
    }

    if (m_renderer_handler)
    {
	m_renderer_handler->ReadFromRegistry(m_pPreferences);
    }

    if (m_meta_format_handler)
    {
	m_meta_format_handler->ReadFromRegistry(m_pPreferences);
    }

    if (m_broadcast_handler)
    {
	m_broadcast_handler->ReadFromRegistry(m_pPreferences);
    }

    if (m_stream_description_handler)
    {
	m_stream_description_handler->ReadFromRegistry(m_pPreferences);
    }

    if (m_allowance_handler)
    {
	m_allowance_handler->ReadFromRegistry(m_pPreferences);
    }

    if (m_misc_handler)
    {
	m_misc_handler->ReadFromRegistry(m_pPreferences);
    }

    if (m_general_handler)
    {
	m_general_handler->ReadFromRegistry(m_pPreferences);
    }

    if(m_factory_handler)
    {
	m_factory_handler->ReadFromRegistry(m_pPreferences);
    }

cleanup:

    return result;
}

STDMETHODIMP
PluginHandler::SetRequiredPlugins(const char** ppszRequiredPlugins)
{
    IHXBuffer* pBuffer = NULL;
  
    if (m_pRequiredPlugins)
    {
	ClearRequiredPluginsList();
	delete m_pRequiredPlugins;
	m_pRequiredPlugins = NULL;
    }

    m_pRequiredPlugins = new CHXSimpleList();

    while (*ppszRequiredPlugins)
    {
	pBuffer = new CHXBuffer();
	pBuffer->AddRef();
	pBuffer->Set((const UCHAR*)*ppszRequiredPlugins, 
		     strlen(*ppszRequiredPlugins) + 1);
	m_pRequiredPlugins->AddTail((void*) pBuffer);
	ppszRequiredPlugins++;
    }

    return HXR_OK;
}

void
PluginHandler::EchoToStartupLog(const char* path, const char* prefix, const char* string)
{
    if (path && *path && prefix && string)
    {
        char *buffer = new char[strlen(prefix)+strlen(string)+1];
        strcpy(buffer, prefix);
        strcat(buffer, string);

        FILE *fp = fopen(path, "a");
        if (fp)
        {
            fprintf(fp, "%s\n", buffer);
            fclose(fp);
        }

        HX_VECTOR_DELETE(buffer);
    }
}

            /* Load each plug-in, read info, store in memory and update prefs*/
PluginHandler::Errors
PluginHandler::Refresh(BOOL bShowOutput)
{
    Errors				result		    = NO_ERRORS;
    BOOL				bIsMisc		    = TRUE;
    const char*				pszDllName	    = NULL;
    const char*				pszDllPath	    = NULL;
    Plugin*				pPlugin		    = NULL;
    char				pErrorTemp[2048];
    
    IUnknown*				pUnknown	    = NULL;
    IHXPlugin*				pHXPlugin	    = NULL;
    IHXDataConvertSystemObject*	pDataConvert	    = NULL;
    IHXFileSystemObject*		pFileSystem	    = NULL;
    IHXFileFormatObject*		pFileFormat	    = NULL;
    IHXMetaFileFormatObject*		pMetaFileFormat	    = NULL;
    IHXRenderer*			pRenderer	    = NULL;
    IHXBroadcastFormatObject*		pBroadcastFormat    = NULL;
    IHXStreamDescription*		pStreamDescription  = NULL;
    IHXPlayerConnectionAdviseSink*	pAllowanceFormat    = NULL;
    IHXCommonClassFactory*		pClassFactory       = NULL;

    IHXPluginFactory*			pFactory	    = NULL;
    PluginHandler::PluginDLL*		pPluginDll	    = NULL;
    DLLAccess*				pDllAccess	    = NULL;
    UINT16				nStart		    = 0;
	
    UINT32				ulIndex		    = 0;
    UINT32				ulNumRequired	    = 0;
    BOOL*				pPluginValidated    = NULL;

    IHXShutDownEverything*		pShutDownEverything = 0;
    int					i;

    if (m_pContext &&
	HXR_OK == m_pContext->QueryInterface(IID_IHXShutDownEverything,
				   (void**) &pShutDownEverything))
    {
	pShutDownEverything->ShutDown();	
	pShutDownEverything->Release();
    }

    // Look to see if the startup log filename
    // is in the registry.  Log loaded plugings to it
    IHXRegistry *pRegistry;
    char *szStartupLogPath = NULL;
    if (m_pContext &&
        HXR_OK == m_pContext->QueryInterface(IID_IHXRegistry, (void**)&pRegistry))
    {
        IHXBuffer *pBuf;
        pRegistry->GetStrByName("server.StartupLogPath", pBuf);
        if (pBuf)
        {
            szStartupLogPath = new char[strlen((const char*)pBuf->GetBuffer())+1];
            strcpy(szStartupLogPath, (const char*)pBuf->GetBuffer());
        }
    }
				    
    if (m_pRequiredPlugins)
    {
	ulNumRequired = (UINT32)m_pRequiredPlugins->GetCount();
	pPluginValidated = new BOOL[ulNumRequired];

	for (UINT32 i = 0; i < ulNumRequired; i++)
	{
	    pPluginValidated[i] = FALSE;
	}
    }

    // cleanup all the resources(i.e. plugin handlers)
    Clear();

    // get ordered list of plugins to try.

    result = RefreshOrderedPluginNames();
    if (result != NO_ERRORS) goto cleanup;

    // collect plugin info.
    for (i=0; i<m_nNumPluginNames; ++i)
    {
        pszDllName = m_pPluginName[i];
        pszDllPath = m_pPluginPath[i];
	
	/****************************************************
	Here we should be calling LoadDLL --- XXXAH
	But, of course it is too close to release to be messing 
	with such an essential feature of the plugin handler.
	******************************************************/


	// Check to see if the DLL Contains multiple plugins

	pFactory=0; pPluginDll=0; pDllAccess=0; nStart=0; //Check to see if this is necessary

	if (!(pPluginDll = new PluginHandler::PluginDLL(pszDllPath, m_pErrorMessages)))
	{
            fprintf(stderr, "Mem error.\n");
	    result = MEMORY_ERROR;
	    goto cleanup;
	}

	if (NO_ERRORS !=pPluginDll->Load()) 
	{
	    sprintf (pErrorTemp, "%-35s Not a valid library", pszDllPath);
	    if (m_pErrorMessages)
		m_pErrorMessages->Report(HXLOG_CRIT, 0, 0, pErrorTemp, 0);

            EchoToStartupLog(szStartupLogPath, "C: ", pErrorTemp);

	    bIsMisc = TRUE;
	    pPluginDll->Release(); // Clean up this plugin reference.
	    continue;
	}
	m_PluginFactory->Add(pPluginDll);	
	
	for(UINT16 i=0;i<pPluginDll->m_NumOfPlugins;i++)								
	{
	    UINT32 ulFlags = 0;
	    bIsMisc = TRUE;

    	    // create a new plugin object
	    if (!(pPlugin = new Plugin(pszDllPath, FALSE, m_pErrorMessages, pPluginDll,i)))
	    {
    		result = MEMORY_ERROR;
		goto cleanup;
	    }

	    // add reference
	    pPlugin->AddRef();

	    // initialize the plugin
	    if (NO_ERRORS != pPlugin->Init(m_pContext))
	    {
		sprintf (pErrorTemp, "%-35s Not a valid library", pszDllPath);
		if (m_pErrorMessages && bShowOutput)
    		    m_pErrorMessages->Report(HXLOG_CRIT, 0, 0, pErrorTemp, 0);

                EchoToStartupLog(szStartupLogPath, "C: ", pErrorTemp);

		// Clean up this plugin reference.
		pPlugin->Release();
		continue;
	    }   

	    // get instance(IUnknown*)
	    if  (NO_ERRORS != pPlugin->GetInstance(&pUnknown))
	    {
		result  = BAD_DLL;

		sprintf (pErrorTemp, "%-35s Not a valid library", pszDllPath);
		if (m_pErrorMessages)
		    m_pErrorMessages->Report(HXLOG_CRIT, 0, 0, pErrorTemp, 0);

                EchoToStartupLog(szStartupLogPath, "C: ", pErrorTemp);

		sprintf (pErrorTemp, "%-35s HXCreateInstance() failed\n",
	    	    pszDllPath);
		if (m_pErrorMessages && bShowOutput)
		    m_pErrorMessages->Report(HXLOG_DEBUG, 0, 0, pErrorTemp, 0);

                EchoToStartupLog(szStartupLogPath, "C: ", pErrorTemp);

		// Clean up this plugin reference.
		pPlugin->Release();
		continue;
	    }

	    // get RMA plugin object
	    if (HXR_OK != pUnknown->QueryInterface(IID_IHXPlugin, (void**) &pHXPlugin))
	    {
		sprintf (pErrorTemp, "%-35s Not a valid library",pszDllPath);
		if (m_pErrorMessages && bShowOutput)
		    m_pErrorMessages->Report(HXLOG_CRIT, 0, 0, pErrorTemp, 0);

                EchoToStartupLog(szStartupLogPath, "C: ", pErrorTemp);

		sprintf (pErrorTemp, "%-35s QueryInterface(IHXPlugin) failed\n",
		    pszDllPath);
		if (m_pErrorMessages && bShowOutput)
		    m_pErrorMessages->Report(HXLOG_DEBUG, 0, 0, pErrorTemp, 0);

                EchoToStartupLog(szStartupLogPath, "C: ", pErrorTemp);

		result = BAD_DLL;    

		// Clean up this plugin reference.
		pUnknown->Release();
		pPlugin->Release();
		continue;
	    }

	    const char *pDesc, *pCopy, *pURL;
	    ULONG32 ulVersionNumber = 0;
	    BOOL junk;

	    pHXPlugin->GetPluginInfo(junk, pDesc, pCopy, pURL, ulVersionNumber);

	    // add it to the general handler which tracks all plugins
	    m_general_handler->Add(pPlugin);

	    // if this is a required plugin, validate it
	    if (IsPluginRequired(pDesc, ulIndex))
	    {
		pPluginValidated[ulIndex] = ValidateRequiredPlugin(pHXPlugin, pDesc);
		ulFlags |= 1;
	    }

	    //
	    // filter the plugins	
	    //
	    
	    // file system
	    if (HXR_OK == pHXPlugin->QueryInterface(IID_IHXFileSystemObject, (void**) &pFileSystem))
	    {
    		bIsMisc = FALSE;
		m_file_sys_handler->Add(pPlugin);
		pFileSystem->Release();
		ulFlags |= 2;
	    }
	
	    // file format
	    if (HXR_OK == pHXPlugin->QueryInterface(IID_IHXFileFormatObject, (void**)&pFileFormat))
	    {
		bIsMisc = FALSE;
		m_file_format_handler->Add(pPlugin);
		pFileFormat->Release();
		ulFlags |= 4;
	    }
	
	    // meta file format
	    if (HXR_OK == pHXPlugin->QueryInterface(IID_IHXMetaFileFormatObject, (void**)&pMetaFileFormat))
	    {
		bIsMisc = FALSE;
		m_meta_format_handler->Add(pPlugin);
		pMetaFileFormat->Release();
		ulFlags |= 8;
	    }

	    // renderer
	    if (HXR_OK == pHXPlugin->QueryInterface(IID_IHXRenderer, (void**)&pRenderer))
	    {
		bIsMisc = FALSE;
		m_renderer_handler->Add(pPlugin);
		pRenderer->Release();
		ulFlags |= 16;
	    }

	    // broadcast
	    if (HXR_OK == pHXPlugin->QueryInterface(IID_IHXBroadcastFormatObject, (void**)&pBroadcastFormat))
	    {
		bIsMisc = FALSE;
		m_broadcast_handler->Add(pPlugin);
		pBroadcastFormat->Release();
		ulFlags |= 32;
	    }

	    // stream description
	    if (HXR_OK == pHXPlugin->QueryInterface(IID_IHXStreamDescription, (void**)&pStreamDescription))
	    {
		bIsMisc = FALSE;
		m_stream_description_handler->Add(pPlugin);
		pStreamDescription->Release();
		ulFlags |= 64;
	    }

	    // allowance
	    if (HXR_OK == pHXPlugin->QueryInterface(IID_IHXPlayerConnectionAdviseSinkManager, (void**)&pAllowanceFormat))
	    {
		bIsMisc = FALSE;
		m_allowance_handler->Add(pPlugin);
		pAllowanceFormat->Release();
		ulFlags |= 128;
	    }

	    if (HXR_OK == pHXPlugin->QueryInterface(IID_IHXPlayerConnectionAdviseSink, (void**)&pAllowanceFormat))
	    {
		bIsMisc = FALSE;
		m_allowance_handler->Add(pPlugin);
		pAllowanceFormat->Release();
		ulFlags |= 256;
	    }

	    // common class factory
	    if(HXR_OK == pHXPlugin->QueryInterface(IID_IHXCommonClassFactory,
						(void**)&pClassFactory))
	    {
		bIsMisc = FALSE;
		m_factory_handler->Add(pPlugin);
		pClassFactory->Release();
		ulFlags |= 512;
	    }

	    if (bIsMisc)
	    {
		m_misc_handler->Add(pPlugin);
		ulFlags |= 1024;
	    }

	    // data convert
	    /* special case:  04.02.01  Damon                       */
	    /* right now we want to enforce a license restriction   */
	    /* on the use of the RPK (SecureMedia) data conversion  */
	    /* plugins.  therefore, we do some logic to ensure that */
	    /* this server is licensed for DataConversion, before   */
	    /* loading the RPK plugin                               */
	    
	    if (HXR_OK == pHXPlugin->QueryInterface(
			IID_IHXDataConvertSystemObject, (void**)&pDataConvert))
	    {
		UINT32 len = strlen (RPK_DATA_CONVERT_PLUGIN_NAME);
		
		if (!strncasecmp(pszDllName, RPK_DATA_CONVERT_PLUGIN_NAME, len))
		{

		    IHXRegistry* pRegistry = NULL;
		    INT32           lTempInt;
		    BOOL            bLicensed = FALSE;

		    // determine if we are licensed or not.
		    if (HXR_OK == m_pContext->QueryInterface(IID_IHXRegistry, (void**)&pRegistry))
		    {
			if (HXR_OK != pRegistry->GetIntByName(REGISTRY_DATA_CONVERT_ENABLED, lTempInt))
			{
			    bLicensed = (BOOL)LICENSE_DATA_CONVERT_ENABLED;
			}
			else
			{
			    bLicensed = (BOOL)lTempInt;
			}
		
			if (bLicensed)
			{
			    /* add the plugin to our map */
			    bIsMisc = FALSE;
			    m_data_convert_handler->Add(pPlugin);
			    ulFlags |= 2048;
			}
			else
			{
			    /* write an error to rmerror */
			    sprintf (pErrorTemp, "This Server is not licensed for Stream Encryption");
			    if (m_pErrorMessages && bShowOutput)
			    {
				m_pErrorMessages->Report(HXLOG_ALERT, HXR_FAIL, 0, pErrorTemp, NULL);
			    }

                            EchoToStartupLog(szStartupLogPath, "C: ", pErrorTemp);
			}
		    }
		    HX_RELEASE(pRegistry);
		}
		else
		{
		    /* add the plugin to our map */
		    bIsMisc = FALSE;
		    m_data_convert_handler->Add(pPlugin);
		    ulFlags |= 2048;
		}
		HX_RELEASE(pDataConvert);
	    }

	    bIsMisc = TRUE;

	    sprintf (pErrorTemp, "%-16s %p  %s", pszDllName,
                     pPluginDll->m_fpCreateInstance, pDesc);
	    if (m_pErrorMessages && bShowOutput)
	        m_pErrorMessages->Report(HXLOG_INFO, 0, 0, pErrorTemp, 0);

            EchoToStartupLog(szStartupLogPath, "I: ", pErrorTemp);

	    sprintf (pErrorTemp, "%-16lu ", ulVersionNumber);
	    for (int i = 15; i >= 0; i--)
	    {
		/* Slow, but it's ok */
		strcat(pErrorTemp, (ulFlags >> i) & 1 ? "1" : "0");
	    }
	    if (m_pErrorMessages && bShowOutput)
		m_pErrorMessages->Report(HXLOG_DEBUG, 0, 0, pErrorTemp, 0);

            EchoToStartupLog(szStartupLogPath, "D: ", pErrorTemp);

	    // release instance
	    pUnknown->Release();
	    pHXPlugin->Release();

	    pPlugin->Release();

	}
    }

    InitMountPoints();
    StoreToRegistry();

//XXXVS: for debugging purposes 
//  m_allowance_handler->PrintDebugInfo();
cleanup:

    ClearOrderedPluginNames();		// don't need to use this memory

    for (UINT32 j = 0; j < ulNumRequired; j++)
    {
	if (!pPluginValidated[j])
	{
	    // Output the description of each required plugin
	    // that was not found in the plugins directory
	    ReportRequiredPluginError(j);
	    result = REQUIRED_PLUGIN_NOT_LOADED;
	}
    }

    if (pPluginValidated)
    {
	delete []pPluginValidated;
	pPluginValidated = NULL;
    }

    return result;
}

BOOL
PluginHandler::IsPluginRequired(const char* pszDescription, UINT32& ulIndex)
{
    BOOL bRequired = FALSE;
    CHXSimpleList::Iterator i;
    IHXBuffer* pBuffer = NULL;
    UINT32 ulMatch = 0;

    if (!m_pRequiredPlugins)
    {
	return FALSE;
    }

    for (i = m_pRequiredPlugins->Begin(); i != m_pRequiredPlugins->End(); ++i)
    {
	pBuffer = (IHXBuffer*) (*i);
	if (pBuffer && !stricmp(pszDescription, (char*)pBuffer->GetBuffer()))
	{
	    bRequired = TRUE;
	    break;
	}
	ulMatch++;
    }

    // If we found a match, return the list index of
    // the matching plugin description in the out parameter
    ulIndex = bRequired ? ulMatch : 0;

    return bRequired;
}

BOOL
PluginHandler::ValidateRequiredPlugin(IHXPlugin* pHXPlugin, 
				      const char* pszDescription)
{
    return TRUE;
}

void 
PluginHandler::ReportRequiredPluginError(UINT32 ulIndex)
{
    IHXBuffer* pBuffer = NULL;
    LISTPOSITION lPos = NULL;
    char pErrorTemp[2048];

    // Get the Plugin Description
    HX_ASSERT(m_pRequiredPlugins);
    lPos = m_pRequiredPlugins->FindIndex(ulIndex);
    pBuffer = (IHXBuffer*)m_pRequiredPlugins->GetAt(lPos);
    HX_ASSERT(pBuffer);

    if (pBuffer)
    {
	sprintf (pErrorTemp, 
	    "Fatal Error: The following required plugin could not be loaded: %s", 
	    (char*)pBuffer->GetBuffer());
    }
    else
    {
	sprintf (pErrorTemp, 
	    "Fatal Error: A required plugin could not be loaded.");	
    }

    if (m_pErrorMessages)
	m_pErrorMessages->Report(HXLOG_EMERG, 0, 0, pErrorTemp, 0);
}

void
PluginHandler::Clear()
{
    if(m_data_convert_handler)
    {
	m_data_convert_handler->Clear();
    }
    if(m_file_sys_handler)
    {
	m_file_sys_handler->Clear();
    }
    if(m_file_format_handler)
    {
	m_file_format_handler->Clear();
    }
    if(m_meta_format_handler)
    {
	m_meta_format_handler->Clear();
    }
    if(m_renderer_handler)
    {
	m_renderer_handler->Clear();
    }
    if(m_allowance_handler)
    {
	m_allowance_handler->Clear();
    }
    if(m_misc_handler)
    {
	m_misc_handler->Clear();
    }
    if(m_general_handler)
    {
	m_general_handler->Clear();
    }
    if(m_broadcast_handler)
    {
	m_broadcast_handler->Clear();
    }
    if(m_factory_handler)
    {
	m_factory_handler->Clear();
    }
    if(m_PluginFactory)
    {
	m_PluginFactory->Clear();
    }
}

void 
PluginHandler::FreeAllLibraries(void)
{
#ifdef _WIN32
    // This function should only be called when you're 
    // shutting down an NT service.  Sometimes Windows 
    // service manager gets moody about the performance 
    // plugin not being unloaded.  Even when we stop all
    // the threads and processes that use the DLL, the 
    // service control manager still locks up unless you
    // do FreeLibrary.
    CHXSimpleList* pList = m_PluginFactory->m_pPluginDlls;
    CHXSimpleList::Iterator i;

    for (i = pList->Begin(); i != pList->End(); ++i)
    {
	PluginDLL* pPluginDll = (PluginDLL*) (*i);
	if (pPluginDll->m_bLoaded)
	{
	    pPluginDll->m_pDllAccess->close();
	}
    }
#endif
}

void
PluginHandler::ClearRequiredPluginsList()
{
    CHXSimpleList::Iterator	i;
    IHXBuffer*			pBuffer = NULL;

    for (i = m_pRequiredPlugins->Begin(); i != m_pRequiredPlugins->End(); ++i)
    {
	pBuffer = (IHXBuffer*) (*i);
	pBuffer->Release();
    }

    m_pRequiredPlugins->RemoveAll();
}

void
PluginHandler::ParseInfoArray(CHXSimpleList* string_list, 
			      const char** info_array)
{
    char* pszTemp;
    if (!info_array)
	return;

    while (*info_array)
    {
	pszTemp = new char[strlen(*info_array) +1];
	strcpy(pszTemp, *info_array);
	string_list->AddTail((void*) pszTemp);
	info_array++;
    }
}

void
PluginHandler::ClearStringList(CHXSimpleList* string_list)
{
    CHXSimpleList::Iterator	i;
    char*			delete_str = NULL;

    if (string_list == 0)
    {
	return;
    }

    for (i = string_list->Begin(); i != string_list->End(); ++i)
    {
	delete[] (char*) (*i);
    }
}

const char*		
PluginHandler::GetDefaultPluginDir()
{
    if (m_pszDefaultPluginDir)
    {
	return m_pszDefaultPluginDir;
    }

    char mask_name[_MAX_PATH + 1] = "";

#if (defined (_WINDOWS) || defined (_WIN32)	) && !defined(_WINCE)
    if (!GetSystemDirectory(mask_name, _MAX_PATH))
    {
	strcpy(mask_name, "");
    }

    if (strlen(mask_name) > 0 && mask_name[strlen(mask_name) - 1] != '\\')
    {
	    strcat(mask_name, "\\");
    }

    strcat(mask_name, "Real");
#elif defined (_UNIX)
    strcpy(mask_name, getenv("HOME"));
    strcat(mask_name, "/Real");
#elif defined (__MWERKS__)
    FSSpec extSpec;
    extSpec.name[0] = 0;
    if (noErr == ::FindFolder (-1, kExtensionFolderType, kDontCreateFolder, 
				&extSpec.vRefNum, &extSpec.parID))
    {
	CHXString str_path;
	PathNameFromFSSpec(&extSpec, str_path);
	strcpy(mask_name, (char*)(const char*)str_path);
    }
    else
	strcpy(mask_name, ":System Folder:Extensions:");

    strcat(mask_name, "Real");
#elif defined(_WINCE)
	strcpy(mask_name, "\\");
#endif //defined (_WINDOWS) || defined (_WIN32)

    ULONG32 mask_len  = strlen(mask_name) + 1;
    
    m_pszDefaultPluginDir = new char[mask_len];
    strcpy(m_pszDefaultPluginDir, mask_name);

    return m_pszDefaultPluginDir;
}

char*
PluginHandler::GetPluginDir()
{
    char*	pszPluginDir = NULL;
    const char*	pPath = NULL;

    if (m_pszPluginDir)
    {
	return new_string(m_pszPluginDir);
    }

    // Get the plugin directory from the Dll Access Paths
    pPath = GetDLLAccessPath()->GetPath(DLLTYPE_PLUGIN);
    if (!pPath || !pPath[0])
    {
	GetDLLAccessPath()->SetPath(DLLTYPE_PLUGIN, 
	    GetDefaultPluginDir());

	/* try again to read plugin dir */
	pPath = GetDLLAccessPath()->GetPath(DLLTYPE_PLUGIN);
	if (!pPath || !pPath[0])
	{
	    return NULL;
	}
    }

    pszPluginDir = new char[strlen(pPath) + 1];
    strcpy(pszPluginDir, pPath);
   
    return pszPluginDir;
}

/*
 * PluginHandler::RefreshOrderedPluginNames()
 *
 * Get an ordered list of plugin names for loading.
 *
 * The order is: names in order from load list in registry, then random.
 * Example .cfg:
 *
 * <List Name="PluginLoadOrder">
 *  <Var 1="lice"/>
 *  <Var 2="wawa"/>
 *  <Var 3="b"/>
 * </List>
 */
typedef struct Regname
{
    Regname* next;
    int priority;
    char* plugin;
} Regname;

PluginHandler::Errors
PluginHandler::RefreshOrderedPluginNames()
{
    Errors       result = NO_ERRORS;
    CFindFile*   pFileFinder = NULL;
    char*        pszPluginDir = NULL;
    const char*  pszDllName = NULL;
    const char*  pszDllPath = NULL;
    int          i;
    int          nextOrderedPos;

    IHXRegistry* pRegistry = NULL;
    IHXValues* pValues = NULL;
    HX_RESULT hResult;
    UINT32 ulID = 0;
    const char* pName = NULL;

    Regname* registryhead = NULL;
    Regname* ptr;
    Regname* prev;

    // reset list to zero

    ClearOrderedPluginNames();

#ifdef _WINDOWS
    const char*	pszFileExtension = "*.dll";
#elif _UNIX
    const char*	pszFileExtension = "*.so*";
#elif _MACINTOSH
    const char*	pszFileExtension = "*.dll";
#endif

    // get the location of the plugins
    pszPluginDir = GetPluginDir();
    if (NULL == pszPluginDir)
    {
	result = CANT_DETERMINE_PLUGIN_DIR;
	goto cleanup;
    }

    // Get list of all names and directories

    pFileFinder = CFindFile::CreateFindFile(pszPluginDir, 0, pszFileExtension);

    if (NULL == pFileFinder)
    {
	result = CANT_OPEN_PLUGIN_DIR;
	goto cleanup;
    }

    // Get a plugin count

    HX_ASSERT(!m_nNumPluginNames);
    pszDllName = pFileFinder->FindFirst();
    while (pszDllName)
    {
        ++m_nNumPluginNames;
        pszDllName = pFileFinder->FindNext();
    }

    // alloc char arrays

    m_pPluginName = new char*[m_nNumPluginNames];
    m_pPluginPath = new char*[m_nNumPluginNames];

    // Enumerate again to get all the names and paths

    i = 0;
    pszDllName = pFileFinder->FindFirst();
    while (pszDllName)
    {
        int size = strlen(pszDllName) + 1;

        m_pPluginName[i] = new char[size];
        memcpy(m_pPluginName[i], pszDllName, size);

        pszDllPath = pFileFinder->GetCurFilePath();
        size = strlen(pszDllPath) + 1;

        m_pPluginPath[i] = new char[size];
        memcpy(m_pPluginPath[i], pszDllPath, size);

        ++i;
        pszDllName = pFileFinder->FindNext();
    }

    // Order list according to names from registry, if any.
    // Get list from registry

    if (HXR_OK != m_pContext->QueryInterface(IID_IHXRegistry,
                                             (void **)&pRegistry))
    {
        result = MEMORY_ERROR;
        goto cleanup;
    }

    pRegistry->GetPropListByName(PLUGINHANDLER_REGISTRY_LOAD_LIST, pValues);
    if (!pValues) goto cleanup;

    hResult = pValues->GetFirstPropertyULONG32(pName, ulID);

    while (pName != NULL && hResult == HXR_OK)
    {
        IHXBuffer* pBuffer = NULL;
        pRegistry->GetStrById(ulID, pBuffer);
        if (pBuffer && pBuffer->GetBuffer())
        {
            Regname* entry = new Regname;

            entry->priority = atoi(pName);

            int size = pBuffer->GetSize() + 1;
            entry->plugin =  new char [ size ];
            memcpy(entry->plugin, pBuffer->GetBuffer(), size - 1);
            entry->plugin[size-1] = '\0';

            pBuffer->Release();

            // insert in list in priority order

            prev = NULL;
            ptr = registryhead;
            while (1)
            {
                if (ptr == NULL || ptr->priority > entry->priority)
                {
                    if (prev)
                    {
                        prev->next = entry;
                    }
                    else
                    {
                        registryhead = entry;
                    }
                    entry->next = ptr;
                    break;
                }
                prev = ptr;
                ptr = ptr->next;
            }
        }
	hResult = pValues->GetNextPropertyULONG32(pName, ulID);
    }

    // walk the ordered list we made from the registry, moving any 
    // matches to the front of the plugin list.

    nextOrderedPos = 0;
    ptr = registryhead;
    while (ptr)
    {
        for (int i=nextOrderedPos; i<m_nNumPluginNames; ++i)
        {
            // there's an implicit * at the end of the registry name,
            // so only compare as many characters as the reg name has.

            if (!strncasecmp(ptr->plugin, m_pPluginName[i], strlen(ptr->plugin)))
            {
                // maybe a waste, but I'm preserving the plugin order
                // by copying the names down before inserting in front.

                int memsize = (i - nextOrderedPos) * sizeof(char*);

                if (memsize > 0)
                {
                    char* pTmpName = m_pPluginName[i];
                    char* pTmpPath = m_pPluginPath[i];

                    memmove(&m_pPluginName[nextOrderedPos+1], 
                            &m_pPluginName[nextOrderedPos], memsize);

                    memmove(&m_pPluginPath[nextOrderedPos+1], 
                            &m_pPluginPath[nextOrderedPos], memsize);

                    m_pPluginName[nextOrderedPos] = pTmpName;
                    m_pPluginPath[nextOrderedPos] = pTmpPath;
                }
                if (memsize >= 0) ++nextOrderedPos;
            }
        }
        ptr = ptr->next;
    }

  cleanup:

    HX_RELEASE(pRegistry);
    HX_RELEASE(pValues);

    delete pFileFinder;
    delete[] pszPluginDir;

    prev = registryhead;
    while (prev)
    {
        ptr = prev->next;
        delete [] prev->plugin;
        delete prev;
        prev = ptr;
    }

    return result;
}

void
PluginHandler::ClearOrderedPluginNames()
{
    for (int i=0; i<m_nNumPluginNames; ++i)
    {
        delete [] m_pPluginName[i];
        delete [] m_pPluginPath[i];
    }
    delete [] m_pPluginName;
    delete [] m_pPluginPath;

    m_pPluginName = NULL;
    m_pPluginPath = NULL;
    m_nNumPluginNames = 0;
}

HX_RESULT
PluginHandler::AddToValues(IHXValues* pValues, char* pPropName, char* pPropVal)
{
    if (!pValues || !pPropName)
    {
        return HXR_FAIL;
    }

    // 1st make into a cstrig and to trim the buffer...
    CHXString theValue = (pPropVal);
    theValue.TrimLeft();
    theValue.TrimRight();

    IHXBuffer* pBuffer;
    CHXBuffer*	pCHXBuffer;
    pCHXBuffer = new CHXBuffer;
    pCHXBuffer->QueryInterface(IID_IHXBuffer, (void**) &pBuffer);
    pBuffer->Set((const unsigned char*)(const char*)theValue, strlen(theValue)+1);
    pValues->SetPropertyCString(pPropName, pBuffer);
    pBuffer->Release();

    return HXR_OK;
}

///////////////////////////////////////////////////////////////////////////////
//
// PluginHandler::FileFormat
//
///////////////////////////////////////////////////////////////////////////////
PluginHandler::FileFormat::FileFormat(ULONG32 ulType, PluginHandler* pparent)
{
    m_ulType = ulType;
    m_pparent = pparent;
    m_pPlugins = new CHXSimpleList();
    m_pMimeMap = new CHXMapStringToOb();
    m_pExtensionMap = new CHXMapStringToOb();
}

PluginHandler::FileFormat::~FileFormat()
{
    Clear();

    if (m_pPlugins)
    {
	delete m_pPlugins;
	m_pPlugins = NULL;
    }

    if (m_pMimeMap)
    {
	delete m_pMimeMap;
	m_pMimeMap = NULL;
    }

    if (m_pExtensionMap)
    {
	delete m_pExtensionMap;
	m_pExtensionMap = NULL;
    }
}

PluginHandler::Errors
PluginHandler::FileFormat::StoreToRegistry(IHXPreferences** ppRegistry)
{
    Errors		    result = NO_ERRORS;
    int			    j = 0;
    char*		    pszData = NULL;
    char*		    pszInfo = NULL;
    char*		    pszTemp = NULL;
    CHXSimpleList*	    pList = NULL;
    PluginInfo*		    pPluginInfo = NULL;
    IHXBuffer*		    pBuffer = NULL;
    CHXSimpleList::Iterator i;
    char		    CharBuffer[_MAX_PATH];


    if (!ppRegistry)
    {
	result = BAD_REGISTRY_HANDLE;
	goto cleanup;
    }

    if (!*ppRegistry)
    {
	result = BAD_REGISTRY_HANDLE;
	goto cleanup;
    }

    // parse/collect all the plugin info.
    for (i = m_pPlugins->Begin(); i != m_pPlugins->End(); ++i)
    {
	// {
	if (!pszInfo)
	{
	    pszInfo = new char[2];
	    strcpy(pszInfo, "{");
	}
	else
	{
	    pszTemp = new char[strlen(pszInfo)+2];
	    sprintf(pszTemp, "%s%s", pszInfo, "{");
	    
	    delete[] pszInfo;
	    pszInfo = pszTemp;
	}

	pPluginInfo = (PluginInfo*) (*i);

	for (j = 0; j < FILEFORMAT_PLUGIN_RECORD_ENTRIES; j++)
	{
	    switch (j)
	    {
	    case 0:
		pszData = pPluginInfo->m_pPlugin->m_pszDllName;
		pList = NULL;
		break;
	    case 1: 
		sprintf(CharBuffer,"%d",pPluginInfo->m_pPlugin->m_nPluginIndex);
		pszData = CharBuffer;
		pList = NULL;
		break;
	    case 2:	    // {<description>
		pszData = (char*) pPluginInfo->m_pPlugin->m_pszDescription;
		pList = NULL;
		break;
	    case 3:	    // {<description>,<copyright>
		pszData = (char*) pPluginInfo->m_pPlugin->m_pszCopyright;
		pList = NULL;
		break;
	    case 4:	    // {<description>,<copyright>,<moreinfo>
		pszData = (char*) pPluginInfo->m_pPlugin->m_pszMoreInfoUrl;
		pList = NULL;
		break;
	    case 5:	    // {<description>,<copyright>,<moreinfo>,<loadmultiple>
		if (pPluginInfo->m_pPlugin->m_load_multiple)
		{
		    pszData = "TRUE";
		}
		else
		{
		    pszData = "FALSE";
		}
		pList = NULL;
		break;
	    case 6:
		pszData = NULL;
		pList = &(pPluginInfo->m_mimeTypes);
		break;
	    case 7:
		pszData = NULL;
		pList = &(pPluginInfo->m_extensions);
		break;
	    case 8:
		pszData = NULL;
		pList = &(pPluginInfo->m_OpenNames);
	    default:
		break;
	    }

	    if (j == 6 || j == 7 || j==8)
	    {		
		AppendListToRecord(pList, &pszInfo);
	    }
	    else
	    {
		AppendStringToRecord(pszData, &pszInfo);
	    }

	    if (j != FILEFORMAT_PLUGIN_RECORD_ENTRIES - 1)
	    {
		// {xxx|xxx,
		pszTemp = new char[strlen(pszInfo)+2];
		sprintf(pszTemp, "%s%c", pszInfo, PLUGIN_ENTRY_SEPARATOR);

		delete[] pszInfo;
		pszInfo = pszTemp;
	    }
	}
	
	// {xxx,xxx,xxx}
	pszTemp = new char[strlen(pszInfo)+2];
	sprintf(pszTemp, "%s}", pszInfo);
	
	delete[] pszInfo;
	pszInfo = pszTemp;
    }	    
    
    if (pszInfo)
    {
	if (!(pBuffer = new CHXBuffer()))
	{
	    result = MEMORY_ERROR;
	    goto cleanup;
	}

	pBuffer->AddRef();
    
	pBuffer->Set((UCHAR*)pszInfo, strlen(pszInfo)+1);

	if (META_FILE_FORMAT == m_ulType)
	{
	    (*ppRegistry)->WritePref(REGKEY_METAFORMAT_PLUGIN, pBuffer);
	}
	else
	{
	    (*ppRegistry)->WritePref(REGKEY_FILEFORMAT_PLUGIN, pBuffer);
	}
	
	pBuffer->Release();
    }

cleanup:
    if (pszInfo)
    {
	delete[] pszInfo;
	pszInfo = NULL;
    }

    return result;
}

PluginHandler::Errors
PluginHandler::FileFormat::ReadFromRegistry(IHXPreferences* pRegistry)
{
    Errors	result		= NO_ERRORS;
    IHXBuffer*	pBuffer		= NULL;
    char*	pszDllName	= NULL;
    char*	pszLoadMultiple = NULL;
    char*       pszindex	= NULL;
    Plugin*	pPlugin		= NULL;
    PluginDLL*	pPluginDLL	= NULL;
    char*	pszInfo		= NULL;
    char*	pszPluginIndex	= NULL;

    if (!pRegistry)
    {
	result = BAD_REGISTRY_HANDLE;
	goto cleanup;
    }

    // clear the existing ones
    Clear();

    if (META_FILE_FORMAT == m_ulType)
    {
	// load from the registry
	if (HXR_OK != pRegistry->ReadPref(REGKEY_METAFORMAT_PLUGIN, pBuffer))
	{
	    result = PLUGIN_NOT_FOUND;
	    goto cleanup;
	}
    }
    else
    {
	// load from the registry
	if (HXR_OK != pRegistry->ReadPref(REGKEY_FILEFORMAT_PLUGIN, pBuffer))
	{
	    result = PLUGIN_NOT_FOUND;
	    goto cleanup;
	}
    }

    pszInfo = (char*) pBuffer->GetBuffer();

    // create plugins based on the info
    while (strlen(pszInfo))
    {
	// get dll name
	pszInfo++;
	RetrieveStringFromRecord(&pszInfo, &pszDllName);
		       
	// get the association...
	pPluginDLL=m_pparent->m_PluginFactory->FindDLLFromName(pszDllName);
	RetrieveStringFromRecord(&pszInfo, &pszindex);
	// create plugin
	pPlugin = new Plugin(pszDllName, FALSE, 0, pPluginDLL, atoi(pszindex));
	pPlugin->AddRef();
	delete[] pszindex;

	// get the plugin index
	pszInfo++;
	RetrieveStringFromRecord(&pszInfo, &pszPluginIndex);
	pPlugin->m_nPluginIndex = atoi(pszPluginIndex);
	delete[] pszPluginIndex;

	// get description
	pszInfo++;
	RetrieveStringFromRecord(&pszInfo, (char**)&(pPlugin->m_pszDescription));

	// get copyright
	pszInfo++;
	RetrieveStringFromRecord(&pszInfo, (char**)&(pPlugin->m_pszCopyright));

	// get moreinfo
	pszInfo++;
	RetrieveStringFromRecord(&pszInfo, (char**)&(pPlugin->m_pszMoreInfoUrl));

	// get loadmultiple
	pszInfo++;
	RetrieveStringFromRecord(&pszInfo, &(pszLoadMultiple));

	if (strcmp(pszLoadMultiple, "TRUE") == 0)
	{
	    pPlugin->m_load_multiple = TRUE;
	}
	else
	{
	    pPlugin->m_load_multiple = FALSE;
	}

	// retreive the mime types and extensions

	CHXSimpleList mimeTypesList;
	CHXSimpleList extensionList;
	CHXSimpleList OpenFileNameList;

	pszInfo++;
	RetrieveListFromRecord(&pszInfo, &mimeTypesList);

	pszInfo++;
	RetrieveListFromRecord(&pszInfo, &extensionList);

	pszInfo++;
	RetrieveListFromRecord(&pszInfo, &OpenFileNameList);

	// pass through the end of this entry
	while (*pszInfo != '}')
	{
	    pszInfo++;
	}

	// add this plugin
	Add(pPlugin, &mimeTypesList, &extensionList, &OpenFileNameList);

	// freeup
	delete[] pszDllName;
	delete[] pszLoadMultiple;

	pszDllName = NULL;
	pszLoadMultiple = NULL;

	pPlugin->Release();

	pszInfo++;
    }	

cleanup:

    if (pBuffer)
    {
	pBuffer->Release();
	pBuffer = NULL;
    }

    return result;
}

PluginHandler::Errors
PluginHandler::FileFormat::Find(const char* pszMimeType, const char* pszExtension, 
				PluginHandler::Plugin*& plugin)
{
    PluginInfo*	pPluginInfo = NULL;

    if (!(pPluginInfo = FindPluginInfo(pszMimeType, pszExtension)))
    {
	return PLUGIN_NOT_FOUND;
    }

    plugin = pPluginInfo->m_pPlugin;

    return NO_ERRORS;
}

PluginHandler::FileFormat::PluginInfo*
PluginHandler::FileFormat::FindPluginInfo(const char* pszMimeType, 
					  const char* pszExtension)
{
    PluginInfo*	pPluginInfo = NULL;

    /* We first look for plugins supporitng given mime type*/
    if (pszMimeType)
    {
	if (m_pMimeMap->Lookup(pszMimeType, (void*&)pPluginInfo) != FALSE)
	{
	    return pPluginInfo;
	}
    }

    /* If not found, look for matching extension */
    if (pszExtension)
    {
	CHXString pnExtString(pszExtension);

	pnExtString.MakeLower();
	if (m_pExtensionMap->Lookup((const char*)pnExtString, (void*&)pPluginInfo) != FALSE)
	{
	    return pPluginInfo;
	}
    }

    return NULL;
}

PluginHandler::Errors
PluginHandler::FileFormat::MapFromExtToMime(const char* pszExtension, 
					    const char*& pszMimeType)
{
    PluginInfo*	pPluginInfo = NULL;

    CHXString pnExtString(pszExtension);

    pnExtString.MakeLower();
    if (!(pPluginInfo = FindPluginInfo(0, (const char*)pnExtString)))
    {
	return PLUGIN_NOT_FOUND;
    }

    pszMimeType = (const char*)pPluginInfo->m_mimeTypes.GetHead();
    
    return NO_ERRORS;
}

UINT32
PluginHandler::FileFormat::GetPriority(PluginInfo* pPlugin)
{
    UINT32 ulCount = 0;
    char buffer[4096];
    IHXBuffer* pBuffer;
    BOOL bContinue;
    do {
	ulCount++;
	sprintf(buffer, "config.FileFormatPriority.Priority_%d", ulCount);
	pBuffer = NULL;
	
	IHXRegistry* pRegistry = NULL;
	if (HXR_OK != m_pparent->m_pContext->QueryInterface(IID_IHXRegistry,
							    (void **)&pRegistry))
	{
	    return 0;
	}
	    
	pRegistry->GetStrByName((const char *)buffer, pBuffer);
	pRegistry->Release();
	if (pBuffer && (0 == strcasecmp(pPlugin->m_pPlugin->m_pszDescription,
	    (const char *)pBuffer->GetBuffer())))
	{
	    pBuffer->Release();
	    return ulCount;
	}
	bContinue = pBuffer ? TRUE : FALSE;
	HX_RELEASE(pBuffer);
    } while (bContinue);

    return 0;
}

PluginHandler::Errors
PluginHandler::FileFormat::Add(Plugin* pPlugin, CHXSimpleList* pMimeTypesList
			       , CHXSimpleList* pExtensionList, CHXSimpleList* pOpenNames)
{
    Errors			result = NO_ERRORS;
    CHXSimpleList::Iterator	i;
    PluginInfo*			pPluginInfo = NULL;
    const char*			pszExtension = NULL;
    const char* 		pszMimeType = NULL;

    if (!(pPluginInfo = new PluginInfo()))
    {
	result = MEMORY_ERROR;
	goto cleanup;
    }

    if (pMimeTypesList && pExtensionList && pOpenNames)
    {
	// we have all the info we need let's not load. 
	// BLEECH! it would be much nicer if m_mimeTypes
	// was a pointer. 
	for(i= pMimeTypesList->Begin(); 
		i!=pMimeTypesList->End();++i)
	{
	    pPluginInfo->m_mimeTypes.AddTail(*i);
	}
	for(i= pExtensionList->Begin(); 
		i!=pExtensionList->End();++i)
	{
	    pPluginInfo->m_extensions.AddTail(*i);
	}
	for(i= pOpenNames->Begin(); 
		i!=pOpenNames->End();++i)
	{
	    pPluginInfo->m_OpenNames.AddTail(*i);
	}

	pPluginInfo->m_pPlugin = pPlugin;
	pPluginInfo->m_pPlugin->AddRef();
    }
    else
    {
	    result = pPluginInfo->Init(pPlugin);
    }

    if (NO_ERRORS != result)
    {
	goto cleanup;
    }

    m_pPlugins->AddTail(pPluginInfo);

    for (i = pPluginInfo->m_mimeTypes.Begin(); 
	 i != pPluginInfo->m_mimeTypes.End(); ++i)
    {
	     // tolower
	pszMimeType = (const char*) (*i);
	(*m_pMimeMap)[pszMimeType] = pPluginInfo;
    }

    for (i = pPluginInfo->m_extensions.Begin(); 
	 i != pPluginInfo->m_extensions.End(); ++i)
    {
	pszExtension = (const char*) (*i);

	// convert to lower cases
	CHXString pnExtString(pszExtension);
	pnExtString.MakeLower();

	PluginInfo* pAlreadyPluginInfo = NULL;

	/* We first look for plugins supporting given mime type*/
	if (m_pExtensionMap->Lookup((const char*)pnExtString, (void*&)pAlreadyPluginInfo) != FALSE)
	{
	    UINT32 ulMyPriority = GetPriority(pPluginInfo);
	    if (ulMyPriority)
	    {
		PluginInfo* pPrev = NULL;
		PluginInfo* pSearch = pAlreadyPluginInfo;
		UINT32 ulSearchPriority = GetPriority(pSearch);
		if ((ulSearchPriority == 0) ||
		    (ulMyPriority < ulSearchPriority))
		{
		    if (pPrev == NULL)
		    {
			pPluginInfo->m_pNextPlugin = pAlreadyPluginInfo;
			(*m_pExtensionMap)[(const char*)pnExtString] = pPluginInfo;
		    }
		    else
		    {
			pPrev->m_pNextPlugin = pPluginInfo;
			pPluginInfo->m_pNextPlugin = pSearch;
		    }
		}
		pPrev = pSearch;
		pSearch = pSearch->m_pNextPlugin;
	    }
	    else if ((GetPriority(pAlreadyPluginInfo) != 0) ||
		(strstr(pPlugin->m_pszDescription, "RealNetworks") == NULL))
	    {
		pPluginInfo->m_pNextPlugin = pAlreadyPluginInfo->m_pNextPlugin;
		pAlreadyPluginInfo->m_pNextPlugin = pPluginInfo;
	    }
	    else
	    {
		pPluginInfo->m_pNextPlugin = pAlreadyPluginInfo;
		(*m_pExtensionMap)[(const char*)pnExtString] = pPluginInfo;
	    }
	}
	else
	{
	    (*m_pExtensionMap)[(const char*)pnExtString] = pPluginInfo;
	}
    }

cleanup:

    if (NO_ERRORS != result)
    {
	if (pPluginInfo)
	{
	    delete pPluginInfo;
	    pPluginInfo = NULL;
	}
    }
    return result;
}

UINT32 PluginHandler::FileFormat::GetNumPlugins()
{
    return m_pPlugins->GetCount();
}
    
void
PluginHandler::FileFormat::Clear()
{
    CHXSimpleList::Iterator i;
    PluginInfo*		    pPluginInfo = NULL;

    for (i = m_pPlugins->Begin(); i != m_pPlugins->End(); ++i)
    {
	pPluginInfo = (PluginInfo*) (*i);
	delete pPluginInfo;
    }
    
    m_pPlugins->RemoveAll();
    m_pMimeMap->RemoveAll();
    m_pExtensionMap->RemoveAll();
}

UINT32
PluginHandler::FileFormat::GetNumOfPlugins()
{
    return (UINT32) m_pPlugins->GetCount();
}

void
PluginHandler::FileFormat::GetPluginInfo(UINT32		    unIndex,
					 char**		    ppszDllPath,
					 char**		    ppszDescription,
					 char**		    ppszCopyright,
					 char**		    ppszMoreInfo,
					 BOOL*		    pbMultiple,
					 CHXSimpleList**    ppszMimeTypes,
					 CHXSimpleList**    ppszExtensions, 
					 CHXSimpleList**    ppszOpenNames)
{
    PluginInfo*	pPluginInfo = NULL;

    // initialize
    *ppszDllPath = NULL;
    *ppszDescription = NULL;
    *ppszCopyright = NULL;
    *ppszMoreInfo = NULL;
    *pbMultiple = FALSE;
    *ppszMimeTypes = NULL;
    *ppszExtensions = NULL;

    pPluginInfo = (PluginInfo*) GetDataFromList(m_pPlugins, unIndex);

    if (!pPluginInfo)
    {
	return;
    }

    pPluginInfo->m_pPlugin->GetPluginInfo(ppszDllPath, 
					  ppszDescription, 
					  ppszCopyright, 
					  ppszMoreInfo,
					  pbMultiple);

    *ppszMimeTypes = &(pPluginInfo->m_mimeTypes);
    *ppszExtensions = &(pPluginInfo->m_extensions);
    *ppszOpenNames =  &(pPluginInfo->m_OpenNames);
    return;
}

PluginHandler::Errors
PluginHandler::FileFormat::FindUsingValues(IHXValues* pValues,
                                           Plugin*& pPlugin)
{
    Errors rc = PLUGIN_NOT_FOUND;

    IHXBuffer* pMimeType = NULL;
    IHXBuffer* pExtension = NULL;
    const char* szMimeType = NULL;
    const char* szExtension = NULL;

    if (SUCCEEDED(pValues->GetPropertyCString(PLUGIN_FILEMIMETYPES, 
        pMimeType)))
    {
        szMimeType = (const char*)pMimeType->GetBuffer();
    }
    if (SUCCEEDED(pValues->GetPropertyCString(PLUGIN_FILEEXTENSIONS,
        pExtension)))
    {
        szExtension = (const char*)pExtension->GetBuffer();
    }

    rc = Find(szMimeType, szExtension, pPlugin);

    HX_RELEASE(pMimeType);
    HX_RELEASE(pExtension);

    return rc;
}

///////////////////////////////////////////////////////////////////////////////
//
// PluginHandler::FileFormat::PluginInfo
//
///////////////////////////////////////////////////////////////////////////////
PluginHandler::FileFormat::PluginInfo::PluginInfo()
{
    m_pPlugin = NULL;
    m_pNextPlugin = NULL;
}

PluginHandler::FileFormat::PluginInfo::~PluginInfo()
{
	CHXSimpleList::Iterator i;

    for(i = m_mimeTypes.Begin(); i!=m_mimeTypes.End(); ++i)
    {
	delete[] (char*)(*i);
    }
    for(i = m_extensions.Begin(); i!=m_extensions.End(); ++i)
    {
	delete[] (char*)(*i);
    }
    for(i = m_OpenNames.Begin(); i!=m_OpenNames.End(); ++i)
    {
	delete[] (char*)(*i);
    }
    
    if (m_pPlugin)
    {
	m_pPlugin->Release();
	m_pPlugin = NULL;
    }
}

PluginHandler::Errors
PluginHandler::FileFormat::PluginInfo::Init(Plugin* pPlugin)
{
    Errors			result = NO_ERRORS;
    const char**		ppszMimeTypes = NULL;
    const char**		ppszExtensions = NULL;
    const char**		ppszOpenNames = NULL;
    IUnknown*			pUnknown = NULL;
    IHXFileFormatObject*	pFileFormat = NULL;
    IHXMetaFileFormatObject*	pMetaFileFormat = NULL;

    if (!pPlugin)
    {
	result = BAD_PLUGIN;
	goto cleanup;
    }

    // type checking
    result = pPlugin->GetInstance(&pUnknown);

    if (NO_ERRORS != result)
    {
	goto cleanup;
    }

    if (HXR_OK == pUnknown->QueryInterface(IID_IHXMetaFileFormatObject, (void**) &pMetaFileFormat))
    {
	if (HXR_OK != pMetaFileFormat->GetMetaFileFormatInfo(ppszMimeTypes, 
							   ppszExtensions, 
							   ppszOpenNames))
	{
	    result = CANT_GET_FILE_FORMAT_INFO;
	    goto cleanup;
	}	
    }
    else if (HXR_OK == pUnknown->QueryInterface(IID_IHXFileFormatObject, (void**) &pFileFormat))
    {
	if (HXR_OK != pFileFormat->GetFileFormatInfo(ppszMimeTypes, 
		    	    			   ppszExtensions, 
						   ppszOpenNames))
	{
	    result = CANT_GET_FILE_FORMAT_INFO;
	    goto cleanup;
	}
    }
    else
    {
	result = BAD_PLUGIN;
	goto cleanup;
    }

    PluginHandler::ParseInfoArray(&m_mimeTypes, ppszMimeTypes);
    PluginHandler::ParseInfoArray(&m_extensions, ppszExtensions);
    PluginHandler::ParseInfoArray(&m_OpenNames, ppszOpenNames);


    m_pPlugin = pPlugin;
    m_pPlugin->AddRef();

cleanup:
    if (pFileFormat)
    {
	pFileFormat->Release();
	pFileFormat = NULL;
    }

    if (pMetaFileFormat)
    {
	pMetaFileFormat->Release();
	pMetaFileFormat = NULL;
    }

    if (pUnknown)
    {
	pUnknown->Release();
	pUnknown = NULL;
    }
   

    return result;
}


///////////////////////////////////////////////////////////////////////////////
//
// PluginHandler::BroadcastFormat
//
///////////////////////////////////////////////////////////////////////////////
PluginHandler::BroadcastFormat::BroadcastFormat(PluginHandler* pparent)
{
    m_pparent = pparent;
    m_pPlugins = new CHXSimpleList();
    m_pBroadcastMap = new CHXMapStringToOb();
}

PluginHandler::BroadcastFormat::~BroadcastFormat()
{
    Clear();

    if (m_pPlugins)
    {
	delete m_pPlugins;
	m_pPlugins = NULL;
    }

    if (m_pBroadcastMap)
    {
	delete m_pBroadcastMap;
	m_pBroadcastMap = NULL;
    }
}

PluginHandler::Errors
PluginHandler::BroadcastFormat::StoreToRegistry(IHXPreferences** ppRegistry)
{
    Errors		    result = NO_ERRORS;
    int			    j = 0;
    char*		    pszData = NULL;
    char*		    pszInfo = NULL;
    char*		    pszTemp = NULL;
    PluginInfo*		    pPluginInfo = NULL;
    IHXBuffer*		    pBuffer = NULL;
    CHXSimpleList::Iterator i;
    char		    CharBuffer[_MAX_PATH];

    if (!ppRegistry)
    {
	result = BAD_REGISTRY_HANDLE;
	goto cleanup;
    }

    if (!*ppRegistry)
    {
	result = BAD_REGISTRY_HANDLE;
	goto cleanup;
    }

    // parse/collect all the plugin info.
    for (i = m_pPlugins->Begin(); i != m_pPlugins->End(); ++i)
    {
	// {
	if (!pszInfo)
	{
	    pszInfo = new char[2];
	    strcpy(pszInfo, "{");
	}
	else
	{
	    pszTemp = new char[strlen(pszInfo)+2];
	    sprintf(pszTemp, "%s%s", pszInfo, "{");
	    
	    delete[] pszInfo;
	    pszInfo = pszTemp;
	}

	pPluginInfo = (PluginInfo*) (*i);

	for (j = 0; j < BROADCAST_PLUGIN_RECORD_ENTRIES; j++)
	{
	    switch (j)
	    {
	    case 0:
		pszData = (char*) pPluginInfo->m_pPlugin->m_pszDllName;
		break;
	    case 1: 
		sprintf(CharBuffer ,"%d",pPluginInfo->m_pPlugin->m_nPluginIndex);
		pszData  = CharBuffer;
		break;
	    case 2:	    // {<description>
		pszData = (char*) pPluginInfo->m_pPlugin->m_pszDescription;
		break;
	    case 3:	    // {<description>,<copyright>
		pszData = (char*) pPluginInfo->m_pPlugin->m_pszCopyright;
		break;
	    case 4:	    // {<description>,<copyright>,<moreinfo>
		pszData = (char*) pPluginInfo->m_pPlugin->m_pszMoreInfoUrl;
		break;
	    case 5:	    // {<description>,<copyright>,<moreinfo>,<loadmultiple>
		if (pPluginInfo->m_pPlugin->m_load_multiple)
		{
		    pszData = "TRUE";
		}
		else
		{
		    pszData = "FALSE";
		}
		break;
	    case 6:
		pszData = (char*) pPluginInfo->m_pszType;
		break;
	    default:
		break;
	    }

	    AppendStringToRecord((char *)pszData, &pszInfo);

	    if (j != BROADCAST_PLUGIN_RECORD_ENTRIES - 1)
	    {
		// {xxx|xxx,
		pszTemp = new char[strlen(pszInfo)+2];
		sprintf(pszTemp, "%s%c", pszInfo, PLUGIN_ENTRY_SEPARATOR);

		delete[] pszInfo;
		pszInfo = pszTemp;
	    }
	}

	// {xxx,xxx}
	pszTemp = new char[strlen(pszInfo)+2];
	sprintf(pszTemp, "%s}", pszInfo);
	
	delete[] pszInfo;
	pszInfo = pszTemp;
    }	    
    
    if (pszInfo)
    {
	if (!(pBuffer = new CHXBuffer()))
	{
	    result = MEMORY_ERROR;
	    goto cleanup;
	}

	pBuffer->AddRef();
    
	pBuffer->Set((UCHAR*)pszInfo, strlen(pszInfo)+1);

	(*ppRegistry)->WritePref(REGKEY_BROADCAST_PLUGIN, pBuffer);
    
	pBuffer->Release();
    }

cleanup:
    
    if (pszInfo)
    {
	delete[] pszInfo;
	pszInfo = NULL;
    }

    return result;
}

PluginHandler::Errors
PluginHandler::BroadcastFormat::ReadFromRegistry(IHXPreferences* pRegistry)
{
    Errors	result		= NO_ERRORS;
    IHXBuffer*	pBuffer		= NULL;
    char*	pszInfo		= NULL;
    char*	pszDllName	= NULL;
    char*	pszLoadMultiple = NULL;
    Plugin*	pPlugin		= NULL;
    PluginDLL*	pPluginDLL	= NULL;
    char*       pszindex	= NULL;
    char*	pszType		= NULL;
    char*	pszPluginIndex	= NULL;

    if (!pRegistry)
    {
	result = BAD_REGISTRY_HANDLE;
	goto cleanup;
    }

    // clear the existing ones
    Clear();

    // load from the registry
    if (HXR_OK != pRegistry->ReadPref(REGKEY_BROADCAST_PLUGIN, pBuffer))
    {
	result = PLUGIN_NOT_FOUND;
	goto cleanup;
    }

    pszInfo = (char*) pBuffer->GetBuffer();

    // create plugins based on the info
    while (strlen(pszInfo))
    {
	// get dll name
	pszInfo++;
	RetrieveStringFromRecord(&pszInfo, &pszDllName);
		       
	// get the association...
	pPluginDLL=m_pparent->m_PluginFactory->FindDLLFromName(pszDllName);
	RetrieveStringFromRecord(&pszInfo, &pszindex);
	// create plugin
	pPlugin = new Plugin(pszDllName, FALSE, 0, pPluginDLL, atoi(pszindex));
	pPlugin->AddRef();
	delete[] pszindex;

	// get the plugin index
	pszInfo++;
	RetrieveStringFromRecord(&pszInfo, &pszPluginIndex);
	pPlugin->m_nPluginIndex = atoi(pszPluginIndex);
	delete[] pszPluginIndex;

	// get description
	pszInfo++;
	RetrieveStringFromRecord(&pszInfo, (char**)&(pPlugin->m_pszDescription));

	// get copyright
	pszInfo++;
	RetrieveStringFromRecord(&pszInfo, (char**)&(pPlugin->m_pszCopyright));

	// get moreinfo
	pszInfo++;
	RetrieveStringFromRecord(&pszInfo, (char**)&(pPlugin->m_pszMoreInfoUrl));

	// get loadmultiple
	pszInfo++;
	RetrieveStringFromRecord(&pszInfo, &(pszLoadMultiple));

	if (strcmp(pszLoadMultiple, "TRUE") == 0)
	{
	    pPlugin->m_load_multiple = TRUE;
	}
	else
	{
	    pPlugin->m_load_multiple = FALSE;
	}
    
	// get type.
	pszInfo++;
	RetrieveStringFromRecord(&pszInfo, &pszType);

	// pass through the end of this entry
	while (*pszInfo != '}')
	{
	    pszInfo++;
	}

	// add this plugin
	Add(pPlugin, pszType);

	// freeup
	delete[] pszDllName;
	delete[] pszLoadMultiple;
	delete[] pszType;

	pszDllName = NULL;
	pszLoadMultiple = NULL;

	pPlugin->Release();

	pszInfo++;
    }	

cleanup:

    if (pBuffer)
    {
	pBuffer->Release();
	pBuffer = NULL;
    }

    return result;
}

PluginHandler::Errors
PluginHandler::BroadcastFormat::Find(const char* pszBroadcastType,
				PluginHandler::Plugin*& plugin)
{
    PluginInfo*	pPluginInfo = NULL;

    if (!(pPluginInfo = FindPluginInfo(pszBroadcastType)))
    {
	return PLUGIN_NOT_FOUND;
    }

    plugin = pPluginInfo->m_pPlugin;

    return NO_ERRORS;
}

PluginHandler::BroadcastFormat::PluginInfo*
PluginHandler::BroadcastFormat::FindPluginInfo(const char* pszBroadcastType)
{
    PluginInfo*			pPluginInfo = NULL;
    CHXSimpleList::Iterator	i;

    /* We first look for plugins supporitng given type*/
    if (pszBroadcastType)
    {
	if (m_pBroadcastMap->Lookup(pszBroadcastType, (void*&)pPluginInfo) != FALSE)
	{
	    return pPluginInfo;
	}
    }

    return NULL;
}

PluginHandler::Errors
PluginHandler::BroadcastFormat::Add(Plugin* pPlugin, char* pszType)
{
    Errors			result = NO_ERRORS;
    CHXSimpleList::Iterator	i;
    PluginInfo*			pPluginInfo = NULL;
    const char*			pszMimeType = NULL;
    const char* 		pszExtension = NULL;

    if (!(pPluginInfo = new PluginInfo()))
    {
	result = MEMORY_ERROR;
	goto cleanup;
    }

    if (pszType)
    {
	pPluginInfo->m_pszType = pszType;
	pPluginInfo->m_pPlugin = pPlugin;
	pPluginInfo->m_pPlugin->AddRef();
    }
    else
    {
	result = pPluginInfo->Init(pPlugin);
    }
    if (NO_ERRORS != result)
    {
	goto cleanup;
    }

    m_pPlugins->AddTail(pPluginInfo);

    (*m_pBroadcastMap)[pPluginInfo->m_pszType] = pPluginInfo;

cleanup:

    return result;
}

void
PluginHandler::BroadcastFormat::Clear()
{
    CHXSimpleList::Iterator	i;
    PluginInfo*			pPluginInfo = NULL;

    for (i = m_pPlugins->Begin(); i != m_pPlugins->End(); ++i)
    {
	pPluginInfo = (PluginInfo*) (*i);
	delete pPluginInfo;
    }
    
    m_pPlugins->RemoveAll();
    m_pBroadcastMap->RemoveAll();
}

UINT32
PluginHandler::BroadcastFormat::GetNumOfPlugins()
{
    return (UINT32) m_pPlugins->GetCount();
}

void
PluginHandler::BroadcastFormat::GetPluginInfo(UINT32	unIndex,
					      char**	ppszDllPath,
					      char**    ppszDescription,
					      char**    ppszCopyright,
					      char**    ppszMoreInfo,
					      BOOL*     pbMultiple,
					      char**    ppszType)
{
    PluginInfo*	pPluginInfo = NULL;

    // initialize
    *ppszDllPath = NULL;
    *ppszDescription = NULL;
    *ppszCopyright = NULL;
    *ppszMoreInfo = NULL;
    *pbMultiple = FALSE;
    *ppszType = NULL;
   
    pPluginInfo = (PluginInfo*) GetDataFromList(m_pPlugins, unIndex);

    if (!pPluginInfo)
    {
	return;
    }

    pPluginInfo->m_pPlugin->GetPluginInfo(ppszDllPath, 
					  ppszDescription, 
					  ppszCopyright, 
					  ppszMoreInfo,
					  pbMultiple);

    *ppszType = (char*) pPluginInfo->m_pszType;
   
    return;
}

///////////////////////////////////////////////////////////////////////////////
//
// PluginHandler::BroadcastFormat::PluginInfo
//
///////////////////////////////////////////////////////////////////////////////
PluginHandler::BroadcastFormat::PluginInfo::PluginInfo()
{
    m_pPlugin	= NULL;
    m_pszType	= NULL;
}

PluginHandler::BroadcastFormat::PluginInfo::~PluginInfo()
{
    if (m_pPlugin)
    {
	m_pPlugin->Release();
	m_pPlugin = NULL;
    }

    if (m_pszType)
    {
	delete [] m_pszType;
	m_pszType = 0;
    }
}

PluginHandler::Errors
PluginHandler::BroadcastFormat::PluginInfo::Init(Plugin* pPlugin)
{
    const char*                 pszBroadcastType = NULL;
    Errors			result = NO_ERRORS;
    IUnknown*			pUnknown = NULL;
    IHXBroadcastFormatObject*	pBroadcastFormat = NULL;

    // type checking
    if (!pPlugin)
    {
	result = BAD_PLUGIN;
	goto cleanup;
    }

    result = pPlugin->GetInstance(&pUnknown);
    if (NO_ERRORS != result)
    {
	goto cleanup;
    }

    if (HXR_OK != pUnknown->QueryInterface(IID_IHXBroadcastFormatObject, 
					(void**) &pBroadcastFormat))
    {
	result = BAD_PLUGIN;
	goto cleanup;
    }

    if (HXR_OK != pBroadcastFormat->GetBroadcastFormatInfo(pszBroadcastType))
    {
	result = CANT_GET_FILE_FORMAT_INFO;
	goto cleanup;
    }

    m_pszType = new_string(pszBroadcastType);

    m_pPlugin = pPlugin;
    m_pPlugin->AddRef();
    
cleanup:

    if (pBroadcastFormat)
    {
	pBroadcastFormat->Release();
	pBroadcastFormat = NULL;
    }

    if (pUnknown)
    {
	pUnknown->Release();
	pUnknown = NULL;
    }
   

    return result;
}


///////////////////////////////////////////////////////////////////////////////
//
// PluginHandler::Renderer
//
///////////////////////////////////////////////////////////////////////////////
PluginHandler::Renderer::Renderer(PluginHandler* pparent)
{
    m_pPlugins = new CHXSimpleList();
    m_pMimeMap = new CHXMapStringToOb();
    m_pparent=pparent;
}

PluginHandler::Renderer::~Renderer()
{
    Clear();

    if (m_pPlugins)
    {
	delete m_pPlugins;
	m_pPlugins = NULL;
    }

    if (m_pMimeMap)
    {
	delete m_pMimeMap;
	m_pMimeMap = NULL;
    }
}

void
PluginHandler::Renderer::Clear()
{
    CHXSimpleList::Iterator	i;
    PluginInfo*			pPluginInfo = NULL;
  
    for (i = m_pPlugins->Begin(); i != m_pPlugins->End(); ++i)
    {
	pPluginInfo = (PluginInfo*) (*i);
	delete pPluginInfo;
    }
    
    m_pPlugins->RemoveAll();
    m_pMimeMap->RemoveAll();
}

PluginHandler::Errors
PluginHandler::Renderer::StoreToRegistry(IHXPreferences** ppRegistry)
{
    Errors		    result = NO_ERRORS;
    int			    j = 0;
    char*		    pszData = NULL;
    char*		    pszInfo = NULL;
    char*		    pszTemp = NULL;
    CHXSimpleList*	    pList = NULL;
    PluginInfo*		    pPluginInfo = NULL;
    IHXBuffer*		    pBuffer = NULL;
    CHXSimpleList::Iterator i;
    char		    CharBuffer[_MAX_PATH];

    if (!ppRegistry)
    {
	result = BAD_REGISTRY_HANDLE;
	goto cleanup;
    }

    if (!*ppRegistry)
    {
	result = BAD_REGISTRY_HANDLE;
	goto cleanup;
    }

    // parse/collect all the plugin info.
    for (i = m_pPlugins->Begin(); i != m_pPlugins->End(); ++i)
    {
	// {
	if (!pszInfo)
	{
	    pszInfo = new char[2];
	    strcpy(pszInfo, "{");
	}
	else
	{
	    pszTemp = new char[strlen(pszInfo)+2];
	    sprintf(pszTemp, "%s%s", pszInfo, "{");
	    
	    delete[] pszInfo;
	    pszInfo = pszTemp;
	}

	pPluginInfo = (PluginInfo*) (*i);

	for (j = 0; j < RENDERER_PLUGIN_RECORD_ENTRIES; j++)
	{
	    switch (j)
	    {
	    case 0:
		pszData = pPluginInfo->m_pPlugin->m_pszDllName;
		pList = NULL;
		break;
	    case 1: 
		sprintf(CharBuffer,"%d",pPluginInfo->m_pPlugin->m_nPluginIndex);
		pszData = CharBuffer;
		pList = NULL;
		break;
	    case 2:	    // {<description>
		pszData = (char*) pPluginInfo->m_pPlugin->m_pszDescription;
		pList = NULL;
		break;
	    case 3:	    // {<description>,<copyright>
		pszData = (char*) pPluginInfo->m_pPlugin->m_pszCopyright;
		pList = NULL;
		break;
	    case 4:	    // {<description>,<copyright>,<moreinfo>
		pszData = (char*) pPluginInfo->m_pPlugin->m_pszMoreInfoUrl;
		pList = NULL;
		break;
	    case 5:	    // {<description>,<copyright>,<moreinfo>,<loadmultiple>
		if (pPluginInfo->m_pPlugin->m_load_multiple)
		{
		    pszData = "TRUE";
		}
		else
		{
		    pszData = "FALSE";
		}
		pList = NULL;
		break;
	    case 6:
		pszData = NULL;
		pList = &(pPluginInfo->m_mimeTypes);
		break;
	    default:
		break;
	    }

	    if (j == 6)
	    {		
		AppendListToRecord(pList, &pszInfo);
	    }
	    else
	    {
		AppendStringToRecord(pszData, &pszInfo);
	    }

	    if (j != RENDERER_PLUGIN_RECORD_ENTRIES - 1)
	    {
		// {xxx|xxx,
		pszTemp = new char[strlen(pszInfo)+2];
		sprintf(pszTemp, "%s%c", pszInfo, PLUGIN_ENTRY_SEPARATOR);

		delete[] pszInfo;
		pszInfo = pszTemp;
	    }
	}

	// {xxx!xxx,xxx}
	pszTemp = new char[strlen(pszInfo)+2];
	sprintf(pszTemp, "%s}", pszInfo);
	
	delete[] pszInfo;
	pszInfo = pszTemp;
    }	    
    
    if (pszInfo)
    {
	if (!(pBuffer = new CHXBuffer()))
	{
	    result = MEMORY_ERROR;
	    goto cleanup;
	}

	pBuffer->AddRef();
    
	pBuffer->Set((UCHAR*)pszInfo, strlen(pszInfo)+1);

	(*ppRegistry)->WritePref(REGKEY_RENDERER_PLUGIN, pBuffer);
    
	pBuffer->Release();
    }

cleanup:
    if (pszInfo)
    {
	delete[] pszInfo;
	pszInfo = NULL;
    }

    return result;
}

PluginHandler::Errors
PluginHandler::Renderer::ReadFromRegistry(IHXPreferences* pRegistry)
{
    Errors	result = NO_ERRORS;
    IHXBuffer*	pBuffer = NULL;
    char*	pszInfo = NULL;
    char*	pszDllName = NULL;
    char*	pszLoadMultiple = NULL;
    Plugin*	pPlugin	= NULL;
    PluginDLL*	pPluginDLL = NULL;
    char*       pszindex = NULL;
    char*	pszPluginIndex	= NULL;
    
    if (!pRegistry)
    {
	result = BAD_REGISTRY_HANDLE;
	goto cleanup;
    }

    // clear the existing ones
    Clear();

    // load from the registry
    if (HXR_OK != pRegistry->ReadPref(REGKEY_RENDERER_PLUGIN, pBuffer))
    {
	result = PLUGIN_NOT_FOUND;
	goto cleanup;
    }

    pszInfo = (char*) pBuffer->GetBuffer();

    // create plugins based on the info
    while (strlen(pszInfo))
    {
	// get dll name
	pszInfo++;
	RetrieveStringFromRecord(&pszInfo, &pszDllName);
		       
	// get the association...
	pPluginDLL=m_pparent->m_PluginFactory->FindDLLFromName(pszDllName);
	RetrieveStringFromRecord(&pszInfo, &pszindex);
	// create plugin
	pPlugin = new Plugin(pszDllName, FALSE, 0, pPluginDLL, atoi(pszindex));
	pPlugin->AddRef();
	delete[] pszindex;

	// get the plugin index
	pszInfo++;
	RetrieveStringFromRecord(&pszInfo, &pszPluginIndex);
	pPlugin->m_nPluginIndex = atoi(pszPluginIndex);
	delete[] pszPluginIndex;

	// get description
	pszInfo++;
	RetrieveStringFromRecord(&pszInfo, (char**)&(pPlugin->m_pszDescription));

	// get copyright
	pszInfo++;
	RetrieveStringFromRecord(&pszInfo, (char**)&(pPlugin->m_pszCopyright));

	// get moreinfo
	pszInfo++;
	RetrieveStringFromRecord(&pszInfo, (char**)&(pPlugin->m_pszMoreInfoUrl));

	// get loadmultiple
	pszInfo++;
	RetrieveStringFromRecord(&pszInfo, &(pszLoadMultiple));

	if (strcmp(pszLoadMultiple, "TRUE") == 0)
	{
	    pPlugin->m_load_multiple = TRUE;
	}
	else
	{
	    pPlugin->m_load_multiple = FALSE;
	}

	// Retreive the mime types.
    	CHXSimpleList mimeTypesList;

	pszInfo++;
	RetrieveListFromRecord(&pszInfo, &mimeTypesList);

	// pass through the end of this entry
	while (*pszInfo != '}')
	{
	    pszInfo++;
	}

	// add this plugin
	Add(pPlugin, &mimeTypesList);

	// freeup
	delete[] pszDllName;
	delete[] pszLoadMultiple;

	pszDllName = NULL;
	pszLoadMultiple = NULL;

	pPlugin->Release();

	pszInfo++;
    }	

cleanup:

    if (pBuffer)
    {
	pBuffer->Release();
	pBuffer = NULL;
    }

    return result;
}

PluginHandler::Errors
PluginHandler::Renderer::Find(const char* pszMimeType, 
			      PluginHandler::Plugin*& plugin)
{
    PluginInfo*	pPluginInfo = NULL;

    if (!(pPluginInfo = FindPluginInfo(pszMimeType)))
    {
	return PLUGIN_NOT_FOUND;
    }

    plugin = pPluginInfo->m_pPlugin;

    return NO_ERRORS;
}

PluginHandler::Renderer::PluginInfo*
PluginHandler::Renderer::FindPluginInfo(const char* pszMimeType)
{
    PluginInfo*	pPluginInfo = NULL;

    /* We first look for plugins supporitng given mime type*/
    if (pszMimeType)
    {
	if (m_pMimeMap->Lookup(pszMimeType, (void*&)pPluginInfo) != FALSE)
	{
	    return pPluginInfo;
	}
    }

    return NULL;
}

PluginHandler::Errors
PluginHandler::Renderer::Add(Plugin* pPlugin, CHXSimpleList* pMimeTypesList)
{
    Errors			result = NO_ERRORS;
    CHXSimpleList::Iterator	i;
    PluginInfo*			pPluginInfo = NULL;
    const char* 		pszMimeType = NULL;

    if (!(pPluginInfo = new PluginInfo()))
    {
	result = MEMORY_ERROR;
	goto cleanup;
    }

    if (pMimeTypesList)
    {
	for(CHXSimpleList::Iterator i= pMimeTypesList->Begin(); 
		i!=pMimeTypesList->End();++i)
	{
	    pPluginInfo->m_mimeTypes.AddTail(*i);
	}
	pPluginInfo->m_pPlugin = pPlugin;
	pPluginInfo->m_pPlugin->AddRef();
    }
    else
    {
	result = pPluginInfo->Init(pPlugin);
    }

    if (NO_ERRORS != result)
    {
	goto cleanup;
    }

    m_pPlugins->AddTail(pPluginInfo);

    for (i = pPluginInfo->m_mimeTypes.Begin(); 
	 i != pPluginInfo->m_mimeTypes.End(); ++i)
    {
	pszMimeType = (const char*) (*i);
	(*m_pMimeMap)[pszMimeType] = pPluginInfo;
    }

cleanup:

    if (NO_ERRORS != result)
    {
	if (pPluginInfo)
	{
	    delete pPluginInfo;
	    pPluginInfo = NULL;
	}
    }
    
    return result;   
}

UINT32
PluginHandler::Renderer::GetNumOfPlugins()
{
    return (UINT32) m_pPlugins->GetCount();
}

void
PluginHandler::Renderer::GetPluginInfo(UINT32		    unIndex,
				       char**		    ppszDllPath,
				       char**		    ppszDescription,
				       char**		    ppszCopyright,
				       char**		    ppszMoreInfo,
				       BOOL*		    pbMultiple,
				       CHXSimpleList**	    ppszMimeTypes)
{
    PluginInfo*	pPluginInfo = NULL;

    // initialize
    *ppszDllPath = NULL;
    *ppszDescription = NULL;
    *ppszCopyright = NULL;
    *ppszMoreInfo = NULL;
    *pbMultiple = FALSE;
    *ppszMimeTypes = NULL;
    
    pPluginInfo = (PluginInfo*) GetDataFromList(m_pPlugins, unIndex);

    if (!pPluginInfo)
    {
	return;
    }

    pPluginInfo->m_pPlugin->GetPluginInfo(ppszDllPath, 
					  ppszDescription, 
					  ppszCopyright, 
					  ppszMoreInfo,
					  pbMultiple);

    *ppszMimeTypes = &(pPluginInfo->m_mimeTypes);
  
    return;
}

///////////////////////////////////////////////////////////////////////////////
//
// PluginHandler::Renderer::PluginInfo
//
///////////////////////////////////////////////////////////////////////////////
PluginHandler::Renderer::PluginInfo::PluginInfo()
{
    m_pPlugin = NULL;
}

PluginHandler::Renderer::PluginInfo::~PluginInfo()
{
    for(CHXSimpleList::Iterator i= m_mimeTypes.Begin(); i!=m_mimeTypes.End();++i)
    {
	delete[] (char*)(*i);
    }
    if (m_pPlugin)
    {
	m_pPlugin->Release();
	m_pPlugin = NULL;
    }
}

PluginHandler::Errors
PluginHandler::Renderer::PluginInfo::Init(Plugin* pPlugin)
{
    Errors			result = NO_ERRORS;
    UINT32			initial_granularity = 0;
    const char**		ppszMimeTypes = NULL;
    IUnknown*			pUnknown = NULL;
    IHXRenderer*		pRenderer = NULL;

    if (!pPlugin)
    {
	result = BAD_PLUGIN;
	goto cleanup;
    }

    // type checking
    result = pPlugin->GetInstance(&pUnknown);

    if (NO_ERRORS != result)
    {
	goto cleanup;
    }

    if (HXR_OK != pUnknown->QueryInterface(IID_IHXRenderer, 
					(void**) &pRenderer))
    {
	result = BAD_PLUGIN;
	goto cleanup;
    }

    if (HXR_OK != pRenderer->GetRendererInfo(ppszMimeTypes, 
					   initial_granularity))
    {
	result = CANT_GET_RENDERER_INFO;
	goto cleanup;
    }

    PluginHandler::ParseInfoArray(&m_mimeTypes, ppszMimeTypes);
   
    m_pPlugin = pPlugin;
    m_pPlugin->AddRef();

cleanup:
    if (pRenderer)
    {
	pRenderer->Release();
	pRenderer = NULL;
    }

    if (pUnknown)
    {
	pUnknown->Release();
	pUnknown = NULL;
    }
   

    return result;
}


///////////////////////////////////////////////////////////////////////////////
//
// PluginHandler::DataConvert
//
///////////////////////////////////////////////////////////////////////////////
PluginHandler::DataConvert::DataConvert(PluginHandler* pparent)
{
    m_pShortNameMap = new CHXMapStringToOb();
    m_pPlugins = new CHXSimpleList;
    m_pparent = pparent;
    m_pMountPointHandler = new MountPointHandler;
}

PluginHandler::DataConvert::~DataConvert()
{
    delete m_pShortNameMap;
    m_pShortNameMap = NULL;
    delete m_pPlugins;
    m_pPlugins = NULL;
    delete m_pMountPointHandler;
    m_pMountPointHandler = NULL;
}

PluginHandler::Errors
PluginHandler::DataConvert::StoreToRegistry(IHXPreferences** ppRegistry)
{
    return NO_ERRORS;
}

PluginHandler::Errors
PluginHandler::DataConvert::ReadFromRegistry(IHXPreferences* pRegistry)
{
    return NO_ERRORS;
}


PluginHandler::Errors
PluginHandler::DataConvert::Add(Plugin* pPlugin, char* pszShortName)
{
    PluginInfo* pPluginInfo = NULL;
    
    pPluginInfo = new PluginInfo;
    if (pszShortName)
    {
	pPluginInfo->m_szShortName = pszShortName;
	pPluginInfo->m_pPlugin = pPlugin;
	pPluginInfo->m_pPlugin->AddRef();
	pPluginInfo->m_ulID = PluginInfo::m_cNextID++;
    }
    else
    {
	pPluginInfo->Init(pPlugin);
    }
    
    m_pPlugins->AddTail(pPluginInfo);
    (*m_pShortNameMap)[pPluginInfo->m_szShortName] = pPluginInfo;

    return NO_ERRORS;
}

void
PluginHandler::DataConvert::Clear()
{
    CHXSimpleList::Iterator	i;
    PluginInfo*			pPluginInfo = NULL;

    for (i = m_pPlugins->Begin(); i != m_pPlugins->End(); ++i)
    {
	pPluginInfo = (PluginInfo*) (*i);
	delete pPluginInfo;
    }
    
    m_pPlugins->RemoveAll();
    m_pShortNameMap->RemoveAll();
}

PluginHandler::Errors
PluginHandler::DataConvert::AddMountPoint(const char* pszShortName,
						  const char* pszMountPoint,
						  IHXValues* pOptions)
{
    Errors	result = NO_ERRORS;
    PluginInfo*	pPluginInfo = NULL;
    char CharLast;

    if (!pszShortName)
    {
	result = INVALID_SHORT_NAME;
	goto cleanup;
    }
    
    if (m_pShortNameMap->Lookup(pszShortName, (void*&)pPluginInfo) != TRUE)
    {
	result = SHORT_NAME_NOT_FOUND;
	goto cleanup;
    }
    
    if (pPluginInfo->m_mount_point.GetLength() != 0)
    {
	PluginInfo* pNewPluginInfo = new PluginInfo();
	if (NO_ERRORS != pNewPluginInfo->Init(pPluginInfo->m_pPlugin))
	    goto cleanup;
	
	m_pPlugins->AddTail(pNewPluginInfo);
	pPluginInfo = pNewPluginInfo;
    }
    if (*pszMountPoint != '/' && *pszMountPoint != '\\')
    {
	// oops , the config file var 
	// doesn't have a leading slash
	// lets add one.
	pPluginInfo->m_mount_point = "/";
    }
    else
    {
	pPluginInfo->m_mount_point = "";
    }
    pPluginInfo->m_mount_point += pszMountPoint;
    CharLast = pPluginInfo->m_mount_point[pPluginInfo->m_mount_point.
							    GetLength()-1];
    if (CharLast != '/' && CharLast != '\\')
    {
	// oops , the config file var 
	// doesn't have a trailing slash
	// lets add one.
	pPluginInfo->m_mount_point += "/";
    }

    if (pOptions)
    {
	IHXBuffer* pTemp;
	pOptions->GetPropertyBuffer("MountPoint", pTemp);
	pTemp->Release();
	pPluginInfo->m_options = pOptions;
	IHXDataConvertSystemObject* pDataConvert;
	
	if (PluginHandler::NO_ERRORS ==
		pPluginInfo->m_pPlugin->GetInstance(&pPluginInfo->m_pInstance))
	{
	    if (HXR_OK == pPluginInfo->m_pInstance->QueryInterface(
			IID_IHXDataConvertSystemObject, (void**)&pDataConvert))
	    {
		pDataConvert->InitDataConvertSystem(pPluginInfo->m_options);
		pDataConvert->Release();
	    }
	}
    }
    else
    {
	pPluginInfo->m_options = NULL;
    }
    
    m_pMountPointHandler->AddMount(pPluginInfo->m_mount_point,
							(void*)pPluginInfo);
    
cleanup:;

    return NO_ERRORS;
}

PluginHandler::Errors
PluginHandler::DataConvert::Find(const char* pszFilePath,
				PluginHandler::DataConvert::PluginInfo*& plugin)
{
    plugin = (PluginInfo*)m_pMountPointHandler->GetMount(pszFilePath, 0);
    if (!plugin)
    {
	return PLUGIN_NOT_FOUND;
    }
    else
    {
	return NO_ERRORS;
    }
}


PluginHandler::DataConvert::PluginInfo::PluginInfo()
{
    m_pInstance = NULL;
    m_pPlugin = NULL;
    m_options = NULL;
}

PluginHandler::DataConvert::PluginInfo::~PluginInfo()
{
    if (m_pInstance)
    {
	m_pInstance->Release();
	m_pInstance = NULL;
    }
    if (m_pPlugin)
    {
	m_pPlugin->Release();
	m_pPlugin = NULL;
    }
    if (m_options)
    {
	m_options->Release();
	m_options = NULL;
    }
}

PluginHandler::Errors
PluginHandler::DataConvert::PluginInfo::Init(Plugin* pPlugin)
{
    Errors result = NO_ERRORS;
    const char* pszShortName = NULL;
    IUnknown* pUnknown = NULL;
    IHXDataConvertSystemObject* pDataConvert = NULL;
    
    if (!pPlugin)
    {
	result = BAD_PLUGIN;
	goto cleanup;
    }
    
    result = pPlugin->GetInstance(&pUnknown);
    if (NO_ERRORS != result)
    {
	goto cleanup;
    }
    
    if (HXR_OK != pUnknown->QueryInterface(IID_IHXDataConvertSystemObject,
							(void**)&pDataConvert))
    {
	result = BAD_PLUGIN;
	goto cleanup;
    }

    if (HXR_OK != pDataConvert->GetDataConvertInfo(pszShortName))
    {
	result = CANT_GET_RENDERER_INFO;
	goto cleanup;
    }
    
    m_ulID = m_cNextID++;
    m_szShortName = pszShortName;
    m_pPlugin = pPlugin;
    m_pPlugin->AddRef();
    
cleanup:;
    if (pDataConvert)
    {
	pDataConvert->Release();
	pDataConvert = NULL;
    }
    
    if (pUnknown)
    {
	pUnknown->Release();
	pUnknown = NULL;
    }
    
    return result;

}


///////////////////////////////////////////////////////////////////////////////
//
// PluginHandler::FileSystem
//
///////////////////////////////////////////////////////////////////////////////
PluginHandler::FileSystem::FileSystem(PluginHandler* pparent)
{
    m_pPlugins = new CHXSimpleList();
    m_pProtocolMap = new CHXMapStringToOb();
    m_pShortNameMap = new CHXMapStringToOb();
    m_pMountPointHandler = new MountPointHandler;
    m_pMountPointHandler->SetTreeID(MP_TREE_FSMOUNT);
    m_pparent = pparent;
}

PluginHandler::FileSystem::~FileSystem()
{
    Clear();

    if (m_pPlugins)
    {
	delete m_pPlugins;
	m_pPlugins = NULL;
    }

    if (m_pProtocolMap)
    {
	delete m_pProtocolMap;
	m_pProtocolMap = NULL;
    }
    
    if (m_pShortNameMap)
    {
	delete m_pShortNameMap;
	m_pShortNameMap = NULL;
    }
    
    delete m_pMountPointHandler;
}

void
PluginHandler::FileSystem::Clear()
{
    CHXSimpleList::Iterator	i;
    PluginInfo*			pPluginInfo = NULL;

    for (i = m_pPlugins->Begin(); i != m_pPlugins->End(); ++i)
    {
	pPluginInfo = (PluginInfo*) (*i);
	delete pPluginInfo;
    }
    
    m_pPlugins->RemoveAll();
    m_pProtocolMap->RemoveAll();
    m_pShortNameMap->RemoveAll();
}

PluginHandler::Errors
PluginHandler::FileSystem::StoreToRegistry(IHXPreferences** ppRegistry)
{
    Errors		    result = NO_ERRORS;
    int			    j = 0;
    char*		    pszData = NULL;
    char*		    pszInfo = NULL;
    char*		    pszTemp = NULL;
    PluginInfo*		    pPluginInfo = NULL;
    IHXBuffer*		    pBuffer = NULL;
    CHXSimpleList::Iterator i;
    char		    CharBuffer[_MAX_PATH];

    if (!ppRegistry)
    {
	result = BAD_REGISTRY_HANDLE;
	goto cleanup;
    }

    if (!*ppRegistry)
    {
	result = BAD_REGISTRY_HANDLE;
	goto cleanup;
    }

    // parse/collect all the plugin info.
    for (i = m_pPlugins->Begin(); i != m_pPlugins->End(); ++i)
    {
	// {
	if (!pszInfo)
	{
	    pszInfo = new char[2];
	    strcpy(pszInfo, "{");
	}
	else
	{
	    pszTemp = new char[strlen(pszInfo)+2];
	    sprintf(pszTemp, "%s%s", pszInfo, "{");
	    
	    delete[] pszInfo;
	    pszInfo = pszTemp;
	}

	pPluginInfo = (PluginInfo*) (*i);

	for (j = 0; j < FILESYSTEM_PLUGIN_RECORD_ENTRIES; j++)
	{
	    switch (j)
	    {
	    case 0:
		pszData = pPluginInfo->m_pPlugin->m_pszDllName;
		break;
	    case 1: 
		sprintf(CharBuffer,"%d",pPluginInfo->m_pPlugin->m_nPluginIndex);
		pszData = CharBuffer;
		break;
	    case 2:	    // {<description>
		pszData = (char*) pPluginInfo->m_pPlugin->m_pszDescription;
		break;
	    case 3:	    // {<description>,<copyright>
		pszData = (char*) pPluginInfo->m_pPlugin->m_pszCopyright;
		break;
	    case 4:	    // {<description>,<copyright>,<moreinfo>
		pszData = (char*) pPluginInfo->m_pPlugin->m_pszMoreInfoUrl;
		break;
	    case 5:	    // {<description>,<copyright>,<moreinfo>,<loadmultiple>
		if (pPluginInfo->m_pPlugin->m_load_multiple)
		{
		    pszData = "TRUE";
		}
		else
		{
		    pszData = "FALSE";
		}
		break;
	    case 6:
		pszData = pPluginInfo->m_szProtocol.GetBuffer(0);
		break;
	    case 7:
		pszData = pPluginInfo->m_szShortName.GetBuffer(0);
		break;
	    default:
		break;
	    }

	    AppendStringToRecord(pszData, &pszInfo);

	    if (j != FILESYSTEM_PLUGIN_RECORD_ENTRIES - 1)
	    {
		// {xxx|xxx,
		pszTemp = new char[strlen(pszInfo)+2];
		sprintf(pszTemp, "%s%c", pszInfo, PLUGIN_ENTRY_SEPARATOR);

		delete[] pszInfo;
		pszInfo = pszTemp;
	    }
	}

	// {xxx,xxx,xxx}
	pszTemp = new char[strlen(pszInfo)+2];
	sprintf(pszTemp, "%s}", pszInfo);
	
	delete[] pszInfo;
	pszInfo = pszTemp;
    }	    
    
    if (pszInfo)
    {
	if (!(pBuffer = new CHXBuffer()))
	{
	    result = MEMORY_ERROR;
	    goto cleanup;
	}

	pBuffer->AddRef();
    
	pBuffer->Set((UCHAR*)pszInfo, strlen(pszInfo)+1);

	(*ppRegistry)->WritePref(REGKEY_FILESYSTEM_PLUGIN, pBuffer);
    
	pBuffer->Release();
    }

cleanup:
    if (pszInfo)
    {
	delete[] pszInfo;
	pszInfo = NULL;
    }

    return result;
}

PluginHandler::Errors
PluginHandler::FileSystem::ReadFromRegistry(IHXPreferences* pRegistry)
{
    Errors	result		= NO_ERRORS;
    IHXBuffer*	pBuffer		= NULL;
    char*	pszInfo		= NULL;
    char*	pszDllName	= NULL;
    char*	pszLoadMultiple = NULL;
    Plugin*	pPlugin		= NULL;
    PluginDLL*	pPluginDLL	= NULL;
    char*       pszindex	= NULL;
    char*	pszProtocol	= NULL;
    char*	pszShortName	= NULL;
    char*	pszPluginIndex	= NULL;
    
    if (!pRegistry)
    {
	result = BAD_REGISTRY_HANDLE;
	goto cleanup;
    }

    // clear the existing ones
    Clear();

    // load from the registry
    if (HXR_OK != pRegistry->ReadPref(REGKEY_FILESYSTEM_PLUGIN, pBuffer))
    {
	result = PLUGIN_NOT_FOUND;
	goto cleanup;
    }

    pszInfo = (char*) pBuffer->GetBuffer();

    // create plugins based on the info
    while (strlen(pszInfo))
    {
	// get dll name
	pszInfo++;
	RetrieveStringFromRecord(&pszInfo, &pszDllName);
		       
	// get the association...
	pPluginDLL=m_pparent->m_PluginFactory->FindDLLFromName(pszDllName);
	RetrieveStringFromRecord(&pszInfo, &pszindex);
	// create plugin
	pPlugin = new Plugin(pszDllName, FALSE, 0, pPluginDLL, atoi(pszindex));
	pPlugin->AddRef();
	delete[] pszindex;

	// get the plugin index
	pszInfo++;
	RetrieveStringFromRecord(&pszInfo, &pszPluginIndex);
	pPlugin->m_nPluginIndex = atoi(pszPluginIndex);
	delete[] pszPluginIndex;


	// get description
	pszInfo++;
	RetrieveStringFromRecord(&pszInfo, (char**)&(pPlugin->m_pszDescription));

	// get copyright
	pszInfo++;
	RetrieveStringFromRecord(&pszInfo, (char**)&(pPlugin->m_pszCopyright));

	// get moreinfo
	pszInfo++;
	RetrieveStringFromRecord(&pszInfo, (char**)&(pPlugin->m_pszMoreInfoUrl));

	// get loadmultiple
	pszInfo++;
	RetrieveStringFromRecord(&pszInfo, &(pszLoadMultiple));

	if (strcmp(pszLoadMultiple, "TRUE") == 0)
	{
	    pPlugin->m_load_multiple = TRUE;
	}
	else
	{
	    pPlugin->m_load_multiple = FALSE;
	}

	// Get the protocol and shortname.


	pszInfo++;
	RetrieveStringFromRecord(&pszInfo, &(pszProtocol));

	pszInfo++;
	RetrieveStringFromRecord(&pszInfo, &(pszShortName));

	// pass through the end of this entry
	while (*pszInfo != '}')
	{
	    pszInfo++;
	}

	// add this plugin
	Add(pPlugin, pszProtocol, pszShortName);

	// freeup
	delete[] pszDllName;
	delete[] pszLoadMultiple;
	delete[] pszProtocol;
	delete[] pszShortName;
	    
	pszDllName = NULL;
	pszLoadMultiple = NULL;

	pPlugin->Release();

	pszInfo++;
    }	

cleanup:

    if (pBuffer)
    {
	pBuffer->Release();
	pBuffer = NULL;
    }

    return result;
}

PluginHandler::Errors
PluginHandler::FileSystem::Find(const char* pszFilePath,
				const char* pszProtocol,
				UINT32& mount_point_len, 
				PluginHandler::FileSystem::PluginInfo*& plugin,
				IHXValues*& options)
{
    PluginInfo*			pPluginInfo = NULL;
    PluginInfo*			pBestPlugin = NULL;
    BOOL			bFound = FALSE;
    char*			pProtocol = NULL;
    CHXSimpleList::Iterator	i;

    mount_point_len = 0;

    if (pszProtocol)
    {
	pProtocol = new char[strlen(pszProtocol)+1];
	strcpy(pProtocol, pszProtocol);

	strlwr(pProtocol);
	
	m_pProtocolMap->Lookup(pProtocol, (void*&)pPluginInfo);

	if (pPluginInfo)
	{
	    bFound = TRUE;
	    pBestPlugin = pPluginInfo;
	}
	
	HX_VECTOR_DELETE(pProtocol);
    }

    if (!bFound && pszFilePath)
    {
	pBestPlugin = (PluginInfo*)m_pMountPointHandler->GetMount(pszFilePath,
								    0);
	if (pBestPlugin)
	{
	    bFound = TRUE;
	    mount_point_len = pBestPlugin->m_mount_point.GetLength();
	}
    }

    if (!bFound)
    {
	return PLUGIN_NOT_FOUND;
    }

    plugin = pBestPlugin;
    options = pBestPlugin->m_options;
    if(options)
	options->AddRef();
    return NO_ERRORS;
}

BOOL
PluginHandler::FileSystem::XPlatformPathCmp(const char* pCharMountPoint, INT32 ulMountPointLength, const char* pCharFilePath)
{
    return 0 == strncmp(pCharMountPoint, pCharFilePath, ulMountPointLength);
}

const char* 
PluginHandler::FileSystem::GetNextPathElement(const char*& pCharPath)
{
    const char* pCharReturn = NULL;

    if (pCharPath)
    {
	pCharReturn = pCharPath;
	while (*pCharPath != '/' && *pCharPath != '\\' && *pCharPath) ++pCharPath;
	++pCharPath;
    }

    return pCharReturn;
}

PluginHandler::Errors
PluginHandler::FileSystem::FindAfter(const char* pszFilePath,
				     UINT32& mount_point_len, 
				     PluginHandler::FileSystem::PluginInfo*& 
				        plugin,
				     IHXValues*& options)
{
    PluginInfo*			pPluginInfo = NULL;
    BOOL			bFound = FALSE;
    CHXSimpleList::Iterator	i;

    if (pszFilePath)
    {
	pPluginInfo = (PluginInfo*)m_pMountPointHandler->GetMount(pszFilePath,
							    (void*)plugin);

	if (pPluginInfo)
	{
	    bFound = TRUE;
	    mount_point_len = pPluginInfo->m_mount_point.GetLength();
	}
    }

    if (!bFound)
    {
	return PLUGIN_NOT_FOUND;
    }

    plugin = pPluginInfo;
    options = pPluginInfo->m_options;
    if(options)
	options->AddRef();
    return NO_ERRORS;
}

PluginHandler::Errors
PluginHandler::FileSystem::FindShort(const char* pszShortName, 
				     PluginHandler::Plugin*& plugin)
{    
    Errors	result = NO_ERRORS;
    PluginInfo*	pPluginInfo = NULL;

    if (!pszShortName)
    {
	result = INVALID_SHORT_NAME;
	goto cleanup;
    }

    if (m_pShortNameMap->Lookup(pszShortName, (void*&)pPluginInfo) == FALSE)
    {
	result = PLUGIN_NOT_FOUND;
	goto cleanup;
    }

    plugin = pPluginInfo->m_pPlugin;

cleanup:

    return result;
}

PluginHandler::Errors
PluginHandler::FileSystem::Add(Plugin* pPlugin, char* pszProtocol, char* pszShortName)
{
    PluginInfo*	    pPluginInfo = NULL;
    Errors	    result = NO_ERRORS;

    if (!(pPluginInfo = new PluginInfo()))
    {
	result = MEMORY_ERROR;
	goto cleanup;
    }

    if (pszProtocol && pszShortName)
    {
	pPluginInfo->m_szProtocol   = pszProtocol;
	pPluginInfo->m_szShortName  = pszShortName;
	pPluginInfo->m_pPlugin = pPlugin;
	pPluginInfo->m_pPlugin->AddRef();
	pPluginInfo->m_ulID = PluginInfo::m_cNextID++;
    }
    else
    {
	result = pPluginInfo->Init(pPlugin);
    }

    if (result != NO_ERRORS)
    {
	goto cleanup;
    }

    m_pPlugins->AddTail(pPluginInfo);

    (*m_pProtocolMap)[pPluginInfo->m_szProtocol] = pPluginInfo;
    (*m_pShortNameMap)[pPluginInfo->m_szShortName] = pPluginInfo;

cleanup:
    return result;
}

PluginHandler::Errors
PluginHandler::FileSystem::AddMountPoint(const char* pszShortName,
					 const char* pszMountPoint,
					 IHXValues* pOptions)
{
    Errors	    result = NO_ERRORS;
    PluginInfo*	    pPluginInfo = NULL;
    char CharLast;

    if (!pszShortName)
    {
	result = INVALID_SHORT_NAME;
	goto cleanup;
    }

    if (m_pShortNameMap->Lookup(pszShortName, (void*&)pPluginInfo) != TRUE)
    {
	result = SHORT_NAME_NOT_FOUND;
	goto cleanup;
    }

    if (pPluginInfo->m_mount_point.GetLength() != 0)
    {
	PluginInfo* pNewPluginInfo = new PluginInfo();
	if(NO_ERRORS != pNewPluginInfo->Init(pPluginInfo->m_pPlugin))
	    goto cleanup;

	m_pPlugins->AddTail(pNewPluginInfo);
	pPluginInfo = pNewPluginInfo;
    }

    if (*pszMountPoint != '/' && *pszMountPoint != '\\')
    {
	// oops , the config file var 
	// doesn't have a leading slash
	// lets add one.
	pPluginInfo->m_mount_point = "/";
    }
    else
    {
	pPluginInfo->m_mount_point = "";
    }
    pPluginInfo->m_mount_point += pszMountPoint;
    CharLast = pPluginInfo->m_mount_point[pPluginInfo->m_mount_point.GetLength()-1];
    if (CharLast != '/' && CharLast != '\\')
    {
	// oops , the config file var 
	// doesn't have a trailing slash
	// lets add one.
	pPluginInfo->m_mount_point += "/";
    }

    if (pOptions)
    {
	pPluginInfo->m_options = pOptions;
	    
	IHXFileSystemObject* pFileSystem;

	/*
	 * The instance is not released until we are done with the PluginInfo
	 */
	if(PluginHandler::NO_ERRORS == 
	   pPluginInfo->m_pPlugin->GetInstance(&pPluginInfo->m_pInstance))
	{
	    if(HXR_OK == pPluginInfo->m_pInstance->QueryInterface(
				IID_IHXFileSystemObject, (void**)&pFileSystem))
	    {
		pFileSystem->InitFileSystem(pPluginInfo->m_options);
		pFileSystem->Release();
	    }
	}

	// make these options 'active'
	CActiveOptions* pActOpts = new CActiveOptions;
	if (pActOpts)
	{
	    pActOpts->AddRef();
	    pActOpts->Init(m_pparent->m_pContext, pOptions);
	    HX_RELEASE(pActOpts);
	}
    }
    else
    {
	pPluginInfo->m_options = NULL;
    }

    m_pMountPointHandler->AddMount(pPluginInfo->m_mount_point,
							(void*)pPluginInfo);
cleanup:

    return NO_ERRORS;
}

UINT32
PluginHandler::FileSystem::GetNumOfPlugins()
{
    return (UINT32) m_pPlugins->GetCount();
}

void
PluginHandler::FileSystem::GetPluginInfo(UINT32	    unIndex,
					 char**	    ppszDllPath,
					 char**	    ppszDescription,
					 char**	    ppszCopyright,
					 char**	    ppszMoreInfo,
					 BOOL*	    pbMultiple,
					 char**	    ppszProtocol,
					 char**     ppszShortName)
{
    PluginInfo*	pPluginInfo = NULL;

    // initialize
    *ppszDllPath = NULL;
    *ppszDescription = NULL;
    *ppszCopyright = NULL;
    *ppszMoreInfo = NULL;
    *pbMultiple = FALSE;
    *ppszProtocol = NULL;
    *ppszShortName = NULL;

    pPluginInfo = (PluginInfo*) GetDataFromList(m_pPlugins, unIndex);

    if (!pPluginInfo)
    {
	return;
    }

    pPluginInfo->m_pPlugin->GetPluginInfo(ppszDllPath, 
					  ppszDescription, 
					  ppszCopyright, 
					  ppszMoreInfo,
					  pbMultiple);

    *ppszProtocol = (char*)(const char*) pPluginInfo->m_szProtocol;
    *ppszShortName = (char*)(const char*) pPluginInfo->m_szShortName;

    return;
}


///////////////////////////////////////////////////////////////////////////////
//
// PluginHandler::FileSystem::PluginInfo
//
///////////////////////////////////////////////////////////////////////////////
PluginHandler::FileSystem::PluginInfo::PluginInfo()
{
    m_pInstance = NULL;
    m_pPlugin   = NULL;
    m_options   = NULL;
}

PluginHandler::FileSystem::PluginInfo::~PluginInfo()
{
    if (m_pInstance)
    {
	m_pInstance->Release();
	m_pInstance = NULL;
    }

    if (m_pPlugin)
    {
	m_pPlugin->Release();
	m_pPlugin = NULL;
    }

    if(m_options)
    {
	m_options->Release();
	m_options = NULL;
    }
}

UINT32	PluginHandler::FileSystem::PluginInfo::m_cNextID = 0;
UINT32  PluginHandler::DataConvert::PluginInfo::m_cNextID = 0;

PluginHandler::Errors
PluginHandler::FileSystem::PluginInfo::Init(Plugin* pPlugin)
{
    Errors			result = NO_ERRORS;
    const char*			pszProtocol = NULL;
    const char*			pszShortName = NULL;
    IUnknown*			pUnknown = NULL;
    IHXFileSystemObject*	pFileSystem = NULL;

    if (!pPlugin)
    {
	result = BAD_PLUGIN;
	goto cleanup;
    }

    result = pPlugin->GetInstance(&pUnknown);
    if (NO_ERRORS != result)
    {
	goto cleanup;
    }

    if (HXR_OK != pUnknown->QueryInterface(IID_IHXFileSystemObject, 
					(void**) &pFileSystem))
    {
	result = BAD_PLUGIN;
	goto cleanup;
    }
   
    if (HXR_OK != pFileSystem->GetFileSystemInfo(pszShortName, pszProtocol))
    {
	result = CANT_GET_RENDERER_INFO;
	goto cleanup;
    }

    m_szShortName = pszShortName;
    m_szProtocol = pszProtocol;
   
    m_pPlugin = pPlugin;
    m_pPlugin->AddRef();

    m_ulID = m_cNextID++;
    //m_pPlugin->m_single_instance = TRUE;

cleanup:
    if (pFileSystem)
    {
	pFileSystem->Release();
	pFileSystem = NULL;
    }

    if (pUnknown)
    {
	pUnknown->Release();
	pUnknown = NULL;
    }


    return result;
}


///////////////////////////////////////////////////////////////////////////////
//
// PluginHandler::StreamDescription
//
///////////////////////////////////////////////////////////////////////////////
PluginHandler::StreamDescription::StreamDescription(PluginHandler* pparent)
{
    m_pparent = pparent;
    m_pPlugins = new CHXSimpleList();
    m_pMimeMap = new CHXMapStringToOb();
}

PluginHandler::StreamDescription::~StreamDescription()
{
    Clear();

    if (m_pPlugins)
    {
	delete m_pPlugins;
	m_pPlugins = NULL;
    }

    if (m_pMimeMap)
    {
	delete m_pMimeMap;
	m_pMimeMap = NULL;
    }
}

PluginHandler::Errors
PluginHandler::StreamDescription::StoreToRegistry(IHXPreferences** ppRegistry)
{
    Errors		    result = NO_ERRORS;
    int			    j = 0;
    char*		    pszData = NULL;
    char*		    pszInfo = NULL;
    char*		    pszTemp = NULL;
    PluginInfo*		    pPluginInfo = NULL;
    IHXBuffer*		    pBuffer = NULL;
    CHXSimpleList::Iterator i;
    char		    CharBuffer[_MAX_PATH];

    if (!ppRegistry)
    {
	result = BAD_REGISTRY_HANDLE;
	goto cleanup;
    }

    if (!*ppRegistry)
    {
	result = BAD_REGISTRY_HANDLE;
	goto cleanup;
    }

    // parse/collect all the plugin info.
    for (i = m_pPlugins->Begin(); i != m_pPlugins->End(); ++i)
    {
	// {
	if (!pszInfo)
	{
	    pszInfo = new char[2];
	    strcpy(pszInfo, "{");
	}
	else
	{
	    pszTemp = new char[strlen(pszInfo)+2];
	    sprintf(pszTemp, "%s%s", pszInfo, "{");
	    
	    delete[] pszInfo;
	    pszInfo = pszTemp;
	}

	pPluginInfo = (PluginInfo*) (*i);

	for (j = 0; j < STREAM_DESCRIPTION_PLUGIN_RECORD_ENTRIES; j++)
	{
	    switch (j)
	    {
	    case 0:
		pszData = (char*) pPluginInfo->m_pPlugin->m_pszDllName;
		break;
	    case 1: 
		sprintf(CharBuffer,"%d",pPluginInfo->m_pPlugin->m_nPluginIndex);
		pszData = CharBuffer;
		break;
	    case 2:	    // {<description>
		pszData = (char*) pPluginInfo->m_pPlugin->m_pszDescription;
		break;
	    case 3:	    // {<description>,<copyright>
		pszData = (char*) pPluginInfo->m_pPlugin->m_pszCopyright;
		break;
	    case 4:	    // {<description>,<copyright>,<moreinfo>
		pszData = (char*) pPluginInfo->m_pPlugin->m_pszMoreInfoUrl;
		break;
	    case 5:	    // {<description>,<copyright>,<moreinfo>,<loadmultiple>
		if (pPluginInfo->m_pPlugin->m_load_multiple)
		{
		    pszData = "TRUE";
		}
		else
		{
		    pszData = "FALSE";
		}
		break;
	    case 6:
		pszData = (char*) pPluginInfo->m_pszMimeType;
		break;
	    default:
		break;
	    }

	    AppendStringToRecord((char *)pszData, &pszInfo);

	    if (j != STREAM_DESCRIPTION_PLUGIN_RECORD_ENTRIES - 1)
	    {
		// {xxx|xxx,
		pszTemp = new char[strlen(pszInfo)+2];
		sprintf(pszTemp, "%s%c", pszInfo, PLUGIN_ENTRY_SEPARATOR);

		delete[] pszInfo;
		pszInfo = pszTemp;
	    }
	}

	// {xxx,xxx}
	pszTemp = new char[strlen(pszInfo)+2];
	sprintf(pszTemp, "%s}", pszInfo);
	
	delete[] pszInfo;
	pszInfo = pszTemp;
    }	    
    
    if (pszInfo)
    {
	if (!(pBuffer = new CHXBuffer()))
	{
	    result = MEMORY_ERROR;
	    goto cleanup;
	}

	pBuffer->AddRef();
    
	pBuffer->Set((UCHAR*)pszInfo, strlen(pszInfo)+1);

	(*ppRegistry)->WritePref(REGKEY_STREAM_DESCRIPTION_PLUGIN, pBuffer);
    
	pBuffer->Release();
    }

cleanup:
    
    if (pszInfo)
    {
	delete[] pszInfo;
	pszInfo = NULL;
    }
    return result;
}

PluginHandler::Errors
PluginHandler::StreamDescription::ReadFromRegistry(IHXPreferences* pRegistry)
{
    Errors	result		    = NO_ERRORS;
    IHXBuffer*	pBuffer		    = NULL;
    char*	pszInfo		    = NULL;
    char*	pszDllName	    = NULL;
    char*	pszLoadMultiple	    = NULL;
    Plugin*	pPlugin		    = NULL;
    PluginDLL*	pPluginDLL	    = NULL;
    char*       pszindex	    = NULL;
    char*	pszMimeType	    = NULL;
    char*	pszPluginIndex	= NULL;
    
    if (!pRegistry)
    {
	result = BAD_REGISTRY_HANDLE;
	goto cleanup;
    }

    // clear the existing ones
    Clear();

    // load from the registry
    if (HXR_OK != pRegistry->ReadPref(REGKEY_STREAM_DESCRIPTION_PLUGIN, pBuffer))
    {
	result = PLUGIN_NOT_FOUND;
	goto cleanup;
    }

    pszInfo = (char*) pBuffer->GetBuffer();

    // create plugins based on the info
    while (strlen(pszInfo))
    {
	// get dll name
	pszInfo++;
	RetrieveStringFromRecord(&pszInfo, &pszDllName);
		       
	// get the association...
	pPluginDLL=m_pparent->m_PluginFactory->FindDLLFromName(pszDllName);
	RetrieveStringFromRecord(&pszInfo, &pszindex);
	// create plugin
	pPlugin = new Plugin(pszDllName, FALSE, 0, pPluginDLL, atoi(pszindex));
	pPlugin->AddRef();
	delete[] pszindex;

	// get the plugin index
	pszInfo++;
	RetrieveStringFromRecord(&pszInfo, &pszPluginIndex);
	pPlugin->m_nPluginIndex = atoi(pszPluginIndex);
	delete[] pszPluginIndex;

	// get description
	pszInfo++;
	RetrieveStringFromRecord(&pszInfo, (char**)&(pPlugin->m_pszDescription));

	// get copyright
	pszInfo++;
	RetrieveStringFromRecord(&pszInfo, (char**)&(pPlugin->m_pszCopyright));

	// get moreinfo
	pszInfo++;
	RetrieveStringFromRecord(&pszInfo, (char**)&(pPlugin->m_pszMoreInfoUrl));

	// get loadmultiple
	pszInfo++;
	RetrieveStringFromRecord(&pszInfo, &(pszLoadMultiple));

	if (strcmp(pszLoadMultiple, "TRUE") == 0)
	{
	    pPlugin->m_load_multiple = TRUE;
	}
	else
	{
	    pPlugin->m_load_multiple = FALSE;
	}

	pszInfo++;
	RetrieveStringFromRecord(&pszInfo, &(pszMimeType));
    


	// pass through the end of this entry
	while (*pszInfo != '}')
	{
	    pszInfo++;
	}

	// add this plugin
	Add(pPlugin, pszMimeType);

	// freeup
	delete[] pszDllName;
	delete[] pszLoadMultiple;

	pszDllName = NULL;
	pszLoadMultiple = NULL;

	pPlugin->Release();

	pszInfo++;
    }	

cleanup:

    if (pBuffer)
    {
	pBuffer->Release();
	pBuffer = NULL;
    }

    return result;
}

PluginHandler::Errors
PluginHandler::StreamDescription::Find(const char* pszMimeType,
				PluginHandler::Plugin*& plugin)
{
    PluginInfo*	pPluginInfo = NULL;

    if (!(pPluginInfo = FindPluginInfo(pszMimeType)))
    {
	return PLUGIN_NOT_FOUND;
    }

    plugin = pPluginInfo->m_pPlugin;

    return NO_ERRORS;
}

PluginHandler::StreamDescription::PluginInfo*
PluginHandler::StreamDescription::FindPluginInfo(const char* pszMimeType)
{
    PluginInfo*			pPluginInfo = NULL;

    /* We first look for plugins supporitng given type*/
    if (pszMimeType)
    {
	if (m_pMimeMap->Lookup(pszMimeType, (void*&)pPluginInfo) != FALSE)
	{
	    return pPluginInfo;
	}
    }

    return NULL;
}

PluginHandler::Errors
PluginHandler::StreamDescription::Add(Plugin* pPlugin, char* pszMimeType)
{
    Errors			result = NO_ERRORS;
    CHXSimpleList::Iterator	i;
    PluginInfo*			pPluginInfo = NULL;
    const char* 		pszExtension = NULL;

    if (!(pPluginInfo = new PluginInfo()))
    {
	result = MEMORY_ERROR;
	goto cleanup;
    }

    if (pszMimeType)
    {
	pPluginInfo->m_pszMimeType  = pszMimeType;
	pPluginInfo->m_pPlugin = pPlugin;
	pPluginInfo->m_pPlugin->AddRef();
    }
    else
    {
	result = pPluginInfo->Init(pPlugin);
    }

    if (NO_ERRORS != result)
    {
	goto cleanup;
    }

    m_pPlugins->AddTail(pPluginInfo);

    (*m_pMimeMap)[pPluginInfo->m_pszMimeType] = pPluginInfo;

cleanup:

    return result;
}

void
PluginHandler::StreamDescription::Clear()
{
    CHXSimpleList::Iterator	i;
    PluginInfo*			pPluginInfo = NULL;

    for (i = m_pPlugins->Begin(); i != m_pPlugins->End(); ++i)
    {
	pPluginInfo = (PluginInfo*) (*i);
	delete pPluginInfo;
    }
    
    m_pPlugins->RemoveAll();
    m_pMimeMap->RemoveAll();
}

UINT32
PluginHandler::StreamDescription::GetNumOfPlugins()
{
    return (UINT32) m_pPlugins->GetCount();
}

void
PluginHandler::StreamDescription::GetPluginInfo(UINT32	unIndex,
					      char**	ppszDllPath,
					      char**    ppszDescription,
					      char**    ppszCopyright,
					      char**    ppszMoreInfo,
					      BOOL*     pbMultiple,
					      char**    ppszMimeType)
{
    PluginInfo*	pPluginInfo = NULL;

    // initialize
    *ppszDllPath = NULL;
    *ppszDescription = NULL;
    *ppszCopyright = NULL;
    *ppszMoreInfo = NULL;
    *pbMultiple = FALSE;
    *ppszMimeType = NULL;
   
    pPluginInfo = (PluginInfo*) GetDataFromList(m_pPlugins, unIndex);

    if (!pPluginInfo)
    {
	return;
    }

    pPluginInfo->m_pPlugin->GetPluginInfo(ppszDllPath, 
					  ppszDescription, 
					  ppszCopyright, 
					  ppszMoreInfo,
					  pbMultiple);

    *ppszMimeType = (char*) pPluginInfo->m_pszMimeType;
   
    return;
}

PluginHandler::Errors
PluginHandler::StreamDescription::FindUsingValues(IHXValues* pValues,
                                                    Plugin*& pPlugin)
{
    Errors rc = PLUGIN_NOT_FOUND;
    IHXBuffer* pBuf = NULL;

    if (SUCCEEDED(pValues->GetPropertyCString(PLUGIN_STREAMDESCRIPTION, pBuf)))
    {
        rc = Find((const char*)pBuf->GetBuffer(), pPlugin);
        pBuf->Release();
    }

    return rc;
}

///////////////////////////////////////////////////////////////////////////////
//
// PluginHandler::StreamDescription::PluginInfo
//
///////////////////////////////////////////////////////////////////////////////
PluginHandler::StreamDescription::PluginInfo::PluginInfo()
{
    m_pPlugin	    = NULL;
    m_pszMimeType   = NULL;
}

PluginHandler::StreamDescription::PluginInfo::~PluginInfo()
{
    if (m_pPlugin)
    {
	m_pPlugin->Release();
	m_pPlugin = NULL;
    }

    if (m_pszMimeType)
    {
	delete [] m_pszMimeType;
	m_pszMimeType = 0;
    }
}

PluginHandler::Errors
PluginHandler::StreamDescription::PluginInfo::Init(Plugin* pPlugin)
{
    const char*                 pszMimeType = NULL;
    Errors			result = NO_ERRORS;
    IUnknown*			pUnknown = NULL;
    IHXStreamDescription*	pStreamDescription = NULL;

    // type checking
    if (!pPlugin)
    {
	result = BAD_PLUGIN;
	goto cleanup;
    }

    result = pPlugin->GetInstance(&pUnknown);
    if (NO_ERRORS != result)
    {
	goto cleanup;
    }

    if (HXR_OK != pUnknown->QueryInterface(IID_IHXStreamDescription, 
					(void**) &pStreamDescription))
    {
	result = BAD_PLUGIN;
	goto cleanup;
    }

    if (HXR_OK != pStreamDescription->GetStreamDescriptionInfo(pszMimeType))
    {
	result = CANT_GET_FILE_FORMAT_INFO;
	goto cleanup;
    }

    m_pszMimeType = new_string(pszMimeType);

    m_pPlugin = pPlugin;
    m_pPlugin->AddRef();
    
cleanup:

    if (pStreamDescription)
    {
	pStreamDescription->Release();
	pStreamDescription = NULL;
    }

    if (pUnknown)
    {
	pUnknown->Release();
	pUnknown = NULL;
    }
   
    return result;
}

///////////////////////////////////////////////////////////////////////////////
//
// PluginHandler::Factory
//
///////////////////////////////////////////////////////////////////////////////
PluginHandler::Factory::Factory(PluginHandler* pparent)
{
    m_pparent = pparent;
    m_pPlugins = new CHXSimpleList();
}

PluginHandler::Factory::~Factory()
{
    HX_DELETE(m_pPlugins);
}

PluginHandler::Errors
PluginHandler::Factory::StoreToRegistry(IHXPreferences** ppRegistry)
{
    Errors		    result = NO_ERRORS;
    int			    j = 0;
    char*		    pszData = NULL;
    char*		    pszInfo = NULL;
    char*		    pszTemp = NULL;
    Plugin*		    pPlugin = NULL;
    IHXBuffer*		    pBuffer = NULL;
    CHXSimpleList::Iterator i;
    char		    CharBuffer[_MAX_PATH];

    if (!ppRegistry)
    {
	result = BAD_REGISTRY_HANDLE;
	goto cleanup;
    }

    if (!*ppRegistry)
    {
	result = BAD_REGISTRY_HANDLE;
	goto cleanup;
    }

    // parse/collect all the plugin info.
    for (i = m_pPlugins->Begin(); i != m_pPlugins->End(); ++i)
    {
	// {
	if (!pszInfo)
	{
	    pszInfo = new char[2];
	    strcpy(pszInfo, "{");
	}
	else
	{
	    pszTemp = new char[strlen(pszInfo)+2];
	    sprintf(pszTemp, "%s%s", pszInfo, "{");
	    
	    delete[] pszInfo;
	    pszInfo = pszTemp;
	}

	pPlugin = (Plugin*) (*i);

	for (j = 0; j < MISC_PLUGIN_RECORD_ENTRIES; j++)
	{
	    switch (j)
	    {
	    case 0:
		pszData = pPlugin->m_pszDllName;
		break;
	    case 1: 
		sprintf(CharBuffer,"%d",pPlugin->m_nPluginIndex);
		pszData = CharBuffer;
		break;
	    case 2:	    // {<description>
		pszData = (char*) pPlugin->m_pszDescription;
		break;
	    case 3:	    // {<description>,<copyright>
		pszData = (char*) pPlugin->m_pszCopyright;
		break;
	    case 4:	    // {<description>,<copyright>,<moreinfo>
		pszData = (char*) pPlugin->m_pszMoreInfoUrl;
		break;
	    case 5:	    // {<description>,<copyright>,<moreinfo>,<loadmultiple>
		if (pPlugin->m_load_multiple)
		{
		    pszData = "TRUE";
		}
		else
		{
		    pszData = "FALSE";
		}
		break;
	    default:
		break;
	    }

	    AppendStringToRecord(pszData, &pszInfo);

	    if (j != MISC_PLUGIN_RECORD_ENTRIES - 1)
	    {
		// {xxx|xxx,
		pszTemp = new char[strlen(pszInfo)+2];
		sprintf(pszTemp, "%s%c", pszInfo, PLUGIN_ENTRY_SEPARATOR);

		delete[] pszInfo;
		pszInfo = pszTemp;
	    }
	}

	// {xxx}
	pszTemp = new char[strlen(pszInfo)+2];
	sprintf(pszTemp, "%s}", pszInfo);
	
	delete[] pszInfo;
	pszInfo = pszTemp;
    }	    
    
    if (pszInfo)
    {
	if (!(pBuffer = new CHXBuffer()))
	{
	    result = MEMORY_ERROR;
	    goto cleanup;
	}

	pBuffer->AddRef();
    
	pBuffer->Set((UCHAR*)pszInfo, strlen(pszInfo)+1);

	(*ppRegistry)->WritePref(REGKEY_FACTORY_PLUGIN, pBuffer);

	pBuffer->Release();
    }

cleanup:
    if (pszInfo)
    {
	delete[] pszInfo;
	pszInfo = NULL;
    }

    return result;
}

PluginHandler::Errors
PluginHandler::Factory::ReadFromRegistry(IHXPreferences* pRegistry)
{
    Errors	result = NO_ERRORS;
    IHXBuffer*	pBuffer = NULL;
    char*	pszInfo = NULL;
    char*	pszDllName = NULL;
    char*	pszLoadMultiple = NULL;
    Plugin*	pPlugin	= NULL;
    PluginDLL*	pPluginDLL = NULL;
    char*       pszindex = NULL;
    char*	pszPluginIndex	= NULL;
    
    if (!pRegistry)
    {
	result = BAD_REGISTRY_HANDLE;
	goto cleanup;
    }

    // clear the existing ones
    Clear();

    // load from the registry
    if (HXR_OK != pRegistry->ReadPref(REGKEY_FACTORY_PLUGIN, pBuffer))
    {
	result = PLUGIN_NOT_FOUND;
	goto cleanup;
    }

    pszInfo = (char*) pBuffer->GetBuffer();

    // create plugins based on the info
    while (strlen(pszInfo))
    {
	// get dll name
	pszInfo++;
	RetrieveStringFromRecord(&pszInfo, &pszDllName);
		       
	// get the association...
	pPluginDLL=m_pparent->m_PluginFactory->FindDLLFromName(pszDllName);
	RetrieveStringFromRecord(&pszInfo, &pszindex);
	// create plugin
	pPlugin = new Plugin(pszDllName, FALSE, 0, pPluginDLL, atoi(pszindex));
	pPlugin->AddRef();
	delete[] pszindex;

	// get the plugin index
	pszInfo++;
	RetrieveStringFromRecord(&pszInfo, &pszPluginIndex);
	pPlugin->m_nPluginIndex = atoi(pszPluginIndex);
	delete[] pszPluginIndex;

	// get description
	pszInfo++;
	RetrieveStringFromRecord(&pszInfo, (char**)&(pPlugin->m_pszDescription));

	// get copyright
	pszInfo++;
	RetrieveStringFromRecord(&pszInfo, (char**)&(pPlugin->m_pszCopyright));

	// get moreinfo
	pszInfo++;
	RetrieveStringFromRecord(&pszInfo, (char**)&(pPlugin->m_pszMoreInfoUrl));

	// get loadmultiple
	pszInfo++;
	RetrieveStringFromRecord(&pszInfo, &(pszLoadMultiple));

	if (strcmp(pszLoadMultiple, "TRUE") == 0)
	{
	    pPlugin->m_load_multiple = TRUE;
	}
	else
	{
	    pPlugin->m_load_multiple = FALSE;
	}

	// pass through the end of this entry
	while (*pszInfo != '}')
	{
	    pszInfo++;
	}

	// add this plugin
	Add(pPlugin);

	// freeup
	delete[] pszDllName;
	delete[] pszLoadMultiple;

	pszDllName = NULL;
	pszLoadMultiple = NULL;

	pPlugin->Release();

	pszInfo++;
    }	

cleanup:

    if (pBuffer)
    {
	pBuffer->Release();
	pBuffer = NULL;
    }

    return result;
}

PluginHandler::Errors
PluginHandler::Factory::Add(Plugin* pPlugin)
{
    m_pPlugins->AddTail(pPlugin);
    pPlugin->AddRef();
    return NO_ERRORS;
}

void
PluginHandler::Factory::Clear()
{
    CHXSimpleList::Iterator i;
    Plugin* pPlugin = NULL;
    for(i = m_pPlugins->Begin(); i != m_pPlugins->End(); ++i)
    {
	pPlugin = (Plugin*) (*i);
	pPlugin->Release();
    }

    m_pPlugins->RemoveAll();
}

UINT32
PluginHandler::Factory::GetNumOfPlugins()
{
    return (UINT32) m_pPlugins->GetCount();
}

void
PluginHandler::Factory::GetPluginInfo(UINT32 unIndex,
				      char** ppszDllPath,
				      char** ppszDescription,
				      char** ppszCopyright,
				      char** ppszMoreInfo,
				      BOOL*  pbMultiple)
{
    Plugin*	pPlugin = NULL;

    // initialize
    *ppszDllPath = NULL;
    *ppszDescription = NULL;
    *ppszCopyright = NULL;
    *ppszMoreInfo = NULL;
    *pbMultiple = FALSE;
   
    pPlugin = (Plugin*) GetDataFromList(m_pPlugins, unIndex);

    if (!pPlugin)
    {
	return;
    }

    pPlugin->GetPluginInfo(ppszDllPath, 
	 		   ppszDescription, 
			   ppszCopyright, 
		      	   ppszMoreInfo,
			   pbMultiple);
    return;
}
    
///////////////////////////////////////////////////////////////////////////////
//
// PluginHandler::AllowancePlugins
//
///////////////////////////////////////////////////////////////////////////////
PluginHandler::AllowancePlugins::AllowancePlugins(PluginHandler* pparent)
                                                : m_pparent(pparent)
                                                , m_bClientStartupOptimization(TRUE)
                                                , m_bDebug(FALSE)
{
    m_pPluginInfoList = new CHXSimpleList();
    m_pNonMultiPluginsWithMountPoint = new MountPointHandler();
}

PluginHandler::AllowancePlugins::~AllowancePlugins()
{
    Clear();

    HX_DELETE(m_pNonMultiPluginsWithMountPoint);
    HX_DELETE(m_pPluginInfoList);
}

void PluginHandler::AllowancePlugins::Init(IHXPreferences* pPreferences)
{
    m_bClientStartupOptimization = TRUE;
    if (pPreferences)
    {
        IHXRegistry* pRegistry = NULL;
        INT32 lTemp = 0;

        if (HXR_OK == m_pparent->m_pContext->QueryInterface(IID_IHXRegistry, (void **)&pRegistry))
        {
            if (HXR_OK == pRegistry->GetIntByName(REGISTRY_KEY_ALLOWANCE_STARTUP_OPTIMIZATION
                                                 , lTemp))
            {
                m_bClientStartupOptimization = (lTemp > 0) ? TRUE : FALSE;
            }
            if (HXR_OK == pRegistry->GetIntByName(REGISTRY_KEY_ALLOWANCE_OPTIMIZATION_TRACE
                                                 , lTemp))
            {
                m_bDebug = (lTemp > 0) ? TRUE : FALSE;
            }
        }
        HX_RELEASE(pRegistry);
    }
}

PluginHandler::Errors
PluginHandler::AllowancePlugins::StoreToRegistry(IHXPreferences** ppRegistry)
{
    Errors		    result = NO_ERRORS;
    int			    j = 0;
    char*		    pszData = NULL;
    char*		    pszInfo = NULL;
    char*		    pszTemp = NULL;
    Plugin*		    pPlugin = NULL;
    IHXBuffer*		    pBuffer = NULL;
    CHXSimpleList::Iterator i;
    char		    CharBuffer[_MAX_PATH];

    if (!ppRegistry)
    {
	result = BAD_REGISTRY_HANDLE;
	goto cleanup;
    }

    if (!*ppRegistry)
    {
	result = BAD_REGISTRY_HANDLE;
	goto cleanup;
    }

    // parse/collect all the plugin info.
    for (i = m_pPluginInfoList->Begin(); i != m_pPluginInfoList->End(); ++i)
    {
	if (!pszInfo)
	{
	    pszInfo = new char[2];
	    strcpy(pszInfo, "{");
	}
	else
	{
	    pszTemp = new char[strlen(pszInfo)+2];
	    sprintf(pszTemp, "%s%s", pszInfo, "{");
	    
	    delete[] pszInfo;
	    pszInfo = pszTemp;
	}

	pPlugin = ((PluginInfo*)(*i))->m_pPlugin;

	for (j = 0; j < MISC_PLUGIN_RECORD_ENTRIES; j++)
	{
	    switch (j)
	    {
	    case 0:
		pszData = pPlugin->m_pPluginDLL->m_pszFileName;
		break;
	    case 1: 
		sprintf(CharBuffer,"%d",pPlugin->m_nPluginIndex);
		pszData = CharBuffer;
		break;
	    case 2:	    // <description>
		pszData = (char*) pPlugin->m_pszDescription;
		break;
	    case 3:	    // <description>,<copyright>
		pszData = (char*) pPlugin->m_pszCopyright;
		break;
	    case 4:	    // <description>,<copyright>,<moreinfo>
		pszData = (char*) pPlugin->m_pszMoreInfoUrl;
		break;
	    case 5:	    // <description>,<copyright>,<moreinfo>,<loadmultiple>
		if (pPlugin->m_load_multiple)
		{
		    pszData = "TRUE";
		}
		else
		{
		    pszData = "FALSE";
		}
		break;
	    default:
		break;
	    }

	    AppendStringToRecord(pszData, &pszInfo);

	    if (j != MISC_PLUGIN_RECORD_ENTRIES - 1)
	    {
		// {xxx|xxx,
		pszTemp = new char[strlen(pszInfo)+2];
		sprintf(pszTemp, "%s%c", pszInfo, PLUGIN_ENTRY_SEPARATOR);

		HX_VECTOR_DELETE(pszInfo);
		pszInfo = pszTemp;
	    }
	}

	// {xxx}
	pszTemp = new char[strlen(pszInfo)+2];
	sprintf(pszTemp, "%s}", pszInfo);
	
	HX_VECTOR_DELETE(pszInfo);
	pszInfo = pszTemp;
    }	    
    
    if (pszInfo)
    {
	if (!(pBuffer = new CHXBuffer()))
	{
	    result = MEMORY_ERROR;
	    goto cleanup;
	}

	pBuffer->AddRef();
    
	pBuffer->Set((UCHAR*)pszInfo, strlen(pszInfo)+1);

	(*ppRegistry)->WritePref(REGKEY_ALLOWANCE_PLUGIN, pBuffer);
    
	pBuffer->Release();
    }

cleanup:
    HX_VECTOR_DELETE(pszInfo);
    return result;
}

PluginHandler::Errors
PluginHandler::AllowancePlugins::ReadFromRegistry(IHXPreferences* pRegistry)
{
    Errors	result = NO_ERRORS;
    IHXBuffer*	pBuffer = NULL;
    char*	pszInfo = NULL;
    char*	pszDllName = NULL;
    char*	pszLoadMultiple = NULL;
    Plugin*	pPlugin	= NULL;
    PluginDLL*	pPluginDLL = NULL;
    char*       pszindex = NULL;
    char*	pszPluginIndex	= NULL;

    if (!pRegistry)
    {
	result = BAD_REGISTRY_HANDLE;
	goto cleanup;
    }

    // clear the existing ones
    Clear();

    // load from the registry
    if (HXR_OK != pRegistry->ReadPref(REGKEY_ALLOWANCE_PLUGIN, pBuffer))
    {
        result = PLUGIN_NOT_FOUND;
        goto cleanup;
    }

    pszInfo = (char*) pBuffer->GetBuffer();

    // create plugins based on the info
    while (strlen(pszInfo))
    {
	// get dll name
	pszInfo++;
	RetrieveStringFromRecord(&pszInfo, &pszDllName);
		       
	// get the association...
	pPluginDLL=m_pparent->m_PluginFactory->FindDLLFromName(pszDllName);
	RetrieveStringFromRecord(&pszInfo, &pszindex);
	// create plugin
	pPlugin = new Plugin(pszDllName, FALSE, 0, pPluginDLL, atoi(pszindex));
	pPlugin->AddRef();
	HX_VECTOR_DELETE(pszindex);

	// get the plugin index
	pszInfo++;
	RetrieveStringFromRecord(&pszInfo, &pszPluginIndex);
	pPlugin->m_nPluginIndex = atoi(pszPluginIndex);
	HX_VECTOR_DELETE(pszPluginIndex);

	// get description
	pszInfo++;
	RetrieveStringFromRecord(&pszInfo, (char**)&(pPlugin->m_pszDescription));

	// get copyright
	pszInfo++;
	RetrieveStringFromRecord(&pszInfo, (char**)&(pPlugin->m_pszCopyright));

	// get moreinfo
	pszInfo++;
	RetrieveStringFromRecord(&pszInfo, (char**)&(pPlugin->m_pszMoreInfoUrl));

	// get loadmultiple
	pszInfo++;
	RetrieveStringFromRecord(&pszInfo, &(pszLoadMultiple));

	if (strcmp(pszLoadMultiple, "TRUE") == 0)
	{
	    pPlugin->m_load_multiple = TRUE;
	}
	else
	{
	    pPlugin->m_load_multiple = FALSE;
	}

	// pass through the end of this entry
	while (*pszInfo != '}')
	{
	    pszInfo++;
	}

	// add this plugin
	Add(pPlugin);

	// freeup
	HX_VECTOR_DELETE(pszDllName);
	HX_VECTOR_DELETE(pszLoadMultiple);

	pszDllName = NULL;
	pszLoadMultiple = NULL;

	pPlugin->Release();

	pszInfo++;
    }	

cleanup:

    HX_RELEASE(pBuffer);
    
    return result;
}

PluginHandler::Errors
PluginHandler::AllowancePlugins::Add(Plugin* pPlugin)
{
    PluginHandler::Errors result = NO_ERRORS;

    PluginInfo* pPluginInfo = new PluginInfo();

    if ( (result = pPluginInfo->Init(pPlugin)) == NO_ERRORS)
    {
        m_pPluginInfoList->AddTail(pPluginInfo);
    }

    return result;
}

void
PluginHandler::AllowancePlugins::Clear()
{
    CHXSimpleList::Iterator	i;
    PluginInfo*			pPluginInfo = NULL;

    for (i = m_pPluginInfoList->Begin(); i != m_pPluginInfoList->End(); ++i)
    {
	pPluginInfo = (PluginHandler::AllowancePlugins::PluginInfo*) (*i);
        if (pPluginInfo && !pPluginInfo->m_bMultiLoad 
                     && pPluginInfo->m_szFileSysMountPoint.GetLength())
        {
            m_pNonMultiPluginsWithMountPoint->
                        RemoveMount(pPluginInfo->m_szFileSysMountPoint.GetBuffer(0));
        }
        HX_DELETE(pPluginInfo);
    }

    m_pPluginInfoList->RemoveAll();
}

UINT32
PluginHandler::AllowancePlugins::GetNumOfPlugins()
{
    return (UINT32) m_pPluginInfoList->GetCount();
}

void
PluginHandler::AllowancePlugins::GetPluginInfo(UINT32		    unIndex,
				   char**		    ppszDllPath,
				   char**		    ppszDescription,
				   char**		    ppszCopyright,
				   char**		    ppszMoreInfo,
				   BOOL*		    pbMultiple)
{
    PluginInfo*	pPluginInfo = NULL;

    // initialize
    *ppszDllPath = NULL;
    *ppszDescription = NULL;
    *ppszCopyright = NULL;
    *ppszMoreInfo = NULL;
    *pbMultiple = FALSE;
   
    pPluginInfo = (PluginInfo*) GetDataFromList(m_pPluginInfoList, unIndex);

    if (!pPluginInfo)
    {
	return;
    }

    pPluginInfo->m_pPlugin->GetPluginInfo(ppszDllPath, 
	 		   ppszDescription, 
			   ppszCopyright, 
		      	   ppszMoreInfo,
			   pbMultiple);
    return;
}

PluginHandler::Errors
PluginHandler::AllowancePlugins::AddMountPoint(IHXBuffer* pShortNameBuff,
                        const char* pszMountPoint,
                        IHXValues* pszOptions)
{
    CHXSimpleList::Iterator iter;
    PluginInfo* pPluginInfo;
    PluginInfo* pDeletePluginInfo = NULL;
    const char *pszShortName = (const char *)pShortNameBuff->GetBuffer();
    UINT32 pBuffSize = pShortNameBuff->GetSize();

    for (iter = m_pPluginInfoList->Begin(); iter != m_pPluginInfoList->End(); ++iter)
    {
        pPluginInfo = (PluginInfo*)*iter;
        if (!strncasecmp(pszShortName, (const char *)(pPluginInfo->m_szFileSysShortName), pBuffSize))
        {
            pPluginInfo->m_szFileSysMountPoint = pszMountPoint;

            //MOVE PluginINFO to MountPointHandler if a valid 
            // mount-point exists
            if (m_bClientStartupOptimization && pszMountPoint && !pPluginInfo->m_bMultiLoad)
            {
                m_pNonMultiPluginsWithMountPoint->AddMount(pszMountPoint, pPluginInfo);

                //REMOVE PluginInfo from PluginInfoList
                // since it has been moved to the mount-point based list
                pDeletePluginInfo = pPluginInfo;
            }
            break;
        }
    }

    // Remove PluginInfo that was moved to mount-point based list
    //  from the general list
    //
    //XXXVS: Commented out the deletion part as OnConnection() needs
    // to be called on all allowance plugins.
    //
    //ClientSession will not add Plugins that are non-multiload 
    // and have a mount-point set to the AllowanceManager list
    /* ***
    if (pDeletePluginInfo)
    {
        LISTPOSITION pos = m_pPluginInfoList->Find(pDeletePluginInfo);
        if (pos)
        {
            m_pPluginInfoList->RemoveAt(pos);
        }
    }
    ***  */

    return NO_ERRORS;
}

PluginHandler::Errors
PluginHandler::AllowancePlugins::FindPluginFromMountPoint(const char* pszFilePath,
                                                          PluginInfo*& pPluginInfo)
{
    PluginInfo *pBestPluginInfo = NULL;

    if (pszFilePath)
    {
        pBestPluginInfo = (PluginInfo*)m_pNonMultiPluginsWithMountPoint->GetMount(pszFilePath, 0);
        if (pBestPluginInfo)
        {
            pPluginInfo = pBestPluginInfo;
        }
        else
        {
            return PLUGIN_NOT_FOUND;
        }
    }

    return NO_ERRORS;
}

void PluginHandler::AllowancePlugins::PrintDebugInfo()
{
    CHXSimpleList::Iterator iter;
    PluginInfo* pPluginInfo = NULL;
    char*       pszDllName = NULL;
    char*       pszDesc = NULL;
    char*       pszCopyright = NULL;
    char*       pszMoreInfo = NULL;
    BOOL        bMultiLoad = FALSE;

    fprintf(stderr, "****** (Multiload/Non-Multiload) Plugin_Description: short_name -- mount_point *******\n");
    for (iter = m_pPluginInfoList->Begin(); iter != m_pPluginInfoList->End(); ++iter)
    {
        pPluginInfo = (PluginInfo*)*iter;
        if (pPluginInfo && pPluginInfo->m_pPlugin)
        {
            pPluginInfo->m_pPlugin->GetPluginInfo( &pszDllName, &pszDesc, &pszCopyright,
                                                &pszMoreInfo, &bMultiLoad);
            fprintf( stderr, "(%s) %s: %s -- %s\n", 
                                bMultiLoad ? "Multiload" : "Non-Multiload", 
                                pszDesc, 
                                (const char*)pPluginInfo->m_szFileSysShortName.GetBuffer(0),
                                (const char*)pPluginInfo->m_szFileSysMountPoint.GetBuffer(0));
        }
    }
    fprintf(stderr, "\r\nPrinting MountPointTree\n");
    fprintf(stderr, "---------------------------\n");
    m_pNonMultiPluginsWithMountPoint->PrintTree();
    fprintf(stderr, "*************************************************************************************\n");
}

PluginHandler::AllowancePlugins::PluginInfo::~PluginInfo()
{
    HX_RELEASE(m_pPlugin);
}

PluginHandler::Errors
PluginHandler::AllowancePlugins::PluginInfo::Init(PluginHandler::Plugin* pPlugin)
{
    Errors                      result = NO_ERRORS;
    IUnknown*                   pUnknown = NULL;
    IHXFileSystemObject*        pFileSystem = NULL;
    const char*                 pszProtocol = NULL;
    const char*                 pszShortName = NULL;
    char*                       pszDllName = NULL;
    char*                       pszDesc = NULL;
    char*                       pszCopyright = NULL;
    char*                       pszMoreInfo = NULL;
    BOOL                        bMultiLoad = FALSE;

    if (!pPlugin)
    {
        result = BAD_PLUGIN;
        goto cleanup;
    }

    m_pPlugin = pPlugin;
    m_pPlugin->AddRef();

    // Get hold of Plugin's MultiLoad support. Required for Allowance startup 
    // optimization
    pPlugin->GetPluginInfo( &pszDllName, &pszDesc, &pszCopyright,
                                         &pszMoreInfo, &m_bMultiLoad);

    result = pPlugin->GetInstance(&pUnknown);
    if (NO_ERRORS != result)
    {
        goto cleanup;
    }


    //Get hold of filesystem information if this plugin 
    // is also a File System plugin
    //
    //This information is required to load relevant Mountpoint
    // information when available.
    //
    if (HXR_OK != pUnknown->QueryInterface(IID_IHXFileSystemObject, 
                                        (void**) &pFileSystem))
    {
        goto cleanup;
    }
   
    if (HXR_OK != pFileSystem->GetFileSystemInfo(pszShortName, pszProtocol))
    {
        result = CANT_GET_RENDERER_INFO;
        goto cleanup;
    }

    m_szFileSysShortName = pszShortName;
    m_szFileSysProtocol = pszProtocol;
cleanup:
    HX_RELEASE(pFileSystem);
    HX_RELEASE(pUnknown);

    return result;
}

///////////////////////////////////////////////////////////////////////////////
//
// PluginHandler::Basic
//
///////////////////////////////////////////////////////////////////////////////
PluginHandler::Basic::Basic(ULONG32 ulType, PluginHandler* pparent)
{
    m_pparent = pparent;
    m_ulType = ulType;
    m_pPlugins = new CHXSimpleList();
}

PluginHandler::Basic::~Basic()
{
    Clear();

    if (m_pPlugins)
    {
	delete m_pPlugins;
	m_pPlugins = NULL;
    }
}

PluginHandler::Errors
PluginHandler::Basic::StoreToRegistry(IHXPreferences** ppRegistry)
{
    Errors		    result = NO_ERRORS;
    int			    j = 0;
    char*		    pszData = NULL;
    char*		    pszInfo = NULL;
    char*		    pszTemp = NULL;
    Plugin*		    pPlugin = NULL;
    IHXBuffer*		    pBuffer = NULL;
    CHXSimpleList::Iterator i;
    char		    CharBuffer[_MAX_PATH];

    if (!ppRegistry)
    {
	result = BAD_REGISTRY_HANDLE;
	goto cleanup;
    }

    if (!*ppRegistry)
    {
	result = BAD_REGISTRY_HANDLE;
	goto cleanup;
    }

    // parse/collect all the plugin info.
    for (i = m_pPlugins->Begin(); i != m_pPlugins->End(); ++i)
    {
	// {
	if (!pszInfo)
	{
	    pszInfo = new char[2];
	    strcpy(pszInfo, "{");
	}
	else
	{
	    pszTemp = new char[strlen(pszInfo)+2];
	    sprintf(pszTemp, "%s%s", pszInfo, "{");
	    
	    delete[] pszInfo;
	    pszInfo = pszTemp;
	}

	pPlugin = (Plugin*) (*i);

	for (j = 0; j < MISC_PLUGIN_RECORD_ENTRIES; j++)
	{
	    switch (j)
	    {
	    case 0:
		pszData = pPlugin->m_pPluginDLL->m_pszFileName;
		break;
	    case 1: 
		sprintf(CharBuffer,"%d",pPlugin->m_nPluginIndex);
		pszData = CharBuffer;
		break;
	    case 2:	    // {<description>
		pszData = (char*) pPlugin->m_pszDescription;
		break;
	    case 3:	    // {<description>,<copyright>
		pszData = (char*) pPlugin->m_pszCopyright;
		break;
	    case 4:	    // {<description>,<copyright>,<moreinfo>
		pszData = (char*) pPlugin->m_pszMoreInfoUrl;
		break;
	    case 5:	    // {<description>,<copyright>,<moreinfo>,<loadmultiple>
		if (pPlugin->m_load_multiple)
		{
		    pszData = "TRUE";
		}
		else
		{
		    pszData = "FALSE";
		}
		break;
	    default:
		break;
	    }

	    AppendStringToRecord(pszData, &pszInfo);

	    if (j != MISC_PLUGIN_RECORD_ENTRIES - 1)
	    {
		// {xxx|xxx,
		pszTemp = new char[strlen(pszInfo)+2];
		sprintf(pszTemp, "%s%c", pszInfo, PLUGIN_ENTRY_SEPARATOR);

		delete[] pszInfo;
		pszInfo = pszTemp;
	    }
	}

	// {xxx}
	pszTemp = new char[strlen(pszInfo)+2];
	sprintf(pszTemp, "%s}", pszInfo);
	
	delete[] pszInfo;
	pszInfo = pszTemp;
    }	    
    
    if (pszInfo)
    {
	if (!(pBuffer = new CHXBuffer()))
	{
	    result = MEMORY_ERROR;
	    goto cleanup;
	}

	pBuffer->AddRef();
    
	pBuffer->Set((UCHAR*)pszInfo, strlen(pszInfo)+1);

	if (MISC_PLUGINS == m_ulType)
	{
	    (*ppRegistry)->WritePref(REGKEY_MISC_PLUGIN, pBuffer);
	}
	else if (GENERAL_PLUGINS == m_ulType)
	{
	    (*ppRegistry)->WritePref(REGKEY_GENERAL_PLUGIN, pBuffer);
	}
	else if (ALLOWANCE_PLUGINS == m_ulType)
	{
	    (*ppRegistry)->WritePref(REGKEY_ALLOWANCE_PLUGIN, pBuffer);
	}
    
	pBuffer->Release();
    }

cleanup:
    if (pszInfo)
    {
	delete[] pszInfo;
	pszInfo = NULL;
    }

    return result;
}

PluginHandler::Errors
PluginHandler::Basic::ReadFromRegistry(IHXPreferences* pRegistry)
{
    Errors	result = NO_ERRORS;
    IHXBuffer*	pBuffer = NULL;
    char*	pszInfo = NULL;
    char*	pszDllName = NULL;
    char*	pszLoadMultiple = NULL;
    Plugin*	pPlugin	= NULL;
    PluginDLL*	pPluginDLL = NULL;
    char*       pszindex = NULL;
    char*	pszPluginIndex	= NULL;
    
    if (!pRegistry)
    {
	result = BAD_REGISTRY_HANDLE;
	goto cleanup;
    }

    // clear the existing ones
    Clear();

    // load from the registry
    if (MISC_PLUGINS == m_ulType)
    {
	if (HXR_OK != pRegistry->ReadPref(REGKEY_MISC_PLUGIN, pBuffer))
	{
	    result = PLUGIN_NOT_FOUND;
	    goto cleanup;
	}
    }
    else if (GENERAL_PLUGINS == m_ulType)
    {
	if (HXR_OK != pRegistry->ReadPref(REGKEY_GENERAL_PLUGIN, pBuffer))
	{
	    result = PLUGIN_NOT_FOUND;
	    goto cleanup;
	}
    }
    else if (ALLOWANCE_PLUGINS == m_ulType)
    {
	if (HXR_OK != pRegistry->ReadPref(REGKEY_ALLOWANCE_PLUGIN, pBuffer))
	{
	    result = PLUGIN_NOT_FOUND;
	    goto cleanup;
	}
    }
    else
    {
	result = PLUGIN_NOT_FOUND;
	goto cleanup;
    }

    pszInfo = (char*) pBuffer->GetBuffer();

    // create plugins based on the info
    while (strlen(pszInfo))
    {
	// get dll name
	pszInfo++;
	RetrieveStringFromRecord(&pszInfo, &pszDllName);
		       
	// get the association...
	pPluginDLL=m_pparent->m_PluginFactory->FindDLLFromName(pszDllName);
	RetrieveStringFromRecord(&pszInfo, &pszindex);
	// create plugin
	pPlugin = new Plugin(pszDllName, FALSE, 0, pPluginDLL, atoi(pszindex));
	pPlugin->AddRef();
	delete[] pszindex;

	// get the plugin index
	pszInfo++;
	RetrieveStringFromRecord(&pszInfo, &pszPluginIndex);
	pPlugin->m_nPluginIndex = atoi(pszPluginIndex);
	delete[] pszPluginIndex;

	// get description
	pszInfo++;
	RetrieveStringFromRecord(&pszInfo, (char**)&(pPlugin->m_pszDescription));

	// get copyright
	pszInfo++;
	RetrieveStringFromRecord(&pszInfo, (char**)&(pPlugin->m_pszCopyright));

	// get moreinfo
	pszInfo++;
	RetrieveStringFromRecord(&pszInfo, (char**)&(pPlugin->m_pszMoreInfoUrl));

	// get loadmultiple
	pszInfo++;
	RetrieveStringFromRecord(&pszInfo, &(pszLoadMultiple));

	if (strcmp(pszLoadMultiple, "TRUE") == 0)
	{
	    pPlugin->m_load_multiple = TRUE;
	}
	else
	{
	    pPlugin->m_load_multiple = FALSE;
	}

	// pass through the end of this entry
	while (*pszInfo != '}')
	{
	    pszInfo++;
	}

	// add this plugin
	Add(pPlugin);

	// freeup
	delete[] pszDllName;
	delete[] pszLoadMultiple;

	pszDllName = NULL;
	pszLoadMultiple = NULL;

	pPlugin->Release();

	pszInfo++;
    }	

cleanup:

    if (pBuffer)
    {
	pBuffer->Release();
	pBuffer = NULL;
    }

    return result;
}

PluginHandler::Errors
PluginHandler::Basic::Add(Plugin* pPlugin)
{    
    m_pPlugins->AddTail((void *)pPlugin);
    pPlugin->AddRef();

    return NO_ERRORS;
}

void
PluginHandler::Basic::Clear()
{
    CHXSimpleList::Iterator	i;
    Plugin*			pPlugin = NULL;

    for (i = m_pPlugins->Begin(); i != m_pPlugins->End(); ++i)
    {
	pPlugin = (Plugin*) (*i);
	pPlugin->Release();
    }

    m_pPlugins->RemoveAll();
}

UINT32
PluginHandler::Basic::GetNumOfPlugins()
{
    return (UINT32) m_pPlugins->GetCount();
}

void
PluginHandler::Basic::GetPluginInfo(UINT32		    unIndex,
				   char**		    ppszDllPath,
				   char**		    ppszDescription,
				   char**		    ppszCopyright,
				   char**		    ppszMoreInfo,
				   BOOL*		    pbMultiple)
{
    Plugin*	pPlugin = NULL;

    // initialize
    *ppszDllPath = NULL;
    *ppszDescription = NULL;
    *ppszCopyright = NULL;
    *ppszMoreInfo = NULL;
    *pbMultiple = FALSE;
   
    pPlugin = (Plugin*) GetDataFromList(m_pPlugins, unIndex);

    if (!pPlugin)
    {
	return;
    }

    pPlugin->GetPluginInfo(ppszDllPath, 
	 		   ppszDescription, 
			   ppszCopyright, 
		      	   ppszMoreInfo,
			   pbMultiple);
    return;
}



///////////////////////////////////////////////////////////////////////////////
//
// PluginHandler::Plugin
//
///////////////////////////////////////////////////////////////////////////////

PluginHandler::Plugin::Plugin(const char* pszDllName,
			     BOOL do_unload,
			     IHXErrorMessages* pErrorMessages, PluginDLL  * pPluginDLL, UINT16 index)
{
#if defined (_WIN32)
    InitializeCriticalSection(&m_critSec);
#endif

//    m_pszDllName = new char[sizeof(pszDllName)+1];
//	strcpy(m_pszDllName, pszDllName);
		
    m_pszDllName = new_string(pszDllName);
    m_do_unload = do_unload;
    m_single_instance = FALSE;
    m_pErrorMessages = pErrorMessages;
	m_pPluginDLL  = pPluginDLL;
	 m_nPluginIndex = index;
    if (m_pErrorMessages)
	m_pErrorMessages->AddRef();

    m_lRefCount = 0;

    m_pszDescription = NULL;
    m_pszCopyright = NULL;
    m_pszMoreInfoUrl = NULL;
}

PluginHandler::Plugin::~Plugin()
{
    if (m_pszDescription)
    {
	delete[] m_pszDescription;
	m_pszDescription = NULL;
    }

    if (m_pszCopyright)
    {
	delete[] m_pszCopyright;
	m_pszCopyright = NULL;
    }

    if (m_pszMoreInfoUrl)
    {	
	delete[] m_pszMoreInfoUrl;
	m_pszMoreInfoUrl = NULL;
    }

    if (m_pszDllName)
    {
	delete[] m_pszDllName;
	m_pszDllName = NULL;
    }

    if (m_pErrorMessages)
    {
	m_pErrorMessages->Release();
    }

#if defined (_WIN32)
    DeleteCriticalSection(&m_critSec);
#endif

}

ULONG32
PluginHandler::Plugin::AddRef()
{
	if (m_pPluginDLL)
	{
	    m_pPluginDLL->AddRef();
	}
	return InterlockedIncrement(&m_lRefCount);
}

ULONG32
PluginHandler::Plugin::Release()
{
    if (m_pPluginDLL)
    {
	m_pPluginDLL->Release();
    }
    
	if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

// This does NOTHING!
void
PluginHandler::Plugin::ReleaseInstance()
{
}


PluginHandler::Errors
PluginHandler::Plugin::GetInstance(IUnknown** ppInstance)
{
    Errors	result = NO_ERRORS;

#if defined (_WIN32)
    EnterCriticalSection(&m_critSec);
#endif
    // Set this to NULL for case of error.
    *ppInstance = NULL;
    
    if (!m_pPluginDLL)
    {
        result = BAD_DLL;
        goto cleanup;
    }

    if (!m_pPluginDLL->m_bLoaded)
    {
	if (NO_ERRORS != (result=m_pPluginDLL->Load()))
	{
	    result = BAD_DLL;
	    goto cleanup;
	}
    }

    if (m_pPluginDLL->m_has_factory)
    {
	if (HXR_OK != m_pPluginDLL->m_pPluginFactory->GetPlugin(m_nPluginIndex,ppInstance))
        {
	    result = CREATE_INSTANCHXR_FAILURE;
	    goto cleanup;
	}
    }
    else
    {
	// XXX Henry : right now we only handle single interface   
	if (HXR_OK != m_pPluginDLL->m_fpCreateInstance(ppInstance))
	{
	    result = CREATE_INSTANCHXR_FAILURE;
	    goto cleanup;
	}
    }

cleanup:   

#if defined (_WIN32)
    LeaveCriticalSection(&m_critSec);
#endif

    return result;
}

PluginHandler::Errors
PluginHandler::Plugin::Load()  // I guess this should be inline...	
{
	return m_pPluginDLL->Load();
}

PluginHandler::Errors
PluginHandler::Plugin::Init(IUnknown* pContext)
{
    Errors	result = NO_ERRORS;
    IHXPlugin*	pPlugin = NULL;
    IUnknown*	pUnknown = NULL;

    const char*	pszDescription = NULL;
    const char* pszCopyright = NULL;
    const char* pszMoreInfoUrl = NULL;
    ULONG32	ulVersionNumber = 0;

    result = m_pPluginDLL->Load();
    if (NO_ERRORS != result)
    {
	goto cleanup;
    }

    result = GetInstance(&pUnknown);
    if (NO_ERRORS != result)
    {
	goto cleanup;
    }

    if (HXR_OK != pUnknown->QueryInterface(IID_IHXPlugin, (void**) &pPlugin))
    {
	result = BAD_PLUGIN;
	goto cleanup;
    }

    if (HXR_OK != pPlugin->GetPluginInfo(m_load_multiple, pszDescription, 
				       pszCopyright, pszMoreInfoUrl, ulVersionNumber))
    {
        result = BAD_PLUGIN;
	goto cleanup;
    }

    if (m_load_multiple == FALSE)
    {
	m_generic = TRUE;
    }
    else
    {
	IHXGenericPlugin* pGeneric = 0;
    	if (HXR_OK == pUnknown->QueryInterface(IID_IHXGenericPlugin,
	    (void**) &pGeneric))
	{
	    pGeneric->IsGeneric(m_generic);
	    HX_RELEASE(pGeneric);
	}
	else
	{
	    m_generic = FALSE;
	}
    }

    m_pszDescription = new char[strlen(pszDescription)+1];
    strcpy(m_pszDescription, pszDescription);

    m_pszCopyright = new char[strlen(pszCopyright)+1];
    strcpy(m_pszCopyright, pszCopyright);

    m_pszMoreInfoUrl = new char[strlen(pszMoreInfoUrl)+1];
    strcpy(m_pszMoreInfoUrl, pszMoreInfoUrl);

cleanup:
    
    if (pPlugin)
    {
	pPlugin->Release();
	pPlugin = NULL;
    }

    if (pUnknown)
    {
	pUnknown->Release();
	pUnknown = NULL;
    }

    return result;
}

void
PluginHandler::Plugin::GetPluginInfo(char** ppszDllPath,
				     char** ppszDescription,
				     char** ppszCopyright,
				     char** ppszMoreInfo,
				     BOOL*  pbMultiple)
{
    *ppszDllPath = m_pszDllName;
    *ppszDescription = (char*) m_pszDescription;
    *ppszCopyright = (char*) m_pszCopyright;
    *ppszMoreInfo = (char*) m_pszMoreInfoUrl;
    *pbMultiple = m_load_multiple;

    return;
}

const char* 	PH_BASE_STR 			= "FSMount.";
const UINT32 	PH_BASE_LEN			= 8;
const char* 	PH_ELEM_STR			= "elem";
const UINT32 	PH_ELEM_LEN 			= 4;
const UINT32 	PH_MAX_INT32_AS_STRING_LEN	= 20;
const UINT32 	PH_MAX_MEMBER_LEN 		= 20;

void
PluginHandler::InitMountPoints()
{
    IHXBuffer*			mount_point = 0;
    IHXBuffer*			real_short_name = 0;
    const char*			short_name = 0;
    UINT32              uSearchOrder = 0;
    IHXValues*			options = 0;
    int				which = 0;
    


#if 0
    base_string = new char[PH_BASE_LEN + PH_ELEM_LEN + 
			   PH_MAX_INT32_AS_STRING_LEN + PH_MAX_MEMBER_LEN];
    strcpy(base_string, PH_BASE_STR);
    end_of_base = base_string + PH_BASE_LEN;
#endif
    
    IHXRegistry* pRegistry;
    IHXValues* pNameList;

    if(HXR_OK != m_pContext->QueryInterface(IID_IHXRegistry,
					    (void**)&pRegistry))
    {
	return;
    }
    
    const char* pMountPointString = "config.FSMount";
    
    for (which = 0; which < 2; which++)
    {

	if(HXR_OK != pRegistry->GetPropListByName(pMountPointString, pNameList))
	{
	    pRegistry->Release();
	    return;
	}

	HX_RESULT res;
	const char* plugName;
	UINT32 plug_id;

	res = pNameList->GetFirstPropertyULONG32(plugName, plug_id);
	while(res == HXR_OK)
	{
	    HXPropType plugtype = pRegistry->GetTypeById(plug_id);
	    if(plugtype != PT_COMPOSITE)
		res = HXR_FAIL;
	    else
	    {
		short_name = strrchr(plugName, '.');
		if(!short_name)
		    short_name = plugName;
		else
		    short_name++;

		IHXValues* pPropList;
		if(HXR_OK == pRegistry->GetPropListById(plug_id, pPropList))
		{
		    const char* propName;
		    UINT32 prop_id;
		    options = new CHXHeader();
		    options->AddRef();
		    IHXBuffer* pLongName = new CHXBuffer();
		    pLongName->AddRef();
		    pLongName->Set((const unsigned char*)plugName,
				    strlen(plugName) + 1);
		    options->SetPropertyBuffer("LongName", pLongName);
		    pLongName->Release();
		    pLongName = 0;
		    res = pPropList->GetFirstPropertyULONG32(propName, prop_id);
		    while(res == HXR_OK)
		    {
			HXPropType type = pRegistry->GetTypeById(prop_id);
			const char*propSubName = strrchr(propName, '.') + 1;
			switch(type)
			{
			    case PT_INTEGER:
			    {
				INT32 val;
				if(HXR_OK == pRegistry->GetIntById(prop_id, val))
				{
				    options->SetPropertyULONG32(propSubName, val);
				}
				break;
			    }
			    case PT_STRING:
			    {
				IHXBuffer* pBuffer;
				if(HXR_OK == pRegistry->GetStrById(prop_id,
								   pBuffer))
				{
				    options->SetPropertyBuffer(propSubName,
								pBuffer);
				    pBuffer->Release();
				}
				break;
			    }
			    case PT_BUFFER:
			    {
				IHXBuffer* pBuffer;
				if(HXR_OK == pRegistry->GetBufById(prop_id,
								   pBuffer))
				{
				    options->SetPropertyBuffer(propSubName,
							       pBuffer);
				    pBuffer->Release();
				}
				break;
			    }
			    default:
				break;
			}
			res = pPropList->GetNextPropertyULONG32(propName, prop_id);
		    }
		    res = HXR_OK;
		}
		
		if(HXR_OK == options->GetPropertyBuffer("MountPoint",
							 mount_point))
		{
		    if(HXR_OK == options->GetPropertyBuffer("ShortName",
							    real_short_name))
		    {
			short_name = (const char*) real_short_name->GetBuffer();
		    }
			
			// Check if "MountPointSearchOrder" property is defined or not.
			// If not, add default value "1".
			if(HXR_OK != options->GetPropertyULONG32("MountPointSearchOrder",
								uSearchOrder))
		    {
				options->SetPropertyULONG32("MountPointSearchOrder", 1);
		    }
			
		    switch (which)
		    {
			case 0:
			    m_file_sys_handler->AddMountPoint(short_name,
				    (const char*)mount_point->GetBuffer(),
				    options);

			    m_allowance_handler->AddMountPoint(real_short_name,
				    (const char*)mount_point->GetBuffer(),
				    options);
			    break;
			    
			case 1:
			    m_data_convert_handler->AddMountPoint(short_name,
				    (const char*)mount_point->GetBuffer(),
				    options);
			    break;
		    }

		    if(real_short_name)
		    {
			real_short_name->Release();
			real_short_name = 0;
		    }
		    mount_point->Release();
		}
		res = pNameList->GetNextPropertyULONG32(plugName, plug_id);
	    }
	}
	pNameList->Release();
	pMountPointString = "config.DataConvertMount";
    }
    pRegistry->Release();
}


///////////////////////////////////////////////////////////////////////////////
//
// PluginHandler::PluginFactory
//
// This class is responsible for managing the list of DLLs
// It can add to list 
// Empty the list
// Retreive the list from the registery
// Save the list to the registery.
// And get a DLL from a name...
//
///////////////////////////////////////////////////////////////////////////////


PluginHandler::PluginFactory::PluginFactory()
{
    m_pPluginDlls = new CHXSimpleList();
}

PluginHandler::PluginFactory::~PluginFactory()
{
    Clear();
    HX_DELETE(m_pPluginDlls);
}

PluginHandler::Errors
PluginHandler::PluginFactory::StoreToRegistry(IHXPreferences** ppRegistry)
{
    Errors		    result = NO_ERRORS;
    int			    j = 0;
    int			    totallen=0;
    char*		    pszData = NULL;
    char*		    pszInfo = NULL;
    char*		    pszTemp = NULL;
    CHXSimpleList*	    pList = NULL;
    PluginDLL*		    pPluginDll = NULL;
    IHXBuffer*		    pBuffer = NULL;
    CHXSimpleList::Iterator i;
    char		    seperator[2];
    PluginDLL*		    temp;

    if (!ppRegistry)
    {
	result = BAD_REGISTRY_HANDLE;
	goto cleanup;
    }

    if (!*ppRegistry)
    {
	result = BAD_REGISTRY_HANDLE;
	goto cleanup;
    }
	
	// Scan the list to find out how long the destination string will be
    for (i = m_pPluginDlls->Begin(); i != m_pPluginDlls->End(); ++i)
    {
	totallen+=strlen( ((PluginDLL*) (*i))->m_pszFileName)+1;
    }

    seperator[0]=PLUGIN_ENTRY_SEPARATOR;
    seperator[1]=0;
    // parse/collect all the plugin info.
    pszInfo = new char[totallen+4];		// one extra for good luck
    if (m_pPluginDlls->Begin()!=m_pPluginDlls->End())
    {
	strcpy(pszInfo, "{");
	i = m_pPluginDlls->Begin(); 
	temp = (PluginDLL*) (*i);
	strcat(pszInfo, temp->m_pszFileName);
	++i;
	for (; i != m_pPluginDlls->End(); ++i)
	{
    	    strcat(pszInfo, seperator);
	    strcat(pszInfo, ((PluginDLL*) (*i))->m_pszFileName);
	}
	
	strcat(pszInfo,"}");
    }
    else
    {
	strcpy(pszInfo,"{}");
    }
    if (pszInfo)
    {
	if (!(pBuffer = new CHXBuffer()))
	{
	    result = MEMORY_ERROR;
	    goto cleanup;
	}

	pBuffer->AddRef();
    
	pBuffer->Set((UCHAR*)pszInfo, strlen(pszInfo)+1);

    (*ppRegistry)->WritePref(REGKEY_DLLLIST, pBuffer);
	
	pBuffer->Release();
    }

cleanup:
    if (pszInfo)
    {
	delete[] pszInfo;
	pszInfo = NULL;
    }

    return result;
}

PluginHandler::Errors
PluginHandler::PluginFactory::ReadFromRegistry(IHXPreferences* pRegistry)
{
    Errors		    result = NO_ERRORS;
    int			    j = 0;
    int			    totallen=0;
    char*		    pszData = NULL;
    char*		    pszInfo = NULL;
    char*		    pszTemp = NULL;
    char*		    token;
    CHXSimpleList*	    pList = NULL;
    PluginDLL*		    pPluginDll = NULL;
    IHXBuffer*		    pBuffer = NULL;
    CHXSimpleList::Iterator i;
    char		    seperators[4];

    if (!pRegistry)
    {
	result = BAD_REGISTRY_HANDLE;
	goto cleanup;
    }

    // clear the existing ones
    Clear();

    if (HXR_OK != pRegistry->ReadPref(REGKEY_DLLLIST, pBuffer))
    {
	result = PLUGIN_NOT_FOUND;
	goto cleanup;
    }

    pszInfo = (char*) pBuffer->GetBuffer();

    // create plugins based on the info

    seperators[0]=PLUGIN_ENTRY_SEPARATOR;
    seperators[1]='{';
    seperators[2]='}';
    seperators[3]=0;

    token = strtok( pszInfo, seperators);

    while( token != NULL )
    {
        pPluginDll = new PluginDLL (token);
//        m_pPluginDlls->AddTail(pPluginDll);
	Add(pPluginDll);
        token = strtok( NULL, seperators);
    }

cleanup:
    pBuffer->Release();
    
    return result;
}

PluginHandler::PluginDLL* PluginHandler::PluginFactory::FindDLLFromName(char* pFilename)
{
    CHXSimpleList::Iterator i;
    char*   blah;

    for (i = m_pPluginDlls->Begin(); i != m_pPluginDlls->End(); ++i)
    {
	blah = ((PluginDLL*) (*i))->m_pszFileName;
	if (!strcasecmp( blah, pFilename))
	{
	    return (PluginDLL*) (*i);
	}
    }
    return (PluginDLL*) 0;
}


PluginHandler::Errors
PluginHandler::PluginFactory::Add(PluginDLL* pPlugin)
{
    m_pPluginDlls->AddTail(pPlugin);
    pPlugin->AddRef();
    return NO_ERRORS;
}

void
PluginHandler::PluginFactory::Clear()
{
    CHXSimpleList::Iterator i;
    PluginDLL* pPlugin = NULL;
    for(i = m_pPluginDlls->Begin(); i != m_pPluginDlls->End(); ++i)
    {
	((PluginDLL*) (*i))->Release();
    }

    m_pPluginDlls->RemoveAll();
}


///////////////////////////////////////////////////////////////////////////////
//
// PluginHandler::PluginDLL
//
///////////////////////////////////////////////////////////////////////////////

PluginHandler::PluginDLL::PluginDLL(const char* pszFileName, IHXErrorMessages* pEM) :
	m_pErrorMessages(pEM)
    ,	m_lRefCount(0)
    ,	m_pDllAccess(NULL)
    ,	m_fpShutdown(NULL)
    ,	m_pPluginFactory(NULL)
    ,	m_has_factory(FALSE)
    ,	m_NumOfPlugins(0)
    ,	m_bLoaded(FALSE)
{
	m_pszFileName= new_string(pszFileName);
}


PluginHandler::PluginDLL::~PluginDLL()	
{
    if (m_has_factory && m_pPluginFactory)
    {
	m_pPluginFactory->Release();
    }
    
    if (m_pDllAccess)
    {
	if (m_fpShutdown)
	{
	    m_fpShutdown();
	    m_fpShutdown	= NULL;
	}

	m_pDllAccess->close();
	delete m_pDllAccess;		
	m_pDllAccess = NULL;
    }

    if (m_pszFileName)
    {
	delete [] m_pszFileName;
	m_pszFileName = NULL;
    }

    m_fpCreateInstance	= NULL;
    m_fpShutdown	= NULL;
}


PluginHandler::Errors
PluginHandler::PluginDLL::Load()	
{								
    Errors			result = NO_ERRORS;
    IUnknown*		pInstance;

    if (m_pDllAccess)
    {
	return result;  
    }
	
    m_bLoaded = TRUE;
    m_pDllAccess = new DLLAccess();

    if (DLLAccess::DLL_OK != m_pDllAccess->open(m_pszFileName))
    {
    	result = CANT_OPEN_DLL;
	
	char pErrorTemp[2048];
	sprintf (pErrorTemp, "%-35s %s", m_pszFileName,	m_pDllAccess->getErrorString());
	if (m_pErrorMessages)
	    m_pErrorMessages->Report(HXLOG_DEBUG, 0, 0, pErrorTemp, 0);
	
	goto cleanup;
    }
	
    m_fpCreateInstance = (FPCREATEINSTANCE) m_pDllAccess->getSymbol(HXCREATEINSTANCESTR);
    if (NULL == m_fpCreateInstance)
    {
	result = BAD_DLL;
	char pErrorTemp[2048];
	sprintf (pErrorTemp, "%-35s %s", m_pszFileName, "NO HXCreateInstance");
	if (m_pErrorMessages)
	    m_pErrorMessages->Report(HXLOG_DEBUG, 0, 0, pErrorTemp, 0);
	goto cleanup;
    }

    m_fpShutdown    = (FPSHUTDOWN) m_pDllAccess->getSymbol("HXShutdown");

    // Now we will test to see if the DLL contains multiple Plugins.
	
    if (HXR_OK != m_fpCreateInstance(&pInstance))
    {
	result = CREATE_INSTANCHXR_FAILURE;
	char pErrorTemp[2048];
	sprintf (pErrorTemp, "%-35s %s", m_pszFileName, "HXCreateInstance Failure");
	if (m_pErrorMessages)
	    m_pErrorMessages->Report(HXLOG_DEBUG, 0, 0, pErrorTemp, 0);
	goto cleanup;
    }
	
    if (HXR_OK != pInstance->QueryInterface
	(IID_IHXPluginFactory, (void**) &m_pPluginFactory))
    {
	// Nope we are a normal Plugin. we mus set a number of 
	m_has_factory = FALSE;
	m_NumOfPlugins =1;		//not really necessary. 
	pInstance->Release(); 
    }
    else
    {
	m_has_factory = TRUE;
	m_NumOfPlugins = m_pPluginFactory->GetNumPlugins();
    }


cleanup:

    if (NO_ERRORS != result)
    {
	if (m_pDllAccess)
	{
	    if (m_fpShutdown)
	    {
		m_fpShutdown();
		m_fpShutdown	= NULL;
	    }

	    m_pDllAccess->close();
	    delete m_pDllAccess;
	    m_pDllAccess = NULL;
	}
    }
    return result;
}


ULONG32
PluginHandler::PluginDLL::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

ULONG32
PluginHandler::PluginDLL::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }
	
    delete this;
    return 0;
}


///////////////////////////////////////////////////////////////////////////////
//
// PluginHandler::CheckDirectory
//
///////////////////////////////////////////////////////////////////////////////

PluginHandler::Errors PluginHandler::Stat(const char* pszFilename, struct stat* pStatBuffer)
{
    CHXString	strFileName;

    memset(pStatBuffer,0,sizeof(*pStatBuffer));
	
#ifndef _MACINTOSH
	if(stat((const char*)pszFilename, pStatBuffer) < 0)
	    return CANT_OPEN_DLL;
#else	    // EEEK! Mac is WAY more complicated!
    CHXDataFile*	    pDataFile=NULL; 

    pDataFile = CHXDataFile::Construct(m_pContext);
    if (!pDataFile)
	    return MEMORY_ERROR;
    if (pDataFile->Open(pszFilename,O_RDONLY)==HXR_OK)
	return CANT_OPEN_DLL;

    int pos = pDataFile->Tell(); 
    pDataFile->Seek(0,SEEK_END);
    pStatBuffer->st_size = pDataFile->Tell();
    delete pDataFile;
    pDataFile = NULL;
#endif

    return NO_ERRORS ;
}

// NOTE: returns a string of length 32 BYTES. in hex format, just so it can 
// print nicely to the registery...
PluginHandler::Errors PluginHandler::ChkSumDirectory(char * pszPluginDir, char* MD5Result)
{
    UCHAR				tempbuf[16];
    md5_state_t				MD5_data;
    Errors				result		    = NO_ERRORS;
    struct stat				stat_stuct;
    CFindFile*				pFileFinder =NULL;
    char*				pszDllName = NULL;
    int					i=0;
#ifdef _WINDOWS
    const char*	pszFileExtension = "*.dll";
#elif _UNIX
    const char*	pszFileExtension = "*.so*";
#elif _MACINTOSH
    const char*	pszFileExtension = "*.dll";
#endif

    md5_init(&MD5_data);

    if (NULL == pszPluginDir)
    {
	return CANT_DETERMINE_PLUGIN_DIR;
    }

    ;

    if (!(pFileFinder = CFindFile::CreateFindFile(pszPluginDir, 0, pszFileExtension)))
    {
	result=	CANT_OPEN_PLUGIN_DIR;
	goto cleanup;
    }

    // collect plugin info.
    pszDllName = pFileFinder->FindFirst();
    while (pszDllName)
    {
	pszDllName = pFileFinder->GetCurFilePath();
	md5_append(&MD5_data,(UCHAR*)pszDllName,strlen(pszDllName));
	if (NO_ERRORS!=Stat(pszDllName, &stat_stuct))
	{
	    result=CANT_OPEN_DLL;
	    goto cleanup;
	}
	md5_append(&MD5_data,(UCHAR*)&(stat_stuct),sizeof(stat_stuct));
	pszDllName = pFileFinder->FindNext();
    }

    md5_finish(tempbuf, &MD5_data);
    for(;i<16;i++)
    {
	MD5Result[i*2]= ((tempbuf[i] >> 4) > 9 ) ? (tempbuf[i] >> 4) + 55 : (tempbuf[i] >> 4) +48;
	MD5Result[i*2+1]= ((tempbuf[i] & 15) > 9 ) ? (tempbuf[i] & 15) + 55 : (tempbuf[i] & 15) +48;
    }
    MD5Result[32]=0;

cleanup:
    if (pFileFinder)
    {
	delete pFileFinder;
	pFileFinder = 0;
    }
    
    return result;
}


PluginHandler::Errors PluginHandler::IsDirValid(IHXPreferences* pRegistry)
{
    IHXBuffer*				pBuffer = NULL;
    char *				pszPluginDir;
    char				MD5Result[33]; 
    Errors				result		    = NO_ERRORS;
    char*				pszInfo;

    if (!pRegistry)
    {
	return BAD_REGISTRY_HANDLE;
    }

    pszPluginDir = GetPluginDir();

    if (NULL == pszPluginDir)
    {
	result = CANT_DETERMINE_PLUGIN_DIR;
	goto cleanup;
    }

    if (HXR_OK != pRegistry->ReadPref(REGKEY_PLUGINHASH, pBuffer))
    {
        result = INVALID_SHORT_NAME;
	goto cleanup;
    }

    pszInfo = (char*) pBuffer->GetBuffer();
    if ( (result=ChkSumDirectory(pszPluginDir, MD5Result))!=HXR_OK)
    {
	goto cleanup;
    }

    if (strcmp(MD5Result,pszInfo))
    {
	result = PLUGIN_DIR_NOT_SAME;
	goto cleanup;
    }
    else
    {
	result = result;
	goto cleanup;
    }

cleanup:

    if (pszPluginDir)
    {
	delete[] pszPluginDir;
	pszPluginDir = NULL;
    }
    if (pBuffer)
    {
	pBuffer->Release();
	pBuffer = 0;
    }

    return result;


}

PluginHandler::Errors PluginHandler::SaveDirChkSum(IHXPreferences** ppRegistry)
{
    char *				pszPluginDir;
    char				MD5Result[33]; 
    IHXBuffer*				pBuffer  = NULL;
    PluginHandler::Errors		result =NO_ERRORS;

    if (!ppRegistry)
    {
	return BAD_REGISTRY_HANDLE;
    }

    if (!*ppRegistry)
    {
	return BAD_REGISTRY_HANDLE;
    }

    pszPluginDir = GetPluginDir();
    if (NULL == pszPluginDir)
    {
	result = CANT_DETERMINE_PLUGIN_DIR;
	goto cleanup;
    }

    if (ChkSumDirectory(pszPluginDir, MD5Result)!=HXR_OK)
    {
	result = CANT_OPEN_PLUGIN_DIR;
	goto cleanup;
    }

    if (!(pBuffer = new CHXBuffer()))
    {
	result = MEMORY_ERROR;
	goto cleanup;
    }

    pBuffer->AddRef();
    
    pBuffer->Set((UCHAR*)MD5Result, strlen(MD5Result)+1);

    (*ppRegistry)->WritePref(REGKEY_PLUGINHASH, pBuffer);

    pBuffer->Release();
    

cleanup:

    if (pszPluginDir)
    {
	delete[] pszPluginDir;
	pszPluginDir = NULL;
    }

    return result;
}
    
