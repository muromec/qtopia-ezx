/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: expatprs.cpp,v 1.11 2005/06/24 16:52:01 albertofloyd Exp $
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
//  $Id: expatprs.cpp,v 1.11 2005/06/24 16:52:01 albertofloyd Exp $

#include "hlxclib/string.h"
#include "hxtypes.h"
#include "hxresult.h"

#include "hxcom.h"
#include "hxcomm.h"
#include "hxfiles.h"
#include "ihxpckts.h"
#include "hxxml.h"
#include "hxplugn.h"
#include "xmlreslt.h"

#include "hxassert.h"
#include "hxheap.h"
#include "hxxres.h"
#include "hxxrsmg.h"

#include "hxxml.ver"
#include "xmlparse.h"
#include "expatprs.h"
#include "expatapi.h"
#include "ctype.h"

#include "hxperf.h"

#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif

#define MAX_ATTRIBUTE_SIZE 255


HXExpatXMLParser::HXExpatXMLParser(IUnknown* pContext)
: m_lRefCount(0)
, m_pResponse(NULL)
, m_pContext(pContext)
, m_pClassFactory(NULL)
, m_pParser(NULL)
, m_pCurrentBuffer(NULL)
, m_ulCurrentOffset(0)
, m_pNSResp(NULL)
, m_bInited(FALSE)
{
    m_pContext->AddRef();
    m_pContext->QueryInterface(IID_IHXCommonClassFactory, 
	(void**)&m_pClassFactory);
}

HXExpatXMLParser::~HXExpatXMLParser()
{
    Close();
    HX_RELEASE(m_pContext);
    HX_RELEASE(m_pClassFactory);
}

/************************************************************************
 *  IUnknown COM Interface Methods                          ref:  hxcom.h
 */
STDMETHODIMP 
HXExpatXMLParser::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
	AddRef();
	*ppvObj = this;
	return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXXMLParser))
    {
	AddRef();
	*ppvObj = (IHXXMLParser*)this;
	return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXXMLNamespaceParser))
    {
	AddRef();
	*ppvObj = (IHXXMLNamespaceParser*)this;
	return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(UINT32)
HXExpatXMLParser::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(UINT32)
HXExpatXMLParser::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
	return m_lRefCount;
    }
    delete this;
    return 0;
}

/*
 * IHXXMLNamespaceParser methods
 */
STDMETHODIMP
HXExpatXMLParser::InitNamespaceParser(
		       IHXXMLParserResponse* /*IN*/  pResponse,
		       IHXXMLNamespaceResponse*      pNSResp,
		       const char*	    /*IN*/    pEncoding,
		       const char		      sepChar)
{
    HX_LOG_BLOCK( "HXExpatXMLParser::InitNamespaceParser" );

    if (m_bInited)
    {
	return HXR_UNEXPECTED;
    }
    HX_RESULT rc = HXR_OK;
    if (pResponse)
    {
	m_pResponse = pResponse;
	m_pResponse->AddRef();
    }

    if (pNSResp)
    {
	m_pNSResp = pNSResp;
	m_pNSResp->AddRef();
    }

    m_cSepChar = sepChar;
    if (pEncoding)
    {
	m_pParser = XML_ParserCreateNS(pEncoding, sepChar);
    }
    else
    {
	/* use iso-8859-1 by default */
	m_pParser = XML_ParserCreateNS("iso-8859-1", sepChar);
    }

    if (m_pParser == NULL)
    {
	rc = HXR_OUTOFMEMORY;
    }
    if (SUCCEEDED(rc))
    {
	XML_SetUserData(m_pParser, this);
	XML_SetElementHandler(m_pParser, ::handleStartElement, 
	    ::handleEndElement);
	XML_SetCharacterDataHandler(m_pParser, ::handleCharacterData);
	XML_SetProcessingInstructionHandler(m_pParser, 
	    ::handleProcessingInstruction);
	XML_SetCommentHandler(m_pParser, ::handleComment);
    
	//XML_SetCdataSectionHandler(m_pParser, ::handleStartCdataSection,  
	//    ::handleEndCdataSection);
    
	XML_SetCharacterDataHandler(m_pParser, ::handleCharacterData);
	XML_SetUnparsedEntityDeclHandler(m_pParser, ::handleUnparsedEntityDecl);
	XML_SetNotationDeclHandler(m_pParser, ::handleNotationDecl);
	XML_SetDefaultHandler(m_pParser, ::handleDefault);

	XML_SetNamespaceDeclHandler(m_pParser, ::handleStartNamespaceDecl,
		::handleEndNamespaceDecl);
    }

    m_bInited = TRUE;
    return rc;
}

