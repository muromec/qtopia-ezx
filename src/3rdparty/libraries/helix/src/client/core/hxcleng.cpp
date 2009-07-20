/* ***** BEGIN LICENSE BLOCK *****
* Source last modified: $Id: hxcleng.cpp,v 1.136 2009/05/08 03:43:27 jain_1982s Exp $
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

#include "hxtypes.h"
#if defined( _WIN32 ) || defined( _WINDOWS )
#include "hlxclib/windows.h"

#if !defined(WIN32_PLATFORM_PSPC)
#include <ddeml.h>
#endif /* !defined(WIN32_PLATFORM_PSPC) */
#endif /* defined( _WIN32 ) || defined( _WINDOWS ) */
#include "hxcom.h"
#include "hxresult.h"
//#include "pnlice.h"
#include "timeval.h"
#include "pq.h"
#include "hxcomm.h"
#include "hxengin.h"
#include "hxslist.h"
#if defined(HELIX_FEATURE_NETINTERFACES)
#include "hxnetif.h"
#endif /* HELIX_FEATURE_NETINTERFACES */
#include "ihxpckts.h"
#include "hxfiles.h"
#include "hxcore.h"
#include "hxprefs.h"
#include "fwctlmgr.h"
#include "auderrs.h"
#include "hxausvc.h"
#include "hxhyper.h"
#include "hxmon.h"
#include "hxsmbw.h"
#include "hxgroup.h"
#if defined(HELIX_FEATURE_NETSERVICES)
#if !defined(_SYMBIAN)
#include "chxclientnetservices.h"
#else
#include "hxsymbiannet.h"   // new netapi
#include "memorymonitor.h"
#endif /* _SYMBIAN */
#if defined(HELIX_FEATURE_NETSERVICES_SHIM) || defined(HELIX_FEATURE_NET_LEGACYAPI)
#include "hxnetapi.h" //HXNetworkServices (old net services)
#endif
#if defined(HELIX_FEATURE_NETSERVICES_SHIM)
#include "shim_net_services.h"
#endif
#endif //HELIX_FEATURE_NETSERVICES
#include "hxxrsmg.h"
#include "hxxml.h"
#include "hxpac.h"

#include "threngin.h"
#include "pckunpck.h"

#if defined(_UNIX) || (defined(_MACINTOSH) && defined(_CARBON) && defined(THREADS_SUPPORTED))
//XXXgfw Just temp. See below in EventOccurred....
#include "conn.h"
#include "thrdconn.h"
#if defined(_UNIX)
#include "UnixThreads.h"  //For new unix message loop....
#elif defined(_MACINTOSH) && defined(_CARBON) && defined(THREADS_SUPPORTED)
#include "carbthrd.h"
#endif
#include "hxmsgs.h"  //for the messages.
#endif

#include "hxaudses.h"
#include "hxaudply.h"
#include "hxwinver.h"
#include "hxplay.h"
#include "hxclsnk.h"
#include "chxpckts.h"
#include "hxsched.h"
#include "hxoptsc.h"
#include "hxpref.h"
#include "hxwin.h"
#include "hxcorgui.h"
#include "hxhypnv.h"
#include "thrhypnv.h"
#include "hxerror.h"
#include "ihxmedpltfm.h"
#include "thrdutil.h"
#include "chxuuid.h"
#include "hxplugn.h"
#include "hxshtdn.h"
#include "chunkres.h"
#include "hxthread.h"
#include "hxresmg.h"
#include "portaddr.h"
#include "clntcore.ver"
#include "rtsputil.h"
#include "hxconnbwinfo.h"

#ifdef _WIN32
#include "hxconv.h"
#include "platform/win/hxdllldr.h"
#include "filespecutils.h"
#endif
#include "hxpsink.h"
#include "hxcorcom.h"
#include "hxcleng.h"
#include "statinfo.h"
#include "createbwman.h"
#include "dbcs.h"

#include "hxmarsh.h"
//#include "hxmime.h"
#include "plsnkctl.h"
#include "hxmangle.h"
#if defined(_STATICALLY_LINKED) || !defined(HELIX_FEATURE_PLUGINHANDLER2)
#if defined(HELIX_CONFIG_CONSOLIDATED_CORE)
#include "basehand.h"
#else /* HELIX_CONFIG_CONSOLIDATED_CORE */
#include "hxpluginmanager.h"
#endif /* HELIX_CONFIG_CONSOLIDATED_CORE */
#else
#include "plghand2.h"
#endif /* _STATICALLY_LINKED */
#include "dllpath.h"
#include "readpath.h"
#include "hxdir.h"
#include "hxstrutl.h"
#include "validatr.h"

#if !defined(_WINCE) || (_WIN32_WCE >= 400)
#include "hxlang.h"
#endif

#include "crdcache.h"
#include "hxresmgr.h"
#include "hxxmlprs.h"
#include "ihxcookies.h"
#include "cookhlpr.h"
#include "cookies.h"
#if defined(HELIX_FEATURE_SMARTERNETWORK)
#include "preftran.h"
#endif /* HELIX_FEATURE_SMARTERNETWORK */
#include "medblock.h"
#include "viewsrc.h"
#include "hxovmgr.h"
#include "hxtlogutil.h"
#if defined(HELIX_FEATURE_AUTO_BANDWIDTH_DETECTION)
#include "hxabd.h"
#endif /* #if defined(HELIX_FEATURE_AUTO_BANDWIDTH_DETECTION) */

#ifdef _TEST_REPLACED_AUDIO_DEVICE
#include "hxaudev.h"
#endif /*_TEST_REPLACED_AUDIO_DEVICE*/

//#include "../dcondev/dcon.h"

#if defined (_WIN16)
#include <stdlib.h>
#include "hlxclib/windows.h"
#include <shellapi.h>
#endif

#if (defined (_WINDOWS) || defined (_WIN32) ) && !defined(_WINCE) && defined (HELIX_FEATURE_VIDEO)
#include "platform/win/sdidde.h"
#include <vfw.h>
#include "ddraw.h"
//#include "rncolor.h"
#endif /*defined (_WINDOWS) || defined (_WIN32)*/

#include "hxver.h"

#ifdef _OPENWAVE
#include "timeline.h"
#endif

#ifdef _UNIX
#include "unix_net.h"
#include "hxslctcb.h"
#include "timeline.h"
#endif
#ifdef _MACINTOSH
#include "hx_moreprocesses.h"
#include "hxmm.h"
#endif

#ifdef __TCS__
#include "platform/tm1/tm1_net.h"
#include "sitemgr.h"
#include "timeline.h"
#endif

#if defined(_MAC_UNIX) || defined(_WINDOWS)
#include "sitemgr.h"
#include "hxwintyp.h"
#include "filespecutils.h"
#endif

#ifdef _MACINTOSH
#include "timeline.h"
#include "sitemgr.h"
#include "hxwintyp.h"

#ifdef _CARBON
#include "genthrd.h"
#include "filespecutils.h"
#endif

#endif
#include "debug.h"
#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE     
static const char HX_THIS_FILE[] = __FILE__;
#endif

#if defined (_WINDOWS) || defined (_WIN32)



#if 0
#ifdef _DEBUG
#include "allochok.h"
HXAllocHook g_MemoryHook;
#endif
#endif

#endif

#define DEFAULT_MIN_BANDWIDTH   57600       // 56.6 Kbps
#define DEFAULT_MAX_BANDWIDTH   10485800    // 10Mbps

#ifdef _TEST_REPLACED_AUDIO_DEVICE
CHXAudioDevice* z_ReplacedAudioDevice = NULL;
#endif /*_TEST_REPLACED_AUDIO_DEVICE*/

#if defined(HELIX_FEATURE_HELIXSIM)
    HXBOOL g_bHelixsimLoadTest = FALSE;
#endif 

#if defined(HELIX_FEATURE_SYSTEMREQUIRED)
STDMETHODIMP 
HXSystemRequired::QueryInterface(REFIID riid, void** ppvObj)
{
    QInterfaceList qiList[] =
    {
        { GET_IIDHANDLE(IID_IHXSystemRequired), (IHXSystemRequired*)this },
        { GET_IIDHANDLE(IID_IUnknown), (IUnknown*)(IHXSystemRequired*)this },
    };

    return ::QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}

STDMETHODIMP_(ULONG32) 
HXSystemRequired::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32) 
HXSystemRequired::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    if(m_lRefCount == 0)
    {
        delete this;
    }

    return 0;
}

STDMETHODIMP 
HXSystemRequired::HasFeatures(IHXUpgradeCollection* pFeatures)
{
    return HXR_FAIL;
}
#endif /* HELIX_FEATURE_SYSTEMREQUIRED */

HXClientEngine::HXClientEngine() :
m_lRefCount (0)
,m_ulPlayerIndex (0)
,m_unRegistryID (0)
,m_pRegistry(NULL)
,m_pCommonClassFactory (0)
#if defined(_MEDIUM_BLOCK)    
,m_pAllocator(NULL)
#endif    
,m_pScheduler (0)
,m_pScheduler2(NULL)
,m_pOptimizedScheduler(0)
,m_pPreferences (0)
,m_pAudioSession(NULL)
#if defined(HELIX_FEATURE_NETSERVICES)
,m_pNetServices(NULL)
#if defined(HELIX_FEATURE_NET_LEGACYAPI)
,m_pOldNetServices(NULL)
#endif
#endif /* HELIX_FEATURE_NETSERVICES */
#ifdef _UNIX
,m_pAsyncIOSelection(NULL)
,m_bNetworkThreading(TRUE)
#endif
,m_pProxyAutoConfig(NULL)
,m_pValidator(NULL)
,m_pASM(NULL)
,m_pPlayerSinkControl(NULL)
,m_pCredentialsCache(NULL)
,m_pResMgr(NULL)
,m_pExternalResourceManager(NULL)
,m_pViewSource(NULL)
,m_pSystemRequired(NULL)
,m_pFWCtlMgr(NULL)
,m_pProxyManager(NULL)
,m_pABDCalibrator(NULL)
,m_pPreferredTransportManager(NULL)
,m_pOverlayManager(NULL)
,m_pMultiPlayPauseSupport(NULL)
,m_pConnBWInfo(NULL)
,m_pPlugin2Handler(NULL)
,m_lROBActive(0)
,m_LastError (0)
,m_bIsSchedulerStarted(FALSE)
,m_bInitialized(FALSE)
#if defined(THREADS_SUPPORTED)
,m_bUseCoreThread(TRUE)
#else
,m_bUseCoreThread(FALSE)
#endif
,m_bUseCoreThreadExternallySet(FALSE)
,m_pCoreComm(NULL)
,m_pCoreMutex(NULL)
#if defined(_MACINTOSH) && defined(_CARBON) && defined(THREADS_SUPPORTED)
,m_bUseMacBlitMutex(FALSE)
,m_pMacBlitMutex(NULL)
#endif
,m_AUName(NULL)
#if defined(HELIX_FEATURE_SYSTEM_EXTERNAL_TIMER)
,m_pCurrentTime(NULL)
#endif /*HELIX_FEATURE_SYSTEM_EXTERNAL_TIMER*/

