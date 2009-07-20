/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: statinfo.cpp,v 1.17 2006/02/07 19:21:09 ping Exp $
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

#include "safestring.h"
#include "hxtypes.h"
#include "hxresult.h"
#include "hxcom.h"
#include "hxcomm.h"
#include "hxbuffer.h"
#include "hxmon.h"
#include "hxstrutl.h"
#include "watchlst.h"
#include "statinfo.h"
#include "pckunpck.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif

CStatisticEntry::CStatisticEntry(IUnknown* 	 pContext,
				 char*		 pszRegKey,
				 UINT32		 ulType)
    : m_pContext(pContext)
    , m_pRegistry(NULL)
    , m_ulRegistryID(0)
    , m_ulType(REG_TYPE_UNKNOWN)
    , m_bAddKey(FALSE)
{
    HX_ADDREF(m_pContext);

    if (HXR_OK == m_pContext->QueryInterface(IID_IHXRegistry, (void**)&m_pRegistry))
    {
	m_ulType = ulType;

	/*
	 * Check to see if this Registry ID already exists
         */

	m_ulRegistryID = m_pRegistry->GetId(pszRegKey);

	if (!m_ulRegistryID)
	{
	    m_bAddKey = TRUE;

	    if (REG_TYPE_STRING == ulType)
	    {
		m_ulRegistryID = m_pRegistry->AddStr(pszRegKey, NULL);
	    }
	    else if (REG_TYPE_NUMBER == ulType)
	    {
		m_ulRegistryID = m_pRegistry->AddInt(pszRegKey, 0);
	    }
	    else if (REG_TYPE_COMPOSITE == ulType)
	    {
		m_ulRegistryID = m_pRegistry->AddComp(pszRegKey);
	    }	    
	    else
	    {
		m_ulType = REG_TYPE_UNKNOWN;
		m_ulRegistryID = 0;
	    }
	}
    }
    else
    {
	m_pRegistry = NULL;
	m_ulRegistryID = 0;
    }
}

CStatisticEntry::~CStatisticEntry(void)
{
    if (m_pRegistry)
    {
	if (m_ulRegistryID && m_bAddKey)
	{
	    m_pRegistry->DeleteById(m_ulRegistryID);
	    m_ulRegistryID = 0;
	}
	HX_RELEASE(m_pRegistry);
    }
    HX_RELEASE(m_pContext);
}

HX_RESULT
CStatisticEntry::SetInt(INT32 lValue)
{    
    HX_RESULT	theErr = HXR_OK;

    if (!m_pRegistry || !m_ulRegistryID ||
	(REG_TYPE_NUMBER != m_ulType))
    {
	theErr = HXR_FAILED;
	goto cleanup;
    }

    theErr = m_pRegistry->SetIntById(m_ulRegistryID, lValue);

cleanup:
    
    return theErr;
}

HX_RESULT
CStatisticEntry::SetStr(char* pszValue)
{    
    IHXBuffer*	pValue = NULL;
    HX_RESULT	theErr = HXR_OK;

    if (!m_pRegistry || !m_ulRegistryID ||
	(REG_TYPE_STRING != m_ulType))
    {
	theErr = HXR_FAILED;
	goto cleanup;
    }

    if (NULL == pszValue)
    {
	theErr = m_pRegistry->SetStrById(m_ulRegistryID, NULL);
    }
    else
    {
	theErr = CreateAndSetBufferCCF(pValue, (UCHAR*)pszValue, strlen(pszValue)+1, m_pContext);
	if (HXR_OK == theErr)
	{
	    theErr = m_pRegistry->SetStrById(m_ulRegistryID, pValue);
	    HX_RELEASE(pValue);
	}
    }

cleanup:
    
    return theErr;
}

INT32
CStatisticEntry::GetInt(void)
{
    INT32	lValue = 0;

    if (!m_pRegistry || !m_ulRegistryID || (REG_TYPE_NUMBER != m_ulType))
    {
	goto cleanup;
    }

    m_pRegistry->GetIntById(m_ulRegistryID, lValue);
   
cleanup:

    return lValue;
}

