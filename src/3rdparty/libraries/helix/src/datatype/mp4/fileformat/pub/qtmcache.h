/****************************************************************************
 * 
 *  $Id: qtmcache.h,v 1.3 2006/03/22 02:35:34 ckarusala Exp $
 *
 *  Copyright (C) 1995-1999 RealNetworks, Inc. All rights reserved.
 *  
 *  http://www.real.com/devzone
 *
 *  This program contains proprietary 
 *  information of Progressive Networks, Inc, and is licensed
 *  subject to restrictions on use and distribution.
 *
 *
 *  Memory Cache
 *
 */

#ifndef _QTMCACHE_H_
#define _QTMCACHE_H_

/****************************************************************************
 *  Defines
 */

/****************************************************************************
 *  Includes
 */
#include "hxtypes.h"
#include "hxcom.h"
#include "hxcomm.h"
#include "ihxpckts.h"


/****************************************************************************
 * 
 *  Class:
 *	CQTMemCache
 *
 *  Purpose:
 *	Enables paging of file sections into memory
 *
 */
class CQTMemCache
{
public:
    /*
     *	Constructor/Destructor
     */
    CQTMemCache(void)
	: m_pBuffer(NULL)
	, m_ulBaseOffset(0)
	, m_ulEndOffset(0)
	, m_bEnabled(TRUE)
    {
	;
    }

    ~CQTMemCache()
    {
	HX_RELEASE(m_pBuffer);
    }

    /*
     *	IUnknown methods
     */
    void SetPage(ULONG32 ulBaseOffset, IHXBuffer* pBuffer)
    {
	if (m_pBuffer)
	{
	    m_pBuffer->Release();
	}
	m_pBuffer = pBuffer;
	pBuffer->AddRef();
	m_ulBaseOffset = ulBaseOffset;
	m_ulEndOffset = ulBaseOffset + m_pBuffer->GetSize();
    }

    void ReleasePage(void)
    {
	HX_RELEASE(m_pBuffer);
    }

    HXBOOL IsInPage(ULONG32 ulBaseOffset, ULONG32 ulSize)
    {
        return (m_pBuffer &&
                (ulBaseOffset >= m_ulBaseOffset) &&
                (ulBaseOffset < m_ulEndOffset) &&
                (ulSize <= (m_ulEndOffset - ulBaseOffset)));
    }

    // COM semantics is not followed here (no AddRef of pBuffer) 
    // for efficiency reasons.
    void Get(ULONG32 ulBaseOffset,
	     IHXBuffer* &pBuffer, ULONG32 &ulPageOffset)
    {
	pBuffer = m_pBuffer;
	ulPageOffset = ulBaseOffset - m_ulBaseOffset;
    }

    HXBOOL IsEnabled(void)    { return m_bEnabled; }

    void Enable(HXBOOL bEnable)	{ m_bEnabled = bEnable; }

private:
    IHXBuffer* m_pBuffer;
    ULONG32 m_ulBaseOffset;
    ULONG32 m_ulEndOffset;
    HXBOOL m_bEnabled;
};

#endif  // _QTMCACHE_H_
