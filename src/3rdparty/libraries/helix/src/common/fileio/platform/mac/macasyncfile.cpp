/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: macasyncfile.cpp,v 1.8 2006/02/16 23:03:01 ping Exp $
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

/*

   Macintosh File buffer

   This is a class which pretends to be a file, yet that reads from the file asynchronously.

   It makes the file async calls which read the file, the callback of which spews into a 
   buffer. This is done by using a timer callback which gets called every 1/2 second to 
   spew more data into the buffer. 

   The global list is meant to be used for storage of all the different file objects
   which will get processed each time through the loop.  

   This will significantly enhance the Macintosh when it comes to local playback of files. 
 */

#include "macasyncfile.h"
#include "hxmm.h"
#include "hxslist.h"
#include "hxcore.h"
#include "hxbuffer.h"
#include "hxstrutl.h"

//#include "../dcondev/dcon.h"

//#define _LOG_DATA	1

// ULONG32 HXAsyncQueueBuffer::msAllocatedBufferCount = 0;

//#define LOG_MULTIPLE_DEFERRED_TASKS 1

#if defined(_DEBUG) && defined (_LOG_DATA)
#define DEBUGSTR(x)	DebugStr(x)
#else
#define DEBUGSTR(x)
#endif

CMacAsyncFile::CMacAsyncFile(IUnknown* pContext) :
     mReadQueue(NULL)
    ,mSeekFromWhere(0)
    ,mSeekPos(0)
    ,mFilePos(0)
    ,m_ulReadPositionInFile(0)
    ,m_pResponse(NULL)
    ,m_bFileDone(FALSE)
    ,m_bReadPending(FALSE)
    ,m_bSeekPending(FALSE)
    ,m_ulPendingReadCount(FALSE)
    ,mAsyncQueue(NULL)
    ,mOutStandingCallbacks(0)
    ,m_bSettingSeekState(FALSE)    
    ,m_bCheckFromPQ(FALSE)
    ,m_bInternalSeekNeeded(FALSE)
    ,m_pPendingCallbackList(NULL)
    ,m_pTimedPQList(NULL)
    ,m_pLocationPQList(NULL)
    ,m_ulTotalPQSize(0)
    ,m_bDeferredTaskPending(FALSE)
    ,m_bIsQuitting(FALSE)
    ,m_bInEnqueueAsyncBuffers(FALSE)
    ,m_bPendingAsyncSeekCompleted(FALSE)
    ,m_bAllCallbacksCompleted(FALSE)
    ,m_bInProcessPendingCallbacks(FALSE)
    ,m_pMutex(NULL)
    ,m_lRefCount(0)
    ,m_pContext(pContext)
{
    HX_ADDREF(m_pContext);

    mAsyncQueue = new CHXSimpleList();
    m_pPendingCallbackList = new CHXSimpleList;
    m_pTimedPQList	      = new CHXSimpleList;
    m_pLocationPQList	      = new CHXSimpleList;
    mReadQueue	= new CBigByteQueue(kMacAsyncBuffer, 1);
    
    m_DeferredTaskStruct.dtReserved = 0;
    m_DeferredTaskStruct.dtFlags = 0;
#ifdef _CARBON
    m_DeferredTaskStruct.dtAddr = NewDeferredTaskUPP(CMacAsyncFile::DeferredTaskProc);
#else
    m_DeferredTaskStruct.dtAddr = NewDeferredTaskProc(CMacAsyncFile::DeferredTaskProc);
#endif
    m_DeferredTaskStruct.dtParam = (long) this; 	
    m_DeferredTaskStruct.qType = dtQType;
    
    m_uNumDeferredTask = 0;

    CreateInstanceCCF(CLSID_IHXMutex, (void**)&m_pMutex, m_pContext);
    HX_ASSERT(m_pMutex);
#endif // defined(THREADS_SUPPORTED) 
}

CMacAsyncFile::~CMacAsyncFile()
{
    if (m_DeferredTaskStruct.dtAddr != NULL)
    {
#ifdef _CARBON
	DisposeDeferredTaskUPP(m_DeferredTaskStruct.dtAddr);
#else
	DisposeRoutineDescriptor(m_DeferredTaskStruct.dtAddr);
#endif
	m_DeferredTaskStruct.dtAddr = NULL;
    }    

    Close();

    HX_DELETE(mAsyncQueue);
    HX_DELETE(m_pPendingCallbackList);
    HX_DELETE(m_pTimedPQList);
    HX_DELETE(m_pLocationPQList);
    HX_DELETE(mReadQueue);
#if defined(_DEBUG) && defined(LOG_MULTIPLE_DEFERRED_TASKS)
    HXBOOL bWaitedForDeferred = FALSE;
    if ( m_bDeferredTaskPending )
    {
	bWaitedForDeferred = TRUE;
	DebugStr("\pCMacAsyncFile dtor -- a deferred task is pending!;g");
    }
#endif
    
    UINT32 timeout = TickCount() + 300L;
    m_bIsQuitting = TRUE;
    while ( m_bDeferredTaskPending && timeout - TickCount() > 0 )
    {
	// sit-n-spin, awaiting completion of callbacks.
    }
    
#if defined(_DEBUG) && defined(LOG_MULTIPLE_DEFERRED_TASKS)
    if ( bWaitedForDeferred )
    {
	if ( m_bDeferredTaskPending )
	{
	    DebugStr("\pdeferred task STILL pending! This is gonna hurt;g");
	}
	else
	{
	    DebugStr( "\pDeferred tasks were successfully purged;g" );
	}
    }
#endif

    HX_RELEASE(m_pMutex);
    HX_RELEASE(m_pContext);
}