STDMETHODIMP_(char)
HXExpatXMLParser::GetSepChar()
{
    return m_cSepChar;
}


/*
 * IHXXMLParser methods
 */
STDMETHODIMP
HXExpatXMLParser::Init(IHXXMLParserResponse* /*IN*/  pResponse,
		       const char*	    /*IN*/	pEncoding,
		       HXBOOL		    /*IN*/	bStrict)
{
    HX_LOG_BLOCK( "HXExpatXMLParser::Init" );

    if (m_bInited)
    {
	return HXR_UNEXPECTED;
    }
    HX_RESULT rc = HXR_OK;
    if (pResponse)
    {
	m_pResponse = pResponse;
	m_pResponse->AddRef();
    }

    if (pEncoding)
    {
	m_pParser = XML_ParserCreate(pEncoding);
    }
    else
    {
	/* use iso-8859-1 by default */
	m_pParser = XML_ParserCreate("iso-8859-1");
    }

    if (m_pParser == NULL)
    {
	rc = HXR_OUTOFMEMORY;
    }
    if (SUCCEEDED(rc))
    {
	XML_SetUserData(m_pParser, this);
	XML_SetElementHandler(m_pParser, ::handleStartElement, 
	    ::handleEndElement);
	XML_SetCharacterDataHandler(m_pParser, ::handleCharacterData);
	XML_SetProcessingInstructionHandler(m_pParser, 
	    ::handleProcessingInstruction);
	XML_SetCommentHandler(m_pParser, ::handleComment);
    
	//XML_SetCdataSectionHandler(m_pParser, ::handleStartCdataSection,  
	//    ::handleEndCdataSection);
    
	XML_SetCharacterDataHandler(m_pParser, ::handleCharacterData);
	XML_SetUnparsedEntityDeclHandler(m_pParser, ::handleUnparsedEntityDecl);
	XML_SetNotationDeclHandler(m_pParser, ::handleNotationDecl);
	XML_SetDefaultHandler(m_pParser, ::handleDefault);
    }
    m_bInited = TRUE;
    return rc;
}

STDMETHODIMP
HXExpatXMLParser::Close()
{
    HX_RELEASE(m_pResponse);
    HX_RELEASE(m_pNSResp);
    HX_RELEASE(m_pCurrentBuffer);
    m_ulCurrentOffset = 0;
    m_bInited = FALSE;
    if (m_pParser)
    {
	XML_ParserFree(m_pParser);
	m_pParser = NULL;
    }
    return HXR_OK;
}


