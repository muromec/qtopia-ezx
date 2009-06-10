/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: memprb.cpp,v 1.7 2007/07/06 20:35:08 jfinnecy Exp $
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
 * terms of the GNU General Public License Version 2 (the
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

/****************************************************************************
 *  Defines
 */
#define MAX_DUMPLINE_LENGTH	1024
#define DUMPBLOCKID_START_CHAR	'{'
#define DUMPMEMUSED_START_CHAR	','
#define DUMPDATA_START_STRING	" Data:"	

#define mem_max(x, y)	((x) > (y) ? (x) : (y))


/****************************************************************************
 *  Includes
 */
#ifdef _DEBUG
#include <io.h>

#include <stdio.h>
#include <string.h>
#include <malloc.h>
#endif	// _DEBUG

#include "hxassert.h"
#include "hxmemprb.h"


/****************************************************************************
 *  Locals
 */
#ifdef _DEBUG
static LONG32 CalcUsedMem(_CrtMemState *MemState, HXBOOL bWithCrt);


static const MemBlockInfo EmptyMemBlockInfo =
{
    0,
    0,
    NULL,
    NULL
};

CHXMemProbe* CHXMemProbe::mz_pActiveProbe = NULL;

HXBOOL CHXMemProbe::m_bMutexInitialized = FALSE;
CRITICAL_SECTION CHXMemProbe::m_Mutex;
#endif  /* _DEBUG */


CHXMemProbe::CHXMemProbe(HXBOOL bIsFrozen)
#ifdef _DEBUG
    :	m_bIsFrozen(TRUE),
	m_MemFreeze(EmptyMemBlockInfo),
	m_pMemFreezeIter(NULL),
	m_MemUsed(0),
	m_MemUsedWithCrt(0),
	m_MaxMemUsed(0),
	m_MaxMemUsedWithCrt(0),
	m_BaseMemUsed(0),
	m_BaseMemUsedWithCrt(0),
	m_pParent(NULL),
	m_pChild(NULL)
#endif /* _DEBUG */
{
#ifdef _DEBUG
    if (!m_bMutexInitialized)
    {
	InitializeCriticalSection(&m_Mutex);
	m_bMutexInitialized = TRUE;
    }

    _CrtSetDbgFlag( _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) |
		    _CRTDBG_ALLOC_MEM_DF | 
		    _CRTDBG_CHECK_CRT_DF);

    _CrtMemCheckpoint(&m_MemRefPoint);
    m_MemFreezePoint = m_MemRefPoint;

    if (!bIsFrozen)
    {
	Reset();  // activate the probe
    }
#endif  /* _DEBUG */
}


CHXMemProbe::~CHXMemProbe()
{
#ifdef _DEBUG
    if (!m_bIsFrozen)
    {
	Freeze();
    }
    DestroyBlockInfo(m_MemFreeze);
#endif  /* _DEBUG */
}


HXBOOL CHXMemProbe::IsRestored(HXBOOL bWithCrt)
{
#ifdef _DEBUG
    HXBOOL bSignificantDiff;
    _CrtMemState MemRefPointDiff;

    if (!m_bIsFrozen)
    {
	_CrtMemCheckpoint(&m_MemFreezePoint);
    }

    bSignificantDiff = _CrtMemDifference(&MemRefPointDiff, &m_MemRefPoint, &m_MemFreezePoint);

    if (bWithCrt)
    {
	return !bSignificantDiff;
    }

    return ((CalcUsedMem(&MemRefPointDiff, HXDBG_CLIENT_MEM) == 0) ? TRUE : FALSE);
#else  /* _DEBUG */
    return TRUE;
#endif  /* _DEBUG */
}


