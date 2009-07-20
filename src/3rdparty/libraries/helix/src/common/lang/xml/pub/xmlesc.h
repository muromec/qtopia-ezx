/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: xmlesc.h,v 1.8 2006/02/07 19:21:21 ping Exp $
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

#ifndef _XMLCONV_H_
#define _XMLCONV_H_
#include "growingq.h"
/****************************************************************************
 *  $Id: xmlesc.h,v 1.8 2006/02/07 19:21:21 ping Exp $
 *
 *  NAME:	xmlesc.h
 *
 *  CLASS
 * 	CEscapeXMLtoHTML
 *
 *  DESCRIPTION:
 *      The declaration for a class that escapes all the XML characters
 *      that are not printable in HTML.  A IHXBuffer is passed into the
 *      class on throgh the Convert Method.  This is a pure virtual 
 *      class. The CheckTag, and OnTag functions must be implemented by 
 *      a child class.
 *		
 ***************************************************************************/


// XXXJHUG - I can't find a max tag length in the XML specs - this works
// with everything we parse so far.
const UINT32 MAXTAGLEN = 128;

class CEscapeXMLtoHTML 
{
public:
    /*_______________________________________________________________________
     *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     *  CEscapeXMLtoHTML(pServerUrl, pOurPath)
     *
     *  PARAMETERS:
     *	    pServerUrl	Full url to the server including the template file.
     *	    pOurPath	The Relative path of the file currently being parsed
     *							no ending /
     *	    bMangleLinks    - to mangle or not
     *	    bUseStyles	    - output in styles or regular HTML
     *	    pHotTags	    - The tags that we will call OnTag For.
     *	    pDefaultView    - The default viewsource to use for remote rstp
     *			      links ":port/viewmount/viewfile"
     *_______________________________________________________________________
     *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
    CEscapeXMLtoHTML(IUnknown* pContext, IHXValues* pOptions, const char** pHotTags);
    
    /*_______________________________________________________________________
     *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     *  ~CEscapeXMLtoHTML()
     *_______________________________________________________________________
     *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
    virtual ~CEscapeXMLtoHTML();
    
    /*_______________________________________________________________________
     *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     *  Convert(pXMLsrc)
     *
     *  PARAMETERS:
     *	    pXMLsrc	Pointer to a IHXBuffer containing the src of xml to
     *			be converted
     *	RETURNS
     *	    A new IHXBuffer containing the parsed XML data ready to be 
     *	    displayed in an HTML browser.
     *_______________________________________________________________________
     *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
    STDMETHOD(Convert)(THIS_ IHXBuffer* pBufIn, REF(IHXBuffer*) pOutBuffer);
    
    /*_______________________________________________________________________
     *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     *  StatesEnum
     *
     *	DESCRIPTION:
     *		enum descibing the different stages of the Parse Function
     *_______________________________________________________________________
     *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
    enum StatesEnum { IN_CONTENT
        , IN_DOC_TYPE_DEC
        , IN_DOC_TYPE_DEC_TAG
        , IN_CDATA
	, ABOUT_TO_BEGIN_TAG
	, IN_BEGIN_TAG
	, IN_TAG
	, BEGIN_ATTRIBUTE_VALUE
	, IN_QUOTED_ATTRIBUTE_VALUE
	, IN_BROKEN_QUOTED_ATTRIBUTE_VALUE
	, IN_UNQUOTED_ATTRIBUTE_VALUE
	, IN_COMMENT
	, IN_AMPERSAND_THINGY 
    };

protected:
    /*_______________________________________________________________________
     *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     *  struct DataObject
     *
     *	DESCRIPTION:
     *		structure that defines the state of the Parse function
     *_______________________________________________________________________
     *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
    class DataObject {
    public:
	DataObject() : state(IN_CONTENT), bInBrokenXML(FALSE), 
	    bPushChar(FALSE), tag_index(0), bInProcessingInstructions(FALSE),
	    bInInternalComment(FALSE), bInDataSection(FALSE), cQuote('\0') {}
	StatesEnum state;
	HXBOOL bInBrokenXML;
	HXBOOL bPushChar;
	char tag[MAXTAGLEN]; /* Flawfinder: ignore */
	UINT32 tag_index;
	HXBOOL bInProcessingInstructions;
	HXBOOL bInInternalComment;
	HXBOOL bInDataSection;
	char cQuote;
    };

    /*_______________________________________________________________________
     *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     *  PURE VIRTUAL FUNCTION
     *	OnTag(cp, pObj, queue)
     *
     *  PARAMETERS:
     *
     *	cp	Pointer to a character string.  It points to the first
     *		letter of the tag name that we want to do special stuff to
     *  ulLen	Length to end of cp so we don't overrun the buffer
     *	pObj	The current state of the Parse Function, this can be used
     *		to continue parsing.
     *	queue	The queue to push the output to.
     *
     *  DESCRIPTION:
     *	    This method is implemented by a specialized class to take care of
     *	    a tag that returned true from the CheckTag call.  It returns how
     *	    many characters it ate off the cp.
     *	    This function can recursivly call the parse function to keep the
     *	    formatting consistant.
     *
     *  RETURNS
     *	    UINT32
     *		number of characters used off cp, thus the position can be
     *		adjusted after the function returns
     *_______________________________________________________________________
     *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
    virtual UINT32 OnTag(const UCHAR* cp, UINT32 ulLen, DataObject* pObj,
	    CBigByteGrowingQueue* queue) = 0;

    /*_______________________________________________________________________
     *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     *  PURE VIRTUAL FUNCTION
     *	PushHeader(queue)
     *
     *  PARAMETERS:
     *	queue	The queue to push the output to.
     *
     *  DESCRIPTION:
     *	    This method is implemented by classes that are derived from us.
     *	It allows the class to output type specific information before the
     *  Actual XML source is presented.
     *
     *  RETURNS
     *	    void
     *_______________________________________________________________________
     *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
    virtual void PushHeader(CBigByteGrowingQueue* queue) = 0;
    /*_______________________________________________________________________
     *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     *	PushCommonHeader(queue)
     *
     *  PARAMETERS:
     *	queue	The queue to push the output to.
     *
     *  DESCRIPTION:
     *	    This method outputs the common information for files.  It usually
     *	should be called from PushHeader after the file specific information
     *	has been queued.
     *
     *  RETURNS
     *	    void
     *_______________________________________________________________________
     *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
    void PushCommonHeader(CBigByteGrowingQueue* queue);
    /*_______________________________________________________________________
     *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     *  Parse(pIn, ulLen, pQueue, pObj)
     *
     *  PARAMETERS:
     *
     *	pIn	The string to be parsed.
     *	ulLen	How far to parse through pIn
     *	pQueue	The output Queue to push onto
     *	pObj	The state object.  This is used to start the queue from a
     *		given state.
     *
     *  DESCRIPTION:
     *
     *	    This function walks through the string one character at a time.
     *	    Every tag is checked with the CheckTag(...) function to see if it
     *	    needs special attention.  If so the OnTag(...) function is 
     *	    called.  This function will be implemented in a child class that
     *	    will replace the tag with parsed imput returning how far along 
     *	    pIn it made it.
     *
     *	    This function is designed such that Inherited classes can
     *	    recusivly call it to parse thier stream from the given state.
     *
     *  RETURNS
     *	    void
     *_______________________________________________________________________
     *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
    void Parse(const UCHAR* pIn, UINT32 ulLen, CBigByteGrowingQueue* pQueue,
						     DataObject* pObj);
    /*_______________________________________________________________________
     *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     *  WrapAttributeWithHREF (cp, pObj, pQueue, pAttribute)
     *
     *  PARAMETERS:
     *
     *  cp	Pointer to a character string.  It points to the first
     *		letter of the tag name that we want to do special stuff to
     *  ulLen	Length to end of cp so we don't overrun the buffer
     *  pObj	The current state of the Parse Function, this can be used
     *		to continue parsing.
     *  queue	The queue to push the output to.
     *  pAttribute	The attribute we want to wrap.
     *
     *  DESCRIPTION:
     *	    This method will take the given cp and wrap the given attribute
     *	with an HREF.  It will be used by Inherited classes in their OnTag
     *	Function
     *
     *  RETURNS
     *	UINT32
     *	    number of characters used off cp, thus the position can be
     *	    adjusted after the function returns
     *_______________________________________________________________________
     *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
    UINT32 WrapAttributeWithHREF(const UCHAR* cp,
	UINT32 ulLen, DataObject* pObj, CBigByteGrowingQueue* pQueue,
	const char* pAttribute);

#ifdef HYPER_LINKING_HREFS_XML
    UINT32 LinkHREF(const UCHAR* cp,
	UINT32 ulLen, DataObject* pObj, CBigByteGrowingQueue* pQueue);
#endif
/*___________________________________________________________________________
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * Helper Methods:  These methods are used to help witht the implentation of
 *		    OnTag function
 *___________________________________________________________________________
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
    HXBOOL m_bMangleLinks;
    
    char* m_pOurPath;
    char* m_pFileName;
    char* m_pRamGen;
    ULONG32 m_ulModDate;
    ULONG32 m_ulFileSize;

private:
    /*_______________________________________________________________________
     *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     *	CheckTag(pObj)
     *
     *  PARAMETERS:
     *	pObj	The State object.  It contains the tag name.
     *
     *  DESCRIPTION:
     *	    This method checks cp to see if it is the start of a tag we are
     *	    looking
     *  RETURNS
     *	    HXBOOL
     *		TRUE if we want OnTag called, or FALSE if we will ignor this
     *		tag
     *_______________________________________________________________________
     *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
    HXBOOL CheckTag(DataObject* pObj);
    
    HXBOOL PushOpenningHREF(const UCHAR* cp, CBigByteGrowingQueue* pQueue, char cEndQuote);
    void PushEndingHREF(CBigByteGrowingQueue* pQueue);
    UINT32 PushMangledDisplayedPath(const UCHAR* cp, 
	    CBigByteGrowingQueue* pQueue, char cEndQuote);
    char* EncryptParameter(char* pPath);
    UCHAR* GetParameter(const UCHAR* cp, UINT32 ulNameLen, HXBOOL bFullPath = FALSE);


    void BeginColorTag (CBigByteGrowingQueue* qu, DataObject *pObj);
    void EndColorTag (CBigByteGrowingQueue* qu, DataObject *pObj);

    const char** m_pEscapeStrings;
    const char** m_pHotTags;
    
    char* m_pDefaultView;
    char* m_pServerUrl;
    
    IUnknown* m_pContext;

    enum m_enumTags
    {
	BeginTagMarkup,
	EndTagMarkup,
	BeginTagNameMarkup,
	EndTagNameMarkup,
	BeginAttributeValueMarkup,
	EndAttributeValueMarkup,
	BeginBrokenAttributeMarkup,
	EndBrokenAttributeMarkup,
	BeginCommentMarkup,
	EndCommentMarkup,
	BeginAmpersandThingyMarkup,
	EndAmpersandThingyMarkup,
	BeginHREF,
        EndHREF,
        BeginProcessingInstructions,
        EndProcessingInstructions,
        BeginCDATA,
        EndCDATA,
        BeginDTD,
        EndDTD
    };

    static const char* const m_pDefaultTags[20];
    static const char* const m_pStyleTags[20];

};

#endif // _XMLCONV_H_
