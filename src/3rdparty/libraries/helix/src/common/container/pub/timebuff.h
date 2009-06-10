/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: timebuff.h,v 1.6 2007/07/06 20:35:02 jfinnecy Exp $
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

#ifndef _TIMEBUFF_H_
#define _TIMEBUFF_H_

#include "ihxpckts.h"
#include "hxtbuf.h"
#include "hxstring.h"

/****************************************************************************
 * 
 *	Class:
 *
 *		CHXTimeStampedBuffer
 *
 *	Purpose:
 *
 *		CHXBuffer with a timestamp
 *
 */
class CHXTimeStampedBuffer : public IHXBuffer, public IHXTimeStampedBuffer
{
public:
    	/*
	 *	IUnknown methods
	 */
    STDMETHOD(QueryInterface)	(THIS_
								REFIID riid,
								void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)	(THIS);

    STDMETHOD_(ULONG32,Release)	(THIS);

    /* IHXTimeStamped Buffer Methods */

    STDMETHOD_(UINT32,GetTimeStamp)(THIS);

    STDMETHOD(SetTimeStamp)(THIS_
			    UINT32	ulTimeStamp);

    private:

	LONG32					m_lRefCount;
	UCHAR*					m_pData;
	ULONG32					m_ulLength;
	UINT32					m_ulTimeStamp;

private:

	~CHXTimeStampedBuffer();


public:
	CHXTimeStampedBuffer()
		: m_lRefCount(0)
		, m_pData(NULL)
		, m_ulLength(0)
		, m_ulTimeStamp(0)
		{
		};

	inline CHXTimeStampedBuffer& operator=(const char* psz);
	inline CHXTimeStampedBuffer& operator=(const unsigned char* psz);
	inline CHXTimeStampedBuffer& operator=(const CHXString &str);
	
	/*
	 *	IHXBuffer methods
	 */
    STDMETHOD(Get)				(THIS_
								REF(UCHAR*)		pData, 
								REF(ULONG32)	ulLength);

    STDMETHOD(Set)				(THIS_
								const UCHAR*	pData, 
								ULONG32			ulLength);

    STDMETHOD(SetSize)			(THIS_
								ULONG32			ulLength);

    STDMETHOD_(ULONG32,GetSize)	(THIS);

    STDMETHOD_(UCHAR*,GetBuffer)
								(THIS);

   // serialization method
   static void	Pack	(IHXTimeStampedBuffer* pTimeStampedBuffer, char* pData,
                         UINT32 ulDataBufSize, UINT32& ulSize);
   static void	UnPack	(IHXTimeStampedBuffer*& pTimeStampedBuffer, char* pData, UINT32 ulSize);
};

CHXTimeStampedBuffer& CHXTimeStampedBuffer::operator=(const char* psz)
{
	Set((const unsigned char*)psz, strlen(psz));
	return(*this);
}

CHXTimeStampedBuffer& CHXTimeStampedBuffer::operator=(const unsigned char* psz)
{
	Set(psz, strlen((const char*)psz));
	return(*this);
}

CHXTimeStampedBuffer& CHXTimeStampedBuffer::operator=(const CHXString& str)
{
	Set((const unsigned char*)(const char *)str, str.GetLength());
	return(*this);
}

#endif // _TIMEBUFF_H_
