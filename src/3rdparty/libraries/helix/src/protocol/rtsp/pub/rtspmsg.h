/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: rtspmsg.h,v 1.6 2006/01/31 23:39:07 ping Exp $
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
 * RTSP/1.0 message and helper classes 
 *
 */

#ifndef _RTSPMSG_H_
#define _RTSPMSG_H_

#include "hxrtsp2.h"
#include "mimehead.h"

class CHXString;
class CHXSimpleList;
struct IHXValues;

const UINT32 RTSP_PLAY_RANGE_BLANK = (UINT32)-1;

/*
 * Contains a begin/end pair (expressed in millisecs)
 */
class RTSPRange: public MIMEHeaderValue
{
public:
    enum RangeType { TR_SMPTE, TR_CLOCK, TR_NPT } m_rangeType;

    RTSPRange(UINT32 begin, UINT32 end, RangeType rType);

    UINT32 m_begin;
    UINT32 m_end;

    void asString(CHXString& str);
    CHXString asString();
};

class RTSPAuthentication: public MIMEHeaderValue
{
public:
    enum AuthenticationType { AU_BASIC, AU_DIGEST, AU_HX_PRIVATE } m_authType;

    RTSPAuthentication(const char* authString, AuthenticationType authType);
    RTSPAuthentication(IHXValues* authValues);
    ~RTSPAuthentication();
    void asString(CHXString& str);
    CHXString asString();

    CHXString   m_authString;
    IHXValues* m_authValues;
};

class RTSPDigestAuthorization: public MIMEHeaderValue
{
public:
    RTSPDigestAuthorization(IHXValues* pValues);
    void asString(CHXString& str);
    CHXString asString();

    IHXValues* m_values;
};

class RTSPPEPInfo: public MIMEHeaderValue
{
public:
    RTSPPEPInfo(HXBOOL bStrengthMust);
    ~RTSPPEPInfo();
    void asString(CHXString& str);
    CHXString asString();

    HXBOOL m_bStrengthMust;
};

/*
 * Base class for all RTSP messages
 */
class RTSPMessage
{
public:
    RTSPMessage();
    virtual ~RTSPMessage();

    static const int MAJ_VERSION;
    static const int MIN_VERSION;

    enum Tag { T_UNKNOWN, T_RESP, T_SETUP, T_REDIRECT, 
	       T_PLAY, T_PAUSE, T_SET_PARAM, T_GET_PARAM,
	       T_OPTIONS, T_DESCRIBE, T_TEARDOWN, T_RECORD,
	       T_ANNOUNCE };

    virtual RTSPMessage::Tag tag() const = 0;
    virtual RTSPMethod GetMethod() const = 0;
    virtual const char* tagStr() const = 0;
    void setVersion(int maj, int min) { m_nMajorVersion = maj;
                                        m_nMinorVersion = min; }
    int majorVersion() { return m_nMajorVersion; }
    int minorVersion() { return m_nMinorVersion; }
    void setContent(const char* pStr) { m_content = pStr; }
    const char* getContent() { return m_content; }
    int contentLength();
    MIMEHeader* getHeader(const char* pName);
    MIMEHeader* getFirstHeader();
    MIMEHeader* getNextHeader();

    // special case retrieval for simple Header: value situations
    CHXString getHeaderValue(const char* pName);
    int getHeaderValue(const char* pName, UINT32& value);

    void addHeader(MIMEHeader* pHeader, HXBOOL bAtHead = FALSE);
    void addHeader(const char* pName, const char* pValue, HXBOOL bAtHead = FALSE);

    void setSeqNo(UINT32 seqNo) { m_seqNo = seqNo; }
    UINT32 seqNo() const { return m_seqNo; }

    virtual void asString(char* pBuf, int& msgLen) = 0;
    virtual CHXString asString() = 0;

    HX_RESULT AsValues(REF(IHXValues*) pIHXValuesHeaders, IUnknown* pContext);

private:
    void clearHeaderList();

    int m_nMajorVersion;
    int m_nMinorVersion;
    UINT32 m_seqNo;
    CHXString m_content;
    CHXSimpleList m_headers;
    LISTPOSITION m_listpos;
};

/*
 * RTSP request is in form:
 *
 * 	method url version sequence-number<CRLF>
 *      *(header<CRLF>)
 *      <CRLF>
 */
class RTSPRequestMessage: public RTSPMessage
{
public:
    virtual RTSPMessage::Tag tag() const = 0;
    virtual RTSPMethod GetMethod() const = 0;
    virtual const char* tagStr() const = 0;
    void setURL(const char* pURL) { m_url = pURL; }
    const char* url() const { return m_url; }

    void asString(char* pBuf, int& msgLen);
    CHXString asString();
private:
    CHXString m_url;
};

/*
 * RTSP response is in form:
 *
 *	version error-code sequence-number error-text<CRLF>
 *      *(header<CRLF>)
 *	<CRLF>
 *
 */
