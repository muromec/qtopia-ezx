/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxpluginarchive.cpp,v 1.12 2007/04/05 05:41:29 gahluwalia Exp $
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
#include "hxccf.h" // IHXCommonClassFactory
#include "ihxpckts.h" // IHXBuffer
#include "hlxclib/fcntl.h"
#include "chxdataf.h" // CHXDataFile
#include "hxtlogutil.h"
#include "hxheap.h"
#include "rtsputil.h"
#include "pckunpck.h"

#ifdef _DEBUG
#undef HX_THIS_FILE
static const char HX_THIS_FILE[] = __FILE__;
#endif

#include "hxpluginarchive.h"

#if defined(HELIX_CONFIG_PLUGIN_ARCHIVE_BUFFER)
#include "ihxfgbuf.h"
#endif

//
// special chars; these need escaping if in value text
//
const char TERM_CHAR = ';';     // denotes end of value for most objects
const char ESC_CHAR = '\\';     // escape char

HXPluginArchiveReader::HXPluginArchiveReader()
: m_idxFile(0)
{
}

//
// Open
//
HX_RESULT HXPluginArchiveReader::Open(IUnknown* pContext, const char* pszFile)
{
    HXLOGL3(HXLOG_CORE, "HXPluginArchiveReader::Open(): file = '%s'", pszFile);
    
    HX_RESULT hr = SetFile(pContext, CHXDataFile::Construct(pContext), TRUE);
    if (SUCCEEDED(hr))
    {
        HX_ASSERT(m_pFile);

	hr = m_pFile->Open(pszFile, O_RDONLY, TRUE /*text*/);
        if (SUCCEEDED(hr))
        {
            LoadFile();
        }
    }

    return hr;
}

HX_RESULT HXPluginArchiveReader::OpenWithFile(IUnknown* pContext, CHXDataFile* pFile)
{
    HXLOGL3(HXLOG_CORE, "HXPluginArchiveReader::OpenWithFile(): file = '0x%x'\n", pFile);
    
    HX_RESULT hr = SetFile(pContext, pFile, FALSE);
    if (SUCCEEDED(hr))
    {
        HX_ASSERT(m_pFile);
        if (SUCCEEDED(hr))
        {
            LoadFile();
        }
    }

    return hr;
}

#if defined(HELIX_CONFIG_PLUGIN_ARCHIVE_BUFFER)
HX_RESULT HXPluginArchiveReader::SetBuffer(IUnknown* pContext, IHXFragmentedBuffer* pFragBuf)
{
    HX_RESULT res = HXPluginArchiveBase::SetBuffer(pContext, pFragBuf);
    if (SUCCEEDED(res))
    {
        LoadFile();
    }
    return res;
}
#endif

void HXPluginArchiveReader::Close()
{
    CloseFile();
}


void HXPluginArchiveReader::LoadFile()
{
    #define BUF_SZ	0x0400	// read file in 1k chunks

    CHXString strBuf;

    char* buf = strBuf.GetBuffer(BUF_SZ + 1);
    if( buf)
    {
        for(;;)
        {
            INT32 cch = _Read(buf, BUF_SZ);
	    if (cch <= 0)
            {
                // end of file
	        break;
            }
        
            buf[cch] = '\0';
            m_strFile += buf;
        }
    }

    m_strFile.TrimRight(); // ensure no trailing space so 'AtEnd()' is easier
    m_idxFile = 0;
   
}


void HXPluginArchiveReader::GetNextToken(CHXString& str, char chTerm)
{
    UINT32 cchFile = m_strFile.GetLength();

    HX_ASSERT(chTerm != ESC_CHAR);

    while( m_idxFile < cchFile)
    {
        char ch = m_strFile[m_idxFile];

        if(chTerm == m_strFile[m_idxFile])
        {
            // found end of token; we're done
            ++m_idxFile;
            break;
        }

        if( ESC_CHAR == ch )
        {
            // skip escape to read in escaped char
            ++m_idxFile;
            if(m_idxFile == cchFile)
            {
                // unexpected: EOF in middle of escape sequence
                HX_ASSERT(false);
                break;
            }
            ch = m_strFile[m_idxFile];
        }
        str += ch;
        ++m_idxFile;
    }
    str.TrimLeft();
    str.TrimRight();
}