STDMETHODIMP_(ULONG32) CMacAsyncFile::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32) CMacAsyncFile::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
	return m_lRefCount;
    }
    
    delete this;
    return 0;
}

void CMacAsyncFile::EmptyAsyncQueue()
{    
    m_pMutex->Lock();

    while(mAsyncQueue && mAsyncQueue->GetCount() > 0)
    {
        HXAsyncQueueBuffer* x = (HXAsyncQueueBuffer*) mAsyncQueue->RemoveHead();
        
        HX_DELETE(x);
    }
    
    while(m_pTimedPQList && m_pTimedPQList->GetCount() > 0)
    {
        HXAsyncQueueBuffer* x = (HXAsyncQueueBuffer*) m_pTimedPQList->RemoveHead();
        
        HX_DELETE(x);
    }  
    
    if (m_pLocationPQList)
    {
        m_pLocationPQList->RemoveAll();
    }
    
    m_ulTotalPQSize = 0;
    
    while(m_pPendingCallbackList && m_pPendingCallbackList->GetCount() > 0)
    {
        HXParamBlockRec* pb = (HXParamBlockRec*) m_pPendingCallbackList->RemoveHead();
        
        HX_DELETE(pb);
    }

    m_pMutex->Unlock();
}

void
CMacAsyncFile::DeleteAsyncQueueBuffer(HXAsyncQueueBuffer* x, LISTPOSITION pos)
{
    m_pMutex->Lock();

    HXBOOL bInserted;
    
    mAsyncQueue->RemoveAt(pos);
    
    /* we only keep full buffers around*/
    if (x->state != AB_STATE_FULL)
    {
        goto discard;
    }
    
    x->current_size    = x->size;
    x->current_buf_ptr = x->buffer;

    // insert in location list
    pos = m_pLocationPQList->GetTailPosition();
    bInserted = FALSE;
    while (pos)
    {
         HXAsyncQueueBuffer* pNode  = (HXAsyncQueueBuffer*) m_pLocationPQList->GetAt(pos);
         if (x->position_in_file >= pNode->position_in_file+pNode->size)
         {
             m_pLocationPQList->InsertAfter(pos, x);
             bInserted = TRUE;
             break;
         }
         
         m_pLocationPQList->GetPrev(pos);
    }
    
    if (!bInserted)
    {
        m_pLocationPQList->AddHead(x);
    }
    
    m_pTimedPQList->AddTail(x);
    m_ulTotalPQSize += x->size;
    
    //dfprintf("mem","PQ Add: %p %lu, %lu, %lu\n", this, x->position_in_file, x->size, m_ulTotalPQSize);
    // time to expire some -- keep a max of 4*48K in memory?
    if (m_ulTotalPQSize > 4*kMacAsyncBuffer)
    {
        x = (HXAsyncQueueBuffer*) m_pTimedPQList->RemoveHead();
        
        // remove from location list
        pos = m_pLocationPQList->Find((void*)x);
        HX_ASSERT(pos != NULL);
        m_pLocationPQList->RemoveAt(pos);
        
        m_ulTotalPQSize -= x->size;
        //dfprintf("mem", "PQ Remove: %lu, %lu, %lu\n", x->position_in_file, x->size, m_ulTotalPQSize);
    }
    else
    {
        // early exit
        m_pMutex->Unlock();
        return;
    }
discard:    
    delete x;

    m_pMutex->Unlock();
}


HXBOOL
CMacAsyncFile::FillBufferFromPQ(UINT32 ulFillPosition)
{
    m_pMutex->Lock();

    HXBOOL bFilled = FALSE;
    
    LISTPOSITION pos = m_pLocationPQList->GetHeadPosition();
    UINT32 ulEmptyCount = mReadQueue->GetAvailableElements();
    UINT32 ulOrigFillPosition = ulFillPosition;
    while(pos && ulEmptyCount > 0)
    {
        HXAsyncQueueBuffer* x = (HXAsyncQueueBuffer*) m_pLocationPQList->GetAt(pos);
        
        // find the starting byte
        if (x->position_in_file <= ulFillPosition && 
            x->position_in_file+x->size > ulFillPosition)
        {
            UINT32 ulBytesToCopy = x->position_in_file+x->size - ulFillPosition;
            if (ulBytesToCopy > ulEmptyCount)
            {
                ulBytesToCopy = ulEmptyCount;
            }
            
            HX_ASSERT(ulBytesToCopy > 0);
            HX_VERIFY(mReadQueue->EnQueue(x->buffer+ulFillPosition-x->position_in_file, 
            				ulBytesToCopy) == ulBytesToCopy);
            ulEmptyCount -= ulBytesToCopy;
            bFilled = TRUE;
            ulFillPosition += ulBytesToCopy;
            LISTPOSITION timedPos = m_pTimedPQList->Find((void*) x);
            HX_ASSERT(timedPos != NULL);
            m_pTimedPQList->RemoveAt(timedPos);
            m_pTimedPQList->AddTail(x);
            
            m_ulReadPositionInFile = ulFillPosition;
            
    //dfprintf("mem","FillNoSeek: %lu, %lu, %lu %lu\n", x->position_in_file, x->size, ulOrigFillPosition, ulEmptyCount);
        }
        else if (x->position_in_file > ulFillPosition)
        {
            break;
        }
        
        (void) m_pLocationPQList->GetNext(pos);
    }
    
    // continue to read data form stores buffers if we were succesfully able
    // to fill buffers this time
    m_bCheckFromPQ = bFilled;

    m_pMutex->Unlock();
    return bFilled;
}