char*
CStatisticEntry::GetStr(void)
{
    HX_RESULT	theErr = HXR_OK;
    IHXBuffer*	pValue = NULL;
    char*       pszValue = new char[MAX_DISPLAY_NAME];

    if (!pszValue || !m_pRegistry || !m_ulRegistryID || (REG_TYPE_STRING != m_ulType))
    {
	theErr = HXR_UNEXPECTED;
	goto cleanup;
    }

    if (HXR_OK != m_pRegistry->GetStrById(m_ulRegistryID, pValue) || !pValue)
    {
	theErr = HXR_UNEXPECTED;
	goto cleanup;
    }

    SafeStrCpy(pszValue, (const char*)pValue->GetBuffer(), MAX_DISPLAY_NAME);
    
cleanup:

    HX_RELEASE(pValue);

    if (HXR_OK != theErr)
    {
	if (pszValue) delete [] pszValue;
	pszValue = NULL;

	return NULL;
    }

    return pszValue;
}

CStatisticEntry&
CStatisticEntry::operator=(CStatisticEntry& rhs)
{
    if( this != &rhs )
    {
        if (REG_TYPE_NUMBER == rhs.m_ulType)    
        {
            SetInt(rhs.GetInt());
        }
        else if (REG_TYPE_STRING == rhs.m_ulType)
        {
            char* pszValue = NULL;
            pszValue = rhs.GetStr();
            if (pszValue)
            {
                SetStr(pszValue);
            }
            HX_VECTOR_DELETE(pszValue);
        }
        else
        {
            // type unknown
            HX_ASSERT(FALSE);
        }
    }

    return *this;
}

