/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxxmlprs.cpp,v 1.10 2006/02/07 19:21:20 ping Exp $
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

#include "hxtypes.h"

#include "hxcom.h"
#include "hxcomm.h"
#include "hxfiles.h"
#include "ihxpckts.h"
#include "hxxml.h"
#include "xmlreslt.h"

#include "hxresult.h"
#include "hxassert.h"
#include "hxheap.h"
#include "cbbqueue.h"
#include "hxbuffer.h"
#include "chxpckts.h"
#include "pckunpck.h"
#include "looseprs.h"

#include "hxxmlprs.h"

#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif

#ifndef _VXWORKS
static const UINT32 BUFSIZE = 1024;
#endif

HXXMLParser::HXXMLParser(IUnknown* pContext, 
			 HXBOOL bAllowNonXMLComments):
    m_lRefCount(0),
    m_pResponse(NULL),
    m_bAllowNonXMLComments(bAllowNonXMLComments),
    m_pParser(NULL),
    m_pContext(NULL)
{
    m_pContext = pContext;
    HX_ADDREF(m_pContext);
}

HXXMLParser::~HXXMLParser()
{
    Close();
}


/*
 * IUnknown methods
 */

STDMETHODIMP 
HXXMLParser::QueryInterface(REFIID riid, void** ppvObj)
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
    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32) 
HXXMLParser::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32) 
HXXMLParser::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}


/*
 * IHXXMLParser methods
 */

STDMETHODIMP
HXXMLParser::Init(IHXXMLParserResponse* /*IN*/  pResponse,
		   const char*	    /*IN*/	pEncoding,
		   HXBOOL		    /*IN*/	bStrict)
{
    HX_RESULT rc = HXR_OK;

    m_pResponse = pResponse;
    m_pResponse->AddRef(); 

    if(bStrict)
    {
	m_pParser = new HXStrictXMLParser(m_pContext);
    }
    else
    {
	m_pParser = new HXLooseXMLParser(m_pContext, m_bAllowNonXMLComments);
    }

    rc = m_pParser->Init(pResponse, pEncoding);

    return rc;
}

STDMETHODIMP
HXXMLParser::Close()
{
    HX_RESULT rc = HXR_OK;

    HX_RELEASE(m_pResponse);
    HX_DELETE(m_pParser);

    HX_RELEASE(m_pContext);
    return rc;
}


STDMETHODIMP
HXXMLParser::Parse(IHXBuffer*	/*IN*/	    pBuffer,
		  HXBOOL		/*IN*/	    bIsFinal)
{
    HX_RESULT rc = HXR_OK;

    rc = m_pParser->Parse(pBuffer, bIsFinal);    

    return rc;
}

STDMETHODIMP
HXXMLParser::GetCurrentLineNumber(REF(ULONG32) /*OUT*/ ulLineNumber)
{
    HX_RESULT rc = HXR_OK;

    rc = m_pParser->GetCurrentLineNumber(ulLineNumber);

    return rc;
}

STDMETHODIMP
HXXMLParser::GetCurrentColumnNumber(REF(ULONG32) /*OUT*/ ulColumnNumber)
{
    HX_RESULT rc = HXR_OK;

    rc = m_pParser->GetCurrentColumnNumber(ulColumnNumber);

    return rc;
}

STDMETHODIMP
HXXMLParser::GetCurrentByteIndex(REF(ULONG32) /*OUT*/ ulByteIndex)
{
    HX_RESULT rc = HXR_OK;

    rc = m_pParser->GetCurrentByteIndex(ulByteIndex);

    return rc;
}

STDMETHODIMP
HXXMLParser::GetCurrentErrorText(REF(IHXBuffer*) /*OUT*/ pBuffer)
{
    HX_RESULT rc = HXR_OK;

    rc = m_pParser->GetCurrentErrorText(pBuffer);

    return rc;
}

HX_RESULT
HXXMLParser::InitErrorNotifier(ErrorNotifier* /*OUT*/ pNotifier)
{
    return m_pParser->InitErrorNotifier(pNotifier);
}


/*
 * virtual base class methods
 */