//
//      Pump as many of the Asynchronous buffers into the mReadQueue
//      as is possible.
//
void CMacAsyncFile::EnqueueAsyncBuffers()
{
    LISTPOSITION pos = NULL;
    
    m_pMutex->Lock();
    if (m_bInEnqueueAsyncBuffers)
    {
        m_pMutex->Unlock();
        return;
    }
    
    m_bInEnqueueAsyncBuffers = TRUE;

    /* See if there are any pending callbacks */
    ProcessPendingCallbacks();
    
    pos = mAsyncQueue->GetHeadPosition();

    while(pos)
    {
	HXAsyncQueueBuffer *x =(HXAsyncQueueBuffer *) mAsyncQueue->GetAt(pos);
	if(x)
	{
	    if(x->state == AB_STATE_IGNORED)
	    {
		//
		//      Due to a seek this buffer has now become out of context
		//      So dispose of it and then check the next buffer.
		//
		DeleteAsyncQueueBuffer(x, pos);

		pos = mAsyncQueue->GetHeadPosition();
		continue;
	    }

	    if(x->state == AB_STATE_FULL)
	    {
	        UINT32 ulEmptyCount = mReadQueue->GetAvailableElements();
	        
	        /* This assumes that the pending count can NEVER be greater than
	         * the total size of the byte queue i.e. the size of the
	         * byte queue is the max read size we allow
	         * currently it is set to 48K (odd number eh?)
	         */
	        
	        if (m_bReadPending && ulEmptyCount > 0)
	        {
	            if (ulEmptyCount > x->current_size)
	            {
	                ulEmptyCount = x->current_size;
	            }
	            
	            HX_ASSERT(ulEmptyCount > 0);

	            // This used to have a HX_VERIFY on the result. After the
	            // most recent change, though, where the "m_bReadPending"
	            // if statement was changed, the assert started yelling.
	            // Nobody's very comfortable about forcibly ignoring the
	            // verify, but it seems to function.
	            HX_VERIFY(mReadQueue->EnQueue(x->current_buf_ptr, ulEmptyCount) == ulEmptyCount);
	            
	            x->current_size    -= ulEmptyCount;
	            x->current_buf_ptr += ulEmptyCount;
	            
	            if (x->current_size == 0)
	            {
			//dfprintf("mem","Done with buffer\n");
			DeleteAsyncQueueBuffer(x, pos);
		    }
		    else
		    {
		        break;
		    }
		}
		else
		{
		    break;
		}
	    }
	    else
	    {
		//
		//      In this case we have to break, because there isn't a buffer available.
		//
		break;
	    }
	}

	pos = mAsyncQueue->GetHeadPosition();
    }

    m_bInEnqueueAsyncBuffers = FALSE;
    m_pMutex->Unlock();
}

CHXDataFile *CMacAsyncFile::Construct()
{
    CMacAsyncFile *result = new CMacAsyncFile();
    
    result->AddRef();

    return result;
}

HX_RESULT CMacAsyncFile::Open(const char *filename, UINT16 mode, HXBOOL textflag)
{
    return HXR_NOTIMPL;
}

HX_RESULT CMacAsyncFile::SafeOpen (const char *filename, UINT16 mode, HXBOOL textflag, HXBOOL bAtInterrupt)
{
#if defined(_DEBUG) && defined (_LOG_DATA)
    char str[255]; /* Flawfinder: ignore */
   ::sprintf(str, "SafeOpen %d;g", (int) bAtInterrupt); /* Flawfinder: ignore */
    DEBUGSTR(c2pstr(str));
#endif
 
   m_pMutex->Lock();
   HX_RESULT result = CMacFile::Open(filename, mode, textflag);
   ULONG32 count = 0;

    mFilePos = 0;
    m_ulReadPositionInFile = 0;

    if(result == HXR_OK)
    {
	    ReadAsyncData(bAtInterrupt);
    }

    //
    //   Set it to null.
    //
    mFilePos = 0;
    
    m_pMutex->Unlock();
    return result;
}
 
HX_RESULT CMacAsyncFile::Close(void)
{
    m_pMutex->Lock();
    HX_RESULT result = CMacFile::Close();

    EmptyAsyncQueue();

    mReadQueue->FlushQueue();

    m_pMutex->Unlock();
    return result;

}

HX_RESULT CMacAsyncFile::Seek(ULONG32 offset, UINT16 fromWhere)
{
    return HXR_NOTIMPL;
}

void
CMacAsyncFile::PerformInternalSeek(HXBOOL ASYNC)
{
    m_pMutex->Lock();
    mSeekFromWhere = fsFromStart;
    mSeekPos = m_ulReadPositionInFile;
    UpdateFilePos(ASYNC, TRUE);
    m_pMutex->Unlock();
}

