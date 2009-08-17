/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: fastfile.cpp,v 1.8 2006/06/28 01:23:41 dcollins Exp $ 
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
 *  FastFile - A read-ahead, memory-caching, block-sharing file 
 *  object wrapper.
 *
 *
 *  Copied from miiplin/fastfob.cpp.
 *
 *  Typically, a module will query the server class factory for a
 *  FastFileWrapperFactory, and use that to instantiate wrappers for
 *  existing file objects.
 *
 *  This implementation is NOT threadsafe; ie, each streamer should
 *  have its own factory and should not share wrapped file objects
 *  with other procs.
 *  (The fastfile IHXBuffers created here are threadsafe, though.)  
 */


//system
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#ifdef _UNIX
#include <sys/time.h>
#include <unistd.h>
#endif // _UNIX

//include
#include "hxtypes.h"
#include "hxcom.h"
#include "hxformt.h"
#include "hxengin.h"
#include "hxfiles.h"
#include "ihxpckts.h"
#include "hxmon.h"
#include "hxcomm.h"

//hxdebug
#include "debug.h"
#include "hxassert.h"
//#include "hxtrace.h" //for PrintfStack()

//hxcont
#include "dict.h"

//proxylib
//#include "fastfile.h"

//servsup
#include "cdist_defs.h"

#include "hxtime.h"

//miiplin
#include "fastfile_factory.h"
#include "fastfile.h"
#include "fastfile_stats.h"
#include "mutex.h"

//#define COPY_BUFFERS TRUE
#define COPY_BUFFERS FALSE

#ifdef DEBUG
#define xassert(x) HX_ASSERT(x)
#else
#define xassert(x)
#endif

#define FF_BLKSIZE_MIN  (1 * 1024)
#define FF_BLKSIZE_MAX  (64 * 1024)
#define FF_BLKSIZE_DEF  (16 * 1024)
#define FF_URL_DICT_LIMIT 256

#ifdef _DEBUG
#undef HX_THIS_FILE
static char HX_THIS_FILE[] = __FILE__;
#endif

// expressed as a percentage
#define READ_RATIO 10
#define READ_MIN 4 * 1024


//////////////////////////////////////////////////////////////////////
// FastFileFactory
//////////////////////////////////////////////////////////////////////

FastFileFactory::FastFileFactory(IUnknown* /*IN*/ pContext)
    : m_ulRefCount(0)
    , m_pContext(pContext)
    , m_pMIIStats(NULL)
    , m_bUseWrap2(0)
    , m_ulMaxMemUse(0)
{
    m_pFastFileDict = new Dict;

    // get ptr to cdist stats, if available.

    IHXRegistry* pRegistry = NULL;
    pContext->QueryInterface(IID_IHXRegistry, (void**)&pRegistry);
    HX_ASSERT(pRegistry);

    IHXBuffer* pBuf = NULL;
    if (pRegistry &&
	HXR_OK == pRegistry->GetBufByName(CDIST_REGISTRY_STATISTICS, pBuf))
    {
	m_pMIIStats = *((CDistMIIStatistics**)pBuf->GetBuffer());
	pBuf->Release();
        pBuf = NULL;
    }

    INT32 temp = 0;
    if (pRegistry)
        pRegistry->GetIntByName("config.FastFileMaxMemUse", temp);

    m_ulMaxMemUse = (UINT32)temp;

    HX_RELEASE(pRegistry);
}

FastFileFactory::~FastFileFactory()
{
    // Typically, the factory is never deleted.

    HX_RELEASE(m_pContext);
    delete m_pFastFileDict;
}

/*
 * IHXFastFileFactory methods
 */

STDMETHODIMP
FastFileFactory::Wrap(REF(IUnknown*) /*OUT*/ pWrapper,
		      IUnknown*      /*IN*/  pFileObj,
		      UINT32         /*IN*/  ulBlockSize,
                      BOOL           /*IN*/  bAlignReads,
                      BOOL           /*IN*/  bCacheStats)
{
    m_bUseWrap2 = FALSE;
    if (pFileObj)
    {
	pWrapper = (IHXFileObject*)new FastFile(pFileObj, NULL,
            m_pContext, ulBlockSize, this, m_pFastFileDict, m_pMIIStats, 
	    bAlignReads, bCacheStats, ulBlockSize, m_ulMaxMemUse);

	pWrapper->AddRef();
	return HXR_OK;
    }

    return HXR_INVALID_PARAMETER;
}

STDMETHODIMP
FastFileFactory::Wrap(REF(IUnknown*) /*OUT*/ pWrapper,
		      IUnknown*      /*IN*/  pFileObj,
		      UINT32         /*IN*/  ulBlockSize,
                      BOOL           /*IN*/  bAlignReads,
                      BOOL           /*IN*/  bCacheStats,
                      UINT32         /*IN*/  ulMaxBlockSize)
{
    m_bUseWrap2 = TRUE;
    if (pFileObj)
    {
	pWrapper = (IHXFileObject*)new FastFile(pFileObj, NULL,
            m_pContext, ulBlockSize, this, m_pFastFileDict, m_pMIIStats, 
	    bAlignReads, bCacheStats, ulMaxBlockSize, m_ulMaxMemUse);

	pWrapper->AddRef();
	return HXR_OK;
    }

    return HXR_INVALID_PARAMETER;
}

/*
 *  IUnknown methods
 */

STDMETHODIMP
FastFileFactory::QueryInterface(REFIID /*IN*/ riid, void** /*OUT*/ ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)(IHXFastFileFactory*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXFastFileFactory))
    {
        AddRef();
        *ppvObj = (IHXFastFileFactory*)this;
        return HXR_OK;
    }
    return HXR_NOINTERFACE;
}

ULONG32
FastFileFactory::AddRef()
{
    return InterlockedIncrement(&m_ulRefCount);
}

ULONG32
FastFileFactory::Release()
{
    if (InterlockedDecrement(&m_ulRefCount) > 0)
    {
	return m_ulRefCount;
    }

    delete this;
    return 0;
}


//////////////////////////////////////////////////////////////////////
// FastFile
//////////////////////////////////////////////////////////////////////

FastFile::FastFile(IUnknown*   /*IN*/ pUnk,
		   const char* /*IN*/ pURL,
                   IUnknown*   /*IN*/ pContext,
		   UINT32      /*IN*/ ulRecommendedBlockSize,
		   FastFileFactory* /*IN*/  pFastFileFactory,
		   Dict*       /*IN*/ pFastFileDict,
		   CDistMIIStatistics* /*IN*/ pMIIStats,
                   BOOL        /*IN*/ bAlignReads,
                   BOOL        /*IN*/ bCacheStats,
                   UINT32      /*IN*/ ulMaxBlockSize,
                   UINT32      /*IN*/ ulMaxMemUse)

    : m_ulRefCount (0)
    , m_pContext (pContext)
    , m_pScheduler (NULL)
    , m_pFileObject (NULL)
    , m_pFileObjectExt (NULL)
    , m_pFileStat (NULL)
    , m_pFastFileStatReport(NULL)
    , m_pFileObjectPlacementRead(NULL)
    , m_pRequestHandler (NULL)
    , m_pPool (NULL)
    , m_pPoolResponse (NULL)
    , m_pURL (NULL)
    , m_pBlockMgr (NULL)
    , m_ulDefaultChunkSize(ulRecommendedBlockSize)
    , m_ulExtraSlopSize (4 * 1024)
    , m_ulCurrentAddressSpaceUsed (0)
    , m_ulMaxRecursionLevel (5)
    , m_pFastFileResponse (NULL)
    , m_pRealResponse (NULL)
    , m_pRealFileStatResponse (NULL)
    , m_bRealFileStatPending (FALSE)
    , m_ulRealReadPos (0)
    , m_ulRealReadSize (0)
    , m_bRealReadPending (FALSE)
    , m_ulInternalReadPos (0)
    , m_bInternalReadPending (FALSE)
    , m_bInternalSeekPending (FALSE)
    , m_ulReadRecursionLevel (0)
    , m_ulFileSize (0)
    , m_ulFileCTime (0)
    , m_ulFileATime (0)
    , m_ulFileMTime (0)
    , m_ulFileMode (0)
    , m_bClosed (FALSE)
    , m_pLastBlock (NULL)
    , m_pFobCount (NULL)
    , m_pAggFastBytesRead (NULL)
    , m_pAggSlowBytesRead (NULL)
    , m_pAggInternalBytesRead (NULL)
    , m_pBlockCount (NULL)
    , m_pInUseBlockCount (NULL)
    , m_ulFastBytesRead (0)
    , m_ulSlowBytesRead (0)
    , m_pFastFileFactory(pFastFileFactory)
    , m_pFastFileDict(pFastFileDict)
    , m_bCacheStats(bCacheStats)
    , m_ulAlignmentReadAmount(0)
    , m_ulAlignmentReadOffset(0)
    , m_bAlignReads(FALSE)
    , m_pIMalloc(NULL)
    , m_pMIIStats(pMIIStats)
    , m_bReadPending(0)
    , m_bReadCancelled(0)
    , m_ulBytesSinceLastRead(0)
    , m_ulMaxBlockSize(ulMaxBlockSize)
    , m_ulMaxMemUse(ulMaxMemUse)
    , m_pMemUse(0)
    , m_pAlignmentReadBuf(0)
    , m_ulChunkSize(0)
    , m_ulMaxAddressSpaceUsed(0)
    , m_ulSectorAlignment(0)
    , m_ulStartReadPos(0)
{
    xprintf (("%p: FastFile::FastFile create pUnk=%p pURL=%s\n",
              this, pUnk, pURL ? pURL : "(null)"));

    m_bDynamicReadSizing = (ulMaxBlockSize != ulRecommendedBlockSize);

    pUnk->QueryInterface(IID_IHXFileObject, (void**)&m_pFileObject);
    pUnk->QueryInterface(IID_IHXFileObjectExt, (void **)&m_pFileObjectExt);
    pUnk->QueryInterface(IID_IHXFileStat, (void**)&m_pFileStat);
    pUnk->QueryInterface(IID_IHXRequestHandler, (void**)&m_pRequestHandler);
    pUnk->QueryInterface(IID_IHXGetFileFromSamePool, (void**)&m_pPool);
    pUnk->QueryInterface(IID_IHXFastFileStats, (void**)&m_pFastFileStatReport);

    m_ReadEnd.tv_sec = 0;
    m_ReadEnd.tv_usec = 0;
    m_ReadStart.tv_sec = 0;
    m_ReadStart.tv_usec = 0;
    m_ReadDuration = 0.0;

    if (bAlignReads &&
        pUnk->QueryInterface(IID_IHXFilePlacementRead,
                             (void**)&m_pFileObjectPlacementRead) == HXR_OK)
    {
        m_ulSectorAlignment = m_pFileObjectPlacementRead->AlignmentBoundary();
        m_bAlignReads = TRUE;
    }

    m_pFastFileFactory->AddRef();

    xassert(m_pFileObject);

    m_pContext->AddRef();
    m_pContext->QueryInterface(IID_IHXThreadSafeScheduler,
                               (void **)&m_pScheduler);
    m_pContext->QueryInterface(IID_IMalloc, (void**)&m_pIMalloc);

    FastFileStats::SetupStats(m_pContext, m_pFobCount, m_pAggFastBytesRead,
                              m_pAggSlowBytesRead, m_pAggInternalBytesRead,
                              m_pBlockCount, m_pInUseBlockCount, m_pMemUse);
    HXAtomicIncUINT32(m_pFobCount);
    xprintf (("%p: FastFile::FastFile: fob=%lu\n",
              this, *m_pFobCount));

    if (pURL)
    {
        m_pURL = new char [strlen(pURL) + 1];
        strcpy (m_pURL, pURL);
    }

    m_pFastFileResponse = new FastFileResponse (this);
    m_pFastFileResponse->AddRef();

    if (m_ulDefaultChunkSize)
    {
	m_ulChunkSize = m_ulDefaultChunkSize;
    }
    else
    {
	m_ulChunkSize = FF_BLKSIZE_DEF;
    }
}

