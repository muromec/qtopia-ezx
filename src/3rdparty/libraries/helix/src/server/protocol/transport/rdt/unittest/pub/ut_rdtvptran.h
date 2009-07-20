/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: ut_rdtvptran.h,v 1.2 2006/12/18 18:42:13 tknox Exp $
 *
 * Portions Copyright (c) 1995-2006 RealNetworks, Inc. All Rights Reserved.
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

#ifndef UT_RDT_VP_TRANSPORT_H
#define UT_RDT_VP_TRANSPORT_H

#include "hxcom.h"
#include "hxcomptr.h"

#include "rtspif.h"
#include "rdt_udp.h"

class RDTUDPTransport;

typedef HXCOMPtr<RDTUDPTransport> SPRDTUDPTransport;

/** \class RDTUDPVectorPacketTransport
  * \brief RDTUDPVectorPacketTransport subclasses RDTUDPTransport to override VectorPacket().
  */
class RDTUDPVectorPacketTransport : public RDTUDPTransport
{
public:

    RDTUDPVectorPacketTransport ();
    RDTUDPVectorPacketTransport (BOOL bIsServer,
                                 BOOL bIsSource,
                                 RTSPTransportTypeEnum lType,
                                 UINT32 uFeatureLevel = 0,
                                 BOOL bDisableResend = FALSE,
                                 bool bCallParentVectorPacket = true);
    virtual ~RDTUDPVectorPacketTransport ();

    /** \brief Should we handle VectorPacket(), or pass it up to RDTUDPTransport?
      */
    void CallParentVectorPacket (bool bCallParentVectorPacket)
            { m_bCallParentVectorPacket = bCallParentVectorPacket; };

private:

    /** \brief My Private Idaho (or in this case, implementation of VectorPacket()).
      */
    HX_RESULT VectorPacket (BasePacket* pPacket, IHXBuffer** ppCur = NULL);

    bool m_bCallParentVectorPacket;
};

#endif /* UT_RDT_VP_TRANSPORT_H */
