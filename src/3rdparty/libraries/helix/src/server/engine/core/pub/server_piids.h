/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: server_piids.h,v 1.8 2005/07/07 22:03:42 jzeng Exp $
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

#ifndef _SERVER_PIIDS_H_
#define _SERVER_PIIDS_H_

DEFINE_GUID(IID_IHXPSourceControl, 0x00000200, 0xb4c8, 0x11d0, 0x99, 0x95,
                    0x0, 0xa0, 0x24, 0x8d, 0xa5, 0xf0);
DEFINE_GUID(IID_IHXPSourcePackets, 0x00000201, 0xb4c8, 0x11d0, 0x99,
                    0x95, 0x0, 0xa0, 0x24, 0x8d, 0xa5, 0xf0);
DEFINE_GUID(IID_IHXPSourceLivePackets, 0x2bfb61ad, 0xbda0, 0x4cd1, 0xa8, 0x59,
                    0x57, 0x6c, 0x95, 0xc6, 0xba, 0x8);
DEFINE_GUID(IID_IHXPSourceLiveResync, 0x00000202, 0xb4c8, 0x11d0, 0x99,
                    0x95, 0x0, 0xa0, 0x24, 0x8d, 0xa5, 0xf0);
DEFINE_GUID(IID_IHXServerPacketSource, 0x00000203, 0xb4c8, 0x11d0, 0x99,
                    0x95, 0x0, 0xa0, 0x24, 0x8d, 0xa5, 0xf0);
DEFINE_GUID(IID_IHXPSinkControl, 0x00000300, 0xb4c8, 0x11d0, 0x99, 0x95,
                    0x0, 0xa0, 0x24, 0x8d, 0xa5, 0xf0);
DEFINE_GUID(IID_IHXPSinkPackets, 0x00000301, 0xb4c8, 0x11d0, 0x99, 0x95,
                    0x0, 0xa0, 0x24, 0x8d, 0xa5, 0xf0);
DEFINE_GUID(IID_IHXPSinkInfo, 0x00000302, 0xb4c8, 0x11d0, 0x99, 0x95,
                    0x0, 0xa0, 0x24, 0x8d, 0xa5, 0xf0);
DEFINE_GUID(IID_IHXServerPacketSink, 0x00000303, 0xb4c8, 0x11d0, 0x99, 0x95,
                    0x0, 0xa0, 0x24, 0x8d, 0xa5, 0xf0);
DEFINE_GUID(IID_IHXServerRDTTransport, 0x00000304, 0xb4c8, 0x11d0, 0x99, 0x95,
                    0x0, 0xa0, 0x24, 0x8d, 0xa5, 0xf0);
DEFINE_GUID(IID_IHXServerPauseAdvise, 0x00000305, 0xb4c8, 0x11d0, 0x99, 0x95,
                    0x0, 0xa0, 0x24, 0x8d, 0xa5, 0xf0);
DEFINE_GUID(IID_IHXServerShutdownResponse, 0xc1fc707d, 0xd62d, 0x4d47,
                    0x83, 0x95, 0xe4, 0x280, 0xf2, 0x14, 0xdb, 0x10);
DEFINE_GUID(IID_IHXLivePacketBufferQueue, 0x4a17f495, 0x0e71, 0x437e, 0xaf, 0x2c,
                    0xb5, 0xd8, 0xe5, 0xc0, 0x7a, 0x88);
DEFINE_GUID(IID_IHXLivePacketBufferProvider, 0x493f4272, 0xd80f, 0x411b, 0xb7, 0x6a,
                    0xc7, 0xd9, 0xbb, 0x4b, 0xe8, 0xee);
DEFINE_GUID(IID_IHXTokenBufferFilter, 0x6682dae4, 0x92ed, 0x4f3c, 0x80, 0xc4,
        0x7e, 0x8e, 0xd4, 0xc1, 0x78, 0xeb);

DEFINE_GUID(IID_IHXHTTPDemuxResponse, 0xfab8b184, 0xfc7b, 0x11d8, 0x88, 0x29,
                    0x00, 0x02, 0xb3, 0x65, 0x87, 0x20);
DEFINE_GUID(IID_IHXHTTPDemux, 0x43481048, 0xfc7c, 0x11d8, 0xa5, 0xf7,
                    0x00, 0x02, 0xb3, 0x65, 0x87, 0x20);

#endif