FastFile::~FastFile()
{
    xprintf (("%p: FastFile::~FastFile delete (fob=%lu)\n",
              this, *m_pFobCount));

    if (m_pFastFileResponse)
    {
        m_pFastFileResponse->SetFastFile(NULL);
        HX_RELEASE(m_pFastFileResponse);
    }

    HX_RELEASE(m_pPoolResponse);
    HX_RELEASE(m_pRealResponse);
    HX_RELEASE(m_pRealFileStatResponse);

    HX_RELEASE(m_pPool);
    HX_RELEASE(m_pRequestHandler);
    HX_RELEASE(m_pFileStat);
    HX_RELEASE(m_pFileObjectPlacementRead);

    HX_RELEASE(m_pFastFileStatReport);

    HX_RELEASE(m_pFileObject);
    HX_RELEASE(m_pFileObjectExt);

    HX_RELEASE(m_pScheduler);
    HX_RELEASE(m_pLastBlock);
    HX_RELEASE(m_pBlockMgr);
    HX_RELEASE(m_pContext);

    HX_RELEASE(m_pFastFileFactory);
    HX_RELEASE(m_pIMalloc);

    if (m_pURL) delete [] m_pURL;
    m_pURL = NULL;

    HXAtomicDecUINT32(m_pFobCount);
}


    /*
     *  IUnknown methods
     */

STDMETHODIMP
FastFile::QueryInterface(REFIID /*IN*/  riid,
                         void** /*OUT*/ ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)(IHXFileObject*)this;
        return HXR_OK;
    }
    if (IsEqualIID(riid, IID_IHXFileObject))
    {
        AddRef();
	*ppvObj = (IHXFileObject*)this;
	return HXR_OK;
    }
    if (IsEqualIID(riid, IID_IHXFileStat) && m_pFileStat)
    {
        AddRef();
    	*ppvObj = (IHXFileStat*)this;
    	return HXR_OK;
    }
    if (IsEqualIID(riid, IID_IHXCallback))
    {
        AddRef();
	*ppvObj = (IHXCallback*)this;
	return HXR_OK;
    }
    if (IsEqualIID(riid, IID_IHXRequestHandler))
    {
        AddRef();
	*ppvObj = (IHXRequestHandler*)this;
	return HXR_OK;
    }
    if (IsEqualIID(riid, IID_IHXGetFileFromSamePool) && m_pPool)
    {
        AddRef();
	*ppvObj = (IHXGetFileFromSamePool*)this;
	return HXR_OK;
    }
    if (IsEqualIID(riid, IID_IHXGetFileFromSamePoolResponse) && m_pPool)
    {
        m_pFastFileResponse->AddRef();
	*ppvObj = (IHXGetFileFromSamePoolResponse*)m_pFastFileResponse;
	return HXR_OK;
    }
    return m_pFileObject->QueryInterface(riid, ppvObj);
}

ULONG32
FastFile::AddRef()
{
#if 0
    printf ("----------------------------------------------------------------------\n");
    xprintf(("%p: FastFile::AddRef refcount=%d\n", this, m_ulRefCount));
    PrintfStack();
    printf("\n");
    printf ("----------------------------------------------------------------------\n");
#endif

    return InterlockedIncrement(&m_ulRefCount);
}

ULONG32
FastFile::Release()
{
#if 0
    printf ("----------------------------------------------------------------------\n");
    xprintf(("%p: FastFile::Release refcount=%d\n", this, m_ulRefCount));
    PrintfStack();
    printf("\n");
    printf ("----------------------------------------------------------------------\n");
#endif

    if (InterlockedDecrement(&m_ulRefCount) > 0)
    {
	return m_ulRefCount;
    }

    delete this;
    return 0;
}

    /*
     *	IHXFileObject methods
     */

STDMETHODIMP
FastFile::Init(ULONG32           /*IN*/ ulFlags,
               IHXFileResponse* /*IN*/ pFileResponse)
{
    xprintf (("%p: FastFile::Init flags=0x%x pResponse=%p\n",
              this, ulFlags, pFileResponse));

    m_ulRealReadPos = 0;
    m_ulRealReadSize = 0;
    if (m_bRealReadPending && m_pRealResponse)
    {
	m_bRealReadPending = FALSE;
        m_pRealResponse->ReadDone (HXR_CANCELLED, NULL);
    }
    HX_RELEASE (m_pRealResponse);

    pFileResponse->AddRef();
    m_pRealResponse = pFileResponse;

    return m_pFileObject->Init(ulFlags, m_pFastFileResponse);
}

STDMETHODIMP
FastFile::Close(void)
{
    xprintf (("%p: FastFile::Close\n", this));

    if (!m_pRealResponse)
    {
	return HXR_UNEXPECTED;
    }

    if (m_pFastFileStatReport)
    {
        m_pFastFileStatReport->UpdateFileObjectStats(m_ulFastBytesRead, m_ulFastBytesRead + m_ulSlowBytesRead);
        HX_RELEASE(m_pFastFileStatReport);
    }
    return m_pFileObject->Close();
}

STDMETHODIMP
FastFile::Read(ULONG32 /*IN*/ ulCount)
{
    HX_RESULT ret = HXR_OK;
    xprintf (("%p: FastFile::Read %lu (recursion=%lu)\n",
             this, ulCount, m_ulReadRecursionLevel));

    if (!m_pRealResponse)
    {
	return HXR_UNEXPECTED;
    }

    if (m_bReadPending)
    {
        return HXR_UNEXPECTED;
    }

    m_bReadPending = TRUE;

    if (!m_pBlockMgr)
    {
        xprintf (("%p: FastFile::Read: getting block manager for url=%s\n",
	          this, m_pURL ? m_pURL : "(null)"));

        m_pBlockMgr = GetBlockManager(m_pURL, m_pBlockCount, m_pInUseBlockCount);
	xassert (m_pBlockMgr);
    }

    m_ulRealReadSize = ulCount;
    if (m_ulFileSize)
    {
        if (m_ulRealReadPos >= m_ulFileSize)
	{   // at EOF
            xprintf (("%p: FastFile::Read: at EOF\n", this));
            m_bReadPending = FALSE;
	    m_pRealResponse->ReadDone (HXR_FAIL, NULL);
	    return HXR_OK;
	}
        if (m_ulRealReadPos + ulCount > m_ulFileSize)
        {
            m_ulRealReadSize = m_ulFileSize - m_ulRealReadPos;
        }
    }
    else
    {
        // Call Stat first so we don't pass munged buffers into the fileformat.
        return m_pFileStat->Stat (m_pFastFileResponse);
    }

    if (m_ulReadRecursionLevel > m_ulMaxRecursionLevel)
    {
        AddRef();
        m_pScheduler->RelativeEnter (this, 0);
    }
    else
    {
        m_ulReadRecursionLevel++; 
	ret = DoRead();
        m_ulReadRecursionLevel--; 
    }
    return ret;
}

STDMETHODIMP
FastFile::Func(void)
{
    xprintf (("%p: FastFile::Func\n", this));
    DoRead();
    Release(); //corresponding AddRef() is in ::Read()
    return HXR_OK;
}

