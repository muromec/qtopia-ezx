/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxprotocol.cpp,v 1.18 2006/01/31 23:37:50 ping Exp $
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

#include "hxcom.h"
#include "hlxclib/string.h"
#include "hxresult.h"
#include "hxtypes.h"
#include "hxassert.h"

#if defined(_WINDOWS) || defined(WIN32)
#include "platform/win/win_net.h"
#endif

#include "hxcomm.h"
#include "ihxpckts.h"
#include "hxfiles.h"
#include "hxengin.h"
#include "chxpckts.h"
#include "hxcore.h"
#include "hxprefs.h"
#include "hxpref.h"
#include "hxpends.h"

#include "hxslist.h"
#include "hxstring.h"
#include "chxelst.h"
#include "chxeven.h"
#include "strminfo.h"
#include "hxntsrc.h"
#include "hxtick.h"
#include "hxmangle.h"

#include "hxprotocol.h"

#include "hxheap.h"

#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif

HXProtocol::HXProtocol(HXNetSource* owner, ULONG32 ulPlatformData) :
	  m_ulRegistryID (0)
	, m_pRegistry (0)
	, m_lRefCount(0)
	, mProxyVersion (0)
	, mLiveStream(FALSE)
	, mSaveAsAllowed(FALSE)
	, m_bPerfectPlayAllowed(FALSE)
	, m_bPrefetch(FALSE)
	, m_bFastStart(FALSE)
	, m_bIsFirstResume(TRUE)
	, mProtocolValid (FALSE)
	, m_bConnectDone (FALSE)
	, mSourceEnd (FALSE)
	, mUseUDPPort (FALSE)
	, mFlowControl (FALSE)
	, m_bPerfectPlay (FALSE)
	, mUseProxy (FALSE)
	, mUsingMulticast (FALSE)
	, m_bHTTPOnly(FALSE)
	, m_bPaused (FALSE)
	, mLossCorrection (FALSE)
	, m_bAreResuming(FALSE)
	, m_bSDPInitiated(FALSE)
	, m_bHTTPvProxy(FALSE)
	, mProtocolVersion (0)
	, mAtInterrupt (0)
	, mLocked (0)
	, mLocale (0)
	, mServerPort (0)
	, mOwner (owner)
	, m_uUDPPort (0)
	, mSendStatsMask (1)
	, mProxyPort (0)
	, mCloakPort(0)
	, mNumFlowControl(0)
	, mServerTimeout (0)
	, m_LastError(HXR_OK)
	, m_pPreferences(0)
	, m_pCredentialsCache(NULL)
	, m_ulServerVersion(0)
	, m_ulLastAlert(0)
	, m_pTextBuf(NULL)
	, m_pCloakPorts(NULL)
	, m_nCloakPorts(0)
	, mCurrentTransport(UnknownMode)
	, m_ulTransportPrefMask(0)
	, m_pContext(NULL)
{
    if (mOwner)
    {
	mOwner->AddRef();
	mOwner->QueryInterface(IID_IHXPreferences, (void **) &m_pPreferences);

	m_pRegistry = mOwner->m_pRegistry;	 

	if (HXR_OK != mOwner->QueryInterface(IID_IHXCredentialsCache, (void**)&m_pCredentialsCache))
	{
	    m_pCredentialsCache = NULL;
	}

	// in case of the Auto. Config
        mOwner->GetContext(m_pContext);
    }
}

HXProtocol::~HXProtocol(void)
{
    HX_RELEASE(m_pPreferences);
    HX_RELEASE(m_pCredentialsCache);
    HX_RELEASE(mOwner);

    HX_VECTOR_DELETE(m_pTextBuf);
    HX_RELEASE(m_pContext);
}

// these get initialized every time we make a new connection with 
// the server
void HXProtocol::initialize_members()
{
    IHXBuffer*	pBuffer = NULL;

    mProtocolValid		= FALSE;
    mFlowControl		= FALSE;
    mLiveStream			= FALSE;
    mSaveAsAllowed		= FALSE;
    m_bPerfectPlayAllowed	= FALSE;
    mNumFlowControl		= 0;

    HXBOOL	    bCanSendGUID    = FALSE;
    ReadPrefBOOL(m_pPreferences, "AllowAuthID", bCanSendGUID);  

    if(bCanSendGUID &&
       m_pPreferences && m_pPreferences->ReadPref(CLIENT_GUID_REGNAME, pBuffer) == HXR_OK)
    {
	char* psz = DeCipher((char*)pBuffer->GetBuffer());
        m_guid = psz;
        HX_VECTOR_DELETE(psz);
    }
    else
    {
	m_guid = CLIENT_ZERO_GUID;	
    }

    HX_ASSERT(m_guid.GetLength() > 0);

    HX_RELEASE(pBuffer);
}

