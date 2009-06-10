/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: plghand2.cpp,v 1.62 2009/06/03 22:11:51 ping Exp $
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


/****************************************************************************
 *
 *
 *  Plugin information are stored into the registry in the following format:
 *
 *      File Format Plugins: {dllpath;description;copyright;moreinfo;loadmultiple;mimetype1|mimetype2;extension1|extension2}{ ... }
 *                           ---------------------------------------------------------------------------------------------------
 *                                                              One Plugin
 *
 *      File System Plugins: {dllpath;description;copyright;moreinfo;loadmultiple;protocol;shortname}{ ... }
 *                           ----------------------------------------------------------------------------
 *                                                              One Plugin
 *
 *      Renderer Plugins:    {dllpath;description;copyright;moreinfo;loadmultiple;mimetype1|mimetype2}{ ... }
 *                           -----------------------------------------------------------------------------
 *                                                              One Plugin
 *
 *      Broadcast Plugins:   {dllpath;description;copyright;moreinfo;loadmultiple;type}{ ... }
 *                           --------------------------------------------------------------
 *                                                              One Plugin
 *
 *      Stream Description Plugins: {dllpath;description;copyright;moreinfo;loadmultiple;mimetype}{ ... }
 *                           ----------------------------------------------------------------------------
 *                                                              One Plugin
 *
 *      Allowance Plugins:   {dllpath;description;copyright;moreinfo;loadmultiple}{ ... }
 *                           ---------------------------------------------------------
 *                                                              One Plugin
 *
 *      Misc. Plugins:       {dllpath;description;copyright;moreinfo;loadmultiple}{ ... }
 *                           ---------------------------------------------------------
 *                                                              One Plugin
 *
 *      Plugins:             {dllpath;description;copyright;moreinfo;loadmultiple}{ ... }
 *                           ---------------------------------------------------------
 *                                                              One Plugin
 *
 */

#include "hxtypes.h"

#ifdef _WINDOWS
#include <windows.h>
#include <ctype.h>
#endif
#ifdef _MACINTOSH
#include <ctype.h>
#include "filespec.h"
#include "filespecutils.h"
#endif

#if defined _UNIX
#include <stdlib.h>
#include <sys/param.h>
#define _MAX_PATH       MAXPATHLEN
#elif defined (_MACINTOSH)
#include <stdlib.h>
#include "fullpathname.h"
#include "chxdataf.h"
#ifdef _MAC_MACHO
#include <sys/stat.h>
#else
#include <stat.h>
#endif
#include <fcntl.h>
#endif

#include "hlxclib/stdio.h"

#include "hlxclib/sys/stat.h"
#include "hxresult.h"
#include "hxassert.h"
#include "hxcom.h"
#include "ihxpckts.h"
#include "hxfiles.h"
#include "hxformt.h"
#include "hxfwrtr.h"
#include "hxrendr.h"
#include "hxprefs.h"
#include "hxplugn.h"
#include "hxdtcvt.h"
#include "hxphand.h"
//#include "hxmeta.h"
#include "hxsdesc.h"
#include "hxauth.h"
#include "hxallow.h"
#include "hxerror.h"
#include "hxcomm.h"
#include "hxengin.h"
#include "hxshtdn.h"
#include "hxplgns.h"
#include "hxmon.h"
#include "chxpckts.h"
#include "hxstring.h"
#include "hxslist.h"
#include "hxmap.h"
#include "hxstrutl.h"
#include "hxdir.h"
#include "findfile.h"
#include "dbcs.h"
#include "hxbdwdth.h"
#include "plghand2.h"
#include "chxuuid.h"
#include "md5.h"
#include "pckunpck.h"
#define HELIX_FEATURE_LOGLEVEL_NONE // uncomment to disable all logging in this file
#include "hxtlogutil.h"

#include "dllacces.h"
#include "dllpath.h"

#include "hxperf.h"
#include "rtsputil.h"

#include "hxver.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE
static const char HX_THIS_FILE[] = __FILE__;
#endif

#ifdef _STATICALLY_LINKED
#include "staticff.h"
#endif

#if defined(HELIX_FEATURE_PREFERENCES)
#include "hxprefs.h"
#include "hxprefutil.h"        
#endif /* HELIX_FEATURE_PREFERENCES */

/*
 * XXXND These are also defined in geminc/gemplatformdata.h
 */

#define NAMESPACE_SEPARATOR ':'

/*
 *  Win98 does not allow reg keys that are larger than 16k a pop. we used this value to break up into more manageable bites
 */

#define PREF_CACHE_SIZE 10000

/*
  Load each plug-in, read info, store in memory and update prefs
  This is only done if the plugin hash within the registery is not the same
  as the plugin hash of the plugins directory.
*/

const char* const Plugin2Handler::zm_pszValueSeperator = "|";
const char* const Plugin2Handler::zm_pszListStart       = "{";
const char* const Plugin2Handler::zm_pszListEnd = "}";
const char* const Plugin2Handler::zm_pszValueSeperator2= ",";

const char* const Plugin2Handler::zm_pszKeyNameRegKey = "~KeyNames~";
const char* const Plugin2Handler::zm_pszRegKeySeperator = "\\";

#if !defined(HELIX_CONFIG_NOSTATICS)
HXBOOL Plugin2Handler::zm_bFasterPrefs = 0;
#else
const HXBOOL Plugin2Handler::zm_bFasterPrefs = 0;
#endif

const char* const Plugin2Handler::zm_pszFileExtension = OS_DLL_PATTERN_STRING;
const char* const Plugin2Handler::zm_pszDirectorySeperator = OS_SEPARATOR_STRING;
const char Plugin2Handler::zm_cDirectorySeperator = OS_SEPARATOR_CHAR;

#define PLUGINHANDLER_RESCAN_CCFPLUGIN_REGNAME  "MountPoints\\ReScanCCFPlugins"

// XXXHP - temporary changes to track PR70528
#ifdef REALPLAYER_PLUGIN_HANDLER_RESEARCH_
inline FILE* OpenErrorLog_ ()
{
    static char const* const realPlayerErrorsLog = "rperrors.log";
    static char const* const appendMode = "a+";

    return fopen (realPlayerErrorsLog, appendMode);
}

inline void LogRegistryRegeneration_ (char const* pKey, IHXBuffer* pIEntry)
{
    FILE* const logStream = OpenErrorLog_ ();
    PRE_REQUIRE_VOID_RETURN (logStream);

    fprintf (logStream, "--------------- INVALID REGISTRY ENTRY: ----------------------------\n");
    fprintf (logStream, "Key: %s\n", pKey);
    fprintf (logStream, "\tData: [%s]\n", pIEntry->GetBuffer ());
    fprintf (logStream, "--------------------------------------------------------------------\n");
    fclose (logStream);
}

inline void LogCriticalError_ (char const* pErrorReport)
{
    FILE* const logStream = OpenErrorLog_ ();
    PRE_REQUIRE_VOID_RETURN (logStream);

    fprintf (logStream, "---------------- CRITICAL ERROR ------------------------------------\n");
    fprintf (logStream, "%s\n", pErrorReport);
    fprintf (logStream, "--------------------------------------------------------------------\n");
    fclose (logStream);
}
#else
inline void LogRegistryRegeneration_ (char const*, IHXBuffer*)
{
}
#endif

/**************************************************************************
 ****************************Plugin2Handler**********************************
 **************************************************************************/

IMPLEMENT_COM_CREATE_FUNCS( Plugin2Handler )

    BEGIN_INTERFACE_LIST_NOCREATE( Plugin2Handler )
    INTERFACE_LIST_ENTRY_SIMPLE( IHXCallback )
    INTERFACE_LIST_ENTRY_SIMPLE( IHXPluginEnumerator )
    INTERFACE_LIST_ENTRY_SIMPLE( IHXPluginReloader )
    INTERFACE_LIST_ENTRY_SIMPLE( IHXPlugin2Handler )
    INTERFACE_LIST_ENTRY_SIMPLE( IHXPlugin2HandlerEnumeratorInterface )
    INTERFACE_LIST_ENTRY_SIMPLE( IHXPluginHandler3 )
    INTERFACE_LIST_ENTRY_SIMPLE( IHXPluginDatabase )
    END_INTERFACE_LIST

inline HX_RESULT Plugin2Handler::VerifyChecksum_ (const char* pData)
{
    REQUIRE_RETURN (pData, HXR_INVALID_PARAMETER);

    static const char endEntry = '}';
    static const int error = -1;

    // find the last end-marker of the last entry.
    const CHXString data = pData;
    const int lastEntry = data.ReverseFind (endEntry);
    REQUIRE_RETURN_QUIET (lastEntry != error, HXR_FAIL);

    // the checksum should be equal to the position of the end of the last entry plus 1.
    const int calculatedChecksum = lastEntry + 1;

    // read in the actual checksum.
    const CHXString checksumString = pData + calculatedChecksum;
    const int actualChecksum = ::atoi (checksumString);

    return actualChecksum == calculatedChecksum ? HXR_OK : HXR_FAIL;
}

Plugin2Handler::Plugin2Handler() :
    m_pPluginDir( NULL )
    ,   m_pPreferences(NULL)
    ,   m_pErrorMessages(NULL)
    ,   m_pContext(NULL)
    ,   m_nCacheSizeBites( (1<<24) )
    ,   m_pIScheduler(NULL)
    ,   m_hScheduler( 0 )
    ,   m_bStatDllsOnStartup(TRUE)
    ,   m_bReScanCCFPlugins(TRUE)
{
#if defined(_DEBUG) && defined(_WINDOWS)
    char szDbgStr[256];
    sprintf(szDbgStr, "CON Plugin2Handler[%p]\n", this);
    OutputDebugString(szDbgStr);
#endif
    HXLOGL4(HXLOG_GENE, "CON Plugin2Handler[%p]", this);
}

Plugin2Handler::~Plugin2Handler()
{
#if defined(_DEBUG) && defined(_WINDOWS)
    char szDbgStr[256];
    sprintf(szDbgStr, "DES Plugin2Handler[%p]\n", this);
    OutputDebugString(szDbgStr);
#endif
    HXLOGL4(HXLOG_GENE, "DES Plugin2Handler[%p]", this);
    // Make sure Close() got called
    if( m_pContext )
    {
        Close();
    }
}

STDMETHODIMP Plugin2Handler::Init(IUnknown* pContext)
{
    HX_RESULT result = HXR_FAIL;

    if( SUCCEEDED( result = RegisterContext( pContext ) ) )
    {
        //result = ReadFromRegistry();
    }
    
    return result;
}


STDMETHODIMP_(ULONG32) Plugin2Handler::GetNumOfPlugins2()
{
    return m_PluginList.GetCount();
}


STDMETHODIMP  Plugin2Handler::GetPluginInfo (UINT32 unIndex,
                                             REF(IHXValues*) /*OUT*/ pValues)
{
    HX_RESULT retVal = HXR_FAIL;
    LISTPOSITION pPos = m_PluginList.FindIndex(unIndex);
    if (pPos)
    {
        Plugin2Handler::Plugin* pPlugin = (Plugin2Handler::Plugin*) m_PluginList.GetAt(pPos);
        if (pPlugin)
        {
            retVal = pPlugin->GetPluginInfo(pValues);
            pValues->AddRef();
        }
    }
    return retVal;
}



// ---------------------------------------------------------- IHXCallback

// **********************************************
STDMETHODIMP
Plugin2Handler::Func( THIS )
{
    if ( !m_pIScheduler || !m_hScheduler )
    {
        return( HXR_UNEXPECTED );
    }

    HX_RESULT outResult = HXR_OK;

    // Our timer is infinite.
    m_hScheduler = m_pIScheduler->RelativeEnter( this, kPingDuration );
    if ( !m_hScheduler )
    {
        outResult = HXR_FAIL;
    }

    UnloadDeadDLLs();

    return( outResult );
}

// **********************************************
// This function is called periodically when the plugin handler
// object is pinged by the scheduler.
void Plugin2Handler::UnloadDeadDLLs( void )
{
    HX_LOG_BLOCK( "Plugin2Handler::UnloadDeadDLLs" );

    LISTPOSITION posCanUnload = m_CanUnload2DllList.GetHeadPosition();
    while ( posCanUnload )
    {
        // Save off current position for delete.
        LISTPOSITION posAt = posCanUnload;

        // Get current item, and increment position.
        Plugin2Handler::PluginDLL* pPluginDLLCanUnload = (Plugin2Handler::PluginDLL*) m_CanUnload2DllList.GetNext( posCanUnload );
        if ( pPluginDLLCanUnload )
        {
            pPluginDLLCanUnload->Unload( TRUE );  // TRUE: "safe" unload
        }
    }
}



/*
  ReconnectDLL()

  This replaces one PluginDLL in m_PluginDLLList with a new instance.
  It removes any plugins that refered to the old DLL.
*/
void Plugin2Handler::ReconnectDLL(const char* pszDLLName, Plugin2Handler::PluginDLL* pNewDLL)
{
    HX_LOG_BLOCK( "Plugin2Handler::ReconnectDLL" );

    // before we add this to the tail we must check to see if this
    // plugin is already within the list....

    Plugin2Handler::PluginDLL*  pOldPluginDll   = NULL;
    LISTPOSITION                pPos            = NULL;

    if (m_FileNameMap.Lookup(pszDLLName, (void*&)pOldPluginDll))
    {
        pPos = m_PluginDLLList.Find(pOldPluginDll);
        if (pPos)
        {
            m_PluginDLLList.RemoveAt(pPos);
        }

        // now see who the heck was connected to this pluginDLL
        pPos = m_PluginList.GetHeadPosition();
        while( pPos )
        {
            // Save off current position for delete
            LISTPOSITION posAt = pPos;

            // Get current item, and increment position
            Plugin2Handler::Plugin* pPlugin = (Plugin2Handler::Plugin*) m_PluginList.GetNext( pPos );

            // If this plugin belongs to the old DLL, remove it.
            if( pPlugin && pPlugin->GetDLL() == pOldPluginDll )
            {
                // Delete from the saved position
                m_PluginList.RemoveAt( posAt );
                HX_RELEASE( pPlugin );
            }
        }

        HX_RELEASE(pOldPluginDll);
    }

    m_PluginDLLList.AddTail(pNewDLL);

    m_FileNameMap.SetAt(pszDLLName, pNewDLL);
}

/*
  LoadDLL()

  This is called during a Refresh() (which is deprecated)
  or if we determine that the DLL is dirty and needs to be updated
*/
Plugin2Handler::Errors Plugin2Handler::LoadDLL( char* pszDllName,
                                                PluginMountPoint* pMountPoint )
{
    Errors                      result      = NO_ERRORS;
    UINT32                      i           = 0;
    struct stat                 stat_stuct;
    IHXBuffer*                  pPathBuffer = pMountPoint->Path();

    // Make sure there is no path in the pszDllName
    HX_ASSERT( !strrchr(pszDllName, Plugin2Handler::zm_cDirectorySeperator) );


    Plugin2Handler::PluginDLL*  pPluginDll  = NULL;
    if( !( pPluginDll = new Plugin2Handler::PluginDLL( pszDllName, pMountPoint, this ) ) )
    {
        return MEMORY_ERROR;
    }

    pPluginDll->AddRef();

    CHXString sFileWithPath = pPathBuffer->GetBuffer();
    UINT32 len = sFileWithPath.GetLength();
    if(len &&
       sFileWithPath.GetAt(len - 1) != Plugin2Handler::zm_cDirectorySeperator)
        sFileWithPath += Plugin2Handler::zm_cDirectorySeperator;
    sFileWithPath += pszDllName;

    if (NO_ERRORS==Stat(sFileWithPath, &stat_stuct))
    {
        pPluginDll->SetFileSize((INT32)stat_stuct.st_size);
    }

    result = pPluginDll->Load(m_pContext);
    if (NO_ERRORS != result )
    {
        goto cleanup;
    }

    // Set the hash
    if (pPathBuffer)
    {
        IHXBuffer* pNewChecksum = ChecksumFile( pszDllName, pPathBuffer );
        if (pNewChecksum)
        {
            HX_RELEASE(pPathBuffer);
            pPluginDll->SetHash((char*)pNewChecksum->GetBuffer());
            HX_RELEASE(pNewChecksum);
        }
    }

    // Remove this DLL from the list of supported DLL (based on GUIDs)
    RemoveDLLFromGUIDSupportLists(pszDllName);

    // ReconnectDLL() replaces one PluginDLL object with another PluginDLL; however,
    // both of these PluginDLL objects refer to the same DLL.  This ensures that
    // only one PluginDLL object per DLL is in the m_PluginDLLList.
    ReconnectDLL(pszDllName, pPluginDll);

    for(i=0;i<pPluginDll->GetNumPlugins();i++)
    {
        Plugin* pPlugin = NULL;

        // create a new plugin object
        if (!(pPlugin = new Plugin(m_pContext)))
        {
            return MEMORY_ERROR;
        }

        // Setup plugin information
        pPlugin->AddRef();
        pPlugin->SetDLL(pPluginDll);
        pPlugin->SetIndex((UINT16)i);
        pPlugin->SetInfoNeedsRefresh(TRUE);

        IUnknown* pUnk = NULL;
        if( NO_ERRORS != pPlugin->GetPlugin( pUnk ) )
        {
            // This plugin doesn't work.  Delete it.
            HX_RELEASE( pPlugin );
        }
        else
        {
            IHXPluginNamespace* pPluginNamespace = NULL;
            if (SUCCEEDED(pUnk->QueryInterface(IID_IHXPluginNamespace, (void**) &pPluginNamespace)))
            {
                /*
                 * Memory for the IHXBuffer is allocated in the plugin
                 */
                IHXBuffer* pBuffer = NULL;
                if (SUCCEEDED(pPluginNamespace->GetPluginNamespace(pBuffer)))
                {
                    pPluginDll->SetNamespace(pBuffer);
                    HX_RELEASE(pBuffer);
                }
                HX_RELEASE(pPluginNamespace);
            }

            IHXComponentPlugin* pIIterator = NULL;
            if( SUCCEEDED( pUnk->QueryInterface( IID_IHXComponentPlugin, (void**) &pIIterator ) ) )
            {
                // We don't need this.
                HX_RELEASE( pPlugin );

                LoadPluginsFromComponentDLL( pPluginDll, pIIterator );
                HX_RELEASE( pIIterator );
            }
            else
            {
                IHXPlugin* pIHXPlugin;
                if( SUCCEEDED( pUnk->QueryInterface(IID_IHXPlugin, (void**)&pIHXPlugin ) ) )
                {
                    pPlugin->GetValuesFromDLL(pIHXPlugin);
                    m_PluginList.AddTail(pPlugin);
                    AddPluginToIndices(pPlugin);

                    // Print out some log info about the plugin we just loaded
                {
                    const char      *pDesc, *pCopy, *pURL;
                    ULONG32 ulVersionNumber = 0;
                    HXBOOL  junk;

                    pIHXPlugin->GetPluginInfo(junk, pDesc, pCopy, pURL, ulVersionNumber);
                    ReportError( HXLOG_INFO, pszDllName, pDesc );
                }


                // At this point since we have the HXPlugin we should query it to see if
                // it supports any of the GUIDs which have been enumerated so far.
                UINT32 nNumGUIDs = GetNumSupportedGUIDs();
                for(; nNumGUIDs; nNumGUIDs--)
                {
                    CHXString       pszGUID;
                    GUID    theGUID;
                    GetGUIDForIndex(nNumGUIDs-1, pszGUID);
                    CHXuuid::HXUuidFromString(pszGUID, (uuid_tt*)&theGUID);
                    IUnknown* pQueryUnk;
                    if (HXR_OK == pIHXPlugin->QueryInterface(theGUID, (void**)&pQueryUnk))
                    {
                        AddSupportForGUID(pszGUID, pPluginDll, i);
                        HX_RELEASE(pQueryUnk);
                    }
                }

//                  if this is a required plugin, validate it       // XXXAH this could be a problem
//                  if (IsPluginRequired(pDesc, pPlugin))
//                  {
//                      bValidated = ValidateRequiredPlugin(pIHXPlugin, pPlugin);
//                  }

                pIHXPlugin->Release();
                }
            }
        }
        HX_RELEASE( pUnk );
    }

  cleanup:

    HX_RELEASE(pPathBuffer);

    if (result != NO_ERRORS)
    {
        HX_RELEASE( pPluginDll );
    }

    return result;
}


void Plugin2Handler::LoadPluginsFromComponentDLL( Plugin2Handler::PluginDLL* pPluginDll,
                                                  IHXComponentPlugin* pIIterator )
{
    IHXPlugin* pIHXPlugin = NULL;
    if( SUCCEEDED( pIIterator->QueryInterface(IID_IHXPlugin, (void**)&pIHXPlugin ) ) )
    {
        pIHXPlugin->InitPlugin(m_pContext);

        for( UINT32 index = 0; index < pIIterator->GetNumComponents(); index++ )
        {
            IHXValues* pIValues = NULL;
            if( SUCCEEDED( pIIterator->GetComponentInfoAtIndex( index, pIValues ) ) )
            {
                IHXBuffer* pBuffer = NULL;

                if (SUCCEEDED(pIValues->GetPropertyCString(PLUGIN_COMPONENT_NAME, pBuffer)))
                {
                    IHXBuffer* pNamespace = pPluginDll->GetNamespace();

                    if (pNamespace)
                    {
                        CHXString TempNamespace = pNamespace->GetBuffer();
                        TempNamespace += NAMESPACE_SEPARATOR;
                        TempNamespace += pBuffer->GetBuffer();

                        IHXBuffer* pTempBuffer = NULL;
                        if (HXR_OK == CreateAndSetBufferCCF(pTempBuffer, (BYTE*)(const char*)TempNamespace,
                                                            TempNamespace.GetLength()+1, m_pContext))
                        {
                            pIValues->SetPropertyCString(PLUGIN_COMPONENT_NAME, pTempBuffer);
                            HX_RELEASE(pTempBuffer);
                        }
                        HX_RELEASE(pNamespace);
                    }

                    HX_RELEASE(pBuffer);
                }

                // create a new plugin object
                Plugin* pPlugin = new Plugin( m_pContext );
                HX_ASSERT( pPlugin );

                // Setup plugin object
                pPlugin->AddRef();
                pPlugin->SetDLL( pPluginDll );

                // XXXP - this isn't necessary, the Plugin is initialized with an index of 0 if no index is found.
                // pPlugin->SetIndex( (UINT16) 0 );

                pPlugin->SetInfoNeedsRefresh( TRUE );

                // XXXND FIX  I don't like this specialized interface.
                // This gets the basic info from pIHXPlugin, and the rest from pValues
                pPlugin->InitializeComponentPlugin( pIHXPlugin, pIValues );

                // Put in plugin list
                m_PluginList.AddTail(pPlugin);

                // Stick CLSID in map
                AddPluginToIndices( pPlugin );
                HX_RELEASE( pIValues );
            }
        }

        HX_RELEASE (pIHXPlugin);
    }
}




STDMETHODIMP Plugin2Handler::ReloadPlugins()
{
    // now we have to tell all other players that they should also
    // reload their plugins.
    IHXShutDownEverything* pShutDown = NULL ;
    if (HXR_OK == m_pContext->QueryInterface(IID_IHXShutDownEverything, (void**) &pShutDown))
    {
        pShutDown->AskAllOtherPlayersToReload();
        HX_RELEASE(pShutDown);
    }

    // This will re-initialize all the MountPoints
    return ReloadPluginsNoPropagate();
}


///////////////////////////////////////////////////////////////////////////////
//  These functions will find all plugins which are different
//  then those loaded into the registry.
//  It will then load them into memory, get their data, and unload them.
//  It will return HXR_FAIL if some DLL has different values within the
//  registry, and is presently in memory (how could this happen??)

//  If anyone was keeping an index to a loaded DLL and assuming that it
//  would remain constant ... that won't work!!

HX_RESULT Plugin2Handler::ReloadPluginsNoPropagate()
{
    HX_LOG_BLOCK( "Plugin2Handler::ReloadPluginsNoPropagate" );

    HX_RESULT result = HXR_OK;

    // Reload them all.
    for(CHXMapStringToOb::Iterator mp = m_MountPoints.Begin(); mp!=m_MountPoints.End(); ++mp)
    {
        PluginMountPoint* pMountPoint = (PluginMountPoint*) *mp;
        if( FAILED( ReloadPluginsNoPropagate( pMountPoint ) ) )
        {
            result = HXR_FAIL;
        }
    }

    return result;
}


HX_RESULT Plugin2Handler::ReloadPluginsNoPropagate( PluginMountPoint* pMountPoint )
{
    HX_LOG_BLOCK( "Plugin2Handler::ReloadPluginsNoPropagate(PluginMP*)" );
#ifndef _STATICALLY_LINKED
    CFindFile*       pFileFinder = NULL;
#else
    CStaticFindFile* pFileFinder = NULL;
#endif
    IHXBuffer*       pPathBuffer   = NULL;
    char*            pszPluginDir  = NULL;
    ULONG32          nPluginDirLen = 0;
    char*            pszDllName    = 0;
    IHXBuffer*       pNewChecksum  = 0;
    HXBOOL           bRegIsDirty   = FALSE;
    HXBOOL           bContinue     = FALSE;

    // if we have no context do not proceed.
    if (!m_pContext)
    {
        return INVALID_CONTEXT;
    }

    // If this is the 1st time, load everything into the registry.
#ifndef _STATICALLY_LINKED
    pPathBuffer = pMountPoint->Path();
    if (!pPathBuffer)
    {
        return HXR_FAIL;
    }

    pPathBuffer->Get((UCHAR*&)pszPluginDir, nPluginDirLen);

    if (!nPluginDirLen)
    {
        return HXR_FAIL;
    }
#else
    pszPluginDir="";
#endif

    pFileFinder =
#ifndef _STATICALLY_LINKED
        CFindFile::CreateFindFile
#else
        CStaticFindFile::CreateFindFile
#endif
        (pszPluginDir, 0, Plugin2Handler::zm_pszFileExtension);

    if (NULL == pFileFinder)
    {
        pPathBuffer->Release();
        HX_DELETE(pFileFinder);
        return HXR_FAIL;
    }
    pszDllName = pFileFinder->FindFirst();
    HXBOOL bDLLIsDirty = FALSE;
    while (pszDllName)
    {
        // See if this file exists in our list of pluginDLLs
        HXBOOL bFound = FALSE;
        CHXSimpleList::Iterator i;
        Plugin2Handler::PluginDLL* pDLL = NULL;

        bFound =  m_FileNameMap.Lookup(pszDllName, (void*&)pDLL);

        // If it was not found it may a misc DLL -- ie not an
        // RMA DLL. Thus, we should ignore it.

        bContinue = FALSE;

        if (!bFound)
        {
            for(i=m_MiscDLLList.Begin();i!=m_MiscDLLList.End(); ++i)
            {
                Plugin2Handler::OtherDLL* pOther = (Plugin2Handler::OtherDLL*) *i;
                if (!stricmp(pOther->m_filename, pszDllName))
                {
                    // ok we have a match does the checksum match?
                    if( DoChecksumsMatch((const char*)pOther->m_fileChecksum, pszDllName, pPathBuffer))
                    {
                        bContinue = TRUE;
                    }
                }
            }
        }

        if (bContinue)
        {
            pszDllName = pFileFinder->FindNext();
            continue;
        }

        if (bFound)
        {
            if( DoChecksumsMatch(pDLL->GetHash(), pszDllName, pPathBuffer) )
            {
                pszDllName = pFileFinder->FindNext();
                continue;   // old checksum == new checksum. no changes.
            }
            
            // Delete all Plugins which are associated with this PluginDLL.
            LISTPOSITION pPos = NULL;
            if ( m_PluginList.GetCount() )
            {
                LISTPOSITION pPos = m_PluginList.GetHeadPosition();
                while (pPos)
                {
                    // save off current position for delete
                    LISTPOSITION posAt = pPos;

                    Plugin2Handler::Plugin* pPlugin = (Plugin2Handler::Plugin*)m_PluginList.GetNext(pPos);

                    HX_ASSERT(pPlugin);

                    if ( pPlugin && pPlugin->GetDLL() == pDLL )
                    {
                        RemovePluginFromIndices( pPlugin );
                        m_PluginList.RemoveAt( posAt );
                        HX_RELEASE( pPlugin );
                    }
                }
            }

            // Remove the pluginDLL from the PluginDLL list.
            pPos = m_PluginDLLList.Find(pDLL);
            if (pPos)
            {
                m_PluginDLLList.RemoveAt(pPos);
                m_FileNameMap.RemoveKey(pszDllName);

                // Since this PluginDLL was unloaded, no need to keep
                // it in the list of PluginDLLs which export CanUnload2().
                LISTPOSITION posCanUnload = m_CanUnload2DllList.Find( pDLL );
                if ( posCanUnload )
                {
                    m_CanUnload2DllList.RemoveAt( posCanUnload );
                }
            }

            bDLLIsDirty = TRUE;
        }
        else
        {
            bDLLIsDirty = TRUE;
        }

        if (bDLLIsDirty)
        {
            // if we got here we have a new dll.

            Plugin2Handler::Errors loadResult;
            loadResult = LoadDLL( pszDllName, pMountPoint );

            if (loadResult!= NO_ERRORS)
            {
                // The DLL had one of the following problems:
                // (1) the DLL was unloadable
                // (2) the DLL did not have an HXCreateInstance
                // (3) an instance could not be created.
                // (4) It did not implement the PLUGIN interface

                // if it was case 2,3,4 then we can safely never attempt to
                // load the DLL again. However if it was (1) then we must attempt
                // to load the DLL ever time through since it was possibly unloadable due
                // to an imp-lib that will be satisfied lated (without modifing the
                // dll). Jeeze. That comment is UNREADABLE. I have to take effective written
                // english again!

                if (loadResult!=CANT_OPEN_DLL)
                {
                    Plugin2Handler::OtherDLL* pDLLData = new Plugin2Handler::OtherDLL;
                    pDLLData->m_filename = pszDllName;
                    pDLLData->m_pMountPoint = pMountPoint;

                    pNewChecksum = ChecksumFile(pszDllName, pPathBuffer);
                    if (pNewChecksum)
                    {
                        pDLLData->m_fileChecksum = (char*)pNewChecksum->GetBuffer();
                        HX_RELEASE(pNewChecksum);
                        bRegIsDirty=TRUE;
                        m_MiscDLLList.AddTail(pDLLData);
                    }
                    else
                    {
                        HX_DELETE(pDLLData);
                    }
                }
            }
            else
            {
                bRegIsDirty = TRUE;
            }
        }
        pszDllName = pFileFinder->FindNext();

    }

    // Check preference to see whether there is any unfulfilled re-scan CCF Plugin request 
    // left from prior run. This could happen if the app is aborted(crash) before the re-scan
    // can be fulfilled.
    BOOL bReScanCCFPlugins = TRUE;
    if (SUCCEEDED(ReadPrefBOOL(m_pContext, PLUGINHANDLER_RESCAN_CCFPLUGIN_REGNAME, bReScanCCFPlugins)))
    {
        m_bReScanCCFPlugins = bReScanCCFPlugins;
    }

    // We will also re-scan CCF Plugins if we detect plugin is dirty(added/modified) in 
    // any one of the MountPoints
    m_bReScanCCFPlugins = (m_bReScanCCFPlugins || bDLLIsDirty);
 
    // Save the re-scan status to persistent storage(Preference) so that we can verify 
    // its status the next time app is launched
    WritePrefUINT32(m_pContext, PLUGINHANDLER_RESCAN_CCFPLUGIN_REGNAME, m_bReScanCCFPlugins);

    // now get the bandwidth data on all renderer plugins
    IHXValues* pVal = NULL;
    IHXBuffer* pBuffer = NULL;

    if (HXR_OK == CreateValuesCCF(pVal, m_pContext) &&
        HXR_OK == CreateBufferCCF(pBuffer, m_pContext))
    {
        pBuffer->Set((const UCHAR*)PLUGIN_RENDERER_TYPE, strlen(PLUGIN_RENDERER_TYPE)+1);
        pVal->SetPropertyCString(PLUGIN_CLASS, pBuffer);
    }
    HX_RELEASE(pBuffer);

    for(CHXSimpleList::Iterator i = m_PluginList.Begin(); i!=m_PluginList.End(); ++i)
    {
        Plugin2Handler::Plugin* pPlug = (Plugin2Handler::Plugin*)*i;
        if (pPlug->DoesInfoNeedsRefresh() && pPlug->DoesMatch(pVal))
        {
            pPlug->GetBandwidthInfo();
        }
    }

    HX_RELEASE(pVal);

    if (bRegIsDirty)
    {
        // Pass the MountPoint to this so it can write the
        // specific plugins to the correct place
        WritePluginInfo( pMountPoint );
    }

    HX_RELEASE(pPathBuffer);
    HX_DELETE(pFileFinder);
    return HXR_OK;
}