STATS::STATS(IUnknown*	     /*IN*/  pContext,
	     UINT32	     /*IN*/  ulRegistryID)
    : m_lastError(HXR_OK)
    , m_pContext(pContext)
    , m_pRegistry(NULL)
    , m_ulRegistryID(0)
    , m_bInitialized(FALSE)
    , m_pNormal(0)
    , m_pRecovered(0)
    , m_pReceived(0)
    , m_pOutOfOrder(0)
    , m_pFirstTimestamp(0)
    , m_pLastTimestamp(0)
    , m_pFilledBufferSize(0)
    , m_pLost(0)
    , m_pLate(0)
    , m_pDuplicate(0)
    , m_pTotal(0)
    , m_pLost30(0)
    , m_pTotal30(0)
    , m_pResendRequested(0)
    , m_pResendReceived(0)
    , m_pClipBandwidth(0)
    , m_pAvgBandwidth(0)
    , m_pCurBandwidth(0)
    , m_pHighLatency(0)
    , m_pLowLatency(0)
    , m_pAvgLatency(0)
{
    HX_RESULT	theErr = HXR_OK;
    IHXBuffer*	pParentName = NULL;
    char	szRegKeyName[MAX_DISPLAY_NAME] = {0}; /* Flawfinder: ignore */

    HX_ADDREF(m_pContext);

    if (HXR_OK == m_pContext->QueryInterface(IID_IHXRegistry, (void**)&m_pRegistry))
    { 
	m_ulRegistryID = ulRegistryID;

	if (HXR_OK == m_pRegistry->GetPropName(m_ulRegistryID, pParentName))
	{
	    SafeSprintf(szRegKeyName, MAX_DISPLAY_NAME, "%s.Normal", pParentName->GetBuffer());
	    if (!(m_pNormal = new CStatisticEntry(m_pContext, szRegKeyName, REG_TYPE_NUMBER)))
	    {
		theErr = HXR_OUTOFMEMORY;
		goto cleanup;
	    }

	    SafeSprintf(szRegKeyName, MAX_DISPLAY_NAME, "%s.Recovered", pParentName->GetBuffer());
	    if (!(m_pRecovered = new CStatisticEntry(m_pContext, szRegKeyName, REG_TYPE_NUMBER)))
	    {
		theErr = HXR_OUTOFMEMORY;
		goto cleanup;
	    }

	    SafeSprintf(szRegKeyName, MAX_DISPLAY_NAME, "%s.Received", pParentName->GetBuffer());
	    if (!(m_pReceived = new CStatisticEntry(m_pContext, szRegKeyName, REG_TYPE_NUMBER)))
	    {
		theErr = HXR_OUTOFMEMORY;
		goto cleanup;
	    }
    	
	    SafeSprintf(szRegKeyName, MAX_DISPLAY_NAME, "%s.OutOfOrder", pParentName->GetBuffer());
	    if (!(m_pOutOfOrder = new CStatisticEntry(m_pContext, szRegKeyName, REG_TYPE_NUMBER)))
	    {
		theErr = HXR_OUTOFMEMORY;
		goto cleanup;
	    }

	    SafeSprintf(szRegKeyName, MAX_DISPLAY_NAME, "%s.FirstTimestamp", pParentName->GetBuffer());
	    if (!(m_pFirstTimestamp = new CStatisticEntry(m_pContext, szRegKeyName, REG_TYPE_NUMBER)))
	    {
		theErr = HXR_OUTOFMEMORY;
		goto cleanup;
	    }

	    SafeSprintf(szRegKeyName, MAX_DISPLAY_NAME, "%s.LastTimestamp", pParentName->GetBuffer());
	    if (!(m_pLastTimestamp = new CStatisticEntry(m_pContext, szRegKeyName, REG_TYPE_NUMBER)))
	    {
		theErr = HXR_OUTOFMEMORY;
		goto cleanup;
	    }

	    SafeSprintf(szRegKeyName, MAX_DISPLAY_NAME, "%s.FilledBufferSize", pParentName->GetBuffer());
	    if (!(m_pFilledBufferSize = new CStatisticEntry(m_pContext, szRegKeyName, REG_TYPE_NUMBER)))
	    {
		theErr = HXR_OUTOFMEMORY;
		goto cleanup;
	    }

	    SafeSprintf(szRegKeyName, MAX_DISPLAY_NAME, "%s.Lost", pParentName->GetBuffer());
	    if (!(m_pLost = new CStatisticEntry(m_pContext, szRegKeyName, REG_TYPE_NUMBER)))
	    {
		theErr = HXR_OUTOFMEMORY;
		goto cleanup;
	    }

	    SafeSprintf(szRegKeyName, MAX_DISPLAY_NAME, "%s.Late", pParentName->GetBuffer());
	    if (!(m_pLate = new CStatisticEntry(m_pContext, szRegKeyName, REG_TYPE_NUMBER)))
	    {
		theErr = HXR_OUTOFMEMORY;
		goto cleanup;
	    }

	    SafeSprintf(szRegKeyName, MAX_DISPLAY_NAME, "%s.Duplicate", pParentName->GetBuffer());
	    if (!(m_pDuplicate = new CStatisticEntry(m_pContext, szRegKeyName, REG_TYPE_NUMBER)))
	    {
		theErr = HXR_OUTOFMEMORY;
		goto cleanup;
	    }

	    SafeSprintf(szRegKeyName, MAX_DISPLAY_NAME, "%s.Total", pParentName->GetBuffer());
	    if (!(m_pTotal = new CStatisticEntry(m_pContext, szRegKeyName, REG_TYPE_NUMBER)))
	    {
		theErr = HXR_OUTOFMEMORY;
		goto cleanup;
	    }

	    SafeSprintf(szRegKeyName, MAX_DISPLAY_NAME, "%s.Lost30", pParentName->GetBuffer());
	    if (!(m_pLost30 = new CStatisticEntry(m_pContext, szRegKeyName, REG_TYPE_NUMBER)))
	    {
		theErr = HXR_OUTOFMEMORY;
		goto cleanup;
	    }

	    SafeSprintf(szRegKeyName, MAX_DISPLAY_NAME, "%s.Total30", pParentName->GetBuffer());
	    if (!(m_pTotal30 = new CStatisticEntry(m_pContext, szRegKeyName, REG_TYPE_NUMBER)))
	    {
		theErr = HXR_OUTOFMEMORY;
		goto cleanup;
	    }

	    SafeSprintf(szRegKeyName, MAX_DISPLAY_NAME, "%s.ResendRequested", pParentName->GetBuffer());
	    if (!(m_pResendRequested = new CStatisticEntry(m_pContext, szRegKeyName, REG_TYPE_NUMBER)))
	    {
		theErr = HXR_OUTOFMEMORY;
		goto cleanup;
	    }

	    SafeSprintf(szRegKeyName, MAX_DISPLAY_NAME, "%s.ResendReceived", pParentName->GetBuffer());
	    if (!(m_pResendReceived = new CStatisticEntry(m_pContext, szRegKeyName, REG_TYPE_NUMBER)))
	    {
		theErr = HXR_OUTOFMEMORY;
		goto cleanup;
	    }

	    SafeSprintf(szRegKeyName, MAX_DISPLAY_NAME, "%s.ClipBandwidth", pParentName->GetBuffer());
	    if (!(m_pClipBandwidth = new CStatisticEntry(m_pContext, szRegKeyName, REG_TYPE_NUMBER)))
	    {
		theErr = HXR_OUTOFMEMORY;
		goto cleanup;
	    }

	    SafeSprintf(szRegKeyName, MAX_DISPLAY_NAME, "%s.AverageBandwidth", pParentName->GetBuffer());
	    if (!(m_pAvgBandwidth = new CStatisticEntry(m_pContext, szRegKeyName, REG_TYPE_NUMBER)))
	    {
		theErr = HXR_OUTOFMEMORY;
		goto cleanup;
	    }

	    SafeSprintf(szRegKeyName, MAX_DISPLAY_NAME, "%s.CurrentBandwidth", pParentName->GetBuffer());
	    if (!(m_pCurBandwidth = new CStatisticEntry(m_pContext, szRegKeyName, REG_TYPE_NUMBER)))
	    {
		theErr = HXR_OUTOFMEMORY;
		goto cleanup;
	    }

	    SafeSprintf(szRegKeyName, MAX_DISPLAY_NAME, "%s.HighLatency", pParentName->GetBuffer());
	    if (!(m_pHighLatency = new CStatisticEntry(m_pContext, szRegKeyName, REG_TYPE_NUMBER)))
	    {
		theErr = HXR_OUTOFMEMORY;
		goto cleanup;
	    }

	    SafeSprintf(szRegKeyName, MAX_DISPLAY_NAME, "%s.LowLatency", pParentName->GetBuffer());
	    if (!(m_pLowLatency = new CStatisticEntry(m_pContext, szRegKeyName, REG_TYPE_NUMBER)))
	    {
		theErr = HXR_OUTOFMEMORY;
		goto cleanup;
	    }

	    SafeSprintf(szRegKeyName, MAX_DISPLAY_NAME, "%s.AverageLatency", pParentName->GetBuffer());
	    if (!(m_pAvgLatency = new CStatisticEntry(m_pContext, szRegKeyName, REG_TYPE_NUMBER)))
	    {
		theErr = HXR_OUTOFMEMORY;
		goto cleanup;
	    }
	}
    }
  
cleanup:

    HX_RELEASE(pParentName);

    if (HXR_OK == theErr)
    {
	m_lastError = HXR_OK;
	m_bInitialized = TRUE;
    }
    else
    {
	m_lastError = theErr;
	m_bInitialized = FALSE;
    }
}

