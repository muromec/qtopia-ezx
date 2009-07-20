/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: rtspmdsc.cpp,v 1.7 2006/02/09 01:10:47 ping Exp $
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

//#include "hlxclib/stdlib.h"
//#include "hlxclib/stdio.h"

#include "hxtypes.h"
#include "hxcom.h"
#include "ihxpckts.h"
#include "hxstrutl.h"
#include "chxpckts.h"
#include "pckunpck.h"
#include "rtsputil.h"
#include "cbqueue.h"
#include "hxslist.h"
#include "looseprs.h"
#include "rtspprop.h"
#include "mhprop.h"
#include "rtspmdsc.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif

RTSPMediaDesc::RTSPMediaDesc(IUnknown* pContext)
	      :m_pFileHeader(0)
	      ,m_pContext(pContext)
{
    HX_ADDREF(m_pContext);
    m_streams = new CHXSimpleList;
}

RTSPMediaDesc::~RTSPMediaDesc()
{
    delete m_pFileHeader;
    clearStreamList();
    delete m_streams;
    HX_RELEASE(m_pContext);
}

HX_RESULT
RTSPMediaDesc::fromExternalRep(char* pData)
{
    XMLParser parser;
    XMLTag* pTag = 0;
    CByteQueue* pQueue = new CByteQueue((UINT16)strlen(pData));
    HXBOOL inMediaHeader = FALSE;
    HXBOOL inMediaStream = FALSE;
    HX_RESULT hresult = HXR_OK;

    RTSPPropertyList* pMediaHeader = 0;
    RTSPPropertyList* pMediaStream = 0;

    pQueue->EnQueue(pData, (UINT16)strlen(pData));
    for(;;)
    {
	UINT32 bytesUsed;
	UINT32 bytesAvail = pQueue->GetQueuedItemCount();
	if(bytesAvail <= 0)
	{
	    break;
	}

	BYTE* pBuf = new BYTE[bytesAvail];
	BYTE* p = pBuf;

	HX_ASSERT(bytesAvail == (UINT16)bytesAvail);

	pQueue->DeQueue(pBuf, (UINT16)bytesAvail);
	bytesUsed = bytesAvail;

	if(pTag)
	{
	    delete pTag;
	    pTag = 0;
	}

	XMLParseResult rc = parser.Parse((const char*&)p, bytesAvail, pTag);

	HX_ASSERT(bytesAvail == (UINT16)bytesAvail);

	pQueue->EnQueue(p, (UINT16)(bytesAvail - (p - pBuf)));

	switch(rc)
	{
	    case XMLPTag:
	    {
		switch(pTag->m_type)
		{
		    case XMLPlainTag:
		    {
			if(strcasecmp(pTag->m_name, "MediaHeader") == 0)
			{
			    if(inMediaHeader)
			    {
				//XXXBAB parse error
			    }
			    if(pMediaHeader)
			    {
				delete pMediaHeader;
			    }
			    pMediaHeader = new RTSPPropertyList;
			    inMediaHeader = TRUE;
			    inMediaStream = FALSE;
			}
			else if(strcasecmp(pTag->m_name, "MediaStream") == 0)
			{
			    if(inMediaStream)
			    {
				//XXXBAB parse error
			    }
			    pMediaStream = new RTSPPropertyList;
			    inMediaHeader = FALSE;
			    inMediaStream = TRUE;
			}
			else
			{
			    RTSPProperty* pProp = makeProperty(pTag);
			    if(pProp)
			    {
				if(inMediaHeader)
				{
				    pMediaHeader->addProperty(pProp);
				}
				else if(inMediaStream)
				{
				    pMediaStream->addProperty(pProp);
				}
			    }
			}
		    }
		    break;

		    case XMLEndTag:
		    {
			if(strcasecmp(pTag->m_name, "MediaHeader") == 0)
			{
			    if(inMediaHeader)
			    {
				m_pFileHeader = pMediaHeader;
			    }
			    else
			    {
				// error
			    }
			    inMediaHeader = FALSE;
			}
			else if(strcasecmp(pTag->m_name, "MediaStream") == 0)
			{
			    if(inMediaStream)
			    {
				m_streams->AddTail(pMediaStream);
			    }
			    else
			    {
				// error
			    }
			    inMediaStream = FALSE;
			}
		    }
		    break;

		    default:
		    {
			// unexpected
		    }
		    break;
		}
	    }
	    break;

	    case XMLPPlainText:
	    {
		// unexpected
	    }
	    break;

	    case XMLPComment:
	    {
		// oh, yeah?
	    }
	    break;

	    default:
	    {
		// unexpected
	    }
	    break;
	}
	delete[] pBuf;
    }

    if(pTag)
    {
	delete pTag;
    }
    delete pQueue;

    return hresult;
}
 
