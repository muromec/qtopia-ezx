/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: rendstats.cpp,v 1.6 2006/02/07 20:56:19 ping Exp $
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

/****************************************************************************
 *  Defines
 */
#define MAX_DISPLAY_NAME_LENGTH	    1024

/****************************************************************************
 *  Includes
 */
#include <stdio.h>
#include "rendstats.h"
#include "hxstrutl.h"


/****************************************************************************
 *  CRendererStatisticsDisplay
 */
/****************************************************************************
 *  Constructor/Destructor
 */
CRendererStatisticsDisplay::CRendererStatisticsDisplay(IUnknown* pContext,
						       UINT32 ulNumEntries)
    : m_pContext(pContext)
    , m_pRegistry(NULL)
    , m_ulRegistryID(0)
    , m_pFormatEntryArray(NULL)
    , m_ulFormatEntryArraySize(0)
{
    HX_ADDREF(m_pContext);

    if (m_pContext)
    {
	m_pContext->QueryInterface(IID_IHXRegistry, (void**)&m_pRegistry);
    }

    m_pFormatEntryArray = new CFormatEntry[ulNumEntries];
    if (m_pFormatEntryArray)
    {
	m_ulFormatEntryArraySize = ulNumEntries;
    }
}

CRendererStatisticsDisplay::~CRendererStatisticsDisplay()
{
    HX_RELEASE(m_pRegistry);
    HX_RELEASE(m_pContext);
    HX_VECTOR_DELETE(m_pFormatEntryArray);
}


/****************************************************************************
 *  MoveToRegID
 */
HX_RESULT CRendererStatisticsDisplay::MoveToRegID(ULONG32 ulRegID)
{
    m_ulRegistryID = ulRegID;

    return ReprimeEntries();
}


HX_RESULT CRendererStatisticsDisplay::ReprimeEntries(void)
{
    UINT32 ulIdx;
    HX_RESULT status;
    HX_RESULT retVal = HXR_OK;

    for (ulIdx = 0; ulIdx < m_ulFormatEntryArraySize; ulIdx++)
    {
	if (m_pFormatEntryArray[ulIdx].IsPrimed())
	{
	    status = PrimeEntry(ulIdx,
				m_pFormatEntryArray[ulIdx].GetStatName(),
				m_pFormatEntryArray[ulIdx].m_ulType);

	    if (SUCCEEDED(retVal))
	    {
		status = retVal;
	    }
	}
    }

    return retVal;
}


/****************************************************************************
 *  MoveToRegID
 */
HX_RESULT CRendererStatisticsDisplay::HideEntry(UINT32 ulEntryID)
{
    if (m_pFormatEntryArray)
    {
	m_pFormatEntryArray[ulEntryID].Hide();
    }

    return HXR_OK;
}


/****************************************************************************
 *  DestroyEntry
 */
HX_RESULT CRendererStatisticsDisplay::DestroyEntry(UINT32 ulEntryID)
{
    if (m_pFormatEntryArray)
    {
	m_pFormatEntryArray[ulEntryID].Kill();
    }

    return HXR_OK;
}


/****************************************************************************
 *  UpdateEntry
 */
HX_RESULT CRendererStatisticsDisplay::UpdateEntry(UINT32 ulEntryID, 
						  INT32 lVal)
{   
    if (m_pFormatEntryArray)
    {
	return m_pFormatEntryArray[ulEntryID].Update(lVal);
    }
    else
    {
	return HXR_FAIL;
    }
}


/****************************************************************************
 *  UpdateEntry
 */
HX_RESULT CRendererStatisticsDisplay::UpdateEntry(UINT32 ulEntryID, 
					          const char* pVal)
{
    if (m_pFormatEntryArray)
    {
	return m_pFormatEntryArray[ulEntryID].Update(pVal);
    }
    else
    {
	return HXR_FAIL;
    }
}


/****************************************************************************
 *  MarkEntryAsDirty
 */
HX_RESULT CRendererStatisticsDisplay::MarkEntryAsDirty(UINT32 ulEntryID)
{
    if (m_pFormatEntryArray)
    {
	m_pFormatEntryArray[ulEntryID].MarkAsDirty();
    }

    return HXR_OK;
}


/****************************************************************************
 *  IsEntryDirty
 */
