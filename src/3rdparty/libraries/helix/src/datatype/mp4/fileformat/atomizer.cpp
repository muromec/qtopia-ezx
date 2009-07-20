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

/****************************************************************************
 *  Defines
 */
#define ATMZR_SEEK_RELATIVE  TRUE
#define ATMZR_SEEK_ABSOLUTE  FALSE

#define ATMZR_MAX_RECURSION_LEVEL   20
#ifdef HELIX_FEATURE_MIN_HEAP
#define ATMZR_PAGING_THRESHOLD	    0x00000400	// 1K
#else	// HELIX_FEATURE_MIN_HEAP
#define ATMZR_PAGING_THRESHOLD	    0x0000FFFF	// 64K
#endif	// HELIX_FEATURE_MIN_HEAP

#define ATMZR_MAX_OFFSET	    0xFFFFFFFF


/****************************************************************************
 *  Includes
 */
#include "atomizer.h"
#include "qtatoms.h"
#include "mempager.h"
#include "qtffrefcounter.h"
#include "hxerror.h"
#if defined(HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS)
#include "hxprdnld.h"       // IHXMediaBytesToMediaDur, IHXPDStatusMgr
#endif /* #if defined(HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS) */

/****************************************************************************
 *  Data Types
 */
struct QTHeader
{
    UINT8 pSize[4];
    UINT8 pType[4];
};

struct QTNewHeader
{
    UINT8 pSize[4];
    UINT8 pType[4];
    UINT8 pAtomID[4];
    UINT8 pRsvd1[2];
    UINT8 pChildCount[2];
    UINT8 pRsvd2[4];
};


/****************************************************************************
 *  Class CAtomizer
 */
/****************************************************************************
 *  Constructor/Destructor
 */
CAtomizer::CAtomizer(void)
    : m_State(ATMZR_Offline)
    , m_pFileSwitcher(NULL)
    , m_pResponse(NULL)
    , m_pCommander(NULL)
    , m_pScheduler(NULL)
    , m_bSyncAccessEnabled(FALSE)
    , m_bPreferLinearAccess(FALSE)
    , m_pRoot(NULL)
    , m_pCurrentRoot(NULL)
    , m_pNewAtom(NULL)
    , m_ulStartOffset(0)
    , m_ulFinalOffset(0)
    , m_ulCurrentOffset(0)
    , m_ulNewAtomOffset(0)
    , m_ulNewAtomDataSize(0)
    , m_AtomType(0)
    , m_ulTotalSize(0)
    , m_pRecursionCallback(NULL)
    , m_ulRecursionCount(0)
    , m_lRefCount(0)
    , m_pErrorMessages(NULL)
{
    g_nRefCount_qtff++;
}


CAtomizer::~CAtomizer()
{
    Close();
    g_nRefCount_qtff--;
}


/****************************************************************************
 *  Main Interface
 */
/****************************************************************************
 *  Init
 */