HX_RESULT
FastFile::DoRead(void)
{
    xprintf(("%p: FastFile::DoRead\n", this));

    if (!m_pRealResponse)
    {
        m_bReadPending = FALSE;
	return HXR_OK;
    }

    // on Solaris we don't allow short reads when doing DMA because the buffer
    // we pass to the filesystem's placement read below needs to be 2 byte
    // aligned, otherwise if we're aligning reads we allow short reads
    // so we can copy any data we have to the placement read buffer
    //
    // XXXtbradley: actually we're just going to disable short reads on
    // all platforms because it wastes cpu
#ifndef ALLOW_SHORT_READS
    BOOL bAllowShort = FALSE;
#else
    BOOL bAllowShort = m_bAlignReads;
#endif // ALLOW_SHORT_READS

    // stats for cdist - don't care if they're fast bytes or not.
    if (m_pMIIStats) HXAtomicAddUINT32(&m_pMIIStats->m_ulFastFileReadBytes,
				       m_ulRealReadSize);

    FastFileBuffer* pBuf = NULL;
    m_pBlockMgr->GetBuffer(m_ulRealReadPos, m_ulRealReadSize, bAllowShort,
                           pBuf, m_pLastBlock);

    ULONG32 ulRequestedReadPos = m_ulRealReadPos;
    if (pBuf)
    {
        xprintf(("%p: FastFile::Read found in blocklist: pBuf=%p size=%d\n",
	          this, pBuf, pBuf->m_ulCurrentSize));

        m_ulRealReadPos += pBuf->m_ulCurrentSize;
	*m_pAggFastBytesRead += pBuf->m_ulCurrentSize;
	m_ulFastBytesRead += pBuf->m_ulCurrentSize;
        m_ulBytesSinceLastRead += pBuf->m_ulCurrentSize;

        // if we're not aligning reads we don't need to worry about
        // a short read and therefore can just return with the buffer
        if (!m_bAlignReads || pBuf->m_ulCurrentSize >= m_ulRealReadSize)
        {
            m_bReadPending = FALSE;
            m_pRealResponse->ReadDone(HXR_OK, pBuf);
            pBuf->Release();
            return HXR_OK;
        }

        xprintf(("pBuf %x has only %u of %u bytes\n", pBuf,
                 pBuf->m_ulCurrentSize, m_ulRealReadSize));
    }

    // if we make it here we need to read
    *m_pAggSlowBytesRead += m_ulRealReadSize;
    m_ulSlowBytesRead += m_ulRealReadSize;

    //
    // use the time it took to read in the last block versus the time
    // it took the server to use all the data in the block to adjust the
    // readsize either up or down (to save memory)
    if (m_ReadEnd.tv_sec != 0)
    {
        HXTime now;
        gettimeofday(&now, 0);
        double msToUseData = (double)((now.tv_sec - m_ReadEnd.tv_sec) * 1000) + ((double)(now.tv_usec - m_ReadEnd.tv_usec)) / 1000;

        // if the time to use the data is less than the time to read then
        // we need to increase our read size
        double target = READ_RATIO * msToUseData / 100;
        if (m_ReadDuration >= target)
        {
            if (m_ulChunkSize < m_ulMaxBlockSize)
            {
                // if we're aligning, all we can do is double the size
                if (m_bAlignReads)
                {
                    m_ulChunkSize *= 2;
                }
                else
                {
                    m_ulChunkSize = (UINT32)(m_ulChunkSize *
                                             m_ReadDuration / target);
                }
            }
            if (m_ulChunkSize > m_ulMaxBlockSize)
                m_ulChunkSize = m_ulMaxBlockSize;
#ifdef VERBOSE_TIMING
            printf("%p %f %f I%u\n", this, msToUseData, m_ReadDuration, m_ulChunkSize);
            fflush(stdout);
#endif // VERBOSE_TIMING
        }
        else if (m_ulChunkSize > READ_MIN)
        {
            // if we're aligning read the duration needs to be much less
            // than the target before we'll adjust (since all we can do is
            // divide by 2
            if (m_bAlignReads)
            {
                if (m_ReadDuration < target / 2)
                    m_ulChunkSize /= 2;
            }
            else
            {
                m_ulChunkSize = (UINT32)(m_ulChunkSize *
                                         m_ReadDuration / target);
            }
            if (m_ulChunkSize < READ_MIN)
                m_ulChunkSize = READ_MIN;
#ifdef VERBOSE_TIMING
            printf("%p %f %f D%u\n",
                   this, msToUseData, m_ReadDuration, m_ulChunkSize);
            fflush(stdout);
#endif // VERBOSE_TIMING
        }
    }

    BOOL bMaxHit = FALSE;
    if (m_ulMaxMemUse)
    {
        // this isn't thread safe, but it doesn't need to be exact
        UINT32 ulLargestBlock = m_ulMaxMemUse - *m_pMemUse;
        if (ulLargestBlock < m_ulChunkSize)
        {
            if (m_bAlignReads)
                m_ulChunkSize /= 2;
            else
                m_ulChunkSize = ulLargestBlock;
         }
         bMaxHit = TRUE;
    }

    if (m_ulRealReadSize > m_ulChunkSize)
    {
        if (bMaxHit)
        {
            // if we don't have enough memory we have to fail the read
            m_pRealResponse->ReadDone(HXR_FAIL, NULL);
            return HXR_OK;
        }

        // if we're aligning reads the chunk size needs to remain a 
        // power of 2
        if (m_bAlignReads)
        {
            while (m_ulRealReadSize > m_ulChunkSize)
            {
                m_ulChunkSize *= 2;
            }
        }
        else
        {
            m_ulChunkSize = m_ulRealReadSize + m_ulExtraSlopSize;
        }
#ifdef VERBOSE_TIMING
        printf("%p A%u\n", this, m_ulChunkSize);
        fflush(stdout);
#endif // VERBOSE_TIMING
    }
    m_bRealReadPending = TRUE;

    HX_RESULT result;

    m_ulStartReadPos = m_ulRealReadPos;
    if (m_bAlignReads)
    {
        // if we need to align the start read position on a sector boundary,
        // then do so
        if (m_ulSectorAlignment)
        {
            m_ulStartReadPos &= ~(m_ulSectorAlignment - 1);
        }

        // create a read size that will leave the file in an aligned
        // position.  First determine the end read position excluding alignment
        m_ulAlignmentReadAmount = m_ulRealReadPos + m_ulRealReadSize;

        // next align the position to the end of the block and subtract the
        // start position to determine the amount that should be read
        m_ulAlignmentReadAmount = ((m_ulAlignmentReadAmount |
                                   (m_ulChunkSize - 1)) + 1) - (m_ulStartReadPos);

        xprintf(("m_ulRealReadPos = %u, m_ulRealReadSize = %u, ulAmount = %u, "
                 "finalPos = %u\n", m_ulRealReadPos, m_ulRealReadSize,
                 m_ulAlignmentReadAmount,
                 m_ulRealReadPos + m_ulAlignmentReadAmount));

#ifdef ALLOW_SHORT_READS
        if (pBuf)
        {
            // we need to decrement the amount to copy from pBuf by the
            // difference between where we want to start reading from
            // disk and where we have to start reading from disk
            LONG32 lCopyAmount = m_ulStartReadPos - ulRequestedReadPos;

            // if lCopyAmount is < 0 that means we need to start reading
            // before any data in the buffer, so we copy nothing and
            // just allocate enough to read everything
            if (lCopyAmount > 0)
            {
                m_pAlignmentReadBuf = (char*)m_pIMalloc->Alloc(
                    lCopyAmount + m_ulAlignmentReadAmount);
                // copy the data from the buffer to a new buffer which will
                // also hold the additional data we need to read from the FS
                memcpy(m_pAlignmentReadBuf, pBuf->GetBuffer(),
                       lCopyAmount);
                m_ulAlignmentReadOffset = lCopyAmount;
                m_ulRealReadPos = m_ulStartReadPos;
            }
            else
            {
                m_pAlignmentReadBuf = (char*)m_pIMalloc->Alloc(
                    m_ulAlignmentReadAmount);
                m_ulAlignmentReadOffset = 0;
                m_ulRealReadPos = ulRequestedReadPos;
            }

            pBuf->Release();
        }
        else
#else
        {

#  ifdef _LINUX
#    if 0
            // XXXtbradley : I can't get DMA working on linux (using O_DIRECT)
            // so we're sticking to using mmapped i/o for now

            // on linux we need to align the memory buffer in addition
            // to what we've done above
            m_pAlignmentReadBuf = (char*)m_pIMalloc->Alloc(
                m_ulAlignmentReadAmount + m_ulSectorAlignment);
            m_ulAlignmentReadOffset = (((size_t)m_pAlignmentReadBuf |
                ((size_t)m_ulSectorAlignment - 1)) + 1) - (size_t)m_pAlignmentReadBuf;
#    else
            m_pAlignmentReadBuf = 0;
            m_ulAlignmentReadOffset = 0;
#    endif // 0

#  else

            m_ulAlignmentReadOffset = 0;
            m_pAlignmentReadBuf =
                (char*)m_pIMalloc->Alloc(m_ulAlignmentReadAmount);

#  endif // _LINUX
        }
#endif // ALLOW_SHORT_READS

    }

    m_ulBytesSinceLastRead = 0;
    if (m_ulStartReadPos != m_ulInternalReadPos)
    {
        xprintf(("%p: FastFile::Read: internal seek to %lu\n",
	          this, m_ulRealReadPos));
        m_bInternalSeekPending = TRUE;
	result = m_pFileObject->Seek(m_ulStartReadPos, FALSE);
    }
    else
    {
        xprintf(("%p: FastFile::Read: internal read of %lu bytes\n",
	          this, m_ulChunkSize));
        m_bInternalReadPending = TRUE;

        if (m_bAlignReads)
        {

#if 0 // defined _LINUX
            BOOL bOffsetBuffer = TRUE;
#else
            BOOL bOffsetBuffer = FALSE;
#endif // _LINUX

            if (m_bDynamicReadSizing)
                gettimeofday(&m_ReadStart, 0);
            result = m_pFileObjectPlacementRead->Read(
                m_ulAlignmentReadAmount, m_ulAlignmentReadOffset,
                m_pAlignmentReadBuf, bOffsetBuffer);
        }
        else
        {
            if (m_bDynamicReadSizing)
                gettimeofday(&m_ReadStart, 0);
            result = m_pFileObject->Read(m_ulChunkSize);
        }
    }
    
    return result;
}