HX_RESULT
CMacAsyncFile::SafeSeek(ULONG32 offset, UINT16 fromWhere, HXBOOL bAtInterrupt)
{
#if defined(_DEBUG) && defined (_LOG_DATA)
    char str[255]; /* Flawfinder: ignore */
   ::sprintf(str, "SafeSeek: %lu %u %d;g", offset, fromWhere, (int) bAtInterrupt); /* Flawfinder: ignore */
    DEBUGSTR(c2pstr(str));
#endif

    HX_LOCK(m_pMutex);

    //dfprintf("mem","SafeSeek: %lu, %u, %d\n", offset, fromWhere, (int) bAtInterrupt);
    m_bFileDone 	= FALSE;
    m_bReadPending	= FALSE;
     
    LISTPOSITION pos = NULL;

    /* If we get a read deferred callback while setting the state for buffers in
     * async queue, ignore it
     */
    m_bSettingSeekState = TRUE;

    pos = mAsyncQueue->GetHeadPosition();

    while(pos)
    {
	HXAsyncQueueBuffer *x =(HXAsyncQueueBuffer *) mAsyncQueue->GetAt(pos);
	if(x->state == AB_STATE_FULL)
	{
	    DeleteAsyncQueueBuffer(x, pos);
	    pos = mAsyncQueue->GetHeadPosition();
	    continue;
	    //x->state = AB_STATE_IGNORED;
	}
	/* All other states are pending states and result in AB_STATE_OUTOFCONTEXT */
	else if(x->state != AB_STATE_IGNORED)
	{
	    x->state = AB_STATE_OUTOFCONTEXT;
	}

	mAsyncQueue->GetNext(pos);
    }

    m_bSettingSeekState = FALSE;

    mReadQueue->FlushQueue();

    m_bSeekPending = TRUE;
    if(BufferedWrite())
    {
	CMacFile::Seek(offset, fromWhere);
	PendingAsyncSeekDone();
    }
    else
    {
        UINT32 ulSeekPosition = 0;
        HXBOOL bSeekNeeded = FALSE;
        
	switch(fromWhere)
	{
	case SEEK_SET:
	    fromWhere = fsFromStart;
	    mSeekFromWhere = fromWhere;
	    mSeekPos = offset;
	    ulSeekPosition = m_ulReadPositionInFile = offset;
	    break;

	case SEEK_CUR:
	    fromWhere = fsFromMark;
	    mSeekFromWhere = fromWhere;
	    mSeekPos = offset;
	    ulSeekPosition = m_ulReadPositionInFile = mFilePos+offset;
	    break;

	case SEEK_END:
	    //HX_ASSERT(!"Need to add support for seek from end -- XXXRA");
	    fromWhere = fsFromLEOF;
	    mSeekFromWhere = fromWhere;
	    mSeekPos = offset;
	    bSeekNeeded = TRUE;
	    break;

	}
	
        if (!bSeekNeeded)
        {
            bSeekNeeded = !(FillBufferFromPQ(ulSeekPosition));	
        }
        
        if (bSeekNeeded)
        {
	    UpdateFilePos(bAtInterrupt);
	}
	else
	{
	    mFilePos 	   = ulSeekPosition;
	    mSeekFromWhere = 0;
            m_bInternalSeekNeeded = TRUE;
	    PendingAsyncSeekDone();
	}
    }
    DEBUGSTR("\p EXIT SafeSeek;g");
    
    HX_UNLOCK(m_pMutex);

    return HXR_OK;
}


HX_RESULT
CMacAsyncFile::SafeRead(ULONG32 count, HXBOOL bAtInterrupt)
{
    AddRef();
    m_pMutex->Lock();
    HX_ASSERT(m_bReadPending == FALSE);

    if(!mReadQueue || m_bReadPending)
    {
        DEBUGSTR("\p SafeRead UNEXPECTED;g");
        //dfprintf("mem", "SafeRead UNEXPECTED\n");
        m_pMutex->Unlock();
        Release();
	return HXR_UNEXPECTED;
    }

#if defined(_DEBUG) && defined (_LOG_DATA)
    char str[255]; /* Flawfinder: ignore */
   ::sprintf(str, "SafeRead: %lu %d;g", count, (int) bAtInterrupt); /* Flawfinder: ignore */
    DEBUGSTR(c2pstr(str));
#endif
    
    //dfprintf("mem","SafeRead: %lu, %d\n", count, (int) bAtInterrupt);
    m_bReadPending	 = TRUE;
    m_ulPendingReadCount = count;

    EnqueueAsyncBuffers();

    ReadAsyncData(bAtInterrupt);

    EnqueueAsyncBuffers();
    
    ProcessPendingRead();
    
   DEBUGSTR("\p EXIT SafeRead;g");
   
   m_pMutex->Unlock();
   Release();
   return HXR_OK;
}

ULONG32
CMacAsyncFile::SafeWrite(IHXBuffer* pBuffer, HXBOOL bAtInterrupt)
{
    m_pMutex->Lock();
    HX_ASSERT(m_bReadPending == FALSE);

    ULONG32 outWrittenSize = 0;
    if(m_bReadPending)
    {
        DEBUGSTR("\p SafeWrite UNEXPECTED;g");
        m_pMutex->Unlock();
	return outWrittenSize;
    }

#if defined(_DEBUG) && defined (_LOG_DATA)
    char str[255]; /* Flawfinder: ignore */
   ::sprintf(str, "SafeWrite: %d;g", (int) bAtInterrupt); /* Flawfinder: ignore */
    DEBUGSTR(c2pstr(str));
#endif
    
    outWrittenSize = WriteAsyncData(pBuffer, bAtInterrupt);

    DEBUGSTR("\p EXIT SafeWrite;g");
    m_pMutex->Unlock();
    return outWrittenSize;
}

ULONG32 CMacAsyncFile::Read(char *buf, ULONG32 count)
{
    return HXR_NOTIMPL;
}

//
//      Static Initializers
//