STATS::~STATS()
{
    HX_RELEASE(m_pRegistry);
    HX_RELEASE(m_pContext);

    HX_DELETE(m_pNormal);
    HX_DELETE(m_pRecovered);
    HX_DELETE(m_pReceived);
    HX_DELETE(m_pOutOfOrder);
    HX_DELETE(m_pFirstTimestamp);
    HX_DELETE(m_pLastTimestamp);
    HX_DELETE(m_pFilledBufferSize);
    HX_DELETE(m_pLost);
    HX_DELETE(m_pLate);
    HX_DELETE(m_pDuplicate);
    HX_DELETE(m_pTotal);
    HX_DELETE(m_pLost30);
    HX_DELETE(m_pTotal30);
    HX_DELETE(m_pClipBandwidth);
    HX_DELETE(m_pResendRequested);
    HX_DELETE(m_pResendReceived);
    HX_DELETE(m_pAvgBandwidth);
    HX_DELETE(m_pCurBandwidth);
    HX_DELETE(m_pHighLatency);
    HX_DELETE(m_pLowLatency);
    HX_DELETE(m_pAvgLatency);
}

STATS& STATS::operator=(const STATS& rhs )
{
    if ((this != &rhs)  &&
        this->m_bInitialized &&
        rhs.m_bInitialized)
    {
        *m_pNormal = *(rhs.m_pNormal);
        *m_pRecovered = *(rhs.m_pRecovered);
        *m_pReceived = *(rhs.m_pReceived);
        *m_pOutOfOrder = *(rhs.m_pOutOfOrder);
        *m_pFirstTimestamp = *(rhs.m_pFirstTimestamp);
        *m_pLastTimestamp = *(rhs.m_pLastTimestamp);
        *m_pFilledBufferSize = *(rhs.m_pFilledBufferSize);
        *m_pLost = *(rhs.m_pLost);
        *m_pLate = *(rhs.m_pLate);
        *m_pDuplicate = *(rhs.m_pDuplicate);
        *m_pTotal = *(rhs.m_pTotal);
        *m_pLost30 = *(rhs.m_pLost30);
        *m_pTotal30 = *(rhs.m_pTotal30);
        *m_pClipBandwidth = *(rhs.m_pClipBandwidth);
        *m_pResendRequested = *(rhs.m_pResendRequested);
        *m_pResendReceived = *(rhs.m_pResendReceived);
        *m_pAvgBandwidth = *(rhs.m_pAvgBandwidth);
        *m_pCurBandwidth = *(rhs.m_pCurBandwidth);
        *m_pHighLatency = *(rhs.m_pHighLatency);
        *m_pLowLatency = *(rhs.m_pLowLatency);
        *m_pAvgLatency = *(rhs.m_pAvgLatency);
    }
    else
    {
        HX_ASSERT(FALSE);
    }
    
    return *this;
}