STDMETHODIMP
FastFile::Write(IHXBuffer* /*IN*/ pBuffer)
{
    xprintf (("%p: FastFile::Write pbuf=%p size=%lu\n",
              this, pBuffer, pBuffer ? pBuffer->GetSize() : 0));

    xassert(0); //XXX not supported yet
    return HXR_NOTIMPL;

    //if (!m_pRealResponse)
    //{
    //	return HXR_UNEXPECTED;
    //}
    //
    //return m_pFileObject->Write(pBuffer);
}

STDMETHODIMP
FastFile::Seek(ULONG32 /*IN*/ ulOffset,
               BOOL    /*IN*/ bRelative)
{
    xprintf (("%p: FastFile::Seek offset=%lu %s\n", this, ulOffset,
              bRelative ? "(relative)" : "(absolute)"));
    if (!m_pRealResponse)
    {
	return HXR_UNEXPECTED;
    }

    m_ulRealReadPos = (bRelative ? m_ulRealReadPos + ulOffset : ulOffset);
    if (m_bReadPending)
    {
        m_bReadCancelled = TRUE;
        m_bReadPending = FALSE;
        m_pRealResponse->ReadDone(HXR_CANCELLED, NULL);
    }

    m_pRealResponse->SeekDone (HXR_OK);
    return HXR_OK;
}

STDMETHODIMP
FastFile::Advise(ULONG32 /*IN*/ ulInfo)
{
    xprintf (("%p: FastFile::Advise\n", this));
    return m_pFileObject->Advise(ulInfo);
}

STDMETHODIMP
FastFile::GetFilename(REF(const char*) /*OUT*/ pFilename)
{
    HX_RESULT ret = m_pFileObject->GetFilename(pFilename);
    xprintf (("%p: FastFile::GetFilename: '%s'\n", this, pFilename));
    return ret;
}

    /*
     *	IHXFileStat methods
     */
STDMETHODIMP
FastFile::Stat (IHXFileStatResponse* /*IN*/ pResponse)
{
    xprintf (("%p: FastFile::Stat %p\n", this, pResponse));

    HX_RELEASE (m_pRealFileStatResponse);

    if (!pResponse)
    {
        return HXR_POINTER;
    }
    
    //
    // XXXtbradley it's not a good idea to cache this because
    // what if the file's changed? In fact the Stat may need to force
    // a flushing of the cache.
    //
    if (m_ulFileSize && m_bCacheStats)
    {  // already statted this file
        xprintf (("%p: FastFile::Stat already statted: size=%lu "
                  "ctime=%lu atpime=%lu mtime=%lu mode=%03o\n",
                  this, m_ulFileSize, m_ulFileCTime, m_ulFileATime,
	          m_ulFileMTime, m_ulFileMode));
        pResponse->StatDone (HXR_OK, m_ulFileSize, m_ulFileCTime,
	                     m_ulFileATime, m_ulFileMTime, m_ulFileMode);
        return HXR_OK;
    }

    m_bRealFileStatPending = TRUE;
    m_pRealFileStatResponse = pResponse;
    m_pRealFileStatResponse->AddRef();
    return m_pFileStat->Stat (m_pFastFileResponse);
}


    /*
     *	FastFileResponse callback methods
     */

HX_RESULT
FastFile::InitDone(HX_RESULT /*IN*/ status)
{
    xprintf(("%p: FastFile::InitDone %lu ref=%ld\n",
	      this, status, m_ulRefCount));
    m_ulInternalReadPos = 0;
    return m_pRealResponse ? m_pRealResponse->InitDone(status) : HXR_OK;
}

HX_RESULT
FastFile::CloseDone(HX_RESULT /*IN*/ status)
{
    HX_RESULT ret = HXR_OK;
    AddRef();
    xprintf(("%p: FastFile::CloseDone %lu\n", this, status));
    if (m_pRealResponse)
    {
        ret = m_pRealResponse->CloseDone(status);
        HX_RELEASE(m_pRealResponse);
    }
    Release();
    return ret;
}

HX_RESULT
FastFile::ReadDone(HX_RESULT   /*IN*/ status,
                   IHXBuffer* /*IN*/ pBuffer)
{
    xprintf(("%p: FastFile::ReadDone %lu, %p\n", this, status, pBuffer));
    if (m_bDynamicReadSizing)
        gettimeofday(&m_ReadEnd, 0);
    m_ReadDuration = (double)(m_ReadEnd.tv_sec - m_ReadStart.tv_sec) * 1000 + ((double)(m_ReadEnd.tv_usec - m_ReadStart.tv_usec)) / 1000;

    // if read was cancelled by a seek we'll already have called read done
    // with HXR_CANCELLED and so we don't need to do anything here.
    if (m_bReadCancelled)
    {
        m_bReadCancelled = FALSE;
        return HXR_OK;
    }

    m_bReadPending = FALSE;

    if (status == HXR_OBSOLETE_VERSION)
    {
        //
        // the file we are caching has changed in some way incompatible
        // with keeping the stream going (e.g. it has been replaced).
        //
        m_pBlockMgr->RemoveFromDictionary();

        if (m_pRealResponse)
            m_pRealResponse->ReadDone(status, NULL);

        return HXR_OK;
    }

    if (!m_pRealResponse)
    {
	return HXR_OK;
    }

    if (status != HXR_OK)
    {
        m_pRealResponse->ReadDone(status, NULL);
        return HXR_OK;
    }

    xassert(m_bInternalReadPending);
    if (m_bInternalReadPending)
    {
        m_bInternalReadPending = FALSE;
	if (pBuffer)
	{
            // if aligning reads, the offset of this buffer in the
            // file needs to take into account the copied data from
            // the old, short-read buffer
            UINT32 ulBlockOffset = m_ulInternalReadPos;
            if (m_bAlignReads)
            {
                ulBlockOffset -= m_ulAlignmentReadOffset;
                m_ulInternalReadPos += m_ulAlignmentReadOffset;
            }

	    *m_pAggInternalBytesRead += pBuffer->GetSize();

            UINT32 ulRequestedReadAmount = m_bAlignReads ?
                m_ulAlignmentReadAmount + m_ulAlignmentReadOffset
                : m_ulChunkSize;

            BOOL bShortRead = FALSE;
	    if (pBuffer->GetSize() != ulRequestedReadAmount)
	    {
                xprintf(("%p: FastFile::ReadDone: short read\n", this));
	        //short read, so eof must have been hit
	        bShortRead = TRUE;
	    }
	    m_pBlockMgr->AddBlock(ulBlockOffset, pBuffer, bShortRead, m_pMemUse);
            m_ulInternalReadPos += pBuffer->GetSize();
	}
    }

    if (m_bAlignReads) m_ulRealReadPos -= m_ulAlignmentReadOffset;

    FastFileBuffer* pBuf = NULL;
    m_pBlockMgr->GetBuffer(m_ulRealReadPos, m_ulRealReadSize,
                           FALSE, pBuf, m_pLastBlock);
    if (pBuf)
    {
        xprintf(("%p: FastFile::ReadDone: found in blocklist: pBuf=%x\n",
	         this, pBuf));
	m_ulRealReadPos += pBuf->m_ulCurrentSize;
	m_pRealResponse->ReadDone(HXR_OK, (IHXBuffer*)pBuf);
	pBuf->Release();
	return HXR_OK;
    }

    m_pRealResponse->ReadDone(HXR_FAIL, pBuf);

    return HXR_OK;
}

HX_RESULT
FastFile::WriteDone (HX_RESULT /*IN*/ status)
{
    xprintf (("%p: FastFile::WriteDone %lu\n", this, status));
    return m_pRealResponse ? m_pRealResponse->WriteDone(status) : HXR_OK;
}

HX_RESULT
FastFile::SeekDone (HX_RESULT /*IN*/ status)
{
    xprintf (("%p: FastFile::SeekDone %lu\n", this, status));

    xassert(m_bInternalSeekPending);
    if (!m_bInternalSeekPending)  return HXR_FAIL;
    
    m_bInternalSeekPending = FALSE;
    m_bInternalReadPending = TRUE;
    m_ulInternalReadPos = m_ulStartReadPos;

    if (m_bAlignReads)
    {
#if 0 // defined _LINUX
        BOOL bOffsetBuffer = TRUE;
#else
        BOOL bOffsetBuffer = FALSE;
#endif // _LINUX

        if (m_bDynamicReadSizing)
            gettimeofday(&m_ReadStart, 0);
        m_pFileObjectPlacementRead->Read(m_ulAlignmentReadAmount,
                                      m_ulAlignmentReadOffset,
                                      m_pAlignmentReadBuf, bOffsetBuffer);
    }
    else
    {
        if (m_bDynamicReadSizing)
            gettimeofday(&m_ReadStart, 0);
        m_pFileObject->Read (m_ulChunkSize);
    }

    return HXR_OK;
}

