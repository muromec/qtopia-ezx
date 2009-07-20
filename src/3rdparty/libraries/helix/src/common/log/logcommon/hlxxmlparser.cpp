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

#ifdef _WIN32
#pragma warning (disable : 4786)
#endif

#include "hxcom.h"
#include "hxassert.h"

#include "hxtxmlparser.h"
#include "xmltok.h"
#include "xmlparse.h"
#include <stdio.h>
#include "hxstring.h"

#include "hxtdirconv.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE
static char HX_THIS_FILE[] = __FILE__;
#endif


static HX_RESULT ConvertANSIToUTF8(const char* szOrig, CHXString& sResult);
static HX_RESULT ConvertUTF8ToANSI(const char* szOrig, CHXString& sResult);


static HX_RESULT ConvertANSIToUTF8(const char* szOrig, CHXString& sTranslated)
{
    HX_RESULT res = HXR_OK;

    sTranslated = "";

    // Validate params
    if (!szOrig || strlen(szOrig) == 0)
    {
        //HX_ASSERT(FALSE);		
        return HXR_FAIL;
    }

    // Convert ANSI to UTF-8
    UINT32 ulOrigSize = strlen(szOrig);
    for (UINT32 nCharCount = 0; nCharCount < ulOrigSize; ++nCharCount)
    {
        // For each byte x
        // If x < 128 then UTF-8 is 1 byte long
        //	byte 1 = x
        // If x >= 128 and < 2048 then UTF-8 is 2 bytes long
        //	byte 1 = 192 + (x div 64)
        //	byte 2 = 128 + x mod 64
        UINT8 bChar = szOrig[nCharCount];		
        if (bChar < 128)
        {
            sTranslated += (char)bChar;
        }
        else if (bChar < 2048)
        {
            char c1 = (char)(192 + (bChar / 64));
            char c2 = (char)(128 + (bChar % 64));

            sTranslated += c1;
            sTranslated += c2;
        }

        else
        {
            // Invalid sequence -- should never happen
            HX_ASSERT(FALSE);
            res = HXR_FAIL;
            break;			
        }		
    }

    return res;
}


static HX_RESULT ConvertUTF8ToANSI(const char* szOrig, CHXString& sTranslated)
{
    HX_RESULT res = HXR_OK;

    sTranslated = "";

    // Validate params
    if (!szOrig || strlen(szOrig) == 0)
    {
        HX_ASSERT(FALSE);		
        return HXR_FAIL;
    }

    UINT32 ulOrigSize = strlen(szOrig);
    for (UINT32 nCharCount = 0; nCharCount < ulOrigSize; ++nCharCount)
    {
        UINT8 bChar = (UINT8)szOrig[nCharCount];

        // Start with the first byte of the buffer
        // If this is < 128 then 1 byte should be consumed
        //		Given byte sequence x, ud = x
        // If this is >=128 but < 224 then 2 bytes should be consumed
        //		Given byte sequence x y, ud = (x-192)*64 + (y-128)
        // If this is >=224 then 3 bytes should be consumed
        //		Given byte sequence x y z, ud = (x-224)*4096 + (y-128)*64 + (z-128)
        if (bChar < 128)
        {
            sTranslated += (char)bChar;
        }
        else if (bChar < 224)
        {
            if (++nCharCount >= ulOrigSize)
            {
                HX_ASSERT(FALSE);
                res = HXR_FAIL;
                break;
            }

            UINT8 bChar2 = szOrig[nCharCount];

            if (bChar2 < 128)
            {
                HX_ASSERT(FALSE);
                res = HXR_FAIL;
                break;
            }

            sTranslated += (char)((bChar-192)*64 + (bChar2-128));
        }

        else
        {
            // Invalid sequence
            HX_ASSERT(FALSE);
            res = HXR_FAIL;
            break;
        }
    }

    return res;
}


CHXTAttribute::CHXTAttribute() 
: m_pData(new AttributeData())
{
}


CHXTAttribute::CHXTAttribute(const CHXTAttribute& rhs) :
m_pData(rhs.m_pData)
{
    ++(m_pData->ulRefCount);
}

