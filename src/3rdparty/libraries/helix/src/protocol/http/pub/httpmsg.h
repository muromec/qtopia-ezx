/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: httpmsg.h,v 1.7 2009/04/11 00:15:39 svaidhya Exp $
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

/*
 * Base class for all HTTP messages
 */

#ifndef _HTTPMSG_H_
#define _HTTPMSG_H_

#include "mimehead.h"

struct IHXValues;


class HTTPAuthentication: public MIMEHeaderValue
{
public:
    enum AuthenticationType { AU_BASIC, AU_DIGEST, AU_HX_PRIVATE } m_authType;

    HTTPAuthentication(const char* authString, AuthenticationType authType);
    HTTPAuthentication(IHXValues* authValues);
    ~HTTPAuthentication();
    void asString(CHXString& str);
    CHXString asString();

    CHXString   m_authString;
    IHXValues* m_authValues;
};

class HTTPMessage
{
public:
    HTTPMessage();
    virtual ~HTTPMessage();

    static const int MAJ_VERSION;
    static const int MIN_VERSION;

    enum Tag { T_UNKNOWN, T_RESP, T_GET, T_POST, T_HEAD , T_SWITCH};

    virtual Tag tag() const = 0;
    virtual const char* tagStr() const = 0;
    void setVersion(int maj, int min) { m_nMajorVersion = maj;
                                        m_nMinorVersion = min; }
    int majorVersion() { return m_nMajorVersion; }
    int minorVersion() { return m_nMinorVersion; }
    void setContent(const char* pStr) { m_content = pStr; }
    void setContent(BYTE* pBuf, UINT32 pBufLen);
    const char* getContent() { return m_content; }
    int contentLength();
    MIMEHeader* getHeader(const char* pName);
    MIMEHeader* getFirstHeader();
    MIMEHeader* getNextHeader();

    // special case retrieval for simple Header: value situations
    CHXString getHeaderValue(const char* pName);
    int getHeaderValue(const char* pName, UINT32& value);

    void addHeader(MIMEHeader* pHeader);
    void addHeader(const char* pName, const char* pValue);

    void setCloakedMsgFlag(HXBOOL flag) { m_isCloakedMsg = flag; }
    HXBOOL isCloakedMsg() { return m_isCloakedMsg; }

    virtual void asString(char* pBuf, int& msgLen, UINT32 ulBufLen) = 0;

private:
    void clearHeaderList();

    int m_nMajorVersion;
    int m_nMinorVersion;
    CHXString m_content;
    CHXSimpleList m_headers;
    LISTPOSITION m_listpos;
    HXBOOL m_isCloakedMsg;
};

/*
 * HTTP request is in form:
 *
 *      method url version <CRLF>
 *      *(header<CRLF>)
 *      <CRLF>
 */
class HTTPRequestMessage: public HTTPMessage
{
public:
    virtual HTTPMessage::Tag tag() const = 0;
    virtual const char* tagStr() const = 0;
    void setURL(const char* pURL) { m_url = pURL; }
    const char* url() const { return m_url; }

    virtual void asString(char* pBuf, int& msgLen, UINT32 ulBufLen);

private:
    CHXString m_url;
};

/*
 * HTTP response is in form:
 *
 *      version error-code error-text <CRLF>
 *      *(header<CRLF>)
 *      <CRLF>
 *
 */
class HTTPResponseMessage: public HTTPMessage
{
public:
    HTTPMessage::Tag tag() const { return HTTPMessage::T_RESP; }
    const char* tagStr() const { return ""; }

    virtual void setErrorCode(const char* eCode) { m_errorCode = eCode; }
    const char* errorCode() { return m_errorCode; }
    void setErrorMsg(const char* eMsg) { m_errorMsg = eMsg; }
    const char* errorMsg() { return m_errorMsg; }

    virtual void asString(char* pBuf, int& msgLen, UINT32 ulBufLen);
protected:
    CHXString m_errorCode;
    CHXString m_errorMsg;
};

class HTTPUnknownMessage: public HTTPRequestMessage
{
public:
    HTTPMessage::Tag tag() const { return HTTPMessage::T_UNKNOWN; }
    const char* tagStr() const { return "UNKNOWN"; }
};

class HTTPGetMessage: public HTTPRequestMessage
{
public:
    HTTPMessage::Tag tag() const { return HTTPMessage::T_GET; }
    const char* tagStr() const { return "GET"; }
};

class HTTPPostMessage: public HTTPRequestMessage
{
public:
    HTTPMessage::Tag tag() const { return HTTPMessage::T_POST; }
    const char* tagStr() const { return "POST"; }
};

class HTTPHeadMessage: public HTTPRequestMessage
{
public:
    HTTPMessage::Tag tag() const { return HTTPMessage::T_HEAD; }
    const char* tagStr() const { return "HEAD"; }
};

#endif /* _HTTPMSG_H_ */
