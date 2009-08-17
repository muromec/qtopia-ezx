/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: vidrendf.cpp,v 1.31 2006/10/19 23:18:22 milko Exp $
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

#ifdef ENABLE_TRACE
#define HX_TRACE_THINGY(x, m, l)					\
    {							\
	FILE* f1;					\
	f1 = ::fopen(x, "a+");				\
	(f1)?(::fprintf(f1, "%ld - %s = %ld \n", HX_GET_BETTERTICKCOUNT(), m, l), ::fclose(f1)):(0);\
    }
#define STAMPBUF(x, m)					\
    {							\
	FILE* f1;					\
	f1 = ::fopen(x, "a+");				\
	(f1)?(::fprintf(f1, "%ld - %s\n", HX_GET_BETTERTICKCOUNT(), m), ::fclose(f1)):(0);\
    }
#else	// ENABLE_TRACE
#define HX_TRACE_THINGY(x, m, l)

#ifdef MEASURE_PERF
#define LOGBUFFSIZE 100000
char textmem[LOGBUFFSIZE];
char *textptr = textmem;
#define STAMPBUF(x, txt) { if (textptr - textmem < LOGBUFFSIZE) textptr += SafeSprintf(textptr, LOGBUFFSIZE - (textptr - textmem), "%s: %ld*\n", txt, HX_GET_BETTERTICKCOUNT()); }
#else	// MEASURE_PERF
#define STAMPBUF(x, txt)
#endif	// MEASURE_PERF

#endif	// ENABLE_TRACE

/****************************************************************************
 *  Defines
 */
#if !defined(HELIX_FEATURE_MIN_HEAP)
#define MAX_BUFFERED_DECODED_FRAMES		12
#else	// HELIX_FEATURE_MIN_HEAP
#define MAX_BUFFERED_DECODED_FRAMES		2
#endif	// HELIX_FEATURE_MIN_HEAP

#define MAX_DECODED_FRAMES_IN_STEP		1

#if defined(HELIX_FEATURE_MIN_HEAP)
#define FORMAT_MINIMUM_PREROLL			0
#define FORMAT_DEFAULT_PREROLL			2000
#define FORMAT_MAXIMUM_PREROLL			3000
#else
#define FORMAT_MINIMUM_PREROLL			0
#define FORMAT_DEFAULT_PREROLL			5000
#define FORMAT_MAXIMUM_PREROLL			15000
#endif


/****************************************************************************
 *  Includes
 */
#include "vidrendf.h"
#include "vidrend.h"
#include "hxtick.h"
#include "hxassert.h"
#define HELIX_FEATURE_LOGLEVEL_NONE
#include "hxtlogutil.h"
#include "hxstrutl.h"
#include "hxpacketflags.h"
#include "pckunpck.h"

/****************************************************************************
 *  Debug
 */
#ifdef ENABLE_FRAME_TRACE
#define MAX_FRAME_TRACE_ENTRIES	100000
ULONG32 ulFrameTraceIdx = 0;
LONG32 frameTraceArray[MAX_FRAME_TRACE_ENTRIES][3];

void DumpFrameEntries(void)
{
    FILE* pFile = NULL;
    ULONG32 ulIdx;

    if (ulFrameTraceIdx > 0)
    {
	pFile = fopen("c:\\buf.txt", "wb");
    }

    if (pFile)
    {
	for (ulIdx = 0; ulIdx < ulFrameTraceIdx; ulIdx++)
	{
	    fprintf(pFile, "%c(%d) = %x\n", (char) frameTraceArray[ulIdx][1], 
					   frameTraceArray[ulIdx][2], 
					   frameTraceArray[ulIdx][0]);
	}

	fclose(pFile);
    }

    ulFrameTraceIdx = 0;
}
#endif	// ENABLE_FRAME_TRACE


/****************************************************************************
 *  Method:
 *    CVideoFormat::CVideoFormat
 *
 */