void
STATS::Reset()
{
    if (m_bInitialized)
    {
	m_pNormal->SetInt(0);
	m_pRecovered->SetInt(0);
	m_pReceived->SetInt(0);
	m_pOutOfOrder->SetInt(0);
        m_pFirstTimestamp->SetInt(0);
        m_pLastTimestamp->SetInt(0);
        m_pFilledBufferSize->SetInt(0);
	m_pLost->SetInt(0);
	m_pLate->SetInt(0);
	m_pDuplicate->SetInt(0);
	m_pTotal->SetInt(0);
	m_pLost30->SetInt(0);
	m_pTotal30->SetInt(0);
	m_pClipBandwidth->SetInt(0);
	m_pResendRequested->SetInt(0);
	m_pResendReceived->SetInt(0);
	m_pAvgBandwidth->SetInt(0);
	m_pCurBandwidth->SetInt(0);
	m_pHighLatency->SetInt(0);
	m_pLowLatency->SetInt(0);
	m_pAvgLatency->SetInt(0);
    }
}

PLAYER_STATS::PLAYER_STATS(IUnknown*	     /*IN*/  pContext, 
			   UINT32	     /*IN*/  ulRegistryID) 
	     :STATS(pContext, ulRegistryID)
	     ,m_pBufferingMode(NULL)
{
    HX_RESULT	theErr = HXR_OK;
    IHXBuffer*	pParentName = NULL;
    char	szRegKeyName[MAX_DISPLAY_NAME] = {0}; /* Flawfinder: ignore */

    if (HXR_OK == m_pRegistry->GetPropName(m_ulRegistryID, pParentName))
    {
	SafeSprintf(szRegKeyName, MAX_DISPLAY_NAME, "%s.BufferingMode", pParentName->GetBuffer());
	if (!(m_pBufferingMode = new CStatisticEntry(m_pContext, szRegKeyName, REG_TYPE_NUMBER)))
	{
	    theErr = HXR_OUTOFMEMORY;
	    goto cleanup;
	}
    }

cleanup:

    HX_RELEASE(pParentName);

    if (HXR_OK == theErr)
    {
	m_lastError = HXR_OK;
	m_bInitialized = TRUE;
    }
    else
    {
	m_lastError = theErr;
	m_bInitialized = FALSE;
    }
}

