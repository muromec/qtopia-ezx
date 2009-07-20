/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxdatasource_ccontent.cpp,v 1.5 2007/08/15 19:00:15 yuryrp Exp $
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

#include "hxcom.h"
#include "hxtypes.h"
#include "hxresult.h"
#include "hxdatasource_ccontent.h"
#include "hxassert.h"
#include "hxstring.h"
#include "hxtlogutil.h"
#include "hlxclib/sys/stat.h"
#include "hlxclib/time.h"
#include "symbian_string.h"


#define MAX_DCF_HEADER_LENGTH 256

CHXDataSourceCContent::CHXDataSourceCContent(HXBOOL bPeek)
    :m_pContent(NULL)
    ,m_pCData(NULL)
{
    if (bPeek)
    {
        m_intent = EPeek;
    }
    else
    {
        m_intent = EPlay;
    }
}

CHXDataSourceCContent* CHXDataSourceCContent::Create(
                        char *pFileName,
                        HXBOOL bPeek)
{
    
    TInt err = KErrNone;

    CHXDataSourceCContent *self = new CHXDataSourceCContent(bPeek);

    if (self)
    {
        TRAP(err, self->ConstructL(pFileName));
        if (err != KErrNone)
        {
            HX_DELETE(self);
        }
    }

    return self;
}

CHXDataSourceCContent* CHXDataSourceCContent::Create(
                        RFile* pFileHandle, 
                        HXBOOL bPeek)
{
    
    HX_RESULT hxr;
    TInt err = KErrNone;


    CHXDataSourceCContent *self = new CHXDataSourceCContent(bPeek);

    if (self)
    {
        TRAP(err, self->ConstructL(pFileHandle));
        if (err != KErrNone)
        {
            HX_DELETE(self);
        }
    }

    return self;
}

void CHXDataSourceCContent::ConstructL(char *pFileName)
{
    TFileName fileName;
    TPtrC8 desFileName((TUint8 *)pFileName, strlen(pFileName));
    fileName.Copy(desFileName);

    m_pContent = CContent::NewL(fileName); 
    m_pCData   = m_pContent->OpenContentL(m_intent, EContentShareReadOnly);
}

void CHXDataSourceCContent::ConstructL(RFile *pFileHandle)
{
    m_pContent = CContent::NewL((RFile&)*pFileHandle); 
    m_pCData   = m_pContent->OpenContentL(m_intent, EContentShareReadOnly);
}

STDMETHODIMP CHXDataSourceCContent::Close()
{
    return HXR_OK;
}

CHXDataSourceCContent::~CHXDataSourceCContent()
{
    HX_DELETE(m_pContent);
    HX_DELETE(m_pCData);
}

UINT32 CHXDataSourceCContent::Read(
            void *pBuf, ULONG32 size, ULONG32 count)
{
    TInt    requestSize = size*count;
    TInt    err;

    TPtr8   ptr8((TUint8 *)pBuf, 0, requestSize);    

    err = m_pCData->Read(ptr8);

    UINT32 bytesRead;
    if (err == KErrNone)
    {
        // return the number of bytes read
        bytesRead = ptr8.Length();
        m_ulPosition += bytesRead;
    }
    else
    {
        bytesRead = 0;
        SetLastError(HXR_READ_ERROR);
        HXLOGL1(HXLOG_FILE, "CHXDataSourceCContent::Read error=%d", err);
    }


    return bytesRead;
}

UINT32 CHXDataSourceCContent::Write(
            void *pBuf, ULONG32 size, ULONG32 count)
{
    SetLastError(HXR_NOTIMPL);
    HX_ASSERT("CHXDataSourceCContent::Write" == NULL);
    return 0;
}


STDMETHODIMP CHXDataSourceCContent::Seek(
            UINT32 offset, INT32 fromWhere)
{
    HX_RESULT hxr = HXR_OK;

    TInt aPosition = offset;

    if(fromWhere == SEEK_SET)
    {
        if (m_pCData->Seek(ESeekStart, aPosition) == KErrNone)
        {
            m_ulPosition = offset;
        }
        else
        {
            hxr = HXR_FAIL;
        }
    }
    else if(fromWhere == SEEK_CUR)
    {
        if (m_pCData->Seek(ESeekCurrent, aPosition) == KErrNone)
        {
            m_ulPosition += offset;
        }
        else
        {
            hxr = HXR_FAIL;
        }
    }
    else if(fromWhere == SEEK_END)
    {
        hxr = HXR_NOT_SUPPORTED;
    }
    else
    {
        hxr = HXR_INVALID_PARAMETER;
    }

    return hxr;
}

STDMETHODIMP CHXDataSourceCContent::GetSize(UINT32 &ulSize)
{
    HX_RESULT hxr;
    TInt dataSize;
    TRAPD(err, m_pCData->DataSizeL(dataSize))

    if (err == KErrNone)
    {
        ulSize = (UINT32 )dataSize;
        hxr = HXR_OK;
    }
    else
    {
        hxr = HXR_FAIL;
        SetLastError(hxr);
    }
    return hxr;
}

STDMETHODIMP CHXDataSourceCContent::GetStringAttribute(UINT32 ulAttribute, 
                                REF(IHXBuffer*)  pBuffer)
{
    HX_RESULT hxr;

    TBuf<MAX_DCF_HEADER_LENGTH> header;

    TInt found = m_pCData->GetStringAttribute((TInt) ulAttribute, header);

    if(found == KErrNone && header.Length() > 0)
    {
        CHXString sValue;
        CHXSymbianString::DesToString(header, sValue);
        pBuffer = new CHXBuffer();
        if (pBuffer)
        {
            HX_ADDREF(pBuffer);
            hxr = pBuffer->Set( (const UCHAR *)(const char*)sValue, sValue.GetLength());
            if (hxr != HXR_OK)
            {
                HX_RELEASE(pBuffer);
            } 
        }
        else
        {
            hxr = HXR_OUTOFMEMORY;
        }
    }
    else
    {
        hxr = HXR_FAIL; 
    }

    return hxr;
}

STDMETHODIMP CHXDataSourceCContent::GetAttribute(INT32 ulAttribute, 
                                INT32 &ulValue)

{
    TInt found = m_pCData->GetAttribute((TInt) ulAttribute, (TInt &) ulValue);

    if (found == KErrNone)
    {
        return HXR_OK;
    }
    else
    {
        return HXR_FAIL;
    }
}