,m_pContext(NULL)
{
#if 0 // XXX HP Atlas
#if defined(HELIX_FEATURE_PREFERENCES)
#if defined(HELIX_FEATURE_NO_INTERNAL_PREFS)
    m_pOrigPreferences = NULL;
#elif defined(HELIX_FEATURE_LITEPREFS)
    m_pOrigPreferences      = CHXLitePrefs::CreateObject();
#endif
#endif /* HELIX_FEATURE_PREFERENCES */
#endif

#if defined(HELIX_FEATURE_ASM)
    m_pASM = CreateBandwidthManager();
#endif /* HELIX_FEATURE_ASM */
    m_pPlayerSinkControl    = new CHXPlayerSinkControl();
#if defined(HELIX_FEATURE_CORECOMM)
    m_pCoreComm         = HXCoreComm::Create(this);
#endif /* HELIX_FEATURE_CORECOMM */
#if defined(HELIX_FEATURE_SYSTEMREQUIRED)
    m_pSystemRequired       = (IHXSystemRequired*) new HXSystemRequired;
#endif /* HELIX_FEATURE_SYSTEMREQUIRED */
#if defined(HELIX_FEATURE_PROXYMGR)
    m_pProxyManager     = new HXProxyManager();
#endif /* HELIX_FEATURE_PROXYMGR */
#if defined(HELIX_FEATURE_SMARTERNETWORK)
    m_pPreferredTransportManager = new HXPreferredTransportManager((IUnknown*)(IHXClientEngine*)this);
#endif /* HELIX_FEATURE_SMARTERNETWORK */
#if defined(HELIX_FEATURE_FW_CTLMGR)
    m_pFWCtlMgr = HXFirewallControlManager::Create();
#endif /* HELIX_FEATURE_FW_CTLMGR */

#if defined(HELIX_FEATURE_AUTO_BANDWIDTH_DETECTION)
    m_pABDCalibrator = new CHXABDCalibrator((IUnknown*)(IHXClientEngine*)this);
#endif /* #if defined(HELIX_FEATURE_AUTO_BANDWIDTH_DETECTION) */

    m_pSiteEventHandler = NULL;
    m_pKicker = NULL;

    if (FALSE
#if defined(HELIX_FEATURE_AUTO_BANDWIDTH_DETECTION)
        || !m_pABDCalibrator
#endif /* #if defined(HELIX_FEATURE_AUTO_BANDWIDTH_DETECTION) */
#if defined(HELIX_FEATURE_ASM)
        || !m_pASM      
#endif /* HELIX_FEATURE_ASM */
        || !m_pPlayerSinkControl
#if defined(HELIX_FEATURE_SYSTEMREQUIRED)
        || !m_pSystemRequired       
#endif /* HELIX_FEATURE_SYSTEMREQUIRED */
#if defined(HELIX_FEATURE_PROXYMGR)
        || !m_pProxyManager 
#endif /* HELIX_FEATURE_PROXYMGR */
#if defined(HELIX_FEATURE_SMARTERNETWORK)
        || !m_pPreferredTransportManager
#endif /* HELIX_FEATURE_SMARTERNETWORK */
        )
    {
        m_LastError = HXR_OUTOFMEMORY;
    }

    if (!m_LastError)
    {
#if defined(HELIX_FEATURE_ASM)
        HX_ADDREF(m_pASM);
#endif /* HELIX_FEATURE_ASM */
        HX_ADDREF(m_pPlayerSinkControl);
#if defined(HELIX_FEATURE_SYSTEMREQUIRED)
        HX_ADDREF(m_pSystemRequired);
#endif /* HELIX_FEATURE_SYSTEMREQUIRED */
#if defined(HELIX_FEATURE_PROXYMGR)
        HX_ADDREF(m_pProxyManager);
#endif /* HELIX_FEATURE_PROXYMGR */
#if defined(HELIX_FEATURE_SMARTERNETWORK)
        HX_ADDREF(m_pPreferredTransportManager);
#endif /* HELIX_FEATURE_SMARTERNETWORK */
#if defined(HELIX_FEATURE_FW_CTLMGR)
        HX_ADDREF(m_pFWCtlMgr);
#endif /* HELIX_FEATURE_FW_CTLMGR */
#if defined(HELIX_FEATURE_AUTO_BANDWIDTH_DETECTION)
        HX_ADDREF(m_pABDCalibrator);
#endif /* #if defined(HELIX_FEATURE_AUTO_BANDWIDTH_DETECTION) */
    }

    if (!m_LastError)
    {
        // Assemble registry's key for the preferences 
        char* pCompanyName = new char[strlen(HXVER_COMMUNITY) + 1];
        strcpy(pCompanyName, HXVER_COMMUNITY); /* Flawfinder: ignore */

        char* pProductName = new char[strlen(HXVER_SDK_PRODUCT) + 1];
        strcpy(pProductName, HXVER_SDK_PRODUCT); /* Flawfinder: ignore */


        char * pComa = HXFindChar(pCompanyName, ',');
        if(pComa)
        {
            *pComa = 0;   
        }

        pComa = HXFindChar(pProductName, ',');
        if(pComa)
        {
            *pComa = 0;   
        }

#if 0 // XXX HP Atlas

        HX_RESULT theErr = HXR_OK;

#if defined(HELIX_FEATURE_PREFERENCES) && !defined(HELIX_FEATURE_NO_INTERNAL_PREFS) 
         const ULONG32 nProdMajorVer = TARVER_MAJOR_VERSION;
         const ULONG32 nProdMinorVer = TARVER_MINOR_VERSION;
#endif.

#if defined(HELIX_FEATURE_PREFERENCES)
#if defined(HELIX_FEATURE_NO_INTERNAL_PREFS)
        theErr = HXR_OK;
#elif defined(HELIX_FEATURE_LITEPREFS)
        theErr = m_pOrigPreferences->Open( 
            (const char*) pCompanyName, (const char*) pProductName, nProdMajorVer, nProdMinorVer);
        if( theErr != HXR_OUTOFMEMORY )
        {
            // HXR_FAIL probably just means the prefs file does not exist. We will
            // auto-create it when we do our first write.
            theErr = HXR_OK;
        }
#endif

        if (HXR_OK == theErr)
        {
            // Read any paths from preferences that weren't set by our loader
            if (m_pOrigPreferences)
            {
                ReadUnsetPathsFromPrefs();
            }

            // HXLOG is preferred. DPRINTF is fallback trace method. It is particularly
            // useful in cases where we need to capture trace before we have a chance
            // to initialize HXLOG. It should be initialized as soon as preferences are
            // obtained or overridden.
            //
            DPRINTF_INIT(m_pPreferences);
        }
        else
        {
            HX_RELEASE(m_pOrigPreferences);
            HX_RELEASE(m_pPreferences);
        }

#endif /* HELIX_FEATURE_PREFERENCES */
#endif

        HX_VECTOR_DELETE(pCompanyName);
        HX_VECTOR_DELETE(pProductName);

#ifndef _VXWORKS
        // Set the Plugin and Codec directories if they are not yet set
        if (!GetDLLAccessPath()->GetPath(DLLTYPE_PLUGIN))
        {
            CreatePluginDir();
        }
        if (!GetDLLAccessPath()->GetPath(DLLTYPE_CODEC))
        {
            CreateCodecDir();
        }
#endif // _VXWORKS
    }

#ifdef _UNIX
    m_select_callbacks = new CHXSimpleList;
#endif
#ifdef _SYMBIAN
    // create memory monitor
    SymbianMemoryMonitor *pMonitor = SymbianMemoryMonitor::Instance();
    if (pMonitor)
    {
        UINT32 ulSize = 1024*512; //default Mem out buffersize
        //Read from config file
        ReadPrefUINT32(m_pPreferences, "MemoryProtection", ulSize);
        pMonitor->Init((IUnknown*) (IHXClientEngine*)this, ulSize);
    }
#endif
}

HXClientEngine::~HXClientEngine() 
{
#if defined(HELIX_FEATURE_SYSTEM_EXTERNAL_TIMER)
    HX_DELETE(m_pCurrentTime); 
#endif /*HELIX_FEATURE_SYSTEM_EXTERNAL_TIMER*/

    Close();
}

void
HXClientEngine::CreatePrefIfNoExist(const char* pName, const char* pValue)
{
#if defined(HELIX_FEATURE_PREFERENCES)
    IHXBuffer* pBuffer = NULL;

    if (m_pPreferences && m_pPreferences->ReadPref(pName, pBuffer) != HXR_OK)
    {
	if (HXR_OK == CreateAndSetBufferCCF(pBuffer, (UCHAR*)pValue, 
					    strlen(pValue) + 1, m_pContext))
	{
	    m_pPreferences->WritePref(pName, pBuffer);
	}
    }
    HX_RELEASE(pBuffer);
#endif
}

void
HXClientEngine::CreatePluginDir()
{
#if !defined(_VXWORKS) && !defined(__TCS__)
    char pPluginDir[_MAX_PATH + 1] = ""; /* Flawfinder: ignore */

#ifdef _WINCE
    SafeStrCpy(pPluginDir, "\\", _MAX_PATH + 1);
#elif defined (_WINDOWS) || defined (_WIN32)    
    if (!GetSystemDirectory(pPluginDir, _MAX_PATH))
    {
        SafeStrCpy(pPluginDir, "", _MAX_PATH + 1);
    }

    if (strlen(pPluginDir) > 0 && pPluginDir[strlen(pPluginDir) - 1] != '\\')
    {
        SafeStrCat(pPluginDir, "\\", _MAX_PATH + 1);
    }

    SafeStrCat(pPluginDir, "Real", _MAX_PATH + 1);
#elif defined (_UNIX)
    SafeStrCpy(pPluginDir, getenv("HOME"), _MAX_PATH+1);
    SafeStrCat(pPluginDir, "/Real", _MAX_PATH+1 - strlen(pPluginDir));
#elif defined (_MACINTOSH)

    // xxxbobclark this assumes that the shared libraries live right
    // next to the executable. It's highly recommended that if you're
    // writing a TLC, you call SetPath yourself...
    SafeStrCpy(pPluginDir, "", _MAX_PATH + 1);

#endif // defined (_WINDOWS) || defined (_WIN32)

    // Set the Path
    GetDLLAccessPath()->SetPath(DLLTYPE_PLUGIN, pPluginDir);
#endif // _VXWORKS
}

void
HXClientEngine::CreateCodecDir()
{
#if !defined(_VXWORKS) && !defined(__TCS__)
    const char* pPath = NULL;
    CHXString codecDir;

    pPath = GetDLLAccessPath()->GetPath(DLLTYPE_PLUGIN);

    if (pPath) codecDir = pPath;

    // xxxbobclark this assumes that the codecs live right next
    // to the executable. It's highly recommended that if you're
    // writing a TLC, you call SetPath yourself...
#ifndef _MACINTOSH
    if (strcmp((const char*)codecDir.Right(1), OS_SEPARATOR_STRING))
    {
        codecDir += OS_SEPARATOR_STRING;
    }

    codecDir += "Codecs";
#endif

    // Set the Path
    GetDLLAccessPath()->SetPath(DLLTYPE_CODEC, (const char*)codecDir);
#endif // _VXWORKS
}

