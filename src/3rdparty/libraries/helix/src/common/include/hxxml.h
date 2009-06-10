/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxxml.h,v 1.6 2009/02/20 20:07:16 ehyche Exp $
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

#ifndef _HXXML_H_
#define _HXXML_H_

/*
 * Forward declarations of some interfaces defined here-in.
 */
typedef _INTERFACE	IHXXMLParserResponse		IHXXMLParserResponse;
//$ Private:
typedef _INTERFACE	IHXXMLNamespaceParser		IHXXMLNamespaceParser;
typedef _INTERFACE	IHXXMLNamespaceResponse	IHXXMLNamespaceResponse;
//$ EndPrivate.
typedef _INTERFACE	IHXBuffer			IHXBuffer;
typedef _INTERFACE	IHXValues			IHXValues;


/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXXMLParser
 * 
 *  Purpose:
 * 
 *	XML Parsing object
 * 
 *  IID_IHXXMLParser:
 * 
 *	{00002D00-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXXMLParser, 0x1a39e773, 0xfe28, 0x4ca9, 0x93, 0x18,
            0x9d, 0x21, 0xee, 0x85, 0xe4, 0x7a);

#define CLSID_IHXXMLParser IID_IHXXMLParser

#undef  INTERFACE
#define INTERFACE   IHXXMLParser

