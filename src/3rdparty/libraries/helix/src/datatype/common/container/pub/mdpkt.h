/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: mdpkt.h,v 1.6 2005/04/27 13:57:38 ehyche Exp $
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

#ifndef _MDPKT_H_
#define _MDPKT_H_

/****************************************************************************
 *  Includes
 */
#include "hxtypes.h"
#include "hxresult.h"
#include "hxassert.h"


/****************************************************************************
 *  Globals
 */
typedef void (*MPBufferKillerFunc) (void* pBuffer, 
				    void* pUserData);
typedef void (*MPSampleDescKillerFunc) (void* pSampleDesc, 
					void* pUserData);

enum
{
    MDPCKT_IS_KEYFRAME_FLAG	    = 0x01,
    MDPCKT_FOLLOWS_LOSS_FLAG	    = 0x02,
    MDPCKT_HAS_UKNOWN_TIME_FLAG	    = 0x04,
    MDPCKT_HAS_MARKER_FLAG	    = 0x08,
    MDPCKT_IS_INVISIBLE		    = 0x10,
    MDPCKT_PARTIALLY_PROCESSED	    = 0x20,
    MDPCKT_USES_IHXBUFFER_FLAG	    = 0x80
};


/****************************************************************************
 *  CMediaPacket
 */
class CMediaPacket
{
public:
    CMediaPacket(void)
	: m_pData(NULL)
	, m_ulDataSize(0)
	, m_ulTime(0)
	, m_ulFlags(0)
	, m_pSampleDesc(NULL)
	, m_pUserData(NULL)
	, m_fpBufferKiller(NULL)
	, m_fpSampleDescKiller(NULL)
	, m_pBuffer(NULL)
	, m_ulBufferSize(0)
    {
	;
    }
 
    CMediaPacket(void* pBuffer,
		 UINT8* pData, 
		 ULONG32 ulBufferSize,
		 ULONG32 ulDataSize,
		 ULONG32 ulTime,
		 ULONG32 ulFlags,
		 void* pSampleDesc)
	: m_pData(pData)
	, m_ulDataSize(ulDataSize)
	, m_ulTime(ulTime)
	, m_ulFlags(ulFlags)
	, m_pSampleDesc(pSampleDesc)
	, m_pUserData(NULL)
	, m_fpBufferKiller(NULL)
	, m_fpSampleDescKiller(NULL)
	, m_pBuffer(pBuffer)
	, m_ulBufferSize(ulBufferSize)
    {
	if (IsUsingIHXBuffer())
	{
	    ((IHXBuffer*) pBuffer)->AddRef();
	}
    }
    
    CMediaPacket(void* pBuffer,
		 ULONG32 ulBufferSize,
		 ULONG32 ulFlags = 0)
	: m_pData(0)
	, m_ulDataSize(0)
	, m_ulTime(0)
	, m_ulFlags(ulFlags)
	, m_pSampleDesc(NULL)
	, m_pUserData(NULL)
	, m_fpBufferKiller(NULL)
	, m_fpSampleDescKiller(NULL)
	, m_pBuffer(pBuffer)
	, m_ulBufferSize(ulBufferSize)
    {
	if (IsUsingIHXBuffer())
	{
	    ((IHXBuffer*) pBuffer)->AddRef();
	}
    }

    ~CMediaPacket()
    {
	;
    }
    
    void Init(UINT8* pData,
	      ULONG32 ulDataSize,
	      ULONG32 ulTime,
	      ULONG32 ulFlags,
	      void* pSampleDesc)
    {
	m_pData = pData;
	m_ulDataSize = ulDataSize;
	m_ulTime = ulTime;
	if (IsUsingIHXBuffer())
	{
	    m_ulFlags = ulFlags | MDPCKT_USES_IHXBUFFER_FLAG;
	}
	else
	{
	    m_ulFlags = ulFlags;
	}

	if (m_pSampleDesc && m_fpSampleDescKiller)
	{
	    (*m_fpSampleDescKiller)(m_pSampleDesc, m_pUserData);
	}
	m_pSampleDesc = pSampleDesc;
    }