HXActualXMLParser::HXActualXMLParser(IUnknown* pContext):
    m_pContext(NULL),
    m_pResponse(NULL)
{
    m_pContext = pContext;
    HX_ADDREF(m_pContext);
}

HXActualXMLParser::~HXActualXMLParser()
{
    HX_RELEASE(m_pContext);
}

void HXActualXMLParser::CheckEncoding(XMLParser* pParser, IHXBuffer* pBuffer)
{
    if (pParser && pBuffer)
    {
        // Check if this buffer has any prolog info
        char* pszVersion  = NULL;
        char* pszEncoding = NULL;
        HX_RESULT rv = pParser->GetPrologInfo((const char*) pBuffer->GetBuffer(),
                                              pBuffer->GetSize(),
                                              pszVersion,
                                              pszEncoding);
        if (SUCCEEDED(rv) && pszEncoding && strlen(pszEncoding) > 0)
        {
            // Get the encoding from the parser
            char* pszParserEncoding = NULL;
            HX_RESULT rv = pParser->GetEncoding(pszParserEncoding);
            if (SUCCEEDED(rv) &&
                strcmp(pszEncoding, pszParserEncoding) != 0)
            {
                pParser->SetEncoding(pszEncoding);
            }
            HX_VECTOR_DELETE(pszParserEncoding);
        }
        HX_VECTOR_DELETE(pszVersion);
        HX_VECTOR_DELETE(pszEncoding);
    }
}

/* 
 * HXLooseXMLParser methods
 */

HXLooseXMLParser::HXLooseXMLParser(IUnknown* pContext, HXBOOL bAllowNonXMLComments):
    HXActualXMLParser(pContext),
    m_pParser(NULL),
    m_pByteQueue(NULL),
    m_bAllowNonXMLComments(bAllowNonXMLComments)
{
}

HXLooseXMLParser::~HXLooseXMLParser()
{
    HX_DELETE(m_pParser);
    HX_DELETE(m_pByteQueue);
}

HX_RESULT
HXLooseXMLParser::Init(IHXXMLParserResponse* pResponse,
		       const char* pEncoding)
{
    m_pResponse = pResponse;
    m_pParser = new XMLParser(FALSE, pEncoding, m_bAllowNonXMLComments);
    m_pByteQueue = new CBigByteQueue(BUFSIZE);

    return HXR_OK;
}

HX_RESULT
HXLooseXMLParser::Parse(IHXBuffer* pBuffer,
			HXBOOL bIsFinal)
{
    HX_RESULT rc = HXR_OK;

    // Check the encoding
    CheckEncoding(m_pParser, pBuffer);

    UINT32 ulBufLen = pBuffer->GetSize();

    if(m_pByteQueue->GetAvailableElements() < ulBufLen)
    {
	m_pByteQueue->Grow(ulBufLen);
    }
    m_pByteQueue->EnQueue(pBuffer->GetBuffer(), ulBufLen);
    rc = DoParse(bIsFinal);

    return rc;
}

HX_RESULT
HXLooseXMLParser::GetCurrentLineNumber(REF(ULONG32) ulLineNumber)
{
    HX_RESULT rc = HXR_OK;

    ulLineNumber = (UINT32)m_pParser->GetCurrentLineNumber();

    return rc;
}

HX_RESULT
HXLooseXMLParser::GetCurrentColumnNumber(REF(ULONG32) ulColumnNumber)
{
    HX_RESULT rc = HXR_OK;

    ulColumnNumber = (UINT32)m_pParser->GetCurrentColumnNumber();

    return rc;
}

HX_RESULT
HXLooseXMLParser::GetCurrentByteIndex(REF(ULONG32) ulByteIndex)
{
    HX_RESULT rc = HXR_OK;

    return rc;
}

HX_RESULT
HXLooseXMLParser::GetCurrentErrorText(REF(IHXBuffer*) pBuffer)
{
    HX_RESULT rc = HXR_FAIL;

    XMLError* pLastError = m_pParser->GetLastError();
    if(pLastError &&
	pLastError->m_pErrorString)
    {
	rc = CreateAndSetBufferCCF(pBuffer, (BYTE*)pLastError->m_pErrorString,
				   strlen(pLastError->m_pErrorString) + 1, m_pContext);
    }

    return rc;
}