HX_RESULT
HXProtocol::setup(const char *host, const char *path, UINT16 port, HXBOOL LossCorrection, 
		  HXBOOL bHTTPCloak, HXBOOL bSDPInitiated, UINT16 cloakPort)
{
    HX_RESULT theErr = HXR_OK;

    if (bSDPInitiated)
    {
        mPath = path; 
    }
    else
    {
        //	get out immediately if we have bogus parameters
        if(!host || !*host)
	    theErr = HXR_DNR;
	    
        if(!theErr && !path)
	    theErr = HXR_INVALID_PATH;
	    
        // initialize member variables
        if(!theErr)
        {
            //	Save a copy of the parameters
	    mHost = host;
	    mPath = path;
           
	    // strip off fragment
	    char* pFragment = (char*)strchr(mPath, '#');
	    if(pFragment)
	    {
	        *pFragment = '\0';
	    }

	    mServerPort = port;
	    mLossCorrection = LossCorrection;
	    m_bHTTPOnly = bHTTPCloak;
	    mCloakPort = cloakPort;
        }
    }

    return(theErr);
}

HX_RESULT
HXProtocol::process_idle (HXBOOL atInterrupt)
{
    HX_RESULT theErr = HXR_OK;

    if(mLocked) 			// process is locked
	return HXR_OK;
	
    mLocked = 1;			// lock out interrupt processing

    if (atInterrupt)
    {
	mAtInterrupt = 1;
    }
	
    theErr = process();
    		
    mAtInterrupt = 0;
    mLocked = 0;			// enable interrupt processing			

    return(theErr);
}

HX_RESULT
HXProtocol::set_client_id(char *clientID) 
{
    m_clientID = clientID; 
    return HXR_OK;
}

// sets up HXProtocol to use a RealAudio proxy
HX_RESULT
HXProtocol::set_proxy(const char* proxy, UINT16 port)
{	
    mProxy = proxy;
    mProxyPort = port;
    mUseProxy = TRUE;
	
    return HXR_OK;
}

void
HXProtocol::LeavePrefetch(void)
{
    m_bPrefetch = FALSE;

    return;
}

void
HXProtocol::SetCloakPortAttempted(UINT16* pCloakPorts, UINT8 nCloakPorts)
{
    m_pCloakPorts = pCloakPorts;
    m_nCloakPorts = nCloakPorts;
}

HX_RESULT	
HXProtocol::stop(void)
{
    mSourceEnd = TRUE;
    HX_RELEASE(mOwner);

    return HXR_OK;
}

#if defined(HELIX_FEATURE_STATS) && defined(HELIX_FEATURE_REGISTRY)
void 
HXProtocol::statistics_cat(char* pStats, UINT32 ulBufLen, LONG32 lData)
{
    char numb[12]; /* Flawfinder: ignore */

    SafeSprintf(numb, 12, "%10lu ", lData); /* Flawfinder: ignore */
    SafeStrCat(pStats,numb, ulBufLen);
}

void
HXProtocol::statistics_cat_ext(char* pszStats, UINT32 ulBufLen, LONG32 lData, char* pszSep, UINT32& ulCount)
{
    char numb[12]; /* Flawfinder: ignore */

    if (pszSep)
    {
	SafeSprintf(numb, 12, "%lu%s", lData, pszSep);
    }
    else
    {
	SafeSprintf(numb,12, "%lu", lData);
    }
    SafeStrCat(pszStats, numb, ulBufLen);
    ulCount += strlen(numb);
}