    void Clear(void)
    {
	if (m_pBuffer)
	{
	    if (IsUsingIHXBuffer())
	    {
		((IHXBuffer*) m_pBuffer)->Release();
	    }
	    else
	    {
		if (m_fpBufferKiller)
		{
		    (*m_fpBufferKiller)(m_pBuffer, m_pUserData);
		}
		else
		{
		    delete [] ((UINT8*) m_pBuffer);
		}
	    }

	    m_pBuffer = NULL;
	}

	m_pData = NULL;

	if (m_pSampleDesc)
	{
	    if (m_fpSampleDescKiller)
	    {
		(*m_fpSampleDescKiller)(m_pSampleDesc, m_pUserData);
	    }
	    m_pSampleDesc = NULL;
	}	
    }

    void SetBuffer(void* pBuffer,
		   UINT8* pData, 
		   ULONG32 ulBufferSize,
		   ULONG32 ulDataSize,
		   ULONG32 ulBufferFlag = 0)
    {
	if (m_pBuffer)
	{
	    if (IsUsingIHXBuffer())
	    {
		((IHXBuffer*) m_pBuffer)->Release();
	    }
	    else
	    {
		if (m_fpBufferKiller)
		{
		    (*m_fpBufferKiller)(m_pBuffer, m_pUserData);
		}
		else
		{
		    delete [] ((UINT8*) m_pBuffer);
		}
	    }  
	}

	m_pBuffer = pBuffer;

	NoteIHXBuffer(ulBufferFlag == MDPCKT_USES_IHXBUFFER_FLAG);

	if (IsUsingIHXBuffer())
	{
	    ((IHXBuffer*) m_pBuffer)->AddRef();
	}

	m_pData = pData;
	m_ulBufferSize = ulBufferSize;
	m_ulDataSize = ulDataSize;
    }

    HXBOOL IsUsingIHXBuffer(void)
    {
	return ((m_ulFlags & MDPCKT_USES_IHXBUFFER_FLAG) != 0);
    }

    void SetSampleDesc(void* pSampleDesc)
    {
	if (m_pSampleDesc && m_fpSampleDescKiller)
	{
	    (*m_fpSampleDescKiller)(m_pSampleDesc, m_pUserData);
	}
	m_pSampleDesc = pSampleDesc;
    }

    void SetSampleDescKiller(MPSampleDescKillerFunc fpSampleDescKiller)
    {
	m_fpSampleDescKiller = fpSampleDescKiller;
    }

    void SetBufferKiller(MPBufferKillerFunc fpBufferKiller)
    {
	m_fpBufferKiller = fpBufferKiller;
    }

    static ULONG32 GetBufferSize(CMediaPacket* pPacket)
    {
	return pPacket->m_ulBufferSize;
    }

    static void DeletePacket(CMediaPacket* pPacket)
    {
	pPacket->Clear();

	delete pPacket;
    }

    
    UCHAR* m_pData;
    ULONG32 m_ulDataSize;  
    ULONG32 m_ulTime;
    ULONG32 m_ulFlags;
    void* m_pSampleDesc;
    void* m_pUserData;

private:
    MPBufferKillerFunc m_fpBufferKiller;
    MPSampleDescKillerFunc m_fpSampleDescKiller;

    void NoteIHXBuffer(HXBOOL bUsesIHXBuffer)
    {
	m_ulFlags = m_ulFlags & (~MDPCKT_USES_IHXBUFFER_FLAG);
	m_ulFlags |= (bUsesIHXBuffer ? MDPCKT_USES_IHXBUFFER_FLAG : 0);
    }

    void*   m_pBuffer;
    ULONG32 m_ulBufferSize;
};

#endif	// _MDPKT_H_
