/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: fileindextable.h,v 1.5 2005/03/14 19:24:44 bobclark Exp $
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

#ifndef _FILEINDEXTABLE_H_
#define _FILEINDEXTABLE_H_

/****************************************************************************
 *  Defines
 */
#define FILEIDXTABLE_TIME_GRANULE   15000   // in milliseconds
#define FILEIDXTABLE_MAX_ENTRIES    2048

/****************************************************************************
 *  Includes
 */
#include "hxresult.h"


/****************************************************************************
 * 
 *  Class:
 *	CStreamIndexTable
 *
 *  Purpose:
 *	Manages mapping of stream index times and file offsets
 *
 */
class CStreamIndexTable
{
public:
    /*
     *	Constructor/Destructor
     */
    CStreamIndexTable(void);

    virtual ~CStreamIndexTable();

    /*
     *	Main Interface
     */
    virtual HX_RESULT Init(UINT32 ulTimeGranularity = FILEIDXTABLE_TIME_GRANULE,
			   UINT32 ulMaxEntries = FILEIDXTABLE_MAX_ENTRIES,
			   HXBOOL bUseAuxData = FALSE);

    virtual HX_RESULT SetIndex(UINT32 ulTime,
			       UINT32 ulOffset,
			       void* pAuxData = NULL);

    virtual HX_RESULT GetIndex(UINT32& ulTime,		// in & out
			       UINT32& ulOffset,	// out
			       void** ppAuxData = NULL);// out

    virtual HXBOOL HasIndex(UINT32 ulTime);

    virtual HX_RESULT ConvertOffsetToTime(UINT32  /* IN  */ ulTargetOffset,
                                          UINT32  /* IN  */ ulTotalFileSize,
                                          UINT32& /* OUT */ ulREFTime,
                                          UINT32& /* OUT */ ulREFOffset,
                                          HXBOOL&   /* OUT */ bREFCurTableInsufficient);

    virtual void UpdateGranularity(UINT32 ulTimeGranularity);

    virtual void UpdateTimeRange(UINT32 ulTime);

    virtual void Close(void);


private:
    typedef struct
    {
	UINT32 m_ulTime;
	UINT32 m_ulOffset;
    } IndexEntry;

    void _Close(void);

    IndexEntry* m_pIndexTable;
    void** m_pAuxDataTable;

    UINT32 m_ulMaxEntries;
    UINT32 m_ulTimeGranulairty;
    UINT32 m_ulNumEntries;
    UINT32 m_ulLastTime;
    UINT32 m_ulRangeTime;
};


/****************************************************************************
 * 
 *  Class:
 *	CFileIndexTable
 *
 *  Purpose:
 *	Manages mapping of stream index times and file offsets
 *
 */
class CFileIndexTable
{
public:
    /*
     *	Constructor/Destructor
     */
    CFileIndexTable(void);

    virtual ~CFileIndexTable();

    /*
     *	Main Interface
     */
    virtual HX_RESULT Init(UINT16 uNumStreams,
			   UINT32 ulTimeGranularity = FILEIDXTABLE_TIME_GRANULE,
			   UINT32 ulMaxEntries = FILEIDXTABLE_MAX_ENTRIES,
			   HXBOOL bUseAuxData = FALSE);

    virtual HX_RESULT SetIndex(UINT16 uStreamNumber,
			       UINT32 ulTime,
			       UINT32 ulOffset,
			       void* pAuxData = NULL);

    virtual HX_RESULT GetIndex(UINT16 uStreamNumber,	// in
			       UINT32& ulTime,		// in & out
			       UINT32& ulOffset,	// out
			       void** ppAuxData = NULL);// out

    virtual HXBOOL HasIndex(UINT16 uStreamNumber,
			  UINT32 ulTime);

    virtual HX_RESULT ConvertOffsetToTime(UINT16  /* IN  */ uStreamNumber,
                                          UINT32  /* IN  */ ulTargetOffset,
                                          UINT32  /* IN  */ ulTotalFileSize,
                                          UINT32& /* OUT */ ulREFTime,
                                          UINT32& /* OUT */ ulREFOffset,
                                          HXBOOL&   /* OUT */ bREFCurTableInsufficient);

    virtual void UpdateGranularity(UINT32 ulTimeGranularity);

    virtual void UpdateTimeRange(UINT16 uStreamNumber,
				 UINT32 ulTime);

    virtual void Close(void);


private:
    void _Close(void);

    CStreamIndexTable* m_pStreamIndexTable;

    UINT16 m_uNumStreams;
};


#endif  // _FILEINDEXTABLE_H_