STDMETHODIMP
HXExpatXMLParser::Parse(IHXBuffer*	/*IN*/	    pBuffer,
			 HXBOOL		/*IN*/	    bIsFinal)
{
    HX_LOG_BLOCK( "HXExpatXMLParser::Parse" );

    if (m_pParser)
    {
	if (m_pCurrentBuffer)
	{
	    m_ulCurrentOffset += m_pCurrentBuffer->GetSize();
	    HX_RELEASE(m_pCurrentBuffer);
	}
	m_pCurrentBuffer = pBuffer;
	m_pCurrentBuffer->AddRef();
	if (!XML_Parse(m_pParser, (const XML_Char*)m_pCurrentBuffer->GetBuffer(), 
		m_pCurrentBuffer->GetSize(), bIsFinal))
	{
	    XML_Error code = XML_GetErrorCode(m_pParser);
	    switch (code)
	    {
	    case XML_ERROR_NONE:
		return HXR_FAIL;
	    case XML_ERROR_NO_MEMORY:
		return HXR_OUTOFMEMORY;
	    case XML_ERROR_SYNTAX:
		return HXR_XML_SYNTAX;
	    case XML_ERROR_NO_ELEMENTS:
		return HXR_XML_NO_ELEMENTS;
// we removed this error in favor of more specific errors
//	    case XML_ERROR_INVALID_TOKEN:
//		return HXR_XML_INVALID_TOKEN;
	    case XML_ERROR_UNCLOSED_TOKEN:
		return HXR_XML_UNCLOSED_TOKEN;
	    case XML_ERROR_PARTIAL_CHAR:
		return HXR_XML_PARTIAL_CHAR;
	    case XML_ERROR_TAG_MISMATCH:
		return HXR_XML_TAG_MISMATCH;
	    case XML_ERROR_DUPLICATE_ATTRIBUTE:
		// HXR_XML_DUPLICATE_ATTRIBUTE;
		return HXR_XML_DUPATTRIBUTE;
	    case XML_ERROR_JUNK_AFTER_DOC_ELEMENT:
		return HXR_XML_JUNK_AFTER_DOC_ELEMENT;
	    case XML_ERROR_PARAM_ENTITY_REF:
		return HXR_XML_PARAM_ENTITY_REF;
	    case XML_ERROR_UNDEFINED_ENTITY:
		return HXR_XML_UNDEFINED_ENTITY;
	    case XML_ERROR_RECURSIVE_ENTITY_REF:
		return HXR_XML_RECURSIVE_ENTITY_REF;
	    case XML_ERROR_ASYNC_ENTITY:
		return HXR_XML_ASYNC_ENTITY;
	    case XML_ERROR_BAD_CHAR_REF:
		return HXR_XML_BAD_CHAR_REF;
	    case XML_ERROR_BINARY_ENTITY_REF:
		return HXR_XML_BINARY_ENTITY_REF;
	    case XML_ERROR_ATTRIBUTE_EXTERNAL_ENTITY_REF:
		return PNR_XML_ATTRIBUTE_EXTEHXAL_ENTITY_REF;
	    case XML_ERROR_MISPLACED_XML_PI:
		return HXR_XML_MISPLACED_XML_PI;
	    case XML_ERROR_UNKNOWN_ENCODING:
		return HXR_XML_UNKNOWN_ENCODING;
	    case XML_ERROR_INCORRECT_ENCODING:
		return HXR_XML_INCORRECT_ENCODING;
	    case XML_ERROR_UNCLOSED_CDATA_SECTION:
		return HXR_XML_UNCLOSED_CDATA_SECTION;
	    case XML_ERROR_EXTERNAL_ENTITY_HANDLING:
		return PNR_XML_EXTEHXAL_ENTITY_HANDLING;
	    case XML_ERROR_NOT_STANDALONE:
		return HXR_XML_NOT_STANDALONE;
            case XML_ERROR_INVALID_NAME:
		return HXR_XML_INVALID_NAME;
            case XML_ERROR_INVALID_CHAR_IN_DOC:
		return HXR_XML_INVALID_CHAR_IN_DOC;
            case XML_ERROR_TWO_DASHES_NOT_ALLOWED_IN_COMMENT:
		return HXR_XML_TWO_DASHES_NOT_ALLOWED_IN_COMMENT;
            case XML_ERROR_INVALID_DECL:
		return HXR_XML_INVALID_DECL;
            case XML_ERROR_INVALID_PI: 
		return HXR_XML_INVALID_PI;
            case XML_ERROR_INVALID_PI_TARGET: 
		return HXR_XML_INVALID_PI_TARGET;
            case XML_ERROR_INVALID_CDATA: 
		return HXR_XML_INVALID_CDATA;
            case XML_ERROR_NO_CLOSING_GT: 
		return HXR_XML_NO_CLOSING_GT;
            case XML_ERROR_INVALID_HEX_CHAR_REF: 
		return HXR_XML_INVALID_HEX_CHAR_REF;
            case XML_ERROR_INVALID_CHAR_REF: 
		return HXR_XML_INVALID_CHAR_REF;
            case XML_ERROR_INVALID_REF: 
		return HXR_XML_INVALID_REF;
            case XML_ERROR_MISSING_EQUALS: 
		return HXR_XML_MISSING_EQUALS;
            case XML_ERROR_MISSING_QUOT_APOS: 
		// HXR_XML_MISSING_QUOT_APOS;
		return HXR_XML_MISSINGQUOTE;
            case XML_ERROR_MISSING_REQ_SPACE: 
		return HXR_XML_MISSING_REQ_SPACE;
            case XML_ERROR_LT_NOT_ALLOWED: 
		return HXR_XML_LT_NOT_ALLOWED;
            case XML_ERROR_EXPECTED_GT: 
		return HXR_XML_EXPECTED_GT;
            case XML_ERROR_INVALID_GT_AFFT_2_RSQB_IN_CONTENT: 
		return HXR_XML_INVALID_GT_AFFT_2_RSQB_IN_CONTENT;
            case XML_ERROR_INVALID_COMMENT: 
		return HXR_XML_INVALID_COMMENT;
	    default:
		return HXR_XML_GENERALERROR;
	    }
	}
    }
    else
    {
	return HXR_NOT_INITIALIZED;
    }
    return HXR_OK;
}

