/* ***** BEGIN LICENSE BLOCK ***** 
 * Version: RCSL 1.0/RPSL 1.0 
 *  
 * Portions Copyright (c) 1995-2002 RealNetworks, Inc. All Rights Reserved. 
 *      
 * The contents of this file, and the files included with this file, are 
 * subject to the current version of the RealNetworks Public Source License 
 * Version 1.0 (the "RPSL") available at 
 * http://www.helixcommunity.org/content/rpsl unless you have licensed 
 * the file under the RealNetworks Community Source License Version 1.0 
 * (the "RCSL") available at http://www.helixcommunity.org/content/rcsl, 
 * in which case the RCSL will apply. You may also obtain the license terms 
 * directly from RealNetworks.  You may not use this file except in 
 * compliance with the RPSL or, if you have a valid RCSL with RealNetworks 
 * applicable to this file, the RCSL.  Please see the applicable RPSL or 
 * RCSL for the rights, obligations and limitations governing use of the 
 * contents of the file.  
 *  
 * This file is part of the Helix DNA Technology. RealNetworks is the 
 * developer of the Original Code and owns the copyrights in the portions 
 * it created. 
 *  
 * This file, and the files included with this file, is distributed and made 
 * available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER 
 * EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS ALL SUCH WARRANTIES, 
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS 
 * FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT. 
 * 
 * Technology Compatibility Kit Test Suite(s) Location: 
 *    http://www.helixcommunity.org/content/tck 
 * 
 * Contributor(s): 
 *  
 * ***** END LICENSE BLOCK ***** */ 

#ifndef _RSXMLPARSER_H_
#define _RSXMLPARSER_H_

/* 
        Summary:

        This class is a simple object wrapper around the Expat XML parser.  It handles the 
        proccessing of the XML, and stores the parsed data in a tree of Element, with
        each Element containing a vector of it's own attributes in the m_vAttributes 
        member and a vector of it's child elements in the m_vElements member.

        Basic Usage:

            HX_RESULT ParserResult;
            CHXTParser* pParser = new CHXTParser();
            
            CHXString sBuffer = "<? xml .... some xml ....";
            ParserResult = pParser->ParserBuffer(sBuffer, sBuffer.size());

                                                    OR
            
            ParserResult = pParser->ParseFile("somexmlfile.xml")

            if( FAILED(ParserResult) )
            {
                char* szError;
                UINT32 nErrorLineNum;
                pParser->GetLastError(&szError, &nErrorLineNum);
            }
            else
            {
                CHXTElement* pRootElement = pParser->GetRootElement();
                char* szElementName;
                szElementName = pRootElement->GetName();
            }               

        Also see the accompianing HLXXmlParserTest.cpp file for a more complete example
*/


#include "hxcom.h"
#include "carray.h"
#include "hxstack.h"
#include <stdio.h>
#include "hxstring.h"


class CHXTAttribute
{
public:
    CHXTAttribute();
    CHXTAttribute(const char* szName, const char* szValue);
    CHXTAttribute(const CHXTAttribute& rhs);
    ~CHXTAttribute();

    CHXTAttribute& operator=(const CHXTAttribute& rhs);

    const char* GetName() const;
    CHXString GetXMLSafeNameUTF8() const;
    const char* GetValue() const;
    CHXString GetXMLSafeValueUTF8() const;
    
    void SetName(const char* szName);
	void SetNameUTF8(const char* szName);
    void SetValue(const char* szValue);
	void SetValueUTF8(const char* szValue);

    operator HXBOOL () const;

    void Release();

private:
    struct AttributeData 
    {
        UINT32 ulRefCount;
        CHXString m_sName;
        CHXString m_sValue;

        AttributeData();
        AttributeData(const char* szName, const char* szValue);
        ~AttributeData();
    };

    AttributeData* m_pData;
};

inline
CHXTAttribute::operator HXBOOL () const
{
    return m_pData ? !m_pData->m_sName.IsEmpty() : false;
}


class CHXTElement;
struct ElementData;

class CHXTElement : public CHXTAttribute
{
public:
    CHXTElement();
    CHXTElement(const CHXTElement& rhs);
    ~CHXTElement();

    CHXTElement& operator=(const CHXTElement& rhs);

    const CHXPtrArray& GetChildren() const;
    const CHXTElement& GetChild(UINT32 nChildIndex) const;
	UINT32 GetChildrenCount() const;
    
    const CHXPtrArray& GetAttributes() const; // CHXTAttribute*
    const CHXTAttribute& GetAttribute(UINT32 nAttributeIndex) const;
    const CHXTAttribute* FindAttribute(const char* szAttributeName) const;
    const CHXTElement* FindChildElement(const char* szElementName) const;
    
    HX_RESULT AddAttribute(const char* szName, const char* szValue);
    HX_RESULT AddChild(const char* szName, const char* szValue, UINT32* pnIndex=NULL);
    HX_RESULT AddChildElement(CHXTElement* pNewElement, UINT32* pnIndex=NULL);

	HX_RESULT RemoveChild(UINT32 nChildIndex);

    void Release();

private:
    ElementData* m_pData;
};



struct ElementData 
{
    UINT32 ulRefCount;
    CHXPtrArray m_vChildElements;
    CHXPtrArray m_vAttributes;

    ElementData();
    ~ElementData();
    ElementData& operator=(const ElementData& rhs);
};


class CHXTXmlParser
{
public:
    CHXTXmlParser();
    ~CHXTXmlParser();

    HX_RESULT ParseFile(const char* szFilename);
    HX_RESULT ParseBuffer(const char* szBuffer, UINT32 nBufferSize);

    HX_RESULT WriteFile(const char* szFilename, HXBOOL bUseUnicode);
	// Passing a NULL buffer to WriteBuffer will cuase the function to return the required
	// buffer size in the pnBufferSize variable
    HX_RESULT WriteBuffer(char* szBuffer, UINT32* pnBufferSize, HXBOOL bUseUnicode);

    HX_RESULT AddRootElement(CHXTElement* pNewElement);
    const CHXTElement& GetRootElement() const;

    static void startElement(void *userData, const char *name, const char **atts);
    static void endElement(void *userData, const char *name);
    static void characterDataHandler(void *userData, const char *szCharData, int nLength);

    void GetLastError(const char** pszError, UINT32* pnLineNum);

private:

    HX_RESULT RecursiveFileWrite(FILE* pXmlFile, const CHXTElement& pElement);
    HX_RESULT RecursiveBufferWrite(char* szBuffer, UINT32* pnBufferLength, const CHXTElement& pElement);
    HX_RESULT WriteToBuffer(char* szBuffer, const char* szSource, UINT32* pnBufferLength);

    CHXTElement* m_pRootElement;
    CHXString m_sEncoding;
    CHXString m_sError;
    UINT32 m_nErrorLineNum;

	CHXStack m_stackElement; //CHXTElement*
};

#endif
