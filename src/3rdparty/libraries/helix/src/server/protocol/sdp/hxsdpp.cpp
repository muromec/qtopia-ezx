/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: hxsdpp.cpp,v 1.6 2007/08/06 06:10:08 vijendrakumara Exp $ 
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

#include "hxtypes.h"

//XXXTDM: it would be nice if this was in hxtypes.h
#if !defined(_VXWORKS)
#ifdef _UNIX
#include <ctype.h>
#endif
#endif

#include <stdlib.h>
#include <stdio.h>

#include "hxcom.h"

#include "ihxpckts.h"

#include "ihxlist.h"

#include "hxsdp.h"
#include "hxsdpp.h"

#include "chxpckts.h"
#include "hxbuffer.h"
#include "hxsbuffer.h"
#include "hxlistp.h"

#ifdef _WINCE
#include <wincestr.h>
#endif

typedef const char* CPCHAR;

#define SDP_RES_FAIL    1
#define SDP_RES_PARTIAL 2
#define SDP_RES_OK      3

/*
 * Parse SDP line, which must be of the form: "([a-z])=(.*)"
 */
static int
sdp_parse_line( CPCHAR pbuf, UINT32 buflen, UINT32* plinelen, char* psdptype )
{
    UINT32 nleft;

    if( buflen < 3 || !isalpha(*pbuf) || *(pbuf+1) != '=' )
    {
        return SDP_RES_FAIL;
    }

    *psdptype = *pbuf;
    nleft = buflen;

    /* Find LF that terminates line */
    while( *pbuf != '\n' )
    {
        if( nleft <= 1 )
        {
            goto exit;
        }

        pbuf++;
        nleft--;
    }

    /* Advance past the LF */
 exit:
    pbuf++;
    nleft--;

    *plinelen = buflen - nleft;

    return SDP_RES_OK;
}

static UINT32
sdp_get_encoded_string_size( char* buf, UINT32 len )
{
    UINT32 esclen = len;
    UINT32 n;

    for( n = 0; n < len; n++ )
    {
        if( buf[n] == '\\' || buf[n] == '"' )
        {
            esclen++;
        }
    }

    return esclen;
}

static UINT32
sdp_get_decoded_string_size( char* buf, UINT32 len )
{
    UINT32 declen = len;
    UINT32 n;

    n = 0;
    while( n < len )
    {
        if( buf[n] == '\\' )
        {
            declen--;
            n++;
        }
        n++;
    }

    return declen;
}

static IHXBuffer*
sdp_encode_string( IHXBuffer* pbuf )
{
    IHXBuffer* pbufEnc = NULL;
    char*   ebuf;
    UINT32  ebuflen;
    char*   dbuf;
    UINT32  dbuflen;
    UINT32  n;

    pbufEnc = new CHXBuffer;
    pbufEnc->AddRef();

    dbuf = (char*)pbuf->GetBuffer();
    dbuflen = pbuf->GetSize();
    ebuflen = sdp_get_encoded_string_size( dbuf, dbuflen ) + 2 + 2;
    pbufEnc->SetSize( ebuflen );
    ebuf = (char*)pbufEnc->GetBuffer();
    *ebuf++ = '"';
    for( n = 0; n < dbuflen; n++ )
    {
        if( *dbuf == '\\' || *dbuf == '"' )
        {
            *ebuf++ = '\\';
        }
        *ebuf++ = *dbuf++;
    }
    *ebuf++ = '"';

    return pbufEnc;
}

static IHXBuffer*
sdp_decode_string( IHXBuffer* pbuf )
{
    IHXBuffer* pbufDec = NULL;
    char*   ebuf;
    UINT32  ebuflen;
    char*   dbuf;
    UINT32  dbuflen;
    UINT32  n;

    // We should be given a buffer of the form "escaped-string" CRLF
    ebuf = (char*)pbuf->GetBuffer();
    ebuflen = pbuf->GetSize();
    if( ebuflen < 3 || *ebuf != '"' )
    {
        return NULL;
    }
    ebuf++;
    ebuflen--;
    while( ebuflen > 0 &&
           (ebuf[ebuflen-1] == '\r' || ebuf[ebuflen-1] == '\n') )
    {
        ebuflen--;
    }
    //XXXTDM: allow empty string?
    if( ebuflen < 2 || ebuf[ebuflen-1] != '"' )
    {
        return NULL;
    }
    ebuflen--;

    dbuflen = sdp_get_decoded_string_size( ebuf, ebuflen );
    pbufDec = new CHXBuffer;
    pbufDec->AddRef();
    pbufDec->SetSize( dbuflen );
    dbuf = (char*)pbufDec->GetBuffer();

    for( n = 0; n < ebuflen; n++ )
    {
        if( *ebuf == '\\' )
        {
            ebuf++;
            n++;
        }
        *dbuf++ = *ebuf++;
    }

    return pbufDec;
}

static const char b64chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                               "abcdefghijklmnopqrstuvwxyz"
                               "0123456789+/";

static UINT32
sdp_get_encoded_buffer_size( IHXBuffer* pbuf )
{
    // Calculate ceil( 4/3 * buflen )
    UINT32 buflen = pbuf->GetSize();
    return (((buflen+2)/3)*4) + 2;
}

static IHXBuffer*
sdp_encode_buffer( IHXBuffer* pbuf )
{
    IHXBuffer* pbufEnc = NULL;
    char*   ebuf;
    UINT32  ebuflen;
    char*   dbuf;
    UINT32  dbuflen;
    int pos, n;

    dbuflen = pbuf->GetSize();
    dbuf = (char*)pbuf->GetBuffer();
    ebuflen = sdp_get_encoded_buffer_size( pbuf ) + 2 + 2;
    pbufEnc = new CHXBuffer;
    pbufEnc->AddRef();
    pbufEnc->SetSize( ebuflen );
    ebuf = (char*)pbufEnc->GetBuffer();

    *ebuf++ = '"';
    for( pos = 0; pos < dbuflen; pos += 3 )
    {
        UINT32 quad = 0;
        for( n = 0; n < 3; n++ )
        {
            quad <<= 8;
            if( pos+n < dbuflen )
            {
                quad += (UINT32)(*dbuf);
            }
            dbuf++;
        }
        for( n = 3; n >= 0; n-- )
        {
            if( quad & 0xFF )
            {
                ebuf[n] = b64chars[ quad & 0x3F ];
            }
            else
            {
                ebuf[n] = '=';
            }
            quad >>= 6;
        }
        ebuf += 4;
    }
    *ebuf++ = '"';

    return pbufEnc;
}

static IHXBuffer*
sdp_decode_buffer( IHXBuffer* pbuf )
{
    IHXBuffer* pbufDec = NULL;
    char*   ebuf;
    UINT32  ebuflen;
    char*   dbuf;
    UINT32  dbuflen;
    int pos, n;

    // We should be given a buffer of the form "base64-buffer" CRLF
    ebuf = (char*)pbuf->GetBuffer();
    ebuflen = pbuf->GetSize();
    if( ebuflen < 3 || *ebuf != '"' )
    {
        return NULL;
    }
    ebuf++;
    ebuflen--;
    while( ebuflen > 0 &&
           (ebuf[ebuflen-1] == '\r' || ebuf[ebuflen-1] == '\n') )
    {
        ebuflen--;
    }
    //XXXTDM: allow empty buffer?
    if( ebuflen < 5 || ebuf[ebuflen-1] != '"' )
    {
        return NULL;
    }
    ebuflen--;
    if( ebuflen%4 != 0 )
    {
        return NULL;
    }

    dbuflen = ebuflen/4 * 3;
    pbufDec = new CHXBuffer;
    pbufDec->AddRef();
    pbufDec->SetSize( dbuflen );
    dbuf = (char*)pbufDec->GetBuffer();

    for( pos = 0; pos < ebuflen; pos += 4 )
    {
        UINT32 trip = 0;
        for( n = 0; n < 4; n++ )
        {
            int val = 0;
            if( *ebuf >= 'A' && *ebuf <= 'Z' )
            {
                val = *ebuf - 'A' + 0;
            }
            else if( *ebuf >= 'a' && *ebuf <= 'z' )
            {
                val = *ebuf - 'a' + 26;
            }
            else if( *ebuf >= '0' && *ebuf <= '9' )
            {
                val = *ebuf - '0' + 52;
            }
            else if( *ebuf == '+' )
            {
                val = 62;
            }
            else if( *ebuf == '/' )
            {
                val = 63;
            }
            else if( *ebuf != '=' )
            {
                HX_ASSERT(FALSE);
                HX_RELEASE( pbufDec );
                return NULL;
            }
            trip = (trip << 6) + val;
            ebuf++;
        }
        for( n = 2; n >= 0; n-- )
        {
            dbuf[n] = trip & 0xFF;
            trip >>= 8;
        }
        dbuf += 3;
    }

    return pbufDec;
}