STDMETHODIMP Plugin2Handler::FindIndexUsingValues       (IHXValues* pValues,
                                                         REF(UINT32) unIndex)
{
    CHXSimpleList   PossibleValues;
    CHXSimpleList   PossibleIndexes;
    UINT32          j = 0;
    CHXSimpleList::Iterator i = m_PluginList.Begin();

    for(; i!= m_PluginList.End(); ++i, j++)
    {
        Plugin2Handler::Plugin* pPlugin = (Plugin2Handler::Plugin*) *i;
        if (pPlugin->DoesMatch(pValues))
        {
            PossibleValues.AddTail(pPlugin);
            PossibleIndexes.AddTail((void*)j);
        }
    }

    if (PossibleValues.Begin() == PossibleValues.End())
    {
        unIndex = 0;
        return HXR_FAIL;
    }

    /****************************************************************
     ** Presently when we arrive at this spot with more than one
     ** plugin which matches the search criteria, we simply take
     ** the first one found. If this is not satisfactory then
     ** some method can be added which will process the list based
     ** upon some criteria.
     ****************************************************************/

    // if there are multiple plugins found, we will pick the one whose
    // plugin description contains "RealNetworks" or "Helix DNA"
    if (PossibleValues.GetCount() > 1)
    {
        UINT32 possibleIndex;
        
        if (HXR_OK == FindIndexWithDescription(&PossibleValues, &PossibleIndexes, "RealNetworks", possibleIndex))
        {
            unIndex = possibleIndex;
            return HXR_OK;
        }
        if (HXR_OK == FindIndexWithDescription(&PossibleValues, &PossibleIndexes, "Helix DNA", possibleIndex))
        {
            unIndex = possibleIndex;
            return HXR_OK;
        }
    }

    i = PossibleIndexes.Begin();
    unIndex = (UINT32)(PTR_INT)*i;

    return HXR_OK;
}

STDMETHODIMP Plugin2Handler::GetInstance (UINT32 index, REF(IUnknown*) pUnknown)
{
    pUnknown = NULL;
    LISTPOSITION pPos = m_PluginList.FindIndex(index);
    if (pPos)
    {
        Plugin2Handler::Plugin* pPlugin = (Plugin2Handler::Plugin*) m_PluginList.GetAt(pPos);
        if (pPlugin)
        {
            Errors retVal = pPlugin->GetInstance(pUnknown);
            if (retVal== NO_ERRORS)
            {
                return HXR_OK;
            }
        }
    }
    return HXR_FAIL;
}

STDMETHODIMP Plugin2Handler::FindIndexUsingStrings (char* PropName1,
                                                    char* PropVal1,
                                                    char* PropName2,
                                                    char* PropVal2,
                                                    char* PropName3,
                                                    char* PropVal3,
                                                    REF(UINT32) unIndex)
{
    unIndex = 0;

    // PropName and PropVal have to to valid tuple
    if ((PropName1 && !PropVal1)    ||
        (PropName2 && !PropVal2)    ||
        (PropName3 && !PropVal3)    ||
        (!PropName1 && PropVal1)    ||
        (!PropName2 && PropVal2)    ||
        (!PropName3 && PropVal3))
        return HXR_FAIL;

    IHXValues*  pValues = NULL;
    HX_RESULT   retVal = HXR_FAIL;

    retVal = CreateValuesCCF(pValues, m_pContext);
    if (HXR_OK == retVal)
    {
        AddToValues(pValues, PropName1, PropVal1, eString);
        AddToValues(pValues, PropName2, PropVal2, eString);
        AddToValues(pValues, PropName3, PropVal3, eString);
        retVal = FindIndexUsingValues(pValues, unIndex);
    }
    pValues->Release();
    return retVal;
}


STDMETHODIMP Plugin2Handler::FindPluginUsingValues      (IHXValues* pValues,
                                                         REF(IUnknown*) pUnk)
{
    return FindPluginUsingValues( pValues, pUnk, NULL );
}

HX_RESULT Plugin2Handler::FindGroupOfPluginsUsingStrings(char* PropName1,
                                                         char* PropVal1,
                                                         char* PropName2,
                                                         char* PropVal2,
                                                         char* PropName3,
                                                         char* PropVal3,
                                                         REF(CPluginEnumerator*) pEnumerator)
{
    // PropName and PropVal have to to valid tuple
    if ((PropName1 && !PropVal1)    ||
        (PropName2 && !PropVal2)    ||
        (PropName3 && !PropVal3)    ||
        (!PropName1 && PropVal1)    ||
        (!PropName2 && PropVal2)    ||
        (!PropName3 && PropVal3))
        return HXR_FAIL;

    IHXValues*  pValues = NULL;
    HX_RESULT   retVal = HXR_FAIL;

    retVal = CreateValuesCCF(pValues, m_pContext);
    if (HXR_OK == retVal)
    {
        AddToValues(pValues, PropName1, PropVal1, eString);
        AddToValues(pValues, PropName2, PropVal2, eString);
        AddToValues(pValues, PropName3, PropVal3, eString);
        retVal = FindGroupOfPluginsUsingValues(pValues, pEnumerator);
    }
    pValues->Release();
    return retVal;
}

HX_RESULT Plugin2Handler::FindGroupOfPluginsUsingValues(IHXValues* pValues,
                                                        REF(CPluginEnumerator*) pEnumerator)
{
    CHXSimpleList::Iterator i = m_PluginList.Begin();
    pEnumerator = NULL;

    for(; i!= m_PluginList.End(); ++i)
    {
        Plugin2Handler::Plugin* pPlugin = (Plugin2Handler::Plugin*) *i;
        if (pPlugin->DoesMatch(pValues))
        {
            if (!pEnumerator)
            {
                pEnumerator = new CPluginEnumerator();
            }
            pEnumerator->Add(pPlugin);
        }
    }

    if (!pEnumerator)
    {
        return HXR_FAIL;
    }

    return HXR_OK;
}

STDMETHODIMP Plugin2Handler::GetNumPluginsSupporting(REFIID iid, REF(UINT32) nNumPlugins)
{
    CHXString       sGUID;
    CHXSimpleList*  pSupportList;

    CHXuuid::HXUuidToString( (uuid_tt*) &iid, &sGUID);

    if (!m_GUIDtoSupportList.Lookup(sGUID, (void*&)pSupportList))
    {
        return HXR_FAIL;
    }

    nNumPlugins = pSupportList->GetCount();
    return HXR_OK;
}

STDMETHODIMP Plugin2Handler::GetPluginIndexSupportingIID(REFIID iid, UINT32 nPluginIndex, REF(UINT32) nIndexOut)
{
    CHXString       sGUID;
    CHXSimpleList*  pSupportList;

    CHXuuid::HXUuidToString( (uuid_tt*) &iid, &sGUID);

    if (m_GUIDtoSupportList.Lookup(sGUID, (void*&)pSupportList))
    {
        if (nPluginIndex < (UINT32)pSupportList->GetCount())
        {
            LISTPOSITION pPos = pSupportList->FindIndex(nPluginIndex);
            PluginSupportingGUID* pSupportItem = (PluginSupportingGUID*) pSupportList->GetAt(pPos);
            if (FindPlugin(pSupportItem ->m_filename, pSupportItem->m_nIndexInDLL, nIndexOut))
            {
                return HXR_OK;
            }
        }
    }
    return HXR_FAIL;
}

STDMETHODIMP Plugin2Handler::AddSupportedIID(REFIID iid)
{
    //1st scan to see if this GUID is already supported...

    CHXString       sGUID;
    CHXSimpleList*  pSupportList;

    CHXuuid::HXUuidToString( (uuid_tt*) &iid, &sGUID);

    if (m_GUIDtoSupportList.Lookup(sGUID, (void*&)pSupportList))
    {
#ifdef _MACINTOSH
        // the preferences are getting messed up on the mac.
        // so here we have to validate that the list of plugins we are about to
        // send over is valid.

        void* pGarbage;
        if (!m_GUIDSupportListIsValid.Lookup(sGUID, pGarbage))
        {
            // to validate we will load all of the plugins within this list and
            // QI them.
            HXBOOL bListIsInvalid = FALSE;

            for(LISTPOSITION pPos = pSupportList->GetHeadPosition(); pPos!=NULL;)
            {
                HXBOOL IsValid = FALSE;
                UINT32 nTestPluginIndex;
                IUnknown* pTestUnk;
                PluginSupportingGUID* pSupportItemToTest =  (PluginSupportingGUID*) pSupportList->GetAt(pPos);
                if (FindPlugin(pSupportItemToTest->m_filename, pSupportItemToTest->m_nIndexInDLL, nTestPluginIndex))
                {
                    if (HXR_OK == GetInstance(nTestPluginIndex, pTestUnk))
                    {
                        IUnknown* pTempUnk;
                        if (HXR_OK == pTestUnk->QueryInterface(iid, (void**)&pTempUnk))
                        {
                            // ohhh we are in trouble now. We HAVE to assume one of the two following
                            // statements:
                            // (1) All interfaces have derive from IUnknown.
                            // (2) All of our interfaces support aggeration correctly.
                            // I guess we'll have to assume (1) since I KNOW (2) is incorrect.
                            HX_RELEASE(pTempUnk);
                            IsValid = TRUE;
                        }
                        HX_RELEASE (pTestUnk);
                    }
                }
                HX_ASSERT(IsValid);
                if (!IsValid)
                {
                    // Should not be part of this list. Delete this node.
                    pSupportList->RemoveAt(pPos);
                    bListIsInvalid = TRUE;
                }
                else
                {
                    pSupportList->GetNext(pPos);
                }
            }
            m_GUIDSupportListIsValid.SetAt(sGUID, NULL);

            // at this point we should rewrite the prefs file if bListIsInvalid
            // however, I do not believe that we have an interface for removing
            // enteries from the preferences. hmmmm...
        }
#endif
        return HXR_FAIL;    // hey! it is already in!
    }

#ifndef _MACINTOSH
    if  (!zm_bFasterPrefs)
    {
        // Write this out for each mount point
        for(CHXMapStringToOb::Iterator mp = m_MountPoints.Begin(); mp!=m_MountPoints.End(); ++mp)
        {
            PluginMountPoint* pMountPoint = (PluginMountPoint*) *mp;
            IHXPreferences* pIPrefs = pMountPoint->Prefs();
            if( pIPrefs )
            {
                IHXBuffer* pName = pMountPoint->Name();

                PreferenceEnumerator* pPrefEnum = new PreferenceEnumerator( pIPrefs );

                HX_VERIFY( HXR_OK == pPrefEnum->BeginSubPref(PLUGIN_REGKEY_MOUNTPOINT));
                HX_VERIFY( HXR_OK == pPrefEnum->BeginSubPref((const char*)pName->GetBuffer()));
                HX_VERIFY( HXR_OK == pPrefEnum->BeginSubPref(PLUGIN_REGKEY_ROOT));
                HX_VERIFY( HXR_OK == pPrefEnum->BeginSubPref(PLUGIN_GUIDINFO));
                IHXBuffer* pIndexBuffer = NULL;
                if (HXR_OK == CreateAndSetBufferCCF(pIndexBuffer, (UCHAR*)"", 1, m_pContext))
                {
                    pPrefEnum->WriteSubPref((const char*)sGUID, pIndexBuffer);
                    pIndexBuffer->Release();
                }
                pPrefEnum->EndSubPref(); // XXXAH these may not be necessary.
                pPrefEnum->EndSubPref(); // XXXAH these may not be necessary.
                pPrefEnum->EndSubPref(); // XXXAH these may not be necessary.
                pPrefEnum->EndSubPref(); // XXXAH these may not be necessary.

                HX_DELETE(pPrefEnum);
                HX_RELEASE(pName);
                HX_RELEASE(pIPrefs);
            }
        }
    }
#endif

    // Now create the new structure.
    CHXSimpleList* pSimpleList = new CHXSimpleList();
    m_GUIDtoSupportList.SetAt(sGUID, pSimpleList);

    // now scan all of the Plugins to see if any of them support this interface,

    for(CHXSimpleList::Iterator i = m_PluginList.Begin(); i!=m_PluginList.End(); ++i)
    {
        IUnknown*   pUnk;
        IUnknown*   pQuery;
        Plugin2Handler::Plugin* pPlugin = (Plugin2Handler::Plugin*) *i;

        if (HXR_OK == pPlugin->GetPlugin(pUnk))
        {
            if(HXR_OK == pUnk->QueryInterface(iid, (void**)&pQuery))
            {
                PluginSupportingGUID* pSupportItem = new PluginSupportingGUID();

                IHXBuffer* pBuffer = pPlugin->GetFileName();
                pSupportItem->m_filename    = (char*) pBuffer->GetBuffer();
                HX_RELEASE( pBuffer );
                pSupportItem->m_pMountPoint = pPlugin->GetDLL()->GetMountPoint();
                pSupportItem->m_nIndexInDLL = pPlugin->GetIndex();

                pSimpleList->AddTail((void*)pSupportItem);
                // now write this info the registry
                char IndexArray[16]; /* Flawfinder: ignore */
                sprintf(IndexArray, "%d", (int) pSupportItem->m_nIndexInDLL); /* Flawfinder: ignore */
                IHXBuffer* pIndexBuffer = NULL;
                CreateAndSetBufferCCF(pIndexBuffer, (UCHAR*)IndexArray, strlen(IndexArray)+1, m_pContext);

                if (!zm_bFasterPrefs)
                {
                    IHXPreferences* pIPrefs = pPlugin->GetDLL()->GetMountPoint()->Prefs();
                    if( pIPrefs )
                    {
                        IHXBuffer* pName = pPlugin->GetDLL()->GetMountPoint()->Name();

                        PreferenceEnumerator* pPrefEnum = new PreferenceEnumerator( pIPrefs );

                        HX_VERIFY(HXR_OK == pPrefEnum->BeginSubPref(PLUGIN_REGKEY_MOUNTPOINT));
                        HX_VERIFY(HXR_OK == pPrefEnum->BeginSubPref((const char*)pName->GetBuffer()));
                        HX_VERIFY(HXR_OK == pPrefEnum->BeginSubPref(PLUGIN_REGKEY_ROOT));
                        HX_VERIFY(HXR_OK == pPrefEnum->BeginSubPref(PLUGIN_GUIDINFO));
                        HX_VERIFY(HXR_OK == pPrefEnum->BeginSubPref((const char*)sGUID));
                        pPrefEnum->WriteSubPref((const char*)pSupportItem->m_filename, pIndexBuffer);
                        pPrefEnum->EndSubPref(); // XXXAH these may not be necessary.
                        pPrefEnum->EndSubPref(); // XXXAH these may not be necessary.
                        pPrefEnum->EndSubPref(); // XXXAH these may not be necessary.
                        pPrefEnum->EndSubPref(); // XXXAH these may not be necessary.
                        pPrefEnum->EndSubPref(); // XXXAH these may not be necessary.

                        HX_DELETE(pPrefEnum);
                        HX_RELEASE(pName);
                        HX_RELEASE(pIPrefs);
                    }
                }

                HX_RELEASE(pIndexBuffer);
                HX_RELEASE(pQuery);
            }
            HX_RELEASE(pUnk);
        }
    }

    WriteSupportedGUIDs();

    return HXR_OK;
}


/********************************************************************
 *
 *       IHXPluginHandler3
 *
 ********************************************************************/

STDMETHODIMP
Plugin2Handler::RegisterContext( IUnknown* pContext )
{
    if( !pContext )
    {
        return INVALID_CONTEXT;
    }

    if( m_pContext )
    {
        return HXR_UNEXPECTED;
    }

    m_pContext = pContext;
    m_pContext->AddRef();

    if ( FAILED( m_pContext->QueryInterface( IID_IHXScheduler, (void**) &m_pIScheduler ) ) )
    {
        return( INVALID_CONTEXT );
    }

    // Set up scheduler to ping us.
    m_hScheduler = m_pIScheduler->RelativeEnter( this, kPingDuration );

    if (HXR_OK != m_pContext->QueryInterface(IID_IHXPreferences, (void**) &m_pPreferences))
    {
        return INVALID_CONTEXT;
    }

    /* We don't check errors because it's ok not to have this available. */
    m_pContext->QueryInterface(IID_IHXErrorMessages, (void**) &m_pErrorMessages);


#if defined(HELIX_FEATURE_PREFERENCES)
#if !defined(HELIX_CONFIG_NOSTATICS)
    /* Check to see if we wish to use the 'faster' prefs.
     * This means using VERY long strings to store the
     * plugin information. Windows supports this but
     * discourages the practice.
     */
    IHXBuffer* pBuffer = NULL;
    if (m_pPreferences)
    {
        if (ReadPrefBOOL(m_pPreferences, "UseFasterPref", zm_bFasterPrefs) != HXR_OK)
        {
#if !defined (_WINCE)
            zm_bFasterPrefs = TRUE;
#else
            zm_bFasterPrefs = FALSE;
#endif
        }

    }
#endif /* #if !defined(HELIX_CONFIG_NOSTATICS) */

    //StatDllsOnStartup
    //
    // 0 == Never
    // 1 == always (default if pref not found)
    // 2 == just this time, then never again.
    UINT32 uTmp = 0;
    if( SUCCEEDED(ReadPrefUINT32( m_pContext, "StatDllsOnStartup", uTmp )) )
    {
        m_bStatDllsOnStartup = (uTmp != 0);
        if( 2== uTmp )
        {
            WritePrefUINT32( m_pContext, "StatDllsOnStartup", 0 );
        }
    }
#endif /* HELIX_FEATURE_PREFERENCES */
    return HXR_OK;
}

STDMETHODIMP
Plugin2Handler::AddPluginMountPoint( const char* pName, UINT32 majorVersion, UINT32 minorVersion, IHXBuffer* pPath )
{
    HX_LOG_BLOCK( "Plugin2Handler::AddPluginMountPoint" );

    const char* pMPKey = pName ? pName : (const char*) pPath->GetBuffer();

    // Make sure this mount point is in the list
    PluginMountPoint* pMountPoint = NULL;
    if( !m_MountPoints.Lookup( pMPKey, (void*&) pMountPoint ) )
    {
        // Create new mount point
        pMountPoint = new PluginMountPoint(m_pContext, this, pName, majorVersion, minorVersion, pPath);
        pMountPoint->AddRef();

        // Put new mount point in list
        m_MountPoints.SetAt( pMPKey, pMountPoint );
    }

    // Increment client count
    pMountPoint->AddClient();

    // Load information from registry, and sync DLLs that aren't up to date
    return RefreshPluginInfo( pMountPoint );
}


STDMETHODIMP
Plugin2Handler::RefreshPluginMountPoint( const char* pName )
{
    HX_RESULT result = HXR_FAIL;

    // If this mount point is in the list, refresh it
    PluginMountPoint* pMountPoint = NULL;
    if( m_MountPoints.Lookup( pName, (void*&) pMountPoint ) )
    {
        result = RefreshPluginInfo( pMountPoint );
    }

    return result;
}


STDMETHODIMP
Plugin2Handler::RemovePluginMountPoint( const char* pName )
{
    HX_RESULT result = HXR_FAIL;

    // Make sure this is a valid mount point
    PluginMountPoint* pMountPoint = NULL;
    if( m_MountPoints.Lookup( pName, (void*&) pMountPoint ) )
    {
        // If this was the last client, do the clean up stuff
        if( !pMountPoint->RemoveClient() )
        {
            // Clean up plugins
            if( m_PluginList.GetCount() )
            {
                LISTPOSITION listPos = m_PluginList.GetHeadPosition();
                while( listPos )
                {
                    // Save off current position for delete
                    LISTPOSITION posAt = listPos;

                    // Get current item, and increment position
                    Plugin2Handler::Plugin* pPlugin = (Plugin2Handler::Plugin*) m_PluginList.GetNext( listPos );

                    // If this plugin belongs to the mountpoint, remove it.
                    if( pPlugin && ( pPlugin->GetDLL()->GetMountPoint() == pMountPoint ) )
                    {
                        // Remove plugin from indices
                        RemovePluginFromIndices( pPlugin );

                        // Delete from the saved position
                        m_PluginList.RemoveAt( posAt );
                        HX_RELEASE( pPlugin );
                    }
                }
            }

            // Clean up dlls
            if (m_PluginDLLList.GetCount())
            {
                LISTPOSITION listPos = m_PluginDLLList.GetHeadPosition();
                while( listPos )
                {
                    // Save off current position for delete
                    LISTPOSITION posAt = listPos;

                    // Get current item, and increment position
                    Plugin2Handler::PluginDLL* pPluginDLL = (Plugin2Handler::PluginDLL*) m_PluginDLLList.GetNext( listPos );

                    // If this plugin belongs to the mountpoint, remove it.
                    if( pPluginDLL && ( pPluginDLL->GetMountPoint() == pMountPoint ) )
                    {
                        // Remove from filename map
                        IHXBuffer* pBuffer = pPluginDLL->GetFileName();
                        m_FileNameMap.RemoveKey( (char*) pBuffer->GetBuffer() );
                        HX_RELEASE( pBuffer );

                        // Remove from the LRU
                        RemoveFromLRU(pPluginDLL);

                        // Delete from the saved position
                        m_PluginDLLList.RemoveAt( posAt );
                        HX_RELEASE( pPluginDLL );

                    }
                }
            }

            // Clean up OtherDLL
            if (m_MiscDLLList.GetCount())
            {
                LISTPOSITION listPos = m_MiscDLLList.GetHeadPosition();
                while( listPos )
                {
                    // Save off current position for delete
                    LISTPOSITION posAt = listPos;

                    // Get current item, and increment position
                    Plugin2Handler::OtherDLL* pOtherDLL = (Plugin2Handler::OtherDLL*) m_MiscDLLList.GetNext( listPos );

                    // If this plugin belongs to the mountpoint, remove it.
                    if( pOtherDLL && ( pOtherDLL->m_pMountPoint == pMountPoint ) )
                    {
                        // Delete from the saved position
                        m_MiscDLLList.RemoveAt( posAt );
                        HX_DELETE( pOtherDLL );
                    }
                }
            }

            // Clean up supported GUIDs
            if (m_GUIDtoSupportList.GetCount())
            {
                CHXMapStringToOb::Iterator k;

                for(k = m_GUIDtoSupportList.Begin(); k!=m_GUIDtoSupportList.End(); ++k)
                {
                    CHXSimpleList* pSupportedList = (CHXSimpleList*) *k;

                    LISTPOSITION listPos = pSupportedList->GetHeadPosition();
                    while( listPos )
                    {
                        // Save off current position for delete
                        LISTPOSITION posAt = listPos;

                        // Get current item, and increment position
                        PluginSupportingGUID* pSupportItem = (PluginSupportingGUID*) pSupportedList->GetNext( listPos );

                        // If this plugin belongs to the mountpoint, remove it.
                        if( pSupportItem && ( pSupportItem->m_pMountPoint == pMountPoint ) )
                        {
                            // Delete from the saved position
                            pSupportedList->RemoveAt( posAt );
                            HX_DELETE( pSupportItem );
                        }
                    }

                    // XXXND Remove the list from m_GUIDtoSupportList if it's empty
                }
            }


            // Remove mount point from list
            m_MountPoints.RemoveKey( pName );
            if (pMountPoint)
            {
                pMountPoint->Release();
                pMountPoint = NULL;
            }
        }
    }

    return result;
}


STDMETHODIMP
Plugin2Handler::FindImplementationFromClassID( REFGUID GUIDClassID, REF(IUnknown*) pIUnknownInstance,
                                               IUnknown* pIUnkOuter, IUnknown* pContext )
{
    // Look though the Component plugins
    HX_RESULT result = HXR_FAIL;

    if( FAILED( result = CreatePluginViaIndex( PLUGIN_COMPONENT_CLSID, &GUIDClassID, &pIUnknownInstance, pIUnkOuter ) ) )
    {
        // XXXND FIX Try doing a manual lookup (FindPluginUsingValues)

        // Couldn't find it with the new method, try the old one.
        result = FindImplementationFromClassIDInternal( GUIDClassID, pIUnknownInstance, pContext );
    }

    return result;
}


STDMETHODIMP
Plugin2Handler::FindCLSIDFromName( const char* pName, REF(IHXBuffer*) pCLSID )
{
    HX_SETUP_CHECKPOINTLIST( "Plugin2Handler::FindCLSIDFromName()" );
    HX_PRIME_ACCUMULATOR( 'idfn', "Looking up CLSID from name" );
    HX_ACCUMULATE( 'idfc', "Number of CLSIDs looked up", 1 );

    // Initialize out params
    pCLSID = NULL;

    HX_RESULT result = HXR_FAIL;

    IHXValues* pIValues = NULL;
    if( SUCCEEDED( FindPluginInfoViaIndex( PLUGIN_COMPONENT_NAME, (char*) pName, &pIValues ) ) )
    {
        pIValues->GetPropertyBuffer( PLUGIN_COMPONENT_CLSID, pCLSID );
        HX_RELEASE( pIValues );

        result = HXR_OK;
    }
    else
    {
        // XXXND  FIX  Try using FindPluginUsingString
    }

    HX_UPDATE_ACCUMULATOR( 'idfn' );

    return result;
}


STDMETHODIMP
Plugin2Handler::FindGroupOfPluginsUsingValues( IHXValues* pValues,
                                               REF(IHXPluginSearchEnumerator*) pIEnumerator)
{
    // Initialize out params
    pIEnumerator = NULL;

    // Use the internal function to build up an enumerator object
    CPluginEnumerator* pEnumerator = NULL;
    HX_RESULT result = FindGroupOfPluginsUsingValues( pValues, pEnumerator );

    // If we have our enumerator, get the appropriate interface
    if( SUCCEEDED( result ) )
    {
        result = pEnumerator->QueryInterface( IID_IHXPluginSearchEnumerator,
                                              (void**) &pIEnumerator );
    }

    return result;
}

STDMETHODIMP
Plugin2Handler::FindGroupOfPluginsUsingStrings( char* PropName1, char* PropVal1,
                                                char* PropName2, char* PropVal2,
                                                char* PropName3, char* PropVal3,
                                                REF(IHXPluginSearchEnumerator*) pIEnumerator)
{
    // Initialize out params
    pIEnumerator = NULL;

    // Use the internal function to build up an enumerator object
    CPluginEnumerator* pEnumerator = NULL;
    HX_RESULT result = FindGroupOfPluginsUsingStrings( PropName1, PropVal1,
                                                       PropName2, PropVal2, PropName3, PropVal3, pEnumerator );

    // If we have our enumerator, get the appropriate interface
    if( SUCCEEDED( result ) )
    {
        result = pEnumerator->QueryInterface( IID_IHXPluginSearchEnumerator,
                                              (void**) &pIEnumerator );
    }

    return result;
}


void Plugin2Handler::ReportError( UINT8 severity, const char* pDLLName, const char* pDesc )
{
    if (m_pErrorMessages)
    {
        int nErrorTempLength;
        char *pErrorTemp;

        nErrorTempLength = strlen(pDLLName) + strlen(pDesc) + 2;

        pErrorTemp = new char[nErrorTempLength];

        if(pErrorTemp)
        {
            SafeSprintf(pErrorTemp, nErrorTempLength, "%s %s", pDLLName, pDesc );
            m_pErrorMessages->Report( severity, 0, 0, pErrorTemp, NULL );
            
            delete [] pErrorTemp;
        }
        else
        {
            m_pErrorMessages->Report( HXLOG_ERR, HXR_OUTOFMEMORY, 0, NULL, NULL );
        }
    }
}