HX_RESULT CAtomizer::Init(IUnknown *pSource,
			  IUnknown *pResponse,
			  IUnknown *pCommander)
{
    HX_RESULT retVal;

    if ((m_State != ATMZR_Offline) &&
	(m_State != ATMZR_Ready))
    {
	return HXR_UNEXPECTED;
    }

    HX_ASSERT(pSource);
    HX_ASSERT(pResponse);

    // Reset
    HX_RELEASE(m_pFileSwitcher);
    HX_RELEASE(m_pResponse);
    HX_RELEASE(m_pCommander);
    HX_RELEASE(m_pRoot);
    HX_RELEASE(m_pNewAtom);
    HX_RELEASE(m_pRecursionCallback);
    HX_RELEASE(m_pErrorMessages);
    m_ulCurrentOffset = 0;
    m_ulRecursionCount = 0;

    // Find Input Interface
    retVal = pSource->QueryInterface(IID_IHXFileSwitcher, (void**) &m_pFileSwitcher);

    // Find out if syncrhonous use of input is possible
    if (SUCCEEDED(retVal))
    {
	if (SUCCEEDED(m_pFileSwitcher->Advise(HX_FILEADVISE_SYNCACCESS)))
	{
	    m_bSyncAccessEnabled = TRUE;
	    m_pFileSwitcher->Advise(HX_FILEADVISE_ASYNCACCESS);
	}
	
	HX_RESULT adviseRes = 
	    m_pFileSwitcher->Advise(HX_FILEADVISE_RANDOMACCESS);

	if (HXR_ADVISE_PREFER_LINEAR == adviseRes)
	{
	    m_bPreferLinearAccess = TRUE;
	}
    }

    // Find Output Interface
    if (SUCCEEDED(retVal))
    {
	retVal = pResponse->QueryInterface(IID_IHXAtomizerResponse, (void**) &m_pResponse);
    }

    // Find Scheduler Interface
    if (SUCCEEDED(retVal))
    {
	retVal = pResponse->QueryInterface(IID_IHXScheduler, (void **) &m_pScheduler);

	if (FAILED(retVal))
	{
	    retVal = pSource->QueryInterface(IID_IHXScheduler, (void **) &m_pScheduler);
	}
    }

    // See if the Atomization Commander is given
    if (SUCCEEDED(retVal))
    {
	if (pCommander)
	{
	    pCommander->QueryInterface(IID_IHXAtomizationCommander, (void**) &m_pCommander);
	}

	if (!m_pCommander)
	{
	    // No commander given, Atomizer will command itself
	    m_pCommander = (IHXAtomizationCommander *) this;
	    m_pCommander->AddRef();
	}

	m_State = ATMZR_Ready;
    }

    if (SUCCEEDED(retVal) && !m_pErrorMessages)
    {
        pResponse->QueryInterface(IID_IHXErrorMessages, (void**) &m_pErrorMessages);
    }
    // Allocate Recursion breaker callback
#ifdef QTCONFIG_RECURSION_PROTECTION
    if (SUCCEEDED(retVal))
    {
	m_pRecursionCallback = new CRecursionCallback(this);
	if (m_pRecursionCallback)
	{
	    m_pRecursionCallback->AddRef();
	}
	else
	{
	    retVal = HXR_OUTOFMEMORY;
	}
    }
#endif	// QTCONFIG_RECURSION_PROTECTION

    return retVal;
}


/****************************************************************************
 *  Close
 */
void CAtomizer::Close(void)
{
    m_State = ATMZR_Offline;

    HX_RELEASE(m_pFileSwitcher);
    HX_RELEASE(m_pResponse);
    HX_RELEASE(m_pCommander);
    HX_RELEASE(m_pScheduler);
    HX_RELEASE(m_pRoot);
    HX_RELEASE(m_pNewAtom);
    HX_RELEASE(m_pRecursionCallback);
    HX_RELEASE(m_pErrorMessages);
}


/****************************************************************************
 *  Atomize
 */
HX_RESULT CAtomizer::Atomize(ULONG32 ulOffset, ULONG32 ulSize)
{
    if (m_State != ATMZR_Ready)
    {
	return HXR_UNEXPECTED;
    }

    if ((m_pFileSwitcher == NULL) ||
	(m_pResponse == NULL) ||
	(m_pCommander == NULL))
    {
	return HXR_FAIL;
    }

    // Reset
    HX_RELEASE(m_pRoot);
    HX_RELEASE(m_pNewAtom);
    m_ulCurrentOffset = 0;

    // Take on parameters
    m_ulStartOffset = ulOffset;
    m_ulNewAtomOffset = ulOffset;
    m_ulTotalSize = ulSize;
    m_ulFinalOffset = (m_ulTotalSize == ATOMIZE_ALL) ? ATOMIZE_ALL : m_ulStartOffset + m_ulTotalSize;

    // Prepare the root
    m_pRoot = new CQTRootAtom();
    if (m_pRoot == NULL)
    {
	return HXR_OUTOFMEMORY;
    }
    m_pRoot->AddRef();

    m_pRoot->SetSize(m_ulTotalSize);
    m_pRoot->SetOffset(m_ulStartOffset);
    m_pCurrentRoot = m_pRoot;

    // Start of the atomization process
    m_State = ATMZR_ProcHeader;
    SeekDataCB(m_ulStartOffset);

    return HXR_OK;
}


/****************************************************************************
 *  Private Methods
 */
/****************************************************************************
 *  SeekData
 */
HX_RESULT CAtomizer::SeekData(ULONG32 ulOffset, HXBOOL bRelative)
{
    if (m_State == ATMZR_Offline)
    {
	return HXR_UNEXPECTED;
    }

    if (bRelative)
    {
	m_ulCurrentOffset += ulOffset;
    }
    else
    {
	m_ulCurrentOffset = ulOffset;
    }

    if (IsOffsetInRange(m_ulCurrentOffset))
    {
	HX_RESULT retVal;

	// Initiate the Seek
	retVal = m_pFileSwitcher->Seek(	ulOffset,
					bRelative,
					(IHXFileResponse *) this);

	if (FAILED(retVal))
	{
	    CompleteAtomization(retVal);
	}
    }
    else
    {
	CompleteAtomization(HXR_OK);
    }

    return HXR_ABORT;
}