HX_RESULT
RTSPMediaDesc::toExternalRep(char *pBuf)
{
    CHXString mDesc = toExternalRep();
    strcpy(pBuf, mDesc);
    return HXR_OK;
}

CHXString
RTSPMediaDesc::toExternalRep()
{
    CHXString mDesc;
   
    mDesc =  "<MediaDescription>\r\n";
    
    if(m_pFileHeader)
    {
	mDesc += "    <MediaHeader>\r\n";
	RTSPProperty* pProp = m_pFileHeader->getFirstProperty();
	while(pProp)
	{
	    mDesc += "        " + pProp->asString() + "\r\n";
	    pProp = m_pFileHeader->getNextProperty();
	}
	mDesc += "    </MediaHeader>\r\n";
    }

    CHXSimpleList::Iterator i;
    for (i = m_streams->Begin();
	     i != m_streams->End();
	     ++i)
    {
	RTSPPropertyList* pStream = (RTSPPropertyList*)*i;
	if(pStream)
	{
	    mDesc += "    <MediaStream>\r\n";

	    RTSPProperty* pProp = pStream->getFirstProperty();
	    while(pProp)
	    {
		mDesc += "        " + pProp->asString() + "\r\n";
		pProp = pStream->getNextProperty();
	    }
	    mDesc += "    </MediaStream>\r\n";
	}
    }

    mDesc += "</MediaDescription>\r\n";
    return mDesc;
}

RTSPPropertyList*
RTSPMediaDesc::getFirstStream()
{
    m_listPos = m_streams->GetHeadPosition();
    if(m_listPos)
    {
	RTSPPropertyList* pStream = 
	    (RTSPPropertyList*)m_streams->GetNext(m_listPos);
	return pStream;
    }
    return 0;
}

RTSPPropertyList*
RTSPMediaDesc::getNextStream()
{
    RTSPPropertyList* pStream = 0;
    if(m_listPos)
    {
	pStream = (RTSPPropertyList*)m_streams->GetNext(m_listPos);
    }
    return pStream;
}

CHXSimpleList*
RTSPMediaDesc::getStreamList()
{
    return m_streams;
}

void 
RTSPMediaDesc::addStream(RTSPPropertyList* pStream)
{
    m_streams->AddTail(pStream);
}

void
RTSPMediaDesc::addHeader(RTSPPropertyList* pHeader)
{
    m_pFileHeader = pHeader;
}

RTSPPropertyList*
RTSPMediaDesc::getFileHeader()
{
    return m_pFileHeader;
}

void
RTSPMediaDesc::clearStreamList()
{
    LISTPOSITION pos = m_streams->GetHeadPosition();
    while(pos)
    {
	RTSPPropertyList* pStream = (RTSPPropertyList*)m_streams->GetNext(pos);
	delete pStream;
    }
    m_streams->RemoveAll();
}

RTSPProperty*
RTSPMediaDesc::makeProperty(XMLTag* pTag)
{
    const char* pDataName = pTag->m_name;
    const char* pDataType = pTag->get_attribute("type");
    if(pDataType)
    {
	const char* pDataValue = pTag->get_attribute("value");
	if(strcasecmp(pDataType, "integer") == 0)
	{
	    INT32 dataValue = strtol(pDataValue, 0, 10);
	    return new MHIntegerProperty(pDataName, dataValue);
	}
	else if(strcasecmp(pDataType, "string") == 0)
	{
	    return new MHStringProperty(pDataName, pDataValue);
	}
	else if(strcasecmp(pDataType, "buffer") == 0)
	{
	    char* pTmpBuf = new char[strlen(pDataValue)];
	    UINT32 dataLen = (UINT32)BinFrom64(pDataValue,
		(INT32)strlen(pDataValue), (BYTE*)pTmpBuf);
	    IHXBuffer* pBuffer = NULL;
	    if (HXR_OK == CreateAndSetBufferCCF(pBuffer, (BYTE*)pTmpBuf, 
						dataLen, m_pContext))
	    {
		RTSPBufferProperty* pProp = new MHBufferProperty(pDataName, pBuffer);
		HX_RELEASE(pBuffer);
	    
    		delete[] pTmpBuf;
		return pProp;
	    }
	}
    }
    return 0;
}
