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

///////////////////
//	include files
#include "hxtypes.h"
#include "hxmap.h"
#include "hxalloc.h"
#include "hxassert.h"
#include "hxerrors.h"
#include "hxheap.h" 
#include "hxtick.h"
#include "chxpckts.h"
#include "pckunpck.h"
#include "hxstrutl.h"

#if defined(_WINDOWS)
#include "hlxclib/windows.h"
#include <windowsx.h>
#endif

///////////////////
//	private data
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif

///////////////////
//	private functions

///////////////////////////////////////////////////////////////////////////////
//	CHXMemoryBlock Implementation
///////////////////////////////////////////////////////////////////////////////
CHXMemoryBlock::CHXMemoryBlock(CHXMemoryAllocator *pAllocator, HXBOOL bGlobalAlloc /* = FALSE */)
{
	HX_ASSERT_VALID_PTR(pAllocator);

	m_pMemBuffer = NULL;
	m_MemBufferSize = 0;
	m_RefCount = 0;
	m_pAllocator = pAllocator;    
	m_bUseGlobalAlloc = bGlobalAlloc;	// Determines whether we use GlobalAlloc or new
}

CHXMemoryBlock::~CHXMemoryBlock()
{
	HX_ASSERT(m_RefCount == 0);
	HX_ASSERT(m_pMemBuffer == NULL);

	// clean up just in case
	if (m_pMemBuffer != NULL)
	{
#if defined(_WINDOWS) && !defined(WIN32_PLATFORM_PSPC)
            if (m_bUseGlobalAlloc)
            {    
                GlobalFreePtr(m_pMemBuffer);
            }
            else
#endif // _WINDOWS
            {
		delete [] m_pMemBuffer;
            }

            m_pMemBuffer = NULL;
            m_MemBufferSize = 0;
	}
}

HXBOOL CHXMemoryBlock::Allocate(ULONG32 uSize)
{
	HX_ASSERT_VALID_PTR(m_pAllocator);
	
	if (uSize > 0)
	{   
		// On x86 systems we'd like the buffer to be aligned to "HXALLOC_BUFFER_ALIGNMENT" bytes.
		// So we'll allocate (uSize + HXALLOC_BUFFER_ALIGNMENT - 1) bytes, and move the returned
		// buffer pointer to the correct alignment
		// clean up just in case
		HX_ASSERT(NULL==m_pMemBuffer);
		if (m_pMemBuffer != NULL)
		{
#if defined(_WINDOWS) && !defined(WIN32_PLATFORM_PSPC)
			if (m_bUseGlobalAlloc)
			{    
				GlobalFreePtr(m_pMemBuffer);
			}
			else
#endif // _WINDOWS
			{
				delete [] m_pMemBuffer;
			}
	        m_pMemBuffer = NULL;
		    m_MemBufferSize = 0;
		}

#if defined(_WINDOWS) && !defined(WIN32_PLATFORM_PSPC)
		if (m_bUseGlobalAlloc)
		{
#if defined(_M_IX86)
		    m_pMemBuffer = (UCHAR*) GlobalAllocPtr(GMEM_MOVEABLE | GMEM_SHARE, uSize + HXALLOC_BUFFER_ALIGNMENT - 1);
#else
		    m_pMemBuffer = (UCHAR*) GlobalAllocPtr(GMEM_MOVEABLE | GMEM_SHARE, uSize);
#endif
		}
		else
#endif
		{
#if defined(_M_IX86)
			m_pMemBuffer = new UCHAR[uSize + HXALLOC_BUFFER_ALIGNMENT - 1];
#else
			m_pMemBuffer = new UCHAR[uSize];
#endif
		}
		
                HX_ASSERT(m_pMemBuffer);
		if (m_pMemBuffer != NULL)
		{
			m_MemBufferSize = uSize;
                        return(TRUE);
		}
	}

	// we consider it a improper to allocate with 0 size.
	return(FALSE);
}