#ifdef _CARBON
IOCompletionUPP CMacAsyncFile::zmIOCallbackUPP = NewIOCompletionUPP((IOCompletionProcPtr)CMacAsyncFile::zmIOCallback);
#else
IOCompletionUPP CMacAsyncFile::zmIOCallbackUPP = NewIOCompletionProc(CMacAsyncFile::zmIOCallback);
#endif

//
//      Tell
//

ULONG32 CMacAsyncFile::Tell(void)
{
    return mFilePos;
}

//
//       This is the zmIOCompletion callback.
//       When this is done, the data is then pumped into the buffer for this file.
//
pascal void CMacAsyncFile::zmIOCallback(HXParamBlockRec * pb)
{

    HXMM_INTERRUPTON();

    /*
     *	Setup and then install a deferred task 
     */
    if (pb != NULL &&  pb->param != NULL)
    {
    
	CMacAsyncFile* fileobj = (CMacAsyncFile*) pb->param;
	fileobj->m_pMutex->Lock();
	if (fileobj->m_DeferredTaskStruct.dtAddr != NULL)
	{
#if defined(_DEBUG) && defined (_LOG_DATA)
	     char tmpStr1[255]; /* Flawfinder: ignore */
	     ::sprintf(tmpStr1, "ENTER INterrupt Callback %p ;g", fileobj); /* Flawfinder: ignore */
	     DEBUGSTR(c2pstr(tmpStr1));
#endif

	    fileobj->AddToThePendingList((void*) pb);
	    
	    fileobj->m_uNumDeferredTask++;
#if defined(_DEBUG) && defined(LOG_MULTIPLE_DEFERRED_TASKS)
	    /*
	    if (fileobj->m_uNumDeferredTask > 1)
	    {
	        char tmpStr[255];
	        ::sprintf(tmpStr, "More than one pending DeferredTask: %u;g", fileobj->m_uNumDeferredTask);
	        DebugStr(c2pstr(tmpStr));
	    }
	    */
#endif
	    
	    if (!fileobj->m_bDeferredTaskPending)
	    {
	        if ( !fileobj->m_bIsQuitting )
	        {
		    fileobj->m_bDeferredTaskPending = TRUE;
#ifdef _CARBON
		    DeferredTaskProc(fileobj->m_DeferredTaskStruct.dtParam);
#else
		    DTInstall(&fileobj->m_DeferredTaskStruct);
#endif
	        }
	    }
	}
	fileobj->m_pMutex->Unlock();
    }
    
    HXMM_INTERRUPTOFF();
}

pascal void CMacAsyncFile::DeferredTaskProc(long inParam)
{
    HXMM_INTERRUPTON();

    DEBUGSTR("\pENTER CMacAsyncFile::DeferredTaskProc;g");
    CMacAsyncFile*   fileobj = (CMacAsyncFile*) inParam;;
    if(fileobj)
    {
#if defined(_DEBUG) && defined (_LOG_DATA)
        char tmpStr[255]; /* Flawfinder: ignore */
        sprintf(tmpStr, " This pointer: %p;g", fileobj); /* Flawfinder: ignore */
        DEBUGSTR(c2pstr(tmpStr));
#endif
#if defined(_DEBUG) && defined(LOG_MULTIPLE_DEFERRED_TASKS)
	    /*
	    if (fileobj->m_uNumDeferredTask != 1)
	    {
	        char tmpStr[255];
	        ::sprintf(tmpStr, "(In DeferredTaskProc) m_uNumDeferredTask != 1: %u;g", fileobj->m_uNumDeferredTask);
	        DebugStr(c2pstr(tmpStr));
	    }
	    */
	    if ( !fileobj->m_bDeferredTaskPending )
	    {
	    	DebugStr( "\p(In DeferredTaskProc) m_bDeferredTaskPending is false!;g" );
	    }
#endif
	if ( !fileobj->m_bIsQuitting )
	{
	    fileobj->ProcessPendingCallbacks();
	}
        fileobj->m_bDeferredTaskPending = FALSE;
    }
    
    DEBUGSTR("\pLEAVE CMacAsyncFile::DeferredTaskProc;g");
    HXMM_INTERRUPTOFF();
}

void CMacAsyncFile::ProcessPendingCallbacks()
{
    m_pMutex->Lock();

    if (m_bInProcessPendingCallbacks)
    {
        m_pMutex->Unlock();
        return;
    }
 
 start:   
    m_bInProcessPendingCallbacks = TRUE;
    
    while (m_pPendingCallbackList && m_pPendingCallbackList->GetCount() > 0)
    {
        m_uNumDeferredTask--;
        HXParamBlockRec* pb = (HXParamBlockRec*) m_pPendingCallbackList->RemoveHead();
        if (pb && pb->param)
        {
            ProcessBlock(pb);
        }
        HX_DELETE(pb);
    }
    
    EnqueueAsyncBuffers();

    if (m_bPendingAsyncSeekCompleted)
    {
        m_bPendingAsyncSeekCompleted = FALSE;
	PendingAsyncSeekDone();
    }
    
    if (mOutStandingCallbacks == 0 && m_bAllCallbacksCompleted)
    {
        m_bAllCallbacksCompleted = FALSE;
        AllPendingCallbacksDone();
    }    
    
    m_bInProcessPendingCallbacks = FALSE;
    
    /* Do we still have more pending callbacks to process? */
    if (m_pPendingCallbackList && m_pPendingCallbackList->GetCount() > 0)
    {
        goto start;
    }

    m_pMutex->Unlock();
}        