/*****************************************************************************
 *
 * CSDPAttrib
 *
 *****************************************************************************/

CSDPAttrib::CSDPAttrib( IHXFastAlloc* pFastAlloc ) :
    m_ulRefCount(0),
    m_pbufAttrib(NULL),
    m_pbufName(NULL),
    m_sdptype(SDP_ATTR_UNKNOWN),
    m_pFastAlloc(pFastAlloc)
{
    if (m_pFastAlloc)
    {
        m_pFastAlloc->AddRef();
    }
}

CSDPAttrib::~CSDPAttrib( void )
{
    if( (m_sdptype & SDP_ATTR_VAL) == SDP_ATTR_VAL )
    {
        if( m_sdptype != SDP_ATTR_VAL_INT )
        {
            HX_RELEASE( m_sdpval.bufval );
        }
    }
    HX_RELEASE( m_pbufName );
    HX_RELEASE( m_pbufAttrib );
    HX_RELEASE( m_pFastAlloc );
}

void
CSDPAttrib::Init( IHXBuffer* pbufAttrib )
{
    pbufAttrib->AddRef();
    m_pbufAttrib = pbufAttrib;
}

void
CSDPAttrib::Parse( void )
{
    HX_ASSERT( m_pbufAttrib != NULL );

    CPCHAR buf = (CPCHAR)m_pbufAttrib->GetBuffer();
    UINT32 len = m_pbufAttrib->GetSize();

    // If there is no colon, it's a flag
    CPCHAR pColon = (CPCHAR)memchr( buf, ':', len );
    if( pColon == NULL )
    {
        m_sdptype = SDP_ATTR_PROP;
        m_pbufName =
            new CHXStaticBuffer(m_pbufAttrib, 2, len-2);
        m_pbufName->AddRef();
    }
    else
    {
        UINT32 valpos;
        UINT32 vallen;

        m_pbufName =
            new CHXStaticBuffer(m_pbufAttrib, 2, (pColon-buf)-2);
        m_pbufName->AddRef();

        // See if it's an RN value
        if( strncmp( pColon, ":integer;", 9 ) == 0 )
        {
            m_sdptype = SDP_ATTR_VAL_INT;
            m_sdpval.ival = atoi( pColon+9 );
        }
        else if( strncmp( pColon, ":string;", 8 ) == 0 )
        {
            valpos = (pColon+8)-buf;
            vallen = len - valpos;
            while( buf[valpos+vallen-1] == '\r' || buf[valpos+vallen-1] == '\n' )
            {
                vallen--;
            }
            m_sdptype = SDP_ATTR_VAL_STR;
            m_sdpval.bufval =
                new CHXStaticBuffer(m_pbufAttrib, valpos,
                                                   vallen);
            m_sdpval.bufval->AddRef();
        }
        else if( strncmp( pColon, ":buffer;", 8 ) == 0 )
        {
            valpos = (pColon+8)-buf;
            vallen = len - valpos;
            while( buf[valpos+vallen-1] == '\r' || buf[valpos+vallen-1] == '\n' )
            {
                vallen--;
            }
            m_sdptype = SDP_ATTR_VAL_BUF;
            m_sdpval.bufval =
                new CHXStaticBuffer(m_pbufAttrib, valpos,
                                                   vallen);
            m_sdpval.bufval->AddRef();
        }
        else
        {
            valpos = (pColon+1)-buf;
            vallen = len - valpos;
            while( buf[valpos+vallen-1] == '\r' || buf[valpos+vallen-1] == '\n' )
            {
                vallen--;
            }
            m_sdptype = SDP_ATTR_VAL_GENERIC;
            m_sdpval.bufval =
                new CHXStaticBuffer(m_pbufAttrib, valpos,
                                                   len - valpos);
            m_sdpval.bufval->AddRef();
        }
    }
}

/*** IUnknown methods ***/

STDMETHODIMP
CSDPAttrib::QueryInterface( REFIID riid, void** ppvObj )
{
    if( IsEqualIID( riid, IID_IUnknown ) )
    {
        AddRef();
        *ppvObj = this;
        return HXR_OK;
    }
    else if( IsEqualIID( riid, IID_IHXSDPAttrib ) )
    {
        AddRef();
        *ppvObj = (IHXSDPAttrib*)this;
        return HXR_OK;
    }

    *ppvObj = NULL;

    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32)
CSDPAttrib::AddRef( void )
{
    return InterlockedIncrement( &m_ulRefCount );
}

STDMETHODIMP_(ULONG32)
CSDPAttrib::Release( void )
{
    HX_ASSERT( m_ulRefCount > 0 );
    if( InterlockedDecrement( &m_ulRefCount ) > 0 )
    {
        return m_ulRefCount;
    }

    delete this;

    return 0;
}

/*** IHXSDPAttrib methods ***/

STDMETHODIMP_(UINT32)
CSDPAttrib::GetSize( void )
{
    UINT32  len;
    char    numbuf[16];

    if( m_pbufAttrib != NULL )
    {
        return m_pbufAttrib->GetSize();
    }

    // Start with "a=" <name> CRLF
    len = 2 + m_pbufName->GetSize() + 2;
    switch( m_sdptype )
    {
    case SDP_ATTR_PROP:
        // Nothing else to add
        break;
    case SDP_ATTR_VAL_INT:  // Add ":integer;" <val>
        len += 9 + sprintf( numbuf, "%ld", m_sdpval.ival );
        break;
    case SDP_ATTR_VAL_GENERIC: // Add ":" <val>
        len += 1 + m_sdpval.bufval->GetSize();
        break;
    case SDP_ATTR_VAL_STR:  // Add ":string;" <quoted-string-buffer>
        len += 8 + m_sdpval.bufval->GetSize();
        break;
    case SDP_ATTR_VAL_BUF:  // Add ":buffer;" <quoted-base64-buffer>
        len += 8 + m_sdpval.bufval->GetSize();
        break;
    default:
        //note that we return 0 on error (garbage data?), caller should
        //allow for this situation
        HX_ASSERT(FALSE);
        return 0;
    }

    return len;
}

STDMETHODIMP_(UINT32)
CSDPAttrib::Write( BYTE* buf )
{
    BYTE* p;

    if( m_pbufAttrib != NULL )
    {
        memcpy( buf, m_pbufAttrib->GetBuffer(), m_pbufAttrib->GetSize() );
        return m_pbufAttrib->GetSize();
    }

    p = buf;

    *p++ = 'a';
    *p++ = '=';
    memcpy( p, m_pbufName->GetBuffer(), m_pbufName->GetSize() );
    p += m_pbufName->GetSize();
    switch( m_sdptype )
    {
    case SDP_ATTR_PROP:
        // Nothing else to add
        break;
    case SDP_ATTR_VAL_INT:  // Add ":integer;" <val>
        p += sprintf( (char*)p, ":integer;%ld", m_sdpval.ival );
        break;
    case SDP_ATTR_VAL_GENERIC: // Add ":" <val>
        *p++ = ':';
        memcpy( p, m_sdpval.bufval->GetBuffer(), m_sdpval.bufval->GetSize() );
        p += m_sdpval.bufval->GetSize();
        break;
    case SDP_ATTR_VAL_STR:  // Add ":string;" <quoted-string-buffer>
        p += sprintf( (char*)p, ":string;" );
        memcpy( p, m_sdpval.bufval->GetBuffer(), m_sdpval.bufval->GetSize() );
        p += m_sdpval.bufval->GetSize();
        break;
    case SDP_ATTR_VAL_BUF:  // Add ":buffer;" <quoted-base64-buffer>
        p += sprintf( (char*)p, ":buffer;" );
        memcpy( p, m_sdpval.bufval->GetBuffer(), m_sdpval.bufval->GetSize() );
        p += m_sdpval.bufval->GetSize();
        break;
    default:
        HX_ASSERT(FALSE);
        return HXR_FAIL;
    }
    *p++ = '\r';
    *p++ = '\n';

    return (p-buf);
}

STDMETHODIMP
CSDPAttrib::GetName( REF(IHXBuffer*) pbufName )
{
    if( m_pbufName == NULL )
    {
        Parse();
    }

    if( m_pbufName != NULL )
    {
        m_pbufName->AddRef();
    }
    pbufName = m_pbufName;

    return HXR_OK;
}