void CHXMemoryBlock::Free()
{
	HX_ASSERT_VALID_PTR(m_pMemBuffer);

	if (m_pMemBuffer != NULL)
	{
#if defined(_WINDOWS) && !defined(WIN32_PLATFORM_PSPC)
		if (m_bUseGlobalAlloc)
		{    
			GlobalFreePtr(m_pMemBuffer);
	        }
		else
#endif // _WINDOWS
		{
           		delete [] m_pMemBuffer;
		}
		
		m_pMemBuffer = NULL;
		m_MemBufferSize = 0;
	}
}

HX_RESULT
CHXMemoryBlock::QueryInterface(HX_IID iid, void** ppvObj)
{
	// Do dynamic Cast
	if (iid == IID_IHXUnknown)
	{
		*ppvObj = (IHXUnknown *)this;
	}
	else
	{
		*ppvObj = NULL;
	}

	// Do lifetime control
	if (*ppvObj != NULL)
	{
		((IHXUnknown *)*ppvObj)->AddRef();
	}

	return( (*ppvObj != NULL)?(HX_NO_ERROR):(HX_NO_INTERFACE));
}

ULONG32 CHXMemoryBlock::AddRef()
{
	// shouldn't add ref empty blocks.
	HX_ASSERT_VALID_PTR(m_pMemBuffer);

	return InterlockedIncrement(&m_RefCount);
}

ULONG32 CHXMemoryBlock::Release()
{
    HX_ASSERT(m_RefCount != 0);

    if(InterlockedDecrement(&m_RefCount) == 0)
    {
	//  This is used instead of delete
	//  The Allocator puts the memory into a free pool and 
	//  deletes it when it is destroyed
	m_pAllocator->NotifyFreeBlock(this);
	return 0;
    }

    return m_RefCount;
}

///////////////////////////////////////////////////////////////////////////////
//	CHXMemoryAllocator Implementation
///////////////////////////////////////////////////////////////////////////////
HX_RESULT
CHXMemoryAllocator::QueryInterface(HX_IID iid, void** ppvObj)
{
	// Do dynamic Cast
	if (iid == IID_IHX20MemoryAllocator)
	{
		*ppvObj = (IHX20MemoryAllocator *)this;
	}
	else if (iid == IID_IHXUnknown)
	{
		*ppvObj = (IHXUnknown *)this;
	}
	else
	{
		*ppvObj = NULL;
	}

	// Do lifetime control
	if (*ppvObj != NULL)
	{
		((IHXUnknown *)*ppvObj)->AddRef();
	}

	return( (*ppvObj != NULL)?(HX_NO_ERROR):(HX_NO_INTERFACE));

}

ULONG32 CHXMemoryAllocator::AddRef()
{
    return InterlockedIncrement(&m_ref);
}

ULONG32 CHXMemoryAllocator::Release()
{
    HX_ASSERT(m_ref != 0);
    if(InterlockedDecrement(&m_ref) == 0)
    {
	delete this;
	return 0;
    }

    return m_ref;
}

CHXMemoryAllocator::CHXMemoryAllocator(IUnknown* pContext,
				       const char* szIdentifier, 
                                       HXBOOL bThreadSafe, 
                                       HXBOOL bUseGlobalAlloc, 
                                       HXBOOL bEstimateFreeListSize, 
                                       UINT32 uiPoolHistoryDepth)
    : m_AllocCount(0)
    , m_uSize(0)
    , m_Count(0)
    , m_ref(0)
    , m_bThreadSafe(bThreadSafe)
    , m_bUseGlobalAlloc(bUseGlobalAlloc)
#ifndef WIN32
    , m_pMutex(NULL)