void CMacAsyncFile::ProcessBlock(HXParamBlockRec* pb)
{
	m_pMutex->Lock();
	if (mOutStandingCallbacks > 0)
	{
	    mOutStandingCallbacks--;
	}

	if(pb->entry->state == AB_STATE_OUTOFCONTEXT ||
	   m_bSettingSeekState)
	{
	    //
	    //      Don't need to do anything with this data
	    //      just ignore it. 
	    //              
	    pb->entry->state = AB_STATE_IGNORED;
	}
	else if (pb->entry->state == AB_STATE_ASYNC_INTERNAL_SEEK)
	{
	    m_ulReadPositionInFile = pb->io.ioParam.ioPosOffset;
	    pb->entry->state = AB_STATE_IGNORED;
	}
	else if (pb->entry->state == AB_STATE_ASYNC_SEEK)
	{
	    mFilePos = pb->io.ioParam.ioPosOffset;
	    m_ulReadPositionInFile = mFilePos;
	    pb->entry->state = AB_STATE_IGNORED;
	    m_bPendingAsyncSeekCompleted = TRUE;	    
	}
	else if (pb->entry->state == AB_STATE_WRITE)
	{
	    HX_VECTOR_DELETE(pb->entry->buffer);
	    pb->entry->state = AB_STATE_IGNORED;
	}
	else
	{
	    /* Are we at the end of the file? */
	    if (pb->io.ioParam.ioActCount < pb->entry->size)
	    {
	        m_bFileDone = TRUE;

#if defined(_DEBUG) && defined (_LOG_DATA)
	        char tmpStr[255]; /* Flawfinder: ignore */
	        sprintf(tmpStr, "ProcessBlock %d %d;g", pb->io.ioParam.ioActCount, pb->entry->size); /* Flawfinder: ignore */
	        DEBUGSTR(c2pstr(tmpStr));
#endif
	    }

	    pb->entry->size = pb->io.ioParam.ioActCount;
	    pb->entry->current_size = pb->io.ioParam.ioActCount;
	    if(pb->io.ioParam.ioBuffer && pb->entry->size > 0)
	    {
	        pb->entry->state = AB_STATE_FULL;
	        m_ulReadPositionInFile	+= pb->entry->size;
	    }
	    else
	    {
	        pb->entry->state = AB_STATE_IGNORED;
	    }
	}

	if (mOutStandingCallbacks == 0)
	{
            m_bAllCallbacksCompleted = TRUE;
	}
	m_pMutex->Unlock();
}

void CMacAsyncFile::AddToThePendingList(void* pNode)
{
    m_pMutex->Lock();

    /* Atleast one of the list MUST be free to operate on */
    HX_ASSERT(m_pPendingCallbackList != NULL);
		
    if (m_pPendingCallbackList)
    {
        m_pPendingCallbackList->AddTail(pNode);
    }

    m_pMutex->Unlock();
}		

//
//
//      Start the Async file IO call which physically reads the data into the buffer.
//
//
void CMacAsyncFile::ReadAsyncData(HXBOOL ASYNC)
{
    m_pMutex->Lock();

    OSErr e = noErr;

#if defined(_DEBUG) && defined (_LOG_DATA)
    char str[255]; /* Flawfinder: ignore */
   ::sprintf(str, "ReadAsyncData: %d;g", (int) ASYNC); /* Flawfinder: ignore */
    DEBUGSTR(c2pstr(str));
#endif
 
    // mOutStandingCallbacks: if there are any, then we bail right
    // now 'cause we don't want scads of pending callbacks... the math
    // will work out in the end because subsequent calls will have us
    // read more bytes.
    
    if(!mRefNum || m_bFileDone || mOutStandingCallbacks > 0)
    {
        m_pMutex->Unlock();
	return;
    }

    ULONG32 emptysize = mReadQueue->GetAvailableElements();

    if(emptysize < kNeedToReadThreshhold)
    {
        m_pMutex->Unlock();
        DEBUGSTR("\p ReadAsyncData: emptysize < kNeedToReadThreshhold ;g");
 	return;
    }
    
    if (m_bCheckFromPQ)
    {
        HXBOOL bFilled = FillBufferFromPQ(m_ulReadPositionInFile);
        if (bFilled)
        {
            emptysize = mReadQueue->GetAvailableElements();
            if(emptysize < kNeedToReadThreshhold)
            {
                m_pMutex->Unlock();
                return;
            }
        }
    }
    
    if (emptysize > kMacMaxChunkToRead)
    {
	emptysize = kMacMaxChunkToRead;
    }
    
    if (m_bInternalSeekNeeded)
    {
        m_bInternalSeekNeeded = FALSE;
        PerformInternalSeek(ASYNC);
        // more stuff will be done once we get async callback for internal seek
        if (ASYNC)
        {
            m_pMutex->Unlock();
            return;
        }
    }

    HXParamBlockRec *pb = new HXParamBlockRec;

    memset(pb, 0, sizeof(HXParamBlockRec));

    pb->param = this;
    pb->io.ioParam.ioCompletion = zmIOCallbackUPP;
    pb->io.ioParam.ioRefNum = mRefNum;

    pb->entry = new HXAsyncQueueBuffer;
    memset(pb->entry, 0, sizeof(HXAsyncQueueBuffer));
    pb->entry->size = emptysize;
    pb->entry->current_size = emptysize;
    
    pb->entry->position_in_file = m_ulReadPositionInFile;

    char* tempbuffer = new char[emptysize];

    pb->entry->buffer 		= tempbuffer;
    pb->entry->current_buf_ptr  = tempbuffer;
    pb->entry->state = AB_STATE_EMPTY;

    pb->io.ioParam.ioBuffer = tempbuffer;
    pb->io.ioParam.ioReqCount = emptysize;

    HX_ASSERT(mSeekFromWhere == 0);

    e = ReadData(pb, ASYNC);


    if (!ASYNC)
    {
         if (e == noErr)
         {
	     pb->entry->state = AB_STATE_FULL;
	     pb->entry->size = pb->io.ioParam.ioActCount;
	     pb->entry->current_size = pb->io.ioParam.ioActCount;
	     m_ulReadPositionInFile += pb->entry->size;
	     mAsyncQueue->AddTail(pb->entry);
	 }
	 else
	 {
	     HX_DELETE(pb->entry);
	 }
	     	 
	 HX_DELETE(pb);
    }
    else
    {
        if (e != noErr)
        {
	    HX_DELETE(pb->entry);
	    HX_DELETE(pb);
        }
    }    

    DEBUGSTR("\p ReadAsyncData EXIT; g");

    m_pMutex->Unlock();

    return;
}