HX_RESULT
HXLooseXMLParser::DoParse(HXBOOL bIsFinal)
{
    HX_RESULT rc = HXR_OK;

    XMLTag* pTag = NULL;

    HXBOOL bDone = FALSE;
    while(!bDone && SUCCEEDED(rc))
    {
	UINT32 bytesUsed;
	UINT32 bytesAvail = m_pByteQueue->GetQueuedItemCount();
	if(bytesAvail <= 0)
	{
	    break;
	}

	BYTE* pBuf = new BYTE[bytesAvail];
	BYTE* p = pBuf;
	m_pByteQueue->DeQueue(pBuf, bytesAvail);
	bytesUsed = bytesAvail;

	if(pTag)
	{
	    delete pTag;
	    pTag = 0;
	}

	XMLParseResult pResult = 
	    m_pParser->Parse((const char*&)p, bytesAvail, pTag, bIsFinal);
	m_pByteQueue->EnQueue(p, (UINT32)(bytesAvail - (p - pBuf)));

	// pBuf is not needed anymore, so delete
	HX_VECTOR_DELETE(pBuf);
	p = NULL;

	switch(pResult)
	{
	    case XMLPNotDone:
	    {
		goto exit;
	    }

	    case XMLPNoClose:
	    {
		rc = HXR_XML_NOCLOSE;
		goto exit;
	    }

	    case XMLPBadAttribute:
	    {
		rc = HXR_XML_NOTAGTYPE;
		goto exit;
	    }

	    case XMLPBadEndTag:
	    {
		rc = HXR_XML_BADENDTAG;
		goto exit;
	    }
	    
	    case XMLPAttributeValueNotQuoted:
	    {
		rc = HXR_XML_MISSINGQUOTE;
		goto exit;
	    }

	    case XMLPBadDirective:
	    {
		rc = HXR_XML_GENERALERROR;
		goto exit;
	    }

	    case XMLPDupAttribute:
	    {
		rc = HXR_XML_DUPATTRIBUTE;
		goto exit;
	    }

	    case XMLPComment:
	    {
		const char* pValue = pTag->m_cur_attribute->value;
		rc = m_pResponse->HandleComment(pValue,0,0);
	    }
	    break;

	    case XMLPPlainText:
	    {
		const char* pValue = pTag->m_cur_attribute->value;
		IHXBuffer* pBuffer = NULL;
		if (HXR_OK == CreateAndSetBufferCCF(pBuffer, (BYTE*)pValue,
						    strlen(pValue)+1, m_pContext))
		{
		    rc = m_pResponse->HandleCharacterData(pBuffer,0,0);
		    HX_RELEASE(pBuffer);
		}
	    }
	    break;

	    case XMLPDirective:
	    {
		const char* pValue = pTag->m_name;
		if(strcmp(pValue, "DOCTYPE") == 0)
		{
		    // m_bExternalDoctype = TRUE;	
		}
	    }
	    break;

	    case XMLPProcInst:
	    // handle processing instruction
	    {
		const char* pTarget = pTag->m_name;

		IHXValues* pValues = NULL;
		CreateValuesCCF(pValues, m_pContext);

		for(UINT32 i=0;i<pTag->m_numAttributes;++i)
		{
		    XMLAttribute* pAttr = pTag->attribute(i);
		    // the XMLParser class supports what it calls attributes
		    // without names.  It's used to communicate processing
		    // instructions and things like that. We just ignore attributes
		    // without names here.
		    if (pAttr->name != NULL)
		    {
			IHXBuffer* pBuf = NULL;
			if (HXR_OK == CreateAndSetBufferCCF(pBuf, (BYTE*)pAttr->value,
							    strlen(pAttr->value)+1, m_pContext))
			{
			    pValues->SetPropertyCString(pAttr->name, pBuf);
			    HX_RELEASE(pBuf);
			}
		    }
		}
		rc = m_pResponse->HandleProcessingInstruction(pTarget,
							      pValues,0,0);
		HX_RELEASE(pValues);
	    }
	    break;

	    case XMLPTag:
	    {
		const char* pName = pTag->m_name;
		if(pTag->m_type != XMLEndTag)
		{
		    IHXValues* pValues = NULL;
		    CreateValuesCCF(pValues, m_pContext);

		    for(UINT32 i=0;i<pTag->m_numAttributes;++i)
		    {
			XMLAttribute* pAttr = pTag->attribute(i);
			// the XMLParser class supports what it calls attributes
			// without names.  It's used to communicate processing
			// instructions and things like that. We just ignore attributes
			// without names here.
			if (pAttr->name != NULL)
			{
			    IHXBuffer* pBuf = NULL;
			    if (HXR_OK == CreateAndSetBufferCCF(pBuf, (BYTE*)pAttr->value,
								strlen(pAttr->value)+1, m_pContext))
			    {
				pValues->SetPropertyCString(pAttr->name, pBuf);
				HX_RELEASE(pBuf);
			    }
			}
		    }
		    rc = m_pResponse->HandleStartElement(pName, pValues,
                                                         m_pParser->GetTagStartLineNumber(),
                                                         m_pParser->GetTagStartColumnNumber());
		    HX_RELEASE(pValues);

		    if(!pTag->m_need_close && SUCCEEDED(rc))
		    {
			rc = m_pResponse->HandleEndElement(pName,0,0);
		    }
		}
		else
		{
		    rc = m_pResponse->HandleEndElement(pName,0,0);
		}
	    }
	    break;

	    default:
	    {
		bDone = TRUE;
	    }
	    break;
	}
    }

exit:
    HX_DELETE(pTag);

    return rc;
}