#endif	// WIN32
    , m_bEstimateFreeListSize(bEstimateFreeListSize)
    , m_uiPoolHistoryDepth(uiPoolHistoryDepth)
    , m_uiCurrentHistoryIndex(0)
    , m_pContext(pContext)
{
    HX_ADDREF(m_pContext);

    m_strIdentifier = szIdentifier;

#ifdef WIN32
    if (m_bThreadSafe) InitializeCriticalSection(&m_critsec);
    if (ERROR_SUCCESS != RegOpenKeyEx(HKEY_CLASSES_ROOT,
				      OS_STRING("RVEncoderStats"),0,
				      KEY_ALL_ACCESS,&m_hkey))
    {
        m_hkey = NULL;
    }
#else	// WIN32
    if (bThreadSafe)
    {
	CreateInstanceCCF(CLSID_IHXMutex, (void**)&m_pMutex, m_pContext);
    }
#endif	// WIN32

    if (m_bEstimateFreeListSize && uiPoolHistoryDepth)
    {
        m_puiBufferUseHistory = new UINT32[uiPoolHistoryDepth]; 
        memset(m_puiBufferUseHistory, 0, sizeof(m_puiBufferUseHistory));
    }
    else
    {
        m_puiBufferUseHistory = NULL;
        m_bEstimateFreeListSize = FALSE;
    }

    WriteRuntimeStats();
}


CHXMemoryAllocator::CHXMemoryAllocator(IUnknown* pContext,
				       HXBOOL bThreadSafe/*=FALSE*/, 
                                       HXBOOL bUseGlobalAlloc/*=FALSE*/,
                                       HXBOOL bEstimateFreeListSize, 
                                       UINT32 uiPoolHistoryDepth)
    : m_AllocCount(0)
    , m_uSize(0)
    , m_Count(0)
    , m_ref(0)
    , m_bThreadSafe(bThreadSafe)
    , m_bUseGlobalAlloc(bUseGlobalAlloc)
#ifndef WIN32
    , m_pMutex(NULL)
#endif	// WIN32
    , m_bEstimateFreeListSize(bEstimateFreeListSize)
    , m_uiPoolHistoryDepth(uiPoolHistoryDepth)
    , m_uiCurrentHistoryIndex(0)
    , m_pContext(pContext)
{
    HX_ADDREF(m_pContext);

#ifdef WIN32
	if (m_bThreadSafe) InitializeCriticalSection(&m_critsec);
        m_hkey = NULL;
#else	// WIN32
	if (bThreadSafe)
	{
	    CreateInstanceCCF(CLSID_IHXMutex, (void**)&m_pMutex, m_pContext);
	}
#endif	// WIN32

        if (m_bEstimateFreeListSize && uiPoolHistoryDepth)
        {
            m_puiBufferUseHistory = new UINT32[uiPoolHistoryDepth]; 
            memset(m_puiBufferUseHistory, 0, sizeof(m_puiBufferUseHistory));
        }
        else
        {
            m_puiBufferUseHistory = NULL;
            m_bEstimateFreeListSize = FALSE;
        }
}

CHXMemoryAllocator::~CHXMemoryAllocator()
{
    WriteRuntimeStats();

        // cleanup memory left by the user in the map
	POSITION currentMapNode = m_MemBlockMap.GetStartPosition();
	CHXMemoryBlock * pMemBlock;
	UCHAR * memPtr;

	// we should find the map empty.
	HX_ASSERT(m_MemBlockMap.IsEmpty());

	while(!m_MemBlockMap.IsEmpty())
	{
		m_MemBlockMap.GetNextAssoc(currentMapNode, (void *&)memPtr, (void *&)pMemBlock);

		if (pMemBlock != NULL)
		{
			// by releasing the memory the memblock and buffer
			// get cleaned up correctly.
			while ((UINT16)pMemBlock->Release())
			{
				;
			}
		}
	}

	// need to clean up all the allocated nodes
	while(!m_freeList.IsEmpty())
	{
		CHXMemoryBlock * pMemBlock = (CHXMemoryBlock *)
			m_freeList.RemoveHead();
		
		pMemBlock->Free();
		delete pMemBlock;
	}

#ifdef WIN32
	if (m_bThreadSafe) DeleteCriticalSection(&m_critsec);
        if (m_hkey)
            RegCloseKey(m_hkey);
#else	// WIN32
	HX_RELEASE(m_pMutex);
#endif	// WIN32

        if (m_puiBufferUseHistory)
        {
            delete [] m_puiBufferUseHistory; 
        }

	HX_RELEASE(m_pContext);
}

