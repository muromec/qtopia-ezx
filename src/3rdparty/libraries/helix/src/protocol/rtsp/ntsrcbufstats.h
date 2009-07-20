/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: ntsrcbufstats.h,v 1.3 2007/01/11 21:12:21 milko Exp $ 
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

#ifndef NTSRCBUFSTATS_H
#define NTSRCBUFSTATS_H

#include "hxsrcbufstats.h"

class RTSPClientProtocol;

class HXNetSourceBufStats : public HXSourceBufferStats
{
public:
    HXNetSourceBufStats(RTSPClientProtocol* pProto);

    // We need to override a few methods from the
    // base class
    
    virtual void Close();

    /*
     * IUnknown methods
     */
    STDMETHOD_(ULONG32,Release)	(THIS);

    /*
     * IHXSourceBufferingStats3 methods
     */
    STDMETHOD(GetCurrentBuffering)  (THIS_ UINT16 uStreamNumber,
				     REF(UINT32) ulLowestTimestamp, 
				     REF(UINT32) ulHighestTimestamp,
				     REF(UINT32) ulNumBytes,
				     REF(HXBOOL) bDone);
protected:
    ~HXNetSourceBufStats();

    void DoClose();

private:
    RTSPClientProtocol* m_pProto;
};

#endif /* NTSRCBUFSTATS_H */
