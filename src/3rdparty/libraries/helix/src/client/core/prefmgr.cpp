/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: prefmgr.cpp,v 1.8 2007/07/06 21:58:11 jfinnecy Exp $
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

#include "hxcom.h"
#include "hxresult.h"
#include "hxstrutl.h"
#include "hxgroup.h"
#include "hxcore.h"
#include "hxengin.h"
#include "hxslist.h"
#include "hxstring.h"
#include "hxplay.h"
#include "hxbsrc.h"
#include "hxsrc.h"
#include "prefmgr.h"

#include "srcinfo.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE
static const char HX_THIS_FILE[] = __FILE__;
#endif

PrefetchManager::PrefetchManager(HXPlayer* pPlayer)
    : m_bSourceMapUpdated(FALSE)
    , m_uSourceCount(0)
    , m_pPlayer(NULL)
    , m_pInterruptState(NULL)
    , m_pSourceMap(NULL)
{
    m_pPlayer = pPlayer;
    if (m_pPlayer)
    {
	m_pPlayer->AddRef();
	m_pPlayer->QueryInterface(IID_IHXInterruptState, (void**) &m_pInterruptState);
    }

    m_pSourceMap = new CHXMapLongToObj;
}

PrefetchManager::~PrefetchManager()
{
    HX_ASSERT(m_pSourceMap->GetCount() == 0);

    HX_RELEASE(m_pPlayer);
    HX_RELEASE(m_pInterruptState);
    
    HX_DELETE(m_pSourceMap);
}

HX_RESULT	
PrefetchManager::AddSource(SourceInfo* pSourceInfo)
{
    HX_RESULT hr = HXR_OK;

    if (pSourceInfo)
    {
	HX_ASSERT(pSourceInfo->m_bPrefetch);
	(*m_pSourceMap)[m_uSourceCount] = pSourceInfo;
	m_uSourceCount = m_pSourceMap->GetCount();
	m_bSourceMapUpdated = TRUE;
    }

    return hr;
}

UINT16	
PrefetchManager::GetNumSources(void)
{
    return m_uSourceCount;
}

HX_RESULT	
PrefetchManager::GetSource(UINT16 uSourceIndex, SourceInfo*& pSourceInfo)
{
    HX_RESULT	hr = HXR_OK;
    
    pSourceInfo = NULL;

    if (!m_pSourceMap->Lookup(uSourceIndex, (void*&)pSourceInfo))
    {
	hr = HXR_UNEXPECTED;
	goto cleanup;
    }
    
cleanup:
    
    return hr;
}

HXBOOL	
PrefetchManager::Lookup(IHXValues* pValues, SourceInfo*& pSourceInfo)
{
    HX_RESULT	hr = HXR_OK;
    HXBOOL	bFound = FALSE;
    char	szUrl[] = "url";
    char	szSrc[] = "src";
    char	szStart[] = "Start";
    char	szDelay[] = "Delay";
    const char* pszURL = NULL;
    UINT32	ulStart = 0;
    UINT32	ulDelay = 0;
    SourceInfo* pSrcInfo = NULL;
    HXSource*  pSource = NULL;
    IHXBuffer*	pBuffer = NULL;
    CHXURL*	pURL = NULL;
    CHXMapLongToObj::Iterator i;

    pSourceInfo = NULL;

    hr = pValues->GetPropertyCString(szUrl,pBuffer);

    /* temp - for now support both "src" & "url" */
    if (hr)
    {
	hr = pValues->GetPropertyCString(szSrc,pBuffer);
    }

    if (hr)
    {	
	goto cleanup;
    }

    pszURL = (const char*)pBuffer->GetBuffer();
    if (!pszURL || !*pszURL)
    {
	goto cleanup;
    }

    pURL = new CHXURL(pszURL, (IHXPlayer*)m_pPlayer); //parse the url
    pszURL = pURL->GetURL();

    pValues->GetPropertyULONG32(szStart, ulStart);
    pValues->GetPropertyULONG32(szDelay, ulDelay);

    i = m_pSourceMap->Begin();    
    for (; i != m_pSourceMap->End(); ++i)
    {
	pSrcInfo = (SourceInfo*)(*i);
	pSource = pSrcInfo->m_pSource;

	if (pSource					&&
	    strcasecmp(pSource->GetURL(), pszURL) == 0	&&
	    pSource->GetStartTime() == ulStart		&&
	    // XXX HP let's give 100ms windows to take care
	    // of the current play time gap between SMIL renderer
	    // and the core
	    pSource->GetDelay() <= (ulDelay + 100))
	{	   
	    pSourceInfo = pSrcInfo;
	    bFound = TRUE;
	    break;
	}
    }

cleanup:

    HX_DELETE(pURL);
    HX_RELEASE(pBuffer);

    return bFound;
}

