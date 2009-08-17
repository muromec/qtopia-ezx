/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: vidrendf.h,v 1.20 2006/10/19 23:18:22 milko Exp $
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

#ifndef __VIDRENDF_H__
#define __VIDRENDF_H__

/****************************************************************************
 *  Includes
 */
#include "hxtypes.h"
#include "hxcom.h"
#include "hxslist.h"
#include "hxwintyp.h"
#include "hxcomm.h"
#include "ihxpckts.h"
#include "hxthread.h"
#include "hxvsurf.h"
#include "chxbufpl.h"
#include "cringbuf.h"
#include "mdpkt.h"


/****************************************************************************
 *  Defines
 */
#define MAX_VIDREND_DECODE_SPIN	    100


/****************************************************************************
 *  Globals
 */
class CVideoRenderer;


/****************************************************************************
 *  CVideoFormat
 */
class CVideoFormat : public IUnknown
{
public:
    /*
     *	Constructor/Destructor
     */
    CVideoFormat(IHXCommonClassFactory* pCommonClassFactory,
		 CVideoRenderer* pVideoRenderer);

    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)	(THIS);

    STDMETHOD_(ULONG32,Release)	(THIS);

    /*
     *	Public but Fixed Core functionality - not deriveable
     */
    HXBOOL Enqueue(IHXPacket* pCodecData);
    void ReturnAssembledPacket(CMediaPacket* pFramePacket);
    HX_RESULT Requeue(CMediaPacket* pFramePacket);
    CMediaPacket* Dequeue(void);
    HXBOOL DecodeFrame(UINT32 ulMaxExtraFrames = MAX_VIDREND_DECODE_SPIN);
    HXBOOL ReturnDecodedPacket(CMediaPacket* pDecodedPacket);
    HXBOOL CanReturnDecodedPacket(void)
    {
	return !m_pOutputQueue->IsFull(TRUE);
    }

    void OnRawPacketsEnded(void);

    HXBOOL GetNextFrameTime(UINT32 &ulTime)
    {
	HXBOOL bRetVal = FALSE;
	CMediaPacket* pFrame = PeekOutQueueHeadPacket();
	
	if (pFrame)
	{
	    ulTime = pFrame->m_ulTime;
	    bRetVal = TRUE;
	}
	
	return bRetVal;
    }

    HXBOOL IsNextFrameSkippable(HXBOOL &bIsSkippable)
    {
	HXBOOL bRetVal = FALSE;
	CMediaPacket* pFrame = PeekOutQueueHeadPacket();
	
	if (pFrame)
	{
	    bIsSkippable = IsFrameSkippable(pFrame);
	    bRetVal = TRUE;
	}
	
	return bRetVal;
    }

    CHXBufferPool* GetFramePool(void)	{ return m_pFramePool; }

    ULONG32 GetAssembledPacketQueueDepth(void)	{ return m_InputQueue.GetCount(); }

    ULONG32 GetDecodedFrameQueueDepth(void) { return m_pOutputQueue->Count(); }
    ULONG32 GetDecodedFrameQueueCapacity(void) { return m_pOutputQueue->MaxCount(); }

    void AdjustDecodedFrameQueueCapacity(LONG32 ulMaxCount)
    {
	m_pOutputQueue->SetMaxCount(ulMaxCount);
    }

    void SetStartTime(ULONG32 ulTime)	{ m_ulStartTime = ulTime; }
    ULONG32 GetStartTime(void)		{ return m_ulStartTime; }

    ULONG32 GetLastDecodedFrameTime(void)   { return m_ulLastDecodedFrameTime; }

    void SuspendDecode(HXBOOL bSuspend)	{ m_bDecodeSuspended = bSuspend; }
    HXBOOL IsDecodeSuspended(void)	{ return m_bDecodeSuspended; }

    void SetDropLateAccelKeyFramesFlag(HXBOOL bFlag) { m_bDropLateAccelKeyFrames = bFlag; }
    HXBOOL GetDropLateAccelKeyFramesFlag() { return m_bDropLateAccelKeyFrames; }

    HXBOOL AreRawPacketsDone(void)	{ return m_bRawPacketsDone; }


    /*
     *	Public and Customizable functionality - derive to customize
     */
    virtual HX_RESULT Init(IHXValues* pHeader);

    virtual HXBOOL IsFrameSkippable(CMediaPacket* pFrame)
    {
	return (pFrame->m_pSampleDesc == NULL);
    }

    virtual void OnDecodedPacketRelease(CMediaPacket* &pPacket);

    virtual void Reset(void);

    virtual void Close(void);

    virtual HX_RESULT InitBitmapInfoHeader(HXBitmapInfoHeader &BitmapInfoHeader,
					   CMediaPacket* pVideoPacket);
    
    virtual HXBOOL IsBitmapFormatChanged(HXBitmapInfoHeader &BitmapInfoHeader,
				       CMediaPacket* pVideoPacket)
    {
	return (pVideoPacket->m_pSampleDesc != NULL);
    }

    virtual ULONG32 GetDefaultPreroll(IHXValues* pValues);

    virtual ULONG32 GetMinimumPreroll(IHXValues* pValues);

    virtual ULONG32 GetMaximumPreroll(IHXValues* pValues);

    virtual const char** GetMimeTypes(void);

    virtual ULONG32 GetMaxDecodedFrames(void);

    virtual ULONG32 GetMaxDecodedFramesInStep(void);

    virtual HX_RESULT SetVelocity(INT32 lVel);
    virtual HX_RESULT SetKeyFrameMode(HXBOOL bMode);

    ULONG32 GetOutputQueueDuration() const;
    void SetFillbackDecodeNeeded(HXBOOL bIsNeeded = TRUE)  { m_bIsFillbackDecodeNeeded = bIsNeeded; }
    HXBOOL IsFillbackDecodeNeeded(void)	{ return m_bIsFillbackDecodeNeeded; }

    HX_RESULT GetLastError();

