/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: chunkres.h,v 1.11 2006/05/19 05:55:38 pankajgupta Exp $
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

///////////////////////////////////////////////////////////////
// 
//	chunkres.h
//
//	A Brief History:
//
//	The previous implementation of the CHXHttp object basically 
//	allowed access to an entire resource as a single block of 
//	contiguous memory. This was not acceptable for large 
//	resources in 16bit version of the player, since FAR pointers
//	can only access 64KB of contiguous memory. Another downside 
//	to this implementation was that for very large HTTP 
//	resources (300K or more) the entire resource had to reside 
//	in memory at all times. Although the win 3.1 virtual memory 
//	manager handles this situation gracefully, it is of course
//	painful for low end machines.
//	
//	Another important "requirement" of this system is that 
//	relative to file storage the system should be relatively 
//	secure for intellectual property rights. In particualr, it 
//	should not be readily obvious (from the external file 
//	system) which file chunks are associate with a particular 
//	resource.
//	

#ifndef _CHUNKRES_H_
#define _CHUNKRES_H_


#include "hxtypes.h"
#include "hxresult.h"
#include "carray.h"
#include "hxmap.h"
#include "hxslist.h"
#include "chxdataf.h"
#include "hxthread.h"
#include "hxengin.h"

class CChunkyResMgr;
class CChunkyRes;
class CChunkyResChunk;

//////////////////////////////////////////////////////////////////
// To prevent temp files from looking like hxfiles, 
// we start temp file chunks at an arbitrary offset.
#define DEF_START_CHUNK_OFFSET				1

#define DEF_CHUNKYRES_DISK_THRESHOLD		0x00300000		//   3 MB

//#define DEF_CHUNKYRES_MEM_THRESHOLD		0x00008000		//  32 KB
//#define DEF_CHUNKYRES_MEM_THRESHOLD		0x00080000		// 512 KB
#define DEF_CHUNKYRES_MEM_THRESHOLD			0x00040000		// 256 KB

//#define DEF_CHUNKYRES_CHUNK_SIZE			0x00002000		//   8 KB
#define DEF_CHUNKYRES_CHUNK_SIZE			0x00008000		//  32 KB

///////////////////////////////////////////////////////////////
//
// CChunkyResMgr has:
// 
// 	* hashTable of "resource names" to CChunkyRes objects 
// 	  for open resources
// 	* hashTable of "resource names" to CChunkyRes objects
// 	  for closed resources
// 	* Manages discarding resources if disk usage is too high.
// 	* All resources are discarded when session is shut down,
// 	  no cross-session cache.
// 
// 	* Opens or creates a new resource
// 
// 	HX_RESULT CChunkyResMgr::OpenResource
// 			(CChunkyRes** ppChunkyRes, const char* pResName);
// 
// 
// 	* Closes a resource (closed resources may be discarded)
// 
// 	HX_RESULT CChunkyResMgr::CloseResource
// 			(CChunkyRes* pChunkyRes);
// 			-or-
// 			(const char* pResName);
// 
// 
// 	* Discards a resource (frees all disk and memory usage 
//	  for that resource)
//
// 	HX_RESULT CChunkyResMgr::DiscardResource
// 			(const char* pResName);
// 
// 	static void CChunkyResMgr::SetDiskUsageThreshold
// 			(ULONG32 diskUsage);
//

class CChunkyResMgr
{
private:
	IUnknown*		m_pContext;
	CHXMapStringToOb	m_OpenResources;
	CHXMapStringToOb	m_ClosedResources;
	CHXStringList		m_LRUResources;
	ULONG32			m_ulDiskUsage;

	void				DiscardDiskData(void);
	void				RemoveFromLRU(const char* pResName);
public:
	HX_RESULT			OpenResource(CChunkyRes** ppChunkyRes, const char* pResName);
	HX_RESULT			CloseResource(CChunkyRes* pChunkyRes);
	HX_RESULT			CloseResource(const char* pResName);
	HX_RESULT			DiscardResource(const char* pResName);
	HX_RESULT			FindResource(const char* pResName);

	void				SetDiskUsageThreshold(ULONG32 diskUsage);


