/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: pcktflowwrap.h,v 1.6 2005/09/30 19:47:51 jc Exp $
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

#ifndef _PCKTFLOWWRAP_H_
#define _PCKTFLOWWRAP_H_

class PPM;
class PacketFlowManager;
class DataConvertShim;
class Player;
class Process;

_INTERFACE IHXPSourceControl;
_INTERFACE IHXPacketFlowControl;
_INTERFACE IHXSessionStats;

#include "hxtypes.h"

class PacketFlowWrapper
{
public:
    PacketFlowWrapper(Process* pProc, BOOL bIsRTP);
    virtual ~PacketFlowWrapper();

    HX_RESULT RegisterSource(IHXPSourceControl* pSourceCtrl,
                             REF(IHXPacketFlowControl*) pSessionControl,
                             IHXSessionStats* pSessionStats,
                             UINT16 unStreamCount,
                             BOOL bUseMDP,
                             BOOL bIsLive,
                             BOOL bIsMulticast,
                             DataConvertShim* pDataConv);
    
    HX_RESULT RegisterSource(IHXPSourceControl* pSourceCtrl,
                             REF(IHXPacketFlowControl*) pSessionControl,
                             IHXSessionStats* pSessionStats,
                             UINT16 unStreamCount,
                             BOOL bUseMDP,
                             BOOL bIsLive,
                             BOOL bIsMulticast,
                             DataConvertShim* pDataConv,
                             Player* pPlayerCtrl,
                             const char* szPlayerSessionId);

private:
    PPM* m_pPPM;
    PacketFlowManager* m_pFlowMgr;
    Process* m_pProc;
    BOOL m_bIsRTP;
};

#endif // _PCKTFLOWWRAP_H_
