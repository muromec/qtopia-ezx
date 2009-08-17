/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: xmlesc.cpp,v 1.13 2006/02/07 19:21:20 ping Exp $
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

//   $Id: xmlesc.cpp,v 1.13 2006/02/07 19:21:20 ping Exp $

/*******************************************************************
 *
 *  NAME:	xmlesc.cpp
 *
 *  CLASS
 *	CEscapeXMLtoHTML
 *
 *  DESCRIPTION:
 *	The implmentation for a class that escapes all the XML characters
 *	that are not printable in HTML.  A IHXBuffer is passed into the
 *	class on throgh the Convert Method. This is a pure virtual class.
 *	The CheckTag, and OnTag functions must be implemented in a child
 *	class.
 *
 *  NOTES:
 *	Started with Mozilla's ColorHTML function
 *		
 *******************************************************************/

#include "hxcom.h"     /* IUnknown */
#include "hxtypes.h" 
#include "hxbuffer.h"
#include "hxiids.h"
#include "hxstrutl.h"
#include "hxstring.h"
#include "perplex.h"
#include "rtsputil.h"
#include "pckunpck.h"

#include "growingq.h"
#include "xmlesc.h"
#include "vsrcinfo.h"

#include "hlxclib/string.h"

/*___________________________________________________________________________
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * Static variable declarations
 *___________________________________________________________________________
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
const char* const
CEscapeXMLtoHTML::m_pDefaultTags[20] = 
{
     tag_BEGIN_TAG
    ,tag_END_TAG
    ,tag_BEGIN_TAG_NAME
    ,tag_END_TAG_NAME
    ,tag_BEGIN_ATTRIBUTE
    ,tag_END_ATTRIBUTE
    ,tag_BEGIN_BROKEN_ATT
    ,tag_END_BROKEN_ATT
    ,tag_BEGIN_COMMENT
    ,tag_END_COMMENT
    ,tag_BEGIN_AMPERSAND
    ,tag_END_AMPERSAND
    ,tag_BEGIN_HREF
    ,tag_END_HREF
    ,tag_PROCESSING_INSTRUCTIONS
    ,tag_END_PI
    ,tag_BEGIN_CDATA
    ,tag_END_CDATA
    ,tag_BEGIN_DTD
    ,tag_END_DTD
};

const char* const
CEscapeXMLtoHTML::m_pStyleTags[20] =
{
     style_BEGIN_TAG
    ,style_END_TAG
    ,style_BEGIN_TAG_NAME
    ,style_END_TAG_NAME
    ,style_BEGIN_ATTRIBUTE
    ,style_END_ATTRIBUTE
    ,style_BEGIN_BROKEN_ATT
    ,style_END_BROKEN_ATT
    ,style_BEGIN_COMMENT
    ,style_END_COMMENT
    ,style_BEGIN_AMPERSAND
    ,style_END_AMPERSAND
    ,style_BEGIN_HREF
    ,style_END_HREF
    ,style_PROCESSING_INSTRUCTIONS
    ,style_END_PI
    ,style_BEGIN_CDATA
    ,style_END_CDATA
    ,style_BEGIN_DTD
    ,style_END_DTD
};

/*___________________________________________________________________________
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * Public Methods
 *___________________________________________________________________________
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*___________________________________________________________________________
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *  CEscapeXMLtoHTML(pServerUrl, pOurPath)
 *
 *  PARAMETERS:
 *	pServerUrl  Full url to the server including the template file. 
 *	pOurPath    The Relative path of the file currently being parsed
 *		    no trailing "/"
 *	bMangleLinks    - to mangle or not
 *	bUseStyles	- output in styles or regular HTML
 *	pHotTags	- The tags that we will call OnTag For.
 *	pDefaultView    - The default viewsource to use for remote rstp
 *			  links ":port/viewmount/viewfile"
 *  DESCRIPTION:
 *	Constructor.
 *	This is the primary means of creating an instance of CEscapeXMLtoHTML.
 *___________________________________________________________________________
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
CEscapeXMLtoHTML::CEscapeXMLtoHTML(IUnknown* pContext, IHXValues* pOptions, const char** pHotTags) 
: m_pOurPath(NULL)
, m_pFileName(NULL)
, m_pRamGen(NULL)
, m_ulModDate(0)
, m_ulFileSize(0)
, m_pEscapeStrings(NULL)
, m_pHotTags(NULL)
, m_pDefaultView(NULL)
, m_pServerUrl(NULL)
, m_pContext(pContext)
{
    HX_ADDREF(m_pContext);

    m_pHotTags = pHotTags;

    IHXBuffer* pViewURL = NULL;
    IHXBuffer* pCurrentPath = NULL;
    IHXBuffer* pRemoteView = NULL;
    IHXBuffer* pFileName = NULL;
    UINT32 ulMangle = 0;
    UINT32 ulStyles = 0;
    if ( FAILED(pOptions->GetPropertyCString("ViewSourceURL", pViewURL)) ||
	 FAILED(pOptions->GetPropertyCString("CurrentPath", pCurrentPath)) ||
	 FAILED(pOptions->GetPropertyULONG32("HidePaths", ulMangle)) ||
	 FAILED(pOptions->GetPropertyULONG32("UseStyles", ulStyles)) ||
	 FAILED(pOptions->GetPropertyCString("RemoteViewSourceURL", pRemoteView)) ||
	 FAILED(pOptions->GetPropertyCString("FileName", pFileName)) ||
	 FAILED(pOptions->GetPropertyULONG32("ModificationTime", m_ulModDate)) ||
	 FAILED(pOptions->GetPropertyULONG32("FileSize", m_ulFileSize)) )
    {
	HX_ASSERT(FALSE);
	// return HXR_INVALID_PARAMETER;
    }

    m_bMangleLinks = ulMangle ? TRUE : FALSE;
    HXBOOL bUseStyles = ulStyles ? TRUE : FALSE;

    m_pServerUrl = new char[pViewURL->GetSize()+1];
    strcpy(m_pServerUrl, (char*)pViewURL->GetBuffer()); /* Flawfinder: ignore */

    m_pOurPath = new char[pCurrentPath->GetSize()+1];
    strcpy(m_pOurPath, (char*)pCurrentPath->GetBuffer()); /* Flawfinder: ignore */

    m_pDefaultView = new char[pRemoteView->GetSize()+1];
    strcpy(m_pDefaultView, (char*)pRemoteView->GetBuffer()); /* Flawfinder: ignore */

    m_pFileName = new char[pFileName->GetSize()+1];
    strcpy(m_pFileName, (char*)pFileName->GetBuffer()); /* Flawfinder: ignore */

    IHXBuffer* pRamGen = NULL;
    if ( SUCCEEDED(pOptions->GetPropertyCString("RamGenURL", pRamGen)) )
    {
	m_pRamGen = new char[pRamGen->GetSize()+1];
	strcpy(m_pRamGen, (char*)pRamGen->GetBuffer()); /* Flawfinder: ignore */
    }

    HX_RELEASE(pRamGen);
    HX_RELEASE(pRemoteView);
    HX_RELEASE(pViewURL);
    HX_RELEASE(pCurrentPath);
    HX_RELEASE(pFileName);

    if ( bUseStyles )
    {
	m_pEscapeStrings = (const char**) m_pStyleTags;
    }
    else
    {
	m_pEscapeStrings = (const char**) m_pDefaultTags;
    }
}