/* 
 * HXStrictXMLParser methods
 */

HXStrictXMLParser::HXStrictXMLParser(IUnknown* pContext):
    HXActualXMLParser(pContext),
    m_pParser(NULL),
    m_pByteQueue(NULL)
    , m_pErrorNotifier(NULL)
{
}

HXStrictXMLParser::~HXStrictXMLParser()
{
    HX_DELETE(m_pParser);
    HX_DELETE(m_pByteQueue);
}

HX_RESULT
HXStrictXMLParser::Init(IHXXMLParserResponse* pResponse,
		       const char* pEncoding)
{
    m_pResponse = pResponse;

    m_pResponse = pResponse;
    m_pParser = new XMLParser(TRUE, pEncoding);
    if (m_pErrorNotifier)
    {
	m_pParser->StoreErrors();
    }
    m_pByteQueue = new CBigByteQueue(BUFSIZE);

    return HXR_OK;
}

HX_RESULT
HXStrictXMLParser::InitErrorNotifier(ErrorNotifier* /*OUT*/ pNotifier)
{
    m_pErrorNotifier = pNotifier; 
    if (m_pParser)
    {
	m_pParser->StoreErrors();
    }
    return HXR_OK; 
}

HX_RESULT
HXStrictXMLParser::Parse(IHXBuffer* pBuffer,
			HXBOOL bIsFinal)
{
    HX_RESULT rc = HXR_OK;

    // See if we have any encoding info
    CheckEncoding(m_pParser, pBuffer);

    UINT32 ulBufLen = pBuffer->GetSize();
    if(m_pByteQueue->GetAvailableElements() < ulBufLen)
    {
	m_pByteQueue->Grow(ulBufLen);
    }
    m_pByteQueue->EnQueue(pBuffer->GetBuffer(), ulBufLen);
    rc = DoParse(bIsFinal);

    return rc;
}

HX_RESULT
HXStrictXMLParser::HandleErrors(CHXPtrArray* pErrs)
{
    if (m_pErrorNotifier && pErrs)
    {
	int size = pErrs->GetSize();

	for(int i=0;i<size;++i)
	{
	    XMLError* pError = (XMLError*) (*pErrs)[i];
	    HX_RESULT code = ConvertToHX_RESULT(pError->m_errorTag);
	    m_pErrorNotifier->ErrorInLastTag(code, pError->m_pErrorString, 
		pError->m_pFrameString,  pError->m_lLineNumber, 
		pError->m_lLinePosition);
	}
    }
    return HXR_OK;
}