/****************************************************************************
 *  ReadData
 */
HX_RESULT CAtomizer::ReadData(ULONG32 ulSize)
{
    HX_RESULT retVal = HXR_OK;

    if (m_State == ATMZR_Offline)
    {
	return HXR_UNEXPECTED;
    }

    if (m_State == ATMZR_ProcHeader)
    {
	retVal = AdjustCurrentRoot();

	if (FAILED(retVal))
	{
	    if (retVal == HXR_CHUNK_MISSING)
	    {
		// udta Atom terminator encountered, must skip over the
		// terminator chunk
		m_ulNewAtomOffset = m_ulCurrentOffset +
				    QT_UDTA_TERMINATOR_LENGTH;
		return SeekDataCB(m_ulNewAtomOffset);
	    }
	    else
	    {
		CompleteAtomization(retVal);

		return HXR_ABORT;
	    }
	}
    }

    m_ulCurrentOffset = (ulSize == FSWTCHR_READ_ALL) ? ATMZR_MAX_OFFSET : (m_ulCurrentOffset + ulSize);

    if (IsOffsetInRange(m_ulCurrentOffset - 1))
    {
	HX_RESULT retVal;

	// Initiate the Read
	retVal = m_pFileSwitcher->Read(	ulSize,
					(IHXFileResponse *) this);

	if (SUCCEEDED(retVal))
	{
	    return retVal;
	}
    }

    CompleteAtomization(retVal);

    return HXR_ABORT;
}


/****************************************************************************
 *  ReadDataCB
 */
HX_RESULT CAtomizer::ReadDataCB(ULONG32 ulSize)
{
#ifdef QTCONFIG_RECURSION_PROTECTION
    if (m_ulRecursionCount < ATMZR_MAX_RECURSION_LEVEL)
    {
	m_ulRecursionCount++;
	return ReadData(ulSize);
    }
    else
    {
	CallbackHandle retVal;

	m_ulRecursionCount = 0;
	m_ulSize = ulSize;
	m_CallbackStep = ATMZR_CBSTEP_Read;
	retVal = m_pScheduler->RelativeEnter(m_pRecursionCallback, 0);
	if (!retVal)
	{
	    CompleteAtomization(HXR_FAIL);
	}

	return HXR_OK;
    }
#else	// QTCONFIG_RECURSION_PROTECTION
    return ReadData(ulSize);
#endif	// QTCONFIG_RECURSION_PROTECTION
}


/****************************************************************************
 *  SeekDataCB
 */
HX_RESULT CAtomizer::SeekDataCB(ULONG32 ulOffset, HXBOOL bRelative)
{
#ifdef QTCONFIG_RECURSION_PROTECTION
    if (m_ulRecursionCount < ATMZR_MAX_RECURSION_LEVEL)
    {
	m_ulRecursionCount++;
	return SeekData(ulOffset, bRelative);
    }
    else
    {
	CallbackHandle retVal;

	m_ulRecursionCount = 0;
	m_ulSize = ulOffset;
	m_bRelative = bRelative;
	m_CallbackStep = ATMZR_CBSTEP_Seek;
	retVal = m_pScheduler->RelativeEnter(m_pRecursionCallback, 0);
	if (!retVal)
	{
	    CompleteAtomization(HXR_FAIL);
	}

	return HXR_OK;
    }
#else	// QTCONFIG_RECURSION_PROTECTION
    return SeekData(ulOffset, bRelative);
#endif	// QTCONFIG_RECURSION_PROTECTION
}


/****************************************************************************
 *  AdjustCurrentRoot
 */