CHXTAttribute::AttributeData::AttributeData() :
ulRefCount(1)
{
}

CHXTAttribute::AttributeData::AttributeData(const char* szName, const char* szValue) :
ulRefCount(1)
{
    m_sName = szName;
    m_sValue = szValue;
}


CHXTAttribute::CHXTAttribute(const char* szName, const char* szValue) :
m_pData(new AttributeData(szName, szValue))
{
}

CHXTAttribute::AttributeData::~AttributeData()
{
}

CHXTAttribute::~CHXTAttribute()
{
    if( --(m_pData->ulRefCount) == 0 )
    {
        delete m_pData;
    }
}

CHXTAttribute& CHXTAttribute::operator=(const CHXTAttribute& rhs)
{
    // Don't bother doing anything if the values are the same
    if( rhs.m_pData == m_pData )
        return *this;

    if( --(m_pData->ulRefCount) == 0 )
        delete m_pData;

    m_pData = rhs.m_pData;
    ++(m_pData->ulRefCount);

    return *this;
}


const char* CHXTAttribute::GetName() const
{
    return m_pData->m_sName;
}

CHXString CHXTAttribute::GetXMLSafeNameUTF8() const
{
    UINT32 nCharCount;
    CHXString sTranslatedName;

    // Replace reserved characters
    for(nCharCount = 0; nCharCount < m_pData->m_sName.GetLength(); ++nCharCount)
    {
        switch(m_pData->m_sName.GetAt(nCharCount))
        {
        case('&'):
            sTranslatedName += "&amp;";
            break;
        case('\n'):
            sTranslatedName += "\x0D\x0A";
            break;
        default:
            sTranslatedName += m_pData->m_sName.GetAt(nCharCount);
            break;
        };
    }

    // Convert from ANSI to UTF-8 encoding
    CHXString sConvertedName;
    ConvertANSIToUTF8(sTranslatedName, sConvertedName);

    return sConvertedName;
}

const char* CHXTAttribute::GetValue() const
{
    return m_pData->m_sValue;
}

CHXString CHXTAttribute::GetXMLSafeValueUTF8() const
{
    UINT32 nCharCount;
    CHXString sTranslatedValue;

    for(nCharCount = 0; nCharCount < m_pData->m_sValue.GetLength(); ++nCharCount)
    {
        switch(m_pData->m_sValue.GetAt(nCharCount))
        {
        case('&'):
            sTranslatedValue += "&amp;";
            break;
        case('\n'):
            sTranslatedValue += "\x0D\x0A";
            break;
        default:
            sTranslatedValue += m_pData->m_sValue.GetAt(nCharCount);
            break;
        };
    }

    // Convert from ANSI to UTF-8 encoding
    CHXString sConvertedName;
    ConvertANSIToUTF8(sTranslatedValue, sConvertedName);

    return sConvertedName;
}

void CHXTAttribute::SetName(const char* szName)
{
    m_pData->m_sName = szName;
}

void CHXTAttribute::SetNameUTF8(const char* szName)
{
    ConvertUTF8ToANSI(szName, m_pData->m_sName);
}

void CHXTAttribute::SetValue(const char* szValue)
{
    m_pData->m_sValue = szValue;
}

void CHXTAttribute::SetValueUTF8(const char* szValue)
{
    ConvertUTF8ToANSI(szValue, m_pData->m_sValue);
}

void CHXTAttribute::Release()
{
    HX_ASSERT(m_pData->ulRefCount > 1);

    --(m_pData->ulRefCount);
}

CHXTElement::CHXTElement() :
m_pData(new ElementData())
{
}

CHXTElement::CHXTElement(const CHXTElement& rhs) :
CHXTAttribute(rhs),
m_pData(rhs.m_pData)
{
    ++(m_pData->ulRefCount);
}

ElementData::ElementData() :
ulRefCount(1)
{
}

ElementData::~ElementData()
{
    CHXPtrArray::Iterator iter;
    for(iter = m_vAttributes.Begin(); iter != m_vAttributes.End(); ++iter)
    {
        CHXTAttribute* pAttr = (CHXTAttribute*)*iter;
        HX_DELETE(pAttr);
    }

    CHXPtrArray::Iterator ElementIter;
    for(ElementIter = m_vChildElements.Begin(); ElementIter != m_vChildElements.End(); ++ElementIter)
    {
        CHXTElement* pElem = (CHXTElement*)*ElementIter;
        HX_DELETE(pElem);
    }
}