STDMETHODIMP
Plugin2Handler::FindPluginUsingValues( IHXValues* pCriteria,
                                       REF(IUnknown*) pIUnkResult,
                                       IUnknown* pIUnkOuter )
{
    HX_SETUP_CHECKPOINTLIST( "Plugin2Handler::FindPluginUsingValues()" );
    HX_PRIME_ACCUMULATOR( 'fpuv', "Plugin lookup with IHXValues" );

    // Initialize out params
    pIUnkResult = NULL;

    CHXSimpleList   PossibleValues;
    CHXSimpleList::Iterator i = m_PluginList.Begin();

    for(; i!= m_PluginList.End(); ++i)
    {
        Plugin2Handler::Plugin* pPlugin = (Plugin2Handler::Plugin*) *i;
        if (pPlugin->DoesMatch(pCriteria))
        {
            PossibleValues.AddTail(pPlugin);
        }
    }

    HX_UPDATE_ACCUMULATOR( 'fpuv' );

    if (PossibleValues.Begin() == PossibleValues.End())
    {
        pIUnkResult = 0;
        return HXR_FAIL;
    }

    /****************************************************************
     ** Presently when we arrive at this spot with more than one
     ** plugin which matches the search criteria, we simply take
     ** the first one found. If this is not satisfactory then
     ** some method can be added which will process the list based
     ** upon some criteria.
     ****************************************************************/

    // if there are multiple plugins found, we will pick the one whose
    // plugin description contains "RealNetworks" or "Helix DNA"
    if (PossibleValues.GetCount() > 1)
    {
        if (HXR_OK == FindPluginWithDescription(&PossibleValues, "RealNetworks", pIUnkResult, pIUnkOuter))
        {
            return HXR_OK;
        }
        if (HXR_OK == FindPluginWithDescription(&PossibleValues, "Helix DNA", pIUnkResult, pIUnkOuter))
        {
            return HXR_OK;
        }
    }

    Plugin2Handler::Plugin* pPlug = (Plugin2Handler::Plugin*) *(PossibleValues.Begin());
    Errors retVal = pPlug->GetInstance( pIUnkResult, pIUnkOuter );

    return ( retVal == NO_ERRORS ) ? HXR_OK : HXR_FAIL;
}


STDMETHODIMP
Plugin2Handler::FindPluginUsingStrings( char* PropName1, char* PropVal1,
                                        char* PropName2, char* PropVal2,
                                        char* PropName3, char* PropVal3,
                                        REF(IUnknown*) pIUnkResult,
                                        IUnknown* pIUnkOuter )
{
    // Initialize out params
    pIUnkResult = NULL;

    // PropName and PropVal have to to valid tuple
    if ((PropName1 && !PropVal1)    ||
        (PropName2 && !PropVal2)    ||
        (PropName3 && !PropVal3)    ||
        (!PropName1 && PropVal1)    ||
        (!PropName2 && PropVal2)    ||
        (!PropName3 && PropVal3))
        return HXR_FAIL;

    IHXValues*  pValues = NULL;
    HX_RESULT   retVal = HXR_FAIL;

    retVal = CreateValuesCCF(pValues, m_pContext);
    if (HXR_OK == retVal)
    {
        AddToValues(pValues, PropName1, PropVal1, eString);
        AddToValues(pValues, PropName2, PropVal2, eString);
        AddToValues(pValues, PropName3, PropVal3, eString);
        retVal = FindPluginUsingValues( pValues, pIUnkResult, pIUnkOuter );
    }
    pValues->Release();
    return retVal;
}


STDMETHODIMP
Plugin2Handler::GetPlugin( ULONG32 ulIndex, REF(IUnknown*) pIUnkResult,
                           IUnknown* pIUnkOuter )
{
    if( ulIndex <= (ULONG32)(m_PluginList.GetCount()-1) && m_PluginList.GetCount() )
    {
        LISTPOSITION pPos = m_PluginList.FindIndex( ulIndex );
        if (pPos)
        {
            Plugin2Handler::Plugin* pPlugin = (Plugin2Handler::Plugin*) m_PluginList.GetAt( pPos );
            if( pPlugin )
            {
                if (NO_ERRORS == pPlugin->GetInstance( pIUnkResult, pIUnkOuter ))
                {
                    return HXR_OK;
                }
                else
                {
                    return HXR_FAIL;
                }
            }
        }
    }
    return HXR_FAIL;

}



//------------------------------------ IHXPluginDatabase interface methods


STDMETHODIMP
Plugin2Handler::AddPluginIndex( THIS_ const char* pKeyName, EPluginIndexType indexType, HXBOOL bScanExisting )
{
    HX_LOG_BLOCK( "Plugin2Handler::AddPluginIndex" );

    HX_RESULT result = HXR_FAIL;

    CPluginDatabaseIndex* pNewIndex = CPluginDatabaseIndex::CreateIndex( indexType );
    if( pNewIndex )
    {
        m_dbIndices.SetAt( pKeyName, pNewIndex );

        if( bScanExisting )
        {
            // XXXND FIX  Scan the existing plugins and add them to this index
        }

        result = HXR_OK;
    }


    return result;
}


STDMETHODIMP
Plugin2Handler::RemovePluginIndex( THIS_ const char* pKeyName )
{
    HX_RESULT result = HXR_UNEXPECTED;

    CPluginDatabaseIndex* pIndex = FindDBIndex( pKeyName );
    if( pIndex )
    {
        if( !m_dbIndices.RemoveKey( pKeyName ) )
        {
            result = HXR_FAIL;
        }
        else
        {
            HX_DELETE( pIndex );
            result = HXR_OK;
        }
    }

    return result;
}


STDMETHODIMP
Plugin2Handler::FindPluginInfoViaIndex( THIS_ const char* pKeyName, const void* pValue, IHXValues** ppIInfo )
{
    HX_RESULT result = HXR_INVALID_PARAMETER;

    if( ppIInfo )
    {
        result = HXR_FAIL;
        *ppIInfo = NULL;

        CPluginDatabaseIndex* pIndex = FindDBIndex( pKeyName );
        if( pIndex )
        {
            IUnknown* pIUnk = NULL;
            if( SUCCEEDED( pIndex->FindItem( pValue, &pIUnk ) ) )
            {
                Plugin2Handler::Plugin* pPlugin = (Plugin2Handler::Plugin*) pIUnk;
                if( SUCCEEDED( result = pPlugin->GetPluginInfo( *ppIInfo ) ) )
                {
                    // Since GetPluginInfo() doesn't addref, we have to here.
                    (*ppIInfo)->AddRef();
                }

                HX_RELEASE( pIUnk );
            }
        }
    }

    return result;
}


STDMETHODIMP
Plugin2Handler::FindPluginSetViaIndex( THIS_ const char* pKeyName, const void* pValue, IHXPluginSearchEnumerator** ppIEnumerator )
{
    // XXXND  Implement this
    *ppIEnumerator = NULL;
    return HXR_NOTIMPL;
}


STDMETHODIMP
Plugin2Handler::CreatePluginViaIndex( THIS_ const char* pKeyName, const void* pValue, IUnknown** ppIUnkPlugin, IUnknown* pIUnkOuter )
{
    HX_RESULT result = HXR_INVALID_PARAMETER;

    if( ppIUnkPlugin )
    {
        result = HXR_FAIL;
        *ppIUnkPlugin = NULL;

        CPluginDatabaseIndex* pIndex = FindDBIndex( pKeyName );
        if( pIndex )
        {
            IUnknown* pIUnk = NULL;
            if( SUCCEEDED( pIndex->FindItem( pValue, &pIUnk ) ) )
            {
                Plugin2Handler::Plugin* pPlugin = (Plugin2Handler::Plugin*) pIUnk;
                if( NO_ERRORS == pPlugin->GetInstance( *ppIUnkPlugin, pIUnkOuter ) )
                {
                    result = HXR_OK;
                }

                HX_RELEASE( pIUnk );
            }
        }
    }

    return result;
}

STDMETHODIMP
Plugin2Handler::UnloadPluginFromClassID(REFGUID GUIDClassID)
{
    HX_RESULT res = HXR_FAIL;

    CPluginDatabaseIndex* pIndex = FindDBIndex( PLUGIN_COMPONENT_CLSID );
    if( pIndex )
    {
        IUnknown* pIUnk = NULL;
        if( SUCCEEDED( pIndex->FindItem( &GUIDClassID, &pIUnk ) ) )
        {
            Plugin2Handler::Plugin* pPlugin = (Plugin2Handler::Plugin*) pIUnk;
            if(pPlugin->GetDLL())
            {
                res = pPlugin->GetDLL()->Unload();
            }
            HX_RELEASE( pIUnk );
        }
    }

    return res;
}

STDMETHODIMP
Plugin2Handler::UnloadPackageByName(const char* pName)
{
    if (!pName)
        return HXR_INVALID_PARAMETER;

    for (CHXSimpleList::Iterator i = m_PluginDLLList.Begin(); i != m_PluginDLLList.End(); ++i)
    {
        PluginDLL* pPluginDLL = (PluginDLL*) *i;
        if (pPluginDLL->GetPackageName() == pName)
        {
            return pPluginDLL->Unload(FALSE);
        }
    }

    return HXR_FAIL;
}


//------------------------------------ Class Methods


HX_RESULT Plugin2Handler::FindImplementationFromClassIDInternal(
    REFGUID GUIDClassID,
    REF(IUnknown*) pIUnknownInstance,
    IUnknown* pContext )
{
    // Initialize out params
    pIUnknownInstance = NULL;
    
    HX_RESULT hxRes = HXR_OK;
    UINT32 ulNumClassFactories = 0;
    UINT32 ulCurrentClassFactory = 0;
    UINT32 ulCurrentClassFactoryIndex = 0;
    IUnknown* pIUnknownClassFactoryCurrent = NULL;
    IHXCommonClassFactory* pIHXCommonClassFactoryCurrent = NULL;
    IHXPlugin* pIHXPluginCurrent = NULL;
    IHXObjectConfiguration* pIHXObjectConfigurationCurrent = NULL;

    // We only rescan the CCF plugins if we detected plugins are dirty.
    // Otherwise, we'll assume CCF plugins have been scanned.
    //
    // This prevents plugins from being un-necessary loaded and reduce
    // startup time.
    if (m_bReScanCCFPlugins)
    {
        m_bReScanCCFPlugins = FALSE;
        AddSupportedIID(IID_IHXCommonClassFactory);
        // Save the status to persistent storage(Preference)
        WritePrefUINT32(m_pContext, PLUGINHANDLER_RESCAN_CCFPLUGIN_REGNAME, m_bReScanCCFPlugins);
    }
    
    hxRes = GetNumPluginsSupporting( IID_IHXCommonClassFactory, ulNumClassFactories );
    
    if( SUCCEEDED(hxRes) && ulNumClassFactories > 0 )
    {
        for( ulCurrentClassFactory = 0, ulCurrentClassFactoryIndex = 0;
             ulCurrentClassFactoryIndex < ulNumClassFactories; ++ulCurrentClassFactoryIndex )
        {
            hxRes = GetPluginIndexSupportingIID( IID_IHXCommonClassFactory,
                                                 ulCurrentClassFactoryIndex,
                                                 ulCurrentClassFactory );
            
            // Create an instance of the plugin at the index ulCurrentClassFactory
            if( SUCCEEDED(hxRes) )
            {
                hxRes = GetInstance( ulCurrentClassFactory, pIUnknownClassFactoryCurrent );
            }

            // If we got a plugin, see if we can get the correct object from it.
            if( SUCCEEDED(hxRes) && pIUnknownClassFactoryCurrent )
            {
                // Initialize the plugin either through IHXPlugin or IHXObjectConfiguration
                if( SUCCEEDED( pIUnknownClassFactoryCurrent->QueryInterface( IID_IHXPlugin,
                                                                             (void**)&pIHXPluginCurrent ) )
                    && pIHXPluginCurrent )
                {
                    pIHXPluginCurrent->InitPlugin( pContext );
                }
                HX_RELEASE(pIHXPluginCurrent);

                if( SUCCEEDED( pIUnknownClassFactoryCurrent->QueryInterface( IID_IHXObjectConfiguration,
                                                                             (void**)&pIHXObjectConfigurationCurrent ) )
                    && pIHXObjectConfigurationCurrent )
                {
                    pIHXObjectConfigurationCurrent->SetContext( pContext );
                }
                HX_RELEASE(pIHXObjectConfigurationCurrent);


                // Now that it's initialized, get the IHXCommonClassFactory interface
                hxRes = pIUnknownClassFactoryCurrent->QueryInterface( IID_IHXCommonClassFactory,
                                                                      (void**)&pIHXCommonClassFactoryCurrent );
            }

            HX_RELEASE(pIUnknownClassFactoryCurrent);


            // We have IHXCommonClassFactory on an intialized plugin.
            // See if it can create the object we want
            if( SUCCEEDED(hxRes) && pIHXCommonClassFactoryCurrent )
            {
                hxRes = pIHXCommonClassFactoryCurrent->CreateInstance( GUIDClassID, (void **)&pIUnknownInstance );
            }

            HX_RELEASE(pIHXCommonClassFactoryCurrent);

            // Check to see if CreateInstance succeeded.  If so, this is the plugin we want
            if( SUCCEEDED(hxRes) && pIUnknownInstance )
            {
                // got It!
                break;
            }
            
            // If CreateInstance allocated something, but returned a failure code, clean up
            HX_RELEASE(pIUnknownInstance);
        }
    }
    else
    {
        // there are no Class factories.
        hxRes = HXR_FAIL;
    }

    return hxRes;
}




HX_RESULT Plugin2Handler::RefreshPluginInfo( PluginMountPoint* pMountPoint )
{
    HX_LOG_BLOCK( "Plugin2Handler::RefreshPluginInfo" );

    HX_RESULT result = HXR_FAIL;

    IHXPreferences* pIPrefs = pMountPoint->Prefs();

    if( pIPrefs )
    {
        if( zm_bFasterPrefs )
        {
            result = ReadPluginInfoFast( pMountPoint );
        }
        else
        {
            result = ReadPluginInfoSlow( pMountPoint );
        }
    }

    if (FAILED (result))
    {
        result = ClearMountPoint_ (pMountPoint);
    }

    if( !pIPrefs || SUCCEEDED( result ) )
    {
        result = ReloadPluginsNoPropagate( pMountPoint );
    }

    HX_RELEASE( pIPrefs );

    return result;
}

HX_RESULT Plugin2Handler::ClearMountPoint_ (PluginMountPoint* pMountPoint)
{
    HX_LOG_BLOCK( "Plugin2Handler::ClearMountPoint_" );

    IHXPreferences* pIPrefs = pMountPoint->Prefs();
    REQUIRE_RETURN_QUIET (pIPrefs, HXR_FAIL);

    IHXPreferences3* pIPrefs3 = NULL;
    if (FAILED (pIPrefs->QueryInterface(IID_IHXPreferences3, (void**)&pIPrefs3)))
    {
        HX_RELEASE (pIPrefs);
        return HXR_FAIL;
    }

    char szRegKey[255]; /* Flawfinder: ignore */

    IHXBuffer* pName = pMountPoint->Name();

    if (pName)
    {
        // delete file info
        ConstructRegKeyPath(&szRegKey[0], PLUGIN_REGKEY_MOUNTPOINT, (const char*)pName->GetBuffer(),
                            PLUGIN_REGKEY_ROOT, PLUGIN_FILENAMES);
        DeleteHugePref_ (pIPrefs, pIPrefs3, szRegKey);

        // delete plugin info
        ConstructRegKeyPath(&szRegKey[0], PLUGIN_REGKEY_MOUNTPOINT, (const char*)pName->GetBuffer(),
                            PLUGIN_REGKEY_ROOT, PLUGIN_PLUGININFO);
        DeleteHugePref_ (pIPrefs, pIPrefs3, szRegKey);

        // delete guid info
        ConstructRegKeyPath(&szRegKey[0], PLUGIN_REGKEY_MOUNTPOINT, (const char*)pName->GetBuffer(),
                            PLUGIN_REGKEY_ROOT, PLUGIN_GUIDINFO);
        DeleteHugePref_ (pIPrefs, pIPrefs3, szRegKey);

        // delete non RMA plugin info
        ConstructRegKeyPath(&szRegKey[0], PLUGIN_REGKEY_MOUNTPOINT, (const char*)pName->GetBuffer(),
                            PLUGIN_REGKEY_ROOT, PLUGIN_NONHXINFO);
        DeleteHugePref_ (pIPrefs, pIPrefs3, szRegKey);

        HX_RELEASE(pName);
    }

    HX_RELEASE(pIPrefs);
    HX_RELEASE(pIPrefs3);

    return HXR_OK;
}

void Plugin2Handler::DeleteHugePref_ (IHXPreferences* pIPrefs, IHXPreferences3* pIPrefs3, const char* pszKeyName)
{
    HX_LOG_BLOCK( "Plugin2Handler::DeleteHugePref_" );

    char szNewKeyName [1024]; /* Flawfinder: ignore */
    char szNumber [16]; /* Flawfinder: ignore */
    IHXBuffer* pIBuffer = NULL;

    for (int i = 0; ; ++i)
    {
        SafeStrCpy(szNewKeyName,  pszKeyName, 1024);
        sprintf (szNumber, "%d", i); /* Flawfinder: ignore */
        SafeStrCat(szNewKeyName,  szNumber, 1024);

        // unfortunately delete pref doesn't give us the return value we want so we will read the prefs for now to
        // determine if they are there.
        if (FAILED (pIPrefs->ReadPref (szNewKeyName, pIBuffer))) break;

        LogRegistryRegeneration_ (szNewKeyName, pIBuffer);

        HX_RELEASE (pIBuffer);
        pIPrefs3->DeletePref (szNewKeyName);
    }
}

HX_RESULT Plugin2Handler::WritePluginInfo( PluginMountPoint* pMountPoint )
{
    HX_LOG_BLOCK( "Plugin2Handler::WritePluginInfo" );

    HX_RESULT result = HXR_FAIL;

    IHXPreferences* pIPrefs = pMountPoint->Prefs();
    if( pIPrefs )
    {
        if( zm_bFasterPrefs )
        {
            result = WritePluginInfoFast( pMountPoint );
        }
        else
        {
            result = WritePluginInfoSlow( pMountPoint );
        }
    }

    HX_RELEASE( pIPrefs );

    return result;
}


HX_RESULT Plugin2Handler::ReadPluginInfoFast( PluginMountPoint* pMountPoint )
{
    HX_LOG_BLOCK( "Plugin2Handler::ReadPluginInfoFast" );

    /*
     *   Code to read from the preferences in one big chunk since
     *   it seems that using many readable preferences is not efficient
     *   on either windows or on macintosh.
     */
    IHXPreferences* pIPrefs = pMountPoint->Prefs();
    if( !pIPrefs )
    {
        // If there are no prefs, there's nothing to read
        return HXR_OK;
    }
    IHXBuffer* pIPathBuffer = pMountPoint->Path();
    if( !pIPathBuffer )
    {
        HX_RELEASE( pIPrefs );

        return HXR_FAIL;
    }

    IHXBuffer* pName = pMountPoint->Name();
    if (!pName)
    {
        HX_RELEASE(pIPrefs);
        return HXR_FAIL;
    }

    char szRegKey[255]; /* Flawfinder: ignore */

    ConstructRegKeyPath(&szRegKey[0], PLUGIN_REGKEY_MOUNTPOINT, (const char*)pName->GetBuffer(),
                        PLUGIN_REGKEY_ROOT, PLUGIN_FILENAMES);

    /*
     *  Get the DLL info.
     */

    HX_LOG_CHECKPOINT( "Get DLL info" );

    IHXBuffer* pInfo = NULL;
    if (HXR_OK == ReadHugePref( pIPrefs, szRegKey, pInfo))
    {
        // the string is defined as follows:
        // {name, checksum, HXBOOL has factory, size, INT numplugins}{ditto}checksum
        char*   pszName = NULL;
        char*   pszCheckSum = NULL;
        HXBOOL  bFactory = FALSE;
        int     nDLLSize = 0, nNumberPlugins = 0;

        char* pszCurrentPos = (char*) pInfo->GetBuffer();
        if (FAILED (VerifyChecksum_ (pszCurrentPos)))
        {
            HX_RELEASE(pIPathBuffer);
            HX_RELEASE(pIPrefs);
            HX_RELEASE(pInfo);
#ifdef _WINDOWS
            HX_ASSERT (!"Plugin handler data is corrupt. Regenerating data.");
#endif
            return HXR_FAIL;
        }

        while( GetPluginFileInfo( pszCurrentPos, pszName, pszCheckSum, bFactory, nDLLSize, nNumberPlugins ) )
        {
            // validate the plugin by comparing the hash of the
            // stats info to the one stored in memory.
            if( DoChecksumsMatch(pszCheckSum, pszName, pIPathBuffer) )
            {
                Plugin2Handler::PluginDLL* pDLL = new Plugin2Handler::PluginDLL( pszName, pMountPoint, this );
                pDLL->AddRef();

                // ReconnectDLL() replaces one PluginDLL object with another PluginDLL; however,
                // both of these PluginDLL objects refer to the same DLL.  This ensures that
                // only one PluginDLL object per DLL is in the m_PluginDLLList.
                ReconnectDLL( pszName, pDLL );
                pDLL->SetPref( nNumberPlugins, pszCheckSum, nDLLSize, bFactory );

            }
        }
        HX_RELEASE(pInfo);
    }

    /*
     *  Get the Plugin info.
     */
    ConstructRegKeyPath(&szRegKey[0], PLUGIN_REGKEY_MOUNTPOINT, (const char*)pName->GetBuffer(),
                        PLUGIN_REGKEY_ROOT, PLUGIN_PLUGININFO);

    if (HXR_OK == ReadHugePref(  pIPrefs, szRegKey, pInfo))
    {
        char* pszCurrentPos = (char*) pInfo->GetBuffer();
        if (FAILED (VerifyChecksum_ (pszCurrentPos)))
        {
            HX_RELEASE(pIPathBuffer);
            HX_RELEASE(pIPrefs);
            HX_RELEASE(pInfo);
#ifdef _WINDOWS
            HX_ASSERT (!"Plugin handler data is corrupt. Regenerating data.");
#endif
            return HXR_FAIL;
        }

        Plugin2Handler::Plugin* pPlugin = NULL;

        while( GetPluginFileInfo( pszCurrentPos, pPlugin ) )
        {
            // XXXND  This really ought to search for duplicates
            if( HXR_OK != ConnectPluginToDLL( pPlugin ) )
            {
                // Must delete from list...
                HX_RELEASE(pPlugin);
            }
            else
            {
                AddPluginToIndices( pPlugin );
                m_PluginList.AddTail(pPlugin);
            }
        }

        HX_RELEASE(pInfo);
    }

    /*
     * Get GUID info for the Plugin Enumerator.
     */

    HX_LOG_CHECKPOINT( "Get GUID" );

    ConstructRegKeyPath(&szRegKey[0], PLUGIN_REGKEY_MOUNTPOINT, (const char*)pName->GetBuffer(),
                        PLUGIN_REGKEY_ROOT, PLUGIN_GUIDINFO);

    if (HXR_OK == ReadHugePref( pIPrefs, szRegKey, pInfo))
    {
        char* pszCurrentPos = (char*) pInfo->GetBuffer();
        if (FAILED (VerifyChecksum_ (pszCurrentPos)))
        {
            HX_RELEASE(pIPathBuffer);
            HX_RELEASE(pIPrefs);
            HX_RELEASE(pInfo);
#ifdef _WINDOWS
            HX_ASSERT (!"Plugin handler data is corrupt. Regenerating data.");
#endif
            return HXR_FAIL;
        }

        CHXSimpleList* pList = NULL;
        char* pszGUID = NULL;
        while(GetGUIDInfo(pszCurrentPos, pMountPoint, pszGUID, pList))
        {
            m_GUIDtoSupportList.SetAt(pszGUID, (void*)pList);
        }
        HX_RELEASE(pInfo);
    }

    /*
     * Get non RMA DLL info
     */
    
    HX_LOG_CHECKPOINT( "Get non RMA DLL info" );
    
    ConstructRegKeyPath(&szRegKey[0], PLUGIN_REGKEY_MOUNTPOINT, (const char*)pName->GetBuffer(),
                        PLUGIN_REGKEY_ROOT, PLUGIN_NONHXINFO);

    if (HXR_OK == ReadHugePref( pIPrefs, szRegKey, pInfo))
    {
        char* pszCurrentPos = (char*) pInfo->GetBuffer();
        if (FAILED (VerifyChecksum_ (pszCurrentPos)))
        {
            HX_RELEASE(pIPathBuffer);
            HX_RELEASE(pIPrefs);
            HX_RELEASE(pInfo);
#ifdef _WINDOWS
            HX_ASSERT (!"Plugin handler data is corrupt. Regenerating data.");
#endif
            return HXR_FAIL;
        }

        Plugin2Handler::OtherDLL* pOtherData = NULL;
        while(GetNonHXInfo(pszCurrentPos, pMountPoint, pOtherData))
        {
            m_MiscDLLList.AddTail((void*)pOtherData);
        }
        HX_RELEASE(pInfo);
    }

    HX_RELEASE(pName);
    HX_RELEASE(pIPathBuffer);
    HX_RELEASE(pIPrefs);

    return HXR_OK;

}


HX_RESULT Plugin2Handler::WritePluginInfoFast( PluginMountPoint* pMountPoint )
{
    HX_LOG_BLOCK( "Plugin2Handler::WritePluginInfoFast" );

    IHXPreferences* pIPrefs = pMountPoint->Prefs();
    if( !pIPrefs )
    {
        return HXR_OK;
    }

    char szRegKey[255]; /* Flawfinder: ignore */
    CHXSimpleList::Iterator i;

    // Create buffer to use by CPluginInfoWriter...
    IHXBuffer* pIHXBuffer = NULL;
    CreateBufferCCF(pIHXBuffer, m_pContext);
    if(pIHXBuffer)
    {
        pIHXBuffer->SetSize(PREF_CACHE_SIZE);
    }

    /*
     *  Now write the PluginDLL info to the reg.
     */

    IHXBuffer* pName = pMountPoint->Name();

    if (m_PluginDLLList.GetCount() && pName)
    {
        ConstructRegKeyPath(&szRegKey[0], PLUGIN_REGKEY_MOUNTPOINT, (const char*)pName->GetBuffer(),
                            PLUGIN_REGKEY_ROOT, PLUGIN_FILENAMES);

        CPluginInfoWriter piw;
        piw.Init(pIPrefs, szRegKey, pIHXBuffer);

        i = m_PluginDLLList.Begin();

        for(; i!=m_PluginDLLList.End(); ++i)
        {
            Plugin2Handler::PluginDLL* pPlugDLL = (Plugin2Handler::PluginDLL*) *i;
            if( pPlugDLL->GetMountPoint() == pMountPoint )
            {
                pPlugDLL->WritePref2(piw);
            }
        }
    }

    /*
     *  Now write the Plugin info to the reg.
     */

    if (m_PluginList.GetCount() && pName)
    {
        ConstructRegKeyPath(&szRegKey[0], PLUGIN_REGKEY_MOUNTPOINT, (const char*)pName->GetBuffer(),
                            PLUGIN_REGKEY_ROOT, PLUGIN_PLUGININFO);

        CPluginInfoWriter piw;
        piw.Init(pIPrefs, szRegKey, pIHXBuffer);

        i = m_PluginList.Begin();

        for(; i!=m_PluginList.End(); ++i)
        {
            Plugin2Handler::Plugin* pPlug = (Plugin2Handler::Plugin*) *i;
            if( pPlug->GetDLL()->GetMountPoint() == pMountPoint )
            {
                pPlug->WritePref2(piw);
            }
        }
    }

    /*
     *  Now write the non-RMA DLL info to the reg.
     */

    if (m_MiscDLLList.GetCount() && pName)
    {
        ConstructRegKeyPath(&szRegKey[0], PLUGIN_REGKEY_MOUNTPOINT, (const char*)pName->GetBuffer(),
                            PLUGIN_REGKEY_ROOT, PLUGIN_NONHXINFO);

        CPluginInfoWriter piw;

        piw.Init(pIPrefs, szRegKey, pIHXBuffer);

        for(i = m_MiscDLLList.Begin();i!=m_MiscDLLList.End(); ++i/*, counter++*/)
        {
            Plugin2Handler::OtherDLL* pOtherData = (Plugin2Handler::OtherDLL*) *i;

            if( pOtherData->m_pMountPoint == pMountPoint )
            {
                // format of the non-rma DLL information is:
                // {filename, checksum}
                piw.Write("{");
                piw.Write((const char*)pOtherData->m_filename);
                piw.Write(",");
                piw.Write((const char*)pOtherData->m_fileChecksum);
                piw.Write("}");
            }
        }
    }

    /*
     *  Now write the GUID info to the reg.
     */

    // format of GUID info:
    // {GUID, filename, index, filename, index, etc}{GUID, filename, index, filename, index}

    if (m_GUIDtoSupportList.GetCount() && pName)
    {
        ConstructRegKeyPath(&szRegKey[0], PLUGIN_REGKEY_MOUNTPOINT, (const char*)pName->GetBuffer(),
                            PLUGIN_REGKEY_ROOT, PLUGIN_GUIDINFO);

        CHXMapStringToOb::Iterator k;

        // Dump the data.
        CPluginInfoWriter piw;
        piw.Init(pIPrefs, szRegKey, pIHXBuffer);

        for(k = m_GUIDtoSupportList.Begin(); k!=m_GUIDtoSupportList.End(); ++k)
        {
            HXBOOL foundFirst = FALSE;
            CHXSimpleList* pSupportedList = (CHXSimpleList*) *k;

            for (i=pSupportedList->Begin(); i!=pSupportedList->End();++i)
            {
                PluginSupportingGUID* pSupportItem = (PluginSupportingGUID*) *i;
                if( pSupportItem->m_pMountPoint == pMountPoint )
                {
                    // If we found an item, write out the header
                    if( !foundFirst )
                    {
                        foundFirst = TRUE;

                        piw.Write("{");
                        piw.Write(k.get_key());
                    }

                    char szScratch[20]; /* Flawfinder: ignore */
                    itoa(pSupportItem->m_nIndexInDLL, szScratch, 10);
                    piw.Write(",");
                    piw.Write((const char*) pSupportItem->m_filename);
                    piw.Write(",");
                    piw.Write(szScratch);
                }
            }

            // If we wrote out a header, write out a tail
            if( foundFirst )
            {
                piw.Write("}");
            }
        }
    }

    HX_RELEASE(pName);
    HX_RELEASE(pIHXBuffer);
    HX_RELEASE(pIPrefs);

    return HXR_OK;
}