CVideoFormat::CVideoFormat(IHXCommonClassFactory* pCommonClassFactory,
			   CVideoRenderer* pVideoRenderer)
	: m_pCommonClassFactory(pCommonClassFactory)
	, m_pHeader(NULL)
	, m_pFramePool(NULL)
	, m_LastError(HXR_OK)
        , m_lPlaybackVelocity(HX_PLAYBACK_VELOCITY_NORMAL)
        , m_bKeyFrameMode(FALSE)
        , m_bDropLateAccelKeyFrames(FALSE)
	, m_bIsFillbackDecodeNeeded(FALSE)
	, m_pMutex(NULL)
	, m_pAssemblerMutex(NULL)
        , m_pDecoderMutex(NULL)
	, m_pOutputQueue(NULL)
        , m_pReversalQueue(NULL)
	, m_lMaxBufferedDecodedFrames(MAX_BUFFERED_DECODED_FRAMES)
	, m_ulStartTime(0)
	, m_ulLastDecodedFrameTime(0)
	, m_bDecodeSuspended(FALSE)
	, m_bRawPacketsDone(FALSE)
	, m_pVideoRenderer(pVideoRenderer)
	, m_lRefCount(0)
{
    HX_ASSERT(m_pCommonClassFactory);

    m_pCommonClassFactory->AddRef();
    m_pVideoRenderer->AddRef();

    CreateInstanceCCF(CLSID_IHXMutex, (void**)&m_pMutex, m_pCommonClassFactory);
    CreateInstanceCCF(CLSID_IHXMutex, (void**)&m_pAssemblerMutex, m_pCommonClassFactory);
    CreateInstanceCCF(CLSID_IHXMutex, (void**)&m_pDecoderMutex, m_pCommonClassFactory);
}


/****************************************************************************
 *  Method:
 *    CVideoFormat::~CVideoFormat
 *
 */
CVideoFormat::~CVideoFormat()
{
    _Close();
    HX_RELEASE(m_pMutex);
}


/****************************************************************************
 *  Method:
 *    CVideoFormat::Close()
 *
 */
void CVideoFormat::Close()
{
    _Close();

#ifdef ENABLE_FRAME_TRACE
    DumpFrameEntries();
#endif	// ENABLE_FRAME_TRACE
}

void CVideoFormat::_Close()
{
    _Reset();

    HX_DELETE(m_pFramePool);
    HX_DELETE(m_pOutputQueue);

    HX_RELEASE(m_pAssemblerMutex);
    HX_RELEASE(m_pDecoderMutex);

    HX_RELEASE(m_pHeader);
    HX_RELEASE(m_pVideoRenderer);
    HX_RELEASE(m_pCommonClassFactory);
}


/****************************************************************************
 *  Method:
 *    CVideoFormat::Init
 *
 */
HX_RESULT CVideoFormat::Init(IHXValues* pHeader)
{
    HX_RESULT retVal = HXR_OK;

    HX_RELEASE(m_pHeader);
    m_pHeader = pHeader;
    if (m_pHeader)
    {
	m_pHeader->AddRef();
    }

    if (SUCCEEDED(retVal))
    {
	m_pFramePool = CreateBufferPool();
    }

    if (SUCCEEDED(retVal))
    {	
        ULONG32 ulMaxDecodedFramesInStep = GetMaxDecodedFramesInStep();

	m_lMaxBufferedDecodedFrames = GetMaxDecodedFrames();
        
        if (ulMaxDecodedFramesInStep > ((UINT32) m_lMaxBufferedDecodedFrames))
        {
            m_lMaxBufferedDecodedFrames = ulMaxDecodedFramesInStep;
        }

	FlushOutputQueue();
	HX_DELETE(m_pOutputQueue);

	m_pOutputQueue = new CRingBuffer(m_lMaxBufferedDecodedFrames);

	retVal = HXR_OUTOFMEMORY;
	if (m_pOutputQueue)
	{
	    if (ulMaxDecodedFramesInStep > 1)
	    {
		// We reduce the target capacity of the ring buffer in order 
		// to have reserve for needed number of frames a decoder can
		// output in one step (one frame in, N frames out).
		m_pOutputQueue->SetMaxCount(m_lMaxBufferedDecodedFrames - 
		                            ulMaxDecodedFramesInStep + 
					    1);
	    }

	    retVal = HXR_OK;
	}
    }

    return retVal;
}