ULONG32 CMacAsyncFile::WriteAsyncData(IHXBuffer* pBuffer, HXBOOL ASYNC)
{
    ULONG32 outWrittenSize = 0;
    OSErr e = noErr;

#if defined(_DEBUG) && defined (_LOG_DATA)
    char str[255]; /* Flawfinder: ignore */
   ::sprintf(str, "WriteAsyncData: %d;g", (int) ASYNC); /* Flawfinder: ignore */
    DEBUGSTR(c2pstr(str));
#endif
 
    if(!mRefNum)
    {
	return outWrittenSize;
    }

    pBuffer->AddRef();

    HXParamBlockRec *pb = new HXParamBlockRec;

    memset(pb, 0, sizeof(HXParamBlockRec));

    pb->param = this;
    pb->io.ioParam.ioCompletion = zmIOCallbackUPP;
    pb->io.ioParam.ioRefNum = mRefNum;

    pb->entry = new HXAsyncQueueBuffer;
    memset(pb->entry, 0, sizeof(HXAsyncQueueBuffer));
    
    outWrittenSize = pBuffer->GetSize();
    pb->entry->size = outWrittenSize;
    pb->entry->current_size = outWrittenSize;
    pb->entry->state = AB_STATE_WRITE;

    char* tempbuffer = NULL;
    if (ASYNC)
    {
    	tempbuffer = new char[outWrittenSize];
    	BlockMoveData(pBuffer->GetBuffer(), tempbuffer, outWrittenSize);
    	pb->entry->buffer = tempbuffer;
    	pb->entry->current_buf_ptr = tempbuffer;
	pb->io.ioParam.ioBuffer = tempbuffer;
    }
    else
    {
	pb->io.ioParam.ioBuffer = ( char* ) pBuffer->GetBuffer();
    }

    pb->io.ioParam.ioReqCount = outWrittenSize;

    HX_ASSERT(mSeekFromWhere == 0);

    e = WriteData(pb, ASYNC);


    if (!ASYNC)
    {
	 HX_DELETE(pb->entry);
	 HX_DELETE(pb);
    }
    else
    {
        if (e != noErr)
        {
	    HX_DELETE(pb->entry);
	    HX_DELETE(pb);
        }
    }    

    DEBUGSTR("\p WriteAsyncData EXIT; g");
    pBuffer->Release();
    return outWrittenSize;
}

void CMacAsyncFile::UpdateFilePos(HXBOOL bASync, HXBOOL bInternal)
{
    AddRef();
    m_pMutex->Lock();
     DEBUGSTR("\p UpdateFilePos;g");
   OSErr e = noErr;

    if(!mRefNum)
    {
	m_pMutex->Unlock();
	Release();
	return;
    }

    HXParamBlockRec *pb = new HXParamBlockRec;

    memset(pb, 0, sizeof(HXParamBlockRec));

    pb->param = this;
    pb->io.ioParam.ioRefNum = mRefNum;
    pb->io.ioParam.ioReqCount = 0;
    pb->io.ioParam.ioCompletion = zmIOCallbackUPP;

    pb->entry = new HXAsyncQueueBuffer;
    memset(pb->entry, 0, sizeof(HXAsyncQueueBuffer));

    HX_ASSERT(mSeekFromWhere > 0);

    if(mSeekFromWhere)
    {
	pb->io.ioParam.ioPosMode = mSeekFromWhere;
	pb->io.ioParam.ioPosOffset = mSeekPos;

	mSeekFromWhere = 0;
	mSeekPos = 0;
	if (bInternal)
	{
            pb->entry->state = AB_STATE_ASYNC_INTERNAL_SEEK;
        }
        else
        {
            pb->entry->state = AB_STATE_ASYNC_SEEK;
        }
    }

    e = SeekData(pb, bASync);

    if (!bASync)
    {
         if (e == noErr)
         {
	     m_ulReadPositionInFile = pb->io.ioParam.ioPosOffset;
             if (!bInternal)
             {
	         mFilePos = pb->io.ioParam.ioPosOffset;
	     }
	 }
	     
	 HX_DELETE(pb->entry);
	 HX_DELETE(pb);
    }
    else
    {
        if (e != noErr)
        {
	    HX_DELETE(pb->entry);
	    HX_DELETE(pb);
        }
    }    

    if (!bASync && !bInternal)
    {
	PendingAsyncSeekDone();
    }

    m_pMutex->Unlock();
    Release();
    return;

}