PLAYER_STATS::~PLAYER_STATS()
{
    HX_DELETE(m_pBufferingMode);
}

PLAYER_STATS& PLAYER_STATS::operator=(const PLAYER_STATS& rhs )
{
    if ((this != &rhs)  &&
        this->m_bInitialized &&
        rhs.m_bInitialized)
    {
        STATS::operator=(rhs);
        *m_pBufferingMode = *(rhs.m_pBufferingMode);
    }
    else
    {
        HX_ASSERT(FALSE);
    }
    
    return *this;
}

void
PLAYER_STATS::Reset()
{
    if (m_bInitialized)
    {
	m_pBufferingMode->SetInt(0);

	STATS::Reset();
    }
}

SOURCE_STATS::SOURCE_STATS(IUnknown*         /*IN*/  pContext,
			   UINT32	     /*IN*/  ulRegistryID)
	     :STATS(pContext, ulRegistryID)
{
    HX_RESULT	theErr = HXR_OK;
    IHXBuffer*	pParentName = NULL;
    char	szRegKeyName[MAX_DISPLAY_NAME] = {0}; /* Flawfinder: ignore */

    m_pTransportMode = NULL;
    m_pBufferingMode = NULL;
    m_pTitle = NULL;
    m_pAuthor = NULL;
    m_pCopyright = NULL;
    m_pAbstract = NULL;
    m_pDescription = NULL;
    m_pKeywords = NULL;
    m_pSourceName = NULL;
    m_pServerInfo = NULL;
    m_pProtocolVersion = NULL;
    m_pProtocol = NULL;

    if (HXR_OK == m_pRegistry->GetPropName(m_ulRegistryID, pParentName))
    {
	SafeSprintf(szRegKeyName, MAX_DISPLAY_NAME, "%s.TransportMode", pParentName->GetBuffer());
	if (!(m_pTransportMode = new CStatisticEntry(m_pContext, szRegKeyName, REG_TYPE_STRING)))
	{
	    theErr = HXR_OUTOFMEMORY;
	    goto cleanup;
	}

	SafeSprintf(szRegKeyName, MAX_DISPLAY_NAME, "%s.BufferingMode", pParentName->GetBuffer());
	if (!(m_pBufferingMode = new CStatisticEntry(m_pContext, szRegKeyName, REG_TYPE_NUMBER)))
	{
	    theErr = HXR_OUTOFMEMORY;
	    goto cleanup;
	}

	SafeSprintf(szRegKeyName, MAX_DISPLAY_NAME, "%s.SourceName", pParentName->GetBuffer());
	if (!(m_pSourceName = new CStatisticEntry(m_pContext, szRegKeyName, REG_TYPE_STRING)))
	{
	    theErr = HXR_OUTOFMEMORY;
	    goto cleanup;
	}

	SafeSprintf(szRegKeyName, MAX_DISPLAY_NAME, "%s.ServerInfo", pParentName->GetBuffer());
	if (!(m_pServerInfo = new CStatisticEntry(m_pContext, szRegKeyName, REG_TYPE_STRING)))
	{
	    theErr = HXR_OUTOFMEMORY;
	    goto cleanup;
	}

	SafeSprintf(szRegKeyName, MAX_DISPLAY_NAME, "%s.ProtocolVersion", pParentName->GetBuffer());
	if (!(m_pProtocolVersion = new CStatisticEntry(m_pContext, szRegKeyName, REG_TYPE_NUMBER)))
	{
	    theErr = HXR_OUTOFMEMORY;
	    goto cleanup;
	}

	SafeSprintf(szRegKeyName, MAX_DISPLAY_NAME, "%s.Protocol", pParentName->GetBuffer());
	if (!(m_pProtocol = new CStatisticEntry(m_pContext, szRegKeyName, REG_TYPE_STRING)))
	{
	    theErr = HXR_OUTOFMEMORY;
	    goto cleanup;
	}
	
	SafeSprintf(szRegKeyName, MAX_DISPLAY_NAME, "%s.Title", pParentName->GetBuffer());
	if (!(m_pTitle = new CStatisticEntry(m_pContext, szRegKeyName, REG_TYPE_STRING)))
	{
	    theErr = HXR_OUTOFMEMORY;
	    goto cleanup;
	}

	SafeSprintf(szRegKeyName, MAX_DISPLAY_NAME, "%s.Author", pParentName->GetBuffer());
	if (!(m_pAuthor = new CStatisticEntry(m_pContext, szRegKeyName, REG_TYPE_STRING)))
	{
	    theErr = HXR_OUTOFMEMORY;
	    goto cleanup;
	}

	SafeSprintf(szRegKeyName, MAX_DISPLAY_NAME, "%s.Copyright", pParentName->GetBuffer());
	if (!(m_pCopyright = new CStatisticEntry(m_pContext, szRegKeyName, REG_TYPE_STRING)))
	{
	    theErr = HXR_OUTOFMEMORY;
	    goto cleanup;
	}

	SafeSprintf(szRegKeyName, MAX_DISPLAY_NAME, "%s.Abstract", pParentName->GetBuffer());
	if (!(m_pAbstract = new CStatisticEntry(m_pContext, szRegKeyName, REG_TYPE_STRING)))
	{
	    theErr = HXR_OUTOFMEMORY;
	    goto cleanup;
	}
    
	SafeSprintf(szRegKeyName, MAX_DISPLAY_NAME, "%s.Description", pParentName->GetBuffer());
	if (!(m_pDescription = new CStatisticEntry(m_pContext, szRegKeyName, REG_TYPE_STRING)))
	{
	    theErr = HXR_OUTOFMEMORY;
	    goto cleanup;
	}

	SafeSprintf(szRegKeyName, MAX_DISPLAY_NAME, "%s.Keywords", pParentName->GetBuffer());
	if (!(m_pKeywords = new CStatisticEntry(m_pContext, szRegKeyName, REG_TYPE_STRING)))
	{
	    theErr = HXR_OUTOFMEMORY;
	    goto cleanup;
	}
    }

cleanup:

    HX_RELEASE(pParentName);

    if (HXR_OK == theErr)
    {
	m_lastError = HXR_OK;
	m_bInitialized = TRUE;
    }
    else
    {
	m_lastError = theErr;
	m_bInitialized = FALSE;
    }
}