/****************************************************************************
 *  Method:
 *    CVideoFormat::CreateBufferPool
 *
 */
CHXBufferPool* CVideoFormat::CreateBufferPool(void)
{
    return new CHXBufferPool((IUnknown*)(IHXCommonClassFactory*)m_pCommonClassFactory,
			     (BufferSizeFunc) CMediaPacket::GetBufferSize,
			     (BufferKillerFunc) CMediaPacket::DeletePacket);
}

/****************************************************************************
 *  Method:
 *    CVideoFormat::CreateAssembledPacket
 *
 */
CMediaPacket* CVideoFormat::CreateAssembledPacket(IHXPacket* pPayloadData)
{
    CMediaPacket* pFramePacket = NULL;

    if (pPayloadData->IsLost())
    {
	pFramePacket = NULL;
    }
    else
    {
	IHXBuffer* pBuffer = pPayloadData->GetBuffer();

	if (pBuffer)
	{
	    pFramePacket = new CMediaPacket(pBuffer,
					    pBuffer->GetBuffer(), 
					    pBuffer->GetSize(),
					    pBuffer->GetSize(),
					    pPayloadData->GetTime(),
					    MDPCKT_USES_IHXBUFFER_FLAG,
					    NULL);

	    pBuffer->Release();
	}
    }

    return pFramePacket;
}


/****************************************************************************
 *  Method:
 *    CVideoFormat::ReturnAssembledPacket
 *
 */
void CVideoFormat::ReturnAssembledPacket(CMediaPacket* pFramePacket)
{
    if (pFramePacket)
    {
        HXLOGL4(HXLOG_BVID, "CVideoFormat::ReturnAssembledPacket() ts=%lu flags=0x%08x",
                pFramePacket->m_ulTime, pFramePacket->m_ulFlags);
        // If we are not in keyframe-only mode; or if 
        // we ARE in keyframe-only mode and this is a keyframe,
        // then add it to the input queue.
        if (!m_bKeyFrameMode ||
            (pFramePacket->m_ulFlags & HX_KEYFRAME_FLAG))
        {
            HXLOGL4(HXLOG_BVID, "\tAdded to input queue", pFramePacket->m_ulTime);
            m_pMutex->Lock();
            m_InputQueue.AddTail(pFramePacket);
            m_pMutex->Unlock();
        }
        else
        {
            HXLOGL4(HXLOG_BVID, "\tAssembled frame deleted");
            pFramePacket->Clear();
            HX_DELETE(pFramePacket);
        }
    }
}


/****************************************************************************
 *  Method:
 *    CVideoFormat::Enqueue
 *
 */