#ifdef RUNTIME_STATISTICS
void CHXMemoryAllocator::WriteRuntimeStats()
{
#ifdef WIN32
    if (m_hkey && m_strIdentifier.GetLength())
    {
        char szData[128]; /* Flawfinder: ignore */
        SafeSprintf(szData,128,"Size:%d AllocCount:%d FreeList:%d Map:%d",
            m_uSize,m_AllocCount,m_freeList.GetCount(),m_MemBlockMap.GetCount());
        RegSetValueEx(m_hkey,m_strIdentifier,0,REG_SZ,(BYTE*)szData,strlen(szData)+1);
    }
#endif
}
#endif

HX_RESULT CHXMemoryAllocator::SetProperties(HX20ALLOCPROPS* pRequest, 
	HX20ALLOCPROPS* pActual)
{
#ifdef WIN32
	if (m_bThreadSafe) EnterCriticalSection(&m_critsec);
#else	// WIN32
	HX_LOCK(m_pMutex);
#endif	// WIN32

	// if there are buffers allocated, then empyt the free list 
	// so we don't give anyone a buffer of the wrong size
	if (m_AllocCount != 0 && pRequest->uBufferSize != m_uSize)
	{
            while (!m_freeList.IsEmpty())
            {
                CHXMemoryBlock * pMemBlock = (CHXMemoryBlock *)
                        m_freeList.RemoveHead();
                HX_ASSERT(pMemBlock);
                pMemBlock->Free();
                delete pMemBlock;
                m_AllocCount--;		
            }
	}
	
	pActual->uBufferSize = m_uSize = pRequest->uBufferSize;
	pActual->nNumBuffers = m_Count = pRequest->nNumBuffers;

        WriteRuntimeStats();

#ifdef WIN32
	if (m_bThreadSafe) LeaveCriticalSection(&m_critsec);
#else	// WIN32
	HX_UNLOCK(m_pMutex);
#endif	// WIN32

	return(HXR_OK);
}

HX_RESULT CHXMemoryAllocator::GetProperties(HX20ALLOCPROPS* pProps)
{
	pProps->uBufferSize = m_uSize;
	pProps->nNumBuffers = m_Count;

	return(HXR_OK);
}

UCHAR * CHXMemoryAllocator::GetPacketBuffer(IHXUnknown ** pPacketBuffer)
{
	HX_ASSERT_VALID_PTR(this);
	HX_ASSERT_VALID_PTR(pPacketBuffer);
	UCHAR * pRetVal = NULL;
	*pPacketBuffer = NULL;
	
#ifdef WIN32
	if (m_bThreadSafe) EnterCriticalSection(&m_critsec);
#else	// WIN32
	HX_LOCK(m_pMutex);
#endif	// WIN32

	if (m_uSize > 0)
	{
		// Get the next free buffer from the buffer pool
		if (!m_freeList.IsEmpty())
		{
			CHXMemoryBlock * pMemBlock;
			HXBOOL bRemoveFromHead = (HX_GET_BETTERTICKCOUNT() & 0x01) ? TRUE : FALSE;
			if (bRemoveFromHead)
			{
			    // Get the first buffer
			    pMemBlock = (CHXMemoryBlock *) m_freeList.RemoveHead();
			}
			else
			{
			    // Get the last buffer
			    pMemBlock = (CHXMemoryBlock *) m_freeList.RemoveTail();
			}
			
			// Add ref the block so we know we are using it
			pMemBlock->AddRef();
			
			// setup the map so we don't loose the block
			pRetVal = pMemBlock->GetSampleBase();
			m_MemBlockMap.SetAt(pRetVal, pMemBlock);
			*pPacketBuffer = (IHXUnknown *)pMemBlock;
		}

		// if we didn't find any blocks in the list allocate a new one
		if (pRetVal == NULL)
		{
			CHXMemoryBlock * pMemBlock = new CHXMemoryBlock(this, m_bUseGlobalAlloc);
                        HX_ASSERT(pMemBlock);
                        
			if (pMemBlock != NULL)
			{
				if (pMemBlock->Allocate(m_uSize))
				{
					pMemBlock->AddRef();
					pRetVal = pMemBlock->GetSampleBase();
					m_MemBlockMap.SetAt(pRetVal, pMemBlock);
					m_AllocCount++;
					*pPacketBuffer = (IHXUnknown *)pMemBlock;
				}
                                else
                                {
                                    HX_ASSERT(FALSE);
                                    delete pMemBlock;
                                }
			}
		}
	}

        if (m_bEstimateFreeListSize)
        {                
            m_uiCurrentHistoryIndex = ++m_uiCurrentHistoryIndex % m_uiPoolHistoryDepth;
            m_puiBufferUseHistory[m_uiCurrentHistoryIndex] = m_MemBlockMap.GetCount(); 
        }

        WriteRuntimeStats();

#ifdef WIN32
	if (m_bThreadSafe) LeaveCriticalSection(&m_critsec);
#else	// WIN32
	HX_UNLOCK(m_pMutex);
#endif	// WIN32

	return(pRetVal);
}