LONG32 CHXMemProbe::GetMemUsed(HXBOOL bWithCrt, HXBOOL bWithBase, CHXMemProbe *pReferenceProbe)
{
#ifdef _DEBUG
    LONG32 BaseMem;
    _CrtMemState MemRefPointDiff;
    
    if (bWithBase)
    {
	BaseMem = bWithCrt ? m_BaseMemUsedWithCrt : m_BaseMemUsed;

	if (pReferenceProbe != NULL)
	{
	    BaseMem -= (bWithCrt ? pReferenceProbe->m_BaseMemUsedWithCrt : pReferenceProbe->m_BaseMemUsed);
	}
    }
    else
    {
	BaseMem = 0;
    }

    if (!m_bIsFrozen)
    {
	_CrtMemCheckpoint(&m_MemFreezePoint);
    }

    (void) _CrtMemDifference(&MemRefPointDiff, &m_MemRefPoint, &m_MemFreezePoint);

    return (BaseMem + CalcUsedMem(&MemRefPointDiff, bWithCrt));
#else  /* DEBUG */
    return 0;
#endif  /* _DEBUG */
}


LONG32 CHXMemProbe::GetBaseMemUsed(HXBOOL bWithCrt)
{
#ifdef _DEBUG
    return bWithCrt ? m_BaseMemUsedWithCrt : m_BaseMemUsed;
#else  /* _DEBUG */
    return 0;
#endif  /* _DEBUG */
}


HX_RESULT CHXMemProbe::DumpMemUsedSinceReset(FILE *pFile, HXBOOL bWithCrt)
{
#ifdef _DEBUG
    FILE *pTempFile;
    char Buffer[MAX_DUMPLINE_LENGTH]; /* Flawfinder: ignore */
    HX_RESULT RetVal = HXR_OK;

    pTempFile = tmpfile();

    if (pTempFile == NULL)
    {
	RetVal = HXR_FAIL;
    }

    if (SUCCEEDED(RetVal))
    {
	_DumpMemUsedSinceReset(pTempFile, bWithCrt);

	if (fseek(pTempFile, 0, SEEK_SET) != 0)
	{
	    RetVal = HXR_FAIL;
	}
    }

    while (SUCCEEDED(RetVal) && (fgets(Buffer, sizeof(Buffer), pTempFile) != NULL))
    {
	if (fputs(Buffer, pFile) == EOF)
	{
	    RetVal = HXR_FAIL;
	}
    }

    if (pTempFile != NULL)
    {
	fclose(pTempFile);
    }

    return RetVal;
#endif  /* _DEBUG */

    return HXR_OK;
}


#ifdef _DEBUG
void CHXMemProbe::_DumpMemUsedSinceReset(FILE *pFile, HXBOOL bWithCrt)
{
    int OldFlag;
    int NewFlag;
    _HFILE hOldFile;
    _HFILE hNewFile;
    int OldMode;

    // Set Debug Flag
    OldFlag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
    if (bWithCrt)
    {
	NewFlag = OldFlag | _CRTDBG_CHECK_CRT_DF;
    }
    else
    {
	NewFlag = OldFlag & (~_CRTDBG_CHECK_CRT_DF);
    }

    (void) _CrtSetDbgFlag(NewFlag);

    // Set Output File
    if (pFile == NULL)
    {
	pFile = (FILE *) stdout;
    }

    hNewFile = (_HFILE) _get_osfhandle(_fileno(pFile));

    OldMode = _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
    hOldFile = _CrtSetReportFile(_CRT_WARN, hNewFile);

    // Dump
    _CrtMemDumpAllObjectsSince(&m_MemRefPoint);
    
    // Restore Debug Mode
    (void) _CrtSetDbgFlag(OldFlag);
    (void) _CrtSetReportMode(_CRT_WARN, OldMode);
    (void) _CrtSetReportFile(_CRT_WARN, hOldFile);
}
#endif  /* _DEBUG */