//
// read name;val;name;val;;
//
//
bool HXPluginArchiveReader::Read(IHXValues*& pval)
{
    HX_ASSERT(!AtEnd());
    UINT32 cchFile = m_strFile.GetLength();

    HX_ASSERT(m_pFactory);

    m_pFactory->CreateInstance(CLSID_IHXValues,(void**)&pval);
    if(!pval)
    {
        return false;
    }


    CHXString strName;
    CHXString strVal;


    // collect name-value pairs until we reach closing double TERM_CHAR
    for(;;)
    {
        strName = "";
        GetNextToken(strName, TERM_CHAR);

        if(strName.IsEmpty())
        {
            // end of list (ends with empty token, i.e., double TERM_CHAR
            return true;
        }

        if(m_idxFile == cchFile)
        {
            // corrupt file; end of file after name
            HX_ASSERT(false); 
            return false;
        }

        
        strVal = "";
        GetNextToken(strVal, TERM_CHAR);

        if(m_idxFile == cchFile)
        {
            // corrupt file
            HX_ASSERT(false); 
            return false;
        }

        if(strVal.IsEmpty())
        {
            // corrupt file
            HX_ASSERT(false);
            return false;
        }
 
        char type = strVal[0];
        CHXString strNakedVal = strVal.Mid(1);
        switch(type)
        {
            case 'N':
                {
                    int val = atoi(strNakedVal);
                    pval->SetPropertyULONG32(strName, ULONG32(val));
                }
                break;
            case 'B':
                {
		       BYTE* pOutBuf = new BYTE[strNakedVal.GetLength()];
			if(pOutBuf)
			    {
		                UINT len =  BinFrom64(strNakedVal, strNakedVal.GetLength(), pOutBuf);			
                              IHXBuffer* pbuff = NULL;
		                CreateAndSetBufferCCF(pbuff,pOutBuf,len,m_pFactory);
                              if( pbuff)
                              {
                                   pval->SetPropertyBuffer(strName, pbuff);
                                   HX_RELEASE(pbuff);
                              }
		                delete[] pOutBuf;
			    }
                }
                break;
            case 'S':
                {
                    IHXBuffer* pbuff = HXBufferUtil::CreateBuffer(m_pFactory, strNakedVal);
                    if( pbuff)
                    {
                        pval->SetPropertyCString(strName, pbuff);
                        HX_RELEASE(pbuff);
                    }
                }
                break;
            default:
                HX_ASSERT(false);
                return false;
        }

    }

    HX_ASSERT(false);
    return false;
}

void HXPluginArchiveReader::Read(bool& b)
{
    HX_ASSERT(!AtEnd());
    UINT32 val = 0;
    Read(val);
    b = (val ? true : false);

}

void HXPluginArchiveReader::Read(UINT16& val)
{
    HX_ASSERT(!AtEnd());
    UINT32 v;
    Read(v);
    HX_ASSERT(v <= 0xffff);
    val = UINT16(v);
}


void HXPluginArchiveReader::Read(UINT32& val)
{
    HX_ASSERT(!AtEnd());
    CHXString str;
    GetNextToken(str, TERM_CHAR);
    val = atoi(str);
}

void HXPluginArchiveReader::Read(CHXString& str)
{
    HX_ASSERT(!AtEnd());
    GetNextToken(str, TERM_CHAR);
}


void HXPluginArchiveReader::Read(IHXBuffer*& pBuff)
{
    HX_ASSERT(!AtEnd());
    CHXString str;
    GetNextToken(str, TERM_CHAR);
    pBuff = HXBufferUtil::CreateBuffer(m_pFactory, str);
}









//
// HXPluginArchiveWriter
//

HXPluginArchiveWriter::HXPluginArchiveWriter()
: m_bAtLeastOneVal(false)
{
}

