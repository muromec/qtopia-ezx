/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: fileindextable.cpp,v 1.7 2005/04/27 13:57:38 ehyche Exp $
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

/****************************************************************************
 *  Defines
 */

/****************************************************************************
 *  Includes
 */
#include "hxresult.h"

#include "fileindextable.h"


/****************************************************************************
 *  Class CFileIndexTable
 */
/****************************************************************************
 *  Constructor/Destructor
 */
CFileIndexTable::CFileIndexTable(void)
    : m_pStreamIndexTable(NULL)
    , m_uNumStreams(0)
{
    ;
}

CFileIndexTable::~CFileIndexTable()
{
    _Close();
}

/****************************************************************************
 *  Main Interface
 */
/****************************************************************************
 *  Init
 */
HX_RESULT CFileIndexTable::Init(UINT16 uNumStreams,
				UINT32 ulTimeGranularity,
				UINT32 ulMaxEntries,
				HXBOOL bUseAuxData)
{
    UINT32 ulIdx;
    HX_RESULT retVal = HXR_OK;

    _Close();

    m_uNumStreams = uNumStreams;

    if (m_uNumStreams != 0)
    {
	m_pStreamIndexTable = new CStreamIndexTable [m_uNumStreams];
	retVal = HXR_OUTOFMEMORY;
	if (m_pStreamIndexTable)
	{
	    retVal = HXR_OK;
	}
    }

    for (ulIdx = 0; SUCCEEDED(retVal) && (ulIdx < uNumStreams); ulIdx++)
    {
	retVal = m_pStreamIndexTable[ulIdx].Init(ulTimeGranularity,
						 ulMaxEntries,
						 bUseAuxData);
    }
	
    return retVal;
}


/****************************************************************************
 *  SetIndex
 */
HX_RESULT CFileIndexTable::SetIndex(UINT16 uStreamNumber,
				    UINT32 ulTime,
				    UINT32 ulOffset,
				    void* pAuxData)
{
    if (uStreamNumber >= m_uNumStreams)
    {
	return HXR_FAIL;
    }

    return m_pStreamIndexTable[uStreamNumber].SetIndex(ulTime,
						       ulOffset,
						       pAuxData);
}


/****************************************************************************
 *  GetIndex
 */
HX_RESULT CFileIndexTable::GetIndex(UINT16 uStreamNumber,
				    UINT32& ulTime,
				    UINT32& ulOffset,
				    void** ppAuxData)
{
    if (uStreamNumber >= m_uNumStreams)
    {
	return HXR_FAIL;
    }

    return m_pStreamIndexTable[uStreamNumber].GetIndex(ulTime,
						       ulOffset,
						       ppAuxData);
}

/****************************************************************************
 *  HasIndex
 */
HXBOOL CFileIndexTable::HasIndex(UINT16 uStreamNumber,
			       UINT32 ulTime)
{
    if (uStreamNumber >= m_uNumStreams)
    {
	return FALSE;
    }

    return m_pStreamIndexTable[uStreamNumber].HasIndex(ulTime);
}

/****************************************************************************
 *  ConvertOffsetToTime
 */
HX_RESULT
CFileIndexTable::ConvertOffsetToTime(UINT16  /* IN  */ uStreamNumber,
                                     UINT32  /* IN  */ ulTargetOffset,
                                     UINT32  /* IN  */ ulTotalFileSize,
                                     UINT32& /* OUT */ ulREFTime,
                                     UINT32& /* OUT */ ulREFOffset,
                                     HXBOOL&   /* OUT */ bREFCurTableInsufficient)
{
    if (uStreamNumber >= m_uNumStreams)
    {
	return HXR_FAIL;
    }

    return m_pStreamIndexTable[uStreamNumber].ConvertOffsetToTime(
            ulTargetOffset, ulTotalFileSize, ulREFTime,
            ulREFOffset, bREFCurTableInsufficient);
}


/****************************************************************************
 *  UpdateGranularity
 */
void CFileIndexTable::UpdateGranularity(UINT32 ulTimeGranularity)
{
    UINT16 ulIdx;

    for (ulIdx = 0; ulIdx < m_uNumStreams; ulIdx++)
    {
	m_pStreamIndexTable[ulIdx].UpdateGranularity(ulTimeGranularity);
    }
}


/****************************************************************************
 *  UpdateTimeRange
 */
void CFileIndexTable::UpdateTimeRange(UINT16 uStreamNumber, UINT32 ulTime)
{
    if (uStreamNumber < m_uNumStreams)
    {
	m_pStreamIndexTable[uStreamNumber].UpdateTimeRange(ulTime);
    }
}


/****************************************************************************
 *  Close
 */
void CFileIndexTable::Close(void)
{
    _Close();
}