ULONG32 CHXMemProbe::GetMaxMemUsed(HXBOOL bWithCrt, HXBOOL bWithBase, CHXMemProbe *pReferenceProbe)
{
#ifdef _DEBUG
    LONG32 BaseMem;
    LONG32 MaxMem;

    if (bWithBase)
    {
	BaseMem = bWithCrt ? m_BaseMemUsedWithCrt : m_BaseMemUsed;

	if (pReferenceProbe != NULL)
	{
	    BaseMem -= (bWithCrt ? pReferenceProbe->m_BaseMemUsedWithCrt : pReferenceProbe->m_BaseMemUsed);
	}
    }
    else
    {
	BaseMem = 0;
    }

    if (m_pChild == NULL)
    {
	MaxMem = bWithCrt ? m_MaxMemUsedWithCrt : m_MaxMemUsed;	
    }
    else
    {
	LONG32 uChildMax;

	uChildMax = (LONG32) m_pChild->GetMaxMemUsed(bWithCrt, PNDBG_NOBASE_MEM);

	if (bWithCrt)
	{
	    MaxMem = mem_max(uChildMax + m_MemUsedWithCrt, m_MaxMemUsedWithCrt);
	}
	else
	{
	    MaxMem = mem_max(uChildMax + m_MemUsed, m_MaxMemUsed);
	}
    }

    return mem_max(BaseMem + MaxMem, 0);
#else  /* DEBUG */
    return 0;
#endif  /* _DEBUG */
}


LONG32 CHXMemProbe::GetGlobalMemUsed(HXBOOL bWithCrt)
{
#ifdef _DEBUG
    _CrtMemState MemRefPoint;

    _CrtMemCheckpoint(&MemRefPoint);

    return CalcUsedMem(&MemRefPoint, bWithCrt);
#else  /* _DEBUG */
    return 0;
#endif  /* _DEBUG */
}


ULONG32 CHXMemProbe::GetGlobalMaxMemUsed(void)
{
#ifdef _DEBUG
    _CrtMemState MemRefPoint;

    _CrtMemCheckpoint(&MemRefPoint);

    return (ULONG32) MemRefPoint.lHighWaterCount;
#else  /* _DEBUG */
    return 0;
#endif  /* _DEBUG */
}


ULONG32 CHXMemProbe::SetGlobalMemBreak(ULONG32 uMemBlockId)
{
#ifdef _DEBUG
    return _CrtSetBreakAlloc(uMemBlockId);
#else  /* _DEBUG */
    return 0;
#endif  /* _DEBUG */
}


HX_RESULT CHXMemProbe::Reset(void)
{
#ifdef _DEBUG
    if (!m_bMutexInitialized)
    {
	InitializeCriticalSection(&m_Mutex);
	m_bMutexInitialized = TRUE;
    }

    if (!m_bIsFrozen)
    {
	Freeze();
    }

    EnterCriticalSection(&m_Mutex);

    DestroyBlockInfo(m_MemFreeze);

    _CrtMemCheckpoint(&m_MemRefPoint);

    m_MemUsed = 0;
    m_MemUsedWithCrt = 0;
    m_MaxMemUsed = 0;
    m_MaxMemUsedWithCrt = 0;
    m_BaseMemUsed = 0;
    m_BaseMemUsedWithCrt = 0;

    m_pChild = NULL;
    m_pParent = mz_pActiveProbe;
    if (m_pParent != NULL)
    {
	m_pParent->m_pChild = this;

	m_BaseMemUsedWithCrt = m_pParent->m_MemUsedWithCrt + m_pParent->m_BaseMemUsedWithCrt;
	m_BaseMemUsed = m_pParent->m_MemUsed + m_pParent->m_BaseMemUsed;
    }
    mz_pActiveProbe = this;

    _CrtSetAllocHook(MemProbeHook);

    m_bIsFrozen = FALSE;
    
    LeaveCriticalSection(&m_Mutex);
#endif  /* _DEBUG */

    return HXR_OK;
}