ElementData& ElementData::operator=(const ElementData& rhs)
{
    //XXXLCM this needs to clear existing elements/attr
    HX_ASSERT(FALSE);
    CHXPtrArray::Iterator iter;
    ElementData& rhs_ = (ElementData&)rhs; // const_cast
    for(iter = rhs_.m_vAttributes.Begin(); iter != rhs_.m_vAttributes.End(); ++iter)
    {
        CHXTAttribute* pAttr = (CHXTAttribute*)*iter;
        CHXTAttribute* pAttrClone = new CHXTAttribute(*pAttr);
        m_vAttributes.Add(pAttrClone);
    }

    CHXPtrArray::Iterator ElementIter;
    for(ElementIter = rhs_.m_vChildElements.Begin(); ElementIter != rhs_.m_vChildElements.End(); ++ElementIter)
    {
        CHXTElement* pElem = (CHXTElement*)*iter;
        CHXTElement* pElemClone = new CHXTElement(*pElem);
        m_vChildElements.Add(pElemClone);
    }

    return *this;
}

CHXTElement::~CHXTElement()
{
    if( --(m_pData->ulRefCount) == 0 )
    {
        delete m_pData;
    }
}

CHXTElement& CHXTElement::operator=(const CHXTElement& rhs)
{
    CHXTAttribute::operator =(rhs);

    // Don't bother doing anything if the values are the same
    if( rhs.m_pData == m_pData )
        return *this;

    if( --(m_pData->ulRefCount) == 0 )
        delete m_pData;

    m_pData = rhs.m_pData;
    ++(m_pData->ulRefCount);

    return *this;
}

const CHXPtrArray& CHXTElement::GetChildren() const
{
    return m_pData->m_vChildElements;
}

const CHXTElement& CHXTElement::GetChild(UINT32 nChildIndex) const
{
    const CHXTElement* pElem = (const CHXTElement*)m_pData->m_vChildElements[nChildIndex];
    return *pElem;
}

const CHXPtrArray& CHXTElement::GetAttributes() const
{
    return m_pData->m_vAttributes;
}

const CHXTAttribute& CHXTElement::GetAttribute(UINT32 nAttributeIndex) const
{
    const CHXTAttribute* pAttr = (const CHXTAttribute*)m_pData->m_vAttributes[nAttributeIndex];
    return *pAttr;
}

HX_RESULT CHXTElement::AddAttribute(const char* szName, const char* szValue)
{
    CHXTAttribute* pAttr = new CHXTAttribute(szName, szValue);

    /* No copy on write for now
    if( m_pData->ulRefCount > 1 )
    {
    // We're trying to modify data owned by multiple Elements, so we need our own copy
    --(m_pData->ulRefCount);
    ElementData* pOldData = m_pData;
    m_pData = new ElementData();
    *m_pData = *pOldData;
    }
    */

    m_pData->m_vAttributes.Add(pAttr);
    return HXR_OK;
}

HX_RESULT CHXTElement::AddChild(const char* szName, const char* szValue, UINT32* pnIndex)
{
    CHXTElement* pNewElement = new CHXTElement();
    pNewElement->SetName(szName);
    if(szValue)
    {
        pNewElement->SetValue(szValue);
    }

    /* No copy on write for now
    if( m_pData->ulRefCount > 1 )
    {
    // We're trying to modify data owned by multiple Elements, so we need our own copy
    --(m_pData->ulRefCount);
    ElementData* pOldData = m_pData;
    m_pData = new ElementData();
    *m_pData = *pOldData;
    }
    */
    m_pData->m_vChildElements.Add(pNewElement);

    if(pnIndex)
    {
        *pnIndex = m_pData->m_vChildElements.GetSize() - 1;
    }

    return HXR_OK;
}