/****************************************************************************
 *  Private Methods
 */
/****************************************************************************
 *  _Close
 */
void CFileIndexTable::_Close(void)
{
    m_uNumStreams = 0;
        
    HX_VECTOR_DELETE(m_pStreamIndexTable);
}


/****************************************************************************
 *  Class CStreamIndexTable
 */
/****************************************************************************
 *  Constructor/Destructor
 */
CStreamIndexTable::CStreamIndexTable(void)
    : m_pIndexTable(NULL)
    , m_pAuxDataTable(NULL)
    , m_ulMaxEntries(0)
    , m_ulTimeGranulairty(0)
    , m_ulLastTime(0)
    , m_ulRangeTime(0)
{
    ;
}

CStreamIndexTable::~CStreamIndexTable()
{
    _Close();
}


/****************************************************************************
 *  Main Interface
 */
/****************************************************************************
 *  Init
 */
HX_RESULT CStreamIndexTable::Init(UINT32 ulTimeGranularity,
				  UINT32 ulMaxEntries,
				  HXBOOL bUseAuxData)
{
    HX_RESULT retVal = HXR_OK;

    _Close();

    m_ulMaxEntries = ulMaxEntries;
    m_ulTimeGranulairty = ulTimeGranularity;

    if (m_ulMaxEntries != 0)
    {	
	m_pIndexTable = new IndexEntry [m_ulMaxEntries];
	retVal = HXR_OUTOFMEMORY;
	if (m_pIndexTable)
	{
	    retVal = HXR_OK;
	}
	
	if (SUCCEEDED(retVal) && bUseAuxData)
	{
	    m_pAuxDataTable = new void* [m_ulMaxEntries];
	    retVal = HXR_OUTOFMEMORY;
	    if (m_pAuxDataTable)
	    {
		retVal = HXR_OK;
	    }
	}
    }

    return retVal;
}


/****************************************************************************
 *  SetIndex
 */
HX_RESULT CStreamIndexTable::SetIndex(UINT32 ulTime,
				      UINT32 ulOffset,
				      void* pAuxData)
{
    HX_RESULT retVal = HXR_FAIL;

    if (m_ulNumEntries >= m_ulMaxEntries)
    {
	return HXR_FAIL;
    }

    if ((m_ulNumEntries == 0) ||
	((ulTime > m_ulLastTime) &&
	 ((ulTime - m_ulLastTime) >= m_ulTimeGranulairty)))
    {
	m_ulLastTime = ulTime;
	if (m_ulLastTime > m_ulRangeTime)
	{
	    m_ulRangeTime = m_ulLastTime;
	}
	m_pIndexTable[m_ulNumEntries].m_ulTime = ulTime;
	m_pIndexTable[m_ulNumEntries].m_ulOffset = ulOffset;
	if (m_pAuxDataTable)
	{
	    m_pAuxDataTable[m_ulNumEntries] = pAuxData;
	}
	m_ulNumEntries++;

	retVal = HXR_OK;
    }

    return retVal;
}

/****************************************************************************
 *  GetIndex
 */
HX_RESULT CStreamIndexTable::GetIndex(UINT32& ulTime,
				      UINT32& ulOffset,
				      void** ppAuxData)
{
    HX_RESULT retVal = HXR_OK;
    ULONG32 ulTargetTime = ulTime;
    ULONG32 ulTableIdx;
    
    // Estimate Table Index to search from
    ulTableIdx = ulTargetTime / m_ulTimeGranulairty;

    if (m_ulNumEntries == 0)
    {
	retVal = HXR_FAIL;
    }
    else
    {
        if (ulTableIdx >= m_ulNumEntries)
        {
	    ulTableIdx = m_ulNumEntries - 1;
        }

        // Scan Forward
        while (((ulTableIdx + 1) < (m_ulNumEntries - 1)) &&
	       (m_pIndexTable[ulTableIdx + 1].m_ulTime < ulTargetTime))
        {
	    ulTableIdx++;
        }
        // Scan Backwards
        while ((ulTableIdx > 0) &&
	       (m_pIndexTable[ulTableIdx].m_ulTime > ulTargetTime))
        {
	    ulTableIdx--;
        }

        // Report results
        ulTime = m_pIndexTable[ulTableIdx].m_ulTime;
        ulOffset = m_pIndexTable[ulTableIdx].m_ulOffset;
        if (m_pAuxDataTable)
        {
	    if (ppAuxData)
	    {
	        *ppAuxData = m_pAuxDataTable[ulTableIdx];
	    }
        }
    
        retVal = (m_ulRangeTime < ulTargetTime) ? HXR_AT_END : HXR_OK;
    }

    return retVal;
}


/****************************************************************************
 *  HasIndex
 */
