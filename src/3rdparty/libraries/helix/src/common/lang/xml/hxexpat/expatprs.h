/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: expatprs.h,v 1.5 2005/03/14 19:36:33 bobclark Exp $
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
struct IHXPlugin;

#include "baseobj.h"

class HXExpatXMLParser: public CHXBaseCountingObject,
                        public IHXXMLParser,
			public IHXXMLNamespaceParser
{
private:
    LONG32			    m_lRefCount;
    IHXXMLParserResponse*	    m_pResponse;
    IUnknown*			    m_pContext;
    IHXCommonClassFactory*	    m_pClassFactory;
    XML_Parser			    m_pParser;

    IHXBuffer*			    m_pCurrentBuffer;
    UINT32			    m_ulCurrentOffset;

    IHXXMLNamespaceResponse*	    m_pNSResp;
    HXBOOL			    m_bInited;
    char			    m_cSepChar;

    void SetAttBuffer(IHXBuffer* pBuf, const char* p);

    virtual ~HXExpatXMLParser	    ();

public:
    HXExpatXMLParser(IUnknown* pContext);

    /* IUnknown methods */

    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)	(THIS);

    STDMETHOD_(ULONG32,Release)	(THIS);
    
    /* expat call backs */
    void handleStartElement(const XML_Char *name, const XML_Char **atts);
    void handleEndElement(const XML_Char *name);
    void handleCharacterData(const XML_Char *s, int len);
    void handleProcessingInstruction(const XML_Char *target,
    	const XML_Char *data);
    void handleComment(const XML_Char *data);
    void handleStartCDataSection();
    void handleEndCDataSection();
    void handleDefault(const XML_Char *s, int len);

    void handleUnparsedEntityDecl(
			    const XML_Char *entityName,
			    const XML_Char *base,
			    const XML_Char *systemId,
			    const XML_Char *publicId,
			    const XML_Char *notationName);
    void handleNotationDecl(
		      const XML_Char *notationName,
		      const XML_Char *base,
		      const XML_Char *systemId,
		      const XML_Char *publicId);
    void handleStartNamespaceDecl(
			      const XML_Char *prefix,
			      const XML_Char *uri);
    void handleEndNamespaceDecl(const XML_Char *prefix);

    int handleNotStandalone();

    void handleStartDoctypeDecl(const XML_Char *doctypeName);
    void handleEndDoctypeDecl();

    /* IHXXMLParser methods */

    STDMETHOD(Init)			(THIS_
					IHXXMLParserResponse*	/*IN*/  pResponse,
					const char*	    /*IN*/	pEncoding,
					HXBOOL		    /*IN*/	bStrict);

    STDMETHOD(Close)			(THIS);	

    STDMETHOD(Parse)			(THIS_
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
    /* IHXXMLNamespaceParser methods */

    STDMETHOD(InitNamespaceParser)	(THIS_
					IHXXMLParserResponse*	/*IN*/  pResponse,
					IHXXMLNamespaceResponse* /*IN*/pNSResp,
					const char*	    /*IN*/	pEncoding,
					const char	    /*IN*/	cSepChar);
    STDMETHOD_(char, GetSepChar)		(THIS);
};

#endif	/* _HXXMLPRS_H_ */