SOURCE_STATS::~SOURCE_STATS()
{
    HX_DELETE(m_pTransportMode);
    HX_DELETE(m_pBufferingMode);
    HX_DELETE(m_pSourceName);
    HX_DELETE(m_pServerInfo);
    HX_DELETE(m_pProtocolVersion);
    HX_DELETE(m_pProtocol);
    HX_DELETE(m_pTitle);
    HX_DELETE(m_pAuthor);
    HX_DELETE(m_pCopyright);
    HX_DELETE(m_pAbstract);
    HX_DELETE(m_pDescription);
    HX_DELETE(m_pKeywords);
}

SOURCE_STATS& SOURCE_STATS::operator=(const SOURCE_STATS& rhs )
{
    if ((this != &rhs)  &&
        this->m_bInitialized &&
        rhs.m_bInitialized)
    {
        STATS::operator=(rhs);

        *m_pTransportMode = *(rhs.m_pTransportMode);
        *m_pBufferingMode = *(rhs.m_pBufferingMode);
        *m_pSourceName = *(rhs.m_pSourceName);
        *m_pServerInfo = *(rhs.m_pServerInfo);
        *m_pProtocolVersion = *(rhs.m_pProtocolVersion);
        *m_pProtocol = *(rhs.m_pProtocol);
        *m_pTitle = *(rhs.m_pTitle);
        *m_pAuthor = *(rhs.m_pAuthor);
        *m_pCopyright = *(rhs.m_pCopyright);
        *m_pAbstract = *(rhs.m_pAbstract);
        *m_pDescription = *(rhs.m_pDescription);
        *m_pKeywords = *(rhs.m_pKeywords);
    }
    else
    {
        HX_ASSERT(FALSE);
    }
    
    return *this;
}