HX_RESULT Plugin2Handler::ReadPluginInfoSlow( PluginMountPoint* pMountPoint )
{
    HX_LOG_BLOCK( "Plugin2Handler::ReadPluginInfoSlow" );

    IHXBuffer*                  pBuffer         = NULL;

    // using IHXPreferences2 we will load all data from the registery
    // we must load all of the information from the registry here.

    UINT32                      nIndex      = 0;
    IHXBuffer*                  pPropName   = 0;

    IHXPreferences* pIPrefs         = pMountPoint->Prefs();
    if( !pIPrefs )
    {
        return HXR_OK;
    }
    
    IHXBuffer*  pPathBuffer         = pMountPoint->Path();
    
    PreferenceEnumerator* pPrefEnum = new PreferenceEnumerator( pIPrefs );
    
    // Read the values for the pluginDLLs.
    
    IHXBuffer* pName = pMountPoint->Name();
    
    HX_VERIFY(HXR_OK == pPrefEnum->BeginSubPref(PLUGIN_REGKEY_MOUNTPOINT));
    HX_VERIFY(HXR_OK == pPrefEnum->BeginSubPref((const char*)pName->GetBuffer()));
    HX_VERIFY(HXR_OK == pPrefEnum->BeginSubPref(PLUGIN_REGKEY_ROOT));
    HX_VERIFY(HXR_OK == pPrefEnum->BeginSubPref(PLUGIN_FILENAMES));
    
    while (HXR_OK == pPrefEnum->GetPrefKey(nIndex++, pPropName))
    {
        if (!strcmp((char*)pPropName->GetBuffer(), zm_pszKeyNameRegKey))
        {
            HX_RELEASE(pPropName);
            continue;
        }
        
        Plugin2Handler::PluginDLL* pDLL = new Plugin2Handler::PluginDLL(
            (const char*)pPropName->GetBuffer(),
            pMountPoint, this);
        
        pDLL->AddRef();
        
        HX_VERIFY(HXR_OK == pPrefEnum->BeginSubPref((const char*)pPropName->GetBuffer()));
        pDLL->ReadPref(pPrefEnum);
        pPrefEnum->EndSubPref();
        
        // Does this file exist on the HD?
        HXBOOL     bIsOK       = TRUE;
        CFindFile* pFileFinder = NULL;

        pFileFinder = CFindFile::CreateFindFile((const char*)pPathBuffer->GetBuffer(), 0,
                                                (const char*)pPropName->GetBuffer());


        if (!pFileFinder->FindFirst())
        {
            HX_RELEASE(pPropName);
            delete pFileFinder;
            delete pDLL;
            continue;
        }

        delete pFileFinder;
        HX_RELEASE(pBuffer);

        // is the hash the same?
        pBuffer = pDLL->GetFileName();

        bIsOK = DoChecksumsMatch(pDLL->GetHash(), (char*)pBuffer->GetBuffer(), pPathBuffer);

        // if everthing is OK then add the DLL to the list
        if (bIsOK)
        {
            // ReconnectDLL() replaces one PluginDLL object with another PluginDLL; however,
            // both of these PluginDLL objects refer to the same DLL.  This ensures that
            // only one PluginDLL object per DLL is in the m_PluginDLLList.
            ReconnectDLL((char*)pBuffer->GetBuffer(), pDLL);
        }
        else
        {
            delete pDLL;
        }

        HX_RELEASE(pBuffer);
        HX_RELEASE(pPropName);
    }
    pPrefEnum->EndSubPref();

    // Read the values for the Plugins.
    HX_VERIFY(HXR_OK == pPrefEnum->BeginSubPref(PLUGIN_PLUGININFO));
    
    nIndex = 0;
    while (HXR_OK == pPrefEnum->GetPrefKey(nIndex++, pPropName))
    {
        if (!strcmp((char*)pPropName->GetBuffer(), zm_pszKeyNameRegKey))
        {
            HX_RELEASE(pPropName);
            continue;
        }
        
        Plugin2Handler::Plugin* pPlugin = new Plugin2Handler::Plugin(
            m_pContext);
        pPlugin->AddRef();
        HX_VERIFY(HXR_OK == pPrefEnum->BeginSubPref((const char*)pPropName->GetBuffer()));
        pPlugin->ReadPref(pPrefEnum);
        pPrefEnum->EndSubPref();
        
        // XXXND  This really ought to search for duplicates
        if( HXR_OK != ConnectPluginToDLL(pPlugin) )
        {
            // Must delete from list...
            pPlugin->Release();
        }
        else
        {
            m_PluginList.AddTail(pPlugin);
        }
        HX_RELEASE(pPropName);
    }
    pPrefEnum->EndSubPref();

    // Read the Prefs for other DLL which are not RMA dlls
    HX_VERIFY(HXR_OK == pPrefEnum->BeginSubPref(PLUGIN_NONHXINFO));
    
    nIndex = 0;
    while (HXR_OK == pPrefEnum->GetPrefKey(nIndex++, pPropName))
    {
        if (!strcmp((char*)pPropName->GetBuffer(), zm_pszKeyNameRegKey))
        {
            HX_RELEASE(pPropName);
            continue;
        }
        
        Plugin2Handler::OtherDLL* pOtherData = new Plugin2Handler::OtherDLL;
        pOtherData->m_filename      = (char*)pPropName->GetBuffer();
        pOtherData->m_pMountPoint = pMountPoint;
        
        IHXBuffer* pCheckSumData   = NULL;
        pPrefEnum->ReadPref((const char*) pPropName->GetBuffer(), pCheckSumData);
        if (pCheckSumData)
        {
            pOtherData->m_fileChecksum    = (char*)pCheckSumData->GetBuffer();
        }
        m_MiscDLLList.AddTail((void*)pOtherData);
        HX_RELEASE(pCheckSumData);
        HX_RELEASE(pPropName);
    }
    pPrefEnum->EndSubPref();
    
    // Read the values for the Supported GUIDs.
    HX_VERIFY(HXR_OK == pPrefEnum->BeginSubPref(PLUGIN_GUIDINFO));
    
    nIndex = 0;
    while (HXR_OK == pPrefEnum->GetPrefKey(nIndex++, pPropName))
    {
        if (!strcmp((char*)pPropName->GetBuffer(), zm_pszKeyNameRegKey))
        {
            HX_RELEASE(pPropName);
            continue;
        }

        CHXSimpleList* pSupportList = new CHXSimpleList();
        m_GUIDtoSupportList.SetAt((char*) pPropName->GetBuffer(), (void*)pSupportList);

        UINT32          nSubIndex                   = 0;
        IHXBuffer*      pSubPropName                = 0;

        if (HXR_OK == pPrefEnum->BeginSubPref((const char*)pPropName->GetBuffer()))
        {
            while (HXR_OK == pPrefEnum->GetPrefKey(nSubIndex++, pSubPropName))
            {
                if (!strcmp((char*)pSubPropName->GetBuffer(), zm_pszKeyNameRegKey))
                {
                    HX_RELEASE(pSubPropName);
                    continue;
                }

                UINT32 nDummyVar;
                Plugin2Handler::PluginSupportingGUID* pGUIDSupport =
                    new Plugin2Handler::PluginSupportingGUID();

                pPrefEnum->ReadPref((const char*)pSubPropName->GetBuffer(), pBuffer);
                if (pBuffer)
                {
                    pGUIDSupport->m_nIndexInDLL = atoi((const char*)pBuffer->GetBuffer());
                    pGUIDSupport->m_filename = pSubPropName->GetBuffer();
                    pGUIDSupport->m_pMountPoint = pMountPoint;
                }

                if (pBuffer && FindPlugin((const char*)pSubPropName->GetBuffer(), pGUIDSupport->m_nIndexInDLL
                                          , nDummyVar))
                {
                    pSupportList->AddTail((void*)pGUIDSupport);
                }
                else
                {
                    delete pGUIDSupport;
                }
                HX_RELEASE(pBuffer);
                HX_RELEASE(pSubPropName);
            }
            pPrefEnum->EndSubPref();
        }
        HX_RELEASE(pPropName);
    }

    pPrefEnum->EndSubPref();
    pPrefEnum->EndSubPref();
    pPrefEnum->EndSubPref();
    pPrefEnum->EndSubPref();

    HX_DELETE(pPrefEnum);

    HX_RELEASE(pName);
    HX_RELEASE(pPathBuffer);
    HX_RELEASE(pIPrefs);

    return HXR_OK;

}


HX_RESULT Plugin2Handler::WritePluginInfoSlow( PluginMountPoint* pMountPoint )
{
    HX_LOG_BLOCK( "Plugin2Handler::WritePluginInfoSlow" );

    UINT32      nCounter        = 0;
    IHXBuffer* pBuffer          = 0;
    IHXBuffer* pBuffer2 = 0;
    char        namebuffer[(1<<8)]; /* Flawfinder: ignore */

    IHXPreferences* pIPrefs = pMountPoint->Prefs();
    if (!pIPrefs)
    {
        return HXR_OK;
    }

    // Save the DLL names.
    PreferenceEnumerator* pPrefEnum = new PreferenceEnumerator( pIPrefs );

    IHXBuffer* pName = pMountPoint->Name();

    HX_VERIFY(HXR_OK == pPrefEnum->BeginSubPref(PLUGIN_REGKEY_MOUNTPOINT));
    HX_VERIFY(HXR_OK == pPrefEnum->BeginSubPref((const char*)pName->GetBuffer()));
    HX_VERIFY(HXR_OK == pPrefEnum->BeginSubPref(PLUGIN_REGKEY_ROOT));
    HX_VERIFY(HXR_OK == pPrefEnum->BeginSubPref(PLUGIN_FILENAMES));

    CHXSimpleList::Iterator i = m_PluginDLLList.Begin();
    for(; i!=m_PluginDLLList.End(); ++i, nCounter++)
    {
        Plugin2Handler::PluginDLL* pPlugDLL = (Plugin2Handler::PluginDLL*) *i;
        if( pPlugDLL->GetMountPoint() == pMountPoint )
        {
            pPlugDLL->WritePref(pPrefEnum);
        }
    }

    pPrefEnum->EndSubPref();
    HX_VERIFY(HXR_OK == pPrefEnum->BeginSubPref(PLUGIN_PLUGININFO));

    INT32 nIndexNumber = 0;

    CreateAndSetBufferCCF(pBuffer,(UCHAR*)"",1,m_pContext); 

    // Save the plugin Info.
    for(i = m_PluginList.Begin(); i!=m_PluginList.End(); ++i, nCounter--)
    {
        Plugin2Handler::Plugin* pPlug = (Plugin2Handler::Plugin*) *i;

        if( pPlug->GetDLL()->GetMountPoint() == pMountPoint )
        {
            IHXBuffer* pNameBuf = pPlug->GetFileName();
            char* pChar = (char*)pNameBuf->GetBuffer();
            SafeSprintf(namebuffer, 256, "%s-%d", pChar, (int)pPlug->GetIndex());
            HX_RELEASE(pNameBuf);

#ifndef _MACINTOSH
            pPrefEnum->WriteSubPref(namebuffer, pBuffer);
#endif
            HX_VERIFY(HXR_OK == pPrefEnum->BeginSubPref(namebuffer));
            pPlug->WritePref(pPrefEnum);

            CreateBufferCCF(pNameBuf, m_pContext);
            if (pNameBuf)
            {
                char tempchar[16]; /* Flawfinder: ignore */
                sprintf(tempchar, "%d", (int)nIndexNumber++); /* Flawfinder: ignore */
                pNameBuf->Set((const UCHAR*)tempchar, strlen(tempchar)+1);
                pPrefEnum->WriteSubPref(namebuffer, pNameBuf);
                HX_RELEASE(pNameBuf);
            }
            pPrefEnum->EndSubPref();
        }
    }
    HX_RELEASE(pBuffer);
    pPrefEnum->EndSubPref();

    // Now write the non-RMA DLL info to the reg.

    HX_VERIFY(HXR_OK == pPrefEnum->BeginSubPref(PLUGIN_NONHXINFO));
    for(i = m_MiscDLLList.Begin(); i!=m_MiscDLLList.End(); ++i)
    {
        Plugin2Handler::OtherDLL* pOtherData = (Plugin2Handler::OtherDLL*) *i;
        if( pOtherData->m_pMountPoint == pMountPoint )
        {
            if (HXR_OK == CreateAndSetBufferCCF(pBuffer, (UCHAR*) (const char*) pOtherData->m_fileChecksum,
                                                pOtherData->m_fileChecksum.GetLength()+1, m_pContext))
            {
                pPrefEnum->WriteSubPref(pOtherData->m_filename, pBuffer);
                HX_RELEASE(pBuffer);
            }
        }
    }
    pPrefEnum->EndSubPref();


    HX_VERIFY(HXR_OK == pPrefEnum->BeginSubPref(PLUGIN_GUIDINFO));

    // Now save the Data to support the plugin Enumerator.
    for(CHXMapStringToOb::Iterator k = m_GUIDtoSupportList.Begin();
        k!=m_GUIDtoSupportList.End(); ++k)
    {
        CHXString sGUID  = k.get_key();
        CHXSimpleList* pSupportedList = (CHXSimpleList*) *k;

        for(CHXSimpleList::Iterator j = pSupportedList->Begin();
            j!=pSupportedList->End(); ++j)
        {
            char IndexArray[16]; /* Flawfinder: ignore */
            PluginSupportingGUID* pSupportItem = (PluginSupportingGUID*) *j;

            if( pSupportItem->m_pMountPoint == pMountPoint )
            {
                IHXBuffer* pIndexBuffer = NULL;
                if (HXR_OK == CreateBufferCCF(pIndexBuffer, m_pContext))
                {
                    sprintf(IndexArray, "%d", (int)pSupportItem->m_nIndexInDLL); /* Flawfinder: ignore */
                    pIndexBuffer->Set((const UCHAR*)IndexArray, strlen(IndexArray)+1);

                    HX_VERIFY(HXR_OK == pPrefEnum->BeginSubPref((const char*)sGUID));
                    pPrefEnum->WriteSubPref((const char*)pSupportItem->m_filename, pIndexBuffer);
                    pPrefEnum->EndSubPref();

                    HX_RELEASE(pIndexBuffer);
                }
            }
        }
    }

    pPrefEnum->EndSubPref();
    pPrefEnum->EndSubPref();
    pPrefEnum->EndSubPref();
    pPrefEnum->EndSubPref();

    HX_DELETE(pPrefEnum);
    HX_RELEASE(pName);
    HX_RELEASE(pIPrefs);

    return HXR_OK;
}



CPluginDatabaseIndex* Plugin2Handler::FindDBIndex( const char* pKeyName )
{
    CPluginDatabaseIndex* pIndex = NULL;

    void* pVoid = NULL;
    if( m_dbIndices.Lookup( pKeyName, pVoid ) )
    {
        pIndex = (CPluginDatabaseIndex*) pVoid;
    }

    return pIndex;
}


void Plugin2Handler::AddPluginToIndices( Plugin2Handler::Plugin* pPlugin )
{
    IHXValues* pIValues = NULL;
    if( SUCCEEDED( pPlugin->GetPluginInfo( pIValues ) ) )
    {
        HX_RESULT status = HXR_FAIL;
        const char* pPropName = NULL;
        IHXBuffer* pBuffer;

        // Iterate over all the elements in pIValues.  If there's a key for that
        // name, add the plugin to that index.

        // Scan the CString entries
        status = pIValues->GetFirstPropertyCString( pPropName, pBuffer );
        while( status == HXR_OK )
        {
            CPluginDatabaseIndex* pIndex = FindDBIndex( pPropName );
            if( pIndex )
            {
                pIndex->AddItem( pBuffer, pPlugin );
            }

            HX_RELEASE( pBuffer );
            status = pIValues->GetNextPropertyCString( pPropName, pBuffer );
        }

        // Scan the Buffer entries
        status = pIValues->GetFirstPropertyBuffer( pPropName, pBuffer );
        while( status == HXR_OK )
        {
            CPluginDatabaseIndex* pIndex = FindDBIndex( pPropName );
            if( pIndex )
            {
                pIndex->AddItem( pBuffer, pPlugin );
            }

            HX_RELEASE( pBuffer );
            status = pIValues->GetNextPropertyBuffer( pPropName, pBuffer );
        }

        // NOTE:  Don't release pIValues.  GetPluginInfo() doesn't AddRef() it
    }
}


void Plugin2Handler::RemovePluginFromIndices( Plugin2Handler::Plugin* pPlugin )
{
    HX_LOG_BLOCK( "Plugin2Handler::RemovePluginFromIndices" );

    CHXMapStringToOb::Iterator iter;

    for(iter = m_dbIndices.Begin(); iter != m_dbIndices.End(); ++iter)
    {
        CPluginDatabaseIndex* pIndex = (CPluginDatabaseIndex*) *iter;
        pIndex->RemoveItem( pPlugin );
    }
}


HX_RESULT
Plugin2Handler::FindIndexWithDescription(
    CHXSimpleList* pPossibleValues,
    CHXSimpleList* pPossibleIndexes,
    const char* pDescription,
    REF(UINT32) outPossibleIndex)
{
    IHXValues*      pPluginValues = NULL;
    IHXBuffer*      pBuffer = NULL;
    CHXSimpleList::Iterator i;
    int j = 0;
    
    outPossibleIndex = 0;
    
    for(i = pPossibleValues->Begin(); i!= pPossibleValues->End(); ++i, j++)
    {
        Plugin2Handler::Plugin* pPlugin = (Plugin2Handler::Plugin*) *i;
        if (HXR_OK == pPlugin->GetPluginInfo(pPluginValues) && pPluginValues)
        {
            if (HXR_OK == pPluginValues->GetPropertyCString(PLUGIN_DESCRIPTION2, pBuffer) &&
                pBuffer)
            {
                if (strstr((const char*)pBuffer->GetBuffer(), pDescription))
                {
                    LISTPOSITION pos = pPossibleIndexes->FindIndex(j);
                    outPossibleIndex = (UINT32)(PTR_INT)pPossibleIndexes->GetAt(pos);
                    HX_RELEASE(pBuffer);
                    return HXR_OK;
                }
            }
            HX_RELEASE(pBuffer);
        }
    }
    return HXR_FAIL;
}

HX_RESULT
Plugin2Handler::FindPluginWithDescription(
    CHXSimpleList* pPossibleValues,
    const char* pDescription,
    REF(IUnknown*) pIUnkResult,
    IUnknown* pIUnkOuter)
{
    IHXValues*      pPluginValues = NULL;
    IHXBuffer*      pBuffer = NULL;
    CHXSimpleList::Iterator i;
    
    for(i = pPossibleValues->Begin(); i!= pPossibleValues->End(); ++i)
    {
        Plugin2Handler::Plugin* pPlugin = (Plugin2Handler::Plugin*) *i;
        if (HXR_OK == pPlugin->GetPluginInfo(pPluginValues) && pPluginValues)
        {
            if (HXR_OK == pPluginValues->GetPropertyCString(PLUGIN_DESCRIPTION2, pBuffer) &&
                pBuffer)
            {
                if (strstr((const char*)pBuffer->GetBuffer(), pDescription))
                {
                    HX_RELEASE(pBuffer);
                    if ( NO_ERRORS == pPlugin->GetInstance( pIUnkResult, pIUnkOuter ))
                    {
                        return HXR_OK;
                    }
                    else
                    {
                        return HXR_FAIL;
                    }
                }
            }
            HX_RELEASE(pBuffer);
        }
    }
    return HXR_FAIL;
}

HX_RESULT
Plugin2Handler::ConstructRegKeyPath(char* pszPath, const char* pszNode0, 
                                    const char* pszNode1, const char* pszNode2,
                                    const char* pszNode3)
{
    if (pszPath)
    {
        if (pszNode0)
        {
            SafeStrCpy(pszPath,  pszNode0, 255);
        }
        else
        {
            goto exit;
        }

        if (pszNode1)
        {
            SafeStrCat(pszPath,  zm_pszRegKeySeperator, 255);
            SafeStrCat(pszPath,  pszNode1, 255);
        }
        else
        {
            goto exit;
        }

        if (pszNode2)
        {
            SafeStrCat(pszPath,  zm_pszRegKeySeperator, 255);
            SafeStrCat(pszPath,  pszNode2, 255);
        }
        else
        {
            goto exit;
        }

        if (pszNode3)
        {
            SafeStrCat(pszPath,  zm_pszRegKeySeperator, 255);
            SafeStrCat(pszPath,  pszNode3, 255);
        }
        else
        {
            goto exit;
        }
    }

  exit:
    return HXR_OK;
}

HX_RESULT Plugin2Handler::RemoveDLLFromGUIDSupportLists(const char* pszFileName)
{
    for(CHXMapStringToOb::Iterator i = m_GUIDtoSupportList.Begin();
        i!=m_GUIDtoSupportList.End(); ++i)
    {
        CHXSimpleList*  pSupportList = (CHXSimpleList*) *i;
        if (pSupportList->IsEmpty())
            continue;
        for(LISTPOSITION pPos = pSupportList->GetHeadPosition();
            pPos != pSupportList->GetTail();)
        {
            if (!pPos) break;
            PluginSupportingGUID* pSupportItem = (PluginSupportingGUID*) pSupportList->GetAt(pPos);
            if (!strcmp(pszFileName, pSupportItem->m_filename))
            {
                pPos = pSupportList->RemoveAt(pPos);
                delete pSupportItem;
            }
            if (!pPos)
            {
                break;
            }
            pSupportList->GetNext(pPos);

        }
    }
    return HXR_OK;
}


UINT32  Plugin2Handler::GetNumSupportedGUIDs()
{
    return m_GUIDtoSupportList.GetCount();
}

HX_RESULT Plugin2Handler::GetGUIDForIndex(UINT32 nIndex, REF(CHXString) sGUID)
{
    if (nIndex>= (UINT32)m_GUIDtoSupportList.GetCount())
        return HXR_FAIL;

    CHXMapStringToOb::Iterator i = m_GUIDtoSupportList.Begin();
    for(; nIndex; nIndex--, ++i) {};  // not a mistake.

    sGUID = (char*)i.get_key();
    return HXR_OK;
}

HX_RESULT Plugin2Handler::AddSupportForGUID(const char* pszGUID, PluginDLL* pDLL, UINT32 nIndexInDLL)
{
    // Get the DLL name.  We'll need it in a couple places below.
    IHXBuffer* pBuffer = pDLL->GetFileName();
    char* pNewDLLName = (char*) pBuffer->GetBuffer();

    // 1st look to see if we have data about this plugin already if so then return
    // Find the GUID which we are supporting.
    CHXSimpleList* pSupportList;

    if (m_GUIDtoSupportList.Lookup(pszGUID, (void*&)pSupportList))
    {
        if (!pSupportList->IsEmpty())
        {
            for(LISTPOSITION pPos = pSupportList->GetHeadPosition();
                pPos != pSupportList->GetTail();)
            {
                if (!pPos) break;
                PluginSupportingGUID* pSupport = (PluginSupportingGUID*) pSupportList->GetAt(pPos);
                if ( (pSupport->m_nIndexInDLL == nIndexInDLL) && (!strcmp(pSupport->m_filename, pNewDLLName)))
                {
                    HX_RELEASE( pBuffer );
                    return HXR_FAIL;
                }
                pSupportList->GetNext(pPos);
            }
        }
    }
    else
    {
        HX_ASSERT(1);
        HX_RELEASE( pBuffer );
        return HXR_NOTIMPL;
    }


    PluginSupportingGUID* pSupport = new PluginSupportingGUID;
    pSupport->m_filename = pNewDLLName;
    pSupport->m_pMountPoint = pDLL->GetMountPoint();
    pSupport->m_nIndexInDLL = nIndexInDLL;
    pSupportList->AddTail((void*) pSupport);
    HX_RELEASE( pBuffer );
    return HXR_OK;
}


STDMETHODIMP Plugin2Handler::FindPluginUsingStrings (char* PropName1,
                                                     char* PropVal1,
                                                     char* PropName2,
                                                     char* PropVal2,
                                                     char* PropName3,
                                                     char* PropVal3,
                                                     REF(IUnknown*) pRetUnk)
{
    return FindPluginUsingStrings( PropName1, PropVal1, PropName2, PropVal2,
                                   PropName3, PropVal3, pRetUnk, NULL );
}


STDMETHODIMP
Plugin2Handler::FindImplementationFromClassID
(
    REFGUID GUIDClassID,
    REF(IUnknown*) pIUnknownInstance
    )
{
    // Defer to the new version
    return FindImplementationFromClassID( GUIDClassID, pIUnknownInstance, NULL, m_pContext );
}

STDMETHODIMP Plugin2Handler::Close ()
{
    CHXSimpleList::Iterator i = m_PluginList.Begin();

    if ( m_pIScheduler && m_hScheduler )
    {
        m_pIScheduler->Remove( m_hScheduler );
    }
    HX_RELEASE( m_pIScheduler );

    // Release all Plugins and Their Associated DLLs
    for(; i!=m_PluginList.End(); ++i)
    {
        Plugin2Handler::Plugin* pPlug = (Plugin2Handler::Plugin*) *i;
        pPlug->Release();
    }
    m_PluginList.RemoveAll();

    for(i = m_PluginDLLList.Begin(); i!=m_PluginDLLList.End(); ++i)
    {
        Plugin2Handler::PluginDLL* pPlugDLL = (Plugin2Handler::PluginDLL*) *i;
        pPlugDLL->Release();
    }
    m_PluginDLLList.RemoveAll();

    for(i = m_MiscDLLList.Begin(); i!=m_MiscDLLList.End(); ++i)
    {
        Plugin2Handler::OtherDLL* pOtherDLL = (Plugin2Handler::OtherDLL*) *i;
        delete pOtherDLL;
    }
    m_MiscDLLList.RemoveAll();

    for(CHXMapStringToOb::Iterator mp = m_MountPoints.Begin(); mp!=m_MountPoints.End(); ++mp)
    {
        Plugin2Handler::PluginMountPoint* pMountPoint = (Plugin2Handler::PluginMountPoint*) *mp;
        pMountPoint->Release();
    }
    m_MountPoints.RemoveAll();

    // Release all of the GUID stuff
    CHXMapStringToOb::Iterator j;
    for(j = m_GUIDtoSupportList.Begin();
        j!= m_GUIDtoSupportList.End(); ++j)
    {
        CHXSimpleList* pList = (CHXSimpleList*) *j;
        for(i = pList->Begin(); i!=pList->End(); ++i)
        {
            PluginSupportingGUID* pSupportItem = (PluginSupportingGUID*) *i;
            delete pSupportItem;
        }
        delete pList;
    }
    m_GUIDtoSupportList.RemoveAll();

    // Clean up all the indices
    for(j = m_dbIndices.Begin(); j!=m_dbIndices.End(); ++j)
    {
        CPluginDatabaseIndex* pIndex = (CPluginDatabaseIndex*) *j;
        HX_DELETE( pIndex );
    }
    m_dbIndices.RemoveAll();

    // release all of the CORE stuff...
    HX_RELEASE(m_pPluginDir);
    HX_RELEASE(m_pPreferences);
    HX_RELEASE(m_pErrorMessages);
    HX_RELEASE(m_pContext);

    m_CanUnload2DllList.RemoveAll();

    return HXR_OK;
}

STDMETHODIMP Plugin2Handler::SetRequiredPlugins (const char** ppszRequiredPlugins)
{
    return HXR_OK;
}


HX_RESULT Plugin2Handler::AddToValues(IHXValues* pValues, char* pPropName, char* pPropVal, eValueTypes eValueType)
{
    if (!pPropName || !pPropVal)
        return HXR_FAIL;
    // 1st make into a cstrig and to trim the buffer...
    CHXString theValue = (pPropVal);
    theValue.TrimLeft();
    theValue.TrimRight();

    switch (eValueType)
    {
       case eString:
       {
           IHXBuffer* pBuffer = NULL;
           if (HXR_OK == CreateAndSetBufferCCF(pBuffer, (UCHAR*)(const char*)theValue,
                                               theValue.GetLength()+1, m_pContext))
           {
               pValues->SetPropertyCString(pPropName, pBuffer);
               pBuffer->Release();
               return HXR_OK;
           }
           else
           {
               return HXR_FAILED;
           }
       }
       case eInt:
       {
           int val = atoi(theValue);
           pValues->SetPropertyULONG32(pPropName, (ULONG32)val);
           return HXR_OK;
       }
    }
    return HXR_NOTIMPL;
}

/*
 *  Win98 does not allow reg keys that are larger than 16k a pop.
 *  Thus, this function splits up regkeys into more managiable bites.
 */
#define PREF_THRESHOLD 10000

void Plugin2Handler::WriteHugePref( IHXPreferences* pIPrefs, const char* pszKeyName, IHXBuffer* pBigBuffer)
{
    int         counter = 0;
    UCHAR*      pPlaceHolder;
    char        szNewKeyName[1024]; /* Flawfinder: ignore */
    char        szNumber[16]; /* Flawfinder: ignore */ // a REALLY big buffer could be put in :)
    int         nOriginalBufferSize = pBigBuffer->GetSize();
    IHXBuffer*  pIHXBuffer = NULL;
    ULONG32     nNewBufferSize;
    char        oldValue;


    if (nOriginalBufferSize>PREF_THRESHOLD)
    {
        CreateBufferCCF(pIHXBuffer, m_pContext);
        pIHXBuffer->SetSize(PREF_THRESHOLD);

        pPlaceHolder = pBigBuffer->GetBuffer();
        // bleech we are going to break up the buffer into more managiable chunks.
        while(((int)(pPlaceHolder - pBigBuffer->GetBuffer() ))!= nOriginalBufferSize)
        {
            nNewBufferSize = nOriginalBufferSize - (ULONG32 ( pPlaceHolder - pBigBuffer->GetBuffer() ));
            if (nNewBufferSize> PREF_THRESHOLD)
            {
                nNewBufferSize = PREF_THRESHOLD;
            }

            memcpy((char*)pIHXBuffer->GetBuffer(), pPlaceHolder, nNewBufferSize); /* Flawfinder: ignore */

            SafeStrCpy(szNewKeyName,  pszKeyName, 1024);
            sprintf(szNumber, "%d", counter); /* Flawfinder: ignore */
            SafeStrCat(szNewKeyName,  szNumber, 1024);
            /*
             *  Check for a null termination at the end .. if not then muck around.
             */
            char* pTempchar = (char*)pIHXBuffer->GetBuffer();

            if (pTempchar[nNewBufferSize-1])
            {
                // not null terminated.
                oldValue = pTempchar[nNewBufferSize-1];
                pTempchar[nNewBufferSize-1] = 0;
                pIPrefs->WritePref(szNewKeyName, pIHXBuffer);
                nNewBufferSize--;
            }
            else
            {
                pIPrefs->WritePref(szNewKeyName, pIHXBuffer);
            }
            pPlaceHolder+=nNewBufferSize;
            counter++;
        }
        HX_RELEASE(pIHXBuffer);
    }
    else
    {
        SafeStrCpy(szNewKeyName,  pszKeyName, 1024);
        SafeStrCat(szNewKeyName,  "0", 1024);
        pIPrefs->WritePref(szNewKeyName, pBigBuffer);
    }
}

