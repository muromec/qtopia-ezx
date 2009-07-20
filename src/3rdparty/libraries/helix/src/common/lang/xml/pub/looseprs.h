/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: looseprs.h,v 1.5 2005/03/14 19:36:33 bobclark Exp $
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

#ifndef _XMLPARSE_H_
#define _XMLPARSE_H_


#include "carray.h"
#include "hxstack.h"

typedef enum {
    GSFoundExpected,
    GSNoValue,
    GSValueOnly,
    GSMissingQuote,
    GSEndTag,
    GSInvalid
} GetStringResult;

typedef enum {
    XMLPTag,
    XMLPNoClose,
    XMLPNoTagType,
    XMLPBadAttribute,
    XMLPAttributeValueNotQuoted,
    XMLPBadEndTag,
    XMLPBadProcInst,
    XMLPBadDirective,
    XMLPDupAttribute,
    XMLPCommentBeforeProcInst,
    XMLPPlainText,
    XMLPComment,
    XMLPDirective,   // Right word?
    XMLPProcInst,
    XMLPNotDone
} XMLParseResult;

typedef enum {
    XMLUnknownError,
    XMLErrorNoClose,
    XMLErrorBadAttribute,
    XMLErrorNoValue,
    XMLErrorMissingQuote,
    XMLErrorBadEndTag,
    XMLErrorNoTagType,
    XMLErrorDupAttribute
    , XMLErrorCommentBeforeProcInst
    
    , XMLErrorInvalidName
    , XMLErrorInvalidCharInDoc
    , XMLErrorTwoDashNotAllowed
    , XMLErrorInvalidDecl
    , XMLErrorInvalidPI
    , XMLErrorInvalidPITarget
    , XMLErrorInvalidCDATA
    
    , XMLErrorInvalidRef
    , XMLErrorMissingEquals
    , XMLErrorMissingReqSpace
    , XMLErrorLTnotAllowed
    , XMLErrorInvalidGTafter2RSQB
    , XMLErrorInvalidComment
} XMLErrorTag;

enum {
    TagType,
    AttributeName,
    AttributeValue,
    AttributeValueNoQuote,
    AttributeValueDirective
};

typedef enum {
    XMLPlainTag,
    XMLEndTag,
    XMLCommentTag,
    XMLProcInstTag,
    XMLDirectiveTag
} XMLTagType;


class XMLAttribute
{
public:
    XMLAttribute()
    {
	name = NULL;
	value = NULL;
    };

    ~XMLAttribute()
    {
	HX_VECTOR_DELETE(name);
	HX_VECTOR_DELETE(value);
    };

    char* name;
    char* value;
};

class XMLError
{
public:
    XMLError(XMLErrorTag errorTag,
	    INT32 lLineNumber,
	    INT32 lLinePosition,
	    const char* pErrorString,
	    const char* pFrameString);
    ~XMLError();

    char* m_pErrorString;
    char* m_pFrameString;
    INT32 m_lLineNumber;
    INT32 m_lLinePosition;
    XMLErrorTag m_errorTag;
};


class XMLTag
{
public:
    XMLTag(HXBOOL bStrictCompliance, HXBOOL bStoreErrors = FALSE);
    ~XMLTag();

    XMLAttribute* new_attribute();
    XMLAttribute* attribute(UINT32 a) { return (XMLAttribute*)m_attributes[(int)a]; };
    const char*   get_attribute(const char*);

    XMLTagType m_type;
    char* m_name;
    HXBOOL  m_need_close;
    UINT32 elem;
    UINT32 m_numAttributes;
    XMLAttribute* m_cur_attribute;
    CHXPtrArray m_attributes;
    HXBOOL m_bStrictCompliance;
    
    CHXPtrArray* m_errs;
};


class XMLParser
{
private:
    CHXStack	m_pStack;
    UINT32	m_comment_state;
    UINT32	m_comment_get_arg;
    UINT16	m_comment_pos;
    HXBOOL	m_comment_start;
    HXBOOL	m_bStrictCompliance;
    char	m_comment_arg[1024]; /* Flawfinder: ignore */
    char	m_comment_command[20]; /* Flawfinder: ignore */
    UINT32	m_ulCurrentLine;
    UINT32	m_ulCurrentCol;
    UINT32	m_ulTagStartLine;
    UINT32	m_ulTagStartCol;
    XMLError*	m_pLastError;
    char*	m_pEncoding;
    HXBOOL        m_bAllowNonXMLComments;
    HXBOOL	m_bCommentWasFound;

    HXBOOL	m_bStoreErrors;

    //This flag is always FALSE until we enable it in a future
    // release at which point we will initialize it to TRUE
    // and only set it to FALSE when an author or server admin
    // specifically specifies that he/she wants our old
    // SMIL parser to handle the (presumably poorly-authored) file.
    // NOTE: as of 1/20/2000, the code to allow authors to
    // force the old parser to be used is not yet written.
    // In addition, rmasmil/smlparse.h/.cpp has a similar flag and
    // the two need to be linked to the same value when that code
    // gets written:
    HXBOOL	m_bXMLandSMIL10FullCompliance;

    class XMLFrame
    {
	XMLFrame() : elemcount(0), name(NULL) {};
	~XMLFrame()
	{
	    if(name)
		delete [] name;
	};

	UINT32 elemcount;
	char*  name;
	friend class XMLParser;
    };

    char GetEscapeMacro(const char*&, const char*);
    GetStringResult GetString(const char*&, const char*,
			      char*&, UINT32);
    void FindCommentClose(const char*&, const char*, const char*);
    void SetError(XMLError*& pErr, XMLErrorTag tag, INT32 lLine,
		 INT32 lPos, const char* pErrorText,
		 INT32 lErrorTextLen,
		 const char* pFrameText);

public:
    XMLParser(HXBOOL bStrictCompliance = FALSE,
	      const char* pEncoding = NULL,
	      HXBOOL bAllowNonXMLComments = FALSE);
    ~XMLParser();

    static HX_RESULT
    GetPrologInfo(const char* pBuf,
		    UINT32 ulBufLen,
		    char*& pVersion,
		    char*& pEncoding);

    void Reset(void);
    XMLParseResult Parse(const char*& buf, UINT32 len, XMLTag*& tag, HXBOOL bIsFinal = TRUE);
    XMLParseResult ParseTag(const char* open, const char* close, 
	XMLTagType ulTagType, XMLTag*& tag);

    XMLParseResult ScanTag(const char* open, const char* close, XMLTag* tag);

    XMLError* GetLastError() { return m_pLastError; }

    UINT32 GetCurrentLineNumber()    { return m_ulCurrentLine; }
    UINT32 GetCurrentColumnNumber()  { return m_ulCurrentCol; }
    UINT32 GetTagStartLineNumber()   { return m_ulTagStartLine; }
    UINT32 GetTagStartColumnNumber() { return m_ulTagStartCol; }

    XMLFrame* m_pCurrentFrame;

    void StoreErrors() { m_bStoreErrors = TRUE; }
    void SetEncoding(const char* pszEncoding);
    HX_RESULT GetEncoding(char*& rpszEncoding);
};



#endif // _XMLPARSE_H_