STDMETHODIMP
CSDPAttrib::SetName( IHXBuffer* pbufName )
{
    HX_RELEASE( m_pbufAttrib );
    HX_RELEASE( m_pbufName );
    pbufName->AddRef();
    m_pbufName = pbufName;

    return HXR_OK;
}

STDMETHODIMP_(UINT32)
CSDPAttrib::GetType( void )
{
    if( m_sdptype == SDP_ATTR_UNKNOWN )
    {
        Parse();
    }

    return m_sdptype;
}

STDMETHODIMP
CSDPAttrib::SetType( UINT32 sdptype )
{
    if( m_sdptype == SDP_ATTR_VAL_STR || m_sdptype == SDP_ATTR_VAL_BUF )
    {
        HX_RELEASE( m_sdpval.bufval );
    }

    m_sdptype = sdptype;

    if( m_sdptype == SDP_ATTR_VAL_STR || m_sdptype == SDP_ATTR_VAL_BUF )
    {
        m_sdpval.bufval = NULL;
    }

    return HXR_OK;
}

STDMETHODIMP
CSDPAttrib::GetValue( REF(IHXBuffer*) pbufValue )
{
    if( m_sdptype == SDP_ATTR_UNKNOWN )
    {
        Parse();
    }

    if( m_sdptype != SDP_ATTR_VAL_GENERIC )
    {
        pbufValue = NULL;
        return HXR_FAIL;
    }

    m_sdpval.bufval->AddRef();
    pbufValue = m_sdpval.bufval;

    return HXR_OK;
}

STDMETHODIMP
CSDPAttrib::SetValue( IHXBuffer* pbufValue )
{
    if( m_sdptype != SDP_ATTR_VAL_GENERIC )
    {
        return HXR_FAIL;
    }

    HX_RELEASE( m_sdpval.bufval );
    pbufValue->AddRef();
    m_sdpval.bufval = pbufValue;

    return HXR_OK;
}

STDMETHODIMP
CSDPAttrib::GetIntValue( REF(INT32) iValue )
{
    if( m_sdptype != SDP_ATTR_VAL_INT )
    {
        return HXR_FAIL;
    }

    iValue = m_sdpval.ival;

    return HXR_OK;
}

STDMETHODIMP
CSDPAttrib::SetIntValue( INT32 iValue )
{
    if( m_sdptype != SDP_ATTR_VAL_INT )
    {
        return HXR_FAIL;
    }

    HX_RELEASE( m_pbufAttrib );
    m_sdpval.ival = iValue;

    return HXR_OK;
}

STDMETHODIMP
CSDPAttrib::GetStrValue( REF(IHXBuffer*) pbufValue )
{
    if( m_sdptype != SDP_ATTR_VAL_STR )
    {
        return HXR_FAIL;
    }

    pbufValue = sdp_decode_string( m_sdpval.bufval );

    return HXR_OK;
}

STDMETHODIMP
CSDPAttrib::SetStrValue( IHXBuffer* pbufValue )
{
    if( m_sdptype != SDP_ATTR_VAL_STR )
    {
        return HXR_FAIL;
    }

    HX_RELEASE( m_pbufAttrib );
    HX_RELEASE( m_sdpval.bufval );
    m_sdpval.bufval = sdp_encode_string( pbufValue );

    return HXR_OK;
}

STDMETHODIMP
CSDPAttrib::GetBufValue( REF(IHXBuffer*) pbufValue )
{
    if( m_sdptype != SDP_ATTR_VAL_BUF )
    {
        return HXR_FAIL;
    }

    pbufValue = sdp_decode_buffer( m_sdpval.bufval );

    return HXR_OK;
}

STDMETHODIMP
CSDPAttrib::SetBufValue( IHXBuffer* pbufValue )
{
    if( m_sdptype != SDP_ATTR_VAL_BUF )
    {
        return HXR_FAIL;
    }

    HX_RELEASE( m_pbufAttrib );
    HX_RELEASE( m_sdpval.bufval );
    m_sdpval.bufval = sdp_encode_buffer( pbufValue );

    return HXR_OK;
}

/*****************************************************************************
 *
 * CSDPTimeDesc
 *
 *****************************************************************************/

CSDPTimeDesc::CSDPTimeDesc(IHXFastAlloc* pFastAlloc) :
    m_ulRefCount(0),
    m_pbufTime(NULL),
    m_plistRepeat(NULL),
    m_pFastAlloc(pFastAlloc)
{
    if (m_pFastAlloc)
    {
        m_pFastAlloc->AddRef();
    }
}

CSDPTimeDesc::~CSDPTimeDesc( void )
{
    HX_RELEASE( m_plistRepeat );
    HX_RELEASE( m_pbufTime );
    HX_RELEASE( m_pFastAlloc );
}

int
CSDPTimeDesc::Init( IHXBuffer* pbufSDP, UINT32& rpos )
{
    int         res;
    char        sdptype;
    CPCHAR      buf;
    UINT32      buflen;
    UINT32      linelen;
    IHXBuffer* pbufRep;
    IUnknown*   punk;

    buf = (CPCHAR)pbufSDP->GetBuffer();
    buflen = pbufSDP->GetSize();

    // m=
    res = sdp_parse_line( buf+rpos, buflen-rpos, &linelen, &sdptype );
    if( res != SDP_RES_OK || sdptype != 't' )
    {
        return SDP_RES_FAIL;
    }
    m_pbufTime = new CHXStaticBuffer(pbufSDP, rpos, linelen);
    m_pbufTime->AddRef();
    rpos += linelen;

    res = sdp_parse_line( buf+rpos, buflen-rpos, &linelen, &sdptype );
    while( rpos < buflen && res == SDP_RES_OK && sdptype == 'r' )
    {
        if( m_plistRepeat == NULL )
        {
            m_plistRepeat = new (m_pFastAlloc) CHXVector(m_pFastAlloc);
            m_plistRepeat->AddRef();
        }
        pbufRep = new CHXStaticBuffer(pbufSDP, rpos, linelen);
        pbufRep->QueryInterface( IID_IUnknown, (void**)&punk );
        m_plistRepeat->InsertTail( punk );
        punk->Release();
        rpos += linelen;
        res = sdp_parse_line( buf+rpos, buflen-rpos, &linelen, &sdptype );
    }

    return ( rpos == buflen || res == SDP_RES_OK ? SDP_RES_OK : SDP_RES_FAIL );
}

/*** IUnknown methods ***/

STDMETHODIMP
CSDPTimeDesc::QueryInterface( REFIID riid, void** ppvObj )
{
    if( IsEqualIID( riid, IID_IUnknown ) )
    {
        AddRef();
        *ppvObj = this;
        return HXR_OK;
    }
    else if( IsEqualIID( riid, IID_IHXSDPTimeDesc ) )
    {
        AddRef();
        *ppvObj = (IHXSDPTimeDesc*)this;
        return HXR_OK;
    }

    *ppvObj = NULL;

    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32)
CSDPTimeDesc::AddRef( void )
{
    return InterlockedIncrement( &m_ulRefCount );
}

STDMETHODIMP_(ULONG32)
CSDPTimeDesc::Release( void )
{
    HX_ASSERT( m_ulRefCount > 0 );
    if( InterlockedDecrement( &m_ulRefCount ) > 0 )
    {
        return m_ulRefCount;
    }

    delete this;

    return 0;
}

/*** IHXSDPTimeDesc methods ***/

STDMETHODIMP_(UINT32)
CSDPTimeDesc::GetSize( void )
{
    UINT32 len;
    IHXListIterator* pIter;
    IUnknown* punk;

    len = 0;

    HX_ASSERT( m_pbufTime != NULL );
    len += m_pbufTime->GetSize();

    if( m_plistRepeat != NULL )
    {
        IHXBuffer* pbufRep;
        pIter = m_plistRepeat->Begin();
        while( pIter->HasItem() )
        {
            punk = pIter->GetItem();
            punk->QueryInterface( IID_IHXBuffer, (void**)&pbufRep );
            HX_ASSERT( pbufRep != NULL );
            len += pbufRep->GetSize();
            pbufRep->Release();
            punk->Release();
            pIter->MoveNext();
        }
        pIter->Release();
    }

    return len;
}