class RTSPResponseMessage: public RTSPMessage
{
public:
    virtual RTSPMessage::Tag tag() const { return RTSPMessage::T_RESP; }
    virtual RTSPMethod GetMethod() const { return RTSP_RESP; }
    const char* tagStr() const { return ""; }

    void setErrorCode(const char* eCode) 
    {
	m_errorCode = eCode; 
	m_ulErrorCode = (UINT32)atol(eCode);
    }
    const char* errorCode() { return m_errorCode; }
    const UINT32 errorCodeAsUINT32() { return m_ulErrorCode; }
    void setErrorMsg(const char* eMsg) { m_errorMsg = eMsg; }
    const char* errorMsg() { return m_errorMsg; }

    void asString(char* pBuf, int& msgLen);
    CHXString asString();

private:
    CHXString m_errorCode;
    CHXString m_errorMsg;
    UINT32 m_ulErrorCode;
};

/*
 *  Request messgage objects are separated by tags for the RTSP
 *  state machine to dispatch events.
 */

class RTSPUnknownMessage: public RTSPRequestMessage
{
public:
    RTSPMessage::Tag tag() const { return RTSPMessage::T_UNKNOWN; }
    RTSPMethod GetMethod() const { return RTSP_UNKNOWN; }
    const char* tagStr() const { return "UNKNOWN"; }
};

class RTSPSetupMessage: public RTSPRequestMessage
{
public:
    RTSPMessage::Tag tag() const { return RTSPMessage::T_SETUP; }
    RTSPMethod GetMethod() const { return RTSP_SETUP; }
    const char* tagStr() const { return "SETUP"; }
};

class RTSPRedirectMessage: public RTSPRequestMessage
{
public:
    RTSPMessage::Tag tag() const { return RTSPMessage::T_REDIRECT; }
    RTSPMethod GetMethod() const { return RTSP_REDIRECT; }
    const char* tagStr() const { return "REDIRECT"; }
};

class RTSPPlayMessage: public RTSPRequestMessage
{
public:
    RTSPMessage::Tag tag() const { return RTSPMessage::T_PLAY; }
    RTSPMethod GetMethod() const { return RTSP_PLAY; }
    const char* tagStr() const { return "PLAY"; }
};

class RTSPPauseMessage: public RTSPRequestMessage
{
public:
    RTSPMessage::Tag tag() const { return RTSPMessage::T_PAUSE; }
    RTSPMethod GetMethod() const { return RTSP_PAUSE; }
    const char* tagStr() const { return "PAUSE"; }
};

class RTSPSetParamMessage: public RTSPRequestMessage
{
public:
    RTSPMessage::Tag tag() const { return RTSPMessage::T_SET_PARAM; }
    RTSPMethod GetMethod() const { return RTSP_SET_PARAM; }
    const char* tagStr() const { return "SET_PARAMETER"; }
};

class RTSPGetParamMessage: public RTSPRequestMessage
{
public:
    RTSPMessage::Tag tag() const { return RTSPMessage::T_GET_PARAM; }
    RTSPMethod GetMethod() const { return RTSP_GET_PARAM; }
    const char* tagStr() const { return "GET_PARAMETER"; }
};

class RTSPTeardownMessage: public RTSPRequestMessage
{
public:
    RTSPMessage::Tag tag() const { return RTSPMessage::T_TEARDOWN; }
    RTSPMethod GetMethod() const { return RTSP_TEARDOWN; }
    const char* tagStr() const { return "TEARDOWN"; }
};

class RTSPOptionsMessage: public RTSPRequestMessage
{
public:
    RTSPOptionsMessage(HXBOOL bKeepAlive) { m_bKeepAlive = bKeepAlive; }
    RTSPMessage::Tag tag() const { return RTSPMessage::T_OPTIONS; }
    RTSPMethod GetMethod() const { return RTSP_OPTIONS; }
    const char* tagStr() const { return "OPTIONS"; }
    HXBOOL IsKeepAlive() const { return m_bKeepAlive; }
protected:
    HXBOOL    m_bKeepAlive;
};

class RTSPDescribeMessage: public RTSPRequestMessage
{
public:
    RTSPMessage::Tag tag() const { return RTSPMessage::T_DESCRIBE; }
    RTSPMethod GetMethod() const { return RTSP_DESCRIBE; }
    const char* tagStr() const { return "DESCRIBE"; }
};

class RTSPRecordMessage: public RTSPRequestMessage
{
public:
    RTSPMessage::Tag tag() const { return RTSPMessage::T_RECORD; }
    RTSPMethod GetMethod() const { return RTSP_RECORD; }
    const char* tagStr() const { return "RECORD"; }
};

class RTSPAnnounceMessage: public RTSPRequestMessage
{
public:
    RTSPMessage::Tag tag() const { return RTSPMessage::T_ANNOUNCE; }
    RTSPMethod GetMethod() const { return RTSP_ANNOUNCE; }
    const char* tagStr() const { return "ANNOUNCE"; }
};

#endif /* _RTSPMSG_H_ */
