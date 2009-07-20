/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: pkthndlr.h,v 1.4 2007/07/06 20:51:42 jfinnecy Exp $
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

#ifndef _PKTHNDLR_H_
#define _PKTHNDLR_H_


#include "hxslist.h"

class RTCPPacket;


/*
*   This is a class to pack up a couple of RTCP packet to make a compund RTCP
*   packet that can be sent off the wire.
*/
class RTCPPacker
{
public:
    RTCPPacker();
    ~RTCPPacker() 
    { /* it doesn't do anything */ };

    /*
    *	pPkt must NOT be packed.
    */
    HX_RESULT	    Set(RTCPPacket* pPkt);

    /*
    * pass in an instance of a buffer...
    */
    HX_RESULT	    Pack(REF(IHXBuffer*) pBuf);

private:
    // mac didn't like too many inline functions...
    void	    PackOne(RTCPPacket* pPkt, REF(UCHAR*) pc, REF(UINT32) ulPackedLen);

    /*
    * in out system, it will be EITHER SR or RR
    */
    RTCPPacket*	    m_pReport;
    RTCPPacket*	    m_pSDES;
    RTCPPacket*	    m_pBYE;        
    CHXSimpleList   m_APPList;
};


/*
*   Given a compund packet, it will unpack each msg.
*/
class RTCPUnPacker
{
public:
    RTCPUnPacker();
    ~RTCPUnPacker();

    /*
    * return HXR_OK as long as there is a pkt
    */
    HX_RESULT	Get(REF(RTCPPacket*) pPkt);

    HX_RESULT	UnPack(IHXBuffer* pCompound);    

private:
    HX_RESULT	Validate(IHXBuffer* pCompound);

    CHXSimpleList   m_PktList;
};

#endif // _PKTHNDLR_H_
