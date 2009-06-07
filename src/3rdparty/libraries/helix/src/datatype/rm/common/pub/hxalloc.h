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

#ifndef __HXALLOC_H_
#define __HXALLOC_H_

///////////////////
//	include files
#include "hxtypes.h"
#include "hxslist.h"
#include "hxstring.h"
#include "hxmap.h"
#include "hxcodec.h"
#include "hxthread.h"

#ifdef WIN32	// for critical section
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include "hlxclib/windows.h"
#include <winreg.h>
#endif

//#define RUNTIME_STATISTICS

// On x86 systems we'd like the buffer to be aligned to "HXALLOC_BUFFER_ALIGNMENT" bytes.
// So we'll allocate (uSize + HXALLOC_BUFFER_ALIGNMENT - 1) bytes, and move the returned
// buffer pointer to the correct alignment.
// This number must be a power of 2.
#define HXALLOC_BUFFER_ALIGNMENT 32

// For more efficient memory use, these allocators can use past pool size samples
// to estimate the number of buffers required, and trim appropriately:
// The maximum number of buffers allocated is the maximum number of buffers in 
// use over the last m_uiPoolHistoryDepth series of get calls.  The maximum 
// free list size is then (max buffers in use - current buffers in use) + 1.  
#define DEFAULT_POOL_HISTORY_DEPTH 30

///////////////////
//	private data

///////////////////
//	private functions

class CHXMemoryAllocator;
class CHXMemoryBlock;
struct IHXBuffer;

class CHXMemoryBlock : public IHXUnknown
{

public:
    HX_RESULT QueryInterface(HX_IID iid, void** ppvObj);
    ULONG32	AddRef();
    ULONG32	Release();
    CHXMemoryBlock(CHXMemoryAllocator * pAllocator,  HXBOOL bGlobalAlloc = FALSE);
    ~CHXMemoryBlock();
    virtual HXBOOL		Allocate(ULONG32 uSize);
    virtual void		Free();
    virtual inline ULONG32 GetLength()	    { return(m_MemBufferSize); } 
#if defined(_M_IX86)
    virtual inline UCHAR*   GetSampleBase() { return ((UCHAR *)(((ULONG32)m_pMemBuffer + HXALLOC_BUFFER_ALIGNMENT - 1) & ~(HXALLOC_BUFFER_ALIGNMENT - 1))); }
#else
    virtual inline UCHAR*   GetSampleBase() { return (m_pMemBuffer); }
#endif
    virtual inline HXBOOL IsFree()	    { return(m_RefCount == 0); }

protected:
    UCHAR *					m_pMemBuffer;
    ULONG32					m_MemBufferSize;
    INT32					m_RefCount;
    HX_BITFIELD					m_bUseGlobalAlloc : 1;	// Determines whether we use GlobalAlloc or new
    CHXMemoryAllocator *			m_pAllocator;  
};

class CHXMemoryAllocator : public IHX20MemoryAllocator
{
public:
    HX_RESULT QueryInterface(HX_IID iid, void** ppvObj);
    ULONG32	AddRef();
    ULONG32	Release();

    CHXMemoryAllocator(IUnknown* pContext,
		       const char* szIdentifier,HXBOOL bThreadSafe=FALSE, 
                       HXBOOL m_bUseGlobalAlloc=FALSE, 
                       HXBOOL bEstimateFreeListSize = FALSE, 
                       UINT32 uiPoolHistoryDepth = DEFAULT_POOL_HISTORY_DEPTH);
    CHXMemoryAllocator(IUnknown* pContext,
		       HXBOOL bThreadSafe=FALSE, HXBOOL m_bUseGlobalAlloc=FALSE,
                       HXBOOL bEstimateFreeListSize = FALSE, 
                       UINT32 uiPoolHistoryDepth = DEFAULT_POOL_HISTORY_DEPTH);
    ~CHXMemoryAllocator();