HX_RESULT
FastFile::StatDone (HX_RESULT /*IN*/ status,
                    UINT32    /*IN*/ ulSize,
                    UINT32    /*IN*/ ulCreationTime,
                    UINT32    /*IN*/ ulAccessTime,
                    UINT32    /*IN*/ ulModificationTime,
                    UINT32    /*IN*/ ulMode)
{
    HX_RESULT ret = HXR_OK;

    xprintf (("%p: FastFile::StatDone status=%lu size=%lu "
              "ctime=%lu atime=%lu mtime=%lu mode=%03o\n",
              this, status, ulSize, ulCreationTime, ulAccessTime,
	      ulModificationTime, ulMode));
    //XXXDC MII has a bug - xassert (m_bRealFileStatPending);
    if (!m_bRealFileStatPending && !m_bReadPending)
    {
        xprintf (("%p: FastFile::StatDone unexpected\n", this));
        return HXR_UNEXPECTED;
    }
    m_bRealFileStatPending = FALSE;
    if (SUCCEEDED (status))
    {
        if (!m_pBlockMgr)
        {
            xprintf (("%p: FastFile::StatDone: getting block manager for url=%s\n",
                      this, m_pURL ? m_pURL : "(null)"));
            m_pBlockMgr = GetBlockManager(m_pURL, m_pBlockCount, m_pInUseBlockCount);
            xassert (m_pBlockMgr);
        }

            
        if (m_pBlockMgr->VerifyFileStats(
            ulSize, ulCreationTime, ulModificationTime) != HXR_OK)
        {
            // the file has changed and the block manager is holding blocks
            // which may be bad.  We'll remove the block manager from the
            // dictionary so new streams won't get old cached blocks
            m_pBlockMgr->RemoveFromDictionary();
        }

        m_ulFileSize = ulSize;
        m_ulFileCTime = ulCreationTime;
        m_ulFileMTime = ulModificationTime;
        m_ulFileMode = ulMode;
    }

    if (m_pRealFileStatResponse)
    {
        IHXFileStatResponse* pResponse = m_pRealFileStatResponse;
        m_pRealFileStatResponse = NULL;
        ret = pResponse->StatDone (status, ulSize, ulCreationTime,
                                   ulAccessTime, ulModificationTime, ulMode);
        pResponse->Release();
    }
    else if (m_bReadPending)
    {
        // we fired-off a stat internally from Read since it hadn't been statted yet
        m_ulReadRecursionLevel++; 
        ret = DoRead();
        m_ulReadRecursionLevel--; 
    }

    return ret;
}

    /*
     *  IHXRequestHandler methods
     */

// There are two types of URLs we care about.  One, when
// we're first being initialized, m_pURL may be null,
// so we set it.  The second type are those that we're
// passed when this file is created from a pool.
// In this case, the URL will be a relative URL with
// no path information ( /a/b/c/d.rm + e.rm => /a/b/c/e.rm )

/* XXXJC : there are other cases! One is /a/b/c.rm + d/e.rm, this failed with
   the original code because it did not allow "/" in the relative url at all.
   Another is "/a/b/c.rm + /d/e.rm" where the URL passed to SetRequest is not 
   a relative url. Since Fastfile only uses the url to share blocks and the 
   block caching is way more critical than the sharing aspect of it, especially
   for relatively small files, it is a minor issue if there is an occasional 
   wierd rp that does not share a block it could have shared and a major bug 
   if Fastfile stops an rp or something from playing. Thus I don't worry about
   legality of the url created here, I am just trying to ensure that
   this routine never fails and that the url is unique so that we don't grab the
   wrong blocks.
 */

STDMETHODIMP
FastFile::SetRequest (IHXRequest* /*IN*/ pRequest)
{
    const char* pURL = NULL;
    pRequest->GetURL (pURL);
    int nLen = pURL ? strlen(pURL) : 0;
    int nOldLen = m_pURL ? strlen(m_pURL) : 0;
    BOOL bIgnore = FALSE;

    xprintf(("%p: FastFile::SetRequest url=%s old url %s\n",
             this, pURL ? pURL : "(null)", m_pURL ? m_pURL : "(null)"));

    if (!m_pURL && nLen)
    {
        m_pURL = new char [nLen + 1];
	memcpy (m_pURL, pURL, nLen + 1);
        xprintf (("%p: FastFile::SetRequest newurl=%s\n",
                 this, m_pURL ? m_pURL : "(null)"));
        bIgnore = TRUE;
    }
    else if (!pURL) 
    {
        bIgnore = TRUE;
    }

    if (!bIgnore && m_pURL && strcmp (m_pURL + nOldLen - nLen, pURL) == 0)
    {
       bIgnore = TRUE;
    }

    int nSlashPos = 0;
    if (!bIgnore && m_pURL)
    {
        nSlashPos = nOldLen - 1;
	while (nSlashPos >= 0 && m_pURL[nSlashPos] != '/')
	{
            --nSlashPos;
	}

	if (nSlashPos < 0) 
	{
            bIgnore = TRUE;
	}
    }

    if (!bIgnore)
    {
        char* pNewURL = new char [ (nSlashPos+1) + nLen + 1 ];
	if (m_pURL)
	{
	    memcpy (pNewURL, m_pURL, nSlashPos+1);
            delete [] m_pURL;
	}
	memcpy (pNewURL + nSlashPos+1, pURL, nLen+1);
	m_pURL = pNewURL;
        xprintf (("%p: FastFile::SetRequest newurl=%s\n",
                 this, m_pURL ? m_pURL : "(null)"));
    }

    return m_pRequestHandler->SetRequest (pRequest);
}

STDMETHODIMP
FastFile::GetRequest (REF(IHXRequest*) /*OUT*/ pRequest)
{
    xprintf (("%p: FastFile::GetRequest\n", this));
    return m_pRequestHandler->GetRequest (pRequest);
}


    /*
     * IHXGetFileFromSamePool methods
     */

STDMETHODIMP
FastFile::GetFileObjectFromPool (IHXGetFileFromSamePoolResponse* /*IN*/ pResp)
{
    xprintf (("%p: FastFile::GetFileObjectFromPool\n", this));

    if (!pResp)
    {
        xassert(0);
        return HXR_POINTER;
    }

    xassert(m_pPool);
    xassert(!m_pPoolResponse);
    HX_RELEASE(m_pPoolResponse);
    m_pPoolResponse = pResp;
    m_pPoolResponse->AddRef();
    return m_pPool->GetFileObjectFromPool(m_pFastFileResponse);
}


/*
 * FastFileResponse callback methods
 */

HX_RESULT
FastFile::FileObjectReady (HX_RESULT /*IN*/ status,
                           IUnknown* /*IN*/ pUnk)
{
    xprintf (("%p: FastFile::FileObjectReady\n", this));
    HX_RESULT ret = HXR_OK;

    IUnknown* pWrapper = NULL;

    // The routine that called us should be holding a ref on pUnk,
    // so we needn't add one of our own.  Wrap() returns a ref
    // on pWrapper that we must release, though.

    if (SUCCEEDED (status))
    {
        if (m_pFastFileFactory->m_bUseWrap2)
            m_pFastFileFactory->Wrap(pWrapper, pUnk, m_ulDefaultChunkSize, m_bAlignReads, m_bCacheStats, m_ulMaxBlockSize);
        else
            m_pFastFileFactory->Wrap(pWrapper, pUnk, m_ulDefaultChunkSize, m_bAlignReads, m_bCacheStats);
    }
    else
    {
	pWrapper = pUnk;
    }

    xassert (m_pPoolResponse);
    IHXGetFileFromSamePoolResponse* pResp = m_pPoolResponse;
    m_pPoolResponse = NULL;
    ret = pResp->FileObjectReady (status, pWrapper);
    pResp->Release();

    if (SUCCEEDED (status))
    {
        HX_RELEASE(pWrapper);
    }

    return ret;
}

/*
 * Other methods
 */
FastFileBlockManager*
FastFile::GetBlockManager (const char* /*IN*/ pURL,
			   UINT32*     /*IN*/ pBlockCount,
			   UINT32*     /*IN*/ pInUseBlockCount)
{
    IHXBuffer *pFullFilename = 0;
    const char *pDictKey = 0;

    if (m_pFileObjectExt)
    {
       if (m_pFileObjectExt->GetFullFilename(pFullFilename) == HXR_OK)
       {
           pDictKey = (const char*)(pFullFilename->GetBuffer());
       }
    }

    if (!pDictKey)
    {
       pDictKey = pURL;
    }

    xprintf(("%p: FastFile::GetBlockManager, pURL: %s - pDictKEy: %s\n", this, pURL, pDictKey));
    xassert (pDictKey);
    if (!pDictKey)
    {
        return NULL;
    }

    xassert (m_pFastFileDict);

    FastFileBlockManager* pBlockMgr = NULL;
    Dict_entry* entry = m_pFastFileDict->find (pDictKey);

    if (!entry)
    {
        pBlockMgr = new FastFileBlockManager(pDictKey, m_pContext, pBlockCount,
	                                     pInUseBlockCount, m_pFastFileDict, m_pMemUse);

        // only add the new url's block mgr to the dictionary if we're
        // under the limit.  XXXtbradley : we should pick a scheme to
        // actually remove a url and replace it with the new one.
       if (m_pFastFileDict->size() < FF_URL_DICT_LIMIT)
        {
            m_pFastFileDict->enter (pDictKey, pBlockMgr);
        }
        else if (!m_bReportedLimitReached)
        {
            // XXXtbradley : need to actually report this condition
            m_bReportedLimitReached = TRUE;
        }

        xprintf (("0xxxxxxxxx: FastFileBlockManager::Create: new mgr %p\n",
	          pBlockMgr));
    }
    else
    {
        pBlockMgr = (FastFileBlockManager*) entry->obj;
        xprintf (("0xxxxxxxxx: FastFileBlockManager::Create: found %p\n",
	          pBlockMgr));
    }
    pBlockMgr->AddRef();

    HX_RELEASE(pFullFilename); 

    return pBlockMgr;
}