HX_RESULT CAtomizer::AdjustCurrentRoot(void)
{
    CQTAtom *pRootParent;

    HX_ASSERT(m_pCurrentRoot);

    while ((m_pCurrentRoot->GetSize() != ATOMIZE_ALL) &&
	   ((m_pCurrentRoot->GetOffset() +
	     m_pCurrentRoot->GetSize() -
	     m_ulNewAtomOffset) <
	    QT_HEADER_SIZE))
    {
	if (m_ulNewAtomOffset ==
	    (m_pCurrentRoot->GetOffset() + m_pCurrentRoot->GetSize()))
	{
	    // Current Atom is completed, move to the parent
	    pRootParent = m_pCurrentRoot->GetParent();

	    if (pRootParent)
	    {
		m_pCurrentRoot = pRootParent;
		continue;
	    }
	    else
	    {
		// We are at the absolute root
		break;
	    }
	}
	else
	{
	    // Not enough room in current parent to read a header, but
	    // still some data left over
	    if ((m_pCurrentRoot->GetType() == QT_udta) &&
		((m_ulNewAtomOffset + QT_UDTA_TERMINATOR_LENGTH) ==
		(m_pCurrentRoot->GetOffset() + m_pCurrentRoot->GetSize())))
	    {
		// Legacy udta Atom termination - need to skip over the
		// terminator
		return HXR_CHUNK_MISSING;
	    }
	    else
	    {
		// Data is corrupted
		return HXR_PARSE_ERROR;
	    }
	}
    }

    return HXR_OK;
}


/****************************************************************************
 *  CompleteAtomization
 */
void CAtomizer::CompleteAtomization(HX_RESULT status)
{
    CQTAtom* pRoot;

    if (!m_pRoot)
    {
	return;
    }

    if (SUCCEEDED(status))
    {
	// Record the amount of data atomized
	CQTAtom *pRoot = m_pCurrentRoot;

	HX_ASSERT(pRoot);

	// If we have the position of where next atom would be,
	// compute unspecified atom sizes
	if (m_ulNewAtomOffset != ATOMIZE_ALL)
	{
	    while (pRoot)
	    {
		if (pRoot->GetSize() == ATOMIZE_ALL)
		{
		    pRoot->SetSize(m_ulNewAtomOffset - m_ulStartOffset);
		}

		// Move to the parent
		pRoot = pRoot->GetParent();
	    }

	    m_pRoot->SetSize(m_ulNewAtomOffset - m_ulStartOffset);
	}

	status = AdjustCurrentRoot();
    }

    if (SUCCEEDED(status))
    {
	// See if parsing fully completed
	if (((m_ulFinalOffset != ATOMIZE_ALL) &&
	     (m_ulFinalOffset != m_ulCurrentOffset)) ||
	    (m_pRoot != m_pCurrentRoot))
	{
	    status = HXR_CORRUPT_FILE;
	}
    }
    else
    {
	// Data compromizing failure occured during atomization
	HX_RELEASE(m_pRoot);
    }

    HX_RELEASE(m_pNewAtom);

    m_State = ATMZR_Ready;

    pRoot = m_pRoot;
    m_pRoot = NULL;
    m_pCurrentRoot = NULL;

    m_pResponse->AtomReady(status, pRoot);

    if (pRoot)
    {
	pRoot->Release();
    }
}


/****************************************************************************
 *  IHXFileResponse methods
 */