HX_RESULT
HXStrictXMLParser::ConvertToHX_RESULT(UINT32 err)
{
    HX_RESULT ret = HXR_OK;
    switch (err)
    {
    case XMLUnknownError:
	ret = HXR_XML_GENERALERROR;
	break;
    case XMLErrorNoClose:
	ret = HXR_XML_NOCLOSE;
	break;
    case XMLErrorBadAttribute:
	ret = HXR_XML_BADATTRIBUTE;
	break;
    case XMLErrorNoValue:
	ret = HXR_XML_NOVALUE;
	break;
    case XMLErrorMissingQuote:
	ret = HXR_XML_MISSINGQUOTE;
	break;
    case XMLErrorBadEndTag:
	ret = HXR_XML_BADENDTAG;
	break;
    case XMLErrorNoTagType:
	ret = HXR_XML_NOTAGTYPE;
	break;
    case XMLErrorDupAttribute:
	ret = HXR_XML_DUPATTRIBUTE;
	break;
    case XMLErrorCommentBeforeProcInst:
	ret = HXR_XML_COMMENT_B4_PROCINST;
	break;
    case XMLErrorInvalidName:
	ret = HXR_XML_INVALID_NAME;
	break;
    case XMLErrorInvalidCharInDoc:
	ret = HXR_XML_INVALID_CHAR_IN_DOC;
	break;
    case XMLErrorTwoDashNotAllowed:
	ret = HXR_XML_TWO_DASHES_NOT_ALLOWED_IN_COMMENT;
	break;
    case XMLErrorInvalidDecl:
	ret = HXR_XML_INVALID_DECL;
	break;
    case XMLErrorInvalidPI:
	ret = HXR_XML_INVALID_PI;
	break;
    case XMLErrorInvalidPITarget:
	ret = HXR_XML_INVALID_PI_TARGET;
	break;
    case XMLErrorInvalidCDATA:
	ret = HXR_XML_INVALID_CDATA;
	break;
    case XMLErrorInvalidRef:
	ret = HXR_XML_INVALID_REF;
	break;
    case XMLErrorMissingEquals:
	ret = HXR_XML_MISSING_EQUALS;
	break;
    case XMLErrorMissingReqSpace:
	ret = HXR_XML_MISSING_REQ_SPACE;
	break;
    case XMLErrorLTnotAllowed:
	ret = HXR_XML_LT_NOT_ALLOWED;
	break;
    case XMLErrorInvalidGTafter2RSQB:
	ret = HXR_XML_INVALID_GT_AFFT_2_RSQB_IN_CONTENT;
	break;
    case XMLErrorInvalidComment:
	ret = HXR_XML_INVALID_COMMENT;
	break;
    }
    return ret;
}