void HXClientEngine::_Initialize(void)
{
    IHXBuffer* pValue = NULL;

#ifdef _MEDIUM_BLOCK
    m_pAllocator = new CMediumBlockAllocator((IUnknown*)(IHXClientEngine*)this);
    HX_ADDREF(m_pAllocator);
    CHXBuffer::SetAllocator(m_pAllocator);
    m_pAllocator->SetScheduler((IUnknown*)(IHXScheduler*)m_pScheduler);
#endif

#if defined(HELIX_FEATURE_AUTHENTICATION)
    m_pCredentialsCache = new CHXCredentialsCache((IUnknown*)(IHXClientEngine*)this);
    HX_ADDREF(m_pCredentialsCache);
#endif /* HELIX_FEATURE_AUTHENTICATION */

#if defined(HELIX_FEATURE_OVERLAYMGR)
    m_pOverlayManager = new HXOverlayManager((IUnknown*)(IHXClientEngine*)this);
    HX_ADDREF(m_pOverlayManager);
    m_pOverlayManager->Initialize();
#endif /* HELIX_FEATURE_OVERLAYMGR */

#if defined(HELIX_FEATURE_AUDIO)
    if (!m_pAudioSession)
    {
        m_pAudioSession = NewAudioSession();
        if( !m_pAudioSession )
        {
            m_LastError = HXR_OUTOFMEMORY;
            return;
        }
        HX_ADDREF(m_pAudioSession);
    }
#endif /* HELIX_FEATURE_AUDIO */

    HXConnectionBWInfo::Create((IUnknown*)(IHXClientEngine*)this, 
			       (IUnknown*)(IHXClientEngine*)this, 
			       m_pConnBWInfo);

    // Init IHXRegistry entries
    InitializeRegistry();

#if defined(HELIX_FEATURE_PREFERENCES)
    if (!m_LastError)
    {
#if 0 // XXX HP Atlas
#if !defined(HELIX_FEATURE_NO_INTERNAL_PREFS)
        if (m_pOrigPreferences)
        {
            m_pOrigPreferences->SetContext((IUnknown*) (IHXClientEngine*)this);
        }
#endif /* !defined(HELIX_FEATURE_NO_INTERNAL_PREFS) */
#endif
        // generate GUID if it doesn't exist
        HXBOOL bRegenerate = TRUE;

        if (m_pPreferences &&
            m_pPreferences->ReadPref(CLIENT_GUID_REGNAME, pValue) == HXR_OK)
        {
            char* pszGUID = DeCipher((char*)pValue->GetBuffer());
            if(pszGUID && strlen(pszGUID) == 36)
                bRegenerate = FALSE;

            HX_RELEASE(pValue);
            if(pszGUID)
                delete[] pszGUID;
        }

        if(bRegenerate)
        {
            CHXString strGUID;
            char* pszGUIDMangled = NULL;
            uuid_tt tmpGUID;
            CHXuuid newGUID;

            newGUID.GetUuid(&tmpGUID);
            if(CHXuuid::HXUuidToString((const uuid_tt*)&tmpGUID, &strGUID) == HXR_OK)
            {
                // mangle the GUID for protection
                pszGUIDMangled = Cipher((char*)strGUID.GetBuffer(strGUID.GetLength()));

                IHXBuffer* lpBuffer = NULL;
		if (m_pPreferences &&
		    HXR_OK == CreateAndSetBufferCCF(lpBuffer, (UCHAR*)pszGUIDMangled, 
						    strlen(pszGUIDMangled)+1, m_pContext))
		{
		    m_pPreferences->WritePref(CLIENT_GUID_REGNAME, lpBuffer);
		    HX_RELEASE(lpBuffer);
                }
                delete[] pszGUIDMangled;
            }
        }

        // create/initialize the preferences which don't exist  
        CreatePrefIfNoExist("AutoTransport", "1");
        CreatePrefIfNoExist("SendStatistics", "1");

        CreatePrefIfNoExist("AttemptRTSPvMulticast", "1");
        CreatePrefIfNoExist("AttemptRTSPvUDP", "1");
        CreatePrefIfNoExist("AttemptRTSPvTCP", "1");
        CreatePrefIfNoExist("AttemptRTSPvHTTP", "1");

        CreatePrefIfNoExist("RTSPProxySupport", "0");
        CreatePrefIfNoExist("RTSPProxyHost", "");
        CreatePrefIfNoExist("RTSPProxyPort", "554");

        CreatePrefIfNoExist("AttemptPNAvMulticast", "1");
        CreatePrefIfNoExist("AttemptPNAvUDP", "1");
        CreatePrefIfNoExist("AttemptPNAvTCP", "1");
        CreatePrefIfNoExist("AttemptPNAvHTTP", "1");

        CreatePrefIfNoExist("PNAProxySupport", "0");
        CreatePrefIfNoExist("PNAProxyHost", "");
        CreatePrefIfNoExist("PNAProxyPort", "1090");

        CreatePrefIfNoExist("HTTPProxySupport", "0");
        CreatePrefIfNoExist("HTTPProxyHost", "");
        CreatePrefIfNoExist("HTTPProxyPort", "80");

        InitPaths();

        UINT32 ulMinBandwidth = DEFAULT_MAX_BANDWIDTH;
        /* Add default min/max bandwidth */

        if (m_pPreferences)
        {
            m_pPreferences->ReadPref("Bandwidth", pValue);
            if (!pValue || (atoi((const char*)pValue->GetBuffer()) == 0))
            {
                HX_RELEASE(pValue);

		if (HXR_OK == CreateBufferCCF(pValue, m_pContext))
		{
		    pValue->SetSize(15);    
		    sprintf((char*)pValue->GetBuffer(), "%u", DEFAULT_MAX_BANDWIDTH); /* Flawfinder: ignore */
		    m_pPreferences->WritePref("Bandwidth", pValue);
		}
            }

            ulMinBandwidth = ::atoi((const char*)pValue->GetBuffer());
        HX_TRACE( "Bandwidth=%d b (%d kb)", ulMinBandwidth, ulMinBandwidth/1024 );

            HX_RELEASE(pValue);
        }

        UINT32 ulMaxBandwidth = 0;
        ReadPrefUINT32(m_pPreferences, "MaxBandwidth", ulMaxBandwidth);

        if (ulMaxBandwidth < ulMinBandwidth)
        {
            /* If we did read some value from the pref but somehow
            * it was set lower to the Bandwidth value, we make
            * it equal to the Bandwidth value...else set it
            * to the default max bandwdith value.
            */
            if (ulMaxBandwidth > 0)
            {
                ulMaxBandwidth = ulMinBandwidth;
            }
            else
            {
                ulMaxBandwidth = DEFAULT_MAX_BANDWIDTH;
            }

            HX_RELEASE(pValue);

	    CreateBufferCCF(pValue, m_pContext);
            pValue->SetSize(15);    

            if (ulMaxBandwidth < ulMinBandwidth)
            {
                ulMaxBandwidth = ulMinBandwidth;
            }

            if (m_pPreferences)
            {
                sprintf((char*)pValue->GetBuffer(), "%lu", ulMaxBandwidth); /* Flawfinder: ignore */
                m_pPreferences->WritePref("MaxBandwidth", pValue);
            }
        }

        HX_RELEASE(pValue);
    }    
#endif /* HELIX_FEATURE_PREFERENCES */

#if defined(_WIN32) || defined(THREADS_SUPPORTED)
    if (!m_bUseCoreThreadExternallySet)
    {
        ReadPrefBOOL(m_pPreferences, "UseCoreThread", m_bUseCoreThread);  
    }
#endif /*_WIN32*/

    InitializeThreadedObjects();

#if defined(HELIX_FEATURE_AUDIO)
    if (m_pAudioSession)
    {
    if (!m_LastError)
    {
        m_LastError = m_pAudioSession->Init((IUnknown*) (IHXClientEngine*)this);
        m_pAudioSession->SetCoreMutex(m_pCoreMutex);
    }
    }
#endif /* HELIX_FEATURE_AUDIO */

    // get the resource manager instance
#if defined(HELIX_FEATURE_RESOURCEMGR)
    m_pExternalResourceManager = HXExternalResourceManager::Instance((IUnknown*)(IHXClientEngine*)this);
    m_pResMgr = new CHXResMgr((IUnknown*)(IHXClientEngine*)this);
#endif /* HELIX_FEATURE_RESOURCEMGR */

#if defined(_MACINTOSH) && defined(_CARBON) && defined(THREADS_SUPPORTED) && defined(HELIX_FEATURE_PREFERENCES)
    ReadPrefBOOL(m_pPreferences, "UseMacOptimizedBlitting", m_bUseMacBlitMutex);
#endif

#if defined(HELIX_FEATURE_PROXYMGR)
    m_pProxyManager->Initialize((IUnknown*)(IHXClientEngine*)this);
#endif /* HELIX_FEATURE_PROXYMGR */

#if defined(HELIX_FEATURE_AUTO_BANDWIDTH_DETECTION)
    if (m_pABDCalibrator)
    {
        // set engine up to receive all the ABD calibration results
        m_pABDCalibrator->AddAutoBWCalibrationSink((IHXAutoBWCalibrationAdviseSink*)this);
    }
#endif /* #if defined(HELIX_FEATURE_AUTO_BANDWIDTH_DETECTION) */

#if defined(HELIX_FEATURE_VIEWSOURCE)
    IHXPluginSearchEnumerator* pPluginEnumerator = NULL;
    IHXPluginHandler3* pPluginHandler3 = NULL;
    m_pPlugin2Handler->QueryInterface(IID_IHXPluginHandler3, (void**) &pPluginHandler3);
    if(pPluginHandler3 != NULL)
    {
        HX_RESULT rc = pPluginHandler3->FindGroupOfPluginsUsingStrings(PLUGIN_LOAD_AT_STARTUP, 
                                                                       PLUGIN_CLIENT_ENGINE,
                                                                       NULL,
                                                                       NULL,
                                                                       NULL, 
                                                                       NULL, 
                                                                       pPluginEnumerator);
        if (SUCCEEDED(rc))
        {
            IUnknown* pUnknown = NULL;
            while (HXR_OK == pPluginEnumerator->GetNextPlugin(pUnknown, NULL))
            {
                IHXPlugin* pPlugin = NULL;
                if (HXR_OK == pUnknown->QueryInterface(IID_IHXPlugin, (void**)&pPlugin))
                {
                    pPlugin->InitPlugin((IHXClientEngine*)this);
                    HX_RELEASE(pPlugin);
                }
                HX_RELEASE(pUnknown);
            }
            HX_RELEASE(pPluginEnumerator);
        } 
        HX_RELEASE(pPluginHandler3);
    } 
#endif /*HELIX_FEATURE_VIEWSOURCE*/

#if defined(HELIX_FEATURE_HELIXSIM)
        ReadPrefBOOL(m_pPreferences, "HelixsimLoadTest",  g_bHelixsimLoadTest);
#endif /*HELIX_FEATURE_HELIXSIM*/
    if (!m_LastError)
    {
        m_bInitialized = TRUE;
    }
}

/*
* IUnknown methods
*/