HXBOOL CVideoFormat::Enqueue(IHXPacket* pPayloadData)
{
#if defined(HELIX_FEATURE_PLAYBACK_VELOCITY)
    HXBOOL bRet = FALSE;

    // Are we in reverse playback?
    if (m_lPlaybackVelocity < 0)
    {
        // We are in reverse playback.
        //
        // If we are in reverse playback, then we need to
        // queue packets in the reversal queue until we
        // encounter the last packet of a keyframe
        // (which would have been the first packet of
        // a keyframe in forward playback)
        if (!m_pReversalQueue)
        {
            m_pReversalQueue = new CHXSimpleList();
        }
        if (m_pReversalQueue && pPayloadData)
        {
            // AddRef the packet
            pPayloadData->AddRef();
            // Add the packet to the HEAD of the reversal queue.
            // Since we'll be taking them off the head as well,
            // then this provides the reversal
            m_pReversalQueue->AddHead((void*) pPayloadData);
            // Is this the first keyframe packet?
            if (IsFirstKeyFramePacket(pPayloadData))
            {
                HXLOGL4(HXLOG_BVID, "Dumping %lu packets in reversal queue to assembler", m_pReversalQueue->GetCount());
                // This is the first packet of a keyframe
                // (in normal forward order), so we can now
                // pass all the packets in the reversal queue
                // into the assembler.
                while (m_pReversalQueue->GetCount() > 0)
                {
                    IHXPacket* pPacket = (IHXPacket*) m_pReversalQueue->RemoveHead();
                    if (pPacket)
                    {
                        bRet = _Enqueue(pPacket);
                    }
                }
            }
        }
    }
    else
    {
        // Forward playback - simply pass on to _Enqueue()
        bRet = _Enqueue(pPayloadData);
    }

    return bRet;
#else /* #if defined(HELIX_FEATURE_PLAYBACK_VELOCITY) */
    return _Enqueue(pPayloadData);
#endif /* #if defined(HELIX_FEATURE_PLAYBACK_VELOCITY) */
}

/****************************************************************************
 *  Method:
 *    CVideoFormat::Requeue
 *
 */
HX_RESULT CVideoFormat::Requeue(CMediaPacket* pFramePacket)
{
    if (pFramePacket != NULL)
    {
        HXLOGL4(HXLOG_BVID, "Adding assembled frame %lu to input queue HEAD", pFramePacket->m_ulTime);
	m_pMutex->Lock();
	m_InputQueue.AddHead(pFramePacket);
	m_pMutex->Unlock();
    }

    return HXR_OK;
}

/****************************************************************************
 *  Method:
 *    CVideoFormat::Dequeue
 *
 */
CMediaPacket* CVideoFormat::Dequeue(void)
{
    CMediaPacket* pPacket = (CMediaPacket*) m_pOutputQueue->Get();

#ifdef ENABLE_FRAME_TRACE
    if (pPacket && (ulFrameTraceIdx < MAX_FRAME_TRACE_ENTRIES))
    {
	frameTraceArray[ulFrameTraceIdx][2] = pPacket->m_ulTime;
	frameTraceArray[ulFrameTraceIdx][0] = 
	    (LONG32) pPacket->m_pData;
	frameTraceArray[ulFrameTraceIdx++][1] = 'G';
    }
#endif	// ENABLE_FRAME_TRACE

    return pPacket;
}


/****************************************************************************
 *  Method:
 *    CVideoFormat::Reset
 *
 */
void CVideoFormat::Reset(void)
{
    _Reset();
}

void CVideoFormat::_Reset()
{
    m_pMutex->Lock();

    CMediaPacket* pFrame;

    while (!m_InputQueue.IsEmpty())
    {
	pFrame = (CMediaPacket*) m_InputQueue.RemoveHead();
	
	pFrame->Clear();
	delete pFrame;
    }
    
    FlushOutputQueue();

    ClearReversalQueue();

    // When the renderer receives a OnEndofPackets(), then the
    // m_bRawPacketsDone flag gets set. However, after a seek,
    // more packets may be on the way, so we need to
    // clear this flag.
    m_bRawPacketsDone = FALSE;
    // Filback decode is one that display portion of the renderer
    // communicates as it releases a displayed frame.
    // Video format object simply maintains the state for it.
    m_bIsFillbackDecodeNeeded = FALSE;

    m_pMutex->Unlock();
}


/****************************************************************************
 *  Method:
 *    CVideoFormat::FlushOutputQueue
 *
 */