//////////////////////////////////////////////////////////////////////
// FastFileResponse
//////////////////////////////////////////////////////////////////////
FastFileResponse::FastFileResponse (FastFile* /*IN*/ pFastFile)
    : m_ulRefCount (0)
    , m_pFastFile (pFastFile)
{
    xprintf (("%p: FastFileResponse::FastFileResponse create pFastFile=%p\n",
             this, pFastFile));
}

FastFileResponse::~FastFileResponse()
{
    xprintf (("%p: FastFileResponse::~FastFileResponse delete\n", this));
    m_pFastFile = NULL;
}

    /*
     * IUnknown methods
     */
STDMETHODIMP
FastFileResponse::QueryInterface(REFIID /*IN*/  riid,
                                 void** /*OUT*/ ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)(IHXFileResponse*)this;
        return HXR_OK;
    }
    if (IsEqualIID(riid, IID_IHXFileResponse))
    {
        AddRef();
	*ppvObj = (IHXFileResponse*)this;
	return HXR_OK;
    }
    if (IsEqualIID(riid, IID_IHXFileStatResponse))
    {
        AddRef();
	*ppvObj = (IHXFileStatResponse*)this;
	return HXR_OK;
    }
    if (IsEqualIID(riid, IID_IHXGetFileFromSamePoolResponse))
    {
        AddRef();
	*ppvObj = (IHXGetFileFromSamePoolResponse*)this;
	return HXR_OK;
    }
    
    return m_pFastFile->QueryInterface(riid, ppvObj);
}

ULONG32
FastFileResponse::AddRef()
{
    return InterlockedIncrement(&m_ulRefCount);
}

ULONG32
FastFileResponse::Release()
{
    if (InterlockedDecrement(&m_ulRefCount) > 0)
    {
	return m_ulRefCount;
    }

    delete this;
    return 0;
}

    /*
     * IHXFileResponse methods
     */
STDMETHODIMP
FastFileResponse::InitDone(HX_RESULT  /*IN*/ status)
{
    xprintf (("%p: FastFileResponse::InitDone status=%lu\n",
             this, status));
    return m_pFastFile ? m_pFastFile->InitDone(status) : HXR_OK;
}

STDMETHODIMP
FastFileResponse::CloseDone(HX_RESULT  /*IN*/ status)
{
    xprintf (("%p: FastFileResponse::CloseDone status=%lu\n",
             this, status));
    return m_pFastFile ? m_pFastFile->CloseDone(status) : HXR_OK;
}

STDMETHODIMP
FastFileResponse::ReadDone(HX_RESULT   /*IN*/ status,
			   IHXBuffer* /*IN*/ pBuffer)
{
    xprintf (("%p: FastFileResponse::ReadDone status=%lu ptr=%p len=%lu\n",
              this, status, pBuffer, pBuffer ? pBuffer->GetSize() : 0));

    return m_pFastFile ? m_pFastFile->ReadDone(status, pBuffer) : HXR_OK;
}

STDMETHODIMP
FastFileResponse::WriteDone(HX_RESULT  /*IN*/  status)
{
    xprintf (("%p: FastFileResponse::WriteDone status=%lu\n",
             this, status));
    return m_pFastFile ? m_pFastFile->WriteDone(status) : HXR_OK;
}

STDMETHODIMP
FastFileResponse::SeekDone(HX_RESULT   /*IN*/  status)
{
    xprintf (("%p: FastFileResponse::SeekDone %lu\n",
             this, status));
    return m_pFastFile ? m_pFastFile->SeekDone(status) : HXR_OK;
}

    /*
     * IHXFileStatResponse methods
     */
STDMETHODIMP
FastFileResponse::StatDone (HX_RESULT /*IN*/ status,
                            UINT32    /*IN*/ ulSize,
                            UINT32    /*IN*/ ulCreationTime,
                            UINT32    /*IN*/ ulAccessTime,
                            UINT32    /*IN*/ ulModificationTime,
                            UINT32    /*IN*/ ulMode)
{
    xprintf (("%p: FastFileResponse::StatDone status=%lu size=%lu "
              "ctime=%lu atime=%lu mtime=%lu mode=%03o\n",
              this, status, ulSize, ulCreationTime, ulAccessTime,
	      ulModificationTime, ulMode));
    return m_pFastFile ? m_pFastFile->StatDone (
                                          status, ulSize, ulCreationTime,
                                          ulAccessTime, ulModificationTime,
                                          ulMode) : 0;
}


    /*
     * IHXGetFileFromSamePoolResponse
     */
STDMETHODIMP
FastFileResponse::FileObjectReady (HX_RESULT /*IN*/ status,
                                   IUnknown* /*IN*/ pUnk)
{
    xprintf (("%p: FastFileResponse::FileObjectReady\n", this));
    return m_pFastFile ? m_pFastFile->FileObjectReady (status, pUnk) : HXR_OK;
}


//////////////////////////////////////////////////////////////////////
// FastFileBlock
//////////////////////////////////////////////////////////////////////
FastFileBlock::FastFileBlock(UINT32*                 /*IN*/ pBlockCount,
                             UINT32*                 /*IN*/ pInUseBlockCount,
                             IHXCommonClassFactory* /*IN*/ pClassFactory,
                             UINT32*                 /*IN*/ pMemUse)
    : m_ulRefCount (1) //NOTE that this is initted to 1!!
    , m_pData (NULL)
    , m_ulOffset (0)
    , m_ulSize (0)
    , m_bEOF(FALSE)
    , m_pBlockCount (pBlockCount)
    , m_pInUseBlockCount (pInUseBlockCount)
{
    HXAtomicIncUINT32(m_pBlockCount);
    xprintf (("%p: FastFileBlock::FastFileBlock create (blocks=%lu)\n",
              this, *m_pBlockCount));
    m_pMemUse = pMemUse;
}

FastFileBlock::~FastFileBlock()
{
    xprintf (("%p: FastFileBlock::~FastFileBlock delete (blocks=%lu)\n",
              this, *m_pBlockCount));
    if (m_pData)
    {
        //(*m_pMemAlloc) -= m_ulSize;
        m_pData->Release();
    }
    HXAtomicDecUINT32(m_pBlockCount);
    HXAtomicSubUINT32(m_pMemUse, m_ulSize);
}


//////////////////////////////////////////////////////////////////////
// FastFileBlockManager
//////////////////////////////////////////////////////////////////////

FastFileBlockManager::FastFileBlockManager(const char* /*IN*/ pURL,
                                           IUnknown*   /*IN*/ pContext,
					   UINT32*     /*IN*/ pBlockCount,
					   UINT32*     /*IN*/ pInUseBlockCount,
					   Dict*       /*IN*/ pDict,
					   UINT32*     /*IN*/ pMemUse)
    : m_ulRefCount(0)
    , m_pBlockList(NULL)
    , m_ulBlockListSize(8)
    , m_ulBlockListEnd(0)
    , m_pURL(0)
    , m_pFastAlloc(0)
    , m_pBlockCount(pBlockCount)
    , m_pInUseBlockCount(pInUseBlockCount)
    , m_pFastFileDict(pDict)
    , m_pClassFactory(NULL)
    , m_bRemoved(FALSE)
    , m_ulSize(0)
    , m_ulCreationTime(0)
    , m_ulModificationTime(0)
{
    xprintf (("%p: FastFileBlockManager::FastFileBlockManager create %s\n",
              this, pURL ? pURL : "(null)"));

    pContext->QueryInterface(IID_IHXFastAlloc, (void**)&m_pFastAlloc);
    pContext->QueryInterface(IID_IHXCommonClassFactory,
                             (void**)&m_pClassFactory);

    m_pBlockList = new FastFileBlock*[m_ulBlockListSize];
    memset((Byte*)m_pBlockList, 0, m_ulBlockListSize * sizeof(FastFileBlock*));
#ifdef USE_FAST_CACHE_MEM
    m_pBlockList[0] = new(m_pFastAlloc) FastFileBlock(m_pBlockCount,
                                                      m_pInUseBlockCount,
                                                      m_pClassFactory,
                                                      pMemUse);
#else
    m_pBlockList[0] = new FastFileBlock(m_pBlockCount,
                                        m_pClassFactory);
#endif

    m_ulBlockListEnd = 1; 
    xassert(m_pBlockList[m_ulBlockListEnd] == NULL);

    xassert(pURL);
    if (pURL)
    {
        m_pURL = new char [strlen(pURL) + 1];
        strcpy (m_pURL, pURL);
    }
}

FastFileBlockManager::~FastFileBlockManager()
{
    xprintf (("%p: FastFileBlockManager::~FastFileBlockManager delete %s\n",
	      this, m_pURL ? m_pURL : "(null)"));

    for (UINT32 i=0; i <= m_ulBlockListEnd; ++i)
    {
        if (m_pBlockList[i])
            m_pBlockList[i]->Release();
    }
    delete [] m_pBlockList;

    if (m_pURL && !m_bRemoved) m_pFastFileDict->remove (m_pURL);
    HX_RELEASE(m_pFastAlloc);
    HX_RELEASE(m_pClassFactory);
    if (m_pURL) delete[] m_pURL;
}

//
// remove the block manager from the dictionary unless already done
// Basically we just lookup the url in the dictionary and if its
// entry points to us, we remove ourself
//
HX_RESULT
FastFileBlockManager::RemoveFromDictionary()
{
    if (m_bRemoved)
        return HXR_OK;

    m_bRemoved = TRUE;

    Dict_entry* entry = m_pFastFileDict->find(m_pURL);

    // someone else has removed us?
    if (!entry)
        return HXR_OK;

    if (((FastFileBlockManager*)entry->obj) == this)
        m_pFastFileDict->remove(m_pURL);

    return HXR_OK;
}