STDMETHODIMP_(UINT32)
CSDPTimeDesc::Write( BYTE* pbuf )
{
    BYTE*  p;
    IHXListIterator* pIter;
    IUnknown* punk;

    p = pbuf;

    HX_ASSERT( m_pbufTime != NULL );
    memcpy( p, m_pbufTime->GetBuffer(), m_pbufTime->GetSize() );
    p += m_pbufTime->GetSize();

    if( m_plistRepeat != NULL )
    {
        IHXBuffer* pbufRep;
        pIter = m_plistRepeat->Begin();
        while( pIter->HasItem() )
        {
            punk = pIter->GetItem();
            punk->QueryInterface( IID_IHXBuffer, (void**)&pbufRep );
            HX_ASSERT( pbufRep != NULL );
            memcpy( p, pbufRep->GetBuffer(), pbufRep->GetSize() );
            p += pbufRep->GetSize();
            pbufRep->Release();
            punk->Release();
            pIter->MoveNext();
        }
        pIter->Release();
    }

    return (p-pbuf);
}

STDMETHODIMP
CSDPTimeDesc::GetTime( REF(IHXBuffer*) pbufTime )
{
    if( m_pbufTime != NULL )
    {
        m_pbufTime->AddRef();
    }
    pbufTime = m_pbufTime;

    return HXR_OK;
}

STDMETHODIMP
CSDPTimeDesc::SetTime( IHXBuffer* pbufTime )
{
    HX_RELEASE( m_pbufTime );
    if( pbufTime != NULL )
    {
        pbufTime->AddRef();
    }
    m_pbufTime = pbufTime;

    return HXR_OK;
}

STDMETHODIMP
CSDPTimeDesc::GetRepeatList( REF(IHXList*) plistRepeat )
{
    if( m_plistRepeat == NULL )
    {
        m_plistRepeat = new (m_pFastAlloc) CHXVector(m_pFastAlloc);
        m_plistRepeat->AddRef();
    }

    plistRepeat = m_plistRepeat;
    m_plistRepeat->AddRef();

    return HXR_OK;
}

STDMETHODIMP
CSDPTimeDesc::GetRepeatListConst( REF(IHXList*) plistRepeat )
{
    if( m_plistRepeat == NULL )
    {
        m_plistRepeat = new (m_pFastAlloc) CHXVector(m_pFastAlloc);
        m_plistRepeat->AddRef();
    }

    plistRepeat = m_plistRepeat->AsConst();

    return HXR_OK;
}

/*****************************************************************************
 *
 * CSDPMedia
 *
 *****************************************************************************/

CSDPMedia::CSDPMedia( IHXFastAlloc* pFastAlloc ) :
    m_ulRefCount(0),
    m_pbufMediaName(NULL),
    m_pbufTitle(NULL),
    m_pbufConnInfo(NULL),
    m_pbufBwidInfo(NULL),
    m_pbufEncryptKey(NULL),
    m_plistAttrib(NULL),
    m_pFastAlloc(pFastAlloc)
{
    if (m_pFastAlloc)
    {
        m_pFastAlloc->AddRef();
    }
}

CSDPMedia::~CSDPMedia( void )
{
    HX_RELEASE( m_plistAttrib );
    HX_RELEASE( m_pbufEncryptKey );
    HX_RELEASE( m_pbufBwidInfo );
    HX_RELEASE( m_pbufConnInfo );
    HX_RELEASE( m_pbufTitle );
    HX_RELEASE( m_pbufMediaName );
    HX_RELEASE( m_pFastAlloc );
}

/*
 * XXXTDM: this results in multiple passes over the m= lines: CSDP::Init
 * scans it to determine that a media block is starting, then we scan it
 * again.  Successive media blocks are scanned three times: once here to
 * determine that the media block has ended, once in CSDP::Init to determine
 * that it's another media block, and a third time here to process it.
 */
int
CSDPMedia::Init( IHXBuffer* pbufSDP, UINT32& rpos )
{
    int         res;
    char        sdptype;
    CPCHAR      buf;
    UINT32      buflen;
    UINT32      linelen;
    IHXBuffer* pbufAttrib;
    CSDPAttrib* pAttrib;
    IUnknown*   punk;

    buf = (CPCHAR)pbufSDP->GetBuffer();
    buflen = pbufSDP->GetSize();

    // m=
    res = sdp_parse_line( buf+rpos, buflen-rpos, &linelen, &sdptype );
    if( res != SDP_RES_OK || sdptype != 'm' )
    {
        return SDP_RES_FAIL;
    }
    m_pbufMediaName =
        new CHXStaticBuffer(pbufSDP, rpos, linelen);
    m_pbufMediaName->AddRef();
    rpos += linelen;

    res = sdp_parse_line( buf+rpos, buflen-rpos, &linelen, &sdptype );
    while( rpos < buflen && res == SDP_RES_OK && sdptype != 'm' )
    {
        switch( sdptype )
        {
        case 'i':
            HX_RELEASE( m_pbufTitle );
            m_pbufTitle =
                new CHXStaticBuffer( pbufSDP, rpos, linelen );
            m_pbufTitle->AddRef();
            rpos += linelen;
            res = sdp_parse_line( buf+rpos, buflen-rpos, &linelen, &sdptype );
            break;
        case 'c':
            HX_RELEASE( m_pbufConnInfo );
            m_pbufConnInfo =
                new CHXStaticBuffer( pbufSDP, rpos, linelen );
            m_pbufConnInfo->AddRef();
            rpos += linelen;
            res = sdp_parse_line( buf+rpos, buflen-rpos, &linelen, &sdptype );
            break;
        case 'b':
            HX_RELEASE( m_pbufBwidInfo );
            m_pbufBwidInfo =
                new CHXStaticBuffer( pbufSDP, rpos, linelen );
            m_pbufBwidInfo->AddRef();
            rpos += linelen;
            res = sdp_parse_line( buf+rpos, buflen-rpos, &linelen, &sdptype );
            break;
        case 'k':
            HX_RELEASE( m_pbufEncryptKey );
            m_pbufEncryptKey =
                new CHXStaticBuffer( pbufSDP, rpos, linelen );
            m_pbufEncryptKey->AddRef();
            rpos += linelen;
            res = sdp_parse_line( buf+rpos, buflen-rpos, &linelen, &sdptype );
            break;
        case 'a':
            if( m_plistAttrib == NULL )
            {
                m_plistAttrib = new (m_pFastAlloc) CHXVector(m_pFastAlloc);
                m_plistAttrib->AddRef();
            }
            pbufAttrib =
                new CHXStaticBuffer( pbufSDP, rpos, linelen );
            pAttrib = new (m_pFastAlloc) CSDPAttrib(m_pFastAlloc);
            pAttrib->Init( pbufAttrib );
            pAttrib->QueryInterface( IID_IUnknown, (void**)&punk );
            m_plistAttrib->InsertTail( punk );
            punk->Release();
            rpos += linelen;
            res = sdp_parse_line( buf+rpos, buflen-rpos, &linelen, &sdptype );
            break;
        default:
            return SDP_RES_FAIL; //XXX
        }
    }

    return ( rpos == buflen || res == SDP_RES_OK ? SDP_RES_OK : SDP_RES_FAIL );
}

/*** IUnknown methods ***/

STDMETHODIMP
CSDPMedia::QueryInterface( REFIID riid, void** ppvObj )
{
    if( IsEqualIID( riid, IID_IUnknown ) )
    {
        AddRef();
        *ppvObj = this;
        return HXR_OK;
    }
    else if( IsEqualIID( riid, IID_IHXSDPMedia ) )
    {
        AddRef();
        *ppvObj = (IHXSDPMedia*)this;
        return HXR_OK;
    }

    *ppvObj = NULL;

    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32)
CSDPMedia::AddRef( void )
{
    return InterlockedIncrement( &m_ulRefCount );
}

STDMETHODIMP_(ULONG32)
CSDPMedia::Release( void )
{
    HX_ASSERT( m_ulRefCount > 0 );
    if( InterlockedDecrement( &m_ulRefCount ) > 0 )
    {
        return m_ulRefCount;
    }

    delete this;

    return 0;
}

/*** IHXSDPMedia methods ***/