void CVideoFormat::FlushOutputQueue(void)
{
    CMediaPacket* pFrame;

    if (m_pOutputQueue)
    {
	while ((pFrame = (CMediaPacket*) m_pOutputQueue->Get()) != NULL)
	{
#ifdef ENABLE_FRAME_TRACE
	    if (ulFrameTraceIdx < MAX_FRAME_TRACE_ENTRIES)
	    {
		frameTraceArray[ulFrameTraceIdx][2] = pFrame->m_ulTime;
		frameTraceArray[ulFrameTraceIdx][0] = 
		    (LONG32) pFrame->m_pData;
		frameTraceArray[ulFrameTraceIdx++][1] = 'F';
	    }
#endif	// ENABLE_FRAME_TRACE

	    pFrame->Clear();
	    delete pFrame;
	}
    }
}

void CVideoFormat::ClearReversalQueue()
{
    if (m_pReversalQueue)
    {
        while (m_pReversalQueue->GetCount() > 0)
        {
            IHXPacket* pPacket = (IHXPacket*) m_pReversalQueue->RemoveHead();
            HX_RELEASE(pPacket);
        }
    }
}

HXBOOL CVideoFormat::_Enqueue(IHXPacket* pPacket)
{
    HXBOOL bEnqueued = FALSE;
    CMediaPacket* pFramePacket = NULL;

    m_pAssemblerMutex->Lock();

    pFramePacket = CreateAssembledPacket(pPacket);
  
    if (pFramePacket != NULL)
    {
        ReturnAssembledPacket(pFramePacket);

	bEnqueued = TRUE;
    } 

    m_pAssemblerMutex->Unlock();
     
    return bEnqueued;
}

/****************************************************************************
 *  Method:
 *    CVideoFormat::DecodeFrame
 *
 */
HXBOOL CVideoFormat::DecodeFrame(UINT32 ulMaxExtraFrames)
{
    CMediaPacket* pEncodedPacket;
    CMediaPacket* pDecodedPacket = NULL;
    ULONG32 ulLoopCounter = 0;

    HXLOGL4(HXLOG_BVID, "CVideoFormat::DecodeFrame() Start");

    m_bIsFillbackDecodeNeeded = FALSE;

    m_pVideoRenderer->BltIfNeeded();

    m_pDecoderMutex->Lock();
    m_pMutex->Lock();
    
    if ((!m_InputQueue.IsEmpty()) &&
	(!m_pOutputQueue->IsFull()) &&
	(!m_bDecodeSuspended))
    {
	do
	{
	    pEncodedPacket = (CMediaPacket*) m_InputQueue.RemoveHead();

	    m_pMutex->Unlock();

	    m_bIsFillbackDecodeNeeded = FALSE;

	    pDecodedPacket = CreateDecodedPacket(pEncodedPacket);

	    if (pDecodedPacket)
	    {
#ifdef ENABLE_FRAME_TRACE
		if (ulFrameTraceIdx < MAX_FRAME_TRACE_ENTRIES)
		{
		    frameTraceArray[ulFrameTraceIdx][2] = pDecodedPacket->m_ulTime;
		    frameTraceArray[ulFrameTraceIdx][0] = 
			(LONG32) pDecodedPacket->m_pData;
		    frameTraceArray[ulFrameTraceIdx++][1] = 'D';
		}
#endif	// ENABLE_FRAME_TRACE

		m_ulLastDecodedFrameTime = pDecodedPacket->m_ulTime;
		HXLOGL4(HXLOG_BVID, "CVideoFormat::DecodeFrame Putting decoded frame PTS=%lu on output queue", pDecodedPacket->m_ulTime);
		m_pOutputQueue->Put(pDecodedPacket, TRUE);

		if (pDecodedPacket->m_pData)
		{
		    m_pDecoderMutex->Unlock();
		    m_pVideoRenderer->BltIfNeeded();
		    HXLOGL4(HXLOG_BVID, "CVideoFormat::DecodeFrame() End");
		    return TRUE;
		}
	    }

	    m_pDecoderMutex->Unlock();

            if( m_LastError == HXR_OUTOFMEMORY )
            {
                m_bDecodeSuspended = TRUE;
            }
            else
            {
                m_pVideoRenderer->BltIfNeeded();
            }
	    ulLoopCounter++;

	    m_pDecoderMutex->Lock();
	    m_pMutex->Lock();
	} while ((!m_InputQueue.IsEmpty()) &&
		 (!m_pOutputQueue->IsFull()) &&
		 ((ulLoopCounter++) < ulMaxExtraFrames) &&
		 (!m_bDecodeSuspended));
    }

    m_pMutex->Unlock();
    m_pDecoderMutex->Unlock();

    HXLOGL4(HXLOG_BVID, "CVideoFormat::DecodeFrame() End");
    
    return pDecodedPacket ? TRUE : FALSE;
}