HX_RESULT
HXProtocol::prepare_statistics(UINT32 ulStatsMask, char*& pszStats)
{
    HX_RESULT	    rc = HXR_OK;
    char	    szRegKeyName[MAX_DISPLAY_NAME] = {0}; /* Flawfinder: ignore */
    char*	    pszCodec = NULL;
    INT32	    lRAStreamNumber = -1;
    UINT32	    i = 0;
    UINT32	    ulStatsLength = 0;
    UINT32	    ulTransport = 0;
    UINT32	    ulLength = 0;
    STREAM_STATS*   pStreamStats = NULL;
    CHXSimpleList*  pLogInfoList = NULL;
    IHXBuffer*	    pParentName = NULL;
    IHXBuffer*	    pValue = NULL;

    pszStats = NULL;

    // collect level 3 stats info.
    ulStatsLength = MAX_DISPLAY_NAME + mOwner->GetLogInfo(pLogInfoList);

    // get the RA stream number
    // Level 1,2 pszStats only apply to RealAudio
    lRAStreamNumber = mOwner->GetRAStreamNumber();

    if (lRAStreamNumber >= 0)
    {
	// retrieve the pszStats
	if (HXR_OK != GetStreamStatistics((UINT32)lRAStreamNumber, &pStreamStats))
	{
	    goto cleanup;
	}

	if (!pStreamStats || !pStreamStats->m_bInitialized)
	{
	    goto cleanup;
	}

	// retreive the pszStats set by the renderer
	if (m_pRegistry &&
            HXR_OK == m_pRegistry->GetPropName(pStreamStats->m_pRenderer->m_ulRegistryID, pParentName))
	{
	    SafeSprintf(szRegKeyName, MAX_DISPLAY_NAME, "%s.Codec", pParentName->GetBuffer());

	    if (HXR_OK == m_pRegistry->GetStrByName(szRegKeyName, pValue) && pValue)
	    {
		ulLength = pValue->GetSize();

		pszCodec = new char[ulLength + 1];
		strcpy(pszCodec, (const char*)pValue->GetBuffer()); /* Flawfinder: ignore */

		// replace space with underscore
		for (i = 0; i < ulLength; i++)
		{
		    if (pszCodec[i] == ' ')
		    {
			pszCodec[i] = '_';
		    }
		}
		HX_RELEASE(pValue);
	    }
	}
	HX_RELEASE(pParentName);

	if (pszCodec)
	{
	    ulStatsLength += 2 * strlen(pszCodec);
	}

	pszStats = new CHAR[ulStatsLength];
	memset(pszStats, 0, ulStatsLength);

	if (ulStatsMask & 1UL)
	{
	    SafeStrCat(pszStats, "Stat1:", ulStatsLength);

	    // build statistics string
	    statistics_cat(pszStats, ulStatsLength, pStreamStats->m_pReceived->GetInt());
	    statistics_cat(pszStats, ulStatsLength, pStreamStats->m_pReceived->GetInt() -
				    pStreamStats->m_pNormal->GetInt());
	    statistics_cat(pszStats, ulStatsLength, pStreamStats->m_pLost->GetInt());
	    statistics_cat(pszStats, ulStatsLength, 0);	// no packets sent early in 6.0
	    statistics_cat(pszStats, ulStatsLength, pStreamStats->m_pLate->GetInt());

	    if (!pszCodec)
	    {
		SafeStrCat(pszStats, "N/A", ulStatsLength);	
	    }
	    else
	    {
		SafeStrCat(pszStats, pszCodec, ulStatsLength);
	    }
	}

	if (ulStatsMask & 2UL)
	{
	    // divide pszStats levels if necessary
	    if (ulStatsMask & 1UL)
	    {
		SafeStrCat(pszStats, "][", ulStatsLength);
	    }

	    SafeStrCat(pszStats, "Stat2:", ulStatsLength);

	    // Bandwidth info
	    statistics_cat(pszStats, ulStatsLength, pStreamStats->m_pClipBandwidth->GetInt());
	    statistics_cat(pszStats, ulStatsLength, pStreamStats->m_pAvgBandwidth->GetInt());

	    // Latency info
	    statistics_cat(pszStats, ulStatsLength, pStreamStats->m_pHighLatency->GetInt());
	    statistics_cat(pszStats, ulStatsLength, pStreamStats->m_pLowLatency->GetInt());
	    statistics_cat(pszStats, ulStatsLength, pStreamStats->m_pAvgLatency->GetInt());

	    // Resend info
	    statistics_cat(pszStats, ulStatsLength, pStreamStats->m_pResendRequested->GetInt());
	    statistics_cat(pszStats, ulStatsLength, pStreamStats->m_pResendReceived->GetInt());
	    statistics_cat(pszStats, ulStatsLength, pStreamStats->m_pLate->GetInt());

	    // rebuffer info (in percent)
	    statistics_cat(pszStats, ulStatsLength, 0);

	    // Transport
	    // 0 or UDP, 1 for TCP, 2 for Multicast...
	    if (mCurrentTransport == UDPMode)
	    {
		ulTransport = 0L;
	    }
	    else if (mCurrentTransport == TCPMode)
	    {
		ulTransport = 1L;
	    }
	    else if (mCurrentTransport == MulticastMode)
	    {
		ulTransport = 2L;
	    }
	    statistics_cat(pszStats, ulStatsLength, ulTransport);

	    // Startup latency, first data packet arrives!
	    statistics_cat(pszStats, ulStatsLength, mOwner->GetFirstDataArriveTime());

	    if (!pszCodec)
	    {
		SafeStrCat(pszStats, "N/A", ulStatsLength);
	    }
	    else
	    {
		SafeStrCat(pszStats, pszCodec, ulStatsLength);
	    }
	}
    }

    if ((ulStatsMask & 4UL) && pLogInfoList && pLogInfoList->GetCount())
    {
	if (!pszStats)
	{
	    pszStats = new CHAR[ulStatsLength];
	    memset(pszStats, 0, ulStatsLength);
	}
	
	// divide stats levels if necessary
        if((lRAStreamNumber >= 0) && ((ulStatsMask & 1UL) || (ulStatsMask & 2UL)))
        {
            SafeStrCat(pszStats, "][", ulStatsLength);
        }

        SafeStrCat(pszStats, "Stat3:", ulStatsLength);
	CHXSimpleList::Iterator ndx = pLogInfoList->Begin();
	for (; ndx != pLogInfoList->End(); ++ndx)
	{
	    char* pszInfo = (char*) (*ndx);
	    SafeStrCat(pszStats, pszInfo, ulStatsLength);
	}
    }

    if (!pszStats || strlen(pszStats) == 0)
    {
	// nothing to send
	goto cleanup;
    }
    else
    {
	SafeStrCat(pszStats, "]", ulStatsLength);
    }

cleanup:

    HX_VECTOR_DELETE(pszCodec);

    return HXR_OK;
}
#endif /* HELIX_FEATURE_STATS && HELIX_FEATURE_REGISTRY */