/////////////////////////////////////////////////////////////////////////
//  Method:
//      IUnknown::QueryInterface
//  Purpose:
//      Implement this to export the interfaces supported by your 
//      object.
//
STDMETHODIMP HXClientEngine::QueryInterface(REFIID riid, void** ppvObj)
{
    // might be memory error in constructor...
    if (m_LastError)
        return m_LastError;

    // create the following objects only if needed
#if defined(HELIX_FEATURE_META)
    if (!m_pValidator && IsEqualIID(riid, IID_IHXValidator))
    {
        m_pValidator = new HXValidator((IUnknown*) (IHXClientEngine*)this);

        if (m_pValidator)
        {
            m_pValidator->AddRef();
        }
    }
#endif /* HELIX_FEATURE_META */
#if defined(HELIX_FEATURE_VIEWSOURCE)
    else if (!m_pViewSource && (IsEqualIID(riid, IID_IHXClientViewSourceSink) || IsEqualIID(riid, IID_IHXClientViewSource)))
    {
        m_pViewSource = new HXViewSource((IUnknown*) (IHXClientEngine*)this);
        
        if ( m_pViewSource )
        {
            m_pViewSource->AddRef();
        }
    }
#endif /* HELIX_FEATURE_VIEWSOURCE */

#if defined(HELIX_FEATURE_PAC)
    else if (!m_pProxyAutoConfig && IsEqualIID(riid, IID_IHXProxyAutoConfig))
    {
        IUnknown*   pUnknown = NULL;
        if (HXR_OK == m_pPlugin2Handler->FindPluginUsingStrings(PLUGIN_CLASS, 
            PLUGIN_PAC_TYPE, 
            NULL,
            NULL, 
            NULL, 
            NULL, 
            pUnknown))
        {
            pUnknown->QueryInterface(IID_IHXProxyAutoConfig, (void**)&m_pProxyAutoConfig);
            m_pProxyAutoConfig->Init((IUnknown*) (IHXClientEngine*) this);
        }
        HX_RELEASE(pUnknown);
    }
#endif /* HELIX_FEATURE_PAC */
#if defined(_MACINTOSH) && defined(_CARBON) && defined(THREADS_SUPPORTED)
    else if (!m_pMacBlitMutex && IsEqualIID(riid, IID_IHXMacBlitMutex))
    {
	CreateInstanceCCF(CLSID_IHXMutex, (void**)&m_pMacBlitMutex, m_pContext);  
    }
#endif

    QInterfaceList qiList[] =
    {
        { GET_IIDHANDLE(IID_IUnknown), (IUnknown*)(IHXClientEngine*)this },
        { GET_IIDHANDLE(IID_IHXClientEngine), (IHXClientEngine*)this },
	{ GET_IIDHANDLE(IID_IHXClientEngine2), (IHXClientEngine2*)this },
        { GET_IIDHANDLE(IID_IHXClientEngineMapper), (IHXClientEngineMapper*)this },
#if defined(_UNIX) && !defined(_VXWORKS)
        { GET_IIDHANDLE(IID_IHXClientEngineSelector), (IHXClientEngineSelector*)this },
#endif
        { GET_IIDHANDLE(IID_IHXClientEngineSetup), (IHXClientEngineSetup*)this },
        { GET_IIDHANDLE(IID_IHXInterruptState), (IHXInterruptState*)this },
        { GET_IIDHANDLE(IID_IHXShutDownEverything), (IHXShutDownEverything*)this },
        { GET_IIDHANDLE(IID_IHXOverrideDefaultServices), (IHXOverrideDefaultServices*)this },
        { GET_IIDHANDLE(IID_IHXErrorMessages), (IHXErrorMessages*)this },
        { GET_IIDHANDLE(IID_IHXCoreMutex), (IHXCoreMutex*)this },
#if defined(HELIX_FEATURE_SYSTEM_EXTERNAL_TIMER)
        { GET_IIDHANDLE(IID_IHXExternalSystemClock), (IHXExternalSystemClock*)this },
#endif /*HELIX_FEATURE_SYSTEM_EXTERNAL_TIMER*/

        { GET_IIDHANDLE(IID_IHXAutoBWCalibrationAdviseSink), (IHXAutoBWCalibrationAdviseSink*)this },
	{ GET_IIDHANDLE(IID_IHXContextUser), (IHXContextUser*)this }
    };
    HX_RESULT retval = ::QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
    if (retval == HXR_OK)
    {
        return retval;
    }

#if defined _UNIX && !defined _VXWORKS
    else if (IsEqualIID(riid, IID_IHXAsyncIOSelection))
    {
        if (m_pAsyncIOSelection) 
        {
            m_pAsyncIOSelection->AddRef();
            *ppvObj = m_pAsyncIOSelection;  
            return HXR_OK;
        }
        AddRef();
        *ppvObj = (IHXAsyncIOSelection*)this;
        return HXR_OK;
    }
#endif

#if defined(_MACINTOSH) && defined(_CARBON) && defined(THREADS_SUPPORTED)

    else if (IsEqualIID(riid, IID_IHXMacBlitMutex))
    {
        AddRef();
        *ppvObj = (IHXMacBlitMutex*)this;
        return HXR_OK;
    }

#endif
    else if (m_pCommonClassFactory &&
        m_pCommonClassFactory->QueryInterface(riid, ppvObj) == HXR_OK)
    {
        return HXR_OK;
    }
    else if (m_pScheduler &&
        m_pScheduler->QueryInterface(riid, ppvObj) == HXR_OK)
    {
        return HXR_OK;
    }
    else if (m_pConnBWInfo &&
        HXR_OK == m_pConnBWInfo->QueryInterface(riid, ppvObj))
    {
        return HXR_OK;
    }
#if defined(HELIX_FEATURE_FW_CTLMGR)
    else if (m_pFWCtlMgr &&
        m_pFWCtlMgr->QueryInterface(riid, ppvObj) == HXR_OK)
    {
        return HXR_OK;
    }
#endif /* HELIX_FEATURE_FW_CTLMGR */
#ifdef HELIX_FEATURE_OPTIMIZED_SCHEDULER
    else if (m_pOptimizedScheduler &&
        m_pOptimizedScheduler->QueryInterface(riid, ppvObj) == HXR_OK)
    {
        return HXR_OK;
    }
#endif /* HELIX_FEATURE_OPTIMIZED_SCHEDULER */
#if defined(HELIX_FEATURE_NETSERVICES)
    else if (m_pNetServices && m_pNetServices->QueryInterface(riid, ppvObj)==HXR_OK)
    {
        return HXR_OK;
    }
#if defined(HELIX_FEATURE_NET_LEGACYAPI)
    else if (m_pOldNetServices && m_pOldNetServices->QueryInterface(riid, ppvObj)==HXR_OK)
    {
        return HXR_OK;
    }
#else
    else if(IsEqualIID(riid, IID_IHXNetworkServices))
    {
        // only new API IHXNetServices is available (calling code needs updating)
        HX_ASSERT(FALSE); 
        return HXR_FAIL;
    }
#endif /* HELIX_FEATURE_NET_LEGACYAPI */
#endif /* HELIX_FEATURE_NETSERVICES */
#if defined(HELIX_FEATURE_REGISTRY)
    else if (m_pRegistry &&
        m_pRegistry->QueryInterface(riid, ppvObj) == HXR_OK)
    {
        return HXR_OK;
    }
#endif /* HELIX_FEATURE_REGISTRY */
#if defined(HELIX_FEATURE_AUDIO)
    else if (m_pAudioSession && 
        m_pAudioSession->QueryInterface(riid,ppvObj) == HXR_OK)
    {
        return HXR_OK;
    }
#endif /* HELIX_FEATURE_AUDIO */
#if defined(HELIX_FEATURE_PREFERENCES)
    else if (m_pPreferences &&
        m_pPreferences->QueryInterface(riid, ppvObj) == HXR_OK)
    {
        return HXR_OK;
    }
#endif /* HELIX_FEATURE_PREFERENCES */
    else if (m_pPlugin2Handler &&
        m_pPlugin2Handler->QueryInterface(riid, ppvObj) == HXR_OK)
    {
        return HXR_OK;
    }
    else if (m_pPlayerSinkControl &&
        m_pPlayerSinkControl->QueryInterface(riid, ppvObj) == HXR_OK)
    {
        return HXR_OK;
    }
#if defined(HELIX_FEATURE_ASM)
    else if (m_pASM &&
        m_pASM->QueryInterface(riid, ppvObj) == HXR_OK)
    {
        return HXR_OK;
    }
#endif /* HELIX_FEATURE_ASM */
#if defined(HELIX_FEATURE_META)
    else if (m_pValidator &&
        m_pValidator->QueryInterface(riid, ppvObj) == HXR_OK)
    {
        return HXR_OK;
    }
#endif /* HELIX_FEATURE_META */
#if defined(HELIX_FEATURE_RESOURCEMGR)
    else if (m_pExternalResourceManager &&
        m_pExternalResourceManager->QueryInterface(riid, ppvObj) == HXR_OK)
    {
        return HXR_OK;
    }
#endif /* HELIX_FEATURE_RESOURCEMGR */
#if defined(HELIX_FEATURE_AUTHENTICATION)
    else if (m_pCredentialsCache &&
        m_pCredentialsCache->QueryInterface(riid, ppvObj) == HXR_OK)
    {
        return HXR_OK;
    }
#endif /* HELIX_FEATURE_AUTHENTICATION */
#if defined(_MEDIUM_BLOCK)
    else if (m_pAllocator &&
        m_pAllocator->QueryInterface(riid, ppvObj) == HXR_OK)
    {
        return HXR_OK;
    }
#endif /* _MEDIUM_BLOCK */
#if defined(HELIX_FEATURE_VIEWSOURCE)
    else if (m_pViewSource &&
        m_pViewSource->QueryInterface(riid, ppvObj) == HXR_OK)
    {
        return HXR_OK;
    }
#endif /* HELIX_FEATURE_VIEWSOURCE */
#if defined(HELIX_FEATURE_SYSTEMREQUIRED)
    else if (m_pSystemRequired && 
        m_pSystemRequired->QueryInterface(riid, ppvObj) == HXR_OK)
    {
        return HXR_OK;
    }
#endif /* HELIX_FEATURE_SYSTEMREQUIRED */
#if defined(HELIX_FEATURE_PROXYMGR)
    else if (m_pProxyManager &&
        m_pProxyManager->QueryInterface(riid, ppvObj) == HXR_OK)
    {
        return HXR_OK;
    }    
#endif /* HELIX_FEATURE_PROXYMGR */
#if defined(HELIX_FEATURE_SMARTERNETWORK)
    else if (m_pPreferredTransportManager &&
        m_pPreferredTransportManager->QueryInterface(riid, ppvObj) == HXR_OK)
    {
        return HXR_OK;
    }
#endif /* HELIX_FEATURE_SMARTERNETWORK */
#if defined(HELIX_FEATURE_OVERLAYMGR)
    else if (m_pOverlayManager &&
        m_pOverlayManager->QueryInterface(riid, ppvObj) == HXR_OK)
    {
        return HXR_OK;
    }
#endif /* HELIX_FEATURE_OVERLAYMGR */
    else if (m_pMultiPlayPauseSupport && IsEqualIID(riid, IID_IHXMultiPlayPauseSupport) &&
        m_pMultiPlayPauseSupport->QueryInterface(riid, ppvObj) == HXR_OK)
    {
        return HXR_OK;
    }
#if defined(HELIX_FEATURE_PAC)
    else if (m_pProxyAutoConfig &&
        m_pProxyAutoConfig->QueryInterface(riid, ppvObj) == HXR_OK)
    {
        return HXR_OK;
    }
#endif /* HELIX_FEATURE_PAC */
#if defined(HELIX_FEATURE_AUTO_BANDWIDTH_DETECTION)
    else if (m_pABDCalibrator &&
        m_pABDCalibrator->QueryInterface(riid, ppvObj) == HXR_OK)
    {
        return HXR_OK;
    }
#endif /* #if defined(HELIX_FEATURE_AUTO_BANDWIDTH_DETECTION) */
    else if (m_pContext &&
	     (m_pContext->QueryInterface(riid, ppvObj) == HXR_OK))
    {
	return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//      IUnknown::AddRef
//  Purpose:
//      Everyone usually implements this the same... feel free to use
//      this implementation.
//
STDMETHODIMP_(ULONG32) HXClientEngine::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//      IUnknown::Release
//  Purpose:
//      Everyone usually implements this the same... feel free to use
//      this implementation.
//
STDMETHODIMP_(ULONG32) HXClientEngine::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    if(m_lRefCount == 0)
    {
        delete this;
    }

    return 0;
}


/*
* IHXClientEngine methods
*/

/************************************************************************
*  Method:
*      IHXClientEngine::CreatePlayer
*  Purpose:
*      TBD.
*
*/
STDMETHODIMP HXClientEngine::CreatePlayer(IHXPlayer* &pPlayer)
{
    if (!m_bInitialized)
    {
        _Initialize();
    }

    // might be memory error in constructor...
    if (m_LastError)
    {
        return m_LastError;
    }

#if !defined( HELIX_FEATURE_MIXER )
    //Are we trying to create more then one player?
    if( m_PlayerList.GetCount() == 1 )
    {
        //Creating multiple players without having HELIX_FEATURE_MIXER
        //defined won't work. You will never get any audio from any
        //player because the audio services will be unable to mix the
        //audio from each player.
        HX_ASSERT("Creating multiple players without MIXER support is not supported"==NULL);
        return HXR_NOT_SUPPORTED;
    }
#endif    


    HXPlayer*      lpPlayer        = NewPlayer();
    if( !lpPlayer )
    {
        m_LastError = HXR_OUTOFMEMORY;
        return m_LastError;
    }

    HXLOGL3(HXLOG_CORE, "HXClientEngine[%p]::CreatePlayer(): player [%p] created ", this, lpPlayer);

    CHXAudioPlayer* pAudioPlayer    = 0;
    HX_RESULT       theErr      = HXR_OK;

    UINT32      unRegistryID            = 0;
    char        szPlayerName[MAX_DISPLAY_NAME]  = {0}; /* Flawfinder: ignore */

    if (!lpPlayer)
    {
        return HXR_OUTOFMEMORY;
    }

    /* Local AddRef to keep the object around */
    lpPlayer->AddRef();

    // create registry entry for this player
    SafeSprintf(szPlayerName, MAX_DISPLAY_NAME, "PlaybackEngine%p.Player%ld", this, m_ulPlayerIndex);
    m_ulPlayerIndex++;

#if defined(HELIX_FEATURE_REGISTRY)
    unRegistryID = m_pRegistry->AddComp(szPlayerName);
#endif /* HELIX_FEATURE_REGISTRY */

#if defined(HELIX_FEATURE_AUDIO)
    if (m_pAudioSession)
    {
    theErr = m_pAudioSession->CreateAudioPlayer(&pAudioPlayer);

    if (theErr == HXR_OK)
    {
        theErr = lpPlayer->Init(this, unRegistryID, pAudioPlayer);
        lpPlayer->SetInterrupt(m_bUseCoreThread);

        /* HXPlayer will keep it around */
        HX_RELEASE(pAudioPlayer);
    }
    }
#endif /* HELIX_FEATURE_AUDIO */

    if (theErr == HXR_OK)
    {
        //Return Copy
        pPlayer   = lpPlayer;
        pPlayer->AddRef();

        // Keep Copy
        m_PlayerList.AddHead((void *) pPlayer);
        pPlayer->AddRef();

        m_pPlayerSinkControl->PlayerCreated(pPlayer);
    } 

    //Release Local Copy
    HX_RELEASE(lpPlayer);

    return theErr;
}

CHXAudioSession*
HXClientEngine::NewAudioSession()
{
    return (new CHXAudioSession());
}

HXPlayer*
HXClientEngine::NewPlayer()
{
    return (new HXPlayer());
}

/************************************************************************
*  Method:
*      IHXClientEngine::ClosePlayer
*  Purpose:
*      Called by the engine when it is done using the player...
*
*/
STDMETHODIMP HXClientEngine::ClosePlayer(IHXPlayer*    pPlayer)
{
    HXLOGL3(HXLOG_CORE, "HXClientEngine[%p]::ClosePlayer(): player [%p]", this, pPlayer);

    LISTPOSITION lPosition = m_PlayerList.Find(pPlayer);

    if (lPosition)
    {
        m_PlayerList.RemoveAt(lPosition);
        HXPlayer* pHXPlayer = (HXPlayer*) pPlayer;
        m_pPlayerSinkControl->PlayerClosed(pPlayer);

        pHXPlayer->ClosePlayer();
        pPlayer->Release();

        return HXR_OK;
    }
    else
    {
        return HXR_INVALID_PARAMETER;
    }
}


/************************************************************************
*  Method:
*    IHXClientEngine::GetPlayerCount
*  Purpose:
*    Returns the current number of source instances supported by
*    this player instance.
*/
STDMETHODIMP_(UINT16) HXClientEngine::GetPlayerCount()
{
    return (UINT16)m_PlayerList.GetCount();
}

/************************************************************************
*  Method:
*    IHXClientEngine::GetPlayer
*  Purpose:
*    Returns the Nth source instance supported by this player.
*/
STDMETHODIMP HXClientEngine::GetPlayer
(
 UINT16      nIndex,
 REF(IUnknown*)  pUnknown
 )
{
    LISTPOSITION pos = m_PlayerList.FindIndex(nIndex);

    if (!pos)
    {
        pUnknown = NULL;
        return HXR_INVALID_PARAMETER;
    }

    HXPlayer* pPlayer = (HXPlayer*)m_PlayerList.GetAt(pos);
    HX_ASSERT(pPlayer);

    return pPlayer->QueryInterface(IID_IUnknown,(void**)&pUnknown);
}

/************************************************************************
*  Method:
*      IHXClientEngine::GetPlayerBySite
*  Purpose:
*      Returns the IHXPlayer instance supported by this client 
*      engine instance that contains the specified IHXSite.
*/
STDMETHODIMP HXClientEngine::GetPlayerBySite
(
 IHXSite*           pSite,
 REF(IUnknown*)  pUnknown
 )
{
    pUnknown = NULL;
    HXPlayer* pPlayer = NULL;

    for (int i = 0; i < m_PlayerList.GetCount(); ++i)
    {
        LISTPOSITION pos = m_PlayerList.FindIndex(i);
        pPlayer = (HXPlayer*)m_PlayerList.GetAt(pos);
        if (pPlayer->IsSitePresent(pSite))
            return pPlayer->QueryInterface(IID_IUnknown,(void**)&pUnknown);
    }

    return HXR_FAIL;
}

/************************************************************************
*   Method:
*       IHXClientEngine::EventOccurred
*   Purpose:
*       Clients call this to pass OS events to all players. HXxEvent
*       defines a cross-platform event.
*/
STDMETHODIMP HXClientEngine::EventOccurred(HXxEvent* pEvent)
{
    //If the TLC is passing in an event, we need to create
    //our SiteEventHandler here first.
#if defined(HELIX_FEATURE_VIDEO)
    if ((NULL != pEvent) && (NULL == m_pSiteEventHandler))
    {
        QueryInterface(IID_IHXSiteEventHandler, (void**) &m_pSiteEventHandler);
        HX_ASSERT( m_pSiteEventHandler );
    }
#endif	// HELIX_FEATURE_VIDEO
    
#ifdef _MAC_UNIX
    HX_LOCK(m_pCoreMutex);
	
    LISTPOSITION lPosition = m_PlayerList.GetHeadPosition();
    HXPlayer* pPlayer;
    while (lPosition != NULL)
    {
        pPlayer = (HXPlayer*) m_PlayerList.GetNext(lPosition);
        pPlayer->EventOccurred(pEvent);
    }
    
    HX_UNLOCK(m_pCoreMutex);
#endif	// _MAC_UNIX
	
    if (m_pSiteEventHandler && pEvent)
    {
        m_pSiteEventHandler->EventOccurred(pEvent);
    }

    if (m_pKicker)
    {
        m_pKicker->Kick(HXGetCurrentThreadID(), NULL);
    }

#if defined(__TCS__)
#if defined(HELIX_FEATURE_NETSERVICES)
    if (!pEvent)
    {
        tm1_TCP::process_idle();
    }
#endif //HELIX_FEATURE_NETSERVICES
#endif  // __TCS__

    return HXR_OK;
}

#ifdef _UNIX
/************************************************************************
*  Method:
*      IHXClientEngine::Select
*  Purpose:
*      Top level clients under Unix should use this instead of
*      select() to select for events.
*/
STDMETHODIMP_(INT32)
HXClientEngine::Select(INT32 n,
                       fd_set* readfds,
                       fd_set* writefds,
                       fd_set* exceptfds,
struct timeval* timeout)
{
    int s=0;

    //XXXgfw no need for these selects if we aren't doing network i/o.
    //Of course, it would be nice for those using this instead of a sleep
    //to time their callbacks back into the core, that it works...
    //I guess we can just turn this into a sleep for timeval seconds if
    //we are not doing any network playback. Of course, if you have a
    //threaded core it won't hurt to much either way.
#if defined(HELIX_FEATURE_NETSERVICES)
    fd_set real_readfds;
    fd_set real_writefds;
    fd_set real_exceptfds;
    struct timeval tv;
    static unix_TCP* un = 0;

    if(!un)
    {
        un = new unix_TCP((IUnknown*)(IHXClientEngine*)this); 
    }

    if(readfds)
    {
        real_readfds = *readfds;
    }
    else
    {
        FD_ZERO(&real_readfds);
    }

    if(writefds)
    {
        real_writefds = *writefds;
    }
    else
    {
        FD_ZERO(&real_writefds);
    }


    if(exceptfds)
    {
        real_exceptfds = *exceptfds;
    }
    else
    {
        FD_ZERO(&real_exceptfds);
    }

    if(timeout)
    {
        tv = *timeout;
    }
    else
    {
        /* If no timeout was passed in, and no player adds a timeout,
        we will not pass a timeout to select()
        */
        tv.tv_sec = -1;
        tv.tv_usec = -1;
    }

    /* It is assumed that the networking code does not care about select
    timeouts, so it is only asked for socket info.
    */
    un->add_select((int*)&n, &real_readfds, &real_writefds, &real_exceptfds);

    CHXSimpleList::Iterator lIterator   = m_PlayerList.Begin();
    LISTPOSITION lPosition = m_PlayerList.GetHeadPosition();
    HXPlayer* pPlayer;

#if defined(HELIX_FEATURE_HELIXSIM)
  if(g_bHelixsimLoadTest)
  {
#endif //defined(HELIX_FEATURE_HELIXSIM)
    while (lPosition != NULL)
    {
        pPlayer = (HXPlayer*) m_PlayerList.GetNext(lPosition);
        pPlayer->CollectSelectInfo(&n, 
            &real_readfds,
            &real_writefds,
            &real_exceptfds,
            &tv);
    }
#if defined(HELIX_FEATURE_HELIXSIM)
  }
#endif //defined(HELIX_FEATURE_HELIXSIM)    

    for(lIterator = m_select_callbacks->Begin();
        lIterator != m_select_callbacks->End();
        ++lIterator)
    {
        CHXSelectCallback* scb = (CHXSelectCallback*)(*lIterator);

        if(scb->m_flags & PNAIO_READ)
            FD_SET(scb->m_lFileDescriptor, &real_readfds);

        if(scb->m_flags & PNAIO_WRITE)
            FD_SET(scb->m_lFileDescriptor, &real_writefds);

        if(scb->m_flags & PNAIO_EXCEPTION)
            FD_SET(scb->m_lFileDescriptor, &real_exceptfds);

        if(scb->m_lFileDescriptor > n)
            n = scb->m_lFileDescriptor + 1;
    }

    UINT32 ulEventTime = 0;
#if defined(HELIX_FEATURE_HELIXSIM)
   // the timeout passed in from helixsim is 10ms, for loadtest with
  // a few hundred players, the tv get from pq is alway 0.
  // helixsim loadtest is more like a network application than a
  // render machine, so we want it to take a break if no packet arriving.
  // those callback will be delayed at most 10ms, no big deal.

  // The performance test shows 7-10% improvement over this change.
  if(g_bHelixsimLoadTest == FALSE)
  {
#endif //defined(HELIX_FEATURE_HELIXSIM)
    if(m_pScheduler2)
    {
        if(m_pScheduler2->GetNextEventDueTimeDiff(ulEventTime))
        {
            if(ulEventTime < ((tv.tv_sec * 1000) + (tv.tv_usec / 1000)))
            {
                tv.tv_sec = ulEventTime / 1000;
                tv.tv_usec = (ulEventTime % 1000) * 1000;
            }
        }
    }
#if defined(HELIX_FEATURE_HELIXSIM)
  }
#endif //defined(HELIX_FEATURE_HELIXSIM)
    if(timeout || (tv.tv_sec >= 0 && tv.tv_usec >= 0))
        s = ::select(n, &real_readfds, &real_writefds, &real_exceptfds, &tv);
    else
        s = ::select(n, &real_readfds, &real_writefds, &real_exceptfds, 0);

    un->process_select(n, &real_readfds, &real_writefds, &real_exceptfds);
#if defined(HELIX_FEATURE_HELIXSIM)
  if(g_bHelixsimLoadTest)
  {
#endif //defined(HELIX_FEATURE_HELIXSIM)
    lIterator   = m_PlayerList.Begin();
    lPosition = m_PlayerList.GetHeadPosition();

    while (lPosition != NULL)
    {
        pPlayer = (HXPlayer*) m_PlayerList.GetNext(lPosition);
        pPlayer->ProcessSelect(&n, 
            &real_readfds,
            &real_writefds,
            &real_exceptfds,
            &tv);
    }
#if defined(HELIX_FEATURE_HELIXSIM)
  }
#endif //defined(HELIX_FEATURE_HELIXSIM)
    for(lIterator = m_select_callbacks->Begin();
        lIterator != m_select_callbacks->End();
        ++lIterator)
    {
        CHXSelectCallback* scb = (CHXSelectCallback*)(*lIterator);

        if((scb->m_flags & PNAIO_READ) && 
            FD_ISSET(scb->m_lFileDescriptor, &real_readfds))
        {
            scb->m_pCallback->Func();
        }

        if((scb->m_flags & PNAIO_WRITE) &&
            FD_ISSET(scb->m_lFileDescriptor, &real_writefds))
        {
            scb->m_pCallback->Func();
        }

        if((scb->m_flags & PNAIO_EXCEPTION) &&
            FD_ISSET(scb->m_lFileDescriptor, &real_exceptfds))
        {
            scb->m_pCallback->Func();
        }
    }
    ulEventTime = 0;
    if(m_pScheduler2)
    {
        if(m_pScheduler2->GetNextEventDueTimeDiff(ulEventTime))
        {
            if(ulEventTime == 0)
            {
                if( m_pKicker )
                {
                    m_pKicker->Kick( HXGetCurrentThreadID(), NULL );
                }
            }
        }
    }
#else
    s = ::select(0, NULL, NULL, NULL, timeout);
#endif //HELIX_FEATURE_NETSERVICES
    return s;
}

STDMETHODIMP
HXClientEngine::Add(IHXCallback* pCallback,
                    INT32 lFileDescriptor,
                    UINT32 ulFlags)
{
    CHXSelectCallback* scb = new CHXSelectCallback(pCallback,
        lFileDescriptor,
        ulFlags);
    m_select_callbacks->AddTail(scb);

    return HXR_OK;
}

STDMETHODIMP
HXClientEngine::Remove(INT32 lFileDescriptor,
                       UINT32 ulFlags)
{
    CHXSimpleList::Iterator i;
    CHXSelectCallback* scb;

    for(i = m_select_callbacks->Begin();
        i != m_select_callbacks->End();
        ++i)
    {
        scb = (CHXSelectCallback*)(*i);
        if(scb->m_lFileDescriptor == lFileDescriptor && 
            scb->m_flags == ulFlags)
        {
            m_select_callbacks->RemoveAt(m_select_callbacks->Find(scb));
            delete scb;
            return HXR_OK;
        }
    }
    return HXR_FAIL;
}

#endif /* _UNIX */

/*
*  IHXMimeTypeMapper methods
*/

STDMETHODIMP HXClientEngine::MapFromExtToMime
(
 const char*     /*IN*/  pExtension,
 REF(const char*)    /*OUT*/ pMimeType
 )
{
#if defined(HELIX_FEATURE_PLUGINHANDLER2)
    HX_RESULT hr = HXR_OK;
    if (!m_pPlugin2Handler)
    {
        hr = HXR_FAIL;
        goto cleanup;
    }

    // try to map from the normal file format plugins

    UINT32 unPluginIndex; 
    IHXValues* pValues; 
    IHXBuffer* pBuffer; 

    // Note that the ref count of the buffer is not increased this is bad. But should be OK in 
    // this case.

    if (HXR_OK == m_pPlugin2Handler->FindIndexUsingStrings(PLUGIN_CLASS, PLUGIN_FILEFORMAT_TYPE, 
        PLUGIN_FILEEXTENSIONS, (char*)pExtension, NULL, NULL, unPluginIndex))
    {
        m_pPlugin2Handler->GetPluginInfo(unPluginIndex, pValues);
        if (HXR_OK == pValues->GetPropertyCString(PLUGIN_FILEMIMETYPES, pBuffer))
        {
            pMimeType = (const char*)pBuffer->GetBuffer();
            pBuffer->Release();
        }
        pValues->Release();
    }

cleanup:

    return hr;
#else
    return HXR_FAIL;
#endif /* HELIX_FEATURE_PLUGINHANDLER2 */
}

/*
* IHXClientEngineSetup methods
*/

/************************************************************************
*  Method:
*      IHXClientEngineSetup::Setup
*  Purpose:
*      Top level clients use this interface to over-ride certain basic 
*      interfaces are: IHXPreferences, IHXHyperNavigate
*/
STDMETHODIMP HXClientEngine::Setup(IUnknown* pContext)
{
    if (m_bInitialized) 
        return HXR_UNEXPECTED;

    /* Make sure this is called before any player is created */
    HX_ASSERT(GetPlayerCount() == 0);
    if (GetPlayerCount() > 0)
    {
        return HXR_UNEXPECTED;
    }

    /* Override Default objects */

    if (pContext)
    {
#ifdef _UNIX
	    IHXAsyncIOSelection* pAsyncIOSelection = 0;
	    if (HXR_OK == pContext->QueryInterface(IID_IHXAsyncIOSelection, (void**) &pAsyncIOSelection))
	    {
		HX_RELEASE(m_pAsyncIOSelection);
		m_pAsyncIOSelection = pAsyncIOSelection;
	    }

#endif

#if defined(HELIX_FEATURE_SYSTEMREQUIRED)
	    IHXSystemRequired* pSystemRequired = NULL;
	    if (HXR_OK == pContext->QueryInterface(IID_IHXSystemRequired, (void**) &pSystemRequired))
	    {
		HX_RELEASE(m_pSystemRequired);
		m_pSystemRequired = pSystemRequired;
	    }
#endif /* HELIX_FEATURE_SYSTEMREQUIRED */

	    IHXMultiPlayPauseSupport* pMPPSupport = NULL;
	    if (HXR_OK == pContext->QueryInterface(IID_IHXMultiPlayPauseSupport, (void**) &pMPPSupport))
	    {
		HX_RELEASE(m_pMultiPlayPauseSupport);
		m_pMultiPlayPauseSupport = pMPPSupport;
	    }

	    // XXXLCM move all overridable interfaces to OverrideServices()?
	    OverrideServices(pContext);
    }

    _Initialize();

    return HXR_OK;
}

/*
*   HXClientEngine methods
*/

STDMETHODIMP
HXClientEngine::Close()
{    
    HXLOGL3(HXLOG_CORE, "HXClientEngine[%p]::Close()", this);
    /* There should not be any outstanding players */
    HX_ASSERT(m_PlayerList.GetCount() == 0);

#ifdef _SYMBIAN
    SymbianMemoryMonitor *pMonitor = SymbianMemoryMonitor::Instance();
    if (pMonitor)
    {
       pMonitor->Close();
    }
#endif
#if 0 // XXX HP Atlas
#if defined(HELIX_FEATURE_PREFERENCES)
    // nhart: if prefs have been overridden, we want to release them before we
    // unload our plugins and restore the original prefs.
    if (m_pPreferences != m_pOrigPreferences)
    {
        HX_RELEASE(m_pPreferences);
        m_pPreferences = m_pOrigPreferences;
        HX_ADDREF(m_pPreferences);
    }
#endif /* HELIX_FEATURE_PREFERENCES */
#endif

    HX_RELEASE(m_pSiteEventHandler);

    HX_RELEASE(m_pConnBWInfo);

    CHXSimpleList::Iterator ndxPlayer = m_PlayerList.Begin();
    for (; ndxPlayer != m_PlayerList.End(); ++ndxPlayer)
    {
        HXPlayer* pHXPlayer = (HXPlayer*) (*ndxPlayer);
        m_pPlayerSinkControl->PlayerClosed(pHXPlayer);
        pHXPlayer->ClosePlayer();
        pHXPlayer->Release();
    }

    m_PlayerList.RemoveAll();
    if (m_pPlayerSinkControl)
        m_pPlayerSinkControl->Terminate();
    HX_RELEASE(m_pPlayerSinkControl);

#if defined(HELIX_FEATURE_AUDIO)
    if (m_pAudioSession)
    {
        m_pAudioSession->Close();
        HX_RELEASE(m_pAudioSession);
    }
#endif /* HELIX_FEATURE_AUDIO */

#if defined(HELIX_FEATURE_REGISTRY)
    if (m_pRegistry)
    {
        // remove the registry
        if (m_unRegistryID)
        {
            m_pRegistry->DeleteById(m_unRegistryID);
            m_unRegistryID = 0;
        }
        HX_RELEASE(m_pRegistry);
    }
#endif /* HELIX_FEATURE_REGISTRY */

#if defined(HELIX_FEATURE_FW_CTLMGR)
    if (m_pFWCtlMgr)
    {
        m_pFWCtlMgr->Close();
        HX_RELEASE(m_pFWCtlMgr);
    }
#endif /* HELIX_FEATURE_FW_CTLMGR */

#if defined(HELIX_FEATURE_PAC)
    if (m_pProxyAutoConfig)
    {
        m_pProxyAutoConfig->Close();
        HX_RELEASE(m_pProxyAutoConfig);
    }
#endif /* HELIX_FEATURE_PAC */

#if defined(HELIX_FEATURE_NETSERVICES)
    HX_RELEASE(m_pNetServices);
#if defined(HELIX_FEATURE_NET_LEGACYAPI)
    HX_RELEASE(m_pOldNetServices);
#endif /* HELIX_FEATURE_NET_LEGACYAPI */
#endif /* HELIX_FEATURE_NETSERVICES */

#ifdef _MEDIUM_BLOCK
    if (m_pAllocator)
    {
        m_pAllocator->SetScheduler(NULL);
    }
#endif /* _MEDIUM_BLOCK */

    HX_RELEASE(m_pScheduler2);
    HX_RELEASE(m_pScheduler);
#if defined(HELIX_FEATURE_OPTIMIZED_SCHEDULER)
    HX_RELEASE(m_pOptimizedScheduler);
#endif /* HELIX_FEATURE_OPTIMIZED_SCHEDULER */

#if defined(HELIX_FEATURE_ASM)
    HX_RELEASE(m_pASM);
#endif /* HELIX_FEATURE_ASM */
#if defined(HELIX_FEATURE_META)
    HX_RELEASE(m_pValidator);
#endif /* HELIX_FEATURE_META */
#if defined(HELIX_FEATURE_RESOURCEMGR)
    HX_RELEASE(m_pExternalResourceManager);
    HX_DELETE(m_pResMgr);
#endif /* HELIX_FEATURE_RESOURCEMGR */
#if defined(HELIX_FEATURE_VIEWSOURCE)
    HX_RELEASE(m_pViewSource);
#endif /* HELIX_FEATURE_VIEWSOURCE */
#if defined(HELIX_FEATURE_SYSTEMREQUIRED)
    HX_RELEASE(m_pSystemRequired);
#endif /* HELIX_FEATURE_SYSTEMREQUIRED */
    HX_RELEASE(m_pMultiPlayPauseSupport);

#if defined(HELIX_FEATURE_PROXYMGR)
    if (m_pProxyManager)
    {
        m_pProxyManager->Close();
        HX_RELEASE(m_pProxyManager);
    }
#endif /* HELIX_FEATURE_PROXYMGR */

#if defined(HELIX_FEATURE_AUTO_BANDWIDTH_DETECTION)
    if (m_pABDCalibrator)
    {
        m_pABDCalibrator->RemoveAutoBWCalibrationSink((IHXAutoBWCalibrationAdviseSink*)this);
        m_pABDCalibrator->Close();
        HX_RELEASE(m_pABDCalibrator);
    }
#endif /* #if defined(HELIX_FEATURE_AUTO_BANDWIDTH_DETECTION) */

#if defined(HELIX_FEATURE_SMARTERNETWORK)
    if (m_pPreferredTransportManager)
    {
        m_pPreferredTransportManager->Close();
        HX_RELEASE(m_pPreferredTransportManager);
    }
#endif /* HELIX_FEATURE_SMARTERNETWORK */

#if defined(HELIX_FEATURE_OVERLAYMGR)
    if (m_pOverlayManager)
    {
        m_pOverlayManager->Close();
        HX_RELEASE(m_pOverlayManager);
    }
#endif /* HELIX_FEATURE_OVERLAYMGR */

#if defined(HELIX_FEATURE_AUTHENTICATION)
    if (m_pCredentialsCache)
    {
        m_pCredentialsCache->Close();
        HX_RELEASE(m_pCredentialsCache);
    }
#endif /* HELIX_FEATURE_AUTHENTICATION */

#if defined(HELIX_FEATURE_CORECOMM)
    HX_DELETE(m_pCoreComm);
#endif /* HELIX_FEATURE_CORECOMM */

#ifdef _WIN32
    HXCloseLibrary();
#endif

#if defined(_MACINTOSH) && defined(_CARBON) && defined(THREADS_SUPPORTED)

    HX_DELETE(m_pMacBlitMutex);

#endif

// XXXHP 
// Moved maintenance of singleton ThreadEngine object to Media Platform   
#if 0
#if defined(HELIX_FEATURE_NETSERVICES)
#if defined(HELIX_FEATURE_NETSERVICES_SHIM)
#ifdef THREADS_SUPPORTED
    ThreadEngine::DestroyThreadEngine();
#elif defined(_UNIX_THREADED_NETWORK_IO)
    if( m_bNetworkThreading )
        ThreadEngine::DestroyThreadEngine();
#endif /*THREADS_SUPPORTED*/
#ifdef _MACINTOSH
    conn::close_drivers(NULL);
#endif
#endif //HELIX_FEATURE_NETSERVICES_SHIM
#endif /* HELIX_FEATURE_NETSERVICES */
#endif

    CHXBuffer::ReleaseAllocator();
#ifdef _MEDIUM_BLOCK
    HX_RELEASE(m_pAllocator);
#endif /* _MEDIUM_BLOCK */

#if 0 // XXX HP Atlas
#if defined(HELIX_FEATURE_PREFERENCES)
    // XXXKB HXPlayer uses preferences on destruction;
    // Since Close() will nuke the underlying implementation
    // regardless of ref. count, we need to place this last:
    HX_RELEASE(m_pPreferences);
    if (m_pOrigPreferences)
    {
#if !defined(HELIX_FEATURE_NO_INTERNAL_PREFS)
        m_pOrigPreferences->Close();
#endif /* !defined(HELIX_FEATURE_NO_INTERNAL_PREFS) */

        HX_RELEASE(m_pOrigPreferences);
    }
#endif /* HELIX_FEATURE_PREFERENCES */
#endif

#ifdef _UNIX
    if (m_select_callbacks)
    {
        while (m_select_callbacks->GetCount() > 0)
        {
            CHXSelectCallback* scb = (CHXSelectCallback*) m_select_callbacks->RemoveHead();
            HX_DELETE(scb);
        }
    }
    HX_DELETE(m_select_callbacks);

#endif /* _UNIX */

    HX_RELEASE(m_pCommonClassFactory);
    HX_RELEASE(m_pCoreMutex);
    HX_RELEASE(m_pPlugin2Handler);
    HX_RELEASE(m_pPreferences);
    HX_RELEASE(m_pContext);
    
#ifdef _UNIX    
    HX_RELEASE(m_pKicker);
#endif

    m_bInitialized = FALSE;

    return HXR_OK;
}

/*
* IRMAInterruptState methods
*/

// xxxbobclark There's a subtle difference between the Mac implementation
// and the other platforms' implementation. Here's what Rahul has to say
// about the matter:
//      a) AtInterruptTime = (CurrentThread != SystemThread ? TRUE : FALSE)
//      b) AtInterruptTime = (CurrentThread == CoreThread ? TRUE : FALSE)
//
//      SystemThread = Thread that the core was loaded on. i.e. Main App Thread
//      Core Thread = Internal Thread spawned by the core to do core processing.
//
//      The actual definition of AtInterruptTime is supposed to be a) but it somehow is coded
//      as b), at least for Windows. The reason it has worked till now is because there really
//      are just two threads on Windows on which the core processing is done, so even with b),
//      if the current thread is NOT core thread, then it must be system thread.
//
//      Renderers (and callbacks) that DO NOT claim to be interrupt safe are supposed to be
//      called ONLY on the SystemThread.
//
//      So, on Mac, since IsMacInCooperativeThread is equivalent to being at system time,
//
//	AtInterruptTime = !IsMacInCooperativeThread().
STDMETHODIMP_(HXBOOL)
HXClientEngine::AtInterruptTime()
{
#if defined(_MACINTOSH)
    return !IsMacInCooperativeThread();
#elif defined(_MAC_UNIX)
    return (::MPTaskIsPreemptive(::MPCurrentTaskID()));
#elif defined (_WIN32) || defined(THREADS_SUPPORTED)
    return m_pScheduler2 ? m_pScheduler2->IsAtInterruptTime() : FALSE;
#else
    return FALSE;
#endif 
}

STDMETHODIMP 
HXClientEngine::EnterInterruptState()
{
#ifdef _MACINTOSH
#ifndef _MAC_MACHO
    HXMM_INTERRUPTON();
#endif
#endif 
    return HXR_OK;
}

STDMETHODIMP 
HXClientEngine::LeaveInterruptState()
{
#ifdef _MACINTOSH
#ifndef _MAC_MACHO
    HXMM_INTERRUPTOFF();
#endif
#endif 
    return HXR_OK;
}

STDMETHODIMP 
HXClientEngine::EnableInterrupt(HXBOOL   bEnable)
{
    /* Make sure all the players are in a stopped state */
    CHXSimpleList::Iterator ndx = m_PlayerList.Begin();
    for (; ndx != m_PlayerList.End(); ++ndx)
    {
        HXPlayer* pHXPlayer = (HXPlayer*) (*ndx);
        HX_ASSERT(pHXPlayer->IsPlaying() == FALSE);
        if (pHXPlayer->IsPlaying())
        {
            return HXR_FAILED;
        }
    }

    m_bUseCoreThreadExternallySet = TRUE;

#ifdef _MACINTOSH
    /* Currently Interrupts are ALWAYS enabled on Mac */
    if (!bEnable)
    {
        return HXR_FAILED;
    }
    return HXR_OK;
#elif THREADS_SUPPORTED
    if (!m_bInitialized)
    {
        m_bUseCoreThread = bEnable;

    }
    else if (m_bUseCoreThread != bEnable)
    {
        m_bUseCoreThread = bEnable;
        InitializeThreadedObjects();
    }
    return HXR_OK;
#else
    return HXR_FAILED;
#endif
}

STDMETHODIMP_(HXBOOL) 
HXClientEngine::IsInterruptEnabled()
{
#ifdef _MACINTOSH
    return TRUE;    //HXMM_INTERRUPTOFF();
#elif THREADS_SUPPORTED
    return m_bUseCoreThread;
#else
    return FALSE;
#endif
}

void
HXClientEngine::InitializeThreadedObjects()
{
    m_pContext->QueryInterface(IID_IHXScheduler, (void**)&m_pScheduler);
    if (m_pScheduler)
    {
	m_pScheduler->QueryInterface(IID_IHXScheduler2, (void**)&m_pScheduler2);
    }

#if defined(HELIX_FEATURE_OPTIMIZED_SCHEDULER)
    m_pContext->QueryInterface(IID_IHXOptimizedScheduler, (void**)&m_pOptimizedScheduler);
#endif /* HELIX_FEATURE_OPTIMIZED_SCHEDULER */

    // find out if scheduler support interrupt (aka core thread)
    HXBOOL bInterruptEnabled = TRUE;
    IHXSchedulerInterruptSupport* pInterruptSupport = NULL;
    if (SUCCEEDED(m_pScheduler->QueryInterface(IID_IHXSchedulerInterruptSupport, 
                                               (void**)&pInterruptSupport)))
    {
         bInterruptEnabled = pInterruptSupport->IsInterruptEnabled();
         HX_RELEASE(pInterruptSupport);
    }
    //if scheduler doesn't support core thread, nothing we can do
    if (!bInterruptEnabled)
    {
        m_bUseCoreThread = FALSE; 
    }

    CHXSimpleList::Iterator ndx = m_PlayerList.Begin();
    for (; ndx != m_PlayerList.End(); ++ndx)
    {
        HXPlayer* pHXPlayer = (HXPlayer*) (*ndx);
        pHXPlayer->SetInterrupt(m_bUseCoreThread);
    }

#ifdef _UNIX    
    //See if we have network threading turned on for unix
    ReadPrefBOOL(m_pPreferences, "NetworkThreading", m_bNetworkThreading); 
#endif

}

void        
HXClientEngine::InitializeRegistry()
{
#if defined(HELIX_FEATURE_REGISTRY)
    IHXBuffer* pBuffer = NULL;
    CHXString   strTemp;
    char        szRegName[MAX_DISPLAY_NAME]  = {0}; /* Flawfinder: ignore */

    // If there has been an error just return
    if (m_LastError)
        return;

    if (HXR_OK != m_pContext->QueryInterface(IID_IHXRegistry, (void**)&m_pRegistry))
    {
	return;
    }

    // create registry entry for statistics
    SafeSprintf(szRegName, MAX_DISPLAY_NAME, "PlaybackEngine%p", this);
    m_unRegistryID = m_pRegistry->AddComp(szRegName);

    // Preferences that TLC's can override are stored in here
    m_pRegistry->AddComp(HXREGISTRY_PREFPROPNAME);

    IHXStatistics* pStats = NULL;
    if (m_pConnBWInfo &&
        HXR_OK == m_pConnBWInfo->QueryInterface(IID_IHXStatistics, (void**)&pStats))
    {
        pStats->InitializeStatistics(m_unRegistryID);
    }
    HX_RELEASE(pStats);

    // RegionData
    strTemp.Format("%s.%s",HXREGISTRY_PREFPROPNAME,"RegionData");
    if(m_pRegistry->GetStrByName(strTemp,pBuffer) != HXR_OK)
    {
        pBuffer = CreateBufferAndSetToString("0");
        m_pRegistry->AddStr(strTemp,pBuffer);
    }

    HX_RELEASE(pBuffer);

    // UserAddress
    strTemp.Format("%s.%s",HXREGISTRY_PREFPROPNAME,"UserAddress");
    if(m_pRegistry->GetStrByName(strTemp,pBuffer) != HXR_OK)
    {
        pBuffer = CreateBufferAndSetToString("");
        m_pRegistry->AddStr(strTemp,pBuffer);
    }

    HX_RELEASE(pBuffer);

    // Title
    strTemp.Format("%s.%s",HXREGISTRY_PREFPROPNAME,"Title");
    if(m_pRegistry->GetStrByName(strTemp,pBuffer) != HXR_OK)
    {
        pBuffer = CreateBufferAndSetToString("");
        m_pRegistry->AddStr(strTemp,pBuffer);
    }

    HX_RELEASE(pBuffer);

#ifdef _WINDOWS
    INT16 nLangId = LANGIDFROMLCID(GetSystemDefaultLCID());
#else
    INT16 nLangId = MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_US);
#endif // _WINDOWS

    // LangID
    strTemp.Format("%s.%s",HXREGISTRY_PREFPROPNAME,"LangID");
    if(m_pRegistry->GetStrByName(strTemp,pBuffer) != HXR_OK)
    {
        CHXString strLangID;
        strLangID.Format("%hd",nLangId);
        pBuffer = CreateBufferAndSetToString(strLangID);
        m_pRegistry->AddStr(strTemp,pBuffer);
    }

    HX_RELEASE(pBuffer);

#if !defined(_WINCE) || (_WIN32_WCE >= 400)
    // Language
    strTemp.Format("%s.%s",HXREGISTRY_PREFPROPNAME,"Language");
    if(m_pRegistry->GetStrByName(strTemp,pBuffer) != HXR_OK)
    {
        CHXString strLanguage;
        strLanguage = CHXLang::GetISO639(CHXLang::FindClosest(nLangId));
        pBuffer = CreateBufferAndSetToString(strLanguage);
        m_pRegistry->AddStr(strTemp,pBuffer);
    }
#endif

    HX_RELEASE(pBuffer);
#endif /* HELIX_FEATURE_REGISTRY */
}