HX_RESULT
HXStrictXMLParser::DoParse(HXBOOL bIsFinal)
{
    HX_RESULT rc = HXR_OK;

    XMLTag* pTag = NULL;

    HXBOOL bDone = FALSE;
    while(!bDone &&
	HXR_OK == rc)
    {
	UINT32 bytesUsed;
	UINT32 bytesAvail = m_pByteQueue->GetQueuedItemCount();
	if(bytesAvail <= 0)
	{
	    break;
	}

	BYTE* pBuf = new BYTE[bytesAvail];
	BYTE* p = pBuf;
	m_pByteQueue->DeQueue(pBuf, bytesAvail);
	bytesUsed = bytesAvail;

	if(pTag)
	{
	    delete pTag;
	    pTag = 0;
	}

	XMLParseResult pResult = 
	    m_pParser->Parse((const char*&)p, bytesAvail, pTag, bIsFinal);
	m_pByteQueue->EnQueue(p, (UINT32)(bytesAvail - (p - pBuf)));

	// pBuf is not used anymore, so delete
	HX_VECTOR_DELETE(pBuf);
	p = NULL;

	UINT32 ulTagStartLine = m_pParser->GetTagStartLineNumber();
	UINT32 ulTagStartCol = m_pParser->GetTagStartColumnNumber();

	switch(pResult)
	{
	    case XMLPNotDone:
	    {
		goto exit;
	    }

	    case XMLPNoClose:
	    {
		rc = HXR_XML_NOCLOSE;
		goto exit;
	    }

	    case XMLPBadAttribute:
	    {
		rc = HXR_XML_NOTAGTYPE;
		goto exit;
	    }

	    case XMLPBadEndTag:
	    {
		rc = HXR_XML_BADENDTAG;
		goto exit;
	    }
	    
	    case XMLPAttributeValueNotQuoted:
	    {
		rc = HXR_XML_MISSINGQUOTE;
		goto exit;
	    }

	    case XMLPBadDirective:
	    {
		rc = HXR_XML_GENERALERROR;
		goto exit;
	    }

	    case XMLPDupAttribute:
	    {
		rc = HXR_XML_DUPATTRIBUTE;
		goto exit;
	    }

	    case XMLPCommentBeforeProcInst:
	    {
		rc = HXR_XML_COMMENT_B4_PROCINST;
		goto exit;
	    }
	    
	    case XMLPComment:
	    {
		const char* pValue = pTag->m_cur_attribute->value;
		rc = m_pResponse->HandleComment(pValue,ulTagStartLine,ulTagStartCol);
		HandleErrors(pTag->m_errs);
	    }
	    break;

	    case XMLPPlainText:
	    {
		const char* pValue = pTag->m_cur_attribute->value;
		IHXBuffer* pBuffer = NULL;
		if (HXR_OK == CreateAndSetBufferCCF(pBuffer, (BYTE*)pValue,
						    strlen(pValue)+1, m_pContext))
		{
		    rc = m_pResponse->HandleCharacterData(pBuffer,ulTagStartLine,ulTagStartCol);
		    HandleErrors(pTag->m_errs);
		    HX_RELEASE(pBuffer);
		}
	    }
	    break;

	    case XMLPDirective:
	    {
		const char* pName = NULL;
		const char* pPublicID = NULL;
		const char* pSystemID = NULL;
		CHXBuffer* pBuffer = NULL;
		UINT32 ulNumAttributes = pTag->m_numAttributes; 
		if(strcmp(pTag->m_name, "DOCTYPE") == 0 &&
		    ulNumAttributes > 0)
		{
		    UINT32 ulCurAttribute = 0;

		    while(ulCurAttribute < ulNumAttributes)
		    {
			XMLAttribute* pAttr = NULL;
			if(ulCurAttribute == 0)
			{
			    pAttr = pTag->attribute(ulCurAttribute);
			    if(pAttr)
			    {
				pName = pAttr->value;
			    }
			}
			else
			{
			    pAttr = pTag->attribute(ulCurAttribute);
			    if(strcmp(pAttr->value, "SYSTEM") == 0)
			    {
				ulCurAttribute++;
				if(ulCurAttribute < ulNumAttributes)
				{
				    pAttr = pTag->attribute(ulCurAttribute);
				    if(pAttr)
				    {
					pSystemID = pAttr->value;
				    }
				}
			    }
			    else if(strcmp(pAttr->value, "PUBLIC") == 0)
			    {
				ulCurAttribute++;
				if(ulCurAttribute < ulNumAttributes)
				{
				    pAttr = pTag->attribute(ulCurAttribute);
				    if(pAttr)
				    {
					pPublicID = pAttr->value;
					ulCurAttribute++;
					if(ulCurAttribute < ulNumAttributes)
					{
					    pAttr = pTag->attribute(ulCurAttribute);
					    if(pAttr)
					    {
						pSystemID = pAttr->value;
					    }
					}
				    }
				}
			    }
			}
			ulCurAttribute++;
		    }

		    rc = m_pResponse->HandleUnparsedDoctypeDecl(pName, pSystemID,
						pPublicID, ulTagStartLine,ulTagStartCol);
		    HX_RELEASE(pBuffer);
		}
	    }
	    break;

	    case XMLPProcInst:
	    // handle processing instruction
	    {
		const char* pTarget = pTag->m_name;

		IHXValues* pValues = NULL;
		IHXValues3* pValues3 = NULL;

		if (HXR_OK == CreateValuesCCF(pValues, m_pContext))
		{
		    if (HXR_OK == pValues->QueryInterface(IID_IHXValues3, (void**)&pValues3))
		    {
			pValues3->PreserveCase(TRUE);
			HX_RELEASE(pValues3);
		    }
		}

		for(UINT32 i=0;i<pTag->m_numAttributes;++i)
		{
		    XMLAttribute* pAttr = pTag->attribute(i);
		    // the XMLParser class supports what it calls attributes
		    // without names.  It's used to communicate processing
		    // instructions and things like that. We just ignore attributes
		    // without names here.
		    if (pAttr->name != NULL)
		    {
			IHXBuffer* pBuf = NULL;
			if (HXR_OK == CreateAndSetBufferCCF(pBuf, (BYTE*)pAttr->value,
							    strlen(pAttr->value)+1, m_pContext))
			{
			    pValues->SetPropertyCString(pAttr->name, pBuf);
			    HX_RELEASE(pBuf);
			}
		    }
		}
		rc = m_pResponse->HandleProcessingInstruction(pTarget,
							      pValues,ulTagStartLine,ulTagStartCol);

		HandleErrors(pTag->m_errs);

		HX_RELEASE(pValues);
	    }
	    break;

	    case XMLPTag:
	    {
		const char* pName = pTag->m_name;
		if(pTag->m_type != XMLEndTag)
		{
		    IHXValues* pValues = NULL;
		    IHXValues3* pValues3 = NULL;

		    if (HXR_OK == CreateValuesCCF(pValues, m_pContext))
		    {
			if (HXR_OK == pValues->QueryInterface(IID_IHXValues3, (void**)&pValues3))
			{
			    pValues3->PreserveCase(TRUE);
			    HX_RELEASE(pValues3);
			}
		    }

		    for(UINT32 i=0;i<pTag->m_numAttributes;++i)
		    {
			XMLAttribute* pAttr = pTag->attribute(i);
			// the XMLParser class supports what it calls attributes
			// without names.  It's used to communicate processing
			// instructions and things like that. We just ignore attributes
			// without names here.
			if (pAttr->name != NULL)
			{
			    IHXBuffer* pBuf = NULL;
			    if (HXR_OK == CreateAndSetBufferCCF(pBuf, (BYTE*)pAttr->value,
								strlen(pAttr->value)+1, m_pContext))
			    {
				pValues->SetPropertyCString(pAttr->name, pBuf);
				HX_RELEASE(pBuf);
			    }
			}
		    }
		    if(HXR_OK == rc)
		    {
			rc = m_pResponse->HandleStartElement(pName, 
			    pValues,ulTagStartLine,ulTagStartCol);

			HandleErrors(pTag->m_errs);
		    }
		    HX_RELEASE(pValues);

		    if(HXR_OK == rc &&
			!pTag->m_need_close)
		    {
			rc = m_pResponse->HandleEndElement(pName,ulTagStartLine,ulTagStartCol);
		    }
		}
		else
		{
		    rc = m_pResponse->HandleEndElement(pName,ulTagStartLine,ulTagStartCol);
		    HandleErrors(pTag->m_errs);
		}
	    }
	    break;

	    default:
	    {
		bDone = TRUE;
	    }
	    break;
	}
    }

exit:
    HX_DELETE(pTag);

    return rc;
}

