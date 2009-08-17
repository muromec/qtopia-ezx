/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: mms_sniffer.h,v 1.2 2004/08/24 21:37:30 tmarshall Exp $
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

///////////////////////////////////////////////////////////////////////////////
// HXMMSSniffer class declaration.
//
// Implementation is different on server/proxy.
// This file is here instead of server/engine/core because of
// HELIX_FEATURE_WMT_MMS conditional compilation.
//
// Server: server/engine/servercore/mms_sniffer_server.cpp
// Proxy: server_rn/proxy/core/mms_sniffer_proxy.cpp
///////////////////////////////////////////////////////////////////////////////


class HXMMSSniffer : public HXProtocolSniffer
{
public:
    HXMMSSniffer(void);
    virtual ~HXMMSSniffer(void);

    virtual BOOL         Match(IHXBuffer* pBuf) { return FALSE; }
    virtual HXProtocol*  Create(CHXServSocket* pSock);
};