IHXBuffer*
HXClientEngine::CreateBufferAndSetToString(const char* pStr)
{
    IHXBuffer* pBuffer = NULL;

    if (m_pCommonClassFactory &&
	HXR_OK == m_pCommonClassFactory->CreateInstance(CLSID_IHXBuffer, (void**)&pBuffer))
    {
        pBuffer->Set((const unsigned char*)pStr,strlen(pStr)+1);
    }

    return pBuffer;
}


/*
* IHXShutDownEverything methods
*/

/************************************************************************
*  Method:
*      IHXShutDownEverything::ShutDown
*  Purpose:
*      Shutdown all the renderers/fileformats
*
*/
STDMETHODIMP
HXClientEngine::ShutDown()
{
    HXLOGL3(HXLOG_CORE, "HXClientEngine[%p]::ShutDown()", this);

    CHXSimpleList::Iterator ndxPlayer = m_PlayerList.Begin();
    for (; ndxPlayer != m_PlayerList.End(); ++ndxPlayer)
    {
        HXPlayer* pHXPlayer = (HXPlayer*) (*ndxPlayer);
        pHXPlayer->ShutDown();
    }

#if defined(HELIX_FEATURE_META)
    if (m_pValidator)
    {
        m_pValidator->RefreshProtocols();
    }
#endif /* HELIX_FEATURE_META */

    return HXR_OK;
}



