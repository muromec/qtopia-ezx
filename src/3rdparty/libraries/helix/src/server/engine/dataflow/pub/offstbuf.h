/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: offstbuf.h,v 1.3 2003/02/03 21:32:41 tbradley Exp $ 
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

#ifndef _OFFSTBUF_H_
#define _OFFSTBUF_H_

#include "hxbuffer.h"

class OffsetBuf: public CHXBuffer
{
public:
					OffsetBuf();

    void				SetOffset(UINT32 offset);
    STDMETHOD_(UCHAR*,GetBuffer)	(THIS);
    UCHAR*				GetBufferNoOffset();
    STDMETHOD_(ULONG32,GetSize)		(THIS);     

private:
    UINT32				m_offset;
};

inline
OffsetBuf::OffsetBuf()
{
    m_offset = 0;
}

inline void
OffsetBuf::SetOffset(UINT32 offset)
{
    m_offset = offset;
}

inline
STDMETHODIMP_(UCHAR*) 
OffsetBuf::GetBuffer()
{
    return CHXBuffer::GetBuffer() + m_offset;
}

inline UCHAR*
OffsetBuf::GetBufferNoOffset()
{
    return CHXBuffer::GetBuffer();
}

inline
STDMETHODIMP_(ULONG32)
OffsetBuf::GetSize()
{
    return CHXBuffer::GetSize() - m_offset;
}

#endif /* _OFFSTBUF_H_ */
