/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: rtsp_listenresp.h,v 1.11 2005/01/25 22:55:21 jzeng Exp $
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

#ifndef _RTSP_LISTENRESP_H_
#define _RTSP_LISTENRESP_H_

#include "inetwork.h"

struct IHXBuffer;

class RTSPListenResponse : public ListenResponse
{
public:
    RTSPListenResponse(Process* _proc);
    HX_RESULT Init(UINT16 uPort);

    // IHXListeningSocketResponse
    STDMETHOD(OnConnection)         (THIS_ IHXSocket* ppNewSock,
                                           IHXSockAddr* ppSource);
    STDMETHOD(OnError)              (THIS_ HX_RESULT err);

private:
    virtual ~RTSPListenResponse(void);
};

#endif /*_RTSP_LISTENRESP_H_*/