STDMETHODIMP
HXExpatXMLParser::GetCurrentLineNumber(REF(ULONG32) /*OUT*/ ulLineNumber)
{
    if ( m_pParser )
    {
	ulLineNumber = XML_GetCurrentLineNumber(m_pParser);
	return HXR_OK;
    }
    else
    {
	return HXR_NOT_INITIALIZED;
    }
}

STDMETHODIMP
HXExpatXMLParser::GetCurrentColumnNumber(REF(ULONG32) /*OUT*/ ulColumnNumber)
{
    if ( m_pParser )
    {
	ulColumnNumber = XML_GetCurrentLineNumber(m_pParser);
	return HXR_OK;
    }
    else
    {
	return HXR_NOT_INITIALIZED;
    }
}

STDMETHODIMP
HXExpatXMLParser::GetCurrentByteIndex(REF(ULONG32) /*OUT*/ ulByteIndex)
{
    if ( m_pParser )
    {
	ulByteIndex = XML_GetCurrentByteIndex(m_pParser);
	return HXR_OK;
    }
    else
    {
	return HXR_NOT_INITIALIZED;
    }
}


STDMETHODIMP
HXExpatXMLParser::GetCurrentErrorText(REF(IHXBuffer*) /*OUT*/ pBuffer)
{
    /* Returns a string describing the error. */
    UINT32 ulByteIndex = XML_GetCurrentByteIndex(m_pParser);
   
    const UCHAR* pPos = m_pCurrentBuffer->GetBuffer();
    if ( SUCCEEDED(m_pClassFactory->CreateInstance(CLSID_IHXBuffer, (void**)&pBuffer)) )
    {
	const char p[] = "No error context";
	if (m_pCurrentBuffer)
	{
	    if (ulByteIndex >= m_ulCurrentOffset && 
		ulByteIndex < m_ulCurrentOffset + m_pCurrentBuffer->GetSize())
	    {
		int i;
		int offset = ulByteIndex - m_ulCurrentOffset;
		int startPos = offset;
		// first we will back the startPos up to the begining of the 
		// line or 0
		while ( startPos > 0 && pPos[startPos-1] != '\n' && pPos[startPos-1] != '\r')
		{
			--startPos; 
		}

		// if the beginning of the line number is > 40 characters 
		// ahead, we want to jump ahead so the error text is not too long.
		if (offset - startPos >= 40)
		{
		    startPos = offset - 40;
		}

		// now we will find the end of the line.
		i=startPos;
		while (pPos[i] && pPos[i] != '\n' && pPos[i] != '\r')
		{
			++i;
		}

		if (SUCCEEDED(pBuffer->Set(&pPos[startPos], i-startPos+1)))
		{
		    pBuffer->GetBuffer()[i-startPos] = '\0';
		    return HXR_OK;
		}
		else
		{
		    return HXR_OUTOFMEMORY;
		}
	    }
	    else
	    {
		return pBuffer->Set((const UCHAR*)p, sizeof(p));
	    }
	}
	else
	{
	    return pBuffer->Set((const UCHAR*)p, sizeof(p));
	}
    }
    else
    {
	return HXR_FAIL;
    }
}