CHXMemoryBlock* CHXMemoryAllocator::PacketPtrToPacketObj(UCHAR * memPtr)
{
        HX_ASSERT(memPtr);
        
	// find the instance pointer for the CHXMemoryBlock that 
	// ownes this memory buffer and AddRef the block
	CHXMemoryBlock * pMemBlock = NULL;

#ifdef WIN32
	if (m_bThreadSafe) EnterCriticalSection(&m_critsec);
#else	// WIN32
	HX_LOCK(m_pMutex);
#endif	// WIN32

	m_MemBlockMap.Lookup(memPtr, (void *&)pMemBlock);

#ifdef WIN32
	if (m_bThreadSafe) LeaveCriticalSection(&m_critsec);
#else	// WIN32
	HX_UNLOCK(m_pMutex);
#endif	// WIN32

	return(pMemBlock);
}

UINT16 CHXMemoryAllocator::AddRefPacketPtr(UCHAR * memPtr)
{
        HX_ASSERT(memPtr);
        
	// find the instance pointer for the CHXMemoryBlock that 
	// ownes this memory buffer and AddRef the block
	CHXMemoryBlock * pMemBlock;
	UINT16 uiRetVal = 0;

#ifdef WIN32
	if (m_bThreadSafe) EnterCriticalSection(&m_critsec);
#else	// WIN32
	HX_LOCK(m_pMutex);
#endif	// WIN32

	if (m_MemBlockMap.Lookup(memPtr, (void *&)pMemBlock))
	{
		uiRetVal = (UINT16)pMemBlock->AddRef();
	}
	else
	{
		// shouldn't AddRef memory that doesn't exist...
		HX_ASSERT(FALSE);
	}

#ifdef WIN32
	if (m_bThreadSafe) LeaveCriticalSection(&m_critsec);
#else	// WIN32
	HX_UNLOCK(m_pMutex);
#endif	// WIN32

	return(uiRetVal);
}

UINT16 CHXMemoryAllocator::ReleasePacketPtr(UCHAR * memPtr)
{
        HX_ASSERT(memPtr);
	// find the instance pointer for the CHXMemoryBlock that 
	// ownes this memory buffer and AddRef the block
	UINT16 uiRetVal = 0;
	if (!memPtr)
	{
	    return(uiRetVal);
	}

#ifdef WIN32
	if (m_bThreadSafe) EnterCriticalSection(&m_critsec);
#else	// WIN32
	HX_LOCK(m_pMutex);
#endif	// WIN32

	CHXMemoryBlock * pMemBlock;
	if (m_MemBlockMap.Lookup(memPtr, (void *&)pMemBlock))
	{
		uiRetVal = (UINT16)pMemBlock->Release();
	}
	else
	{
		// shouldn't AddRef memory that doesn't exist...
		HX_ASSERT(FALSE);
	}

        WriteRuntimeStats();

#ifdef WIN32
	if (m_bThreadSafe) LeaveCriticalSection(&m_critsec);
#else	// WIN32
	HX_UNLOCK(m_pMutex);
#endif	// WIN32

	return(uiRetVal);
}