/*___________________________________________________________________________
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *  ~CEscapeXMLtoHTML()
 *
 *  PARAMETERS:
 *		none
 *
 *  DESCRIPTION:
 *		Destructor.
 *
 *___________________________________________________________________________
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
CEscapeXMLtoHTML::~CEscapeXMLtoHTML()
{
    HX_VECTOR_DELETE(m_pRamGen);
    HX_VECTOR_DELETE(m_pServerUrl);
    HX_VECTOR_DELETE(m_pOurPath);
    HX_VECTOR_DELETE(m_pDefaultView);
    HX_VECTOR_DELETE(m_pFileName);
    HX_RELEASE(m_pContext);
}

/*___________________________________________________________________________
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *  Convert(pXMLsrc, pOutBuffer)
 *
 *  PARAMETERS:
 *	pXMLsrc		Pointer to a IHXBuffer containing the src of xml to 
 *			be converted
 *	pOutBuffer	A new IHXBuffer containing the parsed XML data ready
 *			to be displayed in an HTML browser.
 *
 *  DESCRIPTION:
 *	Creates a growing queue, copies the buffers to a cstring, then calls 
 *	the Parse(...) function to parse the string.  Next the outputed is 
 *	dequeued Off the growing queue and put in a new IHXBuffer and 
 *	returned
 *
 *  RETURNS
 *	
 *___________________________________________________________________________
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
STDMETHODIMP CEscapeXMLtoHTML::Convert(IHXBuffer* pXMLsrc,
				       REF(IHXBuffer*) pOutBuffer)
{
    // TODO: UTF-16
    UCHAR* pIn = NULL;
    UINT32 ulLen;
    
    HX_ASSERT(pXMLsrc != NULL);
    pXMLsrc->Get(pIn, ulLen);
    
    CBigByteGrowingQueue* pQueue = NULL;
    
    if ( ulLen )
    {
	pQueue = new CBigByteGrowingQueue(ulLen*4);
	if ( !pQueue )
	{
	    // panic
	    HX_RELEASE(pXMLsrc);
	    return HXR_OUTOFMEMORY;
	}

	DataObject StateObj;
	StateObj.state = IN_CONTENT;
	StateObj.bPushChar = TRUE;
    
	PushHeader(pQueue);
	pQueue->EnQueue("<pre><!--  Begin Source  -->\n");
	Parse(pIn, ulLen, pQueue, &StateObj);
	pQueue->EnQueue("\n<!--  End Source  --></pre>\n");
	UINT32 ulOutLen;
	ulOutLen =  pQueue->GetQueuedItemCount();

	HX_RELEASE(pOutBuffer);
	if (HXR_OK != CreateBufferCCF(pOutBuffer, m_pContext))
	{
	    HX_RELEASE(pXMLsrc);
	    return HXR_OUTOFMEMORY;
	}

	if ( pOutBuffer->SetSize(ulOutLen) != HXR_OK )
	{
	    HX_RELEASE(pXMLsrc);
	    return HXR_OUTOFMEMORY;
	}
	UCHAR* pOut = pOutBuffer->GetBuffer();
	pQueue->DeQueue(pOut, ulOutLen);
    }
    else
    { // just return an empty buffer.
	HX_RELEASE(pOutBuffer);
	CreateBufferCCF(pOutBuffer, m_pContext);
    }
    HX_DELETE(pQueue);
    return HXR_OK;
}

/*___________________________________________________________________________
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *  PushCommonHeader(queue)
 *
 *  PARAMETERS:
 *
 *  queue	The queue to push the output to.
 *
 *  DESCRIPTION:
 *
 *	Pushes the common information onto the queue.
 *
 *  RETURNS
 *	
 *___________________________________________________________________________
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void CEscapeXMLtoHTML::PushCommonHeader(CBigByteGrowingQueue* queue)
{
    queue->EnQueue(z_pFileName);
    queue->EnQueue(m_pFileName);
    queue->EnQueue(z_pEndLine);
    
    QueueModificationTime(queue, m_ulModDate);

    QueueFileSize(queue, m_ulFileSize);

    if ( m_pRamGen )
    {
	queue->EnQueue(z_pRamLink);
	queue->EnQueue("<a href=\"");
	queue->EnQueue(m_pRamGen);
	const char* p = m_pOurPath;
	if ( *p == '/' )
	{
	    p++;
	}
	queue->EnQueue(p);
	queue->EnQueue("/");
	queue->EnQueue(m_pFileName);
	queue->EnQueue("\">");
	queue->EnQueue(m_pRamGen);
	queue->EnQueue(p);
	queue->EnQueue("/");
	queue->EnQueue(m_pFileName);
	queue->EnQueue("</a>");
	queue->EnQueue(z_pEndLine);
    }
}

/*___________________________________________________________________________
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * Protected Methods
 *___________________________________________________________________________
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*___________________________________________________________________________
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *  WrapAttributeWithHREF (pPositionPointer, ulLen, pObj, pQueue, pAttribute)
 *
 *  PARAMETERS:
 *
 *  pPositionPointer		Pointer to a character string.  It points to the first
 *		whitespace after the tagname
 *  ulLen	Length to end of pPositionPointer so we don't overrun the buffer
 *  pObj	The current state of the Parse Function, this can be used
 *		to continue parsing.
 *  queue	The queue to push the output to.
 *  pAttribute	The attribute we want to wrap.
 *
 *  DESCRIPTION:
 *	This method will take the given pPositionPointer and wrap the given attribute
 *	with an HREF
 *
 *  RETURNS
 *	UINT32
 *	    number of characters used off pPositionPointer, thus the position can be
 *	    adjusted after the function returns
 *___________________________________________________________________________
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
UINT32 CEscapeXMLtoHTML::WrapAttributeWithHREF(const UCHAR* pPositionPointer, 
					UINT32 ulLen,
					DataObject* pObj,
					CBigByteGrowingQueue* pQueue, 
					const char* pAttribute)
{
    // we need to find two things
    // 1 - the end of the tag
    // 2 - the beggining of the attribute
    const UCHAR* pBeginAttribute = NULL;
    const UCHAR* pEndTag = NULL;
    const char* walker = (const char*)pPositionPointer;
    HXBOOL bInComment = FALSE;
    UINT32 ulEscLen = 0;
    UINT16 uAttribLen = strlen(pAttribute);
    while ( (!pEndTag || bInComment) && ulEscLen < ulLen)
    {
	if ( !bInComment )
	{
	    if ( *walker == '>')
	    {
		pEndTag = (const UCHAR*)walker;
		++pEndTag;
	    }
	    else if ( ulEscLen + uAttribLen < ulLen &&
		isspace(*(walker-1)) &&
		!strncmp(walker, pAttribute, uAttribLen) )
	    {
		walker += uAttribLen;
		ulEscLen += uAttribLen;
		// eat ws.
		while ( isspace(*walker) && ulEscLen < ulLen )
		{
		    ++walker;
		    ++ulEscLen;
		}
		// assert for =
		HX_ASSERT(*walker == '=');
		if ( *walker == '=' )
		{
		    ++walker;
		    ++ulEscLen;
		    while ( isspace(*walker) && ulEscLen < ulLen )
		    {
			++walker;
			++ulEscLen;
		    }
		    // eat ws assert for " '
		    HX_ASSERT(*walker == '\"' || *walker == '\'');
		    if ( *walker == '\"' || *walker == '\'' )
		    {
			++walker;
			++ulEscLen;
			pBeginAttribute = (const UCHAR*)walker;
		    }
		}
	    }
	    else if ( ulEscLen + 4 < ulLen && !strncmp(walker, "<!--", 4) )
	    {
		bInComment = TRUE;
		walker += 3;
		ulEscLen += 3;
	    }
	}
	else
	{
	    if ( ulEscLen + 3 < ulLen && !strncmp(walker, "-->", 3) )
	    {
		bInComment = FALSE;
		walker += 2;
		ulEscLen += 2;
	    }
	}
	++walker;
	++ulEscLen;
    }

    UINT32 pos = 0;
    if ( pBeginAttribute ) {
	// parse until the end of the attribute tag.
	pos = pBeginAttribute - pPositionPointer;
	Parse(pPositionPointer, pos, pQueue, pObj);
	// advance the character pointer by how much we parsed.
	pPositionPointer += pos;
	// push our wrapper on
	HXBOOL bPushed = PushOpenningHREF(pPositionPointer, pQueue, pObj->cQuote);
	// push mangled filename on
	UINT32 ulPathLen = PushMangledDisplayedPath(pPositionPointer, pQueue, pObj->cQuote);
	// advancd pPositionPointer by path length
	pPositionPointer += ulPathLen;
	pos += ulPathLen;
	// push ending HREF
	if ( bPushed )
	{
	    PushEndingHREF(pQueue);
	}
	// parse to end of tag
	Parse(pPositionPointer, ulEscLen - pos, pQueue, pObj);
	pObj->bPushChar = FALSE;
	// We want to return pointing to the last '>', therefore we can
	// catch anything that is imediatly following our tag.
	return ulEscLen - 1;
    }

    pObj->bPushChar = TRUE;
    return 0;

}

/*___________________________________________________________________________
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *  WrapHREF (pPositionPointer, ulLen, pObj, pQueue)
 *
 *  PARAMETERS:
 *
 *  pPositionPointer		Pointer to a character string.  It points to the first
 *		whitespace after the tagname
 *  ulLen	Length to end of pPositionPointer so we don't overrun the buffer
 *  pObj	The current state of the Parse Function, this can be used
 *		to continue parsing.
 *  queue	The queue to push the output to.
 *  pAttribute	The attribute we want to wrap.
 *
 *  DESCRIPTION:
 *	This method will take the given pPositionPointer and wrap an HREF=
 *  With a link to itself.
 *
 *  RETURNS
 *	UINT32
 *	    number of characters used off pPositionPointer, thus the position can be
 *	    adjusted after the function returns
 *___________________________________________________________________________
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

#ifdef HYPER_LINKING_HREFS_XML

UINT32 CEscapeXMLtoHTML::LinkHREF(const UCHAR* pPositionPointer, 
					UINT32 ulLen,
					DataObject* pObj,
					CBigByteGrowingQueue* pQueue)
{
    const UCHAR* pBeginHREF = NULL;
    const UCHAR* pEndHREF = NULL;
    const UCHAR* pEndTag = NULL;
    const char* walker = (const char*)pPositionPointer;
    HXBOOL bInComment = FALSE;
    HXBOOL bInString = FALSE;
    UINT32 ulEscLen = 0;
    while ( (!pEndTag) && ulEscLen < ulLen)
    {
	if ( bInString )
	{
	    if ( *walker == '\"' || *walker == '\'')
	    {
		bInString = FALSE;
		pEndHREF = (const UCHAR*)walker;
	    }
	}
	else if ( !bInComment )
	{
	    if ( *walker == '>')
	    {
		pEndTag = (const UCHAR*)walker;
		++pEndTag;
	    }
	    else if ( ulEscLen + 6 < ulLen && 
		isspace(*(walker-1)) && 
		!strncmp(walker, "href", 4) &&
		(*(walker+4) == '=' || isspace(*(walker+4))) )
	    {
		walker += 4;
		ulEscLen += 4;
		while ( *walker != '\"' && *walker != '\'' && ulEscLen < ulLen &&
			(isspace(*walker) || *walker == '='))
		{
		    ++walker;
		    ++ulEscLen;
		}
		HX_ASSERT(*walker == '\"' || *walker == '\'');
		if ( *walker == '\"' || *walker == '\'')
		{
		    pBeginHREF = (const UCHAR*)walker + 1;
		    bInString = TRUE;
		}
	    }
	    else if ( ulEscLen + 4 < ulLen && 
		!strncmp(walker, "<!--", 4) )
	    {
		bInComment = TRUE;
		walker += 3;
		ulEscLen += 3;
	    }
	}
	else
	{
	    if ( ulEscLen + 3 < ulLen && !strncmp(walker, "-->", 3) )
	    {
		bInComment = FALSE;
		walker += 2;
		ulEscLen += 2;
	    }
	}
	++walker;
	++ulEscLen;
    }

    if ( pEndTag == NULL || pBeginHREF == NULL || pEndHREF == NULL )
    {
	return 0;
    }
    else
    {
	const UCHAR* cp = pPositionPointer;
	Parse(pPositionPointer, pBeginHREF - cp, pQueue, pObj);
	cp += pBeginHREF - cp;
	pQueue->EnQueue(m_pEscapeStrings[BeginHREF]);
	pQueue->EnQueue((void*)pBeginHREF, pEndHREF - pBeginHREF);
	pQueue->EnQueue("\">");
	Parse(cp, pEndHREF - pBeginHREF, pQueue, pObj);
	cp += pEndHREF - pBeginHREF;
	pQueue->EnQueue(m_pEscapeStrings[EndHREF]);

	// parse to end of tag
	Parse(cp, pEndTag - cp, pQueue, pObj);
	pObj->bPushChar = FALSE;
	// We want to return pointing to the last '>', therefore we can
	// catch anything that is imediatly following our tag.
	return pEndTag - pPositionPointer - 1;
    }
}

#endif //HYPER_LINKING_HREFS_XML

/*___________________________________________________________________________
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *  Parse(pIn, ulLen, pQueue, pObj)
 *
 *  PARAMETERS:
 *	pIn	The string to be parsed.
 *	ulLen	How far to parse through pIn
 *	pQueue	The output Queue to push onto
 *	pObj	The state object.  This is used to start the queue from a
 *		given state.
 *
 *  DESCRIPTION:
 *	This function walks through the string one character at a time.  
 *	Every tag is checked with the CheckTag(...) function to see if it 
 *	needs special attention.  If so the OnTag(...) function is called.  
 *	This function will be implemented in a child class that will replace 
 *	the tag with parsed imput returning how far along pIn it made it.
 *	This function is designed such that Inherited classes can recusivly 
 *	call it to parse thier stream from the given state.
 *
 *  RETURNS
 *	void
 *___________________________________________________________________________
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void
CEscapeXMLtoHTML::Parse(const UCHAR* pIn, UINT32 ulLen, 
			CBigByteGrowingQueue* pQueue,
			DataObject* pObj)
{
    HX_ASSERT(pIn);
    
    const UCHAR* pPositionPointer;

    UINT32 uPositionOffset;
    for ( uPositionOffset = 0, pPositionPointer = pIn;
	uPositionOffset < ulLen; uPositionOffset++, pPositionPointer++ )
    {
        switch(pObj->state)
        {
            case IN_CONTENT:
	    {
                /* do nothing until you find a '<' "<!--" or '&' */
                if ( *pPositionPointer == '<' )
                {
                    /* XXX we can miss a comment spanning a block boundary */
		    pObj->bPushChar = FALSE;
                    if ( uPositionOffset + 4 <= ulLen && 
			!strncmp((char*)pPositionPointer, "<!--", 4) )
                    {
                        pQueue->EnQueue(m_pEscapeStrings[BeginCommentMarkup]);
                        pQueue->EnQueue("&lt;");
                        pObj->state = IN_COMMENT;
                    }
                   else if ( uPositionOffset + 2 <= ulLen && 
		       !strncmp((char*)pPositionPointer, "<?", 2) )
                   {
                       pQueue->EnQueue(m_pEscapeStrings[BeginTagMarkup]);
		       pQueue->EnQueue("&lt;");
		       pQueue->EnQueue(m_pEscapeStrings[EndTagMarkup]);
                       pQueue->EnQueue(m_pEscapeStrings[BeginProcessingInstructions]);
                       pQueue->EnQueue("?");
                       pQueue->EnQueue(m_pEscapeStrings[BeginTagNameMarkup]);
                       // increment pPositionPointer & uPositionOffset one because of ?
                       ++pPositionPointer;
                       ++uPositionOffset;
                       pObj->bInProcessingInstructions = TRUE;
                       pObj->bPushChar = FALSE;
                       pObj->state = ABOUT_TO_BEGIN_TAG;
                   }
                   else if ( uPositionOffset + 9 <= ulLen && 
		       !strncmp((char*)pPositionPointer, "<![CDATA[", 9) )
                   {
		       pQueue->EnQueue(m_pEscapeStrings[BeginTagMarkup]);
                       pQueue->EnQueue("&lt;");
                       pQueue->EnQueue(m_pEscapeStrings[BeginTagNameMarkup]);
                       pQueue->EnQueue("![CDATA[");
                       pQueue->EnQueue(m_pEscapeStrings[EndTagNameMarkup]);
		       pQueue->EnQueue(m_pEscapeStrings[EndTagMarkup]);
                       pQueue->EnQueue(m_pEscapeStrings[BeginCDATA]);
                       pObj->bPushChar = FALSE;
                       pPositionPointer += 8;
                       uPositionOffset += 8;
                       pObj->state = IN_CDATA;
                   }
                   else if ( uPositionOffset + 2 <= ulLen && 
		       !strncmp((char*)pPositionPointer, "<!", 2) )
                   {
		       pQueue->EnQueue(m_pEscapeStrings[BeginTagMarkup]);
                       pQueue->EnQueue("&lt;");
                       pQueue->EnQueue(m_pEscapeStrings[BeginTagNameMarkup]);
                       pObj->state = IN_DOC_TYPE_DEC_TAG;
                   }
                    else
                    {
                        BeginColorTag(pQueue, pObj);
                    }
                }
                else if ( *pPositionPointer == '&' )
                {
                    pObj->bPushChar = FALSE;
		    pQueue->EnQueue(m_pEscapeStrings[BeginAmpersandThingyMarkup]);
                    pQueue->EnQueue("&amp;");
                    pObj->state = IN_AMPERSAND_THINGY;
                }
	    }
            break;
           case IN_DOC_TYPE_DEC_TAG:
           {
                if ( IS_SPACE(*pPositionPointer) )
                {
                    pQueue->EnQueue(m_pEscapeStrings[EndTagNameMarkup]);
		    pQueue->EnQueue(m_pEscapeStrings[EndTagMarkup]);
		    pQueue->EnQueue(m_pEscapeStrings[BeginDTD]);
		    pObj->bPushChar = TRUE; // we want to still push it
                    pObj->state = IN_DOC_TYPE_DEC;
                }
                else if ( *pPositionPointer == '>' )
                {
                    pQueue->EnQueue(m_pEscapeStrings[EndTagNameMarkup]);
                    pQueue->EnQueue(m_pEscapeStrings[EndDTD]);
                    pQueue->EnQueue("&gt;");
                    pObj->bPushChar = FALSE;
		    pObj->state = IN_CONTENT;
                }
           }
           break;
           case IN_DOC_TYPE_DEC:
           {
               if ( pObj->bInDataSection )
               {
                   if ( pObj->bInInternalComment )
                   {
                       /* do nothing until you find a closing '-->' */
                       if ( !strncmp((char*)pPositionPointer, "-->", 3) )
                       {
                           pObj->bPushChar = FALSE;
                           pObj->bInInternalComment = FALSE;
                           pQueue->EnQueue("--&gt;");
                           pPositionPointer += 2;
                           uPositionOffset += 2;
                           pQueue->EnQueue(m_pEscapeStrings[EndCommentMarkup]);
                       }
                   }
                   else if ( !strncmp((char*)pPositionPointer, "<!--", 4) )
                   {
                        pQueue->EnQueue(m_pEscapeStrings[BeginCommentMarkup]);
                        pQueue->EnQueue("&lt;");
                        pObj->bPushChar = FALSE;
                        pObj->bInInternalComment = TRUE;
                   }
                   else if ( *pPositionPointer == ']' )
                   {
                       pObj->bInDataSection = FALSE;
                   }
               }
               else if ( *pPositionPointer == '[' )
               {
                   pObj->bInDataSection = TRUE;
               }
               else if ( *pPositionPointer == '>' )
               {
                   pObj->bPushChar = FALSE;
                   pQueue->EnQueue(m_pEscapeStrings[EndDTD]);
		   pQueue->EnQueue(m_pEscapeStrings[BeginTagMarkup]);
                   pQueue->EnQueue("&gt;");
		   pQueue->EnQueue(m_pEscapeStrings[EndTagMarkup]);
                   pObj->state = IN_CONTENT;
               }
               if ( pObj->bPushChar )
               {
                   if ( *pPositionPointer == '<' )
                   {
                       pObj->bPushChar = FALSE;
                       /* protect ourselves from markup */
                       pQueue->EnQueue("&lt;");
                   }
                   else if ( *pPositionPointer == '&' )
                   {
                       /* protect ourselves from markup */
                       pQueue->EnQueue("&amp;");
                       pObj->bPushChar = FALSE;
                   }
               }

           }
           break;
           case IN_CDATA:
           {
               if ( !strncmp((char*)pPositionPointer, "]]>", 3) )
               {
                   pQueue->EnQueue(m_pEscapeStrings[EndCDATA]);
		   pQueue->EnQueue(m_pEscapeStrings[BeginTagMarkup]);
                   pQueue->EnQueue(m_pEscapeStrings[BeginTagNameMarkup]);
                   pQueue->EnQueue("]]");
                   pQueue->EnQueue(m_pEscapeStrings[EndTagNameMarkup]);
                   pQueue->EnQueue("&gt;");
		   pQueue->EnQueue(m_pEscapeStrings[EndTagMarkup]);
                   pPositionPointer += 2;
                   uPositionOffset += 2;
                   pObj->bPushChar = FALSE;
                   pObj->state = IN_CONTENT;
               }

               if ( *pPositionPointer == '<' )
                {
                   pObj->bPushChar = FALSE;
                    /* protect ourselves from markup */
                    pQueue->EnQueue("&lt;");
                }
               else if ( *pPositionPointer == '&' )
               {
                    /* protect ourselves from markup */
                   pQueue->EnQueue("&amp;");
                   pObj->bPushChar = FALSE;
               }
           }
           break;
            case ABOUT_TO_BEGIN_TAG:
	    {
                /* we have seen the first '<'
                 * once we see a non-whitespace character
                 * we will be in the tag identifier
                 */
                if ( *pPositionPointer == '>' )
                {
		    pQueue->EnQueue(m_pEscapeStrings[EndTagNameMarkup]);
                    EndColorTag(pQueue, pObj);
                }
                else if ( !IS_SPACE(*pPositionPointer) )
                {
                    pObj->state = IN_BEGIN_TAG;
                    pObj->tag_index = 0;
                    pObj->tag[pObj->tag_index++] = *pPositionPointer;
                    
    		    if ( *pPositionPointer == '<' )
		    {
                        pObj->bPushChar = FALSE;
			pQueue->EnQueue("&lt;");
		    }
                }
	    }
            break;
            case IN_BEGIN_TAG:
	    {
                /* go to the IN_TAG state when we see
                 * the first whitespace
                 */
                if ( IS_SPACE(*pPositionPointer) )
                {
                    pQueue->EnQueue(m_pEscapeStrings[EndTagNameMarkup]);
		    pObj->bPushChar = TRUE; // we want to still push it
                    pObj->state = IN_TAG;
                    if (pObj->tag_index < MAXTAGLEN) pObj->tag[pObj->tag_index] = '\0';
		    if ( CheckTag(pObj) )
		    {
			UINT32 pos = OnTag(pPositionPointer, ulLen - uPositionOffset, pObj, pQueue);
			pPositionPointer += pos;
			uPositionOffset += pos;
		    }
                }
                else if ( *pPositionPointer == '>' )
                {
                    pQueue->EnQueue(m_pEscapeStrings[EndTagNameMarkup]);
                    EndColorTag(pQueue, pObj);
                }
                else if ( *pPositionPointer == '<' )
                {
		    pObj->bPushChar = FALSE;
                    /* protect ourselves from markup */
                    if ( !pObj->bInBrokenXML )
                    {
                        pObj->bInBrokenXML = TRUE;
                        pQueue->EnQueue(m_pEscapeStrings[BeginBrokenAttributeMarkup]);
                        pQueue->EnQueue("&lt;");
                    }
                    else
                    {
                        pQueue->EnQueue("&lt;");
                    }
                }
                else
                {
                    if (pObj->tag_index < MAXTAGLEN)
		    {
                        pObj->tag[pObj->tag_index++] = *pPositionPointer;
		    }
                }
	    }
            break;
            case IN_TAG:
	    {
                /* do nothing until you find a opening '=' or end '>' */
		if ( *pPositionPointer == '=' )
                {
		    pObj->bPushChar = FALSE;
                    pQueue->EnQueue("=");
                    pQueue->EnQueue(m_pEscapeStrings[BeginAttributeValueMarkup]);
                    pObj->state = BEGIN_ATTRIBUTE_VALUE;
                }
                else if ( *pPositionPointer == '>' )
                {
                    EndColorTag(pQueue, pObj);
                }
                else if ( *pPositionPointer == '<' )
                {
		    pObj->bPushChar = FALSE;
                    /* protect ourselves from markup */
                    pQueue->EnQueue("&lt;");
                }
	    }
            break;
            case BEGIN_ATTRIBUTE_VALUE:
	    {
                /* when we reach the first non-whitespace
                 * we will enter the UNQUOTED or the QUOTED
                 * ATTRIBUTE state
                 */
                if ( !IS_SPACE(*pPositionPointer) )
                {
                    if ( *pPositionPointer == '\"' || *pPositionPointer == '\'')
                    {
			pObj->cQuote = *pPositionPointer;
                        pObj->state = IN_QUOTED_ATTRIBUTE_VALUE;
                        /* no need to jump to the quoted attr handler
                         * since this char can't be a dangerous char
                         */
                    }
                    else
                    {
                        pObj->state = IN_UNQUOTED_ATTRIBUTE_VALUE;
                        /* need to jump to the unquoted attr handler
                         * since this char can be a dangerous character
                         */
                        goto unquoted_attribute_jump_point;
                    }
                }
                else if ( *pPositionPointer == '>' )
                {
		    pQueue->EnQueue(m_pEscapeStrings[EndAttributeValueMarkup]);
                    EndColorTag(pQueue, pObj);
                }
                else if ( *pPositionPointer == '<' )
                {
                    /* protect ourselves from markup */
                    pObj->bPushChar = FALSE;
		    pQueue->EnQueue("&lt;");
                }
	    }
            break;
            case IN_UNQUOTED_ATTRIBUTE_VALUE:
	    {
unquoted_attribute_jump_point:
                /* do nothing until you find a whitespace */
                if ( IS_SPACE(*pPositionPointer) )
                {
                    pQueue->EnQueue(m_pEscapeStrings[EndAttributeValueMarkup]);
                    pObj->state = IN_TAG;
                }
                else if ( *pPositionPointer == '>' )
                {
		    pQueue->EnQueue(m_pEscapeStrings[EndAttributeValueMarkup]);
		    EndColorTag(pQueue, pObj);
                }
                else if ( *pPositionPointer == '<' )
                {
		    pObj->bPushChar = FALSE;
                    /* protect ourselves from markup */
                    pQueue->EnQueue("&lt;");
                }
                else if ( *pPositionPointer == '&' )
                {
		    pObj->bPushChar = FALSE;
                    /* protect ourselves from markup */
                    pQueue->EnQueue("&amp;");
                }
                break;
	    }
            case IN_QUOTED_ATTRIBUTE_VALUE:
	    {
                /* do nothing until you find a closing '"' */
		if ( *pPositionPointer == pObj->cQuote )
                {
		    pObj->bPushChar = FALSE;
                    if ( pObj->bInBrokenXML )
                    {
                        pQueue->EnQueue(m_pEscapeStrings[EndBrokenAttributeMarkup]);
                        pObj->bInBrokenXML = FALSE;
                    }
                    pQueue->EnQueue((void*)&pObj->cQuote, 1);
                    pQueue->EnQueue(m_pEscapeStrings[EndAttributeValueMarkup]);
                    pObj->state = IN_TAG;
                }
                else if ( *pPositionPointer == '<' )
                {
		    pObj->bPushChar = FALSE;
                    /* protect ourselves from markup */
                    pQueue->EnQueue("&lt;");
                }
                else if ( *pPositionPointer == '&' )
                {
		    pObj->bPushChar = FALSE;
                    /* protect ourselves from markup */
                    pQueue->EnQueue("&amp;");
                }
                else if ( *pPositionPointer == '>' )
                {
		    pObj->bPushChar = FALSE;
                    /* probably a broken attribute value */
                    if ( !pObj->bInBrokenXML )
                    {
                        pObj->bInBrokenXML = TRUE;
                        pQueue->EnQueue(m_pEscapeStrings[BeginBrokenAttributeMarkup]);
                        pQueue->EnQueue(">");
                    }
                }
	    }
            break;
            case IN_COMMENT:
	    {
                /* do nothing until you find a closing '-->' */
                if ( !strncmp((char*)pPositionPointer, "-->", 3) )
                {
		    pObj->bPushChar = FALSE;
                    pQueue->EnQueue("--&gt;");
                    pPositionPointer += 2;
                    uPositionOffset += 2;
                    pQueue->EnQueue(m_pEscapeStrings[EndCommentMarkup]);
                    pObj->state = IN_CONTENT;
                }
                else if ( *pPositionPointer == '<' )
                {
		    pObj->bPushChar = FALSE;
                    /* protect ourselves from markup */
                    pQueue->EnQueue("&lt;");
                }
                else if ( *pPositionPointer == '&' )
                {
                     /* protect ourselves from markup */
                    pQueue->EnQueue("&amp;");
                    pObj->bPushChar = FALSE;
                }
	    }
            break;
            case IN_AMPERSAND_THINGY:
	    {
                /* do nothing until you find a ';' or space */
                if ( *pPositionPointer == ';' || IS_SPACE(*pPositionPointer) )
                {
		    pObj->bPushChar = FALSE;
                    pQueue->EnQueue((char*)pPositionPointer, 1);
                    pQueue->EnQueue(m_pEscapeStrings[EndAmpersandThingyMarkup]);
                    pObj->state = IN_CONTENT;
                }
                else if ( *pPositionPointer == '<' )
                {
		    pObj->bPushChar = FALSE;
                    /* protect ourselves from markup */
                    pQueue->EnQueue("&lt;");
                }
	    }
            break;
            default:
                HX_ASSERT(0);
            break;
        }

	if ( pObj->bPushChar )
	{
	    pQueue->EnQueue((char*)pPositionPointer, 1);
	}
	pObj->bPushChar = TRUE;
    }
}

