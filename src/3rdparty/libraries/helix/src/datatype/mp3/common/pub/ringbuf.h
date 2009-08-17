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

#ifndef _RINGBUF_H_
#define _RINGBUF_H_

///////////////////////////////////////////////////////////////////////////////
// Class:   CIHXRingBuffer
// Purpose: An IHXBuffer based circular buffer class
// Author:  cts
///////////////////////////////////////////////////////////////////////////////
class CIHXRingBuffer : public IHXBuffer
{
public:
    ///////////////////////////////////////////////////////////////////////////
    // Function:    CIHXRingBuffer
    // Purpose:     Constructor that allocates a ring buffer and allocates
    //              space for data wrapping before the actual buffer.  The
    //              buffer looks like this |Wrap Bytes__|Buffer_______________|
    //
    // Params:      pClassFactory is the Real COM class factory pointer
    //              ulBufSize is the size of the ring buffer
    //              ulWrapSize is the size of the wrap data before the buffer
    // Author:      cts
    ///////////////////////////////////////////////////////////////////////////
    CIHXRingBuffer(IHXCommonClassFactory *pClassFactory,
                    UINT32 ulBufSize,
                    UINT32 ulWrapSize);
    ~CIHXRingBuffer();

    ///////////////////////////////////////////////////////////////////////////
	//	IUnknown methods
	///////////////////////////////////////////////////////////////////////////
    STDMETHOD(QueryInterface)	(THIS_
								REFIID riid,
								void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)	(THIS);

    STDMETHOD_(ULONG32,Release)	(THIS);
	
    ///////////////////////////////////////////////////////////////////////////
	//	IHXBuffer methods
	///////////////////////////////////////////////////////////////////////////
    STDMETHOD(Get)              (THIS_ REF(UCHAR*) pData, 
                                 REF(ULONG32) ulLength);

    STDMETHOD(Set)              (THIS_ const UCHAR*	pData, 
                                 ULONG32 ulLength);

    STDMETHOD(SetSize)          (THIS_ ULONG32 ulLength);

    STDMETHOD_(ULONG32,GetSize) (THIS);

    STDMETHOD_(UCHAR*,GetBuffer)(THIS);
    
    ///////////////////////////////////////////////////////////////////////////
    // Function:    Reset
    // Purpose:     Flushes the buffer and resets all variables.
    // Author:      cts
    ///////////////////////////////////////////////////////////////////////////
    void    Reset();

    ///////////////////////////////////////////////////////////////////////////
    // Function:    CopyData
    // Purpose:     Copies data to the next write position of the ring buffer.
    //              This function handles buffer wrapping.
    // Params:      pData is data to copy into the buffer
    //              ulBytes is the amount of data to copy
    // Returns:     The amount of data copied.  If return is less than ulBytes
    //              the buffer is full.
    // Author:      cts
    ///////////////////////////////////////////////////////////////////////////
    UINT32  CopyData(UCHAR *pData, UINT32 ulBytes);
    
    ///////////////////////////////////////////////////////////////////////////
    // Function:    AdvanceRead
    // Purpose:     Advances the read pointer in the buffer handling wrapping.
    // Params:      ulBytes is number of bytes to advance the read pointer
    // Author:      cts
    ///////////////////////////////////////////////////////////////////////////
    void    AdvanceRead(UINT32 ulBytes);

    ///////////////////////////////////////////////////////////////////////////
    // Function:    DecrementRead
    // Purpose:     Decrements the read pointer in the buffer.
    // Params:      ulBytes is number of bytes to decrement the read pointer
    // Author:      cts
    ///////////////////////////////////////////////////////////////////////////
    void    DecrementRead(UINT32 ulBytes);

    ///////////////////////////////////////////////////////////////////////////
    // Function:    GetReadPointer
    // Purpose:     Gives Access to data by supplying the read pointer and
    //              its size.  Its size denotes how many bytes until the end
    //              of the buffer.
    // Params:      ulBytes will be the size of the read pointer
    // Returns:     The read pointer
    // Author:      cts
    ///////////////////////////////////////////////////////////////////////////
    UCHAR*  GetReadPointer(UINT32 &ulBytes);

    ///////////////////////////////////////////////////////////////////////////
    // Function:    Wrap
    // Purpose:     Move the data from the current read postion to the wrap
    //              area before the buffer and set the read pointer to that
    //              position.
    //
    // Params:      nOldBytes is the amount of bytes before your current read
    //              pointer you want to preserve in the wrap.
    // Author:      cts
    ///////////////////////////////////////////////////////////////////////////
    void    Wrap(UINT32 ulOldBytes=0);

    // Status functions
    HX_RESULT   GetError()          {return m_ulError;}

    UINT32  GetBytesWritten()       {return m_ulBytesWritten;}
    UINT32  GetBytesRead()          {return m_ulBytesRead;}
    UINT32  GetBufferSize()         {return m_ulBufSize;}
    UINT32  GetGuardSize()          {return m_ulGuardSize;}
    
    UINT32  GetBytesInBuffer()      {return m_ulBytesWritten - m_ulBytesRead;}
    UINT32  GetFreeBufferSpace()    {return m_ulBufSize - GetBytesInBuffer();}
    UINT32  GetPrevBytes()          {return m_ulRead - m_ulVBufBegin;}

private:
    LONG32      m_lRefCount;

    UCHAR       *m_pBuffer;

    union       {UCHAR *m_pBufBegin; PTR_INT m_ulBufBegin;};
    union       {UCHAR *m_pBufEnd;   PTR_INT m_ulBufEnd;};
    union       {UCHAR *m_pWrite;    PTR_INT m_ulWrite;};
    union       {UCHAR *m_pRead;     PTR_INT m_ulRead;};
    union       {UCHAR *m_pVBufBegin;PTR_INT m_ulVBufBegin;};

    UINT32      m_ulBytesWritten,
                m_ulBytesRead,
                m_ulBufSize,
                m_ulGuardSize;

    IHXBuffer  *m_pIhxBuffer;

    HX_RESULT   m_ulError;
};
#endif
