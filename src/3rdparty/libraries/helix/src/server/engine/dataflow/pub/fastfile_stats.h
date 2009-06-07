/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: fastfile_stats.h,v 1.4 2003/08/08 15:03:04 dcollins Exp $ 
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

#ifndef _FASTFILE_STATS_H_
#define _FASTFILE_STATS_H_

//////////////////////////////////////////////////////////////////////
// FastFileStats
//////////////////////////////////////////////////////////////////////

// registry location where a pointer to the stats are stored
#define FAST_FILE_STATS "FastFile.Stats"

class FastFileStats
{

public:
    static HX_RESULT SetupStats (IUnknown*     /*IN*/   pContext,
                                 REF(UINT32*)  /*OUT*/  pFobCount,
                                 REF(UINT32*)  /*OUT*/  pFastBytesRead,
                                 REF(UINT32*)  /*OUT*/  pSlowBytesRead,
                                 REF(UINT32*)  /*OUT*/  pInternalBytesRead,
                                 REF(UINT32*)  /*OUT*/  pBlockCount,
                                 REF(UINT32*)  /*OUT*/  pInUseBlockCount,
                                 REF(UINT32*)  /*OUT*/  pMemUse);

    static char* GetFormattedStats (IUnknown*   /*IN*/   pContext);

    static HX_RESULT GetStats   (IUnknown* pContext,
                                 double* pFFEffect,
                                 UINT32* pBlockCount,
                                 UINT32* pInUseBlockCount,
                                 UINT32* pFobCount,
                                 UINT32* pFastBytesRead,
                                 UINT32* pSlowBytesRead,
                                 UINT32* pInternalBytesRead,
                                 UINT32* pMemUse);

    UINT32 m_ulFobCount;
    UINT32 m_ulFastBytesRead;
    UINT32 m_ulSlowBytesRead;
    UINT32 m_ulInternalBytesRead;
    UINT32 m_ulBlockCount;
    UINT32 m_ulInUseBlockCount;
    UINT32 m_ulMemUse;
};

#endif //_FASTFILE_STATS_H_