/*___________________________________________________________________________
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * Private Methods
 *___________________________________________________________________________
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*___________________________________________________________________________
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *  PushMangledDisplayedPath(pIn, pQueue, cEndQuote)
 *
 *  PARAMETERS:
 *	pIn	Pointer to a character string.  It is positioned
 *		inside a quote right infront of the path or file to
 *		to be mangled
 *	pQueue	A pointer to a CBigByteGrowingQueue that is used for queuing up 
 *		the parsered output.
 *      cEndQuote - the character which ends the quote (either ' or ")
 *
 *  DESCRIPTION:
 *	This Method replaces any path after a protocol with a /.../.
 *	If there is no protocol, then it just replaces any relative path with
 *	/.../
 *
 *  RETURNS:
 *	The Number of characters off pIn that it pushed onto the queue
 *___________________________________________________________________________
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
UINT32
CEscapeXMLtoHTML::PushMangledDisplayedPath(const UCHAR*          pIn, 
					   CBigByteGrowingQueue* pQueue,
                                           char                  cEndQuote)
{
    const char* pszQuote = strchr((const char*) pIn, cEndQuote);
    if (pszQuote)
    {
        UINT32 ulLen = ((const UCHAR*) pszQuote) - pIn;
        UINT32 ulPos = 0;
        if ( m_bMangleLinks )
        {
	    const UCHAR* pProtocolEnd = (const UCHAR*)strnstr((const char*)pIn, 
	        "://", (int)ulLen);

	    if ( pProtocolEnd )  // we have protocol
	    {
	        pProtocolEnd += 2; // only push one slash
	        UINT32 ulLenOfProtocol = pProtocolEnd - pIn;
	        pQueue->EnQueue((void*)pIn, ulLenOfProtocol);
	        pIn += ulLenOfProtocol;
	        ulPos += ulLenOfProtocol;
	    }

	    // find last '/'
	    while ( strnchr((const char*)pIn, '/', ulLen - ulPos) ) 
	    {
	        UINT32 temp = ulPos;
	        temp += (const UCHAR*)strnchr((const char*)pIn, '/', ulLen - ulPos) 
		        - pIn + 1;
	        pIn  = (const UCHAR*)strnchr((const char*)pIn, '/', ulLen - ulPos) 
		        + 1;
	        ulPos = temp;
	        
	    }

	    pQueue->EnQueue("/.../");
        }
        // else we just do the whole thing...
        pQueue->EnQueue((void*)pIn, ulLen - ulPos);
        return ulLen;
    }
    return 0;
}

/*___________________________________________________________________________
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *  PushOpenningHREF(pPositionPointer, pQueue, cEndQuote)
 *
 *  PARAMETERS:
 *	pPositionPointer	Pointer to a character string.  It is positioned
 *		inside a quote right infront of the path or file to
 *		to be referenced to.
 *		This contains the absolute url if that is needed, or it
 *		is used in the call to GetParameter(...).
 *	pQueue	A pointer to a CBigByteGrowingQueue that is used for queuing up 
 *		the output.
 *      cEndQuote - the character to end the quote (either ' or ")
 *
 *  DESCRIPTION:
 *	This Method pushes HREF onto the output queue.  It must build the
 *	Url from the m_pServerUrl and the parameter (GetParameter).
 *
 *
 *  RETURNS:
 *	void
 *___________________________________________________________________________
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
HXBOOL
CEscapeXMLtoHTML::PushOpenningHREF(const UCHAR* pPositionPointer, CBigByteGrowingQueue* pQueue, char cEndQuote)
{
    // if relative path
    // TODO: else we need to take care of full paths - 
    //	    pPositionPointer or if m_pOurPath has a protocol as well.
    const char* pszQuote = strchr((const char*) pPositionPointer, cEndQuote);
    if (pszQuote)
    {
        UINT32 ulLen = ((const UCHAR*) pszQuote) - pPositionPointer;

        if ( strncmp((const char*)pPositionPointer, "rtsp://", 7) == 0 )
        {
	    const char* p = (const char*)pPositionPointer;
	    const char* pEnd = p + ulLen;
	    p += 7;
	    // move p to the end of server name find the first : or /
	    while ( *p != ':' && *p != '/' && ++p != pEnd) {} ;

	    pQueue->EnQueue(m_pEscapeStrings[BeginHREF]);
	    // replacing rtsp with http
	    
	    //XXXJHUG  -- If the server in m_pServerUrl is the same as the 
	    // server we are about queue we will use m_pServerUrl
	    // instead of the default port and mountpoint.
	    
	    // mover past http://
	    char* pBeginServer = m_pServerUrl + 7;
	    
	    UINT32 ulServerLen = 0;
	    char* pServerPort = strchr(pBeginServer, ':');
	    // m_pServerURL will always have a port.
	    HX_ASSERT(pServerPort);
	    if ( pServerPort )
	    {
	        ulServerLen = pServerPort - pBeginServer;
	    }
	    // 7 for rtsp://
	    UINT32 ulXMLServerLen = p - (const char*)pPositionPointer - 7;
	    if ( ulServerLen == ulXMLServerLen &&
	        strncmp(pBeginServer, (const char*)pPositionPointer + 7, ulServerLen) == 0 )
	    {
	        // use m_pServerURL
	        pQueue->EnQueue(m_pServerUrl);
	    }
	    else
	    {
	        // use the Default Port
	        pQueue->EnQueue("http");
	        pQueue->EnQueue((void*)(pPositionPointer + 4), p - (const char*)pPositionPointer - 4);
	        pQueue->EnQueue(m_pDefaultView);
	    }

	    while ( *p != '/' && ++p != pEnd) {} ;
	    UCHAR* pParam = GetParameter((const UCHAR*)p, pEnd - p);

	    pQueue->EnQueue("?");
	    pQueue->EnQueue((const char*)pParam);
	    HX_VECTOR_DELETE(pParam);
	    pQueue->EnQueue("\">");
        }
        else if ( strnchr((const char*)pPositionPointer, ':', HX_MIN(6, ulLen)) )
        {
	    if ( strncmp((const char*)pPositionPointer, "pnm://", 6) != 0 &&
	        (strncmp(m_pServerUrl, "http://localhost", sizeof("http://localhost") - 1) == 0 ||
	        strncmp(m_pServerUrl, "http://127.0.0.1", sizeof("http://127.0.0.1") - 1) == 0) )
	    {
	        pQueue->EnQueue(m_pEscapeStrings[BeginHREF]);
	        pQueue->EnQueue(m_pServerUrl);
	        pQueue->EnQueue("?");
	        UCHAR* pParam = GetParameter(pPositionPointer, ulLen, TRUE);
	        pQueue->EnQueue((const char*)pParam);
	        HX_VECTOR_DELETE(pParam);
	        pQueue->EnQueue("\">");
	    }
	    else
	    {
	        return FALSE;
	    }
        }
        else
        {
	    pQueue->EnQueue(m_pEscapeStrings[BeginHREF]);
	    pQueue->EnQueue(m_pServerUrl);
	    pQueue->EnQueue("?");
	    UCHAR* pParam = GetParameter(pPositionPointer, ulLen);

	    pQueue->EnQueue((const char*)pParam);
	    HX_VECTOR_DELETE(pParam);
	    pQueue->EnQueue("\">");
        }
    }
    return TRUE;
}

/*___________________________________________________________________________
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *  PushEndingHREF(pQueue)
 *
 *  PARAMETERS:
 *	pQueue	A pointer to a CBigByteGrowingQueue that is used for queuing up 
 *		the parsered output.
 *
 *  DESCRIPTION:
 *	Pushes end of HREF onto the output queue.
 *
 *  RETURNS
 *	void
 *___________________________________________________________________________
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void
CEscapeXMLtoHTML::PushEndingHREF(CBigByteGrowingQueue* pQueue)
{
    pQueue->EnQueue(m_pEscapeStrings[EndHREF]);
}

/*___________________________________________________________________________
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *  GetParameter(pPositionPointer, ulNameLen)
 *
 *  PARAMETERS:
 *	pPositionPointer	Pointer to a character string.  It is positioned
 *		inside a quote right infront of the path or file to
 *		to be referenced to.
 *	ulNameLen	The length of the name.  It is not zero terminated.
 *
 *  DESCRIPTION:
 *	This method builds a relative path and paramiterizes it.  Then the
 *	parameter is encrypted with a call to EncryptParameter().  A string 
 *	is allocated to be returned
 *
 *  RETURNS
 *	a pointer to a new string that is to be used after the ? for an 
 *	option in a url
 *___________________________________________________________________________
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
UCHAR*
CEscapeXMLtoHTML::GetParameter(const UCHAR* pPositionPointer, UINT32 ulNameLen, 
			       HXBOOL bFullPath)
{
    // allocate longest possible string
    char* pReturnBuffer = new char[(strlen(m_pOurPath) + ulNameLen) + 10]; 
    //  2 for a possible starting '/' and a null terminator
    // 4 for the starting src=
    // 4 for extra bytes in case we have to pad the buffer when we encypt it.
    strcpy(pReturnBuffer, "src="); /* Flawfinder: ignore */

    char* pLinkPath = pReturnBuffer + 4;
    

    // if it starts with '/' then it is a full path
    if ( *pPositionPointer == '/' || bFullPath )
    {
	strncpy(pLinkPath, (const char*)pPositionPointer, ulNameLen); /* Flawfinder: ignore */
	pLinkPath[ulNameLen] = '\0';
    }
    // if it is alpha it is simply a file name
    else if ( isalnum(*pPositionPointer) )
    {
	// 1 for "/"
	UINT32 len = strlen(m_pOurPath) + ulNameLen + 1;
	strcpy(pLinkPath, m_pOurPath); /* Flawfinder: ignore */
	strcat(pLinkPath, "/"); /* Flawfinder: ignore */
	strncat(pLinkPath, (const char*)pPositionPointer, ulNameLen); /* Flawfinder: ignore */
	pLinkPath[len] = '\0';
    }
    else if ( !strncmp((const char*)pPositionPointer, "./", 2) )
    {
	// -1 for .
	UINT32 len = strlen(m_pOurPath) + ulNameLen - 1;
	strcpy(pLinkPath, m_pOurPath); /* Flawfinder: ignore */
	pPositionPointer += 1;
	strncat(pLinkPath, (const char*)pPositionPointer, ulNameLen - 1); /* Flawfinder: ignore */
	pLinkPath[len] = '\0';
    }
    else if ( !strncmp((const char*)pPositionPointer, "../", 3 ) )
    {
	// copy m_pOurPath into pLinkPath ourselves cause we need to be at
	// the end anyway.
	const char* pSrc = m_pOurPath;
	char* pCurrentEndOfPath = pLinkPath;
	const char* pRelativePath = (const char*)pPositionPointer;
	
	// Walk to take care of any ../ that might be in the path...
	char* pDest = pCurrentEndOfPath;
	while ( *pSrc )
	{
	    while ( *pSrc == '.' && *(pSrc + 1) == '.' && *(pSrc + 2) == '/' )
	    {
		--pDest;
		while ( *(pDest-1) != '/' && (pDest-1) >= pLinkPath )
		{
		    --pDest;
		}
		pSrc += 3;
	    }
	    *pDest = *pSrc;
	    ++pDest;
	    ++pSrc;
	}
	*pDest = '\0';

	pCurrentEndOfPath += strlen(pCurrentEndOfPath);

	// back up a directory off of the file path for
	// every ../ we find
	while (!strncmp((const char*)pRelativePath, "../", 3 ))
	{
	    // we found a ../ so back up a directory on the path,
	    // walk backwards to the previous / and set it to null
	    while (*pCurrentEndOfPath != '/' && 
		pCurrentEndOfPath >= pLinkPath)
	    {
		pCurrentEndOfPath--;
	    }
	    // watch to make sure we don't have more ../ than directories
	    if ( pCurrentEndOfPath < pLinkPath)
	    {
		++pCurrentEndOfPath;
	    }
	    *pCurrentEndOfPath = '\0';
	    pRelativePath +=3;
	}

        UINT32 len = (pCurrentEndOfPath - pLinkPath) + 
	    ulNameLen - (pRelativePath - (const char*)pPositionPointer) + 1;
	
	// back 1 off of pRelativePath so we get the / that's there.
	strncat(pLinkPath, (const char*)pRelativePath - 1, /* Flawfinder: ignore */
	    ulNameLen - (pRelativePath - (const char*)pPositionPointer) + 1);
	pLinkPath[len] = '\0';
    }
    else
    {
	HX_ASSERT(FALSE);
	pLinkPath = '\0';
    }

    char* pParam = EncryptParameter(pReturnBuffer);
    HX_VECTOR_DELETE(pReturnBuffer);
    return (UCHAR*)pParam;
}