	CChunkyResMgr(IUnknown* pContext);
	~CChunkyResMgr();
}; 	


///////////////////////////////////////////////////////////////
// 
// CChunkyRes has:
// 	
// 	* array of CChunkyResChunk's which store N-k chunks or the
// 	  resource. It can simply map from offset request, to array element 
// 	  by dividing offset by "chunking factor".
// 
// 	ULONG32 CChunkyRes::GetDiskUsage() const;
// 
// 	HX_RESULT CChunkyRes::GetData
// 			(ULONG32 offset, char* buf, 
// 			ULONG32 count, ULONG32* actual);
// 	HX_RESULT CChunkyRes::SetData
// 			(ULONG32 offset, const char* buf, 
// 			ULONG32 count);
// 
// 	* Does not manage which chunks are in or out of memory,
// 	  this is handled by the Chunk class.
// 
// 	* When resource is deleted, chunks are deleted.
// 
class CChunkyRes
{
private:
	friend class CChunkyResChunk;

	CHXPtrArray			m_Chunks;
	CHXString			m_strTempFileName;
	ULONG32				m_ulNextTempFileChunk;
	HXBOOL				m_bHasBeenOpened;
	HXBOOL				m_bDisableDiskIO;
	HXBOOL				m_bDiscardUsedData;
	UINT32				m_ulFirstChunkIdx;
	UINT32				m_ulUsedBytes;
	CHXSimpleList			m_FreeDiskOffsets;
	IHXMutex*			m_pMutex;

	ULONG32				m_MemUsageThreshold;
	ULONG32				m_CurMemUsage;

	// Those chunks used recently enough to be in memory.
	CHXSimpleList*			m_ChunksMemoryMRU; 
	// Those chunks not used recently enough, and therefor
	// spilled to disk.
	CHXSimpleList*			m_ChunksDiskMRU;
	// 	aka chunking factor
	ULONG32				m_ChunkSize;

	HX_RESULT			DiscardDiskData();

public:
	void				DisableDiskIO	() { m_bDisableDiskIO = TRUE; };
	void				DiscardUsedData	() { m_bDiscardUsedData = TRUE; };
	HX_RESULT			DiscardRange( ULONG32 offset, ULONG32 count );
	ULONG32				GetDiskUsage() const;
    ULONG32             GetCurrentMemoryUsage() const;
	HX_RESULT			GetData
							(
								ULONG32 offset, char* buf, 
								ULONG32 count, ULONG32* actual
							);

	HX_RESULT			SetData
							(
								ULONG32 offset, const char* buf, 
								ULONG32 count, void* pOwner = NULL
							);
	void				Lock()
						{
						    HX_ASSERT(m_pMutex);
						    m_pMutex->Lock();
						}
	void				Unlock()
						{
						    HX_ASSERT(m_pMutex);
						    m_pMutex->Unlock();
						}

	HX_RESULT			GetContiguousDataPointer(ULONG32 offset, char*& buf, ULONG32 count);
	HXBOOL				HasPartialData(ULONG32 length, ULONG32 offset = 0);
	ULONG32				GetContiguousLength(ULONG32 offset = 0);

	HX_RESULT			GetTempFileChunk(CHXDataFile*& pFile,ULONG32& m_ulTempFileOffset);
	HX_RESULT			GetTempFile(CHXDataFile*& pFile);

	void				SetMemUsageThreshold(ULONG32 memUsage);
	void				TrimDownMemoryMRU();

	CChunkyRes(IUnknown* pContext);
	~CChunkyRes();
        
        void AddCursor(void* pCursorOwner, ULONG32 ulCursorLocation = 0);
        void SetCursor(void* pCursorOwner, ULONG32 ulCursorLocation);
        void RemoveCursor(void* pCursorOwner);
        ULONG32 CountCursors();
        HX_RESULT GetNthCursorInformation(int nIndex, REF(void*)pCursorOwner, REF(ULONG32)ulCursorLocation);
        HX_RESULT GetCursorInformation(void* pInCursorOwner, REF(ULONG32)ulOutCursorLocation);
        
private:
	IUnknown*	m_pContext;
        CHXMapPtrToPtr	m_CursorMap;
}; 	