/*
 *  Win98 does not allow reg keys that are larger than 16k a pop.
 *  Thus, this function takes regkeys that were split up and re-joins them
 *  for later processing.
 */

HX_RESULT Plugin2Handler::ReadHugePref( IHXPreferences* pIPrefs, const char* pszKeyName, REF(IHXBuffer*) pBigBuffer)
{
    HX_LOG_BLOCK( "Plugin2Handler::ReadHugePref" );

    CHXSimpleList   listOBuffers;
    HX_RESULT       retVal = HXR_FAIL;
    IHXBuffer*      pBuffer;
    char            szNewKeyName[1024]; /* Flawfinder: ignore */
    char            szNumber[16]; /* Flawfinder: ignore */
    int             counter = 0;
    int             totalSize = 0;
    UCHAR*          pPos;

    pBigBuffer      = NULL;

    SafeStrCpy(szNewKeyName,  pszKeyName, 1024);
    SafeStrCat(szNewKeyName,  "0", 1024);
    retVal = pIPrefs->ReadPref(szNewKeyName, pBuffer);
    while(HXR_OK == retVal)
    {
        totalSize+=pBuffer->GetSize();
        listOBuffers.AddTail((void*) pBuffer);
        counter++;
        SafeStrCpy(szNewKeyName,  pszKeyName, 1024);
        sprintf(szNumber, "%d", counter); /* Flawfinder: ignore */
        SafeStrCat(szNewKeyName,  szNumber, 1024);
        retVal = pIPrefs->ReadPref(szNewKeyName, pBuffer);
    }

    if (listOBuffers.GetCount())
    {
        CreateBufferCCF(pBigBuffer, m_pContext);

        pBigBuffer->SetSize(totalSize);
        pPos = pBigBuffer->GetBuffer();
        *pPos = 0;

        CHXSimpleList::Iterator i;
        for(i=listOBuffers.Begin(); i!=listOBuffers.End(); ++i)
        {
            pBuffer = (IHXBuffer*)*i;
            SafeStrCat((char*)pPos, (char*)pBuffer->GetBuffer(), totalSize);
            HX_RELEASE(pBuffer);
        }
        return HXR_OK;
    }
    else
    {
        return HXR_FAIL;
    }
}


// XXXND This function doesn't correctly deal with mount points.  The last mount point in the list
// that has any contents will overwrite all the others.
void Plugin2Handler::WriteSupportedGUIDs()
{
    if (m_GUIDtoSupportList.GetCount())
    {
        // Create buffer to use by CPluginInfoWriter...
        IHXBuffer* pIHXBuffer = NULL;
        CreateBufferCCF(pIHXBuffer, m_pContext);
        if(pIHXBuffer)
        {
            pIHXBuffer->SetSize(PREF_CACHE_SIZE);
        }

        char szRegKey[255]; /* Flawfinder: ignore */

        // Do this for each MountPoint
        for(CHXMapStringToOb::Iterator mp = m_MountPoints.Begin(); mp!=m_MountPoints.End(); ++mp)
        {
            PluginMountPoint* pMountPoint = (PluginMountPoint*) *mp;

            // format of GUID info:
            // {GUID, filename, index, filename, index, etc}{GUID, filename, index, filename, index}
            IHXPreferences* pIPrefs = pMountPoint->Prefs();
            if( pIPrefs )
            {
                IHXBuffer* pName = pMountPoint->Name();

                ConstructRegKeyPath(&szRegKey[0], PLUGIN_REGKEY_MOUNTPOINT, (pName ? (const char*) pName->GetBuffer() : ""),
                                    PLUGIN_REGKEY_ROOT, PLUGIN_GUIDINFO);

                HX_RELEASE(pName);

                CPluginInfoWriter piw;
                piw.Init(pIPrefs, szRegKey, pIHXBuffer);

                // Dump the data.
                CHXSimpleList::Iterator i;
                CHXMapStringToOb::Iterator k;
                for(k = m_GUIDtoSupportList.Begin(); k!=m_GUIDtoSupportList.End(); ++k)
                {
                    HXBOOL foundFirst = FALSE;

                    CHXSimpleList* pSupportedList = (CHXSimpleList*) *k;
                    for (i=pSupportedList->Begin(); i!=pSupportedList->End();++i)
                    {
                        PluginSupportingGUID* pSupportItem = (PluginSupportingGUID*) *i;
                        if( pSupportItem->m_pMountPoint == pMountPoint )
                        {
                            if( !foundFirst )
                            {
                                foundFirst = TRUE;

                                piw.Write("{");
                                piw.Write(k.get_key());
                            }

                            char    szScratch[20]; /* Flawfinder: ignore */
                            itoa(pSupportItem->m_nIndexInDLL, szScratch, 10);
                            piw.Write(",");
                            piw.Write((const char*) pSupportItem->m_filename);
                            piw.Write(",");
                            piw.Write(szScratch);
                        }
                    }

                    if( foundFirst )
                    {
                        piw.Write("}");
                    }
                }

                HX_RELEASE( pIPrefs );
            }
        }

        HX_RELEASE( pIHXBuffer );
    }
}


STDMETHODIMP_(ULONG32) Plugin2Handler::GetNumOfPlugins()
{
    return m_PluginList.GetCount();
}

STDMETHODIMP Plugin2Handler::GetPlugin(ULONG32 ulIndex, REF(IUnknown*)  /*OUT*/ pInstance)
{
    return GetPlugin( ulIndex, pInstance, NULL );
}


STDMETHODIMP Plugin2Handler::FlushCache()
{
    // if we have no context do not proceed.
    if (!m_pContext)
    {
        return INVALID_CONTEXT;
    }

    INT32 nTempCache = m_nCacheSizeBites;

    m_nCacheSizeBites = 0;
    UpdateCache();
    m_nCacheSizeBites = nTempCache;

    // now we have to tell all other players that they should also
    // flush thier cache.

    IHXShutDownEverything* pShutDown = NULL ;
    if (HXR_OK == m_pContext->QueryInterface(IID_IHXShutDownEverything, (void**) &pShutDown))
    {
        pShutDown->AskAllOtherPlayersToUnload();
        HX_RELEASE(pShutDown);
    }

    return HXR_OK;
}



STDMETHODIMP Plugin2Handler::SetCacheSize(ULONG32 nSizeKB)
{
    m_nCacheSizeBites = (nSizeKB<<10);
    UpdateCache();
    return HXR_OK;
}

HX_RESULT Plugin2Handler::AddtoLRU(Plugin2Handler::PluginDLL* pDLL)
{

    // 1st we have to find if the plugin is in the LRU list
    // and if it is remove it.
    RemoveFromLRU(pDLL);

    // now just add to the LRU list.
    m_DLL_LRUList.AddTail((void*)pDLL);
    return HXR_OK;
}

HX_RESULT Plugin2Handler::RemoveFromLRU(Plugin2Handler::PluginDLL* pDLL)
{
    if (pDLL)
    {
        LISTPOSITION pPos = m_DLL_LRUList.Find(pDLL);
        if (pPos)
        {
            m_DLL_LRUList.RemoveAt(pPos);
        }
        return HXR_OK;
    }
    return HXR_INVALID_PARAMETER;
}

HX_RESULT Plugin2Handler::UpdateCache()
{
    // XXXAH we are disabling the cache feature until
    //       we fix all of our plugins to make all
    //       objects which are supposed to live past the
    //       lifetime of the plugin with the CCF.

    // XXXSM before update the cache, we have to be sure that
    //          it only contains actually loaded dlls.
    //          Removing a mount point during runtime could
    //          invalidate some dlls in this cache resulting in
    //          an invalid memory access.
    //return HXR_OK;

    LISTPOSITION pPos;
    INT32 nTotalSize = 0;

    // find out how many bytes are being used by the plugins.
    for(CHXSimpleList::Iterator i = m_DLL_LRUList.Begin();
        i!= m_DLL_LRUList.End(); ++i)
    {
        Plugin2Handler::PluginDLL* pDLL = (Plugin2Handler::PluginDLL*) *i;
        nTotalSize += pDLL->GetFileSize();
    }

    // are we under the limit?
    if (nTotalSize <= m_nCacheSizeBites)
    {
        return HXR_OK;
    }

    return HXR_OK;
    // since the most recently used portions of the list are stuck at the tail we
    // will go forward unloading DLL until we are below the limit.

    for(pPos = m_DLL_LRUList.GetHeadPosition();
        pPos != m_DLL_LRUList.GetTail();)
    {
        if (!pPos) break;
        Plugin2Handler::PluginDLL* pDLL = (Plugin2Handler::PluginDLL*) m_DLL_LRUList.GetAt(pPos);
        if (HXR_OK == pDLL->Unload())
        {
            nTotalSize -= pDLL->GetFileSize();
            // delete this node from the list
            pPos = m_DLL_LRUList.RemoveAt(pPos);
            if (!pPos)
            {
                break;
            }
            if (nTotalSize<= m_nCacheSizeBites)
            {
                break;
            }
            continue;
        }
        m_DLL_LRUList.GetNext(pPos);
    }
    if (nTotalSize<= m_nCacheSizeBites)
    {
        return HXR_OK;
    }
    else
    {
        return HXR_FAIL;
    }
}

HXBOOL Plugin2Handler::GetPluginFileInfo(REF(char*) pszCurrentPos,
                                         REF(char*) pszName,
                                         REF(char*) pszCheckSum,
                                         REF(HXBOOL) bFactory,
                                         REF(int) nDLLSize,
                                         REF(int) nNumberPlugins)
{
    HX_LOG_BLOCK( "Plugin2Handler::GetPluginFileInfo" );

    char* pszBOOLFactory;
    char* pszINTSize;
    char* pszINTPlugins;

    // eat characters until you find a {

    for(;*pszCurrentPos!='{' && *pszCurrentPos!=0; pszCurrentPos++){};

    if (*pszCurrentPos=='{')
    {
        pszCurrentPos++;
        pszName = pszCurrentPos;
        // eat until a you find a comma
        for(;*pszCurrentPos!=',' && *pszCurrentPos!=0; pszCurrentPos++) {};
        *pszCurrentPos=0;
        pszCurrentPos++;
        pszCheckSum = pszCurrentPos;
        // eat until a you find a comma
        for(;*pszCurrentPos!=',' && *pszCurrentPos!=0; pszCurrentPos++) {};
        *pszCurrentPos=0;
        pszCurrentPos++;
        pszBOOLFactory = pszCurrentPos;
        // eat until a you find a comma
        for(;*pszCurrentPos!=',' && *pszCurrentPos!=0; pszCurrentPos++) {};
        *pszCurrentPos=0;
        pszCurrentPos++;
        pszINTSize = pszCurrentPos;
        // eat until a you find a comma
        for(;*pszCurrentPos!=',' && *pszCurrentPos!=0; pszCurrentPos++) {};
        *pszCurrentPos=0;
        pszCurrentPos++;
        pszINTPlugins = pszCurrentPos;
        // eat until a you find a close brace
        for(;*pszCurrentPos!='}' && *pszCurrentPos!=0; pszCurrentPos++) {};
        *pszCurrentPos=0;
        pszCurrentPos++;

        if (pszBOOLFactory && pszINTSize && pszINTPlugins)
        {
            bFactory         = atoi(pszBOOLFactory);
            nDLLSize         = atoi(pszINTSize);
            nNumberPlugins   = atoi(pszINTPlugins);
        }
        return TRUE;
    }
    return FALSE;
}