void HXExpatXMLParser::handleStartElement(const XML_Char *name, const XML_Char **atts)
{
    HX_LOG_BLOCK( "HXExpatXMLParser::handleStartElement" );
    	
    if (m_pResponse)
    {
	IHXValues* pAtt = NULL;
	{
	    HX_LOG_BLOCK( "HXExpatXMLParser::handleStartElement::createIHXValues" );
	    IUnknown* pUnkAtt = NULL;
	    m_pClassFactory->CreateInstance(CLSID_IHXValues, (void**)&pUnkAtt);
	    pUnkAtt->QueryInterface(IID_IHXValues, (void**)&pAtt);
	    HX_RELEASE(pUnkAtt);
	}
	if (*atts && **atts) 
	{
	    HX_LOG_BLOCK( "HXExpatXMLParser::handleStartElement::createIHXBuffer" );

	    do
	    {
		IHXBuffer* pBuf = NULL;
		m_pClassFactory->CreateInstance(CLSID_IHXBuffer, (void**)&pBuf);
		pBuf->Set((const UCHAR*)atts[1], strlen(atts[1])+1);
		pAtt->SetPropertyCString(atts[0], pBuf);
		HX_RELEASE(pBuf);
	    }
	    while (*(atts+=2));
	}
	
	{
	    int ulLine;
	    int ulCol;
	    {
	        HX_LOG_BLOCK( "HXExpatXMLParser::handleStartElement::XML_GetCurrentLineNumber" );
		ulLine = XML_GetCurrentLineNumber(m_pParser);
	    }
	    {
	        HX_LOG_BLOCK( "HXExpatXMLParser::handleStartElement::XML_GetCurrentColumnNumber" );
		ulCol = XML_GetCurrentColumnNumber(m_pParser);
	    }
	    m_pResponse->HandleStartElement((const char*)name, pAtt, ulLine,ulCol);
	}
	HX_RELEASE(pAtt);
    }
}

void HXExpatXMLParser::handleEndElement(const XML_Char *name)
{
    HX_LOG_BLOCK( "HXExpatXMLParser::handleEndElement" );
    	
    if (m_pResponse)
    {
	int ulLine = XML_GetCurrentLineNumber(m_pParser);
	int ulCol = XML_GetCurrentColumnNumber(m_pParser);
	m_pResponse->HandleEndElement((const char*)name, ulLine,ulCol);
    }
}

void HXExpatXMLParser::handleCharacterData(const XML_Char *s, int len)
{
    if (m_pResponse)
    {
	IHXBuffer* pBuf = NULL;
	m_pClassFactory->CreateInstance(CLSID_IHXBuffer, (void**)&pBuf);
	pBuf->Set((UCHAR*)s, len);
	int ulLine = XML_GetCurrentLineNumber(m_pParser);
	int ulCol = XML_GetCurrentColumnNumber(m_pParser);
	m_pResponse->HandleCharacterData(pBuf, ulLine,ulCol);
	HX_RELEASE(pBuf);
    }
}

void HXExpatXMLParser::handleProcessingInstruction(
				const XML_Char *target, const XML_Char *data)
{
    if (m_pResponse)
    {
	IHXValues* pAtt = NULL;
	IUnknown* pUnkAtt = NULL;
	m_pClassFactory->CreateInstance(CLSID_IHXValues, (void**)&pUnkAtt);
	pUnkAtt->QueryInterface(IID_IHXValues, (void**)&pAtt);
	HX_RELEASE(pUnkAtt);
	const char* pos = data;
	
	const char* pStartName = NULL;
	const char* pEndName = NULL;
	const char* pStartAtt = NULL;
	char cQuote = '"';
	const char* pEndAtt = NULL;

	for (;;)
	{
	    // eat white space
	    for(;*pos && isspace(*pos); ++pos){};

            // if we've reached the end, break out
            if (*pos == '\0') break;

	    // get attribute name
	    pStartName = pos;
	    while( *pos && !isspace(*pos) && *pos != '=') 
		{
			++pos; 
		}
	    
	    // eat white space
	    pEndName = pos;
	    while(*pos && isspace(*pos)) 
		{
			++pos;
		}

	    // require equals sign
	    if (*pos++ != '=')
	    	break;

	    // eat white space
	    while(*pos && isspace(*pos) ) 
		{
			++pos;
		}

	    // get attribute value
	    if (*pos == '\'' || *pos == '"')
	    {
	    	cQuote = *pos++;
	    }
	    else
	    {
		break;
	    }
	    
	    pStartAtt = pos;
	    while( *pos && *pos != cQuote ) 
		{
			++pos;
		}
	    pEndAtt = pos;

            // Advance past the quote
            ++pos;
	    // require a single whitespace or termination...
	    if (*pos != '\0' && !isspace(*pos))
	    {
	    	break;
	    }

	    char* name = new char[pEndName-pStartName+1];
	    if (!name)
	    {
		break;
	    }

	    strncpy(name, pStartName, pEndName - pStartName); /* Flawfinder: ignore */
	    name[pEndName-pStartName] = '\0';

	    IHXBuffer* pBuf = NULL;
	    m_pClassFactory->CreateInstance(CLSID_IHXBuffer, 
	    	(void**)&pBuf);
	    
	    if (!pBuf)
	    {
	    	HX_VECTOR_DELETE(name);
		break;
	    }

	    if (FAILED(pBuf->Set((UCHAR*)pStartAtt,
	    	pEndAtt-pStartAtt+1)))
	    {
	    	HX_VECTOR_DELETE(name);
		HX_RELEASE(pBuf);
		break;
	    }
	    
	    pBuf->GetBuffer()[pEndAtt-pStartAtt] = '\0';
	    
	    if (FAILED(pAtt->SetPropertyCString(name, pBuf)))
	    {
	    	HX_VECTOR_DELETE(name);
		HX_RELEASE(pBuf);
		break;
	    }
	    HX_RELEASE(pBuf);
	    HX_VECTOR_DELETE(name);
	}

	int ulLine = XML_GetCurrentLineNumber(m_pParser);
	int ulCol = XML_GetCurrentColumnNumber(m_pParser);
	m_pResponse->HandleProcessingInstruction(target, pAtt, ulLine,ulCol);
       	HX_RELEASE(pAtt);
    }
}