    UCHAR * 		GetPacketBuffer(IHXUnknown ** pPacketBuffer);
    HX_RESULT		SetProperties(HX20ALLOCPROPS* pRequest, HX20ALLOCPROPS* pActual);
    HX_RESULT		GetProperties(HX20ALLOCPROPS* pProps);
    CHXMemoryBlock*	PacketPtrToPacketObj(UCHAR * memPtr);
    UINT16		AddRefPacketPtr(UCHAR * memPtr);
    UINT16		ReleasePacketPtr(UCHAR * memPtr);
    void		NotifyFreeBlock(CHXMemoryBlock * pMemBlock);

    // if RUNTIME_STATISTICS is not defined, this function becomes an inline no-op
#ifdef RUNTIME_STATISTICS
    void                WriteRuntimeStats();
#else
    void                WriteRuntimeStats(){};
#endif

protected:
    IUnknown*		m_pContext;
    CHXMapPtrToPtr	m_MemBlockMap;
    ULONG32		m_AllocCount;		// number of buffers allocated
    ULONG32		m_uSize;		// size of each buffer in bytes
    ULONG32		m_Count;		// number of buffers initially requested
    CHXSimpleList	m_freeList;		// list of free buffers
    INT32		m_ref;			// reference count for lifetime control
    HX_BITFIELD		m_bThreadSafe : 1;
    HX_BITFIELD		m_bUseGlobalAlloc : 1;	// Determines whether we use GlobalAlloc or new
    CHXString           m_strIdentifier;
#ifdef WIN32
    CRITICAL_SECTION    m_critsec;
    HKEY                m_hkey;
#else	// WIN32
    IHXMutex*		m_pMutex;
#endif	// WIN32

    // For more efficient memory use, these allocators can use past pool size samples
    // to estimate the number of buffers required, and trim appropriately:
    // The maximum number of buffers allocated is the maximum number of buffers in 
    // use over the last m_uiPoolHistoryDepth series of get calls.  The maximum 
    // free list size is then (max buffers in use - current buffers in use) + 1.  
    HXBOOL                m_bEstimateFreeListSize; 
    UINT32              m_uiPoolHistoryDepth;
    UINT32*             m_puiBufferUseHistory; 
    UINT32              m_uiCurrentHistoryIndex;
};

class CHXBufferMemoryAllocator : public IHX20MemoryAllocator
{
public:
    HX_RESULT QueryInterface(HX_IID iid, void** ppvObj);
    ULONG32	AddRef();
    ULONG32	Release();

    CHXBufferMemoryAllocator(IUnknown* pContext, HXBOOL bThreadSafe = FALSE);
    ~CHXBufferMemoryAllocator();

    UCHAR * 		GetPacketBuffer(IHXUnknown ** pPacketBuffer);
    HX_RESULT		SetProperties(HX20ALLOCPROPS* pRequest, HX20ALLOCPROPS* pActual);
    HX_RESULT		GetProperties(HX20ALLOCPROPS* pProps);
    UINT16		AddRefPacketPtr(UCHAR * memPtr);
    UINT16		ReleasePacketPtr(UCHAR * memPtr);

    UCHAR*		AddBuffer(IHXBuffer* pBuf);
    IHXBuffer*		GetBuffer(UCHAR* memPtr);

protected:
    IUnknown*		m_pContext;
#ifdef WIN32
    CRITICAL_SECTION    m_critsec;
    HXBOOL		m_bThreadSafe;
#else	// WIN32
    IHXMutex*		m_pMutex;
#endif	// WIN32
    CHXMapPtrToPtr	m_BufMap;		// map IHXBuffer's to their' Buffer.
    ULONG32		m_uSize;		// size of each buffer in bytes
    ULONG32		m_Count;		// number of buffers initially requested

    INT32		m_ref;			// reference count for lifetime control
};


#endif // __HXALLOC_H_