HXBOOL Plugin2Handler::GetNameValuePair(REF(char*) pszCurrentPos, REF(char*) pszName, REF(char*) pszValue)
{
    // eat until we find either a NULL, a comma, or a close brace

    // check for termination condition
    if (*pszCurrentPos=='{')
    {
        return FALSE;
    }

    pszName = pszCurrentPos;
    for(;*pszCurrentPos && *pszCurrentPos!='}' && *pszCurrentPos!='~';pszCurrentPos++) {};
    if (*pszCurrentPos == '~')
    {
        *pszCurrentPos = 0;
        pszCurrentPos++;
    }
    else
    {
        return FALSE;
    }
    pszValue = pszCurrentPos;
    for(;*pszCurrentPos && *pszCurrentPos!='}' && *pszCurrentPos!='~';pszCurrentPos++) {};
    if (*pszCurrentPos == '}' || *pszCurrentPos=='~')
    {
        *pszCurrentPos = 0;
        pszCurrentPos++;
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

HXBOOL Plugin2Handler::GetPluginFileInfo(REF(char*) pszCurrentPos, REF(Plugin2Handler::Plugin*) pPlugin)
{
    /*  format of data in regestry is as follows:
     *  {ValueName, N|S|BValue, ValueName, N|S|B, ValueName, N|S|B, etc},
     *  {etc}
     */


    // eat until we find a '{' or a null
    for(;*pszCurrentPos!='{' && *pszCurrentPos!=0; pszCurrentPos++) {};

    if (*pszCurrentPos)
    {
        pPlugin = new Plugin2Handler::Plugin(m_pContext);
        pPlugin->AddRef();

        // XXXND  This might do well to get the values from the plugin, and then
        // add each property using the IHXValues interface.

        char* pszName;
        char* pszValue;

        pszCurrentPos++;
        while (GetNameValuePair(pszCurrentPos, pszName, pszValue))
        {
            switch (*pszValue)
            {
               case 'N':
               {
                   pPlugin->SetPropertyULONG32(pszName, pszValue+1);
                   if (!strcasecmp(pszName, "indexnumber"))
                   {
                       pPlugin->SetIndex(atoi(pszValue+1));
                   }
                   break;
               }
               case 'S':
               {
                   pPlugin->SetPropertyCString(pszName, pszValue+1);
                   break;
               }
               case 'B':
               {
                   UINT32 size = strlen(pszValue);
                   pPlugin->SetPropertyBuffer( pszName, (BYTE*) pszValue + 1, size - 1 );
                   break;
               }
               case 'X':
               {
                   UINT32 size = strlen(pszValue);

                   IHXBuffer* pBuf = NULL;
                   CreateBufferCCF(pBuf, m_pContext);
                   pBuf->SetSize(size);

                   // We subtract 1 from size because we move foward in the buffer by 1
                   INT32 s = BinFrom64(pszValue+1, size - 1, pBuf->GetBuffer());

                   HX_ASSERT((UINT32)s <= size);
                   HX_ASSERT(s != -1);

                   if (s != -1)
                   {
                       pPlugin->SetPropertyBuffer(pszName, pBuf->GetBuffer(), s );
                   }

                   HX_RELEASE(pBuf);
                   break;
               }
            }
        }
        return TRUE;
    }
    return FALSE;
}

HXBOOL Plugin2Handler::GetNextSupportingFile(REF(char*) pszCurrentPos, REF(char*) pszFileName, REF(UINT32) index)
{
    char* pszIndex;
    if (*pszCurrentPos == '{')
    {
        return FALSE;
    }
    if (*pszCurrentPos)
    {
        pszFileName = pszCurrentPos;
        // eat until we find a ',' or a null or a '}'
        for(;*pszCurrentPos!=',' && *pszCurrentPos!=0 && *pszCurrentPos!='}'; pszCurrentPos++) {};
        if (*pszCurrentPos && *pszCurrentPos!='}')
        {
            *pszCurrentPos = 0;
            pszCurrentPos++;
            pszIndex = pszCurrentPos;
            // eat until we find a ',' or a null
            for(;*pszCurrentPos!=',' && *pszCurrentPos!=0 && *pszCurrentPos!='}'; pszCurrentPos++) {};
            if (*pszCurrentPos)
            {
                *pszCurrentPos = 0;
                pszCurrentPos++;
                index = atoi(pszIndex);
                return TRUE;
            }
        }
    }
    return FALSE;
}

HXBOOL Plugin2Handler::GetGUIDInfo(REF(char*) pszCurrentPos, PluginMountPoint* pMountPoint, REF(char*) pszGUID, REF(CHXSimpleList*) pList)
{
    // format of GUID info:
    // {GUID, filename, index, filename, index, etc}{GUID, filename, index, filename, index}

    UINT32  nDummyVar;
    UINT32  index;
    char*   pszFileName;
    Plugin2Handler::PluginSupportingGUID* pGUIDSupport;

    // eat until we find a '{' or a null
    for(;*pszCurrentPos!='{' && *pszCurrentPos!=0; pszCurrentPos++) {};

    pList   = NULL;
    pszGUID = NULL;

    if (*pszCurrentPos)
    {
        pszCurrentPos++;
        pszGUID = pszCurrentPos;
        // eat until we find a ',' or a null or a close brace
        for(;*pszCurrentPos!=',' && *pszCurrentPos!=0 && *pszCurrentPos!='}'; pszCurrentPos++) {};

        // do we have support for this GUID?
        if (*pszCurrentPos != '}')
        {
            if (*pszCurrentPos)
            {
                *pszCurrentPos = 0;
                pszCurrentPos++;

                // ok we have a valid list
                pList = new CHXSimpleList;

                // now construct the list

                while (GetNextSupportingFile(pszCurrentPos, pszFileName, index))
                {
                    if (FindPlugin(pszFileName, index, nDummyVar))
                    {
                        pGUIDSupport = new Plugin2Handler::PluginSupportingGUID();

                        pGUIDSupport->m_filename = pszFileName;
                        pGUIDSupport->m_pMountPoint = pMountPoint;
                        pGUIDSupport->m_nIndexInDLL = index;

                        pList->AddTail((void*)pGUIDSupport);
                    }
                }
            }
        }
        else
        {
            pList = new CHXSimpleList;
            *pszCurrentPos = 0;
            pszCurrentPos++;
        }
        return TRUE;
    }
    return FALSE;
}

HXBOOL Plugin2Handler::GetNonHXInfo(REF(char*) pszCurrentPos, PluginMountPoint* pMountPoint, REF(Plugin2Handler::OtherDLL*) pOtherData)
{
    HX_LOG_BLOCK( "Plugin2Handler::GetNonHXInfo" );

    char* pszName;
    char* pszHash;

    // eat until we find a '{' or a null
    for(;*pszCurrentPos!='{' && *pszCurrentPos!=0; pszCurrentPos++) {};

    if (*pszCurrentPos)
    {
        pszCurrentPos++;
        pszName = pszCurrentPos;

        // eat until we find a '{' or a null
        for(;*pszCurrentPos!=',' && *pszCurrentPos!=0; pszCurrentPos++) {};
        if (*pszCurrentPos)
        {
            *pszCurrentPos = 0;
            pszCurrentPos++;
            pszHash = pszCurrentPos;

            // eat until we find a '}' or a null
            for(;*pszCurrentPos!='}' && *pszCurrentPos!=0; pszCurrentPos++) {};
            if (*pszCurrentPos)
            {
                *pszCurrentPos = 0;
                pszCurrentPos++;
            {
                pOtherData = new Plugin2Handler::OtherDLL;
                pOtherData->m_filename = pszName;
                pOtherData->m_pMountPoint = pMountPoint;
                pOtherData->m_fileChecksum = pszHash;
                return TRUE;
            }
            }
        }
    }
    return FALSE;
}


STDMETHODIMP Plugin2Handler::ReadFromRegistry()
{
    // Set up a mount point with the default plugin location
    IHXBuffer* pIPluginDir = GetPluginDir();
    HX_RESULT result = AddPluginMountPoint( HXVER_SDK_PRODUCT, 0, 0, pIPluginDir );
    HX_RELEASE( pIPluginDir );
    return result;
}


HXBOOL Plugin2Handler::FindPlugin(const char* pFileName, UINT32 nDLLIndex, REF(UINT32) nIndex)
{
    UINT32 nTempIndex = 0;

    for(CHXSimpleList::Iterator i = m_PluginList.Begin(); i!=m_PluginList.End(); ++i)
    {
        Plugin2Handler::Plugin* pPlugin = (Plugin2Handler::Plugin*) *i;
        IHXBuffer* pBuffer = pPlugin->GetFileName();
        char* pPluginFileName = (char*) pBuffer->GetBuffer();

        if (!strcasecmp(pPluginFileName, pFileName))
        {
            if (pPlugin->GetIndex() == nDLLIndex)
            {
                nIndex = nTempIndex;
                HX_RELEASE(pBuffer);
                return TRUE;
            }
        }
        HX_RELEASE(pBuffer);
        nTempIndex++;
    }
    return FALSE;
}


HX_RESULT Plugin2Handler::ConnectPluginToDLL(Plugin2Handler::Plugin * pPlugin)
{
    Plugin2Handler::PluginDLL* pPluginDll = NULL;
    IHXBuffer* pBuffer = pPlugin->GetFileName();
    HX_RESULT retVal = HXR_FAIL;

    if (pBuffer)
    {
        char*   pszFileName = (char*)pBuffer->GetBuffer();
        if (m_FileNameMap.Lookup(pszFileName, (void*&)pPluginDll))
        {
            // match found...
            pPlugin->SetDLL(pPluginDll);
            retVal = HXR_OK;
        }
    }
    HX_RELEASE(pBuffer);
    return retVal;
}


IHXBuffer* Plugin2Handler::GetPluginDir()
{
    // If we don't have a cached PluginDir, figure it out
    if( !m_pPluginDir )
    {
#ifdef _STATICALLY_LINKED
        CreateBufferCCF(m_pPluginDir, m_pContext);
        m_pPluginDir->Set((const UCHAR *)"",1);
#else
        const char* pPath = NULL;

        // Get the plugin directory from the Dll Access Paths
        pPath = GetDLLAccessPath()->GetPath(DLLTYPE_PLUGIN);
        if (!pPath || !pPath[0])
        {
            m_pPluginDir = GetDefaultPluginDir();

            GetDLLAccessPath()->SetPath(DLLTYPE_PLUGIN,
                                        (const char*)m_pPluginDir->GetBuffer());
        }
        else
        {
            CreateBufferCCF(m_pPluginDir, m_pContext);
            m_pPluginDir->Set((const UCHAR*)pPath, strlen(pPath) + 1);

            //  Validate this path.
            //

#ifdef _MAC_CFM // XXXSEH: Revisit validation under Mach-O.
            //
            // Couldn't find a cross platform path validator, so I'll do it just for the Macintosh.
            // That's where this is most important anyways.
            //

            char        tempPath[1024]; /* Flawfinder: ignore */
            FSSpec      tempSpec;
            OSErr       err=0;
            UINT32      ulBytesToCopy = (m_pPluginDir->GetSize() > 1023 ? 1023 : m_pPluginDir->GetSize());
            memcpy(tempPath,m_pPluginDir->GetBuffer(),ulBytesToCopy); /* Flawfinder: ignore */
            tempPath[ulBytesToCopy]=0;

            err = FSSpecFromPathName(tempPath,&tempSpec);

            //
            //  Uhoh the Macintosh path validator could not resolve this
            //  path, thus we must refresh it.  Strange how we store this
            //  path but never expect it to change.
            //
            if (err != noErr)
            {
                HX_RELEASE(m_pPluginDir);

                m_pPluginDir = GetDefaultPluginDir();

                GetDLLAccessPath()->SetPath(DLLTYPE_PLUGIN,
                                            (const char*)m_pPluginDir->GetBuffer());
            }
#endif

        }

        // CFindFile is kind of brain dead in that it will append a OS Seperator
        // after the path regardless of what is there currently (bad CFindFile bad!)
        // so we will strip it off if it is the last character,
        // Also all functions within the plugin handler assume that the plugin
        // directory will have not have an OS seperator at the end of it.

        char*       pszPluginDir    = NULL;
        ULONG32     nPluginDirLen   = 0;

        m_pPluginDir->Get((UCHAR*&)pszPluginDir, nPluginDirLen);

        // now we COULD (and should for speed) use nPluginDirLen-1 as the
        // length of the string. However, it is SLIGHTLY safer to use strlen

        if ( *(pszPluginDir+(strlen(pszPluginDir)-1)) == Plugin2Handler::zm_cDirectorySeperator)
        {
            *(pszPluginDir+(strlen(pszPluginDir)-1)) = 0;
        }
#endif // _STATICALLY_LINKED
    }

    // AddRef() our return value
    if( m_pPluginDir )
        m_pPluginDir->AddRef();

    return m_pPluginDir;
}


IHXPreferences*  Plugin2Handler::GetPreferences()
{
    if( m_pPreferences )
    {
        m_pPreferences->AddRef();
    }

    return m_pPreferences;
}


IHXBuffer*  Plugin2Handler::GetDefaultPluginDir()
{
    IHXBuffer*  lpBuffer        = NULL;
    char mask_name[_MAX_PATH + 1] = ""; /* Flawfinder: ignore */

#if (defined (_WINDOWS) || defined (_WIN32)) && !defined(_WINCE)
    if (!GetSystemDirectory(mask_name, _MAX_PATH))
    {
        strcpy(mask_name, ""); /* Flawfinder: ignore */
    }

    if (strlen(mask_name) > 0 && mask_name[strlen(mask_name) - 1] != zm_cDirectorySeperator)
    {
        SafeStrCat(mask_name,  zm_pszDirectorySeperator, _MAX_PATH+1);
    }

    SafeStrCat(mask_name, "Real", _MAX_PATH+1);
#elif defined (_UNIX) && !defined(_MAC_UNIX)
    SafeStrCpy(mask_name, getenv("HOME"), _MAX_PATH+1);
    SafeStrCat(mask_name, "/Real", _MAX_PATH+1);
#elif defined (_MACINTOSH) || defined(_MAC_UNIX)
    FSSpec extSpec;
    extSpec.name[0] = 0;

    // if Sys 8.5 or greater, use Application Support folder, else Extensions
    INT32       sysVersion;
    char*       bytes=(char*)&sysVersion;
    OSType      folderType;

    ::Gestalt(gestaltSystemVersion,&sysVersion);
    if (bytes[2]>8 || ((bytes[2]==8) && (bytes[3] >= 0x50)))
        folderType = kApplicationSupportFolderType;
    else
        folderType = kExtensionFolderType;

    if (noErr == ::FindFolder (-1, folderType, kDontCreateFolder,
                               &extSpec.vRefNum, &extSpec.parID))
    {
        CHXString str_path;
        str_path = extSpec;
        SafeStrCpy(mask_name, (char*)(const char*)str_path, _MAX_PATH);
    }
    else
        SafeStrCpy(mask_name, ":System Folder:Extensions:", _MAX_PATH+1 );

    SafeStrCat(mask_name, "Real", _MAX_PATH+1);

#if defined(_CARBON) || defined(_MAC_UNIX)
    if (bytes[2] >= 0x10) // OS X
    {
#ifdef _MAC_MACHO
        CFBundleRef mainBundle;
        CFURLRef mainBundleURL;
        CFURLRef updirURL;
        CFBundleRef myBundle;

        // get the main bundle for the app
        mainBundle = ::CFBundleGetMainBundle();

        // look for a resource in the main bundle by name
        mainBundleURL = ::CFBundleCopyBundleURL( mainBundle );
        updirURL = ::CFURLCreateCopyDeletingLastPathComponent(NULL, mainBundleURL);

        CFStringRef urlString = CFURLCopyPath(updirURL);
        CFStringGetCString(urlString, mask_name, _MAX_PATH, kCFStringEncodingMacRoman);

#else
        ProcessSerialNumber psn;
        ProcessInfoRec pir;

        GetCurrentProcess(&psn);
        pir.processName = NULL;
        pir.processAppSpec = &extSpec;
        pir.processInfoLength = sizeof(pir);

        GetProcessInformation(&psn, &pir);

        extSpec.name[0] = '\0';

        CHXString str_path;
        str_path = extSpec;
        SafeStrCpy(mask_name, (char*)(const char*)str_path, _MAX_PATH);
#endif
    }
#endif


#elif defined(_WINCE)
    strcpy(mask_name, "\\"); /* Flawfinder: ignore */
#endif //defined (_WINDOWS) || defined (_WIN32)

    CreateAndSetBufferCCF(lpBuffer, (UCHAR*)mask_name, strlen(mask_name)+1, m_pContext);
    return lpBuffer;
}


/**********************************************************************************
 ***                 Plugin2Handler::Plugin                                      ***
 ***********************************************************************************/


Plugin2Handler::Plugin::Plugin(IUnknown* pContext) :
    m_lRefCount(0)
    ,   m_pValues(0)
    ,   m_pPluginDLL(0)
    ,   m_pContext(pContext)
    ,   m_bInfoNeedsRefresh(FALSE)
    ,   m_nPluginIndex(0)
{
    CreateValuesCCF(m_pValues, m_pContext);
}

Plugin2Handler::Plugin::~Plugin()
{
    // Test code to look at all of the data in the map.
#if 0
    const char* pPropName=NULL;
    IHXBuffer* pInBuffer=NULL;

    m_pValues->GetFirstPropertyCString(pPropName, pInBuffer);
    HX_RELEASE(pInBuffer);

    HX_RESULT tempresult = HXR_OK;
    while (tempresult == HXR_OK)
    {
        tempresult = m_pValues->GetNextPropertyCString(pPropName, pInBuffer);
        if (tempresult == HXR_OK)
            pInBuffer->Release();
    }
#endif

    HX_RELEASE(m_pValues);
    HX_RELEASE(m_pPluginDLL);
    //HX_RELEASE(m_pPluginWatcher);
}

HXBOOL Plugin2Handler::Plugin::DoesMatch(IHXValues* pValues)
{
    CHXSimpleList   PossibleValues1;
    CHXSimpleList   PossibleValues2;
    const char*     pPropName = NULL;
    ULONG32         nInValue;
    ULONG32         nOutValue;
    IHXBuffer*      pInBuffer = NULL;
    IHXBuffer*      pOutBuffer = NULL;

    // Check ULONGS 1st
    if (HXR_OK == pValues->GetFirstPropertyULONG32(pPropName, nInValue))
    {
        if (HXR_OK==m_pValues->GetPropertyULONG32(pPropName, nOutValue))
        {
            if (nInValue != nOutValue)
            {
                goto notFoundexit;
            }
        }
        else
        {
            goto notFoundexit;
        }
        while (HXR_OK == pValues->GetNextPropertyULONG32(pPropName, nInValue))
        {
            if (HXR_OK == m_pValues->GetPropertyULONG32(pPropName, nOutValue))
            {
                if (nInValue != nOutValue)
                {
                    goto notFoundexit;
                }
            }
            else
            {
                goto notFoundexit;
            }
        }
    }

    // Test code to look at all of the data in the map.
#ifdef _DEBUG
    HX_RESULT tempresult;

    tempresult = HXR_OK;

    tempresult = m_pValues->GetFirstPropertyCString(pPropName, pInBuffer);
    HX_RELEASE(pInBuffer);

    while (tempresult == HXR_OK)
    {
        tempresult = m_pValues->GetNextPropertyCString(pPropName, pInBuffer);
        if (tempresult == HXR_OK)
        {
            HX_RELEASE(pInBuffer);
        }
    }
#endif /*_DEBUG*/

    // Check String Props.
    if (HXR_OK == pValues->GetFirstPropertyCString(pPropName, pInBuffer))
    {
        if (HXR_OK == m_pValues->GetPropertyCString(pPropName, pOutBuffer))
        {
            if (!AreBufferEqual(pOutBuffer, pInBuffer))
            {
                goto notFoundexit;
            }
        }
        else
        {
            goto notFoundexit;
        }

        HX_RELEASE(pInBuffer);
        HX_RELEASE(pOutBuffer);

        while (HXR_OK == pValues->GetNextPropertyCString(pPropName, pInBuffer))
        {
            if (HXR_OK == m_pValues->GetPropertyCString(pPropName, pOutBuffer))
            {
                if ( !AreBufferEqual(pOutBuffer, pInBuffer))
                {
                    goto notFoundexit;
                }
            }
            else
            {
                goto notFoundexit;
            }

            HX_RELEASE(pInBuffer);
            HX_RELEASE(pOutBuffer);
        }
    }


    // Check Buffer Properties
    if (HXR_OK == pValues->GetFirstPropertyBuffer(pPropName, pInBuffer))
    {
        // XXXND  Make some utility functions for doing this...
        if (HXR_OK == m_pValues->GetPropertyBuffer(pPropName, pOutBuffer))
        {
            if( pOutBuffer->GetSize() == pInBuffer->GetSize() )
            {
                if( ::memcmp( pOutBuffer->GetBuffer(), pInBuffer->GetBuffer(), pOutBuffer->GetSize() ) )
                {
                    goto notFoundexit;
                }
            }
        }
        else
        {
            goto notFoundexit;
        }

        HX_RELEASE(pInBuffer);
        HX_RELEASE(pOutBuffer);

        while (HXR_OK == pValues->GetNextPropertyBuffer(pPropName, pInBuffer))
        {
            if (HXR_OK == m_pValues->GetPropertyBuffer(pPropName, pOutBuffer))
            {
                // XXXND  Make some utility functions for doing this...
                if( pOutBuffer->GetSize() == pInBuffer->GetSize() )
                {
                    if( ::memcmp( pOutBuffer->GetBuffer(), pInBuffer->GetBuffer(), pOutBuffer->GetSize() ) )
                    {
                        goto notFoundexit;
                    }
                }
            }
            else
            {
                goto notFoundexit;
            }

            HX_RELEASE(pInBuffer);
            HX_RELEASE(pOutBuffer);
        }
    }

    return TRUE;    // we made it!

  notFoundexit:
    HX_RELEASE(pInBuffer);
    HX_RELEASE(pOutBuffer);
    return FALSE;
}

void    Plugin2Handler::Plugin::SetDLL(PluginDLL * pPluginDll)
{
    m_pPluginDLL = pPluginDll;
    m_pPluginDLL->AddRef();

    IHXBuffer* pBuffer = pPluginDll->GetFileName();
    HX_ASSERT(pBuffer);

    m_pValues->SetPropertyCString(PLUGIN_FILENAME, pBuffer);
    HX_RELEASE(pBuffer);
}

void    Plugin2Handler::Plugin::SetIndex(UINT16 nIndex)
{
    m_nPluginIndex = nIndex;
    m_pValues->SetPropertyULONG32(PLUGIN_INDEX, nIndex);
}


void    Plugin2Handler::Plugin::SetPropertyULONG32(char* pName, char* pValue)
{
    if (m_pValues)
    {
        m_pValues->SetPropertyULONG32(pName, atoi(pValue));
    }
}

void    Plugin2Handler::Plugin::SetPropertyCString(char* pName, char* pValue)
{
    if (m_pValues)
    {
        IHXBuffer* pTempBuffer = NULL;
        if (HXR_OK == CreateAndSetBufferCCF(pTempBuffer, (UCHAR*)pValue, 
                                            strlen(pValue)+1, m_pContext))
        {
            m_pValues->SetPropertyCString(pName, pTempBuffer);
            HX_RELEASE(pTempBuffer);
        }
    }
}

void    Plugin2Handler::Plugin::SetPropertyBuffer(char* pName, BYTE* pData, UINT32 size )
{
    if (m_pValues)
    {
        // XXXND  FIX  THis really shouldn't have to do this copy
        IHXBuffer* pTempBuffer = NULL;
        if (HXR_OK == CreateAndSetBufferCCF(pTempBuffer, pData, size, m_pContext))
        {
            m_pValues->SetPropertyBuffer(pName, pTempBuffer);
            HX_RELEASE(pTempBuffer);
        }
    }
}

HX_RESULT Plugin2Handler::Plugin::WritePref2(REF(CPluginInfoWriter) piw)
{
    /*  format of data in regestry is as follows:
     *  {ValueName~ N|S|BValue~ ValueName~ N|S|B~ ValueName~ N|S|B~ etc},
     *  {etc}
     */

    // check to see if the DLL we are associated with still exists
    if (!m_pPluginDLL->DoesExist())
    {
        piw.Write("");
        return HXR_OK;
    }

    piw.Write("{");

    HXBOOL bFirst = TRUE;

    // Write out data...
    char szScratchPad[100]; /* Flawfinder: ignore */
    const char* pPropertyName;
    ULONG32 uPropertyValue;
    IHXBuffer* pBuffer;

    if(HXR_OK == m_pValues->GetFirstPropertyULONG32(pPropertyName, uPropertyValue))
    {
        if (!bFirst)
        {
            piw.Write("~");
        }
        else
        {
            bFirst = FALSE;
        }

        piw.Write(pPropertyName);
        itoa(uPropertyValue, szScratchPad, 10);
        piw.Write("~N");
        piw.Write(szScratchPad);

        while (HXR_OK == m_pValues->GetNextPropertyULONG32(pPropertyName, uPropertyValue))
        {
            if (!bFirst)
            {
                piw.Write("~");
            }
            else
            {
                bFirst = FALSE;
            }

            piw.Write(pPropertyName);
            itoa(uPropertyValue, szScratchPad, 10);
            piw.Write("~N");
            piw.Write(szScratchPad);
        }
    }

    if (HXR_OK == m_pValues->GetFirstPropertyCString(pPropertyName, pBuffer))
    {
        /* add a S to the begining for CString */
        if (!bFirst)
        {
            piw.Write("~");
        }
        else
        {
            bFirst = FALSE;
        }

        piw.Write(pPropertyName);
        piw.Write("~S");
        piw.Write((const char*) pBuffer->GetBuffer());
        HX_RELEASE(pBuffer);

        while (HXR_OK == m_pValues->GetNextPropertyCString(pPropertyName, pBuffer))
        {
            if (!bFirst)
            {
                piw.Write("~");
            }
            else
            {
                bFirst = FALSE;
            }
            piw.Write(pPropertyName);
            piw.Write("~S");
            piw.Write((const char*) pBuffer->GetBuffer());
            HX_RELEASE(pBuffer);
        }
    }

    if (HXR_OK == m_pValues->GetFirstPropertyBuffer(pPropertyName, pBuffer))
    {
        UINT32 size = 0;
        IHXBuffer *pBuf = NULL;

        /* add a B to the begining for Buffer */
        if (!bFirst)
        {
            piw.Write("~");
        }
        else
        {
            bFirst = FALSE;
        }

        piw.Write(pPropertyName);

#if 0   // XXXHP do we have other plugins besides Media Playback Engine plugin
        //       write buffer to preference? 
        // XXXND  This is for backwards compatibility
        if( GetDLL()->GetMountPoint()->IsHXCompliant() )
        {
            piw.Write("~B");
            CHXString tmp((const char*) pBuffer->GetBuffer(), pBuffer->GetSize());
            piw.Write( tmp );
        }
        else
#endif
        {
            size = pBuffer->GetSize();
            CreateBufferCCF(pBuf, m_pContext);
            pBuf->SetSize(size * 2);

            INT32 s = BinTo64(pBuffer->GetBuffer(), size, (char *)pBuf->GetBuffer());

            HX_ASSERT(size * 2 >= (UINT32)s);

            // Write out data.  s includes the NULL byte.
            piw.Write("~X");
            piw.Write( (const char*) pBuf->GetBuffer(), s - 1 );
        }

        HX_RELEASE(pBuffer);
        HX_RELEASE(pBuf);

        while (HXR_OK == m_pValues->GetNextPropertyBuffer(pPropertyName, pBuffer))
        {
            if (!bFirst)
            {
                piw.Write("~");
            }
            else
            {
                bFirst = FALSE;
            }

            piw.Write(pPropertyName);

#if 0       // XXXHP do we have other plugins besides Media Playback Engine plugin
            //       write buffer to preference? 
            // XXXND  This is for backwards compatibility
            if( GetDLL()->GetMountPoint()->IsHXCompliant() )
            {
                piw.Write("~B");
                CHXString tmp((const char*) pBuffer->GetBuffer(), pBuffer->GetSize());
                piw.Write( tmp );
            }
            else
#endif
            {
                size = pBuffer->GetSize();
                CreateBufferCCF(pBuf, m_pContext);
                HX_ASSERT(pBuf);
                pBuf->SetSize(size * 2);

                INT32 s = BinTo64(pBuffer->GetBuffer(), size, (char *)pBuf->GetBuffer());

                HX_ASSERT(size * 2 >= (UINT32)s);

                // Write out data.  s includes the NULL byte.
                piw.Write("~X");
                piw.Write( (const char*) pBuf->GetBuffer(), s - 1 );
            }

            HX_RELEASE(pBuffer);
            HX_RELEASE(pBuf);
        }
    }

    piw.Write("}");
    return HXR_OK;
}

HX_RESULT Plugin2Handler::Plugin::WritePref(PreferenceEnumerator* pPrefEnumParam)
{
    PreferenceEnumerator* pPrefEnum = (PreferenceEnumerator*)pPrefEnumParam;
    const char* pPropertyName=NULL;
    ULONG32     uPropertyValue;
    IHXBuffer* pBuffer=NULL;
    IHXBuffer* pOutBuffer=NULL;
    char        pPrefValue[(1<<8)]; /* Flawfinder: ignore */

    CreateBufferCCF(pOutBuffer, m_pContext);

    if (HXR_OK == m_pValues->GetFirstPropertyULONG32(pPropertyName, uPropertyValue))
    {
        /* add a N to the begining for Number */
        sprintf(pPrefValue, "N%d", (int) uPropertyValue); /* Flawfinder: ignore */
        pOutBuffer->Set((UCHAR*)pPrefValue, strlen(pPrefValue)+1);
        pPrefEnum->WriteSubPref(pPropertyName, pOutBuffer);
        while (HXR_OK == m_pValues->GetNextPropertyULONG32(pPropertyName, uPropertyValue))
        {
            sprintf(pPrefValue, "N%d", (int)uPropertyValue); /* Flawfinder: ignore */
            pOutBuffer->Set((UCHAR*)pPrefValue, strlen(pPrefValue)+1);
            pPrefEnum->WriteSubPref(pPropertyName, pOutBuffer);
        }
    }
    HX_RELEASE(pOutBuffer);
    if (HXR_OK == m_pValues->GetFirstPropertyCString(pPropertyName, pBuffer))
    {
        /* add a S to the begining for CString */
        CHXString NewString = "S";
        NewString += (char*) pBuffer->GetBuffer();

        IHXBuffer* pTempBuffer = NULL;
        if (HXR_OK == CreateAndSetBufferCCF(pTempBuffer, (UCHAR*)(const char*)NewString, 
                                            NewString.GetLength()+1, m_pContext))
        {
            pPrefEnum->WriteSubPref(pPropertyName, pTempBuffer);
            HX_RELEASE(pTempBuffer);
        }
        HX_RELEASE(pBuffer);

        while (HXR_OK == m_pValues->GetNextPropertyCString(pPropertyName, pBuffer))
        {
            NewString = "S";
            NewString += (char*) pBuffer->GetBuffer();

            IHXBuffer* pTempBuffer = NULL;
            if (HXR_OK == CreateAndSetBufferCCF(pTempBuffer, (UCHAR*)(const char*)NewString, 
                                                NewString.GetLength()+1, m_pContext))
            {
                pPrefEnum->WriteSubPref(pPropertyName, pTempBuffer);
                HX_RELEASE(pTempBuffer);
            }
            HX_RELEASE(pBuffer);
        }
    }
    if (HXR_OK == m_pValues->GetFirstPropertyBuffer(pPropertyName, pBuffer))
    {
        /*  add a B to the begining for Buffer -- although someone would have to have rocks in
            his head if he attempted to write a buffer to the reg...
        */
        UCHAR* pTempChar = new UCHAR[pBuffer->GetSize()+2];
        *pTempChar='B';
        memcpy(pTempChar+1, pBuffer->GetBuffer(), pBuffer->GetSize()); /* Flawfinder: ignore */
        *(pTempChar+pBuffer->GetSize()+1) = 0;

        IHXBuffer* pTempBuffer = NULL;
        if (HXR_OK == CreateAndSetBufferCCF(pTempBuffer, pTempChar, 
                                            pBuffer->GetSize()+2, m_pContext))
        {
            pPrefEnum->WriteSubPref(pPropertyName, pTempBuffer);
            HX_RELEASE(pTempBuffer);
        }
        HX_VECTOR_DELETE(pTempChar);
        HX_RELEASE(pBuffer);

        while (HXR_OK == m_pValues->GetNextPropertyBuffer(pPropertyName, pBuffer))
        {
            pTempChar = new UCHAR[pBuffer->GetSize()+2];
            *pTempChar='B';
            memcpy(pTempChar+1, pBuffer->GetBuffer(), pBuffer->GetSize()); /* Flawfinder: ignore */
            *(pTempChar+pBuffer->GetSize()+1) = 0;

            IHXBuffer* pTempBuffer = NULL;
            if (HXR_OK == CreateAndSetBufferCCF(pTempBuffer, pTempChar, 
                                                pBuffer->GetSize()+2, m_pContext))
            {
                pPrefEnum->WriteSubPref(pPropertyName, pTempBuffer);
                HX_RELEASE(pTempBuffer);
            }
            HX_VECTOR_DELETE(pTempChar);
            HX_RELEASE(pBuffer);
        }
    }
    return HXR_OK;
}


HX_RESULT Plugin2Handler::Plugin::ReadPref(PreferenceEnumerator* pPrefEnum)
{
    UINT32                      nIndex      = 0;
    IHXBuffer*                  pPropName   = 0;
    IHXBuffer*                  pBuffer     = 0;

    // this function assumes that the enumerator has been properly set up before it is
    // called.

    while (HXR_OK == pPrefEnum->GetPrefKey(nIndex, pPropName))
    {
        if (!strcmp((char*)pPropName->GetBuffer(), zm_pszKeyNameRegKey))
        {
            HX_RELEASE(pPropName);
            nIndex++;
            continue;
        }

        pPrefEnum->ReadPref((const char*)pPropName->GetBuffer(), pBuffer);
        if (pBuffer)
        {
            char* pCharBuffer = (char*)pBuffer->GetBuffer();

            switch (*pCharBuffer)
            {
               case 'N':
               {
                   m_pValues->SetPropertyULONG32((const char*)pPropName->GetBuffer(), atoi(pCharBuffer+1));
                   break;
               }
               case 'S':
               {
                   IHXBuffer* pTempBuffer = NULL;
                   if (HXR_OK == CreateAndSetBufferCCF(pTempBuffer, pBuffer->GetBuffer()+1, 
                                                       pBuffer->GetSize()-1, m_pContext))
                   {
                       m_pValues->SetPropertyCString((const char*)pPropName->GetBuffer(), pTempBuffer);
                       HX_RELEASE(pTempBuffer);
                   }
                   break;
               }
               case 'B':
               {
                   IHXBuffer* pTempBuffer = NULL;
                   if (HXR_OK == CreateAndSetBufferCCF(pTempBuffer, pBuffer->GetBuffer()+1,
                                                       pBuffer->GetSize()-1, m_pContext))
                   {
                       m_pValues->SetPropertyBuffer((const char*)pPropName->GetBuffer(), pTempBuffer);
                       HX_RELEASE(pTempBuffer);
                   }
                   break;
               }
               case 'X':
               {
                   IHXBuffer* pBuf = NULL;
                   CreateBufferCCF(pBuf, m_pContext);

                   UINT32 size = pBuffer->GetSize();
                   pBuf->SetSize(size);

                   // We subtract 2 from size - one for NULL terminator and 1 because we move foward in the buffer by 1
                   INT32 s = BinFrom64((char *)pBuffer->GetBuffer()+1, size - 2, pBuf->GetBuffer());

                   HX_ASSERT(s != -1);
                   HX_ASSERT((UINT32) s <= size);

                   m_pValues->SetPropertyBuffer((const char*)pPropName->GetBuffer(), pBuf);
                   HX_RELEASE(pBuf);
                   break;
               }
            }
        }
        HX_RELEASE(pBuffer);
        HX_RELEASE(pPropName);
        nIndex++;
    }

    ULONG32 nTemp;

    if (HXR_OK == m_pValues->GetPropertyULONG32(PLUGIN_INDEX, nTemp))
    {
        m_nPluginIndex = (UINT16) nTemp;
    }

    return HXR_OK;
}

HXBOOL Plugin2Handler::Plugin::AreBufferEqual(IHXBuffer* pBigBuff,
                                              IHXBuffer* pSmallBuff)
{
    char*   pTemp;
    HXBOOL    bRetVal = FALSE;

    pTemp = new char[pBigBuff->GetSize()];
    SafeStrCpy(pTemp, (char*)pBigBuff->GetBuffer(), pBigBuff->GetSize());

    char* token;
    token = strtok(pTemp, zm_pszValueSeperator);
    while (token)
    {
        CHXString tokenCHXstring;
        CHXString smallCHXstring;

        tokenCHXstring = token;
        smallCHXstring = (char*)pSmallBuff->GetBuffer();
        tokenCHXstring.TrimLeft();
        tokenCHXstring.TrimRight();
        smallCHXstring.TrimLeft();
        smallCHXstring.TrimRight();

        if (!strcasecmp(tokenCHXstring, smallCHXstring))
        {
            bRetVal = TRUE;
            break;
        }
        token = strtok(NULL, zm_pszValueSeperator);
    }
    delete[] pTemp;

    return bRetVal;
}

Plugin2Handler::Errors Plugin2Handler::Plugin::GetValuesFromDLL(IHXPlugin* pHXPlugin)
{
    Plugin2Handler::Errors  retVal;

    retVal = GetBasicValues(pHXPlugin);
    if (retVal == NO_ERRORS)
    {
        retVal = GetExtendedValues(pHXPlugin);
    }
    return retVal;
}

Plugin2Handler::Errors Plugin2Handler::Plugin::GetPlugin(REF(IUnknown*) pUnknown )
{
    pUnknown = NULL;
    Plugin2Handler::Errors retVal = NO_ERRORS;

    if (!m_pPluginDLL)
    {
        return PLUGIN_NOT_FOUND;
    }
    if (!m_pPluginDLL->IsLoaded())
    {
        if (NO_ERRORS != (retVal = m_pPluginDLL->Load(m_pContext)))
        {
            return retVal;
        }
    }
    if (HXR_OK != m_pPluginDLL->CreateInstance(&pUnknown, m_nPluginIndex))
    {
        return CREATE_INSTANCHXR_FAILURE;
    }

//    CreateWatcher(pUnknown);      //XXXAH the watcher is no longer being used
    return retVal;
}


Plugin2Handler::Errors Plugin2Handler::Plugin::GetInstance(REF(IUnknown*) pUnknown, IUnknown* pIUnkOuter )
{
    // Initialize out parameter
    pUnknown = NULL;

    IUnknown* pIUnkPlugin = NULL;
    Plugin2Handler::Errors retVal = GetPlugin( pIUnkPlugin );
    if( retVal == NO_ERRORS )
    {
        // Does the component plugin support IHXPlugin?
        IHXPlugin* pIHXPlugin = NULL;
        HX_RESULT rv = pIUnkPlugin->QueryInterface(IID_IHXPlugin, (void**) &pIHXPlugin);
        if (SUCCEEDED(rv))
        {
            // Call IHXPlugin::InitPlugin on the component plugin
            pIHXPlugin->InitPlugin(m_pContext);
        }
        HX_RELEASE(pIHXPlugin);
        // Get the IHXComponentPlugin interface
        IHXComponentPlugin* pIComp = NULL;
        if( SUCCEEDED( pIUnkPlugin->QueryInterface( IID_IHXComponentPlugin, (void**) &pIComp ) ) )
        {
            // Ask for the correct object by CLSID
            IHXBuffer* pCLSID = NULL;
            if( SUCCEEDED( m_pValues->GetPropertyBuffer( PLUGIN_COMPONENT_CLSID, pCLSID ) ) )
            {
                if( FAILED( pIComp->CreateComponentInstance( *(GUID*) pCLSID->GetBuffer(), pUnknown, pIUnkOuter ) ) )
                {
                    retVal = CREATE_INSTANCHXR_FAILURE;
                }
                HX_RELEASE( pCLSID );
            }
            else
            {
                // Hmmm...we have a component plugin without a CLSID.  Serious internal error
                retVal = BAD_PLUGIN;
            }

            // Release the interface, and destroy the plugin
            HX_RELEASE( pIComp );
            HX_RELEASE( pIUnkPlugin );
        }
        else
        {
            // If this isn't a component plugin, then we can't aggregate anything
            if( pIUnkOuter )
            {
                HX_RELEASE( pIUnkPlugin );
                retVal = AGGREGATION_NOT_SUPPORTED;
            }
            else
            {
                pUnknown = pIUnkPlugin;
            }
        }
    }

    return retVal;
}

STDMETHODIMP Plugin2Handler::Plugin::AllObjectsDeleted  (void*)
{
    //HX_RELEASE(m_pPluginWatcher);
    //m_pPluginWatcher = 0;
    m_pPluginDLL->ReleaseDLLReference();
    return HXR_OK;
}

Plugin2Handler::Errors
Plugin2Handler::Plugin::CreateWatcher(IUnknown* pUnknown)
{
    return NO_ERRORS;   //XXXAH the watcher is no longer being used

#if 0
    if (!m_pPluginWatcher)
    {
        IHXPluginWatcherResponse* pWatcherResp;
//      QueryInterface(IID_IHXPluginWatcherResponse, (void**)&pWatcherResp);  // causes the build to break
        m_pPluginWatcher = new PluginMonitor(pUnknown, pWatcherResp);
        m_pPluginWatcher->AddRef();
        HX_ASSERT(m_pPluginDLL); // Huh?
        m_pPluginDLL->AddDLLReference();
    }
    return NO_ERRORS;
#endif
}

HXBOOL  Plugin2Handler::Plugin::IsLoaded()
{
    if (!m_pPluginDLL)
        return FALSE;

    return m_pPluginDLL->IsLoaded();
}

HX_RESULT Plugin2Handler::Plugin::GetPluginInfo(REF(IHXValues*) pVals)
{
    if (m_pValues)
    {
        pVals = m_pValues;
        return HXR_OK;
    }
    pVals = NULL;
    return HXR_FAIL;
}

IHXBuffer* Plugin2Handler::Plugin::GetFileName()
{
    IHXBuffer* retVal = NULL;

    // Get the filename from m_pValues.  We can't get it from the DLL,
    // because we may be in the process of loading, and we just got it
    // from the preferences
    if( m_pValues )
    {
        m_pValues->GetPropertyCString( PLUGIN_FILENAME, retVal );
    }

    return retVal;
}

Plugin2Handler::Errors
Plugin2Handler::Plugin::GetBasicValues(IHXPlugin* pHXPlugin)
{
    const char* pszDescription = NULL;
    const char* pszCopyright = NULL;
    const char* pszMoreInfoUrl = NULL;
    ULONG32     ulVersionNumber = 0;
    HXBOOL      nload_multiple = 0;

    if (HXR_OK != pHXPlugin->GetPluginInfo(nload_multiple, pszDescription,
                                           pszCopyright, pszMoreInfoUrl, ulVersionNumber))
    {
        return BAD_PLUGIN;
    }

    IHXBuffer* pBuffer = NULL;

    CreateBufferCCF(pBuffer, m_pContext);
    if (pszDescription)
    {
        pBuffer->Set((UCHAR*)pszDescription, strlen(pszDescription)+1);
    }
    m_pValues->SetPropertyCString(PLUGIN_DESCRIPTION2, pBuffer);
    HX_RELEASE(pBuffer);

    CreateBufferCCF(pBuffer, m_pContext);
    if (pszCopyright)
    {
        pBuffer->Set((UCHAR*)pszCopyright, strlen(pszCopyright)+1);
    }
    m_pValues->SetPropertyCString(PLUGIN_COPYRIGHT2, pBuffer);
    HX_RELEASE(pBuffer);

    CreateBufferCCF(pBuffer, m_pContext);
    if (pszMoreInfoUrl)
    {
        pBuffer->Set((UCHAR*)pszMoreInfoUrl, strlen(pszMoreInfoUrl)+1);
    }
    m_pValues->SetPropertyCString(PLUGIN_COPYRIGHT, pBuffer);
    HX_RELEASE(pBuffer);

    m_pValues->SetPropertyULONG32(PLUGIN_LOADMULTIPLE, nload_multiple);
    m_pValues->SetPropertyULONG32(PLUGIN_VERSION, ulVersionNumber);
    return NO_ERRORS;
}


Plugin2Handler::Errors
Plugin2Handler::Plugin::GetExtendedValues(IHXPlugin* pHXPlugin)
{
//    Errors                            result              = NO_ERRORS;
    IHXFileFormatObject*                pFileFormat         = NULL;
//    IHXMetaFileFormatObject*          pMetaFileFormat     = NULL;
    IHXFileWriter*                      pFileWriter         = NULL;
    IHXBroadcastFormatObject*           pBroadcastFormat    = NULL;
    IHXFileSystemObject*                pFileSystem         = NULL;
    IHXRenderer*                        pRenderer           = NULL;
    IHXDataRevert*                      pDataRevert         = NULL;
    IHXStreamDescription*               pStreamDescription  = NULL;
    IHXPlayerConnectionAdviseSink*      pAllowanceFormat    = NULL;
    IHXCommonClassFactory*              pClassFactory       = NULL;
    IHXPluginProperties*                pIHXPluginPropertiesThis = NULL;
    UINT32                              nCountInterfaces    = 0;


    // file system
    if (HXR_OK == pHXPlugin->QueryInterface(IID_IHXFileSystemObject, (void**) &pFileSystem))
    {
        const char* pszShortName;
        const char* pszProtocol;

        if (HXR_OK != pFileSystem->GetFileSystemInfo(pszShortName, pszProtocol))
        {
            HX_RELEASE (pFileSystem);
            return  CANT_GET_RENDERER_INFO; //XXXAH Cleanup?
        }

        SetCStringPropertyCCF(m_pValues, PLUGIN_CLASS, PLUGIN_FILESYSTEM_TYPE, m_pContext);

        SetCStringPropertyCCF(m_pValues, PLUGIN_FILESYSTEMSHORT, pszShortName, m_pContext);

        SetCStringPropertyCCF(m_pValues, PLUGIN_FILESYSTEMPROTOCOL, pszProtocol, m_pContext);

        pFileSystem->Release();
        nCountInterfaces++;
    }

    // file format
    if (HXR_OK == pHXPlugin->QueryInterface(IID_IHXFileFormatObject, (void**)&pFileFormat) ||
        HXR_OK == pHXPlugin->QueryInterface(IID_IHXFileWriter, (void**)&pFileWriter))
    {
        // fine we are in now we will get the correct type.
        if (pFileFormat)
        {
            pFileFormat->Release();
        }
        else
        {
            pFileWriter->Release();
        }

        const char**            ppszMimeTypes = NULL;
        const char**            ppszExtensions = NULL;
        const char**            ppszOpenNames = NULL;

        if (HXR_OK == pHXPlugin->QueryInterface(IID_IHXFileFormatObject, (void**)&pFileFormat))
        {
            pFileFormat->GetFileFormatInfo( ppszMimeTypes,
                                            ppszExtensions,
                                            ppszOpenNames);
            pFileFormat->Release();
            
            SetCStringPropertyCCF(m_pValues, PLUGIN_CLASS, PLUGIN_FILEFORMAT_TYPE, m_pContext);
        }

        if (HXR_OK == pHXPlugin->QueryInterface(IID_IHXFileWriter, (void**)&pFileWriter))
        {
            pFileWriter->GetFileFormatInfo( ppszMimeTypes,
                                            ppszExtensions,
                                            ppszOpenNames);
            pFileWriter->Release();

            SetCStringPropertyCCF(m_pValues, PLUGIN_CLASS, PLUGIN_FILEWRITER_TYPE, m_pContext);
        }

        IHXBuffer* pBuffer = NULL;
        if (ppszMimeTypes)
        {
            CatStringsCCF(pBuffer, ppszMimeTypes, zm_pszValueSeperator, m_pContext);
            m_pValues->SetPropertyCString(PLUGIN_FILEMIMETYPES, pBuffer);
            HX_RELEASE(pBuffer);
        }

        if (ppszExtensions)
        {
            CatStringsCCF(pBuffer, ppszExtensions, zm_pszValueSeperator, m_pContext);
            m_pValues->SetPropertyCString(PLUGIN_FILEEXTENSIONS, pBuffer);
            HX_RELEASE(pBuffer);
        }

        if (ppszOpenNames)
        {
            CatStringsCCF(pBuffer, ppszOpenNames, zm_pszValueSeperator, m_pContext);
            m_pValues->SetPropertyCString(PLUGIN_FILEOPENNAMES, pBuffer);
            HX_RELEASE(pBuffer);
        }
        nCountInterfaces++;
    }

    // renderer
    if (HXR_OK == pHXPlugin->QueryInterface(IID_IHXRenderer, (void**)&pRenderer))
    {
        const char** ppszMimeTypes = NULL;
        UINT32  initial_granularity = 0;

        // get the basic info
        if (HXR_OK == pRenderer->GetRendererInfo(ppszMimeTypes, initial_granularity))
        {
            IHXBuffer* pBuffer = NULL;;
            if (ppszMimeTypes)
            {
                CatStringsCCF(pBuffer, ppszMimeTypes, zm_pszValueSeperator, m_pContext);
            }
            m_pValues->SetPropertyCString(PLUGIN_RENDERER_MIME, pBuffer);
            HX_RELEASE(pBuffer);
            m_pValues->SetPropertyULONG32(PLUGIN_RENDERER_GRANULARITY, initial_granularity);

            SetCStringPropertyCCF(m_pValues, PLUGIN_CLASS, PLUGIN_RENDERER_TYPE, m_pContext);
        }
        HX_RELEASE(pRenderer);
        nCountInterfaces++;
    }

    //data revert
    if (HXR_OK == pHXPlugin->QueryInterface(IID_IHXDataRevert, (void**)&pDataRevert))
    {
        const char** ppConversionTypes = NULL;

        if (HXR_OK == pDataRevert->GetDataRevertInfo(ppConversionTypes))
        {
            IHXBuffer* pBuffer = NULL;
            CatStringsCCF(pBuffer, ppConversionTypes, zm_pszValueSeperator, m_pContext);
            m_pValues->SetPropertyCString(PLUGIN_REVERTER_MIME, pBuffer);
            HX_RELEASE(pBuffer);

            SetCStringPropertyCCF(m_pValues, PLUGIN_CLASS, PLUGIN_REVERTER_TYPE, m_pContext);
        }
        HX_RELEASE(pDataRevert);
        nCountInterfaces++;
    }

    // broadcast
    if (HXR_OK == pHXPlugin->QueryInterface(IID_IHXBroadcastFormatObject, (void**)&pBroadcastFormat))
    {
        const char* pszBroadcastType;

        if (HXR_OK != pBroadcastFormat->GetBroadcastFormatInfo(pszBroadcastType))
        {
            HX_RELEASE (pBroadcastFormat);
            return CANT_GET_FILE_FORMAT_INFO; //XXXAH Cleanup?
        }
        pBroadcastFormat->Release();

        SetCStringPropertyCCF(m_pValues, PLUGIN_BROADCASTTYPE, pszBroadcastType, m_pContext);

        SetCStringPropertyCCF(m_pValues, PLUGIN_CLASS, PLUGIN_BROADCAST_TYPE, m_pContext);

        nCountInterfaces++;
    }

    // stream description
    if (HXR_OK == pHXPlugin->QueryInterface(IID_IHXStreamDescription, (void**)&pStreamDescription))
    {
        const char* pszMimeType;
        if (HXR_OK != pStreamDescription->GetStreamDescriptionInfo(pszMimeType))
        {
            HX_RELEASE (pStreamDescription);
            return CANT_GET_FILE_FORMAT_INFO;   // XXXAH Cleanup?
        }
        pStreamDescription->Release();

        SetCStringPropertyCCF(m_pValues, PLUGIN_STREAMDESCRIPTION, pszMimeType, m_pContext);

        SetCStringPropertyCCF(m_pValues, PLUGIN_CLASS, PLUGIN_STREAM_DESC_TYPE, m_pContext);
        nCountInterfaces++;
    }
    // allowance
    if ( (HXR_OK == pHXPlugin->QueryInterface(IID_IHXPlayerConnectionAdviseSinkManager, (void**)&pAllowanceFormat)) ||
         (HXR_OK == pHXPlugin->QueryInterface(IID_IHXPlayerConnectionAdviseSink, (void**)&pAllowanceFormat)) )
    {
        SetCStringPropertyCCF(m_pValues, PLUGIN_CLASS, PLUGIN_ALLOWANCE_TYPE, m_pContext);

        pAllowanceFormat->Release();
        nCountInterfaces++;
    }

    // common class factory
    if(HXR_OK == pHXPlugin->QueryInterface(IID_IHXCommonClassFactory,
                                           (void**)&pClassFactory))
    {
        SetCStringPropertyCCF(m_pValues, PLUGIN_CLASS, PLUGIN_CLASS_FACTORY_TYPE, m_pContext);

        HX_RELEASE (pClassFactory);
        nCountInterfaces++;
    }

    // NO MORE NEW PLUGIN INFORMATION INTERFACES!!!!
    //
    // THIS IS THE LAST ONE!!!!!
    if( SUCCEEDED( pHXPlugin->QueryInterface( IID_IHXPluginProperties, (void**)&pIHXPluginPropertiesThis ) ) )
    {
        IHXValues* pIHXValuesProperties = NULL;

        pHXPlugin->InitPlugin(m_pContext);

        if( SUCCEEDED( pIHXPluginPropertiesThis->GetProperties( pIHXValuesProperties ) ) && pIHXValuesProperties )
        {
            CHXHeader::mergeHeaders( m_pValues, pIHXValuesProperties );
        }

        HX_RELEASE(pIHXValuesProperties);

        // XXXkshoop Let this coincide with other interfaces.. for now..
        //nCountInterfaces++;
    }

    HX_RELEASE(pIHXPluginPropertiesThis);

    HX_ASSERT(nCountInterfaces<2);
    return NO_ERRORS;
}


HX_RESULT Plugin2Handler::Plugin::GetBandwidthInfo()
{

    IHXPlugin* pHXPlugin;
    IUnknown*   pUnk;

    if (HXR_OK != GetInstance(pUnk))
    {
        return HXR_FAIL;
    }

    if (HXR_OK != pUnk->QueryInterface(IID_IHXPlugin, (void**)& pHXPlugin))
    {
        return HXR_FAIL;
    }

    HX_RELEASE(pUnk);

    pHXPlugin->InitPlugin(m_pContext);

    IHXBandwidthLister* bandwidth_lister;
    if(HXR_OK == pHXPlugin->QueryInterface(IID_IHXBandwidthLister,
                                           (void**)&bandwidth_lister))
    {
        IHXValues* pValues = NULL;
        
        CreateValuesCCF(pValues, m_pContext);
        if(HXR_OK == bandwidth_lister->GetBandwidthInfo(pValues))
        {
            // now that we have these new values we must transcribe them to the
            // m_pValues.
            const char*     pValueName;
            ULONG32         nPropValue;
            IHXBuffer*      pPropValue;

            if (HXR_OK == pValues->GetFirstPropertyULONG32(pValueName, nPropValue))
            {
                m_pValues->SetPropertyULONG32(pValueName, nPropValue);
                while (HXR_OK == pValues->GetNextPropertyULONG32(pValueName, nPropValue))
                {
                    m_pValues->SetPropertyULONG32(pValueName, nPropValue);
                }
            }
            if (HXR_OK == pValues->GetFirstPropertyBuffer(pValueName, pPropValue))
            {
                m_pValues->SetPropertyBuffer(pValueName, pPropValue);
                pPropValue->Release();
                while (HXR_OK == pValues->GetNextPropertyBuffer(pValueName, pPropValue))
                {
                    m_pValues->SetPropertyBuffer(pValueName, pPropValue);
                    pPropValue->Release();
                }
            }
            if (HXR_OK == pValues->GetFirstPropertyCString(pValueName, pPropValue))
            {
                m_pValues->SetPropertyCString(pValueName, pPropValue);
                pPropValue->Release();
                while (HXR_OK == pValues->GetNextPropertyCString(pValueName, pPropValue))
                {
                    m_pValues->SetPropertyCString(pValueName, pPropValue);
                    pPropValue->Release();
                }
            }
        }
        HX_RELEASE(bandwidth_lister);
        HX_RELEASE(pValues);
        HX_RELEASE(pHXPlugin);
        m_bInfoNeedsRefresh = FALSE;
        return HXR_OK;
    }
    HX_RELEASE(pHXPlugin);
    return HXR_FAIL;
}


void Plugin2Handler::Plugin::InitializeComponentPlugin( IHXPlugin* pIPlugin, IHXValues* pIValues )
{
    // Setup basic data
    // XXXHP - this is unnecessary information as it is stored on a PER COMPONENT not PER PLUGIN basis in this case.
    // GetBasicValues( pIPlugin );

    // Copy data from pIValues
    CHXHeader::mergeHeaders( m_pValues, pIValues );
}



HX_RESULT Plugin2Handler::Plugin::CatPropertiesULONG32(REF(IHXBuffer*) pBuffer,
                                                       const char* pPropName,
                                                       ULONG32 nValue)
{
    CHXString NewString;
    const char* pchar;
    ULONG32 nLen;

    if (pBuffer)
    {
        pBuffer->Get((UCHAR*&)pchar, nLen);
        NewString=pchar;
        pBuffer->Release();
    }
    else
    {
        NewString = "";
    }

    NewString = NewString  + Plugin2Handler::zm_pszListStart + pPropName + " = ";
    NewString.AppendULONG(nValue);
    NewString += Plugin2Handler::zm_pszListEnd;

    return CreateAndSetBufferCCF(pBuffer, (UCHAR*)(const char*)NewString, 
                                 NewString.GetLength()+1, m_pContext);
}


HX_RESULT Plugin2Handler::Plugin::CatPropertiesCString(REF(IHXBuffer*) pBuffer,
                                                       const char* pPropName,
                                                       IHXBuffer* pValue)
{
    CHXString       NewString;
    const char*     pchar;
    ULONG32         nLen;

    if (pBuffer)
    {
        pBuffer->Get((UCHAR*&)pchar, nLen);
        NewString=pchar;
        pBuffer->Release();
    }
    else
    {
        NewString ="";
    }

    pValue->Get((UCHAR*&)pchar, nLen);

    NewString = NewString + Plugin2Handler::zm_pszListStart + pPropName  + " = " + pchar + Plugin2Handler::zm_pszListEnd;

    return CreateAndSetBufferCCF(pBuffer, (UCHAR*)(const char*)NewString, 
                                 NewString.GetLength()+1, m_pContext);
}


/////////////////////////////////////////////////////////////////////////
//  Method:
//      Plugin2Handler::Plugin::QueryInterface
//  Purpose:
//      Implement this to export the interfaces supported by your
//      object.
//
STDMETHODIMP
Plugin2Handler::Plugin::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)this;
        return HXR_OK;
    }