OSErr
CMacAsyncFile::ReadData(HXParamBlockRec* pb, HXBOOL ASYNC)
{
    m_pMutex->Lock();

    OSErr e = noErr;

    if(ASYNC)
    {
	mAsyncQueue->AddTail(pb->entry);
	mOutStandingCallbacks++;
    }
    
    // xxxbobclark large PBRead's on SMB-mounted volumes are failing here.
    // That's why we reduced the constants in the .h file for carbon builds.

    e = PBRead((ParmBlkPtr) pb, ASYNC);

    if (e == eofErr)
    {
	    m_bFileDone = TRUE;
	    
        DEBUGSTR("\pFIle Done in ReadData;g");
	    /* Mask this error if this is the last read */
	    /* If either we are in aysnc mode OR we actually read some data */
	    if (ASYNC ||  pb->io.ioParam.ioActCount > 0)
	    {
	    	e = noErr;
	    }
    }

    if(e != noErr && ASYNC && mOutStandingCallbacks > 0)
    {
        mAsyncQueue->RemoveTail();
        mOutStandingCallbacks--;
    }

    m_pMutex->Unlock();

    return e;
}

OSErr
CMacAsyncFile::WriteData(HXParamBlockRec* pb, HXBOOL ASYNC)
{
    m_pMutex->Lock();

    OSErr e = noErr;

    if(ASYNC)
    {
	mAsyncQueue->AddTail(pb->entry);
	mOutStandingCallbacks++;
    }

    e = PBWrite((ParmBlkPtr) pb, ASYNC);

    if(e != noErr && ASYNC && mOutStandingCallbacks > 0)
    {
        mAsyncQueue->RemoveTail();
        mOutStandingCallbacks--;
    }

    m_pMutex->Unlock();

    return e;
}

OSErr
CMacAsyncFile::SeekData(HXParamBlockRec* pb, HXBOOL ASYNC)
{
    m_pMutex->Lock();

    OSErr e = noErr;

    if(ASYNC)
    {
	    mAsyncQueue->AddTail(pb->entry);
	    mOutStandingCallbacks++;
    }

    e = PBSetFPos((ParmBlkPtr) pb, ASYNC);

    if (e == eofErr)
    {
	DEBUGSTR("\p File done in SeekData");
	m_bFileDone = TRUE;
	e = noErr;
    }

    if(e != noErr && ASYNC && mOutStandingCallbacks > 0)
    {
        mAsyncQueue->RemoveTail();
	mOutStandingCallbacks--;
    }

    m_pMutex->Unlock();

    return e;
}

void
CMacAsyncFile::PendingAsyncSeekDone()
{
    AddRef();
    m_pMutex->Lock();
    HX_ASSERT(m_bSeekPending == TRUE);

    m_bSeekPending = FALSE;
     DEBUGSTR("\p PendingAsyncSeekDone;g");
   // ? should check status
    m_pResponse->AsyncSeekDone(HXR_OK);
    m_pMutex->Unlock();
    Release();
}

void
CMacAsyncFile::AllPendingCallbacksDone()
{   
    DEBUGSTR("\p AllPendingCallbacksDone;g");

    m_pMutex->Lock();
    
    HX_ASSERT(m_pPendingCallbackList->GetCount() == 0);
     	      
    if(m_bReadPending)
    {
        EnqueueAsyncBuffers();
	ProcessPendingRead();
    }

    /* If there is still a pending read and no outstanding reads
     * issue one more read
     */
    if(m_bReadPending && mOutStandingCallbacks == 0)
    {
        DEBUGSTR("\pm_bReadPending && mOutStandingCallbacks == 0;g");
	ReadAsyncData(TRUE);
    }
    m_pMutex->Unlock();
}

void
CMacAsyncFile::ProcessPendingRead()
{
    AddRef();
    m_pMutex->Lock();
    
    ULONG32 totalinqueue = mReadQueue->GetQueuedItemCount();
    ULONG32 numberInPendingCallbackList = m_pPendingCallbackList->GetCount();
    if(m_bReadPending &&
       ((m_bFileDone && numberInPendingCallbackList == 0) ||
       (totalinqueue >= m_ulPendingReadCount)))
    {
	m_bReadPending = FALSE;

	if (totalinqueue == 0)
	{
       DEBUGSTR("\p AsyncReadDone FAIL;g");
 	    m_pResponse->AsyncReadDone(HXR_FAIL, NULL);
	}
	else
	{
   	    UINT32 count = m_ulPendingReadCount;
	
	    if (count > totalinqueue)
	    {
		count = totalinqueue;
	    }

	    IHXBuffer *pBuffer = new CHXBuffer;
	    pBuffer->AddRef();
	    pBuffer->SetSize(count);
	    char *buf = (char*) pBuffer->GetBuffer();

		    mFilePos += mReadQueue->DeQueue(buf, count);
		    
#if defined(_DEBUG) && defined (_LOG_DATA)
       char str[255]; /* Flawfinder: ignore */
       ::sprintf(str, "AsyncReadDone: HXR_OK: %d;g", (int) count); /* Flawfinder: ignore */
        DEBUGSTR(c2pstr(str));
#endif	    
	    m_pResponse->AsyncReadDone(HXR_OK, pBuffer);
	    pBuffer->Release();
	}
    }
    m_pMutex->Unlock();
    Release();
}

HX_RESULT
CMacAsyncFile::SetAsyncResponse(CMacAsyncFileResponse * pResponse)
{
    if(!pResponse)
    {
	return HXR_UNEXPECTED;
    }

    m_pMutex->Lock();
    m_pResponse = pResponse;
    m_pMutex->Unlock();
    return HXR_OK;
}