/************************************************************************
*  Method:
*      IHXShutDownEverything::StopAllOtherPlayers
*  Purpose:
*      Shutdown all players
*
*/

STDMETHODIMP
HXClientEngine::StopAllOtherPlayers()
{
#if defined(HELIX_FEATURE_CORECOMM)
    if (m_pCoreComm)
    {
        return m_pCoreComm->StopAllOtherAudioPlayers();
    }
#endif /* HELIX_FEATURE_CORECOMM */

    return HXR_OK;
}

/************************************************************************
*  Method:
*      IHXShutDownEverything::AskAllOtherPlayersToUnload
*  Purpose:
*      Ask all other players in other processes to unload their 
*      unused DLLs.
*/

STDMETHODIMP
HXClientEngine::AskAllOtherPlayersToUnload()
{
#if defined(HELIX_FEATURE_CORECOMM)
    if (m_pCoreComm)
    {
        return m_pCoreComm->AskAllOtherPlayersToUnload();
    }
#endif /* HELIX_FEATURE_CORECOMM */

    return HXR_OK;
}

/************************************************************************
*  Method:
*      IHXShutDownEverything::AskAllOtherPlayersToReload
*  Purpose:
*      Ask all other players in other processes to reload their 
*      DLLs.
*
*/

STDMETHODIMP
HXClientEngine::AskAllOtherPlayersToReload()
{
#if defined(HELIX_FEATURE_CORECOMM)
    if (m_pCoreComm)
    {
        return m_pCoreComm->AskAllOtherPlayersToReload();
    }
#endif /* HELIX_FEATURE_CORECOMM */

    return HXR_OK;
}