/****************************************************************************
 *  Method:
 *    CVideoFormat::ReturnDecodedPacket
 *
 */
HXBOOL CVideoFormat::ReturnDecodedPacket(CMediaPacket* pDecodedPacket)
{
    HXBOOL bPacketAccepted = FALSE;

    if (pDecodedPacket)
    {
#ifdef ENABLE_FRAME_TRACE
	if (ulFrameTraceIdx < MAX_FRAME_TRACE_ENTRIES)
	{
	    frameTraceArray[ulFrameTraceIdx][2] = pDecodedPacket->m_ulTime;
	    frameTraceArray[ulFrameTraceIdx][0] = 
		(LONG32) pDecodedPacket->m_pData;
	    frameTraceArray[ulFrameTraceIdx++][1] = 'D';
	}
#endif	// ENABLE_FRAME_TRACE
	    
	m_ulLastDecodedFrameTime = pDecodedPacket->m_ulTime;
	HXLOGL4(HXLOG_BVID, "CVideoFormat::ReturnDecodedPacket Putting decoded frame PTS=%lu on the output queue", pDecodedPacket->m_ulTime);
	bPacketAccepted = m_pOutputQueue->Put(pDecodedPacket, TRUE);
    }

    return bPacketAccepted;
}

void CVideoFormat::OnRawPacketsEnded(void)
{
    // Lock the assembler mutex
    m_pAssemblerMutex->Lock();
    // Call the derived class to do any operations
    // that need to be done when packets are finished
    OnPacketsEnded();
    // Unlock the assembler mutex
    m_pAssemblerMutex->Unlock();
    // Set the flag saying no more raw packets are coming
    m_bRawPacketsDone = TRUE;
}

/****************************************************************************
 *  Method:
 *    CVideoFormat::GetMaxDecodedFrames
 *
 */
ULONG32 CVideoFormat::GetMaxDecodedFrames(void)
{
    return MAX_BUFFERED_DECODED_FRAMES;
}


/****************************************************************************
 *  Method:
 *    CVideoFormat::GetMaxDecodedFramesInStep
 *
 */
ULONG32 CVideoFormat::GetMaxDecodedFramesInStep(void)
{
    return MAX_DECODED_FRAMES_IN_STEP;
}

HX_RESULT CVideoFormat::SetVelocity(INT32 lVel)
{
    m_lPlaybackVelocity = lVel;
    return HXR_OK;
}

HX_RESULT CVideoFormat::SetKeyFrameMode(HXBOOL bMode)
{
    m_bKeyFrameMode = bMode;
    return HXR_OK;
}

ULONG32 CVideoFormat::GetOutputQueueDuration() const
{
    // Compute the number of milliseconds represented by the
    // output queue assuming 30 fps. Since this value is
    // used to adjust preroll, we want to round up so that
    // we ensure we get enough data to fill the queue.
    ULONG32 ulFPS = 30;
    ULONG32 ulRet = 0;

    if (m_pOutputQueue)
    {
        ulRet = (1000 * m_pOutputQueue->MaxCount() + ulFPS - 1) / ulFPS;
    }

    return ulRet;
}