HX_RESULT
HXStrictXMLParser::GetCurrentLineNumber(REF(ULONG32) ulLineNumber)
{
    HX_RESULT rc = HXR_OK;

    ulLineNumber = (UINT32)m_pParser->GetCurrentLineNumber();

    return rc;
}

HX_RESULT
HXStrictXMLParser::GetCurrentColumnNumber(REF(ULONG32) ulColumnNumber)
{
    HX_RESULT rc = HXR_OK;

    ulColumnNumber = (UINT32)m_pParser->GetCurrentColumnNumber();

    return rc;
}

HX_RESULT
HXStrictXMLParser::GetCurrentByteIndex(REF(ULONG32) ulByteIndex)
{
    HX_RESULT rc = HXR_NOTIMPL;

    return rc;
}

HX_RESULT
HXStrictXMLParser::GetCurrentErrorText(REF(IHXBuffer*) pBuffer)
{
    HX_RESULT rc = HXR_FAIL;

    XMLError* pLastError = m_pParser->GetLastError();
    if(pLastError &&
	pLastError->m_pErrorString)
    {
	rc = CreateAndSetBufferCCF(pBuffer, (BYTE*)pLastError->m_pErrorString,
				   strlen(pLastError->m_pErrorString) + 1, m_pContext);
    }

    return rc;
}