/************************************************************************
*  Method:
*      IHXOverrideDefaultServices::OverrideServices
*  Purpose:
*      Override default services provided by the G2 system.
* 
*/
STDMETHODIMP
HXClientEngine::OverrideServices(IUnknown* pContext)
{
    if (!pContext)
    {
        return HXR_UNEXPECTED;
    }

#if defined(HELIX_FEATURE_NETSERVICES)
    /* override (new) IHXNetServices */
    IHXNetServices* pNetSvc = NULL;
    if (pContext->QueryInterface(IID_IHXNetServices, (void**) &pNetSvc)
        == HXR_OK)
    {
        HX_RELEASE(m_pNetServices);
        m_pNetServices = pNetSvc;
    }
#if defined(HELIX_FEATURE_NET_LEGACYAPI)
    /* override (old) IHXNetworkServices */
    IHXNetworkServices* pOldNetSvc = NULL;
    if (pContext->QueryInterface(IID_IHXNetworkServices, (void**) &pOldNetSvc)
        == HXR_OK)
    {
        HX_RELEASE(m_pOldNetServices);
        m_pOldNetServices = pOldNetSvc;
    }
#endif /* HELIX_FEATURE_NET_LEGACYAPI */
#endif /* HELIX_FEATURE_NETSERVICES */

    /* override IHXPreferences */
    IHXPreferences* pPreferences = NULL;
    if (pContext->QueryInterface(IID_IHXPreferences, (void**) &pPreferences)
        == HXR_OK)
    {
        HX_RELEASE(m_pPreferences);
        m_pPreferences = pPreferences;

        DPRINTF_INIT(m_pPreferences);
        DPRINTF(D_INFO, ("HXClientEngine::OverrideServices(): DPRINTF initialized\n"));
    }

    return HXR_OK;
}

/*
* IHXErrorMessages methods
*/

/************************************************************************
*  Method:
*      IHXErrorMessages::Report
*  Purpose:
*      Call this method to report an error, event, or status message.
*  Parameters:
*
*      const UINT8 unSeverity
*      Type of report. This value will impact how the player, tool, or
*      server will react to the report. Possible values are described 
*      above. Depending on the error type, an error message with the 
*      RMA code, anda string translation of that code will be displayed. 
*      The error dialog includes a "more info" section that displays the
*      user code and string, and a link to the more info URL. In the 
*      server these messages are logged to the log file.
*
*      const ULONG32   ulHXCode
*      Well known RMA error code. This will be translated to a text
*      representation for display in an error dialog box or log file.
*
*      const ULONG32   ulUserCode
*      User specific error code. This will NOT be translated to a text
*      representation. This can be any value the caller wants, it will
*      be logged or displayed but not interpretted.
*
*      const char*     pUserString
*      User specific error string. This will NOT be translated or 
*      modified. This can be any value the caller wants, it will
*      be logged or displayed but not interpretted.
*
*      const char*     pMoreInfoURL
*      User specific more info URL string.
*
*/
STDMETHODIMP
HXClientEngine::Report
(
 const UINT8 unSeverity,
 HX_RESULT   ulHXCode,
 const ULONG32       ulUserCode,
 const char* pUserString,
 const char* pMoreInfoURL
 )
{
    if (m_PlayerList.GetCount())
    {
        HXPlayer* pHXPlayer = (HXPlayer*)m_PlayerList.GetTail();
        pHXPlayer->Report(unSeverity,
            ulHXCode,
            ulUserCode,
            pUserString,
            pMoreInfoURL);
        return HXR_OK;
    }
    return HXR_NOTIMPL;
}