#ifdef XXXJEFFA_DEBUG
UINT32 g_NextLogTime = 0;
#endif

void CHXMemoryAllocator::NotifyFreeBlock(CHXMemoryBlock * pMemBlock)
{
        HX_ASSERT(pMemBlock);
#ifdef WIN32
	if (m_bThreadSafe) EnterCriticalSection(&m_critsec);
#else	// WIN32
	HX_LOCK(m_pMutex);
#endif	// WIN32

	// Remove the memory from the map since it's not going
	// to be considered valid anymore
	m_MemBlockMap.RemoveKey(pMemBlock->GetSampleBase());

        UINT32 uiMaxRecentlyUsed = 0;
        if (m_bEstimateFreeListSize)
        {
            for (UINT32 i = 0; i < m_uiPoolHistoryDepth; i++)
            {
                if (m_puiBufferUseHistory[i] > uiMaxRecentlyUsed)
                {
                    uiMaxRecentlyUsed = m_puiBufferUseHistory[i];   
                }
            }
        }

	// as long as this block is the right size we will
	// recycle it
	if (m_uSize == pMemBlock->GetLength() 
	    && ( (m_freeList.GetCount() < (int) m_Count) &&
                 (m_bEstimateFreeListSize ? m_AllocCount <= uiMaxRecentlyUsed + 1 : TRUE)
               )
           )
	{
		HXBOOL bAddToTail = (HX_GET_BETTERTICKCOUNT() & 0x01) ? TRUE : FALSE;
		if (bAddToTail)
		{
		    m_freeList.AddTail(pMemBlock);
		}
		else
		{
		    m_freeList.AddHead(pMemBlock);
		}

#ifdef XXXJEFFA_DEBUG		
		if (!HXMM_ATINTERRUPT() && HX_GET_TICKCOUNT() > g_NextLogTime)
		{
			FILE* pFile = fopen("Boot Drive:Desktop Folder:log.txt", "a+t");
			if (pFile)
			{
				fprintf(pFile, "%p\t%d\t%d\t%lu\n", this, m_freeList.GetCount(), m_uSize, (m_freeList.GetCount()*m_uSize));
				fclose(pFile);
			}
			g_NextLogTime = HX_GET_TICKCOUNT()  + 10000UL;
		}
#endif
	}
	else
	{
		// if it doesn't go on the free block list 
		// then delete it
		pMemBlock->Free();
		delete pMemBlock;
		m_AllocCount--;
	}

#ifdef WIN32
	if (m_bThreadSafe) LeaveCriticalSection(&m_critsec);
#else	// WIN32
	HX_UNLOCK(m_pMutex);
#endif	// WIN32
}

///////////////////////////////////////////////////////////////////////////////
//	CHXBufferMemoryAllocator Implementation
///////////////////////////////////////////////////////////////////////////////
HX_RESULT
CHXBufferMemoryAllocator::QueryInterface(HX_IID iid, void** ppvObj)
{
    HX_ASSERT(ppvObj);
    // Do dynamic Cast
    if (iid == IID_IHX20MemoryAllocator)
    {
	*ppvObj = (IHX20MemoryAllocator *)this;
    }
    else if (iid == IID_IHXUnknown)
    {
	*ppvObj = (IHXUnknown *)this;
    }
    else
    {
	*ppvObj = NULL;
    }

    // Do lifetime control
    if (*ppvObj != NULL)
    {
	((IHXUnknown *)*ppvObj)->AddRef();
    }

    return( (*ppvObj != NULL)?(HX_NO_ERROR):(HX_NO_INTERFACE));

}