HX_RESULT CHXTElement::AddChildElement(CHXTElement* pNewElement, UINT32* pnIndex)
{
    /* No copy on write for now
    if( m_pData->ulRefCount > 1 )
    {
    // We're trying to modify data owned by multiple Elements, so we need our own copy
    --(m_pData->ulRefCount);
    ElementData* pOldData = m_pData;
    m_pData = new ElementData();
    *m_pData = *pOldData;
    }
    */
    m_pData->m_vChildElements.Add(pNewElement);
    if(pnIndex)
    {
        *pnIndex = m_pData->m_vChildElements.GetSize() - 1;
    }

    return HXR_OK;
}

UINT32 CHXTElement::GetChildrenCount() const
{
    return m_pData->m_vChildElements.GetSize();
}

HX_RESULT CHXTElement::RemoveChild(UINT32 nChildIndex)
{
    if( nChildIndex < (UINT32)m_pData->m_vChildElements.GetSize() )
    {
        CHXTElement* pElem = (CHXTElement*)(m_pData->m_vChildElements[nChildIndex]);
        m_pData->m_vChildElements.RemoveAt(nChildIndex);
        delete pElem;
        return HXR_OK;
    }

    return HXR_FAIL;
}

const CHXTAttribute* CHXTElement::FindAttribute(const char* szAttributeName) const
{
    CHXPtrArray::Iterator iterAttributes;

    for(iterAttributes = m_pData->m_vAttributes.Begin(); iterAttributes != m_pData->m_vAttributes.End(); ++iterAttributes)
    {
        CHXTAttribute* pAttr = (CHXTAttribute*)*iterAttributes;
        if( strcmp(szAttributeName, pAttr->GetName()) == 0 )
        {
            return pAttr;
        }
    }

    return NULL;
}

const CHXTElement* CHXTElement::FindChildElement(const char* szElementName) const
{
    CHXPtrArray::Iterator iterElements;

    for(iterElements = m_pData->m_vChildElements.Begin(); iterElements != m_pData->m_vChildElements.End(); ++iterElements)
    {
        CHXTElement* pElem = (CHXTElement*)*iterElements;

        if( strcmp(szElementName, pElem->GetName()) == 0 )
        {
            return pElem;
        }
    }

    return NULL;
}

void CHXTElement::Release()
{
    HX_ASSERT(m_pData->ulRefCount > 1);

    CHXTAttribute::Release();
    --(m_pData->ulRefCount);
}

HX_RESULT CHXTXmlParser::AddRootElement(CHXTElement* pNewElement)
{
    m_pRootElement = pNewElement;
    return HXR_OK;
}

const CHXTElement& CHXTXmlParser::GetRootElement() const
{
    return *m_pRootElement;
}

void CHXTXmlParser::startElement(void *userData, const char *name, const char **atts)
{
    CHXTElement* pNewElement = new CHXTElement();
    UINT32 nAttributeIndex = 0;

    // Set the name of the new element -- name is UTF8
    pNewElement->SetNameUTF8(name);

    // Now read and add all the attributes for the element
    while( atts[nAttributeIndex] != '\0' )
    {
        pNewElement->AddAttribute(atts[nAttributeIndex], atts[nAttributeIndex+1]);
        nAttributeIndex += 2;
    }

    CHXTXmlParser* pThis = (CHXTXmlParser*)userData;

    // Now add the element into the tree of objects.  If there is no element currently on the
    // stack then this must be the root element, so add it accordingly
    if (pThis->m_stackElement.IsEmpty())
    {        
        pThis->AddRootElement(pNewElement);
    }
    else
    {
        CHXTElement* pElem = (CHXTElement*)pThis->m_stackElement.TopOfStack();
        pElem->AddChildElement(pNewElement,NULL);
    }

    // Push the element on the stack so it's children can add themselves to it later
    pThis->m_stackElement.Push(pNewElement);
}


void CHXTXmlParser::endElement(void *userData, const char *name)
{
    CHXTXmlParser* pThis = (CHXTXmlParser*)userData;
    CHXTElement* pTopElement = (CHXTElement*)pThis->m_stackElement.Pop();
}