//
// Open/create archive, wiping out existing if necessary
//
HX_RESULT HXPluginArchiveWriter::Open(IUnknown* pContext, const char* pszFile)
{
    HXLOGL3(HXLOG_CORE, "HXPluginArchiveWriter::Open(): file = '%s'\n", pszFile);

    HX_RESULT hr = SetFile(pContext, CHXDataFile::Construct(pContext), TRUE);
    if (SUCCEEDED(hr))
    {
        HX_ASSERT(m_pFile);
        hr = m_pFile->Open(pszFile, O_WRONLY | O_CREAT | O_TRUNC, TRUE /*text*/);
    }

    return hr;
}


HX_RESULT HXPluginArchiveWriter::OpenWithFile(IUnknown* pContext, CHXDataFile* pFile)
{
    HXLOGL3(HXLOG_CORE, "HXPluginArchiveWriter::OpenWithFile(): file = '0x%x'\n", pFile);

    HX_RESULT hr = SetFile(pContext, pFile, FALSE);
    if (SUCCEEDED(hr))
    {
        HX_ASSERT(m_pFile);
    }

    return hr;
}


void HXPluginArchiveWriter::Close()
{
    CloseFile();
}




void HXPluginArchiveWriter::EscapeValue(const CHXString& str, CHXString& strOut)
{
    //
    // Escape special characters used for escaping and field termination
    //
    if( -1 == str.Find(ESC_CHAR) && -1 == str.Find(TERM_CHAR) )
    {
        // this may avoid copy if string class is optimized
        strOut = str;
    }
    else
    {
        INT32 cch = str.GetLength();
        for(INT32 idx = 0; idx < cch; ++idx)
        {
            char ch = str[idx];
            if(ch == ESC_CHAR || ch == TERM_CHAR)
            {
                strOut += ESC_CHAR;
            }

            strOut += ch;
        }
    }
}


// write line break (for sake of human consumption of file contents)
void HXPluginArchiveWriter::Break()
{
    const char* const BREAK_STRING = "\n";
    const UINT32 CCH_BREAK_STRING = 1;

    _Write(BREAK_STRING, CCH_BREAK_STRING);
}

void HXPluginArchiveWriter::Write(const char* psz)
{
    CHXString strEscaped;
    EscapeValue(psz, strEscaped);

//    HX_ASSERT(m_pFile);
    strEscaped += TERM_CHAR;
    _Write(strEscaped, strEscaped.GetLength());  
}

void HXPluginArchiveWriter::Write(IHXBuffer* pBuffer)
{
    HX_ASSERT(pBuffer);
    const char* psz = (const char*)pBuffer->GetBuffer();
  
    Write(psz);
}

void HXPluginArchiveWriter::Write(bool b)
{
    UINT32 val = (b ? 1 : 0);
    Write(val);
}

void HXPluginArchiveWriter::Write(UINT32 val)
{
    CHXString str;
    str.Format("%lu", val);
    Write(str);
}

void HXPluginArchiveWriter::Write(UINT16 val)
{
    Write(UINT32(val));
}


void HXPluginArchiveWriter::Write(IHXValues* pval)
{
    HX_ASSERT(pval);

    CHXString str;
    ULONG32 ulValue;
    const char* pszKey = NULL;

    // Dump ULONG32 values
    HX_RESULT hr = pval->GetFirstPropertyULONG32( pszKey, ulValue );
    if( SUCCEEDED(hr) )
    {
	do
	{
            //key
            Write(pszKey);

            //value
	    str.Format("N%d",  ulValue);
            Write(str);
	   
	    hr = pval->GetNextPropertyULONG32( pszKey, ulValue);
	}
	while( SUCCEEDED(hr) );
    }

    // Dump IHXBuffer values
    IHXBuffer* pbuff;
    hr = pval->GetFirstPropertyBuffer( pszKey, pbuff );
    if( SUCCEEDED(hr) )
    {
	do
	{	
            //key
            Write(pszKey);
            char* pbuffer = new char[ pbuff->GetSize()*4+1];
	     if(pbuffer)
	         {
                   UINT len = BinTo64(pbuff->GetBuffer(), pbuff->GetSize(), pbuffer);
	             pbuffer[len] = 0;
                    //value
	             str.Format("B%s",  pbuffer, len);
                    Write(str);
	             delete[] pbuffer;
	     	  }
	     HX_RELEASE(pbuff);
	     hr = pval->GetNextPropertyBuffer( pszKey,pbuff);
	}
	while( SUCCEEDED(hr) );
    }

    // Dump CString values
    hr = pval->GetFirstPropertyCString( pszKey, pbuff );
    if( SUCCEEDED(hr) )
    {
	do
	{
            //key
            Write(pszKey);

            //value
	    str.Format("S%.*s",  pbuff->GetSize(), pbuff->GetBuffer() );
            Write(str);

	    HX_RELEASE(pbuff);
	    hr = pval->GetNextPropertyCString( pszKey, pbuff);
	}
	while( SUCCEEDED(hr) );
    }

    // empty value terminates list
    Write("");	
}


