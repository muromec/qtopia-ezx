/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: gensdptest.cpp,v 1.6 2006/05/15 18:49:25 damann Exp $
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

#include "hlxclib/stdio.h"

#define  INITGUID
#include "hxplugn.h"
#include "hxtlogutil.h"
#include "ihxfgbuf.h"
#include "hxengin.h"
#include "hxcom.h"
#include "chxminiccf.h"
#include "ihxpckts.h"
#include "sdpmdparse.h"
#include "hxcore.h"
#include "hxpiids.h"

IHXBuffer* ReadSDPFile(IHXCommonClassFactory* pCCF, const char* pFilename)
{
    IHXBuffer* pBuf = 0;

    FILE* pFile = ::fopen(pFilename, "rb");

    if (pFile)
    {
	if (!fseek(pFile, 0, SEEK_END))
	{
	    long fileSize = ftell(pFile);
	    
	    if (HXR_OK == pCCF->CreateInstance(CLSID_IHXBuffer,(void**)&pBuf))
	    {
		HXBOOL bFailed = TRUE;

		if ((HXR_OK == pBuf->SetSize(fileSize)) &&
		    (!fseek(pFile, 0, SEEK_SET)) &&
		    (fread(pBuf->GetBuffer(), fileSize, 1, pFile) == 1))
		{
		    bFailed = FALSE;
		}

		if (bFailed)
		{
		    pBuf->Release();
		    pBuf = 0;
		}

	    }
	}
	::fclose(pFile);
    }

    return pBuf;
}

void PrintString(IHXBuffer* pBuf)
{
    const char* pCur = (const char*)pBuf->GetBuffer();
    const char* pEnd = pCur + pBuf->GetSize();
    
    for (; pCur < pEnd; pCur++)
    {
	if (*pCur == '\r')
	{
	    printf("\\r");
	}
	else if (*pCur == '\n')
	{
	    printf("\\n");
	}
	else if (*pCur == '"')
	{
	    printf("\\\"");
	}
	else if (*pCur == '\\')
	{
	    printf("\\\\");
	}
	else if(*pCur)
	{
	    printf("%c", *pCur);
	}
    }
}

ULONG32 CountULONG32(IHXValues* pHdr)
{
    ULONG32 ulCount = 0;
    
    const char* pName = 0;
    ULONG32 ulValue = 0;
    HX_RESULT res = pHdr->GetFirstPropertyULONG32(pName, ulValue);
    
    while (HXR_OK == res)
    {
	ulCount++;
	res = pHdr->GetNextPropertyULONG32(pName, ulValue);
    }

    return ulCount;
}

void OutputULONG32Tests(UINT16 ulStreamID, IHXValues* pHdr)
{
    const char* pName = 0;
    ULONG32 ulValue = 0;
    HX_RESULT res = pHdr->GetFirstPropertyULONG32(pName, ulValue);
    
    while (HXR_OK == res)
    {
	printf ("GetInt %u %s %lu\n", ulStreamID, pName, ulValue);

	res = pHdr->GetNextPropertyULONG32(pName, ulValue);
    }
}

ULONG32 CountStrings(IHXValues* pHdr)
{
    ULONG32 ulCount = 0;
    
    const char* pName = 0;
    IHXBuffer* pValue = 0;
    HX_RESULT res = pHdr->GetFirstPropertyCString(pName, pValue);
    
    while (HXR_OK == res)
    {
	ulCount++;
	
	HX_RELEASE(pValue);
	
	res = pHdr->GetNextPropertyCString(pName, pValue);
    }
    
    HX_RELEASE(pValue);

    return ulCount;
}

void OutputStrings(UINT16 ulStreamID, IHXValues* pHdr)
{
    const char* pName = 0;
    IHXBuffer* pValue = 0;
    HX_RESULT res = pHdr->GetFirstPropertyCString(pName, pValue);
    
    while (HXR_OK == res)
    {
	printf("GetString %u \"%s\" \"", ulStreamID, pName);
	PrintString(pValue);
	printf("\"\n");
	
	HX_RELEASE(pValue);
	
	res = pHdr->GetNextPropertyCString(pName, pValue);
    }
    
    HX_RELEASE(pValue);
}