DECLARE_INTERFACE_(IHXXMLParser, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /*
     *	IHXXMLParser methods
     */

    /************************************************************************
     *	Method:
     *	    IHXXMLParser::Init
     *	Purpose:
     *	    Initilizes an XML parser with a response object, an encoding (normally
     *	    NULL, but can be any supported character encoding string such as 
     *	    "Shift_JIS" or "Big5"). The boolean bStrict is set to FALSE if a 'loose'
     *	    interpretation of the XML spec is allowed by the client; this includes
     *	    tag/attribute case insensitivity
     *
     *  commonly supported encodings:
     *	    "Default"   (assumes DBCS; any non-ASCII char is a lead byte)
     *	    "US-ASCII"
     *	    "Shift-JIS"
     *	    "Big5"
     *	    "GB2312"
     *	    "EUC-KR"
     *	    "ISO-2022-KR"
     *	    "ISO-8859-1"
     *
     *	NOTE: it's usually pretty safe to report NULL for the encoding, unless
     *	    you have a specific character encoding to use.
     */
    STDMETHOD(Init)	(THIS_
			IHXXMLParserResponse*	/*IN*/  pResponse,
			const char*	    /*IN*/	pEncoding,
			HXBOOL		    /*IN*/	bStrict) PURE;

    /************************************************************************
     *	Method:
     *	    IHXXMLParser::Close
     *	Purpose:
     *	    Releases all resources associated with the object.
     *
     */
    STDMETHOD(Close)	(THIS) PURE;	

    /************************************************************************
     *	Method:
     *	    IHXXMLParser::Parse
     *	Purpose:
     *	    Parse the buffer calling any IHXXMLParserResponse methods as various
     *	    XML entities are parsed. The boolean bIsFinal should be set to TRUE
     *	    when the last buffer to parse is passed to this method. Parse()
     *      can be called multiple times without calling Close() and then
     *      Init() again, even if bIsFinal is TRUE each time.
     *
     *      Also note that pBuffer should NOT be NULL terminated since
     *      the NULL terminator is not considered part of the XML document.
     *	
     */
    STDMETHOD(Parse)	(THIS_
			IHXBuffer*	/*IN*/	    pBuffer,
			HXBOOL		/*IN*/	    bIsFinal) PURE;

    /************************************************************************
     *	Method:
     *	    IHXXMLParser::GetCurrentLineNumber
     *	Purpose:
     *	    Returns the line number at which the parser's internal cursor is 
     *	    positioned.
     *
     */
    STDMETHOD(GetCurrentLineNumber)	(THIS_
					REF(ULONG32) /*OUT*/ ulLineNumber) PURE;

    /************************************************************************
     *	Method:
     *	    IHXXMLParser::GetCurrentColumnNumber
     *	Purpose:
     *	    Returns the column number at which the parser's internal cursor is
     *
     */
    STDMETHOD(GetCurrentColumnNumber)	(THIS_
					REF(ULONG32) /*OUT*/ ulColumnNumber) PURE;

    /************************************************************************
     *	Method:
     *	    IHXXMLParser::GetCurrentByteIndex
     *	Purpose:
     *	    Returns the byte index at which the parser's internal cursor is
     *
     */
    STDMETHOD(GetCurrentByteIndex)	(THIS_
					REF(ULONG32) /*OUT*/ ulByteIndex) PURE;


    /************************************************************************
     *	Method:
     *	    IHXXMLParser::GetCurrentErrorText
     *	Purpose:
     *	    Returns the text of the last error 
     *
     */
    STDMETHOD(GetCurrentErrorText)	(THIS_
					REF(IHXBuffer*) /*OUT*/ pBuffer) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXXMLParserResponse
 * 
 *  Purpose:
 * 
 *	XML Parsing response object
 * 
 *  IID_IHXXMLParserResponse:
 * 
 *	{00002D01-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXXMLParserResponse, 0x00002D01, 0x901, 0x11d1, 
	    0x8b, 0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXXMLParserResponse

DECLARE_INTERFACE_(IHXXMLParserResponse, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /*
     *	IHXXMLParserResponse methods
     */

    /************************************************************************
     *	Method:
     *	    IHXXMLParser::HandleStartElement
     *	Purpose:
     *	    Called with a start tag (<tag a="foo" b="bar">)
     *	    Line/column numbers are for the start of the entity
     */
    STDMETHOD(HandleStartElement)	(THIS_
					const char*	/*IN*/	pName,
					IHXValues*	/*IN*/	pAttributes,
					UINT32		/*IN*/	ulLineNumber,
					UINT32		/*IN*/	ulColumnNumber) PURE;

    /************************************************************************
     *	Method:
     *	    IHXXMLParser::HandleEndElement
     *	Purpose:
     *	    Called with an end tag (</tag>)
     *	    Line/column numbers are for the start of the entity
     */
    STDMETHOD(HandleEndElement)		(THIS_
					const char*	/*IN*/	pName,
					UINT32		/*IN*/	ulLineNumber,
					UINT32		/*IN*/	ulColumnNumber) PURE;

    /************************************************************************
     *	Method:
     *	    IHXXMLParser::HandleCharacterData
     *	Purpose:
     *	    Called with stuff outside of tags
     *	    Line/column numbers are for the start of the entity.
     *      Note that you CANNOT assume that pBuffer contains a NULL-terminated string.
     */
    STDMETHOD(HandleCharacterData)	(THIS_
					IHXBuffer*	/*IN*/	pBuffer,
					UINT32		/*IN*/	ulLineNumber,
					UINT32		/*IN*/	ulColumnNumber) PURE;


    /************************************************************************
     *	Method:
     *	    IHXXMLParser::HandleProcessingInstruction
     *	Purpose:
     *	    Called with processing instruction (<?foobar flotz="fred" glootz="mary"?>)
     *	    Line/column numbers are for the start of the entity
     */
    STDMETHOD(HandleProcessingInstruction)  (THIS_
					    const char* /*IN*/  pTarget,
					    IHXValues* /*IN*/  pAttributes,
					    UINT32	/*IN*/	ulLineNumber,
					    UINT32	/*IN*/	ulColumnNumber) PURE;


    /************************************************************************
     *	Method:
     *	    IHXXMLParser::HandleUnparsedEntityDecl
     *	Purpose:
     *	    Called with entity info (<!ENTITY hatch-pic SYSTEM "../grafix/OpenHatch.gif" 
     *					NDATA gif>)
     *	    Line/column numbers are for the start of the entity
     */
    STDMETHOD(HandleUnparsedEntityDecl)	    (THIS_
					    const char* /*IN*/  pEntityName,
					    const char* /*IN*/  pSystemID,
					    const char* /*IN*/  pPublicID,
					    const char* /*IN*/  pNotationName,
					    UINT32	/*IN*/	ulLineNumber,
					    UINT32	/*IN*/	ulColumnNumber) PURE;


    /************************************************************************
     *	Method:
     *	    IHXXMLParser::HandleNotationDecl
     *	Purpose:
     *	    Called with notation info (<!NOTATION gif PUBLIC "http://www.gif.com/gif.not">)
     *	    Line/column numbers are for the start of the entity
     *
     */
    STDMETHOD(HandleNotationDecl)	    (THIS_
					    const char* /*IN*/  pNotationName,
					    const char* /*IN*/  pSystemID,
					    const char* /*IN*/  pPublicID,
					    UINT32	/*IN*/	ulLineNumber,
					    UINT32	/*IN*/	ulColumNumber) PURE;

    /************************************************************************
     *	Method:
     *	    IHXXMLParser::HandleComment
     *	Purpose:
     *	    Called with comment info (<!-- this is a comment -->)
     *	    Line/column numbers are for the start of the entity
     *
     */
    STDMETHOD(HandleComment)		    (THIS_
					    const char* /*IN*/  pComment,
					    UINT32	/*IN*/	ulLineNumber,
					    UINT32	/*IN*/	ulColumnNumber) PURE;


    /************************************************************************
     *	Method:
     *	    IHXXMLParser::HandleUnparsedDoctypeDecl
     *	Purpose:
     *	    Called with declaration info (<!DOCTYPE foo SYSTEM "foo.dtd">)
     *	    Line/column numbers are for the start of the entity
     *
     */
    STDMETHOD(HandleUnparsedDoctypeDecl)   (THIS_
					    const char* /*IN*/  pDoctype,
					    const char* /*IN*/  pSystemID,
					    const char* /*IN*/  pPublicID,
					    UINT32	/*IN*/	ulLineNumber,
					    UINT32	/*IN*/	ulColumnNumber) PURE;

    /************************************************************************
     *	Method:
     *	    IHXXMLParser::HandleDefault
     *	Purpose:
     *	    Called with unhandled entitiy data
     *	    Line/column numbers are for the start of the entity
     *
     */
    STDMETHOD(HandleDefault)		    (THIS_
					    IHXBuffer*	/*IN*/	pBuffer,
					    UINT32	/*IN*/	ulLineNumber,
					    UINT32	/*IN*/	ulColumnNumber) PURE;

};

//$ Private:
/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXXMLNamespaceParser
 * 
 *  Purpose:
 * 
 *	Extended XML Parsing to support Namespaces
 * 
 *  IID_IHXXMLNamespaceParser:
 * 
 *	{00002D02-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXXMLNamespaceParser, 0x00002D02, 0x901, 0x11d1, 
	    0x8b, 0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXXMLNamespaceParser

DECLARE_INTERFACE_(IHXXMLNamespaceParser, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /*
     *	IHXXMLNamespaceParser methods
     */
    
    /************************************************************************
     *	Method:
     *	    IHXXMLParser::Init
     *	Purpose:
     *	    Initilizes an XML parser with a response object, an encoding (normally
     *	    NULL, but can be any supported character encoding string such as 
     *	    "Shift_JIS" or "Big5"). The boolean bStrict is set to FALSE if a 'loose'
     *	    interpretation of the XML spec is allowed by the client; this includes
     *	    tag/attribute case insensitivity
     */
    STDMETHOD(InitNamespaceParser)	(THIS_
			IHXXMLParserResponse*	/*IN*/  pResponse,
			IHXXMLNamespaceResponse* /*IN*/pNSResp,
			const char*	    /*IN*/	pEncoding,
			const char	    /*IN*/	cSepChar) PURE;
    STDMETHOD_(char, GetSepChar)		(THIS) PURE;

};

/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXXMLNamespaceResponse
 * 
 *  Purpose:
 * 
 *	Namespace response object
 * 
 *  IID_IHXXMLNamespaceResponse:
 * 
 *	{00002D03-0901-11d1-8B06-00A024406D59}
 *
 */ 
DEFINE_GUID(IID_IHXXMLNamespaceResponse, 0x00002D03, 0x901, 0x11d1, 
	    0x8b, 0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXXMLNamespaceResponse

DECLARE_INTERFACE_(IHXXMLNamespaceResponse, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /*
     *	IHXXMLNamespaceResponse methods
     */

    /************************************************************************
     *	Method:
     *	    IHXXMLNamespaceResponse::HandleStartNamespaceDecl
     *	Purpose:
     *	    Called with xmlns:prefix="uri" or xmlns="uri" when it enters
     *	    scope before the tag in which it was declared
     */
    STDMETHOD(HandleStartNamespaceDecl)	(THIS_
					const char*	/*IN*/	pPrefix,
					const char*	/*IN*/	pURI) PURE;

    /************************************************************************
     *	Method:
     *	    IHXXMLParser::HandleEndElement
     *	Purpose:
     *	    Called when the namespace leaves scope.
     */
    STDMETHOD(HandleEndNamespaceDecl) (THIS_
				      const char* pPrefix) PURE;

};

//$ EndPrivate.

#endif	/* _HXXML_H_ */