//XXXLCM util
IHXBuffer* HXBufferUtil::CreateBuffer(IHXCommonClassFactory* pFact, const char* psz)
{
    if(!psz)
    {
        psz = "";
    }
    IHXBuffer* pbuff = 0;
    HX_RESULT hr = pFact->CreateInstance(CLSID_IHXBuffer, (void**)&pbuff);
    if( SUCCEEDED(hr) )
    {
        hr = pbuff->Set( (BYTE*)psz, strlen(psz) + 1);
        if( FAILED(hr) )
        {
            HX_RELEASE(pbuff);
        }
    }
    return pbuff;
}



HXPluginArchiveBase::HXPluginArchiveBase()
    : m_pFactory(NULL)
    , m_pFile(NULL)
    , m_bOwnFile(FALSE)
#if defined(HELIX_CONFIG_PLUGIN_ARCHIVE_BUFFER)
    , m_pFragBuf(NULL)
    , m_uReadPos(0)
#endif
{
}


HXPluginArchiveBase::~HXPluginArchiveBase()
{
    CloseFile(); 
    HX_RELEASE(m_pFactory);
}


HX_RESULT
HXPluginArchiveBase::SetContext(IUnknown* pContext)
{
    HXLOGL3(HXLOG_CORE, "HXPluginArchiveBase::SetContext(): context = '0x%x'\n", pContext);

    HX_RESULT hr = HXR_INVALID_PARAMETER;
    if (pContext)
    {
        HX_RELEASE(m_pFactory);
        hr = pContext->QueryInterface(IID_IHXCommonClassFactory, (void**)&m_pFactory);
    }
    return hr;
}


HX_RESULT
HXPluginArchiveBase::SetFile(IUnknown* pContext, CHXDataFile* pDataFile, bool bOwnFile)
{
    HXLOGL3(HXLOG_CORE, "HXPluginArchiveBase::SetFile(): file = '0x%x'\n", pDataFile);

    HX_RESULT hr = SetContext(pContext);
    if (SUCCEEDED(hr))
    {
        CloseFile();
        if (pDataFile)
        {
            m_pFile = pDataFile;
            m_bOwnFile = bOwnFile;
        }
    }
    return hr;
}


#if defined(HELIX_CONFIG_PLUGIN_ARCHIVE_BUFFER)
HX_RESULT
HXPluginArchiveBase::SetBuffer(IUnknown* pContext, IHXFragmentedBuffer* pFragBuf)
{
    HXLOGL3(HXLOG_CORE, "HXPluginArchiveBase::SetBuffer(): buf = '0x%x'\n", pFragBuf);

    HX_RESULT hr = SetContext(pContext);
    if (SUCCEEDED(hr))
    {
        CloseFile();
        if (pFragBuf)
        {
            m_pFragBuf = pFragBuf;
            m_pFragBuf->AddRef();
            hr = HXR_OK;
        }
    }
    return hr;
}
#endif