ULONG32 CountBuffers(IHXValues* pHdr)
{
    ULONG32 ulCount = 0;
    
    const char* pName = 0;
    IHXBuffer* pValue = 0;
    HX_RESULT res = pHdr->GetFirstPropertyBuffer(pName, pValue);
    
    while (HXR_OK == res)
    {
	ulCount++;
	
	HX_RELEASE(pValue);
	
	res = pHdr->GetNextPropertyBuffer(pName, pValue);
    }
    
    HX_RELEASE(pValue);

    return ulCount;
}

void OutputBuffers(UINT16 ulStreamID, IHXValues* pHdr)
{
    const char* pName = 0;
    IHXBuffer* pValue = 0;
    HX_RESULT res = pHdr->GetFirstPropertyBuffer(pName, pValue);
    
    while (HXR_OK == res)
    {
	printf("GetBuffer %u \"%s\" \"", ulStreamID, pName);

	UCHAR* pCur = pValue->GetBuffer();
	UCHAR* pEnd = pCur + pValue->GetSize();

	static const char z_hexChars[] = "0123456789abcdef";

	for (;pCur < pEnd; pCur++)
	{
	    printf("%c%c", z_hexChars[*pCur >> 4], z_hexChars[*pCur & 0xf]);
	}

	printf("\"\n");
	
	HX_RELEASE(pValue);
	
	res = pHdr->GetNextPropertyBuffer(pName, pValue);
    }
    
    HX_RELEASE(pValue);
}



HX_RESULT Parse(IUnknown* pContext, ULONG32 ulVersion, IHXBuffer* pSDP)
{
    SDPMediaDescParser parser(ulVersion);

    HX_RESULT res = parser.Init(pContext);

    if (HXR_OK == res)
    {
	UINT16 nValues = 0;
	IHXValues** ppValues = 0;

	res = parser.Parse(pSDP, nValues, ppValues);

	
	printf("Init %lu\n", ulVersion);
	
	printf("Parse 0x%08x %lu \"", res, nValues);

	PrintString(pSDP);

	printf ("\"\n");

	for (UINT16 i = 0; i < nValues; i++)
	{
	    IHXValues* pHdr = ppValues[i];

	    printf ("IntCount %u %lu\n", i, CountULONG32(pHdr));
	    OutputULONG32Tests(i, pHdr);

	    printf ("StringCount %u %lu\n", i, CountStrings(pHdr));
	    OutputStrings(i, pHdr);

	    printf ("BufferCount %u %lu\n", i, CountBuffers(pHdr));
	    OutputBuffers(i, pHdr);
	}
    }

    return res;
}

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
	fprintf(stderr, "Usage : %s <sdp version> <sdp file>\n", argv[0]);
    }
    else
    {
	IUnknown* pContext = new CHXMiniCCF();
	
	if (pContext)
	{
	    pContext->AddRef();
	    
	    IHXCommonClassFactory* pCCF = 0;

	    if (HXR_OK == pContext->QueryInterface(IID_IHXCommonClassFactory,
						   (void**)&pCCF))
	    {
		IHXBuffer* pSDP = ReadSDPFile(pCCF, argv[2]);

		if (pSDP)
		{
		    if (HXR_OK != Parse(pContext, strtoul(argv[1], 0, 10), 
					pSDP))
		    {
			fprintf(stderr, "Failed to parse SDP\n", argv[1]);
		    }

		    pSDP->Release();
		    pSDP = 0;
		}
		else
		{
		    fprintf(stderr, "Failed to read SDP file '%s'\n", argv[2]);
		}

		pCCF->Release();
		pCCF = 0;
	    }
	    	    
	    pContext->Release();
	    pContext = 0;
	}
	
    }

    return 0;
}