//    else if (IsEqualIID(riid, IID_IHXPluginWatcherResponse))
//    {
//        AddRef();
//        *ppvObj = (IHXPluginWatcherResponse*)this;
//        return HXR_OK;
//    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//      Plugin2Handler::Plugin::AddRef
//  Purpose:
//      Everyone usually implements this the same... feel free to use
//      this implementation.
//
STDMETHODIMP_(ULONG32)
    Plugin2Handler::Plugin::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//      Plugin2Handler::Plugin::Release
//  Purpose:
//      Everyone usually implements this the same... feel free to use
//      this implementation.
//
STDMETHODIMP_(ULONG32)
    Plugin2Handler::Plugin::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}




/**********************************************************************************
 ***                 Plugin2Handler::PluginDLL                                   ***
 ***********************************************************************************/

Plugin2Handler::PluginDLL::PluginDLL( const char* pszFileName, PluginMountPoint* pMountPoint,
                                      Plugin2Handler* pPlugin2Handler )
    : m_fpCreateInstance(NULL)
    , m_fpShutdown(NULL)
    , m_fCanUnload(NULL)
    , m_pMountPoint( pMountPoint )
    , m_pFileName( NULL )
    , m_pNamespace( NULL )
    , m_nSizeBites(0)
    , m_lRefCount(0)
    , m_NumOfPlugins(0)
    , m_pDLLAccess(NULL)
    , m_bHas_factory(FALSE)
    , m_bLoaded(FALSE)
    , m_nActiveReferences(0)
    , m_pPlugin2Handler(pPlugin2Handler)
    , m_bDoesExist(TRUE)
{
    // Always create an IHXBuffer and store the filename there
    if (pszFileName)
    {
        CreateAndSetBufferCCF(m_pFileName, (BYTE*) pszFileName, 
                              ::strlen( pszFileName )+1, m_pPlugin2Handler->m_pContext);
    }

    m_pDLLAccess = new DLLAccess();
}


Plugin2Handler::PluginDLL::~PluginDLL()
{
    HX_RELEASE( m_pFileName );
    HX_RELEASE( m_pNamespace );

    if (m_pDLLAccess)
    {
        if (m_bLoaded)
        {
            if (m_fpShutdown)
            {
                m_fpShutdown();
                m_fpShutdown = NULL;
            }

            m_pDLLAccess->close();
        }

        delete m_pDLLAccess;
        m_pDLLAccess = 0;
    }

    // Remove ourself from the plugin handler's CanUnload2DllList.
    LISTPOSITION posCanUnload = m_pPlugin2Handler->GetCanUnload2DllList().Find( this );
    if ( posCanUnload )
    {
        m_pPlugin2Handler->GetCanUnload2DllList().RemoveAt( posCanUnload );
    }
}


Plugin2Handler::Errors
Plugin2Handler::PluginDLL::Load(IUnknown* pContext)
{
    HX_LOG_BLOCK( "Plugin2Handler::PluginDLL::Load()" );

    Errors      result      = NO_ERRORS;

    IUnknown*   pInstance   = NULL;
    IHXPlugin* pPlugin = NULL;
    IHXPluginFactory* pIFactory = NULL;
    HX_RESULT createResult = HXR_FAIL;


    if (m_bLoaded)
    {
        return PLUGIN_ALREADY_HAS_MOUNT_POINT; //XXXAH Huh?
    }

    if( m_pFileName->GetSize() <= 1 )
    {
        return PLUGIN_NOT_FOUND;
    }

    // Build complete path for DLL
    IHXBuffer* pBuffer = m_pMountPoint->Path();

    CHXString fileNameWithPath = (char*) pBuffer->GetBuffer();
    UINT32 len = fileNameWithPath.GetLength();
    if(len &&
       fileNameWithPath.GetAt(len - 1) != Plugin2Handler::zm_cDirectorySeperator)
        fileNameWithPath += Plugin2Handler::zm_pszDirectorySeperator;
    fileNameWithPath += (char *) m_pFileName->GetBuffer();

    HX_RELEASE(pBuffer);

    // 1st load the DLL into memory
    HX_PRIME_ACCUMULATOR( 'pdll', "Total DLL Load Time" );
    int dllLoadResult = m_pDLLAccess->open(fileNameWithPath);
    HX_UPDATE_ACCUMULATOR( 'pdll' );

    if( dllLoadResult != DLLAccess::DLL_OK )
    {
#ifdef REALPLAYER_PLUGIN_HANDLER_RESEARCH_
    {
        // XXXHP -- although using HXLOG_ALERT below I woudld think would
        // notify the user (based on documentation), it apparently does not
        // // m_pPlugin2Handler->ReportError( HXLOG_ALERT, (char *)
        // m_pFileName->GetBuffer(), m_pDLLAccess->getErrorString() );
        CHXString errorReport;
        errorReport.Format ("Please contact Millie or Jeff Chasen immediately!\nThe DLL '%s' cannot be opened. ERROR: %s", m_pFileName->GetBuffer (), m_pDLLAccess->getErrorString ());
        LogCriticalError_ (errorReport);
        ::MessageBox (0, "Critical Error", errorReport, MB_OK);
    }
#else
    m_pPlugin2Handler->ReportError( HXLOG_DEBUG, (char *) m_pFileName->GetBuffer(), m_pDLLAccess->getErrorString() );
#endif
    return CANT_OPEN_DLL;
    }

    HX_LOG_CHECKPOINT( "DLL Loaded" );

    // Now look for the HXCreateInstance exported function
    m_fpCreateInstance = (FPCREATEINSTANCE) m_pDLLAccess->getSymbol(HXCREATEINSTANCESTR);
    if (NULL == m_fpCreateInstance)
    {
        m_pPlugin2Handler->ReportError( HXLOG_DEBUG, (char *) m_pFileName->GetBuffer(), "No " HXCREATEINSTANCESTR );
	result = NO_HX_CREATE_INSTANCE;
	goto cleanup;
    }

    // And look for HXShutdown exported function... not required.
    m_fpShutdown    = (FPSHUTDOWN) m_pDLLAccess->getSymbol(HXSHUTDOWNSTR);

    // and look for CanUnload2 exported function
    //JE 3/26/01: look for CanUnload2 instead of CanUnload. This way we will
    //not try and unload any DLLs that may have incorrectly implemented the old
    // CanUnload. If you implement CanUnload2, you better get it right ;)
    m_fCanUnload    = (FPSHUTDOWN) m_pDLLAccess->getSymbol("CanUnload2");
    if ( m_fCanUnload )
    {
        // This PluginDLL exports CanUnload2(), so add it to the list of such PluginDLL's.
        m_pPlugin2Handler->GetCanUnload2DllList().AddTail( this );
    }

    HX_LOG_CHECKPOINT( "Exported symbols found" );

    // Does this thing support the IHXPlugin Interface
    // Now we will test to see if the DLL contains multiple Plugins.
    HX_PRIME_ACCUMULATOR( 'plmk', "Total Plugin Allocation time" );
    createResult = m_fpCreateInstance( &pInstance );
    HX_UPDATE_ACCUMULATOR( 'plmk' );

    if( HXR_OK != createResult )
    {
        m_pPlugin2Handler->ReportError( HXLOG_DEBUG, (char *) m_pFileName->GetBuffer(), HXCREATEINSTANCESTR " Failure");
        result = CREATE_INSTANCHXR_FAILURE;
        goto cleanup;
    }


    HX_LOG_CHECKPOINT( "Plugin instance created" );

    // To be a valid plugin a DLL must support IHXPlugin
    // In addition, it may support IHXPluginFactory
    if( SUCCEEDED( pInstance->QueryInterface( IID_IHXPluginFactory, (void**) &pIFactory ) ) )
    {
        m_bHas_factory = TRUE;
        m_NumOfPlugins = pIFactory->GetNumPlugins();

        HX_RELEASE( pIFactory );
    }
    else if( SUCCEEDED( pInstance->QueryInterface( IID_IHXPlugin, (void**) &pPlugin ) ) )
    {
        m_bHas_factory = FALSE;
        m_NumOfPlugins = 1;

        IHXComponentPlugin* pIPackage = NULL;
        if (SUCCEEDED (pInstance->QueryInterface (IID_IHXComponentPlugin, (void**) &pIPackage)))
        {
            pPlugin->InitPlugin (pContext);
            m_packageName = pIPackage->GetPackageName ();
            HX_RELEASE (pIPackage);
        }

        HX_RELEASE( pPlugin );
    }
    else
    {
        result = BAD_PLUGIN;
        goto cleanup;
    }

    // We are now loaded.
    HX_RELEASE(pInstance);
    m_bLoaded = TRUE;

  cleanup:

    if(!m_bLoaded)
    {
	m_pDLLAccess->close();
    }

    HX_LOG_CHECKPOINT( "Plugin2Handler::PluginDLL::Load() exiting" );

    return result;
}

HX_RESULT Plugin2Handler::PluginDLL::WritePref2(REF(CPluginInfoWriter) piw)
{
    // the string is defined as follows:
    // {name, checksum, HXBOOL has factory, size, INT numplugins},{ditto}

    // get the checksum

    IHXBuffer* pPathBuffer = GetMountPoint()->Path();
    IHXBuffer*  pNewChecksum = m_pPlugin2Handler->ChecksumFile( (char *) m_pFileName->GetBuffer(), pPathBuffer);

    if (!pNewChecksum)
    {
        HX_RELEASE(pPathBuffer);
        piw.Write("");
        m_bDoesExist = FALSE;
        return HXR_OK;
    }

    char* pszCheckSum = (char*) pNewChecksum->GetBuffer();

    char szSize[16]; /* Flawfinder: ignore */ // XXXAH ok, if some day in the future we have terabite size DLLs then this will fail.
    itoa(m_nSizeBites, szSize, 10);

    char szNumPlugins[16]; /* Flawfinder: ignore */
    itoa(m_NumOfPlugins, szNumPlugins, 10);

    piw.Write("{");
    piw.Write((const char*) m_pFileName->GetBuffer());
    piw.Write(",");
    piw.Write(pszCheckSum);
    piw.Write(",");
    if (m_bHas_factory)
    {
        piw.Write("1");
    }
    else
    {
        piw.Write("0");
    }
    piw.Write(",");
    piw.Write(szSize);
    piw.Write(",");
    piw.Write(szNumPlugins);
    piw.Write("}");

    HX_RELEASE(pNewChecksum);
    HX_RELEASE(pPathBuffer);

    return HXR_OK;
}


HX_RESULT Plugin2Handler::PluginDLL::WritePref(PreferenceEnumerator* pPrefEnum)
{
    char pPrefValue[(1<<8)]; /* Flawfinder: ignore */

    IHXBuffer* pBuffer = NULL;
    CreateAndSetBufferCCF(pBuffer, (UCHAR*)"", 1, m_pPlugin2Handler->m_pContext);
#ifndef _MACINTOSH
    pPrefEnum->WriteSubPref( (const char *) m_pFileName->GetBuffer(), pBuffer );
#endif
    HX_VERIFY(HXR_OK == pPrefEnum->BeginSubPref( (const char *) m_pFileName->GetBuffer() ) );

    // the number of plugins
    sprintf(pPrefValue, "%d", m_NumOfPlugins); /* Flawfinder: ignore */
    pBuffer->Set((UCHAR*)(const char*)pPrefValue, strlen(pPrefValue)+1);
    pPrefEnum->WriteSubPref(PLUGIN_NUM_PLUGINS, pBuffer);

    // the checksum of the file.
    IHXBuffer* pPathBuffer = GetMountPoint()->Path();
    IHXBuffer*  pNewChecksum = m_pPlugin2Handler->ChecksumFile((char*)m_pFileName->GetBuffer(), pPathBuffer);
    if (pNewChecksum)
    {
        pPrefEnum->WriteSubPref(PLUGIN_FILE_CHECKSUM, pNewChecksum);
        HX_RELEASE(pNewChecksum);
    }
    HX_RELEASE(pPathBuffer);

    // the size of the DLL
    sprintf(pPrefValue, "%d", (int)m_nSizeBites); /* Flawfinder: ignore */
    pBuffer->Set((const UCHAR*)pPrefValue, strlen(pPrefValue)+1);
    pPrefEnum->WriteSubPref(PLUGIN_DLL_SIZE, pBuffer);

    // if the DLL has a factory or not.
    if (m_bHas_factory)
    {
        pBuffer->Set((const UCHAR*)"TRUE", strlen("TRUE")+1);
    }
    else
    {
        pBuffer->Set((const UCHAR*)"FALSE", strlen("FALSE")+1);
    }

    pPrefEnum->WriteSubPref(PLUGIN_HAS_FACTORY, pBuffer);

    HX_RELEASE(pBuffer);
    pPrefEnum->EndSubPref();
    return HXR_OK;
}


void Plugin2Handler::PluginDLL::SetPref(int nNumPlugins, char* pszCheckSum, int nSize, HXBOOL factory)
{
    m_NumOfPlugins = nNumPlugins;
    m_hash = pszCheckSum;
    m_nSizeBites = nSize;
    m_bHas_factory = factory;
}


HX_RESULT Plugin2Handler::PluginDLL::ReadPref(PreferenceEnumerator* pPrefEnum)
{
    IHXBuffer*      pBuffer;

    // Number of plugins in this DLL
    pPrefEnum->ReadPref(PLUGIN_NUM_PLUGINS, pBuffer);
    m_NumOfPlugins = pBuffer?atoi((const char*)pBuffer->GetBuffer()):0;
    HX_RELEASE(pBuffer);

    // The Checksum of the DLL
    pPrefEnum->ReadPref(PLUGIN_FILE_CHECKSUM, pBuffer);
    m_hash = pBuffer?(char*)pBuffer->GetBuffer():NULL;
    HX_RELEASE(pBuffer);

    // the size of the DLL
    pPrefEnum->ReadPref(PLUGIN_DLL_SIZE, pBuffer);
    m_nSizeBites = pBuffer?atoi((const char*)pBuffer->GetBuffer()):0;
    HX_RELEASE(pBuffer);

    // if the DLL has a factory or not.
    pPrefEnum->ReadPref(PLUGIN_HAS_FACTORY, pBuffer);
    if (pBuffer && strcmp((const char*)pBuffer->GetBuffer(), "FALSE"))
    {
        m_bHas_factory = TRUE;
    }
    else
    {
        m_bHas_factory = FALSE;
    }
    HX_RELEASE(pBuffer);
    return HXR_OK;
}

HX_RESULT Plugin2Handler::PluginDLL::Unload(HXBOOL safe)
{
    if (m_bLoaded)
    {
        if (!safe || ( m_fCanUnload && m_fCanUnload()==HXR_OK ) )
        {
            if (m_fpShutdown)
            {
                if (FAILED (m_fpShutdown ())) return HXR_FAIL;
                m_fpShutdown = NULL;
            }

            if (DLLAccess::DLL_OK == m_pDLLAccess->close())
            {
                m_bLoaded = FALSE;

                // Remove ourself from the plugin handler's CanUnload2DllList.
                LISTPOSITION posCanUnload = m_pPlugin2Handler->GetCanUnload2DllList().Find( this );
                if ( posCanUnload )
                {
                    m_pPlugin2Handler->GetCanUnload2DllList().RemoveAt( posCanUnload );
                }
                return HXR_OK;
            }
        }
    }
    return HXR_FAIL;
}

HXBOOL Plugin2Handler::PluginDLL::IsLoaded()
{
    return m_bLoaded;
}

Plugin2Handler::Errors  Plugin2Handler::PluginDLL::CreateInstance(IUnknown** ppUnk,
                                                                  UINT32 uIndex)
{
    if (!m_bLoaded)
        return PLUGIN_NOT_FOUND;

    if (!m_bHas_factory)
    {
        if (HXR_OK != m_fpCreateInstance(ppUnk))
        {
            return CREATE_INSTANCHXR_FAILURE;
        }
    }
    else
    {
        if (uIndex > (ULONG32)(m_NumOfPlugins-1) && m_NumOfPlugins)
        {
            return CANT_LOAD_INTERFACE;
        }
        IUnknown*               pUnk;
        IHXPluginFactory*       pPluginFactory;

        m_fpCreateInstance(&pUnk);
        if (HXR_OK != pUnk->QueryInterface
            (IID_IHXPluginFactory, (void**) &pPluginFactory))
        {
            HX_RELEASE(pUnk);
            return CREATE_INSTANCHXR_FAILURE;
        }
        else
        {
            HX_RELEASE(pUnk);
            if (HXR_OK != pPluginFactory->GetPlugin((UINT16)uIndex, ppUnk))
            {
                HX_RELEASE(pPluginFactory);
                return CREATE_INSTANCHXR_FAILURE;
            }
            HX_RELEASE(pPluginFactory);
        }
    }
    m_pPlugin2Handler->AddtoLRU(this);
    m_pPlugin2Handler->UpdateCache();
    return NO_ERRORS;
}

IHXBuffer* Plugin2Handler::PluginDLL::GetFileName()
{
    m_pFileName->AddRef();
    return m_pFileName;
}

IHXBuffer* Plugin2Handler::PluginDLL::GetNamespace()
{
    if (m_pNamespace)
    {
        m_pNamespace->AddRef();
    }

    return m_pNamespace;
}

void Plugin2Handler::PluginDLL::SetNamespace(IHXBuffer* pNamespace)
{
    m_pNamespace = pNamespace;

    if (m_pNamespace)
    {
        m_pNamespace->AddRef();
    }
}

UINT32 Plugin2Handler::PluginDLL::AddDLLReference()
{
    return ++m_nActiveReferences;
}


UINT32 Plugin2Handler::PluginDLL::ReleaseDLLReference()
{
    --m_nActiveReferences;
    Unload();       //XXXAH this should only happen if we have set DLL unloading to true.

    return 0;
}


/////////////////////////////////////////////////////////////////////////
//  Method:
//      Plugin2Handler::PluginDLL::QueryInterface
//  Purpose:
//      Implement this to export the interfaces supported by your
//      object.
//
STDMETHODIMP
Plugin2Handler::PluginDLL::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)this;
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//      Plugin2Handler::PluginDLL::AddRef
//  Purpose:
//      Everyone usually implements this the same... feel free to use
//      this implementation.
//
STDMETHODIMP_(ULONG32)
    Plugin2Handler::PluginDLL::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//      Plugin2Handler::PluginDLL::Release
//  Purpose:
//      Everyone usually implements this the same... feel free to use
//      this implementation.
//
STDMETHODIMP_(ULONG32)
    Plugin2Handler::PluginDLL::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}


/**********************************************************************************
 ***                 Plugin2Handler::PluginMountPoint                            ***
 ***********************************************************************************/

Plugin2Handler::PluginMountPoint::PluginMountPoint( IUnknown* pContext, 
                                                    Plugin2Handler* pHandler, 
                                                    const char* pName,
                                                    UINT32 majorVersion, 
                                                    UINT32 minorVersion, 
                                                    IHXBuffer* pPath ) :
    m_lRefCount( 0 ),
    m_lClientCount( 0 ),
    m_bHXCompliant( FALSE ),
    m_pIPrefs( NULL ),
    m_pIPath( NULL ),
    m_pName(NULL)
{

#ifdef HELIX_FEATURE_PREFERENCES
    // Set up a preferences object
    m_pIPrefs = pHandler->GetPreferences();
#endif

    if (pName)
    {
        CreateAndSetBufferCCF(m_pName, (UCHAR*)pName, strlen(pName)+1, pContext);
    }

    // Set up the path.
    if( !pPath && m_pIPrefs )
    {
        // Handle the special case path as well
        m_pIPath = pHandler->GetPluginDir();
    }
    else
    {
        m_pIPath = pPath;
        if( m_pIPath )
        {
            m_pIPath->AddRef();
        }
    }
}