STDMETHODIMP_(UINT32)
CSDPMedia::GetSize( void )
{
    UINT32 len;
    IHXListIterator* pIter;
    IUnknown* punk;

    len = 0;

    HX_ASSERT( m_pbufMediaName != NULL );
    len += m_pbufMediaName->GetSize();

    if( m_pbufTitle != NULL )
    {
        len += m_pbufTitle->GetSize();
    }

    if( m_pbufConnInfo != NULL )
    {
        len += m_pbufConnInfo->GetSize();
    }

    if( m_pbufBwidInfo != NULL )
    {
        len += m_pbufBwidInfo->GetSize();
    }

    if( m_pbufEncryptKey != NULL )
    {
        len += m_pbufEncryptKey->GetSize();
    }

    if( m_plistAttrib != NULL )
    {
        IHXBuffer* pbufAttr;
        pIter = m_plistAttrib->Begin();
        while( pIter->HasItem() )
        {
            punk = pIter->GetItem();
            punk->QueryInterface( IID_IHXBuffer, (void**)&pbufAttr );
            HX_ASSERT( pbufAttr != NULL );
            len += pbufAttr->GetSize();
            pbufAttr->Release();
            punk->Release();
            pIter->MoveNext();
        }
        pIter->Release();
    }

    return len;
}

STDMETHODIMP_(UINT32)
CSDPMedia::Write( BYTE* pbuf )
{
    BYTE*  p;
    IHXListIterator* pIter;
    IUnknown* punk;

    p = pbuf;

    HX_ASSERT( m_pbufMediaName != NULL );
    memcpy( p, m_pbufMediaName->GetBuffer(), m_pbufMediaName->GetSize() );
    p += m_pbufMediaName->GetSize();

    if( m_pbufTitle != NULL )
    {
        memcpy( p, m_pbufTitle->GetBuffer(), m_pbufTitle->GetSize() );
        p += m_pbufTitle->GetSize();
    }

    if( m_pbufConnInfo != NULL )
    {
        memcpy( p, m_pbufConnInfo->GetBuffer(), m_pbufConnInfo->GetSize() );
        p += m_pbufConnInfo->GetSize();
    }

    if( m_pbufBwidInfo != NULL )
    {
        memcpy( p, m_pbufBwidInfo->GetBuffer(), m_pbufBwidInfo->GetSize() );
        p += m_pbufBwidInfo->GetSize();
    }

    if( m_pbufEncryptKey != NULL )
    {
        memcpy( p, m_pbufEncryptKey->GetBuffer(), m_pbufEncryptKey->GetSize() );
        p += m_pbufEncryptKey->GetSize();
    }

    if( m_plistAttrib != NULL )
    {
        IHXBuffer* pbufAttr;
        pIter = m_plistAttrib->Begin();
        while( pIter->HasItem() )
        {
            punk = pIter->GetItem();
            punk->QueryInterface( IID_IHXBuffer, (void**)&pbufAttr );
            HX_ASSERT( pbufAttr != NULL );
            memcpy( p, pbufAttr->GetBuffer(), pbufAttr->GetSize() );
            p += pbufAttr->GetSize();
            pbufAttr->Release();
            punk->Release();
            pIter->MoveNext();
        }
        pIter->Release();
    }

    return (p-pbuf);
}

STDMETHODIMP
CSDPMedia::GetMediaName( REF(IHXBuffer*) pbufMediaName )
{
    if( m_pbufMediaName != NULL )
    {
        m_pbufMediaName->AddRef();
    }
    pbufMediaName = m_pbufMediaName;

    return HXR_OK;
}

STDMETHODIMP
CSDPMedia::SetMediaName( IHXBuffer* pbufMediaName )
{
    HX_RELEASE( m_pbufMediaName );
    if( pbufMediaName != NULL )
    {
        pbufMediaName->AddRef();
    }
    m_pbufMediaName = pbufMediaName;

    return HXR_OK;
}

STDMETHODIMP
CSDPMedia::GetTitle( REF(IHXBuffer*) pbufTitle )
{
    if( m_pbufTitle != NULL )
    {
        m_pbufTitle->AddRef();
    }
    pbufTitle = m_pbufTitle;

    return HXR_OK;
}

STDMETHODIMP
CSDPMedia::SetTitle( IHXBuffer* pbufTitle )
{
    HX_RELEASE( m_pbufTitle );
    if( pbufTitle != NULL )
    {
        pbufTitle->AddRef();
    }
    m_pbufTitle = pbufTitle;

    return HXR_OK;
}

STDMETHODIMP
CSDPMedia::GetConnInfo( REF(IHXBuffer*) pbufConnInfo )
{
    if( m_pbufConnInfo != NULL )
    {
        m_pbufConnInfo->AddRef();
    }
    pbufConnInfo = m_pbufConnInfo;

    return HXR_OK;
}

STDMETHODIMP
CSDPMedia::SetConnInfo( IHXBuffer* pbufConnInfo )
{
    HX_RELEASE( m_pbufConnInfo );
    if( pbufConnInfo != NULL )
    {
        pbufConnInfo->AddRef();
    }
    m_pbufConnInfo = pbufConnInfo;

    return HXR_OK;
}

STDMETHODIMP
CSDPMedia::GetBwidInfo( REF(IHXBuffer*) pbufBwidInfo )
{
    if( m_pbufBwidInfo != NULL )
    {
        m_pbufBwidInfo->AddRef();
    }
    pbufBwidInfo = m_pbufBwidInfo;

    return HXR_OK;
}

STDMETHODIMP
CSDPMedia::SetBwidInfo( IHXBuffer* pbufBwidInfo )
{
    HX_RELEASE( m_pbufBwidInfo );
    if( pbufBwidInfo != NULL )
    {
        pbufBwidInfo->AddRef();
    }
    m_pbufBwidInfo = pbufBwidInfo;

    return HXR_OK;
}

STDMETHODIMP
CSDPMedia::GetEncryptKey( REF(IHXBuffer*) pbufEncryptKey )
{
    if( m_pbufEncryptKey != NULL )
    {
        m_pbufEncryptKey->AddRef();
    }
    pbufEncryptKey = m_pbufEncryptKey;

    return HXR_OK;
}

STDMETHODIMP
CSDPMedia::SetEncryptKey( IHXBuffer* pbufEncryptKey )
{
    HX_RELEASE( m_pbufEncryptKey );
    if( pbufEncryptKey != NULL )
    {
        pbufEncryptKey->AddRef();
    }
    m_pbufEncryptKey = pbufEncryptKey;

    return HXR_OK;
}

STDMETHODIMP
CSDPMedia::GetAttribList( REF(IHXList*) plistAttrib )
{
    if( m_plistAttrib == NULL )
    {
        m_plistAttrib = new (m_pFastAlloc) CHXVector(m_pFastAlloc);
        m_plistAttrib->AddRef();
    }

    plistAttrib = m_plistAttrib;
    m_plistAttrib->AddRef();

    return HXR_OK;
}

STDMETHODIMP
CSDPMedia::GetAttribListConst( REF(IHXList*) plistAttrib )
{
    if( m_plistAttrib == NULL )
    {
        m_plistAttrib = new (m_pFastAlloc) CHXVector(m_pFastAlloc);
        m_plistAttrib->AddRef();
    }

    plistAttrib = m_plistAttrib->AsConst();

    return HXR_OK;
}

/*****************************************************************************
 *
 * CSDP
 *
 *****************************************************************************/

CSDP::CSDP(IHXFastAlloc* pFastAlloc) :
    m_ulRefCount(0),
    m_pbufSDP(NULL),
    m_nVersion(0),
    m_pbufOrigin(NULL),
    m_pbufName(NULL),
    m_pbufInfo(NULL),
    m_pbufUrl(NULL),
    m_pbufEmail(NULL),
    m_pbufPhone(NULL),
    m_pbufConnInfo(NULL),
    m_pbufBwidInfo(NULL),
    m_plistTimeDesc(NULL),
    m_pbufTimeZone(NULL),
    m_pbufEncryptKey(NULL),
    m_plistAttrib(NULL),
    m_plistMedia(NULL),
    m_pFastAlloc(pFastAlloc)
{
    if (m_pFastAlloc)
    {
        m_pFastAlloc->AddRef();
    }
}

CSDP::~CSDP( void )
{
    HX_RELEASE( m_plistMedia );
    HX_RELEASE( m_plistAttrib );
    HX_RELEASE( m_pbufEncryptKey );
    HX_RELEASE( m_pbufTimeZone );
    HX_RELEASE( m_plistTimeDesc );
    HX_RELEASE( m_pbufBwidInfo );
    HX_RELEASE( m_pbufConnInfo );
    HX_RELEASE( m_pbufPhone );
    HX_RELEASE( m_pbufEmail );
    HX_RELEASE( m_pbufUrl );
    HX_RELEASE( m_pbufInfo );
    HX_RELEASE( m_pbufName );
    HX_RELEASE( m_pbufOrigin );
    HX_RELEASE( m_pbufSDP );
    HX_RELEASE( m_pFastAlloc );
}