HX_RESULT CHXMemProbe::Freeze(HXBOOL bGenerateDetailInfo, HXBOOL bWithCrt)
{
#ifdef _DEBUG
    HX_RESULT RetVal = HXR_OK;

    if (m_bIsFrozen)
    {
	return HXR_UNEXPECTED;
    }

    EnterCriticalSection(&m_Mutex);

    _CrtMemCheckpoint(&m_MemFreezePoint);

    // Fixup Parent
    if (m_pParent != NULL)
    {
	m_pParent->m_MaxMemUsedWithCrt = mem_max(  m_pParent->m_MaxMemUsedWithCrt, 
							m_pParent->m_MemUsedWithCrt + m_MaxMemUsedWithCrt);
	m_pParent->m_MemUsedWithCrt += m_MemUsedWithCrt;
	m_pParent->m_MaxMemUsed = mem_max( m_pParent->m_MaxMemUsed, 
					    m_pParent->m_MemUsed + m_MaxMemUsed);
	m_pParent->m_MemUsed += m_MemUsed;

	m_pParent->m_pChild = m_pChild;
    }

    if (m_pChild == NULL)
    {
	// Since there are no children, this must be the active probe
	HX_ASSERT(mz_pActiveProbe == this);
	mz_pActiveProbe = m_pParent;
    }
    else
    {
	// Collect Info from child and disown it
	LONG32 uChildMax;

	HX_ASSERT(!m_pChild->m_bIsFrozen);

	uChildMax = (LONG32) m_pChild->GetMaxMemUsed(PNDBG_ALL_MEM, PNDBG_NOBASE_MEM);
	m_MaxMemUsedWithCrt = mem_max( m_MaxMemUsedWithCrt,
					m_MemUsedWithCrt + uChildMax);
	    
	uChildMax = m_pChild->GetMaxMemUsed(HXDBG_CLIENT_MEM, PNDBG_NOBASE_MEM);
	m_MaxMemUsed = mem_max(m_MaxMemUsed,
				m_MemUsed + uChildMax);

	m_pChild->m_pParent = m_pParent;
    }

    m_pParent = NULL;
    m_pChild = NULL;

    if (SUCCEEDED(RetVal))
    {
	DestroyBlockInfo(m_MemFreeze);
	if (bGenerateDetailInfo)
	{
	    RetVal = MakeBlockInfo(m_MemFreeze, bWithCrt);
	}
    }
    
    m_bIsFrozen = TRUE;
    
    if (mz_pActiveProbe == NULL)
    {
	m_bMutexInitialized = FALSE;
	DeleteCriticalSection(&m_Mutex);
    }
    else
    {
	LeaveCriticalSection(&m_Mutex);
    }

    return RetVal;
#else  /* DEBUG */
    return HXR_OK;
#endif  /* _DEBUG */
}


const char* CHXMemProbe::SearchMemFrozen(ULONG32 uMemBlockId)
{
#ifdef _DEBUG
    MemBlockInfo *pInfo;

    for (pInfo = m_MemFreeze.pNext; pInfo != NULL; pInfo = pInfo->pNext)
    {
	if (pInfo->uId == uMemBlockId)
	{
	    return pInfo->pDetail;
	}
    }
#endif  /* _DEBUG */

    return NULL;
}


HX_RESULT CHXMemProbe::GetFirstMemFrozen(ULONG32 &uMemBlockId, ULONG32 &uMemUsed)
{
#ifdef _DEBUG
    m_pMemFreezeIter = m_MemFreeze.pNext;

    if (m_pMemFreezeIter != NULL)
    {
	uMemBlockId = m_pMemFreezeIter->uId;
	uMemUsed = m_pMemFreezeIter->uMemUsed;

	return HXR_OK;
    }
#endif  /* _DEBUG */

    return HXR_FAIL;
}


HX_RESULT CHXMemProbe::GetNextMemFrozen(ULONG32 &uMemBlockId, ULONG32 &uMemUsed)
{
#ifdef _DEBUG
    if (m_pMemFreezeIter != NULL)
    {
	m_pMemFreezeIter = m_pMemFreezeIter->pNext;
    }

    if (m_pMemFreezeIter != NULL)
    {
	uMemBlockId = m_pMemFreezeIter->uId;
	uMemUsed = m_pMemFreezeIter->uMemUsed;

	return HXR_OK;
    }
#endif  /* _DEBUG */

    return HXR_FAIL;
}