void CHXTXmlParser::characterDataHandler(void *userData, const char *szCharData, int nLength)
{
    //For some reason, Expat sometimes makes multiple calls to this function 
    // for any given element, so append the data

    char* pszTempBuffer	= new char[nLength + 1];
    strncpy(pszTempBuffer, szCharData, nLength);
    pszTempBuffer[nLength] = '\0';

    // Note -- szCharData is UTF8 while the attribute value is ANSI -- convert to ANSI and append
    CHXString sTranslated;
    ConvertUTF8ToANSI(pszTempBuffer, sTranslated);
    HX_VECTOR_DELETE(pszTempBuffer);

    CHXTXmlParser* pThis = (CHXTXmlParser*)userData;

    CHXTElement* pTopElement = (CHXTElement*)pThis->m_stackElement.TopOfStack();

    CHXString sAccumulatedValue = pTopElement->GetValue();
    sAccumulatedValue += sTranslated;
    pTopElement->SetValue(sAccumulatedValue);
}

CHXTXmlParser::CHXTXmlParser()
{
}

CHXTXmlParser::~CHXTXmlParser()
{
    HX_DELETE(m_pRootElement);

    while(!m_stackElement.IsEmpty())
    {
        CHXTElement* pTopElement = (CHXTElement*)m_stackElement.Pop();
        HX_DELETE(pTopElement);
    }
}

HX_RESULT CHXTXmlParser::ParseFile(const char* szFilename)
{
    HX_RESULT res = HXR_OK;

    // Validate params
    if (!szFilename)
    {
        HX_ASSERT(FALSE);
        return HXR_FAIL;
    }

    // Convert POSIX to HFS
    CHXString strInternalName;
    if (SUCCEEDED(res))
        res = CHXTDirConv::GetInternalPathname(szFilename, strInternalName);

    // Open XML file for reading
    FILE* pFile = NULL;    
    if (SUCCEEDED(res))
    {
        pFile = fopen(strInternalName, "rb");
        if (!pFile)
            res = HXR_INVALID_FILE;
    }

    // Read/parse the file
    if (SUCCEEDED(res))
    {
        // Figure out the size of the file
        fseek(pFile, 0, SEEK_END);
        UINT32 nFileSize = ftell(pFile);

        // Load the file contents into a character buffer
        char *pBuffer = new char[nFileSize + 1];
        fseek(pFile, 0, SEEK_SET);
        fread(pBuffer, nFileSize, 1, pFile);

        // Parse the contents of the file
        res = ParseBuffer(pBuffer, nFileSize);

        delete[] pBuffer;
    }

    // Close the file
    if (pFile)
        fclose(pFile); 

    return res;
}

HX_RESULT CHXTXmlParser::ParseBuffer(const char* szBuffer, UINT32 nBufferSize)
{
    while( szBuffer[nBufferSize - 1] == '\0' )
    {
        --nBufferSize;
    }
    XML_Parser parser = XML_ParserCreate(NULL);

    // Set the parser as data availiable to all Expat callbacks
    XML_SetUserData(parser, this);

    // Initialize the basic start and end tag handlers to the corresponding
    // methods in CHXTXmlParser
    XML_SetElementHandler(parser, &(CHXTXmlParser::startElement), &(CHXTXmlParser::endElement));
    XML_SetCharacterDataHandler(parser, &(CHXTXmlParser::characterDataHandler));

    if( !XML_Parse(parser, szBuffer, nBufferSize, 1) )
    {
        // If an error occurred while processing the XML, log the error, and store the 
        // error string and line number for later retreival
        m_sError = XML_ErrorString ( XML_GetErrorCode(parser));
        m_nErrorLineNum = XML_GetCurrentLineNumber(parser);

        // Pop any elements left on the stack at the time of the error
        while(!m_stackElement.IsEmpty())
        {
            CHXTElement* pTopElement = (CHXTElement*)m_stackElement.Pop();
            delete pTopElement;
        }

        // Destroy the parser
        XML_ParserFree(parser);

        return HXR_FAIL;
    }
    else
    {
        // Destroy the parser
        XML_ParserFree(parser);
        return HXR_OK;
    }
}