void HXExpatXMLParser::handleComment(const XML_Char *data)
{
    if (m_pResponse)
    {
	int ulLine = XML_GetCurrentLineNumber(m_pParser);
	int ulCol = XML_GetCurrentColumnNumber(m_pParser);
	m_pResponse->HandleComment((const char*) data, ulLine, ulCol);
    }
}

void HXExpatXMLParser::handleStartCDataSection()
{
}

void HXExpatXMLParser::handleEndCDataSection()
{
}


void HXExpatXMLParser::handleDefault(const XML_Char *s, int len)
{
    if (m_pResponse)
    {
	IHXBuffer* pBuf = NULL;
	m_pClassFactory->CreateInstance(CLSID_IHXBuffer, (void**)&pBuf);
	pBuf->Set((UCHAR*)s, len);
	
	int ulLine = XML_GetCurrentLineNumber(m_pParser);
	int ulCol = XML_GetCurrentColumnNumber(m_pParser);
	m_pResponse->HandleDefault(pBuf, ulLine, ulCol);
	HX_RELEASE (pBuf);
    }
}

void HXExpatXMLParser::handleUnparsedEntityDecl(
			    const XML_Char *entityName,
			    const XML_Char *base,
			    const XML_Char *systemId,
			    const XML_Char *publicId,
			    const XML_Char *notationName)
{
    if (m_pResponse)
    {
	// note ... call back interface missing *base
	int ulLine = XML_GetCurrentLineNumber(m_pParser);
	int ulCol = XML_GetCurrentColumnNumber(m_pParser);
	m_pResponse->HandleUnparsedEntityDecl(entityName,systemId, publicId,
			    notationName, ulLine, ulCol);
    }
}

void HXExpatXMLParser::handleNotationDecl(
		      const XML_Char *notationName,
		      const XML_Char *base,
		      const XML_Char *systemId,
		      const XML_Char *publicId)
{
    if (m_pResponse)
    {
	// note ... call back interface missing *base
	int ulLine = XML_GetCurrentLineNumber(m_pParser);
	int ulCol = XML_GetCurrentColumnNumber(m_pParser);
	m_pResponse->HandleNotationDecl(notationName, systemId, publicId,
			    ulLine, ulCol);
    }
}

void HXExpatXMLParser::handleStartNamespaceDecl(
			      const XML_Char *prefix,
			      const XML_Char *uri)
{
    if (m_pNSResp)
    {
	m_pNSResp->HandleStartNamespaceDecl(prefix, uri);
    }
}

void HXExpatXMLParser::handleEndNamespaceDecl(const XML_Char *prefix)
{
    if (m_pNSResp)
    {
	m_pNSResp->HandleEndNamespaceDecl(prefix);
    }
}


int HXExpatXMLParser::handleNotStandalone()
{
    /* This is called if the document is not standalone (it has an
     * external subset or a reference to a parameter entity, but does not
     * have standalone="yes"). If this handler returns 0, then processing
     * will not continue, and the parser will return a
     * XML_ERROR_NOT_STANDALONE error.
     */
    return 1;
}

void HXExpatXMLParser::handleStartDoctypeDecl(const XML_Char *doctypeName)
{
    // note... no DTD callbacks...
}

void HXExpatXMLParser::handleEndDoctypeDecl()
{
    // note... no DTD callbacks...
}