ULONG32 CHXBufferMemoryAllocator::AddRef()
{
    HX_ASSERT(this);
    return InterlockedIncrement(&m_ref);
}

ULONG32 CHXBufferMemoryAllocator::Release()
{
    HX_ASSERT(this);
    HX_ASSERT(m_ref != 0);

    if(InterlockedDecrement(&m_ref) == 0)
    {
	delete this;
	return 0;
    }
    
    return m_ref;
}

struct BufferBlock
{
    IHXBuffer* m_pBuf;
    INT32	m_lRefCount;
};

CHXBufferMemoryAllocator::CHXBufferMemoryAllocator(IUnknown* pContext, HXBOOL bThreadSafe)
#ifndef WIN32
    : m_pMutex(NULL)
#else	// WIN32
    : m_bThreadSafe(bThreadSafe)
#endif	// WIN32
    , m_uSize(0)
    , m_Count(0)
    , m_ref(0)
    , m_pContext(NULL)
{
    m_pContext = pContext;
    HX_ADDREF(m_pContext);

#ifdef WIN32
    if (m_bThreadSafe) InitializeCriticalSection(&m_critsec);
#else	// WIN32
    if (bThreadSafe)
    {
	CreateInstanceCCF(CLSID_IHXMutex, (void**)&m_pMutex, m_pContext);
    }
#endif	// WIN32
}

CHXBufferMemoryAllocator::~CHXBufferMemoryAllocator()
{
    // cleanup memory left by the user in the map
    POSITION currentMapNode = m_BufMap.GetStartPosition();
    BufferBlock* pBufBlock;
    UCHAR* memPtr;
    
    // we should find the map empty.
    HX_ASSERT(m_BufMap.IsEmpty());

    while(!m_BufMap.IsEmpty())
    {
	m_BufMap.GetNextAssoc(currentMapNode, (void *&)memPtr, (void *&)pBufBlock);

	if (pBufBlock != NULL)
	{
	    HX_ASSERT(FALSE);
	    HX_RELEASE(pBufBlock->m_pBuf);
            HX_DELETE(pBufBlock);
	}
    }

#ifdef WIN32
    if (m_bThreadSafe) DeleteCriticalSection(&m_critsec);
#else	// WIN32
    HX_RELEASE(m_pMutex);
#endif	// WIN32
    HX_RELEASE(m_pContext);
}

HX_RESULT CHXBufferMemoryAllocator::SetProperties(HX20ALLOCPROPS* pRequest, 
	HX20ALLOCPROPS* pActual)
{
    pActual->uBufferSize = m_uSize = pRequest->uBufferSize;
    pActual->nNumBuffers = m_Count = pRequest->nNumBuffers;
    return HXR_OK;
}

HX_RESULT CHXBufferMemoryAllocator::GetProperties(HX20ALLOCPROPS* pProps)
{
    pProps->uBufferSize = m_uSize;
    pProps->nNumBuffers = m_Count;

    return HXR_OK;
}

UCHAR* CHXBufferMemoryAllocator::GetPacketBuffer(IHXUnknown ** pPacketBuffer)
{
    // XXXX JHUG we are not keeping a free list, should do that...
    HX_ASSERT_VALID_PTR(this);
    HX_ASSERT_VALID_PTR(pPacketBuffer);
    UCHAR * pRetVal = NULL;
    *pPacketBuffer = NULL;

    if (m_uSize > 0)
    {
        BufferBlock* pBufBlock = new BufferBlock();
        IHXBuffer* pBuffer = NULL;
	CreateBufferCCF(pBuffer, m_pContext);
        HX_ASSERT(pBufBlock && pBuffer);
        if (pBuffer && pBufBlock)
        {
                pBufBlock->m_pBuf = pBuffer;
                pBufBlock->m_lRefCount = 1;

                if (SUCCEEDED(pBuffer->SetSize(m_uSize)))
                {
                        pRetVal = pBuffer->GetBuffer();

#ifdef WIN32
			if (m_bThreadSafe) EnterCriticalSection(&m_critsec);
#else	// WIN32
			HX_LOCK(m_pMutex);
#endif	// WIN32

                        m_BufMap.SetAt(pRetVal, pBufBlock);

#ifdef WIN32
			if (m_bThreadSafe) LeaveCriticalSection(&m_critsec);
#else	// WIN32
			HX_UNLOCK(m_pMutex);
#endif	// WIN32

                        *pPacketBuffer = (IHXUnknown *) pBuffer;
                }
                else
                {
                    HX_RELEASE(pBuffer);
                }
        }
        else
        {
            HX_DELETE(pBufBlock);
            HX_RELEASE(pBuffer);
        }
    }

    return(pRetVal);
}