//
// checks to see if the file stats provided match the stats
// provided initially for the file.
//
HX_RESULT
FastFileBlockManager::VerifyFileStats(size_t ulSize,
                                      UINT32 ulCreationTime,
                                      UINT32 ulModificationTime)
{
    // if this is the invocation call for this method, record
    // the passed values and return ok.
    if (m_ulCreationTime == 0)
    {
        m_ulSize = ulSize;
        m_ulCreationTime = ulCreationTime;
        m_ulModificationTime = ulModificationTime;
        return HXR_OK;
    }

    if (m_ulSize != ulSize ||
        m_ulCreationTime != ulCreationTime ||
        m_ulModificationTime != ulModificationTime)
    {
        return HXR_FAIL;
    }

    return HXR_OK;
}


// If a byterange is in one of the blocks in our list, return a buffer
// that contains the corresponding data.  Also return the buffer
// to speed up future lookups.
HX_RESULT
FastFileBlockManager::GetBuffer (UINT32              /*IN*/     ulOffset,
                                 UINT32              /*IN*/     ulCount,
                                 BOOL                /*IN*/     bShortBufferOK,
			         REF(FastFileBuffer*)/*OUT*/    pBuffer,
			         REF(FastFileBlock*) /*IN/OUT*/ pLastBlock)
{
    xprintf (("%p: FastFileBlockManager::GetBuffer "
              "offset=%lu count=%lu\n", this, ulOffset, ulCount));

    xassert (ulCount);

    UINT32 ulBlockOffset=0;
    UINT32 ulBlockSize=0;
    UINT32 ulNewCount=0;
    FastFileBlock* pBlock=0;

    if (pLastBlock)
    {
        //xprintf (("%p: FastFileBlockManager::GetBuffer "
	//          "checking last block %p (%lu/%lu)\n",
	//          this, pLastBlock, pLastBlock->m_ulOffset,
	//          pLastBlock->m_ulSize));
        ulBlockSize = pLastBlock->m_ulSize;
        ulBlockOffset = pLastBlock->m_ulOffset;
	ulNewCount = IsInBlock (ulOffset, ulCount,
	                        ulBlockOffset, ulBlockSize,
			        pLastBlock->m_bEOF);
        if (ulNewCount)
	{
	    pBlock = pLastBlock;
	}
    }

    if (!pBlock)
    {
        FastFileBlock** pBlockItem = m_pBlockList;
        while (*pBlockItem)
        {
	    if (*pBlockItem == pLastBlock)
	    {
	        pBlockItem++;
	        continue;
	    }
            //xprintf (("%p: FastFileBlockManager::GetBuffer "
	    //          "checking block %p (%lu/%lu)\n",
	    //          this, *pBlockItem, (*pBlockItem)->m_ulOffset,
	    //          (*pBlockItem)->m_ulSize));
            ulBlockSize = (*pBlockItem)->m_ulSize;
            ulBlockOffset = (*pBlockItem)->m_ulOffset;
	    ulNewCount = IsInBlock(ulOffset, ulCount,
                                   ulBlockOffset, ulBlockSize,
                                   bShortBufferOK || (*pBlockItem)->m_bEOF);
	    if (ulNewCount)
	    {
	        pBlock = *pBlockItem;
	        break;
	    }
	    pBlockItem++;
        }
    }

    if (pBlock)
    {
	ulCount = ulNewCount;
        ulBlockSize = pBlock->m_ulSize;
        ulBlockOffset = pBlock->m_ulOffset;

        xprintf(("%p: FastFileBlockManager::GetBuffer using block "
                 "%p (%lu/%lu)\n", this, pBlock, ulBlockOffset, ulBlockSize));

#ifdef USE_FAST_CACHE_MEM
	pBuffer = new(m_pFastAlloc)FastFileBuffer(1, m_pClassFactory);
#else
	pBuffer = new FastFileBuffer(1, m_pClassFactory);
#endif
	ulBlockOffset = ulOffset - ulBlockOffset;
	xassert(pBlock->m_pData);
	xassert(pBlock->m_pData->GetBuffer());
	pBuffer->SetBlock(pBlock, pBlock->m_pData->GetBuffer() + ulBlockOffset,
	                  ulCount);

        if (pLastBlock != pBlock)
	{
	    if (pLastBlock) pLastBlock->Release();
            pLastBlock = pBlock;
            pLastBlock->AddRef();
	}

        return HXR_OK;
    }

    xprintf (("%p: FastFileBlockManager::GetBuffer: not found\n", this));
    return HXR_FAIL;
}



UINT32
FastFileBlockManager::IsInBlock (UINT32 /*IN*/ ulOffset,
                                 UINT32 /*IN*/ ulCount,
                                 UINT32 /*IN*/ ulBlockOffset,
                                 UINT32 /*IN*/ ulBlockSize,
				 BOOL   /*IN*/ bShortBufferOK)
{
    UINT32 ulFoundSize = 0;

    if (ulBlockSize == 0 || ulOffset < ulBlockOffset)
    {
	return 0;
    }
    if (ulOffset + ulCount > ulBlockOffset + ulBlockSize)
    {
	if (!bShortBufferOK || ulOffset > ulBlockOffset + ulBlockSize)
	{
	    return 0;
	}

	UINT32 ulNewCount = ulBlockSize - (ulOffset - ulBlockOffset);
	if (ulNewCount == 0 || ulNewCount > ulCount)
	{
	    return 0;
	}
	return ulNewCount;
    }

    return ulCount;
}



HX_RESULT
FastFileBlockManager::AddBlock (UINT32      /*IN*/ ulOffset,
                                IHXBuffer* /*IN*/ pBuffer,
                                BOOL        /*IN*/ bEOF,
                                UINT32*     /*OUT*/pMemUse)
{
    xprintf (("%p: FastFileBlockManager::AddBlock off=%lu len=%lu pbuf=%p\n",
             this, ulOffset, pBuffer ? pBuffer->GetSize() : 0, pBuffer));
    xassert (pBuffer);
    if (!pBuffer)
    {
        return HXR_POINTER;
    }

    FastFileBlock** pBlock = m_pBlockList;
    while (*pBlock && (*pBlock)->GetRefCount() > 1)
    {
	++pBlock;
    }

    if (*pBlock == NULL)
    {
	if (m_ulBlockListEnd >= m_ulBlockListSize-1)
	{   // list is full so grow it
            xprintf (("%p: FastFileBlockManager::AddBlock: "
	              "block list full, growing to size %d\n",
	              this, m_ulBlockListSize * 2));
	    FastFileBlock** pNewBlockList = new
	        FastFileBlock*[m_ulBlockListSize * 2];
	    memcpy (pNewBlockList, m_pBlockList,
	            m_ulBlockListSize * sizeof(FastFileBlock*));
	    memset (pNewBlockList + m_ulBlockListSize, 0,
	            m_ulBlockListSize * sizeof(FastFileBlock*));
	    delete [] m_pBlockList;

            m_pBlockList = pNewBlockList;
	    m_ulBlockListSize *= 2;
	    return AddBlock (ulOffset, pBuffer, bEOF, pMemUse);
	}
        m_ulBlockListEnd++;
	xassert (m_ulBlockListEnd < m_ulBlockListSize);
        xassert (m_pBlockList[m_ulBlockListEnd] == NULL);
#ifdef USE_FAST_CACHE_MEM
        *pBlock = new(m_pFastAlloc) FastFileBlock(m_pBlockCount,
                                                  m_pInUseBlockCount,
	                                          m_pClassFactory,
                                                  pMemUse);
#else
        *pBlock = new FastFileBlock(m_pBlockCount, m_pInUseBlockCount,
	                            m_pClassFactory);
#endif

        xprintf (("%p: FastFileBlockManager::AddBlock: "
	          "allocated new block %p\n", this, *pBlock));
    }
    else
    {
        xprintf (("%p: FastFileBlockManager::AddBlock: "
	          "reusing block %p\n", this, *pBlock));
        if ((*pBlock)->m_pData)
	{
            //(*m_pMemAlloc) -= (*pBlock)->m_ulSize;
            (*pBlock)->m_pData->Release();
	}
    }

    pBuffer->AddRef();
    (*pBlock)->m_pData = pBuffer;
    (*pBlock)->m_ulSize = pBuffer->GetSize();
    (*pBlock)->m_ulOffset = ulOffset;
    (*pBlock)->m_bEOF = bEOF;
    //(*m_pMemAlloc) += (*pBlock)->m_ulSize;
    HXAtomicAddUINT32(pMemUse, (*pBlock)->m_ulSize);
    
    return HXR_OK;
}


//////////////////////////////////////////////////////////////////////
// FastFileBuffer
//////////////////////////////////////////////////////////////////////
FastFileBuffer::FastFileBuffer(UINT32                  /*IN*/ ulInitialRefCount,
                               IHXCommonClassFactory* /*IN*/ pClassFactory)
    : m_ulRefCount (ulInitialRefCount)
    , m_pBlock (NULL)
    , m_pData (NULL)
    , m_ulCurrentSize (0)
    , m_bPrivateData (FALSE)
{
    //(*m_pBufferCount)++;
    xprintf (("%p: FastFileBuffer::FastFileBuffer create\n", this));
}

FastFileBuffer::~FastFileBuffer(void)
{
    xprintf (("%p: FastFileBuffer::~FastFileBuffer delete\n", this));
    if (m_bPrivateData)
    {
	delete [] m_pData;
    }
    HX_RELEASE (m_pBlock);
    //(*m_pBufferCount)--;
}

    /*
     *  IUnknown methods
     */
STDMETHODIMP_(ULONG32)
FastFileBuffer::AddRef(void)
{
    return InterlockedIncrement(&m_ulRefCount);
}

STDMETHODIMP_(ULONG32)
FastFileBuffer::Release(void)
{
    xassert (m_ulRefCount > 0);
    if (InterlockedDecrement(&m_ulRefCount) > 0)
    {
        return m_ulRefCount;
    }
    delete this;
    return 0;
}

