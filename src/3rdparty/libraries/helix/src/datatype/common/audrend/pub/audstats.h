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

#ifndef __AUDSTATS_H__
#define __AUDSTATS_H__

/****************************************************************************
 *  Includes
 */
#include "hxtypes.h"
#include "hxresult.h"
#include "hxcom.h"
#include "hxcomm.h"
#include "hxbuffer.h"
#include "hxmon.h"
#include "statinfo.h"
#include "rendstats.h"

typedef enum
{
    AS_REND_NAME = 0,
    AS_CODEC_4CC,
    AS_CODEC_NAME,
    AS_CODEC_VERSION,
    AS_SURESTREAM,
    AS_CODECS,
    AS_CHANNELS,
    AS_SAMPLING_RATE,
    AS_SAMPLE_SIZE,
    AS_SURROUND,
    AS_NUM_ENTRIES
} AudioStatEntryID;

class CAudioStatistics
{
public:
    /*
     *  Costructor/Destructor
     */
    CAudioStatistics(IUnknown* pContext);
    
    ~CAudioStatistics();
    
    /*
     *  Main Interface
     */
    HX_RESULT DisplayStats(UINT32 ulRegistryID);

    HX_RESULT ReportStat(AudioStatEntryID eEntryID, char* pVal)
    {
	return m_pDisplay->UpdateEntry((UINT32) eEntryID, pVal);
    }
    
    HX_RESULT ReportStat(AudioStatEntryID eEntryID, INT32 lVal)
    {
	return m_pDisplay->UpdateEntry((UINT32) eEntryID, lVal);
    }

private:
    HX_RESULT PrimeEntries(void);

    IHXRegistry* m_pRegistry;
    ULONG32 m_ulRegistryID;
    CRendererStatisticsDisplay* m_pDisplay;
};

#endif /* __AUDSTATS_H__ */