void
HXPluginArchiveBase::CloseFile()
{
    if (m_bOwnFile)
    {
        HX_DELETE(m_pFile);
        m_bOwnFile = FALSE;
    }
#if defined(HELIX_CONFIG_PLUGIN_ARCHIVE_BUFFER)
    else if (m_pFragBuf)
    {
        HX_RELEASE(m_pFragBuf);
        m_uReadPos = 0;
    }
#endif
    else
    {
        m_pFile = NULL;
    }
}


INT32
HXPluginArchiveBase::_Write(const char* pBuf, UINT32 uSize)
{
    HX_ASSERT(pBuf);
    HX_ASSERT(uSize);
    if (m_pFile)
    {
        return (INT32)m_pFile->Write(pBuf, uSize);
    }
#if defined(HELIX_CONFIG_PLUGIN_ARCHIVE_BUFFER)
    else if (m_pFragBuf)
    {
        IHXBuffer* pBuffer = CreateBuffer((const UCHAR*)pBuf, uSize);
        HX_ASSERT(pBuffer);
        if (pBuffer)
        {
            HX_ASSERT(uSize == pBuffer->GetSize());
            HX_RESULT res = m_pFragBuf->Append(pBuffer, 0, uSize);
            HX_RELEASE(pBuffer);
            if (SUCCEEDED(res))
            {
                return (INT32)uSize;
            }
        }
    }
#endif
    else
    {
        HX_ASSERT(FALSE);
    }
    return -1;
}


INT32
HXPluginArchiveBase::_Read(char* pBuf, UINT32 uSize)
{
    HX_ASSERT(pBuf);
    HX_ASSERT(uSize);
    if (m_pFile)
    {
        return m_pFile->Read(pBuf, uSize);
    }
#if defined(HELIX_CONFIG_PLUGIN_ARCHIVE_BUFFER)
    else if (m_pFragBuf)
    {
        // XXXNH: the current implementation of CHXFragmentedBuffer will
        // return a buffer larger than the remaining amount to be read,
        // so we ensure that we don't request too much data here.
        UINT32 uFragSize = 0;
        IHXBuffer* pHXBuf = NULL;
        HX_VERIFY(SUCCEEDED(m_pFragBuf->QueryInterface(IID_IHXBuffer, (void**)&pHXBuf)));
        uFragSize = pHXBuf->GetSize();
        HX_RELEASE(pHXBuf);
        if (m_uReadPos + uSize > uFragSize)
        {
            uSize = uFragSize - m_uReadPos;
        }
        // save ourselves the trouble if the requested size is 0
        if (uSize > 0)
        {
            // get the data from the frag buffer
            UCHAR* pTmpBuf = NULL;
            UINT32 uTmpSize = 0;
            HX_RESULT res = m_pFragBuf->Get(m_uReadPos, uSize, pTmpBuf, uTmpSize);
            if (SUCCEEDED(res))
            {
                HX_ASSERT(uTmpSize);
                // XXXNH: it'd be nice if we could get the frag buf to write
                // directly to our buffer...
                memcpy(pBuf, pTmpBuf, HX_MIN(uSize, uTmpSize));
                // XXXNH: we'll probably never have a 4gb buffer...
                HX_ASSERT(((UINT32)m_uReadPos + uTmpSize) > m_uReadPos);
                m_uReadPos += uTmpSize;
                return (INT32)uTmpSize;
            }
        }
        else
        {
            return 0;
        }
    }
#endif
    else
    {
        HX_ASSERT(FALSE);
    }
    return -1;
}


IHXBuffer*
HXPluginArchiveBase::CreateBuffer(const UCHAR* pData, UINT32 uSize)
{
    IHXBuffer* pBuffer = NULL;
    if (m_pFactory)
    {
        IUnknown* pUnk;
        HX_RESULT res = m_pFactory->CreateInstance(CLSID_IHXBuffer,
                                                   (void**)&pUnk);
        if (SUCCEEDED(res))
        {
            res = pUnk->QueryInterface(IID_IHXBuffer, (void**)&pBuffer);
            HX_RELEASE(pUnk);
            if (SUCCEEDED(res))
            {
                res = pBuffer->Set(pData, uSize);
            }
        }
        HX_ASSERT(SUCCEEDED(res));
    }
    return pBuffer;
}