HX_RESULT CHXTXmlParser::WriteFile(const char* szFilename, HXBOOL bUseUnicode)
{
    HX_RESULT res = HXR_OK;

    // Validate params
    if (!szFilename)
    {
        HX_ASSERT(FALSE);
        return HXR_POINTER;
    }

    // Convert POSIX to HFS
    CHXString strInternalName;
    if (SUCCEEDED(res))
        res = CHXTDirConv::GetInternalPathname(szFilename, strInternalName);

    // Open XML file for writing
    FILE* pXmlFile = NULL;
    if (SUCCEEDED(res))
    {
        pXmlFile = fopen(strInternalName, "wb");
        if (!pXmlFile)
            res = HXR_FAIL;
    }

    // Write the XML file	    
    if (SUCCEEDED(res))
    {
        // Write encoding type
        if(bUseUnicode)
        {
            fwrite("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\x0D\x0A",40,1,pXmlFile);
        }
        else
        {
            fwrite("<?xml version=\"1.0\" encoding=\"US-ASCII\"?>\x0D\x0A",43,1,pXmlFile);
        }

        res = RecursiveFileWrite(pXmlFile, GetRootElement());
    }

    // Close the XML file
    if (pXmlFile)
        fclose(pXmlFile);

    return res;
}

HX_RESULT CHXTXmlParser::WriteBuffer(char* szBuffer, UINT32* pnBufferSize, HXBOOL bUseUnicode)
{
    HX_RESULT result = HXR_FAIL;

    if( szBuffer )
    {       
        if( bUseUnicode )
        {
            if( *pnBufferSize >= 38 )
            {
                strcpy(szBuffer, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
            }
            *pnBufferSize -= 38;
        }
        else
        {
            if( *pnBufferSize >= 41 )
            {
                strcpy(szBuffer, "<?xml version=\"1.0\" encoding=\"US-ASCII\"?>");
            }
            *pnBufferSize -= 41;
        }
        if( *pnBufferSize > 0 )
        {
            result = RecursiveBufferWrite(szBuffer, pnBufferSize, GetRootElement());
        }
        else
        {
            result = HXR_FAIL;
        }
    }
    else
    {
        *pnBufferSize = 0;

        if( bUseUnicode )
        {
            *pnBufferSize += 38;
        }
        else
        {
            *pnBufferSize += 41;
        }

        result = RecursiveBufferWrite(szBuffer, pnBufferSize, GetRootElement());
    }

    ++(*pnBufferSize);

    return result;
}


HX_RESULT CHXTXmlParser::RecursiveFileWrite(FILE* pXmlFile, const CHXTElement& element)
{
    HX_RESULT result = HXR_OK;
    CHXPtrArray::Iterator iterAttributes;
    CHXPtrArray::Iterator iterChildElements; 
    CHXPtrArray& childElements = (CHXPtrArray&)element.GetChildren(); // const_cast

#if defined (_SYMBIAN)
    HX_ASSERT(FALSE); // level logic needs work (static data)
#else
    static 
#endif
        UINT32 ulLevel = 0;

    HX_ASSERT(pXmlFile);

    if(pXmlFile)
    {
        // Beautify the output
        UINT32 nSpaceCount;
        for( nSpaceCount = 0; nSpaceCount < ulLevel; ++nSpaceCount)
        {
            fwrite("  ", 1, 2, pXmlFile);
        }

        // Write the begin element tag
        if( 1 != fwrite("<",1,1,pXmlFile) )
            result = HXR_FAIL;
        if( SUCCEEDED(result) )
        {
            CHXString sElementName = element.GetXMLSafeNameUTF8();
            if(sElementName.GetLength() != fwrite(sElementName, 1, sElementName.GetLength(), pXmlFile))
                result = HXR_FAIL;
        }

        // Write any attributes of this element
        CHXPtrArray& attributes = (CHXPtrArray&)element.GetAttributes(); // const_cast

        for(iterAttributes = attributes.Begin(); iterAttributes != attributes.End(); ++iterAttributes)
        {
            CHXTAttribute* pAttr = (CHXTAttribute*)*iterAttributes;
            if( SUCCEEDED(result) )
            {
                if( iterAttributes != attributes.Begin() )
                {
                    if( 2 != fwrite("\x0D\x0A", 1, 2, pXmlFile) )
                    {
                        result = HXR_FAIL;
                    }

                    // Beautify the output
                    UINT32 nSpaceCount;
                    for( nSpaceCount = 0; nSpaceCount < ulLevel + 1; ++nSpaceCount)
                    {
                        fwrite("  ", 1, 2, pXmlFile);
                    }
                }
                else
                {
                    fwrite(" ", 1, 1, pXmlFile);
                }

            }
            if( SUCCEEDED(result) )
            {
                CHXString sElementName = pAttr->GetXMLSafeNameUTF8();
                if( sElementName.GetLength() != fwrite(sElementName, 1, sElementName.GetLength(), pXmlFile) )
                    result = HXR_FAIL;
            }
            if( SUCCEEDED(result) )
            {
                if( 2 != fwrite("=\"", 1, 2, pXmlFile) )
                    result = HXR_FAIL;
            }
            if( SUCCEEDED(result) )
            {
                CHXString sElementValue = pAttr->GetXMLSafeValueUTF8();
                if( sElementValue.GetLength() != fwrite(sElementValue, 1, sElementValue.GetLength(), pXmlFile) )
                    result = HXR_FAIL;
            }
            if( SUCCEEDED(result) )
            {
                if( 1 != fwrite("\"", 1, 1, pXmlFile) )
                    result = HXR_FAIL;
            }
        }

        // Close the begin element tag
        if( SUCCEEDED(result) )
        {
            CHXString sElementValue = element.GetXMLSafeValueUTF8();
            if( !(childElements.IsEmpty()) || sElementValue.GetLength() > 0)
            {
                if( 1 != fwrite(">", 1, 1, pXmlFile) )
                    result = HXR_FAIL;
            }
            else
            {
                if( 1 != fwrite("/>", 1, 1, pXmlFile) )
                    result = HXR_FAIL;
            }
        }

        if( SUCCEEDED(result) )
        {
            // An XML element can either have a value, or child elements, not both
            if( !(childElements.IsEmpty()) )
            {
                if( 2 != fwrite("\x0D\x0A", 1, 2, pXmlFile) )
                    result = HXR_FAIL;

                ++ulLevel;
                for(iterChildElements = childElements.Begin(); iterChildElements != childElements.End(); ++iterChildElements)
                {
                    CHXTElement* pElem = (CHXTElement*)*iterChildElements;
                    result = RecursiveFileWrite(pXmlFile, *pElem);

                    if( FAILED(result) )
                        break;
                }
                --ulLevel;

                // Beautify the output
                UINT32 nSpaceCount;
                for( nSpaceCount = 0; nSpaceCount < ulLevel; ++nSpaceCount)
                {
                    fwrite("  ", 1, 2, pXmlFile);
                }

            }
            else
            {
                // Write out the element's value
                CHXString sElementValue = element.GetXMLSafeValueUTF8();
                if( sElementValue.GetLength() > 0 && sElementValue.GetLength() != fwrite(sElementValue, 1, sElementValue.GetLength(), pXmlFile) )
                    result = HXR_FAIL;
            }
        }

        // Write the end element tag
        if( SUCCEEDED(result) )
        {
            CHXString sElementValue = element.GetXMLSafeValueUTF8();
            if( !(childElements.IsEmpty()) || sElementValue.GetLength() > 0)
            {
                if( 2 != fwrite("</", 1, 2, pXmlFile) )
                    result = HXR_FAIL;
                if( SUCCEEDED(result) )
                {
                    CHXString sElementName = element.GetXMLSafeNameUTF8();
                    if(sElementName.GetLength() != fwrite(sElementName, 1, sElementName.GetLength(), pXmlFile))
                        result = HXR_FAIL;
                }
            }

            if( SUCCEEDED(result) )
            {
                if( 3 != fwrite(">\x0D\x0A",1,3,pXmlFile) )
                    result = HXR_FAIL;
            }           
        }
    }
    else
    {
        result = HXR_FAIL;
    }

    return result;
}

HX_RESULT CHXTXmlParser::RecursiveBufferWrite(char* szBuffer, UINT32* pnBufferLength, const CHXTElement& element)
{
    HX_RESULT result = HXR_OK;
    CHXPtrArray::Iterator iterAttributes;
    CHXPtrArray::Iterator iterChildElements;

    HX_ASSERT(pnBufferLength);

    if(pnBufferLength)
    {
        // Write the begin element tag
        result = WriteToBuffer(szBuffer, "<", pnBufferLength);

        if( SUCCEEDED(result) )
        {
            CHXString sElementName = element.GetXMLSafeNameUTF8();

            result = WriteToBuffer(szBuffer, sElementName, pnBufferLength);
        }


        // Write any attributes of this element
        CHXPtrArray& attributes = (CHXPtrArray&)element.GetAttributes(); // const_cast

        for(iterAttributes = attributes.Begin(); iterAttributes != attributes.End(); ++iterAttributes)
        {
            CHXTAttribute* pAttr = (CHXTAttribute*)*iterAttributes;
            if( SUCCEEDED(result) )
            {
                result = WriteToBuffer(szBuffer, " ", pnBufferLength);
            }
            if( SUCCEEDED(result) )
            {
                CHXString sAttributeName = pAttr->GetXMLSafeNameUTF8();
                result = WriteToBuffer(szBuffer, sAttributeName, pnBufferLength);
            }
            if( SUCCEEDED(result) )
            {
                result = WriteToBuffer(szBuffer, "=\"", pnBufferLength);
            }
            if( SUCCEEDED(result) )
            {
                CHXString sAttributeValue = pAttr->GetXMLSafeValueUTF8();
                result = WriteToBuffer(szBuffer, sAttributeValue, pnBufferLength);
            }
            if( SUCCEEDED(result) )
            {
                result = WriteToBuffer(szBuffer, "\"", pnBufferLength);
            }
        }

        // Close the begin element tag
        if( SUCCEEDED(result) )
        {
            result = WriteToBuffer(szBuffer, ">", pnBufferLength);
        }

        if( SUCCEEDED(result) )
        {
            CHXPtrArray& childElements = (CHXPtrArray&)element.GetChildren(); // const_cast

            // An XML element can either have a value, or child elements, not both
            if( !(childElements.IsEmpty()) )
            {
                for(iterChildElements = childElements.Begin(); iterChildElements != childElements.End(); ++iterChildElements)
                {
                    CHXTElement* pElem = (CHXTElement*)*iterChildElements;
                    result = RecursiveBufferWrite(szBuffer, pnBufferLength, *pElem);
                    if( FAILED(result) )
                        break;
                }
            }
            else
            {
                // Write out the element's value
                result = WriteToBuffer(szBuffer, (element.GetXMLSafeValueUTF8()), pnBufferLength);
            }
        }

        // Write the end element tag
        if( SUCCEEDED(result) )
        {
            result = WriteToBuffer(szBuffer, "</", pnBufferLength);
            if( SUCCEEDED(result) )
            {
                CHXString sElementName = element.GetXMLSafeNameUTF8();
                result = WriteToBuffer(szBuffer, sElementName, pnBufferLength);
            }
            if( SUCCEEDED(result) )
            {
                result = WriteToBuffer(szBuffer, ">", pnBufferLength);
            }           
        }
    }
    else
    {
        result = HXR_FAIL;
    }

    return result;
}

HX_RESULT CHXTXmlParser::WriteToBuffer(char* szBuffer, const char* szSource, UINT32* pnBufferLength)
{
    // Small helper function -- Checks if the buffer length is large enough to accomodate the
    // source string, and if so, writes it into the buffer and updates the buffer length
    // IMPORTANT -- this function assumes the buffer is already a null-terminated string

    if( NULL == szBuffer )
    {
        *pnBufferLength += strlen(szSource);
        return HXR_OK;
    }

    if( *pnBufferLength >= strlen(szSource) )
    {
        strcat(szBuffer, szSource);
        *pnBufferLength -= strlen(szSource);
        return HXR_OK;
    }
    else
    {
        *pnBufferLength = 0;
        return HXR_FAIL;
    }
}


void CHXTXmlParser::GetLastError(const char** pszError, UINT32* pnLineNum)
{
    *pszError = m_sError;
    *pnLineNum = m_nErrorLineNum;
}


