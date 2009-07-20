/* ***** BEGIN LICENSE BLOCK ***** 
 * Version: RCSL 1.0/RPSL 1.0 
 *  
 * Portions Copyright (c) 1995-2002 RealNetworks, Inc. All Rights Reserved. 
 *      
 * The contents of this file, and the files included with this file, are 
 * subject to the current version of the RealNetworks Public Source License 
 * Version 1.0 (the "RPSL") available at 
 * http://www.helixcommunity.org/content/rpsl unless you have licensed 
 * the file under the RealNetworks Community Source License Version 1.0 
 * (the "RCSL") available at http://www.helixcommunity.org/content/rcsl, 
 * in which case the RCSL will apply. You may also obtain the license terms 
 * directly from RealNetworks.  You may not use this file except in 
 * compliance with the RPSL or, if you have a valid RCSL with RealNetworks 
 * applicable to this file, the RCSL.  Please see the applicable RPSL or 
 * RCSL for the rights, obligations and limitations governing use of the 
 * contents of the file.  
 *  
 * This file is part of the Helix DNA Technology. RealNetworks is the 
 * developer of the Original Code and owns the copyrights in the portions 
 * it created. 
 *  
 * This file, and the files included with this file, is distributed and made 
 * available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER 
 * EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS ALL SUCH WARRANTIES, 
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS 
 * FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT. 
 * 
 * Technology Compatibility Kit Test Suite(s) Location: 
 *    http://www.helixcommunity.org/content/tck 
 * 
 * Contributor(s): 
 *  
 * ***** END LICENSE BLOCK ***** */ 

#include <stdio.h>

#include "hxtypes.h"

#include "hxcom.h"
#include "hxcomm.h"
#include "hxmon.h"
#include "hxcore.h"
#include "hxengin.h"
#include "hxclsnk.h"
#include "hxgroup.h"
#include "hxstrutl.h"
#include "exadvsnk.h"

#include "hxstring.h"

#include "print.h"

#include "globals.h"

struct _stGlobals*& GetGlobal(); //in main.cpp

#ifdef __TCS__
extern unsigned long   gStreamTime;
 
static int		iOpened = 0;


#if	defined(__cplusplus)
extern	"C"	{
#endif	/* defined(__cplusplus) */

typedef enum hookBuffering {
	eContacting = 0,
	eConnecting = 1,
	eBuffering = 2,
	ePlaying = 3
}hookBuffering;

void hookRealAudio_Buffering(hookBuffering connectState, int pct);

void hookRealAudio_PlayPosition(unsigned long current,unsigned long duration);

typedef enum hookState {
	ePlay = 0,
	ePause = 1,
	eStop = 2,
	eResume = 3,
	eComplete				// Clip is done playing
}hookState;
void hookRealAudio_State(hookState newState);


#if	defined(__cplusplus)
}
#endif	/* defined(__cplusplus) */

#endif // __TCS__

void PrintBuffer(const char* pszName,const unsigned char* pbBuf, unsigned int dwBytes);

ExampleClientAdviceSink::ExampleClientAdviceSink(IUnknown* pUnknown, LONG32 lClientIndex)
    : m_lRefCount (0)
    , m_lClientIndex (lClientIndex)
    , m_pUnknown (NULL)
    , m_pRegistry (NULL)
    , m_pScheduler (NULL)
    , m_lCurrentBandwidth(0)
    , m_lAverageBandwidth(0)
    , m_bOnStop(0)
	, m_pPlayer(0)
    , m_bWaitForTrackStart(0)
{
    if (pUnknown)
    {
	m_pUnknown = pUnknown;
	m_pUnknown->AddRef();

	if (HXR_OK != m_pUnknown->QueryInterface(IID_IHXRegistry, (void**)&m_pRegistry))
	{
	    m_pRegistry = NULL;
	}

	if (HXR_OK != m_pUnknown->QueryInterface(IID_IHXScheduler, (void**)&m_pScheduler))
	{
	    m_pScheduler = NULL;
	}

	IHXPlayer* pPlayer;
	if(HXR_OK == m_pUnknown->QueryInterface(IID_IHXPlayer,
						(void**)&pPlayer))
	{
		m_pPlayer = pPlayer;

            IHXGroupManager* pGroupMgr = NULL;
            if(HXR_OK == m_pPlayer->QueryInterface(IID_IHXGroupManager,
                                                   (void**)&pGroupMgr))
            {
                pGroupMgr->AddSink((IHXGroupSink*)this);
                pGroupMgr->Release();
            }

	    pPlayer->AddAdviseSink(this);

	}
    }

#ifdef __TCS__
    bEnableAdviceSink = TRUE;
    iOpened = 0;
#endif 
}

