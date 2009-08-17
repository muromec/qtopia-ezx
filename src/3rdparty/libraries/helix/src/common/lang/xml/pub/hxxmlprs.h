/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxxmlprs.h,v 1.6 2006/01/31 23:35:05 ping Exp $
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

#ifndef _HXXMLPRS_H_
#define _HXXMLPRS_H_

/* forward declarations */
struct IHXXMLParserResponse;
struct IHXBuffer;
struct IHXValues;
class XMLParser;
class CBigByteQueue;
class CHXPtrArray;

class ErrorNotifier
{
public:
    virtual HX_RESULT ErrorInLastTag(HX_RESULT Error, 
		const char* pErrorString, const char* pFrameString, 
		UINT32 ulLineNumber, UINT32 ulLinePosition) = 0;
};

class HXActualXMLParser
{
protected:
    IUnknown*				m_pContext;
    IHXXMLParserResponse*		m_pResponse;

    void CheckEncoding(XMLParser* pParser, IHXBuffer* pBuffer);

public:
    HXActualXMLParser			(IUnknown* pContext);
    virtual ~HXActualXMLParser		();

    virtual HX_RESULT Init		(IHXXMLParserResponse* pResponse,
					const char* pEncoding) = 0;

    virtual HX_RESULT Parse		(IHXBuffer* pBuffer,
					HXBOOL bIsFinal) = 0;

    virtual HX_RESULT GetCurrentLineNumber	(REF(ULONG32) ulLineNumber) = 0;

    virtual HX_RESULT GetCurrentColumnNumber	(REF(ULONG32) ulColumnNumber) = 0;

    virtual HX_RESULT GetCurrentByteIndex	(REF(ULONG32) ulByteIndex) = 0;

    virtual HX_RESULT GetCurrentErrorText	(REF(IHXBuffer*) pBuffer) = 0;

    virtual HX_RESULT InitErrorNotifier(ErrorNotifier* /*OUT*/ pNotifier) = 0;
};

class HXStrictXMLParser: public HXActualXMLParser
{
private:
    XMLParser*				m_pParser;
    CBigByteQueue*			m_pByteQueue;
    ErrorNotifier*			m_pErrorNotifier;

    HX_RESULT DoParse			(HXBOOL bIsFinal);

    HX_RESULT HandleErrors(CHXPtrArray* pErrs);
    HX_RESULT ConvertToHX_RESULT(UINT32 err);

public:
    HXStrictXMLParser			(IUnknown* pContext);
    virtual ~HXStrictXMLParser		();

    virtual HX_RESULT Init		(IHXXMLParserResponse* pResponse,
					const char* pEncoding);

    virtual HX_RESULT Parse		(IHXBuffer* pBuffer,
					HXBOOL bIsFinal);

    virtual HX_RESULT GetCurrentLineNumber	(REF(ULONG32) ulLineNumber);

    virtual HX_RESULT GetCurrentColumnNumber	(REF(ULONG32) ulColumnNumber);

    virtual HX_RESULT GetCurrentByteIndex	(REF(ULONG32) ulByteIndex);

    virtual HX_RESULT GetCurrentErrorText	(REF(IHXBuffer*) pBuffer);
    
    virtual HX_RESULT InitErrorNotifier(ErrorNotifier* /*OUT*/ pNotifier);
};

class HXLooseXMLParser: public HXActualXMLParser
{
private:
    XMLParser*				m_pParser;
    CBigByteQueue*			m_pByteQueue;
    HXBOOL                                m_bAllowNonXMLComments;

    HX_RESULT DoParse			(HXBOOL bIsFinal);

public:
    HXLooseXMLParser			(IUnknown* pContext, HXBOOL bAllowNonXMLComments = FALSE);
    virtual ~HXLooseXMLParser		();

    virtual HX_RESULT Init		(IHXXMLParserResponse* pResponse,
					const char* pEncoding);

    virtual HX_RESULT Parse		(IHXBuffer* pBuffer,
					HXBOOL bIsFinal);

    virtual HX_RESULT GetCurrentLineNumber	(REF(ULONG32) ulLineNumber);

    virtual HX_RESULT GetCurrentColumnNumber	(REF(ULONG32) ulColumnNumber);

    virtual HX_RESULT GetCurrentByteIndex	(REF(ULONG32) ulByteIndex);

    virtual HX_RESULT GetCurrentErrorText	(REF(IHXBuffer*) pBuffer);

    virtual HX_RESULT InitErrorNotifier(ErrorNotifier* /*OUT*/ pNotifier) { return HXR_OK; }
};


class HXXMLParser: public IHXXMLParser
{
private:
    LONG32			    m_lRefCount;
    IHXXMLParserResponse*	    m_pResponse;
    IUnknown*			    m_pContext;
    HXBOOL                          m_bAllowNonXMLComments;

    // parser wrapper
    HXActualXMLParser*		    m_pParser;

    virtual ~HXXMLParser	    ();

public:
    HXXMLParser(IUnknown* pContext, HXBOOL bAllowNonXMLComments = FALSE);

    /* IUnknown methods */

    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)	(THIS);

    STDMETHOD_(ULONG32,Release)	(THIS);


    /* IHXXMLParser methods */

    STDMETHOD(Init)		(THIS_
				IHXXMLParserResponse*	/*IN*/  pResponse,
				const char*	    /*IN*/	pEncoding,
				HXBOOL		    /*IN*/	bStrict);

    STDMETHOD(Close)		(THIS);	

    STDMETHOD(Parse)		(THIS_
				IHXBuffer*	/*IN*/	    pBuffer,
				HXBOOL		/*IN*/	    bIsFinal);

    STDMETHOD(GetCurrentLineNumber)	(THIS_
					REF(ULONG32) /*OUT*/ ulLineNumber);

    STDMETHOD(GetCurrentColumnNumber)	(THIS_
					REF(ULONG32) /*OUT*/ ulColumnNumber);

    STDMETHOD(GetCurrentByteIndex)	(THIS_
					REF(ULONG32) /*OUT*/ ulByteIndex);

    STDMETHOD(GetCurrentErrorText)	(THIS_
					REF(IHXBuffer*) /*OUT*/ pBuffer);

    HX_RESULT InitErrorNotifier(ErrorNotifier* /*OUT*/ pNotifier);
};

#endif	/* _HXXMLPRS_H_ */