Plugin2Handler::PluginMountPoint::~PluginMountPoint()
{
    HX_RELEASE(m_pName);
    HX_RELEASE(m_pIPrefs);
    HX_RELEASE(m_pIPath);
}


STDMETHODIMP_(ULONG32)
    Plugin2Handler::PluginMountPoint::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}


STDMETHODIMP_(ULONG32)
    Plugin2Handler::PluginMountPoint::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}


void Plugin2Handler::PluginMountPoint::AddClient()
{
    m_lClientCount++;
}


INT32 Plugin2Handler::PluginMountPoint::RemoveClient()
{
    return ( --m_lClientCount );
}


// XXXND  This is for backward compatibility
HXBOOL Plugin2Handler::PluginMountPoint::IsHXCompliant()
{
    return m_bHXCompliant;
}


IHXPreferences* Plugin2Handler::PluginMountPoint::Prefs()
{
    if( m_pIPrefs )
        m_pIPrefs->AddRef();

    return m_pIPrefs;
}


IHXBuffer* Plugin2Handler::PluginMountPoint::Path()
{
    if( m_pIPath )
        m_pIPath->AddRef();

    return m_pIPath;
}

IHXBuffer* Plugin2Handler::PluginMountPoint::Name()
{
    if( m_pName )
        m_pName->AddRef();

    return m_pName;
}



#if 0
/**********************************************************************************
 ***                 Plugin2Handler::PluginMonitor                               ***
 ***********************************************************************************/

/*
  The plugin monitor is responsible for monitoring each and every plugin
  that get instance is used to create. It attaches itself to the addref
  and relase of the target, and will mimic the refcount. When the object
  is destroyed it will send a notification to the plugin handler, thus
  the plugin handler will be able to determine if it is safe to
  unload the plugins.
*/


CHXMapPtrToPtr Plugin2Handler::PluginMonitor::MapTargetToMonitor;

// when we are constructed we will attach ourselves to the target.
Plugin2Handler::PluginMonitor::PluginMonitor(IUnknown* MonitorTarget,
                                             IHXPluginWatcherResponse* pResponse):
    m_lFakeRefCount(1)
    ,   m_lRefCount(0)
    ,   m_pPluginWatcherResponse(pResponse)
{
// put this back in if you ever expect plugin watchers to work
//    if (HXR_OK !=pResponse->QueryInterface(IID_IHXPluginWatcherResponse,
//      (void**) &m_pPluginWatcherResponse))
//    {
//      return;
//    }

    void** ppVoid           =   *(void***)MonitorTarget;
    m_fpAddRefPointer       =   (FPWatchRef) (*(ppVoid+1));     // cast to (FPWatchRef) ??
    m_fpReleasePointer      =   (FPWatchRef) (*(ppVoid+2));

    *(ppVoid+1) = Plugin2Handler::PluginMonitor::FakeAddRef;
    *(ppVoid+2) = Plugin2Handler::PluginMonitor::FakeRelease;

    AddMonitorTarget(MonitorTarget, this);
}

Plugin2Handler::PluginMonitor::~PluginMonitor()
{
    // XXXAH should we do something here?
}

HX_RESULT Plugin2Handler::PluginMonitor::AddMonitorTarget(void* pMonitorTarget,
                                                          PluginMonitor* pPluginMonitor)
{
    void* TempVal = NULL;
    if (MapTargetToMonitor.Lookup(pMonitorTarget, TempVal ))
    {
        return HXR_FAIL;
    }
    MapTargetToMonitor.SetAt(pMonitorTarget, (void*)pPluginMonitor);
    return HXR_OK;
}

HX_RESULT Plugin2Handler::PluginMonitor::ReleaseMonitorTarget(void* pMonitorTarget)
{
    void* TempVal = NULL;
    if (!MapTargetToMonitor.Lookup(pMonitorTarget, TempVal))
    {
        return HXR_FAIL;
    }
    MapTargetToMonitor.RemoveKey(pMonitorTarget);
    return HXR_OK;
}

Plugin2Handler::PluginMonitor* Plugin2Handler::PluginMonitor::GetThisPointerFromTarget(void* pMonitorTarget)
{
    PluginMonitor*      RetVal = NULL;

    if (!MapTargetToMonitor.Lookup(pMonitorTarget, (void*&)RetVal))
    {
        return NULL;
    }
    return RetVal;
}

ULONG32 STDMETHODCALLTYPE Plugin2Handler::PluginMonitor::FakeAddRef(void* pvoid)
{
    PluginMonitor*  pThis   = GetThisPointerFromTarget(pvoid);
    ULONG32         nResult = 0;

    if (pThis)
    {
        nResult = pThis->m_fpAddRefPointer(pvoid);
        InterlockedIncrement(&(pThis->m_lFakeRefCount));
    }
    return nResult;
}

ULONG32 STDMETHODCALLTYPE Plugin2Handler::PluginMonitor::FakeRelease(void* pvoid)
{
    PluginMonitor*  pThis   = GetThisPointerFromTarget(pvoid);
    ULONG32         nResult = 0;

    if (pThis)
    {
        nResult = pThis->m_fpReleasePointer(pvoid);
        InterlockedDecrement(&(pThis->m_lFakeRefCount));
        if (pThis->m_lFakeRefCount)
            return nResult;
        if (nResult<1)
        {
            pThis->m_pPluginWatcherResponse->AllObjectsDeleted((void*)pvoid);
            pThis->m_pPluginWatcherResponse->Release();
            ReleaseMonitorTarget(pvoid);
            // restore the v-table.
            void** pVoid        = *(void***)pvoid;
            (*(pVoid+1)) = pThis->m_fpAddRefPointer;
            (*(pVoid+2)) = pThis->m_fpReleasePointer;

            delete pThis;
            return 0;
        }
    }
    return nResult;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//      Plugin2Handler::PluginMonitor::QueryInterface
//  Purpose:
//      Implement this to export the interfaces supported by your
//      object.
//
STDMETHODIMP
Plugin2Handler::PluginMonitor::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)this;
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//      Plugin2Handler::PluginMonitor::AddRef
//  Purpose:
//      Everyone usually implements this the same... feel free to use
//      this implementation.
//
STDMETHODIMP_(ULONG32)
    Plugin2Handler::PluginMonitor::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//      Plugin2Handler::PluginMonitor::Release
//  Purpose:
//      Everyone usually implements this the same... feel free to use
//      this implementation.
//
STDMETHODIMP_(ULONG32)
    Plugin2Handler::PluginMonitor::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}
#endif


///////////////////////////////////////////////////////////////////////////////
//
// Plugin2Handler::CheckDirectory
//
///////////////////////////////////////////////////////////////////////////////

Plugin2Handler::Errors Plugin2Handler::Stat(const char* pszFilename, struct stat* pStatBuffer)
{
    CHXString   strFileName;

    memset(pStatBuffer,0,sizeof(*pStatBuffer));
#ifndef _STATICALLY_LINKED
    if(stat(pszFilename, pStatBuffer) < 0)
        return CANT_OPEN_DLL;
#endif
    pStatBuffer->st_atime = 0;
    return NO_ERRORS ;
}


IHXBuffer* Plugin2Handler::ConvertToAsciiString(char* pBuffer, UINT32 nBuffLen)
{
    char* pszOut = new char[nBuffLen*2+1];
    char* pszStartOut = pszOut;
    char Nibble;

    IHXBuffer* pOutBuffer = NULL;
    CreateBufferCCF(pOutBuffer, m_pContext);

    for (int i = 0; i<(int)nBuffLen; i++)
    {
        Nibble = (*pBuffer >> 4) & 15;
        *pszOut= (Nibble > 9 ) ? Nibble+55 : Nibble +48;
        pszOut++;
        Nibble = *pBuffer & 15;
        *pszOut= (Nibble> 9 ) ? Nibble+ 55 : Nibble+48;
        pszOut++;
        pBuffer++;
    }
    *pszOut = 0;
    pOutBuffer->Set((UCHAR*)pszStartOut, strlen(pszStartOut)+1);
    delete[] pszStartOut;
    return pOutBuffer;
}

//Returns true if the MD5 hash pszOrigMD5 matches the MD5 hash for the file
//pszFileName. If 'stat'ing has been disabled, we do not check at all and just
//return TRUE.
HXBOOL Plugin2Handler::DoChecksumsMatch(const char* pszOrigMD5,
                                        char* pszFilename,
                                        IHXBuffer*  pPath)
{
    HXBOOL     bMatch = TRUE;
    IHXBuffer* pBuf   = NULL;

    if( m_bStatDllsOnStartup )
    {
        bMatch = FALSE;
        pBuf = ChecksumFile(pszFilename, pPath);
        if( pBuf )
        {
            if(0==strcasecmp((char*)pBuf->GetBuffer(), pszOrigMD5))
            {
                bMatch = TRUE;
            }
        }
        HX_RELEASE(pBuf);
    }
    
    return bMatch;
}



IHXBuffer* Plugin2Handler::ChecksumFile(char* pszFileName, IHXBuffer* pPathBuffer)
{
#ifdef _STATICALLY_LINKED /* don't need checksumming for static linking */
    return ConvertToAsciiString("abc", 3);
#endif

    char pszFileNameWithPath[(1<<10)]; /* Flawfinder: ignore */
    SafeStrCpy(pszFileNameWithPath, (char*)pPathBuffer->GetBuffer(), (1<<10));
    INT32 nLen = ::strlen(pszFileNameWithPath);
    if (pszFileNameWithPath[nLen - 1] != Plugin2Handler::zm_cDirectorySeperator)
    {
        SafeStrCat(pszFileNameWithPath, Plugin2Handler::zm_pszDirectorySeperator, (1<<10));
        nLen += strlen(Plugin2Handler::zm_pszDirectorySeperator);
    }
    SafeStrCat(pszFileNameWithPath, pszFileName, (1<<10));

    UCHAR tempbuf[16] = "";
    struct stat stat_stuct;
    Errors statError = PLUGIN_NOT_FOUND;
#ifdef _MAC_CFM
    CHXFileSpecifier fileSpecifier(pszFileNameWithPath);
    if (SUCCEEDED(CHXFileSpecUtils::ResolveFileSpecifierAlias(fileSpecifier)))
    {
        statError = Stat(fileSpecifier.GetPathName(), &stat_stuct);
    }
#elif defined(_MAC_MACHO)
    FSRef targetFSRef;
    Boolean isDir;
    OSStatus err = ::FSPathMakeRef((UInt8*)pszFileNameWithPath, &targetFSRef, &isDir);
    if (err == noErr)
    {
        Boolean resolveAliasChains = true;
        Boolean targetIsDir, wasAlias;

        err = FSResolveAliasFileWithMountFlags(&targetFSRef, resolveAliasChains, &targetIsDir, &wasAlias, kResolveAliasFileNoUI);
        if (err == noErr && wasAlias)
        {
            err = FSRefMakePath(&targetFSRef, (UInt8*)pszFileNameWithPath, (1<<10));
        }
    }
    statError = Stat(pszFileNameWithPath, &stat_stuct);
#else
    statError = Stat(pszFileNameWithPath, &stat_stuct);
#endif

    stat_stuct.st_atime = 0;


    if (NO_ERRORS!=statError)
    {
        return NULL;
    }
    md5_state_t MD5_data;
    md5_init(&MD5_data);
    md5_append(&MD5_data,(UCHAR*)&(stat_stuct),sizeof(stat_stuct));
    md5_finish(tempbuf, &MD5_data);
    return ConvertToAsciiString((char*)tempbuf, sizeof(tempbuf));
}



/************************************************************************
 *                   PreferenceEnumerator                                *
 *                                                                       *
 *   This class encapsulates the optional IHXPreference Enumerator       *
 *   interafce which the top level client need not implement.            *
 *   If the top level client does implement IHXPreferenceEnumerator      *
 *   then this class is simply a wrapper. If the top level client        *
 *   does not implement it (why not?) then this class will perform       *
 *   the necessary leg work.                                             *
 ************************************************************************/


Plugin2Handler::PreferenceEnumerator::PreferenceEnumerator(IUnknown* pContext)
    :   m_pPrefEnum(NULL)
    ,   m_pPreferences(NULL)
    ,   m_pContext(pContext)
{
    HX_ADDREF(m_pContext);

    if (HXR_OK == m_pContext->QueryInterface(IID_IHXPreferences, (void**)&m_pPreferences))
    {
        IHXPreferences2* pPref2 = NULL;
        if (HXR_OK == m_pPreferences->QueryInterface(IID_IHXPreferences2, (void**) &pPref2))
        {
            pPref2->GetPreferenceEnumerator(m_pPrefEnum);
            pPref2->Release();
        }
    }
}


Plugin2Handler::PreferenceEnumerator::~PreferenceEnumerator()
{
    ULONG32 nRefCount;
    /* go through the list of prop names and delete all of the buffers */
    for(CHXSimpleList::Iterator i = m_ListofProps.Begin(); i!= m_ListofProps.End(); ++i)
    {
        nRefCount = ((IHXBuffer*)*i )->Release();
    }
    m_ListofProps.RemoveAll();
    HX_RELEASE(m_pPrefEnum);
    HX_RELEASE(m_pPreferences);
    HX_RELEASE(m_pContext);
}


HX_RESULT Plugin2Handler::PreferenceEnumerator::ResetPropNameList()
{
    ULONG32 nRefCount;
    /* go through the list of prop names and delete all of the buffers */
    for(CHXSimpleList::Iterator i = m_ListofProps.Begin(); i!= m_ListofProps.End(); ++i)
    {
        nRefCount = ((IHXBuffer*)*i )->Release();
    }
    m_ListofProps.RemoveAll();
    /* create the list of props at this level*/
    /* now store the name of the preference in the 'special' reg value. */

    char            pszRegKey[(1<<8)]; /* Flawfinder: ignore */
    IHXBuffer*      pKeyNamesBuffer = NULL;
    IHXBuffer*      pTempBuffer = NULL;

    SafeStrCpy(pszRegKey, (const char*)m_RegKey, (1<<8));
    SafeStrCat(pszRegKey, zm_pszRegKeySeperator, (1<<8));
    SafeStrCat(pszRegKey, zm_pszKeyNameRegKey, (1<<8));

    if (HXR_OK == m_pPreferences->ReadPref(pszRegKey, pKeyNamesBuffer))
    {
        /* ok it was found ... parse the list */
        char* token;
        token = strtok((char*)pKeyNamesBuffer->GetBuffer(), zm_pszValueSeperator);
        while (token)
        {
            if (HXR_OK == CreateAndSetBufferCCF(pTempBuffer, (UCHAR*) token, 
                                                strlen(token)+1, m_pContext))
            {
                m_ListofProps.AddTail((void*)pTempBuffer);
            }
            token = strtok(NULL, zm_pszValueSeperator);
        }
        HX_RELEASE(pKeyNamesBuffer);
    }
    else
    {
        return HXR_FAIL;
    }

    return HXR_OK;
}


HX_RESULT Plugin2Handler::PreferenceEnumerator::BeginSubPref(const char* pszSubPref)
{
    if (m_RegKey.GetLength())
    {
        m_RegKey += "\\";
    }
    m_RegKey += pszSubPref;

    if (m_pPrefEnum)
    {
        return m_pPrefEnum->BeginSubPref(pszSubPref);
    }

    ResetPropNameList();
    return HXR_OK;
}


HX_RESULT Plugin2Handler::PreferenceEnumerator::EndSubPref()
{
    char* pNewEnd = (char*)strrchr((const char*) m_RegKey , '\\');

    if (pNewEnd)
    {
        *pNewEnd = 0;
        CHXString sTemp = (const char*) m_RegKey;
        m_RegKey = sTemp;
    }
    else
    {
        m_RegKey = "";
    }

    if (m_pPrefEnum)
    {
        return m_pPrefEnum->EndSubPref();
    }

    ResetPropNameList();
    return HXR_OK;
}

HX_RESULT Plugin2Handler::PreferenceEnumerator::WriteSubPref(const char* pszSubName, IHXBuffer* pBuffer)
{
    char            pszRegKey[(1<<8)]; /* Flawfinder: ignore */
    IHXBuffer*      pKeyNameBuffer = NULL;

    SafeStrCpy(pszRegKey, (const char*)m_RegKey, (1<<8));
    SafeStrCat(pszRegKey, zm_pszRegKeySeperator, (1<<8));
    SafeStrCat(pszRegKey, pszSubName, (1<<8));

    m_pPreferences->WritePref(pszRegKey, pBuffer);
    /* now store the name of the preference in the 'special' reg value. */

    SafeStrCpy(pszRegKey, (const char*)m_RegKey, (1<<8));
    SafeStrCat(pszRegKey, zm_pszRegKeySeperator, (1<<8));
    SafeStrCat(pszRegKey, zm_pszKeyNameRegKey, (1<<8));

    /*  note: since we do not have an API for removing reg entries it
        is OK to simply append to the list all of the time */

    if (HXR_OK == m_pPreferences->ReadPref(pszRegKey, pKeyNameBuffer))
    {
        /*  hey let's check to see if the string we are adding is
            already in the list of tokens. If so we won't add it */
        char* pszTempString = new char[strlen((char*)pKeyNameBuffer->GetBuffer())+1];
        strcpy(pszTempString, (char*)pKeyNameBuffer->GetBuffer()); /* Flawfinder: ignore */
        char* token;
        HXBOOL  bFound = FALSE;
        token = strtok(pszTempString, zm_pszValueSeperator);
        while (token)
        {
            if (!strcasecmp(token, pszSubName))
            {
                bFound = TRUE;
                break;
            }
            token = strtok(NULL, zm_pszValueSeperator);
        }
        delete[] pszTempString;
        if (bFound)
        {
            pKeyNameBuffer->Release();
            return HXR_OK;
        }

        /* ok we already have this string. Let's append to it. */
        INT32 nLen = pKeyNameBuffer->GetSize()+strlen(pszSubName) + 2;
        char* pszTemp = new char [nLen];
        SafeStrCpy(pszTemp,  (char*)pKeyNameBuffer->GetBuffer(), nLen);
        SafeStrCat(pszTemp, zm_pszValueSeperator, nLen);
        SafeStrCat(pszTemp, pszSubName, nLen);
        HX_RELEASE(pKeyNameBuffer);
        
        if (HXR_OK == CreateAndSetBufferCCF(pKeyNameBuffer, (UCHAR*) pszTemp, 
                                            strlen(pszTemp)+1, m_pContext))
        {
            m_pPreferences->WritePref(pszRegKey, pKeyNameBuffer);
            HX_RELEASE(pKeyNameBuffer);
        }
        delete [] pszTemp;

        /* add this to the list of props */
        if (HXR_OK == CreateAndSetBufferCCF(pKeyNameBuffer, (UCHAR*) pszSubName, 
                                            strlen(pszSubName)+1, m_pContext))
        {
            m_ListofProps.AddTail((void*)pKeyNameBuffer);
            HX_RELEASE(pKeyNameBuffer);
        }
    }
    else
    {
        if (HXR_OK == CreateAndSetBufferCCF(pKeyNameBuffer, (UCHAR*) pszSubName, 
                                            strlen(pszSubName)+1, m_pContext)) 
        {
            m_pPreferences->WritePref(pszRegKey, pKeyNameBuffer);
            /* add this to the list of props */
            m_ListofProps.AddTail((void*)pKeyNameBuffer);       
        }
    }
    return HXR_OK;
}

HX_RESULT Plugin2Handler::PreferenceEnumerator::ReadPref(const char* pszSubName, REF(IHXBuffer*) /*OUT*/ pBuffer)
{
    if (m_pPrefEnum)
    {
        return m_pPrefEnum->ReadPref(pszSubName, pBuffer);
    }

    char            pszRegKey[(1<<8)]; /* Flawfinder: ignore */

    SafeStrCpy(pszRegKey, (const char*)m_RegKey, (1<<8));
    SafeStrCat(pszRegKey, zm_pszRegKeySeperator, (1<<8));
    SafeStrCat(pszRegKey, pszSubName, (1<<8));

    if (m_pPreferences)
        return m_pPreferences->ReadPref(pszRegKey, pBuffer);
    else
        return HXR_FAIL;
}

HX_RESULT Plugin2Handler::PreferenceEnumerator::GetPrefKey(UINT32 nIndex, IHXBuffer*& pBuffer)
{
    if (m_pPrefEnum)
    {
        return m_pPrefEnum->GetPrefKey(nIndex, pBuffer);
    }

    // get the Ith element of the list and and return it.
    LISTPOSITION pPos = m_ListofProps.FindIndex(nIndex);
    if (pPos)
    {
        pBuffer = (IHXBuffer*)m_ListofProps.GetAt(pPos);
        pBuffer->AddRef();
    }
    else
    {
        return HXR_FAIL;
    }
    return HXR_OK;
}


/********************************************************************
 *
 *       Plugin Enumeration
 *
 ********************************************************************/

BEGIN_INTERFACE_LIST_NOCREATE( CPluginEnumerator )
    INTERFACE_LIST_ENTRY_SIMPLE( IHXPluginSearchEnumerator )
    END_INTERFACE_LIST

CPluginEnumerator::CPluginEnumerator() :
    m_nIndex(0)
{
}

CPluginEnumerator::~CPluginEnumerator()
{
}


STDMETHODIMP_( UINT32 )
    CPluginEnumerator::GetNumPlugins()
{
    return m_ListOfPlugins.GetCount();
}

STDMETHODIMP_( void )
    CPluginEnumerator::GoHead()
{
    m_nIndex = 0;
}


STDMETHODIMP
CPluginEnumerator::GetNextPlugin( REF(IUnknown*) pIUnkResult, IUnknown* pIUnkOuter )
{
    // Initialize out params
    pIUnkResult = NULL;

    HX_RESULT res = GetPluginAt( m_nIndex, pIUnkResult, pIUnkOuter );
    m_nIndex++;

    return res;
}

STDMETHODIMP
CPluginEnumerator::GetNextPluginInfo( REF(IHXValues*) pRetValues )
{
    // Initialize out params
    pRetValues = NULL;

    HX_RESULT res = GetPluginInfoAt( m_nIndex, pRetValues );
    m_nIndex++;

    return res;
}


STDMETHODIMP
CPluginEnumerator::GetPluginAt( UINT32 index, REF(IUnknown*) pIUnkResult, IUnknown* pIUnkOuter )
{
    // Initialize out params
    pIUnkResult = NULL;

    HX_RESULT res = HXR_FAIL;

    LISTPOSITION pos = m_ListOfPlugins.FindIndex(index);
    if (pos)
    {
        Plugin2Handler::Plugin* pPlugin = (Plugin2Handler::Plugin*) m_ListOfPlugins.GetAt(pos);
        if (pPlugin)
        {
            if (Plugin2Handler::NO_ERRORS == pPlugin->GetInstance( pIUnkResult, pIUnkOuter ))
            {
                res = HXR_OK;
            }
        }
    }
    return res;
}


STDMETHODIMP
CPluginEnumerator::GetPluginInfoAt( UINT32 index, REF(IHXValues*) pRetValues )
{
    // Initialize out params
    pRetValues = NULL;

    HX_RESULT res = HXR_FAIL;

    LISTPOSITION pos = m_ListOfPlugins.FindIndex(m_nIndex);
    m_nIndex++;
    if (pos)
    {
        Plugin2Handler::Plugin* pPlugin = (Plugin2Handler::Plugin*) m_ListOfPlugins.GetAt(pos);
        if (pPlugin)
        {
            res = pPlugin->GetPluginInfo( pRetValues );
        }
    }

    return res;
}


void CPluginEnumerator::Add(Plugin2Handler::Plugin* pPlugin)
{
    IHXValues*      pPluginValues   = NULL;
    IHXBuffer*      pBuffer         = NULL;
    HXBOOL          bAdded          = FALSE;

    if (HXR_OK == pPlugin->GetPluginInfo(pPluginValues) && pPluginValues)
    {
        if (HXR_OK == pPluginValues->GetPropertyCString(PLUGIN_DESCRIPTION2, pBuffer) &&
            pBuffer)
        {
            if (strstr((const char*)pBuffer->GetBuffer(), "RealNetworks"))
            {
                m_ListOfPlugins.AddHead(pPlugin);
                bAdded = TRUE;
            }
        }
        HX_RELEASE(pBuffer);
    }
    if (!bAdded)
    {
        m_ListOfPlugins.AddTail(pPlugin);
    }
}


HX_RESULT CPluginEnumerator::GetNext(REF(IUnknown*) pRetUnk)
{
    pRetUnk = NULL;
    return GetNextPlugin( pRetUnk, NULL );
}

CPluginInfoWriter::CPluginInfoWriter() :
    m_pIHXBuffer(NULL),
    m_pIHXPreferences(NULL),
    m_NumWrites(0),
    m_BufUsed(0),
    m_BufSize(0),
    m_Checksum(0)
{
}

CPluginInfoWriter::~CPluginInfoWriter()
{
    if (m_Checksum)
    {
        CHXString checksum;
        checksum.Format ("%u", m_Checksum);

        Write (checksum); // note this will increment m_Checksum but we don't care, we got the value we wanted.
    }

    Flush ();

    HX_RELEASE(m_pIHXBuffer);
    HX_RELEASE(m_pIHXPreferences);
}

HX_RESULT CPluginInfoWriter::Init(IHXPreferences* pPrefs, const char* pBaseKeyName, IHXBuffer* pIHXBuffer)
{
    // Helper class, validation is done by caller; we only assert that it has been done
    HX_ASSERT(pPrefs);
    HX_ASSERT(pBaseKeyName);
    HX_ASSERT(pIHXBuffer);

    HX_RESULT result = HXR_FAIL;

    if(pIHXBuffer)
    {
        m_pIHXBuffer = pIHXBuffer;
        m_pIHXBuffer->AddRef();
        m_BufSize = pIHXBuffer->GetSize();

        // Store base name used to generate plugin info reg key names...
        m_BaseKeyName = pBaseKeyName;

        m_pIHXPreferences = pPrefs;
        m_pIHXPreferences->AddRef();

        result = HXR_OK;
    }

    return result;
}

HX_RESULT CPluginInfoWriter::Write(IHXBuffer *pBuffer)
{
    HX_ASSERT(pBuffer);

    HX_RESULT result = HXR_FAIL;

    if(pBuffer)
    {
        result =  Write((const char*) pBuffer->GetBuffer(), pBuffer->GetSize());
    }

    return result;
}

HX_RESULT CPluginInfoWriter::Write(const char *pInfo)
{
    HX_ASSERT(pInfo);

    HX_RESULT result = HXR_FAIL;

    if(pInfo)
    {
        result = Write(pInfo, strlen(pInfo));
    }

    return result;
}

HX_RESULT CPluginInfoWriter::Write(const char *pInfo, UINT32 len)
{
    // Helper class, validation is done by caller; we only assert that it has been done
    HX_ASSERT(pInfo);
    HX_ASSERT(m_pIHXBuffer);

    // verify ASCII-7 compliance as we've had issues on double-byte machines with writing
    // ASCII-8 characters to the registry.
    HX_ASSERT(IsAscii7Compliant(pInfo, len));

    HX_RESULT result = HXR_FAIL;

    m_Checksum += len;

    // Determine how much needs to be written
    UINT32 toWrite = len;

    UCHAR* pSrcPos = (UCHAR *)pInfo;
    UCHAR* pWritePos = m_pIHXBuffer->GetBuffer();
    pWritePos += m_BufUsed;

    while(toWrite)
    {
        // We subtract one that will be used to store NULL terminator added to end of buffer
        UINT32 bufUnused = m_BufSize - m_BufUsed - 1;

        //  We should never be negative...
        HX_ASSERT(bufUnused >= 0);

        if(bufUnused >= toWrite)
        {
            memcpy(pWritePos, pSrcPos, toWrite); /* Flawfinder: ignore */

            pSrcPos += toWrite;
            pWritePos += toWrite;
            m_BufUsed += toWrite;
            toWrite = 0;
        }
        else
        {
            memcpy(pWritePos, pSrcPos, bufUnused); /* Flawfinder: ignore */

            pSrcPos += bufUnused;
            pWritePos += bufUnused;
            m_BufUsed += bufUnused;
            toWrite -= bufUnused;
        }

        // We need to subtract 1 for the NULL terminator added to buffer
        if(m_BufUsed == m_BufSize - 1)
        {
            TerminateBuffer();
            WriteToRegistry();
            pWritePos = m_pIHXBuffer->GetBuffer();
            m_BufUsed = 0;
        }
    }

    return result;
}

void CPluginInfoWriter::TerminateBuffer()
{
    HX_ASSERT(m_pIHXBuffer);

    // We needed to make sure we have enough room for NULL terminator...
    HX_ASSERT(m_BufUsed <= m_BufSize  -1);

    char* pChar = (char *)m_pIHXBuffer->GetBuffer();
    pChar[m_BufUsed] = '\0';
}

void CPluginInfoWriter::WriteToRegistry()
{
    HX_ASSERT(m_pIHXBuffer);
    HX_ASSERT(m_pIHXPreferences);
    HX_ASSERT(m_NumWrites >= 0);

    // Build key name
    CHXString key;
    key.Format("%s%u", (const char *)m_BaseKeyName, m_NumWrites);

    m_pIHXPreferences->WritePref(key, m_pIHXBuffer);
    m_NumWrites++;
}

HX_RESULT CPluginInfoWriter::Flush()
{
    if(m_BufUsed > 0)
    {
        TerminateBuffer();
        WriteToRegistry();
    }

    return HXR_OK;
}

HXBOOL CPluginInfoWriter::IsAscii7Compliant(const char* data, const UINT32 len)
{
    for (UINT32 i = 0; i < len; ++i)
    {
        static const unsigned char ascii7Max = 0x80;
        if (((const unsigned char)data [i]) & ascii7Max)
        {
            return FALSE;
        }
    }

    return TRUE;
}