/************************************************************************
*  Method:
*      IHXErrorMessages::GetErrorText
*  Purpose:
*      Call this method to get the text description of a RMA error code.
*  Parameters:
*      HX_RESULT ulHXCode (A RMA error code)
*  Return Value:
*      IHXBuffer* containing error text.
*/
STDMETHODIMP_ (IHXBuffer*)
HXClientEngine::GetErrorText(HX_RESULT ulHXCode)
{
#if defined(HELIX_FEATURE_RESOURCEMGR)
    if (!m_bInitialized)
    {
        _Initialize();
    }
    return GetResMgr()->GetErrorString(ulHXCode);
#else
    return NULL;
#endif /* HELIX_FEATURE_RESOURCEMGR */
}

/************************************************************************
*  Method:
*      HXClientEngine::StopAudioPlayback
*  Purpose:
*      Stop all players in this process if we use audio.
*
*/
STDMETHODIMP
HXClientEngine::StopAudioPlayback()
{
    HXLOGL3(HXLOG_CORE, "HXClientEngine[%p]::StopAudioPlayback()", this);
    HXBOOL bAudio = FALSE;

    // First look for audio
    CHXSimpleList::Iterator ndxPlayer = m_PlayerList.Begin();
    for (; !bAudio && ndxPlayer != m_PlayerList.End(); ++ndxPlayer)
    {
        HXPlayer* pHXPlayer = (HXPlayer*) (*ndxPlayer);
        IUnknown* pUnk;

        HX_ASSERT(pHXPlayer);
        if (pHXPlayer->QueryInterface(IID_IHXAudioPlayer, (void**)&pUnk) == HXR_OK)
        {
            HX_ASSERT(pUnk);

            bAudio = ((IHXAudioPlayer*)pUnk)->GetAudioStreamCount() > 0;

            pUnk->Release();
        }
    }

    // If we use audio, stop all the players.
    if (bAudio)
    {
        ndxPlayer = m_PlayerList.Begin();
        for (; ndxPlayer != m_PlayerList.End(); ++ndxPlayer)
        {
            HXPlayer* pHXPlayer = (HXPlayer*) (*ndxPlayer);
            pHXPlayer->Stop();
        }
    }

    return HXR_OK;
}

void    
HXClientEngine::NotifyPlayState(HXBOOL bInPlayingState)
{
    if (m_pScheduler2)
    {
        m_pScheduler2->NotifyPlayState(bInPlayingState);
    }
}

//initialize the user's path, and store it in the registry
void
HXClientEngine::InitPaths()
{
#if !defined(__TCS__)
    HXBOOL bShouldUseDocsAndSettingsPath = FALSE;
#ifdef _WIN32
    HXVERSIONINFO pnvi;

    ::memset(&pnvi, 0, sizeof(pnvi));
    HXGetWinVer(&pnvi);
    if(pnvi.dwPlatformId == HX_PLATFORM_WINNT)
    {
        //this will be for win2k, nt, and XP
        bShouldUseDocsAndSettingsPath = TRUE;
    }
#endif

#if defined(_CARBON) || defined(_MAC_UNIX)
    bShouldUseDocsAndSettingsPath = TRUE;
#endif

    IHXBuffer* pBuffer = NULL;
    CHXString strUserDataPath;

#if defined(_CARBON) || defined(_MAC_UNIX)
    if (1) // on the Mac we'll always determine this at runtime since paths are not stable between runs
#else
    if (!m_pPreferences ||
        m_pPreferences->ReadPref("UserSDKDataPath", pBuffer) != HXR_OK)
#endif
    {
        HXBOOL bCreated = FALSE;
        // see if RealPlayer App Data Path is set and derive SDK path
        // from that
        if (bShouldUseDocsAndSettingsPath && m_pPreferences &&
            m_pPreferences->ReadPref("UserDataPath", pBuffer) == HXR_OK)
        {
            char* pDataPath = (char*) pBuffer->GetBuffer();
            char* pRealPlayerStart = ::strstr(pDataPath, "RealPlayer");
            if (pRealPlayerStart)
            {
                char* pTemp = new_string(pDataPath);
                pTemp[pRealPlayerStart-pDataPath] = '\0';
                strUserDataPath = pTemp;
                delete [] pTemp;
                strUserDataPath += HXVER_SDK_PRODUCT;
                strUserDataPath += OS_SEPARATOR_STRING;
                bCreated = TRUE;
            }

            HX_RELEASE(pBuffer);
        }

        if (!bCreated)
        {
            if (bShouldUseDocsAndSettingsPath)
            {
#if defined _WIN32 || defined _CARBON || defined _MAC_UNIX
                // Do this for Win 2K, XP, and Mac OS X
                CHXDirSpecifier dirBase = CHXFileSpecUtils::GetAppDataDir(HXVER_SDK_PRODUCT);
                strUserDataPath = dirBase.GetPathName();
                if(strUserDataPath[strUserDataPath.GetLength() - 1] != OS_SEPARATOR_CHAR)
                {
                    strUserDataPath += OS_SEPARATOR_STRING;
                }
#endif
            }
            else
            {
#ifdef _UNIX
                strUserDataPath = (const char*) getenv("HOME");
                strUserDataPath += OS_SEPARATOR_CHAR;
                strUserDataPath += ".helix";
                strUserDataPath += OS_SEPARATOR_CHAR;
#else
                strUserDataPath = (const char*) GetDLLAccessPath()->GetPath(DLLTYPE_COMMON);
#endif        
            }
            // need to create our own
        }

	if (m_pPreferences &&
	    HXR_OK == CreateAndSetBufferCCF(pBuffer, (UCHAR*)(const char*) strUserDataPath, 
					    strlen((const char*) strUserDataPath) + 1, m_pContext))
	{
            m_pPreferences->WritePref("UserSDKDataPath", pBuffer);
        }
    }
    else
    {
        strUserDataPath = pBuffer->GetBuffer();
    }

#ifdef _WIN32
    DirCreatePath(strUserDataPath);
#endif

    HX_RELEASE(pBuffer);
#endif /* __TCS__ */
}

/************************************************************************
*  Method:
*      IHXCoreMutex::LockCoreMutex
*  Purpose:
*      Call this method to lock the client engine's core mutex.
*/
STDMETHODIMP
HXClientEngine::LockCoreMutex()
{
    HX_RESULT retval = HXR_FAIL;
    if (m_pCoreMutex)
    {
        retval = HXR_OK;
        m_pCoreMutex->Lock();
    }
    return retval;
}

/************************************************************************
*  Method:
*      IHXCoreMutex::UnlockCoreMutex
*  Purpose:
*      Call this method to unlock the client engine's core mutex.
*/
STDMETHODIMP
HXClientEngine::UnlockCoreMutex()
{
    HX_RESULT retval = HXR_FAIL;
    if (m_pCoreMutex)
    {
        retval = HXR_OK;
        m_pCoreMutex->Unlock();
    }
    return retval;
}

STDMETHODIMP
HXClientEngine::AutoBWCalibrationDone(HX_RESULT  status,
                                      UINT32     ulBW)
{
    HXLOGL3(HXLOG_CORE, "HXClientEngine[%p]::AutoBWCalibrationDone(): bw = %ul; res = 0x%08x", this, ulBW, status);

#if defined(HELIX_FEATURE_SMARTERNETWORK)
    if (HXR_OK == status && ulBW)
    {
        m_pPreferredTransportManager->SetAutoBWDetectionValue(ulBW);
    }
#endif /* HELIX_FEATURE_SMARTERNETWORK */

    return HXR_OK;
}

STDMETHODIMP
HXClientEngine::RegisterContext(IUnknown* pIContext)
{
    if (m_pContext)
    {
	return HXR_FAILED;
    }

    m_pContext = pIContext;
    HX_ADDREF(m_pContext);

    m_pContext->QueryInterface(IID_IHXCommonClassFactory, (void**)&m_pCommonClassFactory);
    m_pContext->QueryInterface(IID_IHXMutex, (void**)&m_pCoreMutex);
    m_pContext->QueryInterface(IID_IHXPlugin2Handler, (void**)&m_pPlugin2Handler);
    m_pContext->QueryInterface(IID_IHXPreferences, (void**)&m_pPreferences);

#ifdef _UNIX
    if( NULL == m_pKicker)
    {
        m_pContext->QueryInterface(IID_IHXMediaPlatformKicker, (void**)&m_pKicker);
        HX_ASSERT(m_pKicker);
    }
#endif    

    return HXR_OK;
}

#if defined(_MACINTOSH) && defined(_CARBON) && defined(THREADS_SUPPORTED)

/************************************************************************
*  Method:
*      IHXMacBlitMutex::LockMacBlitMutex
*  Purpose:
*      Call this method to lock the Mac blitting mutex
*/
STDMETHODIMP
HXClientEngine::LockMacBlitMutex(THIS)
{
    HX_RESULT retval = HXR_FAIL;
    if (m_pMacBlitMutex)
    {
        retval = HXR_OK;
        if (m_bUseMacBlitMutex) m_pMacBlitMutex->Lock();
    }
    return retval;
}

/************************************************************************
*  Method:
*      IHXMacBlitMutex::UnlockMacBlitMutex
*  Purpose:
*      Call this method to unlock the Mac blitting mutex
*/
STDMETHODIMP
HXClientEngine::UnlockMacBlitMutex(THIS)
{
    HX_RESULT retval = HXR_FAIL;
    if (m_pMacBlitMutex)
    {
        retval = HXR_OK;
        if (m_bUseMacBlitMutex) m_pMacBlitMutex->Unlock();
    }
    return retval;
}

#endif
/*
*************************IMPORTANT*******************************************
The current implementation of making use of external clock depends on setting
of these two variables g_ulHXExternalTimerTick, g_bExternalTimerUsed declared
in gettickcount.c. Since each shared library/dll gets its own version of these 
vars, they need to be set in each of the shard libs/dll to turn on this code. 

In the current incarnation, only the client core dll sets these. This is 
because this support is currently used only by helixsim that makes use of 
null renderer, so the only component that makes use of HX_GETTICKCOUNT 
extensively is client core.

Please note that if this feature is used by some other app that uses actual 
renderer and other plugins, calls to HX_GET_TICKCOUNT in those components
will still make use of gettimeofday() even ifHELIX_FEATURE_SYSTEM_EXTERNAL_TIMER
is turned on. 


The "proper fix" to propage use of externally set clock to all the plugins 
is yet to be implemented.
*************************IMPORTANT*******************************************
*/
#if defined(HELIX_FEATURE_SYSTEM_EXTERNAL_TIMER)

/* this is defined only in UNIX gettickcount.c impelemntation */
#ifdef _UNIX
extern ULONG32 g_ulHXExternalTimerTick;
extern HXBOOL g_bExternalTimerUsed;
#endif
/*
 * 	IHXExternalSystemClock methods
 */

/************************************************************************
 *  Method:
 *      
 *  Purpose:
 *      TBD.
 *
 */
  
STDMETHODIMP HXClientEngine::InitClock(HXTimeval* pCurTime)
{
/* this is defined only in UNIX gettickcount.c impelemntation */
#ifdef _UNIX
        g_bExternalTimerUsed = TRUE;
#endif //_UNIX

	if (!m_pCurrentTime)
	{
		m_pCurrentTime = new HXTimeval;
	}

    UpdateClock(pCurTime);
    return HXR_OK;
}
 
 
STDMETHODIMP HXClientEngine::UpdateClock(HXTimeval* pCurTime) 
{   
	if (!m_pCurrentTime)
	{
		return HXR_UNEXPECTED;
	}

    m_pCurrentTime->tv_sec = pCurTime->tv_sec;
    m_pCurrentTime->tv_usec = pCurTime->tv_usec;

/* this is defined only in UNIX gettickcount.c impelemntation */
#ifdef _UNIX
    g_ulHXExternalTimerTick = m_pCurrentTime->tv_sec * 1000 + m_pCurrentTime->tv_usec / 1000;
#endif //_UNIX
}

STDMETHODIMP_ (HXBOOL) HXClientEngine::IsExternalClockUsed(void)
{
    return (m_pCurrentTime ? TRUE : FALSE);
}

STDMETHODIMP_ (ULONG32) HXClientEngine::GetCurrentClock(HXTimeval* pCurTime)
{
    if(!pCurTime || !m_pCurrentTime)
    {
        return 0;
    }

    pCurTime->tv_sec  = m_pCurrentTime->tv_sec;
    pCurTime->tv_usec = m_pCurrentTime->tv_usec;

    return (ULONG32)((pCurTime->tv_sec*1000) + (pCurTime->tv_usec/1000)); 
}
#endif /*HELIX_FEATURE_SYSTEM_EXTERNAL_TIMER*/