HXBOOL CRendererStatisticsDisplay::IsEntryDirty(UINT32 ulEntryID)
{
    if (m_pFormatEntryArray)
    {
	return m_pFormatEntryArray[ulEntryID].IsDirty();
    }

    return FALSE;
}


/****************************************************************************
 *  RefreshEntries
 */
HX_RESULT CRendererStatisticsDisplay::RefreshEntries(ULONG32 ulRegID)
{
    ULONG32 ulIdx;
    HX_RESULT retVal = HXR_INVALID_PARAMETER;

    if (ulRegID != 0)
    {
	retVal = HXR_OK;

	if (ulRegID != m_ulRegistryID)
	{
	    retVal = MoveToRegID(ulRegID);
	}
	
	if (SUCCEEDED(retVal))
	{
	    if (!m_pFormatEntryArray)
	    {
		retVal = HXR_FAIL;
	    }
	}

	if (SUCCEEDED(retVal))
	{
	    for (ulIdx = 0; ulIdx < m_ulFormatEntryArraySize; ulIdx++)
	    {
		m_pFormatEntryArray[ulIdx].Refresh(m_pContext);
	    }
	}
    }

    return retVal;
}


/****************************************************************************
 *  PrimeEntry
 */
HX_RESULT CRendererStatisticsDisplay::PrimeEntry(UINT32 ulEntryID,
						 const char* pzName, 
						 UINT32 ulType)
{
    char sRegKeyName[MAX_DISPLAY_NAME_LENGTH]; /* Flawfinder: ignore */
    IHXBuffer*	pParentNameBuffer = NULL;
    HX_RESULT retVal = HXR_FAIL;

    if (m_pRegistry)
    {
	char* pRegKeyName = NULL;

	retVal = HXR_OK;

	if (m_ulRegistryID != 0)
	{
	    retVal = m_pRegistry->GetPropName(m_ulRegistryID, pParentNameBuffer);

	    if (SUCCEEDED(retVal))
	    {
		SafeSprintf(sRegKeyName, MAX_DISPLAY_NAME_LENGTH, "%s.%s", 
			pParentNameBuffer->GetBuffer(), 
			pzName);

		pRegKeyName = &(sRegKeyName[0]);
	    }
	}
	else
	{
	    pRegKeyName = (char*) pzName;
	}

	if (SUCCEEDED(retVal))
	{
	    if (!m_pFormatEntryArray)
	    {
		retVal = HXR_FAIL;
	    }
	}

	if (SUCCEEDED(retVal))
	{
	    retVal = m_pFormatEntryArray[ulEntryID].Prime(m_pContext,
							  pRegKeyName, 
							  ulType);
	}
    }

    HX_RELEASE(pParentNameBuffer);

    return retVal;
}


/****************************************************************************
 *  CRendererStatisticsDisplay::CFormatEntry
 */
/****************************************************************************
 *  GetStatName
 */
const char* CRendererStatisticsDisplay::CFormatEntry::GetStatName(void)
{
    const char* pStatName = NULL;

    if (m_pName)
    {
	pStatName = strrchr(m_pName, '.');
	if (!pStatName)
	{
	    pStatName = m_pName;
	}
    }

    return pStatName;
}

/****************************************************************************
 *  CFormatEntry::Prime
 */
HX_RESULT CRendererStatisticsDisplay::CFormatEntry::Prime(IUnknown* pContext, 
							  char* pName, 
							  UINT32 ulType)
{
    HX_RESULT retVal = HXR_OK;
    HXBOOL bIsActive = (m_pEntry != NULL);
    
    HX_DELETE(m_pEntry);
    if (pName != m_pName)
    {
	HX_VECTOR_DELETE(m_pName);

	if (pName)
	{
	    m_pName = new char [strlen(pName) + 1];
	    
	    retVal = HXR_OUTOFMEMORY;
	    if (m_pName)
	    {
		strcpy(m_pName, pName); /* Flawfinder: ignore */
		retVal = HXR_OK;
	    }
	}
    }

    m_ulType = ulType;

    if (SUCCEEDED(retVal) && 
	m_pName &&
	(bIsActive || m_bIsDirty))
    {
	m_pEntry = new CStatisticEntry(pContext, 
				       pName, 
				       ulType);
	
	if (!m_pEntry)
	{
	    retVal = HXR_OUTOFMEMORY;
	}
	m_bIsDirty = TRUE;
    }

    return retVal;
}