/*___________________________________________________________________________
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *  EncryptParameter(pPath)
 *
 *  PARAMETERS:
 *	pPath	Pointer to a string to be parameterized and encrypted.
 *
 *  DESCRIPTION:
 *	First it is assigned to a variable (src), and then the parameter is
 *	encrypted
 *
 *  RETURNS
 *	a pointer to a new string that is to be used after the ? for an 
 *	option in a url
 *___________________________________________________________________________
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
char*
CEscapeXMLtoHTML::EncryptParameter(char* pPath)
{
    UINT32 FinalLen;			    // length of encoded data
    UINT32 Offset = strlen(pPath);
    UINT32 nAlign = Offset % sizeof(ULONG32);
    if (nAlign > 0)
    {
	for (; nAlign < sizeof(ULONG32); nAlign++)
	{
	    pPath[Offset++] = 0;
	}
    }
    FinalLen = (Offset) * Perplex_PER_ULONG32 / sizeof(ULONG32);	
    // calc size of the outout (Perplexed) buffer.
    // alloc mem for final perplexed buffer
    // Add one to length 'cause low level perplex adds
    // a '\0' to the resulting string

    char* output = new char[FinalLen+1];

    CHXPerplex::DumpToPerplex((char*)output,FinalLen+1,(UCHAR*) pPath, Offset);
    return output;
}

/*___________________________________________________________________________
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *  CheckTag (pObj)
 *
 *  PARAMETERS:
 *	pObj		current state
 *
 *  DESCRIPTION:
 *	This method pushes the begging of a color tag onto the queue
 *
 *  RETURNS
 *	void
 *___________________________________________________________________________
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
HXBOOL
CEscapeXMLtoHTML::CheckTag(DataObject* pObj)
{
    if ( m_pHotTags ) 
    {
	for ( INT32 i = 0; m_pHotTags[i]; i++ )
	{
	    if ( !strcmp(pObj->tag, m_pHotTags[i]) )
	    {
		    return TRUE;
	    }
	}
    }
    return FALSE;
}
/*___________________________________________________________________________
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *  BeginColorTag (CBigByteGrowingQueue* qu, DataObject *pObj)
 *
 *  PARAMETERS:
 *		qu			queue to output to
 *		pObj		current state
 *  DESCRIPTION:
 *		This method pushes the begging of a color tag onto the queue
 *	RETURNS
 *		void
 *___________________________________________________________________________
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void
CEscapeXMLtoHTML::BeginColorTag (CBigByteGrowingQueue* qu, DataObject *pObj)
{
    qu->EnQueue(m_pEscapeStrings[BeginTagMarkup]);
    qu->EnQueue("&lt;");
    qu->EnQueue(m_pEscapeStrings[BeginTagNameMarkup]);
    pObj->state = ABOUT_TO_BEGIN_TAG;
    pObj->bPushChar = FALSE;
}

/*___________________________________________________________________________
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *  EndColorTag (CBigByteGrowingQueue* qu, DataObject *pObj)
 *
 *  PARAMETERS:
 *		qu		queue to output to
 *		pObj		current state
 *
 *  DESCRIPTION:
 *	This method pushes the end of a color tag onto the queue
 *
 *  RETURNS
 *	void
 *___________________________________________________________________________
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void
CEscapeXMLtoHTML::EndColorTag (CBigByteGrowingQueue* qu, DataObject *pObj)
{
    if ( pObj->bInBrokenXML )
    {
        qu->EnQueue(m_pEscapeStrings[EndBrokenAttributeMarkup]);
        pObj->bInBrokenXML = FALSE;
    }
    if ( pObj->bInProcessingInstructions )
    {
       qu->EnQueue(m_pEscapeStrings[EndProcessingInstructions]);
       qu->EnQueue(m_pEscapeStrings[BeginTagMarkup]);
       qu->EnQueue("&gt;");
       qu->EnQueue(m_pEscapeStrings[EndTagMarkup]);
       pObj->bInProcessingInstructions = FALSE;
    }
    else
    {
       qu->EnQueue("&gt;");
       qu->EnQueue(m_pEscapeStrings[EndTagMarkup]);
    }
    pObj->state = IN_CONTENT;
    pObj->bPushChar = FALSE;
}

