/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: rendstats.h,v 1.8 2007/07/06 22:00:24 jfinnecy Exp $
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

#ifndef __RENDSTATS_H__
#define __RENDSTATS_H__

/****************************************************************************
 *  Includes
 */
#include "hxtypes.h"
#include "hxresult.h"
#include "hxcom.h"
#include "hxcomm.h"
#include "hxbuffer.h"
#include "hxmon.h"
#include "statinfo.h"

/****************************************************************************
 *  CRendererStatisticsDisplay
 */
class CRendererStatisticsDisplay
{
public:
    typedef class CFormatEntry
    {
    public:
	/*
	 *  Costructor/Destructor
	 */
	CFormatEntry(void)
	: m_pEntry(NULL)
	, m_pName(NULL)
	, m_ulType(REG_TYPE_UNKNOWN)
	, m_bIsDirty(FALSE)
	, m_lVal(0)
	, m_pVal(NULL)
	{
	    ;
	}

	~CFormatEntry()
	{
	    HX_DELETE(m_pEntry);
	    HX_VECTOR_DELETE(m_pName);
	    HX_VECTOR_DELETE(m_pVal);
	}

	/*
	 *  Main Interface
	 */
	HX_RESULT Prime(IUnknown* pContext, 
			char* pName, 
			UINT32 ulType);

	void Hide(void)
	{
	    HX_DELETE(m_pEntry);
	    m_bIsDirty = FALSE;
	}

	void Kill(void)
	{
	    HX_DELETE(m_pEntry);
	    HX_VECTOR_DELETE(m_pName);
	    HX_VECTOR_DELETE(m_pVal);
	    m_lVal = 0;
	    m_ulType = REG_TYPE_UNKNOWN;
	    m_bIsDirty = FALSE;
	}

	HX_RESULT Update(INT32 lVal)
	{
	    HX_RESULT retVal = HXR_UNEXPECTED;

	    if (m_ulType == REG_TYPE_NUMBER)
	    {
		m_lVal = lVal;
		m_bIsDirty = TRUE;
	    }

	    return retVal;
	}

	HX_RESULT Update(const char* pVal)
	{
	    HX_RESULT retVal = HXR_UNEXPECTED;

	    if (m_ulType == REG_TYPE_STRING)
	    {
		retVal = HXR_INVALID_PARAMETER;

		HX_VECTOR_DELETE(m_pVal);

		if (pVal)
		{
		    m_pVal = new char [strlen(pVal) + 1];

		    retVal = HXR_OUTOFMEMORY;
		    if (m_pVal)
		    {
			strcpy(m_pVal, pVal); /* Flawfinder: ignore */
			m_bIsDirty = TRUE;
                        // Clear the return value
                        retVal = HXR_OK;
		    }
		}
	    }

	    return retVal;
	}

	HX_RESULT Refresh(IUnknown* pContext)
	{
	    HX_RESULT retVal = HXR_OK;

	    if (m_bIsDirty)
	    {
		if (!m_pEntry)
		{
		    retVal = Prime(pContext, 
				   m_pName,
				   m_ulType);
		}

		if (SUCCEEDED(retVal))
		{
		    HX_ASSERT(m_pEntry);

		    switch (m_ulType)
		    {
		    case REG_TYPE_NUMBER:
			retVal = m_pEntry->SetInt(m_lVal);
			break;
		    case REG_TYPE_STRING:
			if (m_pVal)
			{
			    retVal = m_pEntry->SetStr(m_pVal);
			}
			break;
		    default:
			retVal = HXR_UNEXPECTED;
			break;
		    }
		}  

		if (SUCCEEDED(retVal))
		{
		    m_bIsDirty = FALSE;
		}
	    }

	    return retVal;
	}

	const char* GetStatName(void);

	void MarkAsDirty(void)	{ m_bIsDirty = TRUE; }

	HXBOOL IsDisplayed(void) { return (m_pEntry != NULL); }
	HXBOOL IsPrimed(void) { return (m_pName != NULL); }
	HXBOOL IsDirty(void)  { return m_bIsDirty; }
	
	CStatisticEntry* m_pEntry;
	char*		 m_pName;
	UINT32		 m_ulType;
	HXBOOL		 m_bIsDirty;

	INT32		 m_lVal;
	char*		 m_pVal;
    };

    /*
     *	Costructor/Destructor
     */
    CRendererStatisticsDisplay(IUnknown* pContext, UINT32 ulNumEntries);
    ~CRendererStatisticsDisplay();

    /*
     *	Main Interface
     */
    HX_RESULT MoveToRegID(ULONG32 ulRegID);
    ULONG32 GetRegID(void)  { return m_ulRegistryID; }

    HX_RESULT RefreshEntries(UINT32 ulRegistryID);

    HX_RESULT HideEntry(UINT32 ulEntryID);
    HX_RESULT DestroyEntry(UINT32 ulEntryID);

    HX_RESULT ReprimeEntries(void);
    HX_RESULT PrimeEntry(UINT32 ulEntryID,
			 const char* pzName, 
			 UINT32 ulType);
    HX_RESULT UpdateEntry(UINT32 ulEntryID, INT32 lVal);
    HX_RESULT UpdateEntry(UINT32 ulEntryID, const char* pVal);

    HX_RESULT MarkEntryAsDirty(UINT32 ulEntryID);
    HXBOOL IsEntryDirty(UINT32 ulEntryID);

private:
    CFormatEntry* m_pFormatEntryArray;
    UINT32 m_ulFormatEntryArraySize;

    IUnknown*		m_pContext;
    IHXRegistry*	m_pRegistry;
    UINT32		m_ulRegistryID;
};

#endif	// __RENDSTATS_H__