/*
 * Incoming SDP is always in a single, complete buffer so we don't need to
 * worry about keeping state (as we do for incoming RTSP messages).  This
 * may change in the future.  In reality, the server rarely gets incoming SDP
 * so the motivation to sacrifice simplicity for performance is reduced.
 */
HX_RESULT
CSDP::Init( IHXBuffer* pbufSDP )
{
    int             res;
    char            sdptype;
    CPCHAR          buf;
    UINT32          buflen;
    UINT32          pos;
    UINT32          linelen;
    CSDPTimeDesc*   pDesc;
    IHXBuffer*     pbufAttrib;
    CSDPAttrib*     pAttrib;
    IUnknown*       punk;

    buf = (CPCHAR)pbufSDP->GetBuffer();
    buflen = pbufSDP->GetSize();
    pos = 0;

    // v=
    res = sdp_parse_line( buf+pos, buflen-pos, &linelen, &sdptype );
    if( res != SDP_RES_OK || sdptype != 'v' )
    {
        return HXR_FAIL;
    }
    m_nVersion = atoi( buf+pos+2 );
    pos += linelen;

    // o=
    res = sdp_parse_line( buf+pos, buflen-pos, &linelen, &sdptype );
    if( res != SDP_RES_OK || sdptype != 'o' )
    {
        return HXR_FAIL;
    }
    m_pbufOrigin = new CHXStaticBuffer( pbufSDP, pos, linelen );
    m_pbufOrigin->AddRef();
    pos += linelen;

    // s=
    res = sdp_parse_line( buf+pos, buflen-pos, &linelen, &sdptype );
    if( res != SDP_RES_OK || sdptype != 's' )
    {
        return HXR_FAIL;
    }
    m_pbufName = new CHXStaticBuffer( pbufSDP, pos, linelen );
    m_pbufName->AddRef();
    pos += linelen;

    res = sdp_parse_line( buf+pos, buflen-pos, &linelen, &sdptype );
    while( res == SDP_RES_OK && sdptype != 'm' )
    {
        switch( sdptype )
        {
        case 'i':
            HX_RELEASE( m_pbufInfo );
            m_pbufInfo =
                new CHXStaticBuffer( pbufSDP, pos, linelen );
            m_pbufInfo->AddRef();
            pos += linelen;
            res = sdp_parse_line( buf+pos, buflen-pos, &linelen, &sdptype );
            break;
        case 'u':
            HX_RELEASE( m_pbufUrl );
            m_pbufUrl =
                new CHXStaticBuffer( pbufSDP, pos, linelen );
            m_pbufUrl->AddRef();
            pos += linelen;
            res = sdp_parse_line( buf+pos, buflen-pos, &linelen, &sdptype );
            break;
        case 'e':
            HX_RELEASE( m_pbufEmail );
            m_pbufEmail =
                new CHXStaticBuffer( pbufSDP, pos, linelen );
            m_pbufEmail->AddRef();
            pos += linelen;
            res = sdp_parse_line( buf+pos, buflen-pos, &linelen, &sdptype );
            break;
        case 'p':
            HX_RELEASE( m_pbufPhone );
            m_pbufPhone =
                new CHXStaticBuffer( pbufSDP, pos, linelen );
            m_pbufPhone->AddRef();
            pos += linelen;
            res = sdp_parse_line( buf+pos, buflen-pos, &linelen, &sdptype );
            break;
        case 'c':
            HX_RELEASE( m_pbufConnInfo );
            m_pbufConnInfo =
                new CHXStaticBuffer( pbufSDP, pos, linelen );
            m_pbufConnInfo->AddRef();
            pos += linelen;
            res = sdp_parse_line( buf+pos, buflen-pos, &linelen, &sdptype );
            break;
        case 'b':
            HX_RELEASE( m_pbufBwidInfo );
            m_pbufBwidInfo =
                new CHXStaticBuffer( pbufSDP, pos, linelen );
            m_pbufBwidInfo->AddRef();
            pos += linelen;
            res = sdp_parse_line( buf+pos, buflen-pos, &linelen, &sdptype );
            break;
        case 't':
            pDesc = new (m_pFastAlloc) CSDPTimeDesc(m_pFastAlloc);
            res = pDesc->Init( pbufSDP, pos );
            if( res != SDP_RES_OK )
            {
                break;
            }
            if( m_plistTimeDesc == NULL )
            {
                m_plistTimeDesc = new (m_pFastAlloc) CHXVector(m_pFastAlloc);
                m_plistTimeDesc->AddRef();
            }
            pDesc->QueryInterface( IID_IUnknown, (void**)&punk );
            m_plistTimeDesc->InsertTail( punk );
            punk->Release();
            res = sdp_parse_line( buf+pos, buflen-pos, &linelen, &sdptype );
            break;
        case 'z':
            HX_RELEASE( m_pbufTimeZone );
            m_pbufTimeZone =
                new CHXStaticBuffer( pbufSDP, pos, linelen );
            m_pbufTimeZone->AddRef();
            pos += linelen;
            res = sdp_parse_line( buf+pos, buflen-pos, &linelen, &sdptype );
            break;
        case 'k':
            HX_RELEASE( m_pbufEncryptKey );
            m_pbufEncryptKey =
                new CHXStaticBuffer( pbufSDP, pos, linelen );
            m_pbufEncryptKey->AddRef();
            pos += linelen;
            res = sdp_parse_line( buf+pos, buflen-pos, &linelen, &sdptype );
            break;
        case 'a':
            if( m_plistAttrib == NULL )
            {
                m_plistAttrib = new (m_pFastAlloc) CHXVector(m_pFastAlloc);
                m_plistAttrib->AddRef();
            }
            pbufAttrib = new CHXStaticBuffer( pbufSDP, pos, linelen );
            pAttrib = new (m_pFastAlloc) CSDPAttrib(m_pFastAlloc);
            pAttrib->Init( pbufAttrib );
            pAttrib->QueryInterface( IID_IUnknown, (void**)&punk );
            m_plistAttrib->InsertTail( punk );
            punk->Release();
            pos += linelen;
            res = sdp_parse_line( buf+pos, buflen-pos, &linelen, &sdptype );
            break;
        default:
            return HXR_FAIL; //XXX
            break;
        }
    }

    while( res == SDP_RES_OK && sdptype == 'm' )
    {
        CSDPMedia* pMedia = new (m_pFastAlloc) CSDPMedia(m_pFastAlloc);
        res = pMedia->Init( pbufSDP, pos );
        if( res != SDP_RES_OK )
        {
            break;
        }
        if( m_plistMedia == NULL )
        {
            m_plistMedia = new (m_pFastAlloc) CHXVector(m_pFastAlloc);
            m_plistMedia->AddRef();
        }
        pMedia->QueryInterface( IID_IUnknown, (void**)&punk );
        m_plistMedia->InsertTail( punk );
        punk->Release();

        res = sdp_parse_line( buf+pos, buflen-pos, &linelen, &sdptype );
    }

    // We must have used the entire buffer to be successful
    if( pos == buflen )
    {
        pbufSDP->AddRef();
        m_pbufSDP = pbufSDP;
        return HXR_OK;
    }

    return HXR_FAIL;
}

/*** IUnknown methods ***/

STDMETHODIMP
CSDP::QueryInterface( REFIID riid, void** ppvObj )
{
    if( IsEqualIID( riid, IID_IUnknown ) )
    {
        AddRef();
        *ppvObj = this;
        return HXR_OK;
    }
    else if( IsEqualIID( riid, IID_IHXSDP ) )
    {
        AddRef();
        *ppvObj = (IHXSDP*)this;
        return HXR_OK;
    }

    *ppvObj = NULL;

    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32)
CSDP::AddRef( void )
{
    return InterlockedIncrement( &m_ulRefCount );
}

STDMETHODIMP_(ULONG32)
CSDP::Release( void )
{
    HX_ASSERT( m_ulRefCount > 0 );
    if( InterlockedDecrement( &m_ulRefCount ) > 0 )
    {
        return m_ulRefCount;
    }

    delete this;

    return 0;
}

/*** IHXSDP methods ***/

