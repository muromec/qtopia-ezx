/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: ppbin.cpp,v 1.2 2003/01/23 23:42:51 damonlan Exp $ 
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
**  This is a class to be used by ppffplin & pplyplin
*/

#include "hxresult.h"
#include "hxcom.h"

#include "carray.h"
#include "hxslist.h"
#include "ppbin.h"    

CTimeoutBin::CTimeoutBin()
{
#ifdef _DEBUG
    m_state = CREATED;
#endif    
}

CTimeoutBin::~CTimeoutBin()
{
    HX_ASSERT(m_bins.IsEmpty());
}

HX_RESULT
CTimeoutBin::Init(INT32 lNumBins)
{    
    HX_ASSERT(CREATED == m_state);

    // when the newest is updated, it will increment it
    m_lNewest = -1;
    // as long as this is not valid, it will stay -1.
    m_lOldest = -1;

    m_lNumBins = lNumBins;
    m_bins.SetSize(lNumBins);

    CHXSimpleList* pBin = NULL;

    for (int i = 0; i < m_lNumBins; i++)
    {
	pBin = new CHXSimpleList();
	if (pBin)
	{
	    m_bins.SetAt(i, pBin);
	}
	else
	{
	    // delete all and return HXR_OUTOFMEMORY
	    HX_ASSERT(m_bins.GetSize() == i);
	    for (int j = 0; j < i; j++)
	    {
		pBin = (CHXSimpleList*)m_bins.GetAt(j);
		HX_DELETE(pBin);
	    }
	    
	    return HXR_OUTOFMEMORY;
	}
    }

#ifdef _DEBUG
    m_state = INITED;
#endif    

    return HXR_OK;    
}


HX_RESULT
CTimeoutBin::PutInNewestBin(void* pEntry, REF(BinInfo) binInfo)
{
    HX_ASSERT(UPDATED == m_state);
    HX_ASSERT(m_lNewest < m_lNumBins);
    HX_ASSERT(m_lNewest >= 0);

    if (binInfo.lBin == m_lNewest)
    {
	// it's already in the list
	return HXR_OK;
    }
    else if (-1 != binInfo.lBin && NULL != binInfo.pos)
    {
	RemoveEntry(binInfo);
    }

    CHXSimpleList* pBin = (CHXSimpleList*)m_bins.GetAt(m_lNewest);
    if (pBin)
    {
	binInfo.lBin = m_lNewest;
	binInfo.pos = pBin->AddTail(pEntry);
	return HXR_OK;
    }
    
    return HXR_FAIL;
}

// don't delete this array!
HX_RESULT
CTimeoutBin::GetOldestBin(REF(CHXSimpleList*) pBin)
{
#ifdef _DEBUG
    HX_ASSERT(UPDATED == m_state);
    HX_ASSERT(m_lOldest < m_lNumBins);
    HX_ASSERT(-1 == m_lOldest || m_lOldest >= 0);
    m_state = REMOVED;
#endif    


    if (-1 == m_lOldest)
    {
	// even though this is oldest, it is not valid yet.
	pBin = NULL;
	return HXR_FAIL;
    }

    pBin = (CHXSimpleList*)m_bins.GetAt(m_lOldest);
    
    return pBin ? HXR_OK : HXR_FAIL;
}

void
CTimeoutBin::UpdateBins()
{
#ifdef _DEBUG
    HX_ASSERT(INITED == m_state || REMOVED == m_state);
    m_state = UPDATED;
#endif    

    m_lNewest = ++m_lNewest % m_lNumBins;

    if (-1 == m_lOldest && m_lNewest == m_lNumBins - 1)
    {
	// m_lNewest starts from 0, now it is on the last bin
	m_lOldest = 0;
    }
    else if (-1 != m_lOldest)
    {
#ifdef _DEBUG
	// all the entries in this bin should have been removed
	CHXSimpleList* pBin = (CHXSimpleList*)m_bins.GetAt(m_lOldest);
	HX_ASSERT(pBin->IsEmpty());
#endif
	m_lOldest = ++m_lOldest % m_lNumBins;
    }
#ifdef _DEBUG
    else
    {
	// only the first m_lNumBins times
	HX_ASSERT(-1 == m_lOldest);
	HX_ASSERT(m_lNewest < m_lNumBins);
    }
#endif
}

void
CTimeoutBin::DeleteAllBins()
{
    // all the entries have better been cleaned up ;)
    CHXSimpleList* pBin = NULL;
    for (int i = 0; i < m_lNumBins; i++)
    {	
	pBin = (CHXSimpleList*)m_bins.GetAt(i);

	pBin->RemoveAll();	
	HX_DELETE(pBin);
    }    

    m_bins.RemoveAll();
}

void
CTimeoutBin::RemoveEntry(BinInfo binInfo)
{
    CHXSimpleList* pBin = (CHXSimpleList*)m_bins.GetAt(binInfo.lBin);

    HX_ASSERT(pBin);
    pBin->RemoveAt(binInfo.pos);
}