UCHAR* CHXBufferMemoryAllocator::AddBuffer(IHXBuffer* pBuf)
{
    HX_ASSERT(pBuf);

    UCHAR* pMem = NULL;
    if (pBuf)
    {
        BufferBlock* pBufBlock = new BufferBlock;
        HX_ASSERT(pBufBlock);
        if (pBufBlock)
        {
            pBuf->AddRef();
            pBufBlock->m_pBuf = pBuf;
            pBufBlock->m_lRefCount = 1;
            pMem = pBuf->GetBuffer();
	    
#ifdef WIN32
	    if (m_bThreadSafe) EnterCriticalSection(&m_critsec);
#else	// WIN32
	    HX_LOCK(m_pMutex);
#endif	// WIN32

            m_BufMap.SetAt(pMem, pBufBlock);

#ifdef WIN32
	    if (m_bThreadSafe) LeaveCriticalSection(&m_critsec);
#else	// WIN32
	    HX_UNLOCK(m_pMutex);
#endif	// WIN32
        }

    }

    return pMem;
}

UINT16 CHXBufferMemoryAllocator::AddRefPacketPtr(UCHAR * memPtr)
{
    HX_ASSERT(memPtr);
    
    BufferBlock* pBlock = NULL;
    UINT16 uiRetVal = 0;

#ifdef WIN32
    if (m_bThreadSafe) EnterCriticalSection(&m_critsec);
#else	// WIN32
    HX_LOCK(m_pMutex);
#endif	// WIN32

    if (m_BufMap.Lookup(memPtr, (void *&)pBlock))
    {
	uiRetVal = (UINT16)++(pBlock->m_lRefCount);
    }
    else
    {
	 // shouldn't AddRef memory that doesn't exist...
	 HX_ASSERT(FALSE);
    }

#ifdef WIN32
    if (m_bThreadSafe) LeaveCriticalSection(&m_critsec);
#else	// WIN32
    HX_UNLOCK(m_pMutex);
#endif	// WIN32

    return uiRetVal;
}

UINT16 CHXBufferMemoryAllocator::ReleasePacketPtr(UCHAR * memPtr)
{
    UINT16 uiRetVal = 0;
    HX_ASSERT(memPtr);
    if (!memPtr)
    {
	return uiRetVal;
    }

#ifdef WIN32
    if (m_bThreadSafe) EnterCriticalSection(&m_critsec);
#else	// WIN32
    HX_LOCK(m_pMutex);
#endif	// WIN32

    BufferBlock* pBlock = NULL;
    if (m_BufMap.Lookup(memPtr, (void *&)pBlock))
    {
	uiRetVal = (UINT16)--(pBlock->m_lRefCount);
	if (pBlock->m_lRefCount == 0)
        {
            HX_RELEASE(pBlock->m_pBuf);
            HX_DELETE(pBlock);
            m_BufMap.RemoveKey(memPtr);
        }
    }
    else
    {
	// shouldn't Release memory that doesn't exist...
	HX_ASSERT(FALSE);
    }

#ifdef WIN32
    if (m_bThreadSafe) LeaveCriticalSection(&m_critsec);
#else	// WIN32
    HX_UNLOCK(m_pMutex);
#endif	// WIN32

    return uiRetVal;
}