STDMETHODIMP_(UINT32)
CSDP::GetSize( void )
{
    UINT32 len;
    IHXListIterator* pIter;
    IUnknown* punk;

    if( m_pbufSDP != NULL )
    {
        return m_pbufSDP->GetSize();
    }

    len = 0;

    char tmpbuf[2+12+2+1];
    len += sprintf( tmpbuf, "v=%u\r\n", (unsigned int)m_nVersion );

    HX_ASSERT( m_pbufOrigin != NULL );
    len += m_pbufOrigin->GetSize();

    HX_ASSERT( m_pbufName != NULL );
    len += m_pbufName->GetSize();

    if( m_pbufInfo != NULL )
    {
       len += m_pbufInfo->GetSize();
    }

    if( m_pbufUrl != NULL )
    {
       len += m_pbufUrl->GetSize();
    }

    if( m_pbufEmail != NULL )
    {
       len += m_pbufEmail->GetSize();
    }

    if( m_pbufPhone != NULL )
    {
       len += m_pbufPhone->GetSize();
    }

    if( m_pbufConnInfo != NULL )
    {
       len += m_pbufConnInfo->GetSize();
    }

    if( m_pbufBwidInfo != NULL )
    {
       len += m_pbufBwidInfo->GetSize();
    }

    if( m_plistTimeDesc != NULL )
    {
        IHXSDPTimeDesc* pDesc;
        pIter = m_plistTimeDesc->Begin();
        while( pIter->HasItem() )
        {
            punk = pIter->GetItem();
            punk->QueryInterface( IID_IHXSDPTimeDesc, (void**)&pDesc );
            HX_ASSERT( pDesc != NULL );
            len += pDesc->GetSize();
            pDesc->Release();
            punk->Release();
            pIter->MoveNext();
        }
        pIter->Release();
    }

    if( m_pbufTimeZone != NULL )
    {
       len += m_pbufTimeZone->GetSize();
    }

    if( m_pbufEncryptKey != NULL )
    {
        len += m_pbufEncryptKey->GetSize();
    }

    if( m_plistAttrib != NULL )
    {
        IHXBuffer* pbufAttr;
        pIter = m_plistAttrib->Begin();
        while( pIter->HasItem() )
        {
            punk = pIter->GetItem();
            punk->QueryInterface( IID_IHXBuffer, (void**)&pbufAttr );
            HX_ASSERT( pbufAttr != NULL );
            len += pbufAttr->GetSize();
            pbufAttr->Release();
            punk->Release();
            pIter->MoveNext();
        }
        pIter->Release();
    }

    if( m_plistMedia != NULL )
    {
        IHXSDPMedia* pMedia;
        pIter = m_plistMedia->Begin();
        while( pIter->HasItem() )
        {
            punk = pIter->GetItem();
            punk->QueryInterface( IID_IHXSDPMedia, (void**)&pMedia );
            HX_ASSERT( pMedia != NULL );
            len += pMedia->GetSize();
            pMedia->Release();
            punk->Release();
            pIter->MoveNext();
        }
        pIter->Release();
    }

    return len;
}

STDMETHODIMP_(UINT32)
CSDP::Write( BYTE* pbuf )
{
    BYTE*  p;
    IHXListIterator* pIter;
    IUnknown* punk;

    if( m_pbufSDP != NULL )
    {
        memcpy( pbuf, m_pbufSDP->GetBuffer(), m_pbufSDP->GetSize() );
        return m_pbufSDP->GetSize();
    }

    p = pbuf;

    p += sprintf( (char*)p, "v=%u\r\n", (unsigned int)m_nVersion );

    HX_ASSERT( m_pbufOrigin != NULL );
    memcpy( p, m_pbufOrigin->GetBuffer(), m_pbufOrigin->GetSize() );
    p += m_pbufOrigin->GetSize();

    HX_ASSERT( m_pbufName != NULL );
    memcpy( p, m_pbufName->GetBuffer(), m_pbufName->GetSize() );
    p += m_pbufName->GetSize();

    if( m_pbufInfo != NULL )
    {
        memcpy( p, m_pbufInfo->GetBuffer(), m_pbufInfo->GetSize() );
        p += m_pbufInfo->GetSize();
    }

    if( m_pbufUrl != NULL )
    {
        memcpy( p, m_pbufUrl->GetBuffer(), m_pbufUrl->GetSize() );
        p += m_pbufUrl->GetSize();
    }

    if( m_pbufEmail != NULL )
    {
        memcpy( p, m_pbufEmail->GetBuffer(), m_pbufEmail->GetSize() );
        p += m_pbufEmail->GetSize();
    }

    if( m_pbufPhone != NULL )
    {
        memcpy( p, m_pbufPhone->GetBuffer(), m_pbufPhone->GetSize() );
        p += m_pbufPhone->GetSize();
    }

    if( m_pbufConnInfo != NULL )
    {
        memcpy( p, m_pbufConnInfo->GetBuffer(), m_pbufConnInfo->GetSize() );
        p += m_pbufConnInfo->GetSize();
    }

    if( m_pbufBwidInfo != NULL )
    {
        memcpy( p, m_pbufBwidInfo->GetBuffer(), m_pbufBwidInfo->GetSize() );
        p += m_pbufBwidInfo->GetSize();
    }

    if( m_plistTimeDesc != NULL )
    {
        IHXSDPTimeDesc* pDesc = NULL;
        pIter = m_plistTimeDesc->Begin();
        while( pIter->HasItem() )
        {
            punk = pIter->GetItem();
            punk->QueryInterface( IID_IHXSDPTimeDesc, (void**)&pDesc );
            HX_ASSERT( pDesc != NULL );
            p += pDesc->Write( p );
            pDesc->Release();
            punk->Release();
            pIter->MoveNext();
        }
        pIter->Release();
    }

    if( m_pbufTimeZone != NULL )
    {
        memcpy( p, m_pbufTimeZone->GetBuffer(), m_pbufTimeZone->GetSize() );
        p += m_pbufTimeZone->GetSize();
    }

    if( m_pbufEncryptKey != NULL )
    {
        memcpy( p, m_pbufEncryptKey->GetBuffer(), m_pbufEncryptKey->GetSize() );
        p += m_pbufEncryptKey->GetSize();
    }

    if( m_plistAttrib != NULL )
    {
        IHXBuffer* pbufAttr;
        pIter = m_plistAttrib->Begin();
        while( pIter->HasItem() )
        {
            punk = pIter->GetItem();
            punk->QueryInterface( IID_IHXBuffer, (void**)&pbufAttr );
            HX_ASSERT( pbufAttr != NULL );
            memcpy( p, pbufAttr->GetBuffer(), pbufAttr->GetSize() );
            p += pbufAttr->GetSize();
            pbufAttr->Release();
            punk->Release();
            pIter->MoveNext();
        }
        pIter->Release();
    }

    if( m_plistMedia != NULL )
    {
        IHXSDPMedia* pMedia;
        pIter = m_plistMedia->Begin();
        while( pIter->HasItem() )
        {
            punk = pIter->GetItem();
            punk->QueryInterface( IID_IHXSDPMedia, (void**)&pMedia );
            HX_ASSERT( pMedia != NULL );
            p += pMedia->Write( p );
            pMedia->Release();
            punk->Release();
            pIter->MoveNext();
        }
        pIter->Release();
    }

    return (p-pbuf);
}

STDMETHODIMP_(IHXBuffer*)
CSDP::AsBuffer( void )
{
    if( m_pbufSDP != NULL )
    {
        m_pbufSDP->AddRef();
        return m_pbufSDP;
    }

    IHXBuffer* pbuf = new CHXBuffer;
    pbuf->AddRef();
    pbuf->SetSize( GetSize() );
    Write( pbuf->GetBuffer() );
    return pbuf;
}

STDMETHODIMP_(UINT32)
CSDP::GetVersion( void )
{
    return m_nVersion;
}

STDMETHODIMP
CSDP::SetVersion( UINT32 ver )
{
    HX_RELEASE( m_pbufSDP );

    m_nVersion = ver;

    return HXR_OK;
}

STDMETHODIMP
CSDP::GetOrigin( REF(IHXBuffer*) pbufOrigin )
{
    if( m_pbufOrigin != NULL )
    {
        m_pbufOrigin->AddRef();
    }
    pbufOrigin = m_pbufOrigin;

    return HXR_OK;
}

STDMETHODIMP
CSDP::SetOrigin( IHXBuffer* pbufOrigin )
{
    HX_RELEASE( m_pbufSDP );
    HX_RELEASE( m_pbufOrigin );
    if( pbufOrigin != NULL )
    {
        pbufOrigin->AddRef();
    }
    m_pbufOrigin = pbufOrigin;

    return HXR_OK;
}