/////////////////////////////////////////////////////////////////////////
//  Method:
//	IHXFileResponse::ReadDone
//  Purpose:
//	Notification interface provided by users of the IHXFileObject
//	interface. This method is called by the IHXFileObject when the
//	last read from the file is complete and a buffer is available.
//
STDMETHODIMP CAtomizer::ReadDone
(
    HX_RESULT		status,
    IHXBuffer*		pBuffer
)
{
    HX_RESULT retVal = HXR_OK;
    QTAtomType AtomType = 0;
    ULONG32 ulAtomSize = 0;
    UINT8* pData = NULL;
    ULONG32 ulDataLen = 0;
    ULONG32 ulNormalizedLen = 0;
    ULONG32 ulAtomID = 0;
    UINT16 uChildCount = 0;
    HXBOOL bLoop = FALSE;

    if (SUCCEEDED(status) && (m_State != ATMZR_Offline))
    {
	HX_ASSERT(pBuffer);

	pBuffer->Get(pData, ulDataLen);
	ulNormalizedLen = ulDataLen;

	do
	{
	    bLoop = FALSE;

	    switch (m_State)
	    {
	    case ATMZR_ProcNewHeader:
		ulNormalizedLen -= (QT_NEW_HEADER_SIZE - QT_HEADER_SIZE);
		{
		    QTNewHeader *pHeader = (QTNewHeader *) (pData);

		    ulAtomID = CQTAtom::GetUL32(pHeader->pAtomID);
		    uChildCount = CQTAtom::GetUI16(pHeader->pChildCount);
		}
		// drop to case below

	    case ATMZR_ProcHeader:
		if (ulNormalizedLen == QT_HEADER_SIZE)
		{
		    QTHeader *pHeader = (QTHeader *) (pData);

		    AtomType = CQTAtom::GetUL32(pHeader->pType);
		    ulAtomSize = CQTAtom::GetUL32(pHeader->pSize);

		    if (AtomType == 0)
		    {
			if (m_State == ATMZR_ProcHeader)
			{
			    // Try to interpret this as CQTNewAtom
			    m_State = ATMZR_ProcNewHeader;
			    SeekDataCB(QT_NEW_HEADER_GAP, ATMZR_SEEK_RELATIVE);
			    return retVal;
			}
			status = HXR_PARSE_ERROR;
		    }
		    else if (ulAtomSize == 1)
		    {
			// This must be the Atom with extended size
			m_State = ATMZR_ProcExtendedSize;
			m_AtomType = AtomType;
			ReadDataCB(QT_EXTENDED_SIZE);
			return retVal;
		    }
		    else if (ulAtomSize == 0)
		    {
			// This Atom extends to the of enclosing container
			ulAtomSize = ATOMIZE_ALL;
		    }

		    // drop to case below;
		    m_State = ATMZR_MakeAtom;
		}
		else
		{
		    break;
		}

	    case ATMZR_MakeAtom:
		{
		    QTAtomizerCmd AtomCmd;

		    HX_ASSERT(!m_pNewAtom);

		    AtomCmd = m_pCommander->GetAtomCommand( AtomType,
							    m_pCurrentRoot);

		    if (ulAtomSize == ATOMIZE_ALL)
		    {
			if (m_pCurrentRoot->GetSize() != ATOMIZE_ALL)
			{
			    ulAtomSize = m_pCurrentRoot->GetOffset() +
					 m_pCurrentRoot->GetSize() -
					 m_ulNewAtomOffset;

			    HX_ASSERT(ulAtomSize != 0);

			    if (ulAtomSize == 0)
			    {
				status = HXR_PARSE_ERROR;
				break;
			    }
			}
		    }

		    switch (AtomCmd)
		    {
		    case ATMZR_CMD_LOAD:
		    case ATMZR_CMD_OUTLINE:
			m_pNewAtom = CreateQTAtom(  AtomType,
						    m_ulNewAtomOffset,
						    ulAtomSize,
						    NULL, // no parent
						    ulAtomID,
						    uChildCount);

			if (m_pNewAtom != NULL)
			{
			    if (m_pNewAtom->IsLeafType())
			    {
				// A Leaf Atom
				if (AtomCmd == ATMZR_CMD_LOAD)
				{
				    m_ulNewAtomDataSize = 0;
				    if (ulAtomSize != ATOMIZE_ALL)
				    {
					m_ulNewAtomDataSize = ulAtomSize - ulDataLen;
				    }

#ifndef QTCONFIG_NO_PAGING
				    if ((m_ulNewAtomDataSize > ATMZR_PAGING_THRESHOLD)
					&& m_pNewAtom->IsPagingAtom()
#ifdef QTCONFIG_NO_ASYNC_PAGING
					&& m_bSyncAccessEnabled
#endif	// QTCONFIG_NO_ASYNC_PAGING
#if defined(HELIX_FEATURE_PROGDOWN)
                    && m_bPreferLinearAccess == FALSE
#endif // HELIX_FEATURE_PROGDOWN 
#if defined(HELIX_FEATURE_S60_PROGDOWN)
                    && m_bPreferLinearAccess == FALSE
#endif // HELIX_FEATURE_S60_PROGDOWN
					&& (m_ulNewAtomDataSize != ATOMIZE_ALL)
				       )
				    {
					CMemPager* pMemPager = new CMemPager;

					status = HXR_OUTOFMEMORY;
					if (pMemPager)
					{
					    pMemPager->AddRef();
					    status = pMemPager->Init(m_pFileSwitcher,
								     m_ulCurrentOffset,
								     m_ulNewAtomDataSize);
					    if (SUCCEEDED(status))
					    {
						m_pNewAtom->AddRef();
						m_State = ATMZR_ProcBody;
						m_pNewAtom->SetMemPager(pMemPager);
						status = pMemPager->LoadPage((IHXFileResponse*) this);
					    }

					    pMemPager->Release();

					    if (SUCCEEDED(status))
					    {
						return retVal;
					    }
					}
					// Failure
					HX_ASSERT(status != HXR_OK);
					break;
				    }
				    else
#endif	// QTCONFIG_NO_PAGING
				    if ((m_ulNewAtomDataSize != 0) ||
					(ulAtomSize == ATOMIZE_ALL))
				    {
					// Leaf atom with data
					m_pNewAtom->AddRef();
					m_State = ATMZR_ProcBody;
					if (m_ulNewAtomDataSize != 0)
					{
					    ReadDataCB(m_ulNewAtomDataSize);
					    return retVal;
					}
					else
					{
					    //Read in the remainder of the file
					    ReadDataCB(FSWTCHR_READ_ALL);
					    return retVal;
					}
				    }
				    else
				    {
					// Leaf atom with no data
					m_pCurrentRoot->AddPresentChild(m_pNewAtom);
					m_ulNewAtomOffset = m_ulCurrentOffset;
					m_pNewAtom = NULL;
					m_State = ATMZR_ProcHeader;
					ReadDataCB(QT_HEADER_SIZE);
				    }
				}
				else
				{
				    // We are not to load the data
				    // so we are ready to attach the atom shell
				    // to the atom tree.
				    m_pCurrentRoot->AddPresentChild(m_pNewAtom);
				    m_pNewAtom = NULL;

				    if (ulAtomSize != ATOMIZE_ALL)
				    {
					m_ulNewAtomOffset = m_ulCurrentOffset +
							    ulAtomSize -
							    ulDataLen;

					m_State = ATMZR_ProcHeader;
					SeekDataCB(m_ulNewAtomOffset);
					return retVal;
				    }

				    // Location of the end of file - unknown
				    m_ulNewAtomOffset = ATOMIZE_ALL;
				}
			    }
			    else
			    {
				// A Non Leaf Atom - has no direct data
				m_pCurrentRoot->AddPresentChild(m_pNewAtom);

				m_pCurrentRoot = m_pNewAtom;
				m_ulNewAtomOffset = m_ulCurrentOffset;
				m_pNewAtom = NULL;
				m_State = ATMZR_ProcHeader;
				ReadDataCB(QT_HEADER_SIZE);
				return retVal;
			    }

			    break;
			}

			// If we couldn't create the atom, skip it.
			// Drop down to case below.

		    case ATMZR_CMD_SKIP:
			if (ulAtomSize != ATOMIZE_ALL)
			{
			    m_ulNewAtomOffset = m_ulCurrentOffset +
						ulAtomSize -
						ulDataLen;
			    m_State = ATMZR_ProcHeader;

                            //  Do this for all cases; if the file sys says it
                            // prefers linear access, then obey that (not just
                            // when HELIX_FEATURE_PROGDOWN is defined):
                            HX_RESULT adviseRes = m_pFileSwitcher->Advise(
                                    HX_FILEADVISE_RANDOMACCESS);
                            if (HXR_ADVISE_PREFER_LINEAR == adviseRes)
                            {
                                m_bPreferLinearAccess = TRUE;
                            }

			    /* Check to see if we are trying to skip
			     * an mdat atom while using a file object
			     * that prefers linear access (ie. HTTP/1.0)
			     */
			    if (m_bPreferLinearAccess && (QT_mdat == AtomType))
			    {
                // don't report HXR_INVALID_FILE if mp4 file moov atom at the end of file

				if ((m_pCurrentRoot) &&
				    (m_pCurrentRoot->FindPresentChild(QT_moov)))
				{
				    /* We have the moov atom already so
				     * this file is likely formatted for 
				     * progressive download
				     */
                     CompleteAtomization(HXR_OK);
                }
                else
                {
                if (m_pErrorMessages)
                {
                     m_pErrorMessages->Report(HXLOG_INFO, HXR_FULL_DOWNLOAD_NEEDED, 1, // we use 1 to tell the difference
                         (const char*) "This file is not optimized for progressive download", 
                         NULL);
                }
                                //if no moov in file, helix will report error in the end
                SeekDataCB(m_ulNewAtomOffset);
				}
			    }
			    else
			    {
				SeekDataCB(m_ulNewAtomOffset);
			    }
			    return retVal;
			}
			else
			{
			    m_ulNewAtomOffset = ATOMIZE_ALL;
			    break;
			}

			// If we couldn't skip the atom, stop.
			// Drop to case below

		    case ATMZR_CMD_STOP:
			break;

		    default:
			status = HXR_INVALID_PARAMETER;
			break;
		    }
		}
		break;

	    case ATMZR_ProcExtendedSize:
		if (ulNormalizedLen == QT_EXTENDED_SIZE)
		{
		    AtomType = m_AtomType;
		    ulAtomSize = CQTAtom::GetUL32(pData);

		    HX_ASSERT(ulAtomSize == 0);

		    if (ulAtomSize == 0)
		    {
			ulAtomSize = CQTAtom::GetUL32(pData + 4);
			// DataLen needs to reflect how much of the atom we have
			// processed thus far.  Since in case of extended size
			// atoms we take two reads to process the header,
			// increment the length to properly include all of the
			// bytes processed since the start of the atom.
			// Note: QTNewAtoms cannot have extended size.
			ulDataLen += QT_HEADER_SIZE;
			m_State = ATMZR_MakeAtom;
			bLoop = TRUE;
		    }
		    else
		    {
			// can't handle atom size beyond 4GB
			status = HXR_ABORT;
		    }
		}
		break;

	    case ATMZR_ProcBody:
		HX_ASSERT(m_pNewAtom);

		if (m_ulNewAtomDataSize != ATOMIZE_ALL)
		{
		    if ((!m_pNewAtom->IsPagingEnabled()) &&
			(ulDataLen == m_ulNewAtomDataSize))
		    {
			HX_ASSERT(pBuffer);

			m_pNewAtom->SetBuffer(pBuffer);
			m_pCurrentRoot->AddPresentChild(m_pNewAtom);
			m_ulNewAtomOffset = m_ulCurrentOffset;
			m_pNewAtom->Release();
			m_pNewAtom = NULL;
			m_State = ATMZR_ProcHeader;
			ReadDataCB(QT_HEADER_SIZE);
			return retVal;
		    }
		    else if (m_pNewAtom->IsPagingEnabled() &&
			     (ulDataLen != 0))
		    {
			m_ulCurrentOffset += ulDataLen;
			m_pCurrentRoot->AddPresentChild(m_pNewAtom);
			HX_ASSERT(m_ulNewAtomDataSize >= ulDataLen);
			m_ulNewAtomOffset = m_ulCurrentOffset +
					    m_ulNewAtomDataSize -
					    ulDataLen;
			m_pNewAtom->Release();
			m_pNewAtom = NULL;
			m_State = ATMZR_ProcHeader;
			if (m_ulNewAtomOffset != m_ulCurrentOffset)
			{
			    SeekDataCB(m_ulNewAtomOffset);
			}
			else
			{
			    ReadDataCB(QT_HEADER_SIZE);
			}
			return retVal;
		    }
		}
		else if (ulDataLen > 0)
		{
		    // Body extended to the end of file - stop here
		    HX_ASSERT(pBuffer);

		    m_ulNewAtomDataSize = ulDataLen;

		    m_pNewAtom->SetBuffer(pBuffer);
		    m_pNewAtom->SetSize(m_ulNewAtomDataSize);
		    m_ulNewAtomOffset = m_ulCurrentOffset;
		    m_pNewAtom->Release();
		    m_pNewAtom = NULL;
		}

		break;

	    default:
		status = HXR_FAIL;
		retVal = HXR_UNEXPECTED;
		break;
	    }
	} while (bLoop);
    }
    else
    {
	// Did not clear for processing
	if (m_State == ATMZR_Offline)
	{
	    return HXR_UNEXPECTED;
	}
	else
	{
	    // The read must have failed - complete atomization as is
	    status = HXR_OK;
	}
    }

    CompleteAtomization(status);

    return retVal;
}


