/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: httppars.h,v 1.5 2006/05/19 18:06:16 atin Exp $
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

/*
 * Class:
 *	HTTPParser
 * 
 * Purpose:
 *	Transform HTTP/1.0 data into HTTPMessage objects. 
 *
 */

#ifndef _HTTPPARS_H_
#define _HTTPPARS_H_

#include "mimescan.h"
#include "hxstring.h"
#include "hxslist.h"
#include "httpmsg.h"

#ifdef HELIX_FEATURE_SERVER
// the limits are being put to prevent a DOS attack with infinite HTTP
// requests (infinite headers within a HTTP message)
#define MAX_HTTP_HEADER_COUNT	64 
#define MAX_HTTP_HEADER_SIZE	4096
#define MAX_HTTP_MSG_SIZE	65536
#endif /* HELIX_FEATURE_SERVER */

class HTTPParser
{
public:
    HTTPParser(IUnknown* pContext = NULL);
    virtual ~HTTPParser();

#if !defined HELIX_FEATURE_SERVER
      virtual HTTPMessage* parse(const char* pMsg, UINT32& nMsgLen);
#else
    virtual HTTPMessage* parse(const char* pMsg, UINT32& nMsgLen,
	BOOL& bMessageTooLarge);
#endif /* !HELIX_FEATURE_SERVER */

protected:
    MIMEHeader* parseHeader(CHXString& str);
    int parseParameter(const char* parameter, MIMEHeader* pHeader);
    int parseHeaderValue(const char* pValue, MIMEHeader* pHeader);
    int defaultParseHeaderValues(const char* pValue, MIMEHeader* pHeader);
    int parseWWWAuthenticateHeaderValues(const char* pValue,
					MIMEHeader* pHeader);
    int parseProtocolVersion(const CHXString& prot, int& major, int& minor);

    int scanMessageHeader(const char* pMsg, UINT32 nMsgLen);

    virtual HTTPMessage* parseResponse();
    HTTPResponseMessage* parseResponseLine(CHXString& str);

    virtual HTTPMessage* parseRequest();
    HTTPRequestMessage* parseRequestLine(CHXString& str);

    void clearMessageLines();

    CHXSimpleList m_msglines;
    IUnknown* m_pContext;
#if defined HELIX_FEATURE_SERVER
    BOOL m_bMessageLimitExceeded;
#endif /* HELIX_FEATURE_SERVER */
};

#endif	/* _HTTPPARS_H_ */