/****************************************************************************
 *  Private Methods
 */
#ifdef _DEBUG

HX_RESULT CHXMemProbe::MakeBlockInfo(MemBlockInfo &Info, HXBOOL bWithCrt)
{
    FILE *pTempFile;
    HX_RESULT RetVal = HXR_OK;
    char Buffer[MAX_DUMPLINE_LENGTH]; /* Flawfinder: ignore */
    char *pString;
    ULONG32 uId;
    ULONG32 uMemUsed;

    // Create temporary file
    pTempFile = tmpfile();

    if (pTempFile == NULL)
    {
	RetVal = HXR_FAIL;
    }

    // Dump Current Heap State
    if (SUCCEEDED(RetVal))
    {
	_DumpMemUsedSinceReset(pTempFile, bWithCrt);

	if (fseek(pTempFile, 0, SEEK_SET) != 0)
	{
	    RetVal = HXR_FAIL;
	}
    }

    // Parse Dump
    while (SUCCEEDED(RetVal) && (fgets(Buffer, sizeof(Buffer), pTempFile) != NULL))
    {
	if (strncmp(Buffer, DUMPDATA_START_STRING, strlen(DUMPDATA_START_STRING)) == 0)
	{
	    // skip the data section
	    continue;
	}

	pString = strchr(Buffer, DUMPBLOCKID_START_CHAR);
	if (pString != NULL)
	{
	    pString++;
	    if (sscanf(pString, "%ld", &uId) != 1)
	    {
		RetVal = HXR_PARSE_ERROR;
	    }

	    if (SUCCEEDED(RetVal))
	    {
		pString = strchr(pString, DUMPMEMUSED_START_CHAR);
		if (pString == NULL)
		{
		    RetVal = HXR_PARSE_ERROR;
		}
		else
		{
		    pString++;
		    if (sscanf(pString, "%ld", &uMemUsed) != 1)
		    {
			RetVal = HXR_PARSE_ERROR;
		    }
		}
	    }

	    if (SUCCEEDED(RetVal))
	    {
		pString = strchr(pString, '\n');
		if (pString != NULL)
		{
		    *pString = '\0';
		}
	    }

	    if (SUCCEEDED(RetVal))
	    {
		RetVal = AddBlockInfo(Info, uId, uMemUsed, Buffer);
	    }
	}
    }


    if (pTempFile != NULL)
    {
	fclose(pTempFile);
    }

    return RetVal;
}


HX_RESULT CHXMemProbe::AddBlockInfo(MemBlockInfo &Info, ULONG32 uId, ULONG32 uMemUsed, const char *pDetail)
{
    MemBlockInfo *pNewInfo = NULL;
    char *pNewDetail = NULL;

    HX_RESULT RetVal = HXR_OK;

    pNewInfo = (MemBlockInfo *) _malloc_dbg(sizeof(MemBlockInfo),
					    _CRT_BLOCK, 
					    __FILE__, 
					    __LINE__);

    if (pNewInfo == NULL)
    {
	RetVal = HXR_OUTOFMEMORY;
    }

    if (SUCCEEDED(RetVal))
    {
	if (pDetail != NULL)
	{
	    pNewDetail = (char *) _calloc_dbg(  strlen(pDetail) + 1, 
						sizeof(char), 
						_CRT_BLOCK, 
						__FILE__, 
						__LINE__);

	    if (pNewDetail == NULL)
	    {
		RetVal = HXR_OUTOFMEMORY;
	    }
	    else
	    {
		strcpy(pNewDetail, pDetail); /* Flawfinder: ignore */
	    }
	}
    }

    if (SUCCEEDED(RetVal))
    {
	pNewInfo->uId = uId;
	pNewInfo->uMemUsed = uMemUsed;
	pNewInfo->pDetail = pNewDetail;

	pNewInfo->pNext = Info.pNext;
	Info.pNext = pNewInfo;
    }
    else
    {
	if (pNewInfo != NULL)
	{
	    free(pNewInfo);
	}
	if (pNewDetail != NULL)
	{
	    free(pNewDetail);
	}
    }

    return RetVal;
}