ExampleClientAdviceSink::~ExampleClientAdviceSink(void)
{
    if (m_pScheduler)
    {
        m_pScheduler->Release();
        m_pScheduler = NULL;
    }

    if (m_pRegistry)
    {
	m_pRegistry->Release();
	m_pRegistry = NULL;
    }

    if (m_pPlayer)
    {
	m_pPlayer->Release();
	m_pPlayer = NULL;
    }
    if (m_pUnknown)
    {
	m_pUnknown->Release();
	m_pUnknown = NULL;
    }
}


// *** IUnknown methods ***

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::QueryInterface
//  Purpose:
//	Implement this to export the interfaces supported by your 
//	object.
//
STDMETHODIMP ExampleClientAdviceSink::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
	AddRef();
	*ppvObj = (IUnknown*)(IHXClientAdviseSink*)this;
	return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXClientAdviseSink))
    {
	AddRef();
	*ppvObj = (IHXClientAdviseSink*)this;
	return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXGroupSink))
    {
	AddRef();
	*ppvObj = (IHXGroupSink*)this;
	return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::AddRef
//  Purpose:
//	Everyone usually implements this the same... feel free to use
//	this implementation.
//
STDMETHODIMP_(ULONG32) ExampleClientAdviceSink::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::Release
//  Purpose:
//	Everyone usually implements this the same... feel free to use
//	this implementation.
//
STDMETHODIMP_(ULONG32) ExampleClientAdviceSink::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

/*
 *	IHXClientAdviseSink methods
 */

/************************************************************************
 *	Method:
 *	    IHXClientAdviseSink::OnPosLength
 *	Purpose:
 *	    Called to advise the client that the position or length of the
 *	    current playback context has changed.
 */

#ifdef __TCS__
static long	lastPosition = -1;
#endif

STDMETHODIMP
ExampleClientAdviceSink::OnPosLength(UINT32	  ulPosition,
				   UINT32	  ulLength)
{
#ifdef __TCS__
    gStreamTime = ulPosition;
    if (iOpened == 1)
    {
	if (lastPosition < 0)
	{
	    hookRealAudio_PlayPosition(ulPosition, ulLength);
	    lastPosition = 0;
	}
	else
	{
	    if ((ulPosition/1000) != lastPosition)
	    {
		hookRealAudio_PlayPosition(ulPosition, ulLength);
		lastPosition = ulPosition/1000;
	    }
	}
    }
#else
    if (GetGlobal()->bEnableAdviceSink)
    {
        STDOUT("OnPosLength(%lu,%lu)\n", ulPosition, ulLength);
    }
#endif /* __TCS__ */

    // Are we doing a multi-seek?
    if (GetGlobal()->g_bMultiSeek)
    {
        // Have we run out of seek times?
        if (GetGlobal()->g_ulMultiSeekIndex < GetGlobal()->g_ulNumMultiSeeks)
        {
            STDOUT("Multi-seeking the first player to %lu\n",
                   GetGlobal()->g_ulMultiSeekTime[GetGlobal()->g_ulMultiSeekIndex]);
            GetGlobal()->g_Players[0]->Seek(GetGlobal()->g_ulMultiSeekTime[GetGlobal()->g_ulMultiSeekIndex++]);
        }
        else
        {
            // We've run out - stop the multi-seek
            GetGlobal()->g_bMultiSeek       = FALSE;
            GetGlobal()->g_ulNumMultiSeeks  = 0;
            GetGlobal()->g_ulMultiSeekIndex = 0;
        }
    }

    return HXR_OK;
}

/************************************************************************
 *	Method:
 *	    IHXClientAdviseSink::OnPresentationOpened
 *	Purpose:
 *	    Called to advise the client a presentation has been opened.
 */
STDMETHODIMP ExampleClientAdviceSink::OnPresentationOpened()
{
#ifdef __TCS__
    iOpened = 1;
    lastPosition = -1;
#else
    if (GetGlobal()->bEnableAdviceSink)
    {
        STDOUT("OnPresentationOpened()\n");
    }
#if defined(HELIX_FEATURE_PLAYBACK_VELOCITY)
    // Try to get the IHXPlaybackVelocity interface
    // from the first player
    if (GetGlobal()->g_nPlayers > 0 && GetGlobal()->g_Players[0])
    {
        IHXPlaybackVelocity* pVel = NULL;
        HX_RESULT rv = GetGlobal()->g_Players[0]->QueryInterface(IID_IHXPlaybackVelocity, (void**) &pVel);
        if (SUCCEEDED(rv))
        {
            // Get the capabilities object from the player
            rv = pVel->QueryVelocityCaps(GetGlobal()->g_pVelocityCaps);
            if (SUCCEEDED(rv) && GetGlobal()->bEnableAdviceSink)
            {
                STDOUT("Available Velocity Ranges:");
                UINT32 ulNumRanges = GetGlobal()->g_pVelocityCaps->GetNumRanges();
                for (UINT32 i = 0; i < ulNumRanges && SUCCEEDED(rv); i++)
                {
                    INT32 lMin = 0;
                    INT32 lMax = 0;
                    rv = GetGlobal()->g_pVelocityCaps->GetRange(i, lMin, lMax);
                    if (SUCCEEDED(rv))
                    {
                        STDOUT(" [%ld,%ld]", lMin, lMax);
                    }
                }
                STDOUT("\n");
            }
        }
        HX_RELEASE(pVel);
    }
#endif /* #if defined(HELIX_FEATURE_PLAYBACK_VELOCITY) */
#endif

    UINT32 sourceCount = m_pPlayer->GetSourceCount();	
    for (UINT32 sourceID = 0; sourceID < sourceCount; sourceID ++)
    {
	// get HXSource
        IUnknown* pUnkSource = NULL;
        IHXStreamSource* pStreamSrc = NULL;

        if (HXR_OK != m_pPlayer->GetSource (sourceID, pUnkSource))
        {
            continue;
        }

        pUnkSource->QueryInterface(IID_IHXStreamSource, (void **)&pStreamSrc);

        HX_RELEASE(pUnkSource);

        if (!pStreamSrc)
        {
            continue;
        }

        UINT32 num_streams = pStreamSrc->GetStreamCount();

        // get information from Streams
        for (UINT32 streamID = 0; streamID < num_streams ; streamID++)
        {
            IUnknown* pUnkStream = NULL;

            if (HXR_OK == pStreamSrc->GetStream(streamID, pUnkStream))
            {
                IHXStream* pStream = NULL;

                if (HXR_OK == pUnkStream->QueryInterface(IID_IHXStream,
                                                         (void**)&pStream))
                {
                    const char* pMimeType = pStream->GetStreamType();
                    if( 0 == strcmp(pMimeType, "application/ram"))
                    {
		        m_bWaitForTrackStart = TRUE;
                    }
                    else
               	    {
                        m_bWaitForTrackStart = FALSE;
                    }
                }
                HX_RELEASE(pStream);
            }
            HX_RELEASE(pUnkStream);
            if (!m_bWaitForTrackStart)
            {
                break;
            }
        }

        if (!m_bWaitForTrackStart)
        {
            break;
        }
        HX_RELEASE(pStreamSrc);
    }

    if (!m_bWaitForTrackStart)
    {
	STDOUT("----------------clip info--------------------\n");
	if(m_pScheduler)	{
		m_hCallback = m_pScheduler->RelativeEnter(this,50);
	}
    }
    return HXR_OK;
}


/************************************************************************
 *	Method:
 *	    IHXClientAdviseSink::OnPresentationClosed
 *	Purpose:
 *	    Called to advise the client a presentation has been closed.
 */
STDMETHODIMP ExampleClientAdviceSink::OnPresentationClosed()
{
#ifdef __TCS__
    iOpened = 0;
    lastPosition = -1;
#else
    if (GetGlobal()->bEnableAdviceSink)
    {
        STDOUT("OnPresentationClosed()\n");
    }
#endif

    return HXR_OK;
}

void ExampleClientAdviceSink::GetStatistics (char* pszRegistryKey)
{
    char    szRegistryValue[MAX_DISPLAY_NAME] = {0}; /* Flawfinder: ignore */
    INT32   lValue = 0;
    INT32   i = 0;
    INT32   lStatistics = 8;
    UINT32 *plValue;
    
#ifdef __TCS__    
    return;	  // DISABLED FOR NOW
#endif

    // collect statistic
    for (i = 0; i < lStatistics; i++)
    {
	plValue = NULL;
	switch (i)
	{
	case 0:
	    SafeSprintf(szRegistryValue, MAX_DISPLAY_NAME, "%s.Normal", pszRegistryKey);
	    break;
	case 1:
	    SafeSprintf(szRegistryValue, MAX_DISPLAY_NAME, "%s.Recovered", pszRegistryKey);
	    break;
	case 2:
	    SafeSprintf(szRegistryValue, MAX_DISPLAY_NAME, "%s.Received", pszRegistryKey);
	    break;
	case 3:
	    SafeSprintf(szRegistryValue, MAX_DISPLAY_NAME, "%s.Lost", pszRegistryKey);
	    break;
	case 4:
	    SafeSprintf(szRegistryValue, MAX_DISPLAY_NAME, "%s.Late", pszRegistryKey);
	    break;
	case 5:
	    SafeSprintf(szRegistryValue, MAX_DISPLAY_NAME, "%s.ClipBandwidth", pszRegistryKey);
	    break;
	case 6:
	    SafeSprintf(szRegistryValue, MAX_DISPLAY_NAME, "%s.AverageBandwidth", pszRegistryKey);
	    plValue = &m_lAverageBandwidth;
	    break;
	case 7:
	    SafeSprintf(szRegistryValue, MAX_DISPLAY_NAME, "%s.CurrentBandwidth", pszRegistryKey);
	    plValue = &m_lCurrentBandwidth;
	    break;
	default:
	    break;
	}

	m_pRegistry->GetIntByName(szRegistryValue, lValue);
	if (plValue)
	{
	    if (m_bOnStop || lValue == 0)
	    {
		lValue = *plValue;
	    }
	    else
	    {
		*plValue = lValue;
	    }
	}
	if (GetGlobal()->bEnableAdviceSink || (GetGlobal()->bEnableVerboseMode && m_bOnStop))
	{
	    STDOUT("%s = %ld\n", szRegistryValue, lValue);
	}
    }
}

void ExampleClientAdviceSink::GetAllStatistics(void)
{
    UINT32  unPlayerIndex = 0;
    UINT32  unSourceIndex = 0;
    UINT32  unStreamIndex = 0;

    char*   pszRegistryPrefix = "Statistics";
    char    szRegistryName[MAX_DISPLAY_NAME] = {0}; /* Flawfinder: ignore */

#ifdef __TCS__    
    return;	  // DISABLED FOR NOW
#endif
    
    // display the content of whole statistic registry
    if (m_pRegistry)
    {
	// ok, let's start from the top (player)
	SafeSprintf(szRegistryName, MAX_DISPLAY_NAME, "%s.Player%ld", pszRegistryPrefix, m_lClientIndex);
	if (PT_COMPOSITE == m_pRegistry->GetTypeByName(szRegistryName))
	{
	    // display player statistic
	    GetStatistics(szRegistryName);

	    SafeSprintf(szRegistryName, MAX_DISPLAY_NAME, "%s.Source%ld", szRegistryName, unSourceIndex);
	    while (PT_COMPOSITE == m_pRegistry->GetTypeByName(szRegistryName))
	    {
		// display source statistic
		GetStatistics(szRegistryName);

		SafeSprintf(szRegistryName, MAX_DISPLAY_NAME, "%s.Stream%ld", szRegistryName, unStreamIndex);
		while (PT_COMPOSITE == m_pRegistry->GetTypeByName(szRegistryName))
		{
		    // display stream statistic
		    GetStatistics(szRegistryName);

		    unStreamIndex++;

		    SafeSprintf(szRegistryName, MAX_DISPLAY_NAME, "%s.Player%ld.Source%ld.Stream%ld", 
			pszRegistryPrefix, unPlayerIndex, unSourceIndex, unStreamIndex);
		}

		unSourceIndex++;

		SafeSprintf(szRegistryName, MAX_DISPLAY_NAME, "%s.Player%ld.Source%ld",
		    pszRegistryPrefix, unPlayerIndex, unSourceIndex);
	    }

	    unPlayerIndex++;

	    SafeSprintf(szRegistryName, MAX_DISPLAY_NAME, "%s.Player%ld", pszRegistryPrefix, unPlayerIndex);
	}
    }
}

/************************************************************************
 *	Method:
 *	    IHXClientAdviseSink::OnStatisticsChanged
 *	Purpose:
 *	    Called to advise the client that the presentation statistics
 *	    have changed. 
 */
STDMETHODIMP ExampleClientAdviceSink::OnStatisticsChanged(void)
{
    char        szBuff[1024]; /* Flawfinder: ignore */
    HX_RESULT   res     = HXR_OK;
    UINT16      uPlayer = 0;

#ifdef __TCS__    
    return HXR_OK;	  // DISABLED FOR NOW
#endif

    if(GetGlobal()->bEnableAdviceSink)
    {
        STDOUT("OnStatisticsChanged():\n");

        SafeSprintf(szBuff, 1024, "Statistics");        
        res = DumpRegTree( szBuff );
    }

    return HXR_OK;
}

HX_RESULT ExampleClientAdviceSink::DumpRegTree(const char* pszTreeName )
{
    const char* pszName = NULL;
    ULONG32     ulRegID   = 0;
    HX_RESULT   res     = HXR_OK;
    INT32       nVal    = 0;
    IHXBuffer* pBuff   = NULL;
    IHXValues* pValues = NULL;

    //See if the name exists in the reg tree.
    res = m_pRegistry->GetPropListByName( pszTreeName, pValues);
    if( HXR_OK!=res || !pValues )
        return HXR_FAIL;

    //make sure this is a PT_COMPOSITE type reg entry.
    if( PT_COMPOSITE != m_pRegistry->GetTypeByName(pszTreeName))
        return HXR_FAIL;

    //Print out the value of each member of this tree.
    res = pValues->GetFirstPropertyULONG32( pszName, ulRegID );
    while( HXR_OK == res )
    {
        //We have at least one entry. See what type it is.
        HXPropType pt = m_pRegistry->GetTypeById(ulRegID);
        switch(pt)
        {
           case PT_COMPOSITE:
               DumpRegTree(pszName);
               break;
           case PT_INTEGER :
               nVal = 0;
               m_pRegistry->GetIntById( ulRegID, nVal );
               STDOUT("%s : %d\n", pszName, nVal ); 
               break;
           case PT_INTREF :
               nVal = 0;
               m_pRegistry->GetIntById( ulRegID, nVal );
               STDOUT("%s : %d\n", pszName, nVal ); 
               break;
           case PT_STRING :
               pBuff = NULL;
               m_pRegistry->GetStrById( ulRegID, pBuff );
               STDOUT("%s : \"", pszName ); 
               if( pBuff )
                   STDOUT("%s", (const char *)(pBuff->GetBuffer()) );
               STDOUT("\"\n" ); 
               HX_RELEASE(pBuff);
               break;
           case PT_BUFFER :
               STDOUT("%s : BUFFER TYPE NOT SHOWN\n",
                        pszName, nVal ); 
               break;
           case PT_UNKNOWN:
               STDOUT("%s Unkown registry type entry\n", pszName );
               break;
           default:
               STDOUT("%s Unkown registry type entry\n", pszName );
               break;
        }
        res = pValues->GetNextPropertyULONG32( pszName, ulRegID);
    }

    HX_RELEASE( pValues );
    
    return HXR_OK;
}


/************************************************************************
 *	Method:
 *	    IHXClientAdviseSink::OnPreSeek
 *	Purpose:
 *	    Called by client engine to inform the client that a seek is
 *	    about to occur. The render is informed the last time for the 
 *	    stream's time line before the seek, as well as the first new
 *	    time for the stream's time line after the seek will be completed.
 *
 */
STDMETHODIMP ExampleClientAdviceSink::OnPreSeek(	ULONG32	ulOldTime,
						ULONG32	ulNewTime)
{
#if !defined(__TCS__)
    if (GetGlobal()->bEnableAdviceSink)
    {
        STDOUT("OnPreSeek(%ld, %ld)\n", ulOldTime, ulNewTime);
    }
#endif

    return HXR_OK;
}


/************************************************************************
 *	Method:
 *	    IHXClientAdviseSink::OnPostSeek
 *	Purpose:
 *	    Called by client engine to inform the client that a seek has
 *	    just occured. The render is informed the last time for the 
 *	    stream's time line before the seek, as well as the first new
 *	    time for the stream's time line after the seek.
 *
 */
STDMETHODIMP ExampleClientAdviceSink::OnPostSeek(	ULONG32	ulOldTime,
						ULONG32	ulNewTime)
{
#ifdef __TCS__
    lastPosition = -1;
#else
    if (GetGlobal()->bEnableAdviceSink)
    {
        STDOUT("OnPostSeek(%ld, %ld)\n", ulOldTime, ulNewTime);
    }
#endif

    return HXR_OK;
}


/************************************************************************
 *	Method:
 *	    IHXClientAdviseSink::OnStop
 *	Purpose:
 *	    Called by client engine to inform the client that a stop has
 *	    just occured. 
 *
 */
STDMETHODIMP ExampleClientAdviceSink::OnStop(void)
{
    HXTimeval now;

#ifdef __TCS__
    hookRealAudio_State(eStop);
#else
    if (GetGlobal()->bEnableAdviceSink)
    {
        STDOUT("OnStop()\n");
    }

    if (GetGlobal()->bEnableVerboseMode)
    {
        STDOUT("Player %ld stopped.\n", m_lClientIndex);
        m_bOnStop = TRUE;
	GetAllStatistics();
    }
#endif

    // Find out the current time and subtract the beginning time to
    // figure out how many seconds we played
    now = m_pScheduler->GetCurrentSchedulerTime();
    m_ulStopTime = now.tv_sec;

    GetGlobal()->g_ulNumSecondsPlayed = m_ulStopTime - m_ulStartTime;

    return HXR_OK;
}

/************************************************************************
 *	Method:
 *	    IHXClientAdviseSink::OnPause
 *	Purpose:
 *	    Called by client engine to inform the client that a pause has
 *	    just occured. The render is informed the last time for the 
 *	    stream's time line before the pause.
 *
 */
STDMETHODIMP ExampleClientAdviceSink::OnPause(ULONG32 ulTime)
{
#ifdef __TCS__
    hookRealAudio_State(ePause);
#else
    if (GetGlobal()->bEnableAdviceSink)
    {
        STDOUT("OnPause(%ld)\n", ulTime);
    }
#endif 

    return HXR_OK;
}


/************************************************************************
 *	Method:
 *	    IHXClientAdviseSink::OnBegin
 *	Purpose:
 *	    Called by client engine to inform the client that a begin or
 *	    resume has just occured. The render is informed the first time 
 *	    for the stream's time line after the resume.
 *
 */
STDMETHODIMP ExampleClientAdviceSink::OnBegin(ULONG32 ulTime)
{
    HXTimeval now;

#if !defined(__TCS__)
    if (GetGlobal()->bEnableAdviceSink)
    {
        STDOUT("OnBegin(%ld)\n", ulTime);
    }

    if (GetGlobal()->bEnableVerboseMode)
    {
        STDOUT("Player %ld beginning playback...\n", m_lClientIndex);
    }
#endif

    GetGlobal()->g_bOnBeginOccurred = TRUE;

    // Record the current time, so we can figure out many seconds we played
    now = m_pScheduler->GetCurrentSchedulerTime();
    m_ulStartTime = now.tv_sec;

    return HXR_OK;
}


/************************************************************************
 *	Method:
 *	    IHXClientAdviseSink::OnBuffering
 *	Purpose:
 *	    Called by client engine to inform the client that buffering
 *	    of data is occuring. The render is informed of the reason for
 *	    the buffering (start-up of stream, seek has occured, network
 *	    congestion, etc.), as well as percentage complete of the 
 *	    buffering process.
 *
 */
STDMETHODIMP ExampleClientAdviceSink::OnBuffering(ULONG32	ulFlags,
						UINT16	unPercentComplete)
{
#ifdef __TCS__
static UINT16	lastPct = 0;
    if (unPercentComplete > 0)
    {
	UINT16	nextPct = lastPct + 10;
	if (unPercentComplete < lastPct)
	{
	    lastPct = 0;
	    nextPct = lastPct + 10;
	}
	if (unPercentComplete >= nextPct)
	{
	    lastPct = (unPercentComplete / 10) * 10;
	    nextPct = lastPct + 10;
	    hookRealAudio_Buffering(eBuffering,lastPct);
	}
    }
#else
    if (GetGlobal()->bEnableAdviceSink)
    {
        STDOUT("OnBuffering(%ld, %d)\n", ulFlags, unPercentComplete);
    }
#endif

    return HXR_OK;
}


/************************************************************************
 *	Method:
 *	    IHXClientAdviseSink::OnContacting
 *	Purpose:
 *	    Called by client engine to inform the client is contacting
 *	    hosts(s).
 *
 */
STDMETHODIMP ExampleClientAdviceSink::OnContacting(const char* pHostName)
{
#ifdef __TCS__
    printf("Contacting\n");
    hookRealAudio_Buffering(eContacting,0);
#else
    if (GetGlobal()->bEnableAdviceSink)
    {
        STDOUT("OnContacting(\"%s\")\n", pHostName);
    }
#endif

    return HXR_OK;
}

void ExampleClientAdviceSink::PrintPropName( IHXValues* pHeader )
{
    if ( pHeader == NULL )
    {
        return ;
    }

    const char *pChar;
    ULONG32 pValue;
    if(HXR_OK == pHeader->GetFirstPropertyULONG32(pChar, pValue))
    {
        do
        {
            STDOUT("%s %d\n", pChar, pValue);
        }
        while(HXR_OK == pHeader->GetNextPropertyULONG32(pChar, pValue));
    }

    IHXBuffer *pBuffer;

    if(HXR_OK == pHeader->GetFirstPropertyBuffer(pChar, pBuffer))
    {
        do
        {
            STDOUT("%s %s\n", pChar, (const char *) ( pBuffer->GetBuffer() ));
		PrintBuffer(pChar, pBuffer->GetBuffer() ,pBuffer->GetSize());
            HX_RELEASE(pBuffer);
        } while(HXR_OK == pHeader->GetNextPropertyBuffer(pChar, pBuffer ));
    }
    
	if(HXR_OK == pHeader->GetFirstPropertyCString(pChar, pBuffer))
    {
        do
        {
            STDERR("%s %s\n", pChar, (const char *) ( pBuffer->GetBuffer() ));
            HX_RELEASE(pBuffer);
        } while(HXR_OK == pHeader->GetNextPropertyCString(pChar, pBuffer ));
    }
    
    fflush(stdout);
}


void ExampleClientAdviceSink::SetClipInfo( IHXPlayer* m_pRMAPlayer)
{
    bool bSendOnClipInfo = false;

    // Get HXSource and try to get clip infor. 
    UINT32 sourceCount = m_pRMAPlayer->GetSourceCount();	
    for (UINT32 sourceID = 0; sourceID < sourceCount; sourceID ++)
    {
	// get HXSource
        STDOUT("========Source %d========\n",sourceID);
        IUnknown* pUnkSource = NULL;
        IHXStreamSource* pStreamSrc = NULL;

        if (HXR_OK != m_pRMAPlayer->GetSource (sourceID, pUnkSource))
        {
            continue;
        }

        pUnkSource->QueryInterface(IID_IHXStreamSource, (void **)&pStreamSrc);

        HX_RELEASE(pUnkSource);

        if (!pStreamSrc)
        {
            continue;
        }

	UINT32 num_streams = pStreamSrc->GetStreamCount();
	
	STDOUT("====File Header====\n");

        IHXValues* pFileHdr = NULL;
        pStreamSrc->QueryInterface(IID_IHXValues, (void**)&pFileHdr);

        if ( pFileHdr)
        {
	    PrintPropName(pFileHdr);
        }
        HX_RELEASE(pFileHdr);

        // get information from Streams
        for (UINT32 streamID = 0; streamID < num_streams ; streamID++)
        {
            STDOUT("====Stream %d====\n",streamID);
            IUnknown* pUnkStream = NULL;

            if (HXR_OK == pStreamSrc->GetStream(streamID, pUnkStream))
            {
                IHXStream* pStream = NULL;

                if (HXR_OK == pUnkStream->QueryInterface(IID_IHXStream,
                                                         (void**)&pStream))
                {
                    IHXValues* pStreamHdr = pStream->GetHeader();
                    if (pStreamHdr)
                    {
                        PrintPropName(pStreamHdr);
                    }
                    HX_RELEASE(pStreamHdr);
                }
                HX_RELEASE(pStream);
            }
            HX_RELEASE(pUnkStream);
        }
        STDOUT("---------------------------------------------\n");

        HX_RELEASE(pStreamSrc);
    }
}


STDMETHODIMP ExampleClientAdviceSink::Func()
{
    m_hCallback = 0;

    // set clip info
	if(m_pPlayer)
	    SetClipInfo(m_pPlayer);

    return HXR_OK;
}

void PrintBuffer(const char* pszName,const unsigned char* pbBuf, unsigned int dwBytes)
{
	if(pszName == NULL || pbBuf == NULL)
		return;

	STDOUT("    ");
	for(unsigned int i= 0; i<dwBytes; i++)	{
		STDOUT("0x%02x  ",*pbBuf++);
		if(i % 15 == 0 && i!=0)
			STDOUT("\n    ");
	}
	STDOUT("\n");
	return;
}

/*
 *  IHXGroupSink methods
 */
/************************************************************************
 *  Method:
 *      IHXGroupSink::GroupAdded
 *  Purpose:
 *		Notification of a new group being added to the presentation.
 */
STDMETHODIMP ExampleClientAdviceSink::GroupAdded( UINT16 /*IN*/ uGroupIndex,
			    IHXGroup* /*IN*/ pGroup)
{
    return HXR_OK;
}

/************************************************************************
 *  Method:
 *      IHXGroupSink::GroupRemoved
 *  Purpose:
 *		Notification of a group being removed from the presentation.
 */
STDMETHODIMP ExampleClientAdviceSink::GroupRemoved( UINT16 /*IN*/ uGroupIndex,
				IHXGroup*  /*IN*/ pGroup)
{
    return HXR_OK;
}

/************************************************************************
 *  Method:
 *      IHXGroupSink::AllGroupsRemoved
 *  Purpose:
 *		Notification that all groups have been removed from the 
 *		current presentation.
 */
STDMETHODIMP ExampleClientAdviceSink::AllGroupsRemoved()
{
    return HXR_OK;
}

/************************************************************************
 *  Method:
 *      IHXGroupSink::TrackAdded
 *  Purpose:
 *		Notification of a new track being added to a group.
 */
STDMETHODIMP ExampleClientAdviceSink::TrackAdded( UINT16 /*IN*/ uGroupIndex,
			    UINT16     /*IN*/ uTrackIndex,
			    IHXValues* /*IN*/ pTrack)
{
    return HXR_OK;
}

/************************************************************************
 *  Method:
 *      IHXGroupSink::TrackRemoved
 *  Purpose:
 *		Notification of a track being removed from a group.
 */
STDMETHODIMP ExampleClientAdviceSink::TrackRemoved( UINT16 /*IN*/ uGroupIndex,
				UINT16     /*IN*/ uTrackIndex,
				IHXValues* /*IN*/ pTrack)
{
    return HXR_OK;
}

/************************************************************************
 *  Method:
 *      IHXGroupSink::TrackStarted
 *  Purpose:
 *		Notification of a track being started (to get duration, for
 *		instance...)
 */
STDMETHODIMP ExampleClientAdviceSink::TrackStarted( UINT16 /*IN*/ uGroupIndex,
				UINT16     /*IN*/ uTrackIndex,
				IHXValues* /*IN*/ pTrack)
{
    if ( m_bWaitForTrackStart )
    {
        STDOUT("----------------clip info--------------------\n");
        if(m_pScheduler)
        {
            m_hCallback = m_pScheduler->RelativeEnter(this,50);
        }
	m_bWaitForTrackStart = FALSE;
    }
    return HXR_OK;
}

/************************************************************************
 *  Method:
 *      IHXGroupSink::TrackStopped
 *  Purpose:
 *		Notification of a track being stopped
 *
 */
STDMETHODIMP ExampleClientAdviceSink::TrackStopped( UINT16 /*IN*/ uGroupIndex,
				UINT16     /*IN*/ uTrackIndex,
				IHXValues* /*IN*/ pTrack)
{
    return HXR_OK;
}

/************************************************************************
 *  Method:
 *      IHXGroupSink::CurrentGroupSet
 *  Purpose:
 *		This group is being currently played in the presentation.
 */
STDMETHODIMP ExampleClientAdviceSink::CurrentGroupSet( UINT16 /*IN*/ uGroupIndex,
				IHXGroup* /*IN*/ pGroup)
{
    return HXR_OK;
}