/****************************************************************************
 *  Method:
 *    CVideoFormat::CreateDecodedPacket
 *
 */
CMediaPacket* CVideoFormat::CreateDecodedPacket(CMediaPacket* pCodedPacket)
{
    CMediaPacket* pDecodedPacket = NULL;

    if (pCodedPacket != NULL)
    {
	pCodedPacket->Clear();
	delete pCodedPacket;
	pCodedPacket = NULL;
    }

    return pDecodedPacket;
}


/****************************************************************************
 *  Method:
 *    CVideoFormat::OnDecodedPacketRelease
 *
 */
void CVideoFormat::OnDecodedPacketRelease(CMediaPacket* &pPacket)
{
#ifdef ENABLE_FRAME_TRACE
    if (ulFrameTraceIdx < MAX_FRAME_TRACE_ENTRIES)
    {
	frameTraceArray[ulFrameTraceIdx][2] = pPacket->m_ulTime;
	frameTraceArray[ulFrameTraceIdx][0] = 
	    (LONG32) pPacket->m_pData;
	frameTraceArray[ulFrameTraceIdx++][1] = 'R';
    }
#endif	// ENABLE_FRAME_TRACE
    ;
}

/****************************************************************************
 *  Method:
 *    CVideoFormat::InitBitmapInfoHeader
 *
 */
HX_RESULT CVideoFormat::InitBitmapInfoHeader(
    HXBitmapInfoHeader &BitmapInfoHeader,
    CMediaPacket* pVideoPacket)

{
    HX_RESULT retVal = HXR_OK;

    return retVal;
}


/****************************************************************************
 *  Method:
 *    CVideoFormat::GetDefaultPreroll
 *
 */
ULONG32 CVideoFormat::GetDefaultPreroll(IHXValues* pValues)
{
    return FORMAT_DEFAULT_PREROLL;
}


/****************************************************************************
 *  Method:
 *    CVideoFormat::GetMinimumPreroll
 *
 */
ULONG32 CVideoFormat::GetMinimumPreroll(IHXValues* pValues)
{
    return FORMAT_MINIMUM_PREROLL;
}


/****************************************************************************
 *  Method:
 *    CVideoFormat::GetMaximumPreroll
 *
 */
ULONG32 CVideoFormat::GetMaximumPreroll(IHXValues* pValues)
{
    return FORMAT_MAXIMUM_PREROLL;
}

/****************************************************************************
 *  Method:
 *    CVideoFormat::GetMimeTypes
 *
 */
const char** CVideoFormat::GetMimeTypes(void)
{
    return NULL;
}


// *** IUnknown methods ***

/****************************************************************************
*  IUnknown::AddRef                                            ref:  hxcom.h
*
*  This routine increases the object reference count in a thread safe
*  manner. The reference count is used to manage the lifetime of an object.
*  This method must be explicitly called by the user whenever a new
*  reference to an object is used.
*/
STDMETHODIMP_(ULONG32) CVideoFormat::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/****************************************************************************
*  IUnknown::Release                                           ref:  hxcom.h
*
*  This routine decreases the object reference count in a thread safe
*  manner, and deletes the object if no more references to it exist. It must
*  be called explicitly by the user whenever an object is no longer needed.
*/
STDMETHODIMP_(ULONG32) CVideoFormat::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }
    
    delete this;
    return 0;
}

/****************************************************************************
*  IUnknown::QueryInterface                                    ref:  hxcom.h
*
*  This routine indicates which interfaces this object supports. If a given
*  interface is supported, the object's reference count is incremented, and
*  a reference to that interface is returned. Otherwise a NULL object and
*  error code are returned. This method is called by other objects to
*  discover the functionality of this object.
*/
STDMETHODIMP CVideoFormat::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
	AddRef();
	*ppvObj = this;
	return HXR_OK;
    }
    
    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

HX_RESULT
CVideoFormat::GetLastError()
{
    return m_LastError;
}