HXBOOL	
PrefetchManager::Lookup(HXSource* pSource, SourceInfo*& pSourceInfo)
{
    HXBOOL bResult = FALSE;

    pSourceInfo = NULL;

    CHXMapLongToObj::Iterator i = m_pSourceMap->Begin();
    for (; i != m_pSourceMap->End(); ++i)
    {
	SourceInfo* pSrcInfo = (SourceInfo*) (*i);
	if (pSrcInfo->m_pSource == pSource)
	{
	    pSourceInfo = pSrcInfo;
	    bResult = TRUE;
	    break;
	}
    }

    return bResult;
}

HX_RESULT	
PrefetchManager::RemoveSource(SourceInfo* pSourceInfo)
{    
    HX_RESULT		hr = HXR_OK;
    int			j = 0;
    HXBOOL		bRemoved = FALSE;
    CHXMapLongToObj*	pNewSourceMap = NULL;
    SourceInfo*		pSrcInfo = NULL;
    
    pNewSourceMap = new CHXMapLongToObj;

    CHXMapLongToObj::Iterator i = m_pSourceMap->Begin();    
    for (; i != m_pSourceMap->End(); ++i,j++)
    {
	pSrcInfo = (SourceInfo*)(*i);
	
	if (pSrcInfo == pSourceInfo)
	{
	    bRemoved = TRUE;
	    m_bSourceMapUpdated = TRUE;
	}
	else if (bRemoved)
	{
	    (*pNewSourceMap)[j - 1] = pSrcInfo;
	    // track ID has to be consistent with the map ID
	    pSrcInfo->m_uTrackID = j - 1;
	}
	else
	{
	    (*pNewSourceMap)[j] = pSrcInfo;
	}
    }
    
    HX_DELETE(m_pSourceMap);
    m_pSourceMap = pNewSourceMap;

    m_uSourceCount = m_pSourceMap->GetCount();

    return hr;
}

HX_RESULT	
PrefetchManager::ProcessIdle(void)
{
    HX_RESULT	hr = HXR_OK;	

    CHXMapLongToObj::Iterator i = m_pSourceMap->Begin();    
    for (; i != m_pSourceMap->End(); ++i)
    {
	SourceInfo* pSourceInfo = (SourceInfo*)(*i);
	HXSource*  pSource = pSourceInfo->m_pSource;

	if (!pSource || pSource->IsPrefetchDone())
	{
	    continue;
	}

	if (pSourceInfo->m_bToBeResumed		    && 
	    pSource->IsInitialized()		    &&
	    !m_pInterruptState->AtInterruptTime() )
	{
	    if (!pSourceInfo->m_bAreStreamsSetup)
	    {
		hr = pSourceInfo->SetupStreams();
		//m_pPlayer->m_ulPresentationDuration = HX_MAX(m_pPlayer->m_ulPresentationDuration,
		//					  pSourceInfo->GetActiveDuration());
	    }

	    if (pSource->CanBeResumed())
	    {   
		pSourceInfo->m_bToBeResumed = FALSE;

		pSourceInfo->Register();
		m_pPlayer->RegisterSourcesDone();
		hr = pSource->DoResume();
	    }
	}
	else
	{
	    hr = pSource->ProcessIdle();
	}

	if (m_bSourceMapUpdated)
	{
	    m_bSourceMapUpdated = FALSE;
	    break;
	}
    }

    return hr;
}

void
PrefetchManager::Cleanup(void)
{
    SourceInfo* pSourceInfo = NULL;

    CHXMapLongToObj::Iterator i = m_pSourceMap->Begin();    
    for (; i != m_pSourceMap->End(); ++i)
    {
	pSourceInfo = (SourceInfo*) (*i);

	pSourceInfo->Remove();
	HX_DELETE(pSourceInfo);
    }
    m_pSourceMap->RemoveAll();

    return;
}