/////////////////////////////////////////////////////////////////////////
//  Method:
//	IHXFileResponse::SeekDone
//  Purpose:
//	Notification interface provided by users of the IHXFileObject
//	interface. This method is called by the IHXFileObject when the
//	last seek in the file is complete.
//
STDMETHODIMP CAtomizer::SeekDone(HX_RESULT status)
{
    HX_RESULT retVal = HXR_OK;

    if (SUCCEEDED(status) && (m_State != ATMZR_Offline))
    {
	// We seek headers only, so start the read of a header
	switch(m_State)
	{
	case ATMZR_ProcHeader:
	    ReadDataCB(QT_HEADER_SIZE);
	    return retVal;
	case ATMZR_ProcNewHeader:
	    ReadDataCB(QT_NEW_HEADER_SIZE);
	    return retVal;
	default:
	    status = HXR_FAIL;
	    retVal = HXR_UNEXPECTED;
	    break;
	}
    }
    else
    {
	// Did not clear for processing
	if (m_State == ATMZR_Offline)
	{
	    return HXR_UNEXPECTED;
	}
	else
	{
	    // The seek must have failed - complete atomization as is
	    status = HXR_OK;
	}
    }

    CompleteAtomization(status);

    return retVal;
}


/****************************************************************************
 *  IUnknown methods
 */