void
SOURCE_STATS::Reset()
{
    if (m_bInitialized)
    {
	m_pTransportMode->SetStr(NULL);
	m_pBufferingMode->SetInt(0);
	m_pSourceName->SetStr(NULL);
	m_pServerInfo->SetStr(NULL);
	m_pProtocolVersion->SetInt(0);
	m_pProtocol->SetStr(NULL);
	m_pTitle->SetStr(NULL);
	m_pAuthor->SetStr(NULL);
	m_pCopyright->SetStr(NULL);
	m_pAbstract->SetStr(NULL);
	m_pDescription->SetStr(NULL);
	m_pKeywords->SetStr(NULL);

	STATS::Reset();
    }
}

STREAM_STATS::STREAM_STATS(IUnknown*         /*IN*/  pContext,
			   UINT32	     /*IN*/  ulRegistryID)
	     :STATS(pContext, ulRegistryID)
{
    HX_RESULT	theErr = HXR_OK;
    IHXBuffer*	pParentName = NULL;
    char	szRegKeyName[MAX_DISPLAY_NAME] = {0}; /* Flawfinder: ignore */
    
    m_pRenderer = NULL;
    m_pMimeType = NULL;
    
    if (HXR_OK == m_pRegistry->GetPropName(m_ulRegistryID, pParentName))
    {
	SafeSprintf(szRegKeyName, MAX_DISPLAY_NAME, "%s.Renderer", pParentName->GetBuffer());
	if (!(m_pRenderer = new CStatisticEntry(m_pContext, szRegKeyName, REG_TYPE_COMPOSITE)))
	{
	    theErr = HXR_OUTOFMEMORY;
	    goto cleanup;
	}

	SafeSprintf(szRegKeyName, MAX_DISPLAY_NAME, "%s.MimeType", pParentName->GetBuffer());
	if (!(m_pMimeType = new CStatisticEntry(m_pContext, szRegKeyName, REG_TYPE_STRING)))
	{
	    theErr = HXR_OUTOFMEMORY;
	    goto cleanup;
	}
	
    }
        
cleanup:

    HX_RELEASE(pParentName);

    if (HXR_OK == theErr)
    {
	m_lastError = HXR_OK;
	m_bInitialized = TRUE;
    }
    else
    {
	m_lastError = theErr;
	m_bInitialized = FALSE;
    }
}

STREAM_STATS::~STREAM_STATS()
{
    HX_DELETE(m_pRenderer);
    HX_DELETE(m_pMimeType);
}

STREAM_STATS& STREAM_STATS::operator=(const STREAM_STATS& rhs )
{
    if ((this != &rhs)  &&
        this->m_bInitialized &&
        rhs.m_bInitialized)
    {
	STATS::operator=(rhs);
        *m_pMimeType = *(rhs.m_pMimeType);
    }
    else
    {
        HX_ASSERT(FALSE);
    }
    
    return *this;
}

void
STREAM_STATS::Reset()
{
    if (m_bInitialized)
    {
	m_pMimeType->SetStr(NULL);
	STATS::Reset();
    }
}

HXBOOL SetIntIfNecessary(CStatisticEntry* pEntry, INT32 lValue)
{
    HXBOOL bResult = FALSE;

    if (!pEntry)
    {
	goto cleanup;
    }

    if (pEntry->GetInt() != lValue)
    {
	pEntry->SetInt(lValue);
	bResult = TRUE;
    }

cleanup:

    return bResult;
}