STDMETHODIMP
CSDP::GetName( REF(IHXBuffer*) pbufName )
{
    if( m_pbufName != NULL )
    {
        m_pbufName->AddRef();
    }
    pbufName = m_pbufName;

    return HXR_OK;
}

STDMETHODIMP
CSDP::SetName( IHXBuffer* pbufName )
{
    HX_RELEASE( m_pbufSDP );
    HX_RELEASE( m_pbufName );
    if( pbufName != NULL )
    {
        pbufName->AddRef();
    }
    m_pbufName = pbufName;

    return HXR_OK;
}

STDMETHODIMP
CSDP::GetInfo( REF(IHXBuffer*) pbufInfo )
{
    if( m_pbufInfo != NULL )
    {
        m_pbufInfo->AddRef();
    }
    pbufInfo = m_pbufInfo;

    return HXR_OK;
}

STDMETHODIMP
CSDP::SetInfo( IHXBuffer* pbufInfo )
{
    HX_RELEASE( m_pbufSDP );
    HX_RELEASE( m_pbufInfo );
    if( pbufInfo != NULL )
    {
        pbufInfo->AddRef();
    }
    m_pbufInfo = pbufInfo;

    return HXR_OK;
}

STDMETHODIMP
CSDP::GetUrl( REF(IHXBuffer*) pbufUrl )
{
    if( m_pbufUrl != NULL )
    {
        m_pbufUrl->AddRef();
    }
    pbufUrl = m_pbufUrl;

    return HXR_OK;
}

STDMETHODIMP
CSDP::SetUrl( IHXBuffer* pbufUrl )
{
    HX_RELEASE( m_pbufSDP );
    HX_RELEASE( m_pbufUrl );
    if( pbufUrl != NULL )
    {
        pbufUrl->AddRef();
    }
    m_pbufUrl = pbufUrl;

    return HXR_OK;
}

STDMETHODIMP
CSDP::GetEmail( REF(IHXBuffer*) pbufEmail )
{
    if( m_pbufEmail != NULL )
    {
        m_pbufEmail->AddRef();
    }
    pbufEmail = m_pbufEmail;

    return HXR_OK;
}

STDMETHODIMP
CSDP::SetEmail( IHXBuffer* pbufEmail )
{
    HX_RELEASE( m_pbufSDP );
    HX_RELEASE( m_pbufEmail );
    if( pbufEmail != NULL )
    {
        pbufEmail->AddRef();
    }
    m_pbufEmail = pbufEmail;

    return HXR_OK;
}

STDMETHODIMP
CSDP::GetPhone( REF(IHXBuffer*) pbufPhone )
{
    if( m_pbufPhone != NULL )
    {
        m_pbufPhone->AddRef();
    }
    pbufPhone = m_pbufPhone;

    return HXR_OK;
}

STDMETHODIMP
CSDP::SetPhone( IHXBuffer* pbufPhone )
{
    HX_RELEASE( m_pbufSDP );
    HX_RELEASE( m_pbufPhone );
    if( pbufPhone != NULL )
    {
        pbufPhone->AddRef();
    }
    m_pbufPhone = pbufPhone;

    return HXR_OK;
}

STDMETHODIMP
CSDP::GetConnInfo( REF(IHXBuffer*) pbufConnInfo )
{
    if( m_pbufConnInfo != NULL )
    {
        m_pbufConnInfo->AddRef();
    }
    pbufConnInfo = m_pbufConnInfo;

    return HXR_OK;
}

STDMETHODIMP
CSDP::SetConnInfo( IHXBuffer* pbufConnInfo )
{
    HX_RELEASE( m_pbufSDP );
    HX_RELEASE( m_pbufConnInfo );
    if( pbufConnInfo != NULL )
    {
        pbufConnInfo->AddRef();
    }
    m_pbufConnInfo = pbufConnInfo;

    return HXR_OK;
}

STDMETHODIMP
CSDP::GetBwidInfo( REF(IHXBuffer*) pbufBwidInfo )
{
    if( m_pbufBwidInfo != NULL )
    {
        m_pbufBwidInfo->AddRef();
    }
    pbufBwidInfo = m_pbufBwidInfo;

    return HXR_OK;
}

STDMETHODIMP
CSDP::SetBwidInfo( IHXBuffer* pbufBwidInfo )
{
    HX_RELEASE( m_pbufSDP );
    HX_RELEASE( m_pbufBwidInfo );
    if( pbufBwidInfo != NULL )
    {
        pbufBwidInfo->AddRef();
    }
    m_pbufBwidInfo = pbufBwidInfo;

    return HXR_OK;
}

STDMETHODIMP
CSDP::GetTimeDescList( REF(IHXList*) plistTimeDesc )
{
    HX_RELEASE( m_pbufSDP );
    if( m_plistTimeDesc == NULL )
    {
        m_plistTimeDesc = new (m_pFastAlloc) CHXVector(m_pFastAlloc);
        m_plistTimeDesc->AddRef();
    }

    plistTimeDesc = m_plistTimeDesc;
    m_plistTimeDesc->AddRef();

    return HXR_OK;
}

STDMETHODIMP
CSDP::GetTimeDescListConst( REF(IHXList*) plistTimeDesc )
{
    if( m_plistTimeDesc == NULL )
    {
        m_plistTimeDesc = new (m_pFastAlloc) CHXVector(m_pFastAlloc);
        m_plistTimeDesc->AddRef();
    }

    plistTimeDesc = m_plistTimeDesc->AsConst();

    return HXR_OK;
}

STDMETHODIMP
CSDP::GetTimeZone( REF(IHXBuffer*) pbufTimeZone )
{
    if( m_pbufTimeZone != NULL )
    {
        m_pbufTimeZone->AddRef();
    }
    pbufTimeZone = m_pbufTimeZone;

    return HXR_OK;
}

STDMETHODIMP
CSDP::SetTimeZone( IHXBuffer* pbufTimeZone )
{
    HX_RELEASE( m_pbufSDP );
    HX_RELEASE( m_pbufTimeZone );
    if( pbufTimeZone != NULL )
    {
        pbufTimeZone->AddRef();
    }
    m_pbufTimeZone = pbufTimeZone;

    return HXR_OK;
}

STDMETHODIMP
CSDP::GetEncryptKey( REF(IHXBuffer*) pbufEncryptKey )
{
    if( m_pbufEncryptKey != NULL )
    {
        m_pbufEncryptKey->AddRef();
    }
    pbufEncryptKey = m_pbufEncryptKey;

    return HXR_OK;
}

STDMETHODIMP
CSDP::SetEncryptKey( IHXBuffer* pbufEncryptKey )
{
    HX_RELEASE( m_pbufSDP );
    HX_RELEASE( m_pbufEncryptKey );
    if( pbufEncryptKey != NULL )
    {
        pbufEncryptKey->AddRef();
    }
    m_pbufEncryptKey = pbufEncryptKey;

    return HXR_OK;
}

STDMETHODIMP
CSDP::GetAttribList( REF(IHXList*) plistAttrib )
{
    HX_RELEASE( m_pbufSDP );
    if( m_plistAttrib == NULL )
    {
        m_plistAttrib = new (m_pFastAlloc) CHXVector(m_pFastAlloc);
        m_plistAttrib->AddRef();
    }

    plistAttrib = m_plistAttrib;
    m_plistAttrib->AddRef();

    return HXR_OK;
}

STDMETHODIMP
CSDP::GetAttribListConst( REF(IHXList*) plistAttrib )
{
    if( m_plistAttrib == NULL )
    {
        m_plistAttrib = new (m_pFastAlloc) CHXVector(m_pFastAlloc);
        m_plistAttrib->AddRef();
    }

    plistAttrib = m_plistAttrib->AsConst();

    return HXR_OK;
}

STDMETHODIMP
CSDP::GetMediaList( REF(IHXList*) plistMedia )
{
    HX_RELEASE( m_pbufSDP );
    if( m_plistMedia == NULL )
    {
        m_plistMedia = new (m_pFastAlloc) CHXVector(m_pFastAlloc);
        m_plistMedia->AddRef();
    }

    plistMedia = m_plistMedia;
    m_plistMedia->AddRef();

    return HXR_OK;
}

STDMETHODIMP
CSDP::GetMediaListConst( REF(IHXList*) plistMedia )
{
    if( m_plistMedia == NULL )
    {
        m_plistMedia = new (m_pFastAlloc) CHXVector(m_pFastAlloc);
        m_plistMedia->AddRef();
    }

    plistMedia = m_plistMedia->AsConst();

    return HXR_OK;
}