protected:
    virtual ~CVideoFormat();

    /*
     *	Protected but Customizable functionality - derive to customize
     */
    virtual CHXBufferPool* CreateBufferPool(void);
    virtual CMediaPacket*  CreateAssembledPacket(IHXPacket* pPayloadData);
    virtual CMediaPacket*  CreateDecodedPacket(CMediaPacket* pCodedPacket);
    virtual HXBOOL         IsKeyFramePacket(IHXPacket* pPacket) { return FALSE; }
    virtual HXBOOL         IsFirstKeyFramePacket(IHXPacket* pPacket) { return FALSE; }
    virtual void           OnPacketsEnded() {}

    /*
     *	Usable from the derived renderer format
     */
    IHXCommonClassFactory*	m_pCommonClassFactory;
    IHXValues*			m_pHeader;
    CHXBufferPool*		m_pFramePool;
    HX_RESULT			m_LastError;
    INT32                       m_lPlaybackVelocity;
    HXBOOL                      m_bKeyFrameMode;
    HXBOOL                      m_bDropLateAccelKeyFrames;

private:
    /*
     *	Private and Fixed functionality
     */
    void _Reset(void);
    void _Close(void);

    CMediaPacket* PeekOutQueueHeadPacket(void)
    {
	return (CMediaPacket*) m_pOutputQueue->Peek();
    }

    void FlushOutputQueue(void);
    void ClearReversalQueue();
    HXBOOL _Enqueue(IHXPacket* pPacket);

    /*
     *	Controls of the renderer format packet pump
     */
    IHXMutex*			m_pMutex;
    IHXMutex*			m_pAssemblerMutex;
    IHXMutex*			m_pDecoderMutex;
    HXBOOL			m_bIsFillbackDecodeNeeded;
    CHXSimpleList		m_InputQueue;
    CRingBuffer*		m_pOutputQueue;
    CHXSimpleList*              m_pReversalQueue;
    LONG32			m_lMaxBufferedDecodedFrames;
    ULONG32			m_ulStartTime;
    ULONG32			m_ulLastDecodedFrameTime;
    HXBOOL			m_bDecodeSuspended;
    HXBOOL			m_bRawPacketsDone;

    CVideoRenderer*		m_pVideoRenderer;

    LONG32			m_lRefCount;
};

#endif	// __VIDRENDF_H__