/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::QueryInterface
//
STDMETHODIMP CAtomizer::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IHXFileResponse))
    {
	AddRef();
	*ppvObj = (IHXFileResponse*) this;
	return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXAtomizationCommander))
    {
	AddRef();
	*ppvObj = (IHXAtomizationCommander*) this;
	return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXThreadSafeMethods))
    {
	AddRef();
	*ppvObj = (IHXThreadSafeMethods*) this;
	return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IUnknown))
    {
	AddRef();
	*ppvObj = this;
	return HXR_OK;
    }
#if defined(HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS)
    else if (IsEqualIID(riid, IID_IHXMediaBytesToMediaDur))
    {
        if (m_pResponse)
        {
	    return m_pResponse->QueryInterface(IID_IHXMediaBytesToMediaDur,
                    ppvObj);
        }
    }
    else if (IsEqualIID(riid, IID_IHXPDStatusMgr))
    {
        if (m_pResponse)
        {
	    return m_pResponse->QueryInterface(IID_IHXPDStatusMgr,
                    ppvObj);
        }
    }
#endif /* #if defined(HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS). */

    *ppvObj = NULL;

    return HXR_NOINTERFACE;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::AddRef
//
STDMETHODIMP_(ULONG32) CAtomizer::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::Release
//
STDMETHODIMP_(ULONG32) CAtomizer::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
//  Method:
//	IHXThreadSafeMethods::IsThreadsafe
//
//  Purpose:
//	This routine returns threadsafeness information about the file object
//	which is used by the server for improved performance.
//
STDMETHODIMP_(UINT32)
CAtomizer::IsThreadSafe()
{
    return HX_THREADSAFE_METHOD_FF_GETPACKET | HX_THREADSAFE_METHOD_FSR_READDONE;
}