///////////////////////////////////////////////////////////////
// 
// CChunkyResChunkGroup is:
//
// The CChunkyResChunkGroup is a class that represents chunks
// in the chunk file. It is used to track whether the contents
// of a chunk are valid on disc or not.
//
// 
class CChunkyResChunkGroup
{
private:
public:
}; 	

///////////////////////////////////////////////////////////////
// 
// CChunkyResChunk has:
//
// 	* base offset for this chunk
// 	* length of this chunk (should be chunking factor for chunks 
// 	  1 to (N-1) of the CChunkyRes
// 	* partial length (less than length in cases where chunks are
// 	  still downloading).
// 	* file name of temporary file which stores chunk.
// 	* memory location of chunk (if in memory).
// 
// 	HX_RESULT CChunkyResChunk::GetData
// 				(ULONG32 offset, char* buf, 
// 				ULONG32 count, ULONG32* actual);
// 	HX_RESULT CChunkyResChunk::SetData
// 				(ULONG32 offset, const char* buf, 
// 				ULONG32 count);
// 	HX_RESULT CChunkyResChunk::SpillToDisk();
// 	HX_RESULT CChunkyResChunk::LoadFromDisk();
// 	HX_RESULT CChunkyResChunk::DiscardDiskData();
// 
// 	static void CChunkyResChunk::SetMemUsageThreshold
// 			(ULONG32 memUsage);
// 
// 	* a static MRU of CChunkyResChunk's to keep memory usage low.
// 
// 	* SpillToDisk() and LoadFromDisk() are only called from
// 	  within this class. All memory management handled by
// 	  this classes static MRU management.
// 
// 	* When chunk is deleted, data is discarded from disk.
// 
class CChunkyResChunk
{
private:
	//	CChunkyres needs access to chunk size.
	friend class CChunkyRes;

private:
// 	* base offset for this chunk
	ULONG32					m_ChunkOffset;

// 	* memory location of chunk (if in memory).
	UCHAR*					m_pChunkData;

//	* offset of this chunk into temp file...
	ULONG32					m_ulTempFileOffset;

//	* Flag indicating that we have previously spilled to disk
	HXBOOL					m_bPreviouslySpilled;

//	* Flag indicating that we have been modified since last being spilled!
	HXBOOL					m_bModified;

//	* This is the resource we belong to...
	CChunkyRes*				m_pChunkRes;

//	* Flag indicating we should never spill to disk
	HXBOOL					m_bDisableDiskIO;
	
//	* list to hold valid ranges for this chunk
	struct ValidRange
	{
		ULONG32	offset;
		ULONG32 length;
	};
	CHXSimpleList				m_ValidRanges;
	
	HX_RESULT				AddValidRange(ULONG32 offset, ULONG32 length, HXBOOL bValid = TRUE);

	HX_RESULT				SpillToDisk();
	HX_RESULT				LoadFromDisk();
	HX_RESULT				DiscardDiskData();
	HX_RESULT				MakeSureChunkIsInMemory();
	void					Lock()
							{
							    HX_ASSERT(m_pChunkRes);
							    m_pChunkRes->Lock();
							}
	void					Unlock()
							{
							    HX_ASSERT(m_pChunkRes);
							    m_pChunkRes->Unlock();
							}

public:
	void					DisableDiskIO() { m_bDisableDiskIO = TRUE; };
	
	ULONG32					GetValidLength(ULONG32 offset = 0) const;

	ULONG32					GetSize() const
								{
									return m_pChunkRes->m_ChunkSize;
								};

	ULONG32					GetTempFileOffset() const
								{
									return m_ulTempFileOffset;
								};

	HX_RESULT				GetData
								(
									ULONG32 offset, char* buf, 
									ULONG32 count, ULONG32* actual
								);

	HX_RESULT				SetData
								(
									ULONG32 offset, const char* buf, 
									ULONG32 count
								);


	HX_RESULT				GetContiguousDataPointer(ULONG32 offset, char*& buf, ULONG32 count);

	CChunkyResChunk(CChunkyRes* pChunkyRes);
	~CChunkyResChunk();

}; 	

#endif // ndef _CHUNKRES_H_