void CHXMemProbe::DestroyBlockInfo(MemBlockInfo &Info)
{
    MemBlockInfo *pInfo;
    MemBlockInfo *pDeadInfo;

    m_pMemFreezeIter = NULL;

    pInfo = Info.pNext;
    
    while(pInfo != NULL)
    {
	pDeadInfo = pInfo;

	pInfo = pInfo->pNext;

	if (pDeadInfo->pDetail != NULL)
	{
	    free(pDeadInfo->pDetail);
	}
	free(pDeadInfo);
    }

    Info = EmptyMemBlockInfo;
}

#if (defined(_MSC_VER) && (_MSC_VER > 1100) && defined(_BASETSD_H_)) /* VC 6 Check */
int __cdecl CHXMemProbe::MemProbeHook(int allocType, void *userData, size_t size, int blockType,
					     long requestNumber, const unsigned char *filename, int lineNumber)
#else
int __cdecl CHXMemProbe::MemProbeHook(int allocType, void *userData, size_t size, int blockType,
					     long requestNumber, const char *filename, int lineNumber)
#endif
{
    if (mz_pActiveProbe != NULL)
    {
	if (allocType == _HOOK_REALLOC)
	{
	    size_t old_size;

	    old_size = _msize_dbg(userData, blockType);

	    if (size > old_size)
	    {
		size = size - old_size;
		allocType = _HOOK_ALLOC;
	    }
	    else if (size < old_size)
	    {
		size = old_size - size;
		allocType = _HOOK_FREE;
	    }
	}
	else if (allocType == _HOOK_FREE)
	{
	    size = _msize_dbg(userData, blockType);
	}

	switch(allocType)
	{
	case _HOOK_ALLOC:
	    mz_pActiveProbe->m_MemUsedWithCrt += size;
	    if (mz_pActiveProbe->m_MemUsedWithCrt > ((LONG32) mz_pActiveProbe->m_MaxMemUsedWithCrt))
	    {
		mz_pActiveProbe->m_MaxMemUsedWithCrt = mz_pActiveProbe->m_MemUsedWithCrt;
	    }

	    if (blockType != _CRT_BLOCK)
	    {
		mz_pActiveProbe->m_MemUsed += size;
		if (mz_pActiveProbe->m_MemUsed > ((LONG32) mz_pActiveProbe->m_MaxMemUsed))
		{
		    mz_pActiveProbe->m_MaxMemUsed = mz_pActiveProbe->m_MemUsed;
		}
	    }
	    break;

	case _HOOK_FREE:
	    mz_pActiveProbe->m_MemUsedWithCrt -= size;

	    if (blockType != _CRT_BLOCK)
	    {
		mz_pActiveProbe->m_MemUsed -= size;
	    }
	    break;
	
	case _HOOK_REALLOC:
	    // nothing to do: no size change
	    break;

	default:
	    HX_ASSERT(FALSE);
	    break;
	}
    }

    return TRUE;
}
#endif  /* _DEBUG */


/****************************************************************************
 *  Helper Functions
 */
#ifdef _DEBUG

static LONG32 CalcUsedMem(_CrtMemState *MemState, HXBOOL bWithCrt)
{
    LONG32 UsedMem = 0;

    if (MemState != NULL)
    {
	int BlockType;

	for (BlockType = 0; BlockType < _MAX_BLOCKS; BlockType++)
	{
	    if ((BlockType != _FREE_BLOCK) && ((BlockType != _CRT_BLOCK) || bWithCrt))
	    {
		UsedMem += MemState->lSizes[BlockType];
		// printf("BlockType(%ld) Block(%ld) Size = %ld\n", BlockType, i, MemState->lSizes[BlockType]);
	    }
	}
    }

    return UsedMem;
}

#endif  /* _DEBUG */