/****************************************************************************
 *  Class CAtomizer::RecursionCallback
 */
#ifdef QTCONFIG_RECURSION_PROTECTION
/****************************************************************************
 *  Constructor/Destructor
 */
CAtomizer::CRecursionCallback::CRecursionCallback(CAtomizer *pAtomizer)
    : m_pAtomizer(pAtomizer)
    , m_lRefCount(0)
{
    HX_ASSERT(m_pAtomizer);
    m_pAtomizer->AddRef();
}

CAtomizer::CRecursionCallback::~CRecursionCallback(void)
{
    HX_RELEASE(m_pAtomizer);
}

/****************************************************************************
 *  IHXCallback methods
 */
/****************************************************************************
 *  Func
 */
STDMETHODIMP CAtomizer::CRecursionCallback::Func(void)
{
    switch (m_pAtomizer->m_CallbackStep)
    {
    case ATMZR_CBSTEP_Read:
	m_pAtomizer->ReadData(m_pAtomizer->m_ulSize);
	return HXR_OK;
    case ATMZR_CBSTEP_Seek:
	m_pAtomizer->SeekData(m_pAtomizer->m_ulSize,
			      m_pAtomizer->m_bRelative);
	return HXR_OK;
    default:
	// nothing to do
	break;
    }

    return HXR_UNEXPECTED;
}

/****************************************************************************
 *  IUnknown methods
 */
/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::QueryInterface
//
STDMETHODIMP CAtomizer::CRecursionCallback::QueryInterface(
    REFIID riid,
    void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
	AddRef();
	*ppvObj = this;
	return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXCallback))
    {
        AddRef();
        *ppvObj = (IHXCallback*)this;
        return HXR_OK;
    }

    *ppvObj = NULL;

    return HXR_NOINTERFACE;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::AddRef
//
STDMETHODIMP_(ULONG32) CAtomizer::CRecursionCallback::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::Release
//
STDMETHODIMP_(ULONG32) CAtomizer::CRecursionCallback::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}
#endif	// QTCONFIG_RECURSION_PROTECTION