STDMETHODIMP
FastFileBuffer::QueryInterface (REFIID /*IN*/  riid,
                                void** /*OUT*/ ppvObj)
{
    if (IsEqualIID(IID_IUnknown, riid))
    {
        AddRef();
        *ppvObj = (IUnknown*)(IHXBuffer*)this;
        return HXR_OK;
    }

    if (IsEqualIID(IID_IHXBuffer, riid))
    {
        AddRef();
        *ppvObj = (IHXBuffer*)this;
        return HXR_OK;
    }

    *ppvObj = 0;
    return HXR_NOINTERFACE;
}


    /*
     *  IHXBuffer methods
     */
STDMETHODIMP
FastFileBuffer::Get (REF(UCHAR*)  /*OUT*/ pData,
                     REF(ULONG32) /*OUT*/ ulLength)
{
    pData = m_pData;
    ulLength = m_ulCurrentSize;
    return HXR_OK;
}

STDMETHODIMP
FastFileBuffer::Set (const UCHAR* /*IN*/  pData,
                     ULONG32      /*IN*/  ulLength)
{
    xprintf (("%p: FastFileBuffer::Set\n", this));
    return HXR_NOTIMPL;
}

STDMETHODIMP
FastFileBuffer::SetSize (ULONG32  /*IN*/  ulLength)
{
    xprintf (("%p: FastFileBuffer::SetSize\n", this));
    return HXR_NOTIMPL;
}

STDMETHODIMP_(ULONG32)
FastFileBuffer::GetSize(void)
{
    return m_ulCurrentSize;
}

STDMETHODIMP_(UCHAR*)
FastFileBuffer::GetBuffer(void)
{
    return (UCHAR*)m_pData;
}


    /*
     *  other methods
     */

HX_RESULT
FastFileBuffer::SetBlock (FastFileBlock* /*IN*/ pBlock,
                          Byte*          /*IN*/ pData,
                          UINT32         /*IN*/ ulDataSize)
{
    xprintf (("%p: FastFileBuffer::SetBlock pBlock=%p pData=%p len=%lu\n",
              this, pBlock, pData, ulDataSize));

    xassert (m_ulRefCount == 1);
    xassert (pBlock);
    xassert (ulDataSize);

    if (pBlock != m_pBlock)
    {
	HX_RELEASE (m_pBlock);
	m_pBlock = pBlock;
	m_pBlock->AddRef();
    }

//XXX need to copy if data is not aligned properly?
#if COPY_BUFFERS
    xassert(0);
    m_pData = new Byte [ulDataSize];
    memcpy (m_pData, pData, ulDataSize);
    m_bPrivateData = TRUE;
#else
    m_pData = pData;
    m_bPrivateData = FALSE;
#endif

    m_ulCurrentSize = ulDataSize;

    return HXR_OK;
}


//////////////////////////////////////////////////////////////////////
// FastFileStats
//////////////////////////////////////////////////////////////////////
HX_RESULT
FastFileStats::SetupStats (IUnknown*     /*IN*/   pContext,
                           REF(UINT32*)  /*OUT*/  pFobCount,
                           REF(UINT32*)  /*OUT*/  pFastBytesRead,
                           REF(UINT32*)  /*OUT*/  pSlowBytesRead,
                           REF(UINT32*)  /*OUT*/  pInternalBytesRead,
                           REF(UINT32*)  /*OUT*/  pBlockCount,
                           REF(UINT32*)  /*OUT*/  pInUseBlockCount,
                           REF(UINT32*)  /*OUT*/  pMemUse)
{
    IHXBuffer* pBuf = NULL;
    IHXRegistry* pRegistry = NULL;
    FastFileStats** ppStats = NULL;
    FastFileStats* pStats = NULL;

    xassert (pContext);
    pContext->AddRef();
    pContext->QueryInterface(IID_IHXRegistry, (void**)&pRegistry);

    if (HXR_OK != pRegistry->GetBufByName(FAST_FILE_STATS, pBuf))
    {
	IMalloc* pMalloc = NULL;
        IHXCommonClassFactory* pClassFactory = NULL;
	pContext->QueryInterface (IID_IMalloc, (void**)&pMalloc);
        pContext->QueryInterface(IID_IHXCommonClassFactory,
			         (void**)&pClassFactory);

	// Allocate+initialize a structure stored in shared memory
	// which contains pointers to the shared counters.
        ppStats = new FastFileStats*;
        *ppStats = (FastFileStats*) pMalloc->Alloc (sizeof(FastFileStats));
	memset (*ppStats, 0, sizeof(FastFileStats));

	// Stuff it in the registry
	pClassFactory->CreateInstance (CLSID_IHXBuffer, (void**)&pBuf);
	pBuf->SetSize(sizeof(FastFileStats*));
	memcpy ((UCHAR*)pBuf->GetBuffer(), (UCHAR*)ppStats, sizeof(FastFileStats*));
	pRegistry->AddBuf (FAST_FILE_STATS, pBuf);

	HX_RELEASE (pMalloc);
        HX_RELEASE (pBuf);
	pStats = *ppStats;
    }
    else
    {
	// We found the structure, so use it....
	UCHAR* buf = pBuf->GetBuffer();
        memcpy (&pStats, buf, sizeof(FastFileStats*));
    }

    xassert (pStats);

    if (pStats)
    {
        pFobCount		= &(pStats->m_ulFobCount);
        pFastBytesRead		= &(pStats->m_ulFastBytesRead);
        pSlowBytesRead		= &(pStats->m_ulSlowBytesRead);
        pInternalBytesRead	= &(pStats->m_ulInternalBytesRead);
        pBlockCount		= &(pStats->m_ulBlockCount);
        pInUseBlockCount	= &(pStats->m_ulInUseBlockCount);
        pMemUse         	= &(pStats->m_ulMemUse);
    }
    else
    {
        pFobCount		= NULL;
        pFastBytesRead		= NULL;
        pSlowBytesRead		= NULL;
        pInternalBytesRead	= NULL;
        pBlockCount		= NULL;
        pMemUse  		= NULL;
    }

    return HXR_OK;
}




// DEPRICATED -- Use GetStats()
// Format current FastFile stats, ready for printing
char*
FastFileStats::GetFormattedStats (IUnknown* pContext)
{
    IHXBuffer* pPointerBuffer=0;
    IHXRegistry* pRegistry=0;
    static char szBuf[256];
    char* p=szBuf;
    *p = '\0';

    pContext->QueryInterface(IID_IHXRegistry, (void **)&pRegistry);
    if (SUCCEEDED (pRegistry->GetBufByName(FAST_FILE_STATS, pPointerBuffer)))
    {
        HX_RESULT theErr = HXR_OK;
        FastFileStats* pStats = *((FastFileStats**)pPointerBuffer->GetBuffer());
        HX_ASSERT (pStats);

        UINT32 ulFFTotal =
            pStats->m_ulFastBytesRead + pStats->m_ulSlowBytesRead;
        double fFFEffect = ulFFTotal ?
        (100.0 * pStats->m_ulFastBytesRead / ulFFTotal) : 0.0;

        p += sprintf(p, "    FastFile: %0.2f%% Effective, "
                        "%lu Blocks (%lu), %lu Fobs\n",
                        fFFEffect,
                        pStats->m_ulBlockCount,
                        pStats->m_ulInUseBlockCount,
                        pStats->m_ulFobCount);

        p += sprintf(p, "    FastFile: Bytes: %lu Fast, "
                        "%lu Slow, %lu Internal %lu Total\n",
                        pStats->m_ulFastBytesRead,
                        pStats->m_ulSlowBytesRead,
                        pStats->m_ulInternalBytesRead,
                        pStats->m_ulMemUse);

        pStats->m_ulFastBytesRead = 0;
        pStats->m_ulSlowBytesRead = 0;
        pStats->m_ulInternalBytesRead = 0;

        pPointerBuffer->Release();
    }

    HX_RELEASE (pRegistry);

    return szBuf;
}

// Fetch FastFile stats
HX_RESULT
FastFileStats::GetStats (IUnknown* pContext,
                         double* pFFEffect,
                         UINT32* pBlockCount,
                         UINT32* pInUseBlockCount,
                         UINT32* pFobCount,
                         UINT32* pFastBytesRead,
                         UINT32* pSlowBytesRead,
                         UINT32* pInternalBytesRead,
                         UINT32* pMemUse)
{
    IHXBuffer* pPointerBuffer=0;
    IHXRegistry* pRegistry=0;

    pContext->QueryInterface(IID_IHXRegistry, (void **)&pRegistry);
    if (SUCCEEDED (pRegistry->GetBufByName(FAST_FILE_STATS, pPointerBuffer)))
    {
        HX_RESULT theErr = HXR_OK;
        FastFileStats* pStats = *((FastFileStats**)pPointerBuffer->GetBuffer());
        HX_ASSERT (pStats);

        UINT32 ulFFTotal = pStats->m_ulFastBytesRead + pStats->m_ulSlowBytesRead;

        *pFFEffect = ulFFTotal ? (100.0 * pStats->m_ulFastBytesRead / ulFFTotal) : 0.0;
        *pBlockCount = pStats->m_ulBlockCount;
        *pInUseBlockCount = pStats->m_ulInUseBlockCount;
        *pFobCount = pStats->m_ulFobCount;
        *pFastBytesRead = pStats->m_ulFastBytesRead;
        *pSlowBytesRead = pStats->m_ulSlowBytesRead;
        *pInternalBytesRead = pStats->m_ulInternalBytesRead;
        *pMemUse = pStats->m_ulMemUse;

        pPointerBuffer->Release();

        return HXR_OK;
    }

    HX_RELEASE (pRegistry);

    return HXR_FAILED;
}
