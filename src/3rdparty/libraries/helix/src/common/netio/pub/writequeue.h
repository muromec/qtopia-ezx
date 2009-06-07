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

#ifndef _WRITEQUEUE_H_
#define _WRITEQUEUE_H_

class CHXWriteQueue
{
public:
    CHXWriteQueue(void);
    virtual ~CHXWriteQueue(void);

    HXBOOL        IsEmpty(void) { return (m_uQueueUsed == 0); }
    UINT32      GetSize(void) { return MAX_IP_PACKET*(m_uQueueLen/HX_IOV_MAX); }
    UINT32      GetQueuedBytes(void) { return m_uQueuedBytes; }

    virtual HX_RESULT SetSize(UINT32 uSize);
    virtual HX_RESULT Enqueue(UINT32 nVecLen, IHXBuffer** ppBufVec,
                              IHXSockAddr* pAddr) = 0;
    virtual void      Discard(void);
    virtual void      FillVector(UINT32* pvlen, hx_iov* piov, IHXSockAddr** ppAddr) = 0;
    virtual HX_RESULT Dequeue(UINT32 uSize) = 0;

protected:
    UINT32              m_uHeadPos;
    UINT32              m_uTailPos;
    UINT32              m_uQueuedBytes;
    UINT32              m_uQueueUsed;
    UINT32              m_uQueueLen;
    IHXBuffer**         m_ppBufQueue;
};

class CHXStreamWriteQueue : public CHXWriteQueue
{
public:
    CHXStreamWriteQueue(void);
    virtual ~CHXStreamWriteQueue(void);

    virtual HX_RESULT Enqueue(UINT32 nVecLen, IHXBuffer** ppBufVec,
                              IHXSockAddr* pAddr);
    virtual void      FillVector(UINT32* pvlen, hx_iov* piov, IHXSockAddr** ppAddr);
    virtual HX_RESULT Dequeue(UINT32 uSize);

protected:
    UINT32              m_uOffset;
};

class CHXDatagramWriteQueue : public CHXWriteQueue
{
public:
    CHXDatagramWriteQueue(void);
    virtual ~CHXDatagramWriteQueue(void);

    virtual HX_RESULT SetSize(UINT32 uSize);
    virtual HX_RESULT Enqueue(UINT32 nVecLen, IHXBuffer** ppBufVec,
                              IHXSockAddr* pAddr);
    virtual void      FillVector(UINT32* pvlen, hx_iov* piov, IHXSockAddr** ppAddr);
    virtual HX_RESULT Dequeue(UINT32 uSize);

protected:
    IHXSockAddr**       m_ppAddrQueue;
};

#endif /* _WRITEQUEUE_H_ */