HXBOOL CStreamIndexTable::HasIndex(UINT32 ulTime)
{
    return (ulTime <= m_ulLastTime);
}


/****************************************************************************
 *  ConvertOffsetToTime
 */
HX_RESULT 
CStreamIndexTable::ConvertOffsetToTime(UINT32  /* IN  */ ulTargetOffset,
                                       UINT32  /* IN  */ ulTotalFileSize,
                                       UINT32& /* OUT */ ulREFTime,
                                       UINT32& /* OUT */ ulREFOffset,
                                       HXBOOL&   /* OUT */ bREFCurTableInsufficient)
{ 
    HX_RESULT retVal = HXR_OK;
    
    bREFCurTableInsufficient = FALSE;

    if (!m_ulNumEntries)
    { 
        bREFCurTableInsufficient = TRUE;
        ulREFOffset = 0;
        ulREFTime = 0; // /We have nothing for this stream yet, thus dur==0.
        retVal = HXR_FAIL; 
    } 
    else if (ulTargetOffset > ulTotalFileSize)
    {
        retVal = HXR_INVALID_PARAMETER;
    } 
    else 
    { 
        // /Worst case is we start from end and work back if necessary.  Most
        // of the time, in progressive download situations, we want the last
        // one anyway:
        ULONG32 ulTableIdx = m_ulNumEntries - 1;
    
        // /If we can, make an estimate of the table index assuming roughly
        // constant bitrate:
        UINT32 ulFirstIndxOffset = m_pIndexTable[0].m_ulOffset;
        UINT32 ulLastIndxOffset  = m_pIndexTable[m_ulNumEntries - 1].m_ulOffset;
        if (ulTargetOffset <= ulLastIndxOffset)
        {
            // /The last index is the *start* of the last block of data so that
            // block is not included in the offset, thus we divide by 1 less
            // than #entries, e.g., we have 5 entries ([0]-[4]) and we are 40%
            // through the file, then we want [1] since the byte ranges covered
            // by each index are: {[0]:0%-24.999%, [1]:25%-49.999%,
            // [2]:50%-74.999%, [3]:75%-99.999%, and [4]:100% or more} :
            ulTableIdx = (UINT32)(
                    (((double)(ulTargetOffset - ulFirstIndxOffset)) /
                    ((double)(ulLastIndxOffset - ulFirstIndxOffset)))  *
                    ((double)m_ulNumEntries-1) );
        } 

        // /This is impossible if the logic above is correct, but...:
        if (ulTableIdx >= m_ulNumEntries)
        { 
	    ulTableIdx = m_ulNumEntries - 1; 
        } 

        // /Scan Forward:
        while (((ulTableIdx + 1) < (m_ulNumEntries - 1))  && 
	       (m_pIndexTable[ulTableIdx + 1].m_ulOffset < ulTargetOffset)) 
        { 
	    ulTableIdx++;
        } 
        // /Scan Backward:
        while ((ulTableIdx > 0) &&  
	       (m_pIndexTable[ulTableIdx].m_ulOffset > ulTargetOffset)) 
        { 
	    ulTableIdx--; 
        } 
 
        if (ulTableIdx == m_ulNumEntries - 1)
        {
            ulREFOffset = ulLastIndxOffset;
            if (m_pIndexTable[ulTableIdx].m_ulOffset < ulTargetOffset)
            {
                // /Our target is past the end of what's in the table so far,
                // so hoist the flag:
                bREFCurTableInsufficient = TRUE;
            }
        }
        else
        {
            ulREFOffset = m_pIndexTable[ulTableIdx].m_ulOffset;
        }

        ulREFTime = m_pIndexTable[ulTableIdx].m_ulTime;
    }

    return retVal;
}


/****************************************************************************
 *  UpdateGranularity
 */
void CStreamIndexTable::UpdateGranularity(UINT32 ulTimeGranularity)
{
    m_ulTimeGranulairty = ulTimeGranularity;
}


/****************************************************************************
 *  UpdateTimeRange
 */
void CStreamIndexTable::UpdateTimeRange(UINT32 ulTime)
{
    if (m_ulRangeTime < ulTime)
    {
	m_ulRangeTime = ulTime;
    }
}


/****************************************************************************
 *  Close
 */
void CStreamIndexTable::Close(void)
{
    _Close();
}


/****************************************************************************
 *  Private Methods
 */
/****************************************************************************
 *  _Close
 */
void CStreamIndexTable::_Close(void)
{
    m_ulNumEntries = 0;
    m_ulMaxEntries = 0;
    m_ulLastTime = 0;
    m_ulRangeTime = 0;
        
    HX_VECTOR_DELETE(m_pIndexTable);
    HX_VECTOR_DELETE(m_pAuxDataTable);
}
