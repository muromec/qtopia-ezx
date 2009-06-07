/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxsymbiansockhlp.cpp,v 1.5 2006/12/06 10:15:07 gahluwalia Exp $
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

#include "hxsymbiansockhlp.h"
#include "hlxclib/string.h"
#include "hxtbuf.h"

HX_RESULT HXSymbianSocketHelper::ResizeOrCreate(IHXCommonClassFactory* pCCF,
                                                UINT32 ulSize,
                                                REF(IHXBuffer*) pBuffer)
{
    HX_RESULT res = HXR_FAILED;

    if (pBuffer)
    {
        // We have a buffer so just resize it
        res = pBuffer->SetSize(ulSize);
    }
    else
    {
        // We don't have a buffer so create one
        res = CreateBuffer(pCCF, ulSize, pBuffer);
    }

    return res;
}

HX_RESULT HXSymbianSocketHelper::CopyOrTransfer(IHXCommonClassFactory* pCCF,
                                                UINT32 ulSize,
                                                REF(IHXBuffer*) pSrcBuf,
                                                REF(IHXBuffer*) pDestBuf)
{
    HX_RESULT res = HXR_FAILED;
    if (HXR_OK == CreateBuffer(pCCF, ulSize, pDestBuf))
    {
        // Copy the data into the new buffer
        memcpy(pDestBuf->GetBuffer(), pSrcBuf->GetBuffer(), ulSize);
	IHXTimeStampedBuffer* pTSBuffer = 0;
	UINT32 timeStamp = 0;
	if(HXR_OK == pSrcBuf->QueryInterface(IID_IHXTimeStampedBuffer, (void **)&pTSBuffer))
	{
		timeStamp = pTSBuffer->GetTimeStamp();
		HX_RELEASE(pTSBuffer);
	}       	
	if(HXR_OK == pDestBuf->QueryInterface(IID_IHXTimeStampedBuffer, (void **)&pTSBuffer))
	{
		pTSBuffer->SetTimeStamp(timeStamp);
		HX_RELEASE(pTSBuffer);
	}		
        res = HXR_OK;
    }
    else if (pSrcBuf)
    {
        // We couldn't create a new buffer. Use the buffer we have
        
        // Transfer ownership
        pDestBuf = pSrcBuf;
        pSrcBuf = NULL;

        if (pDestBuf)
        {
            // Set the size
            pDestBuf->SetSize(ulSize);
        }

        res = HXR_OK;
    }

    return res;
}

HX_RESULT HXSymbianSocketHelper::CreateBuffer(IHXCommonClassFactory* pCCF,
                                              UINT32 ulSize,
                                              REF(IHXBuffer*) pBuffer)
{
    HX_RESULT res = HXR_FAILED;

    if (pCCF)
    {
        IHXTimeStampedBuffer* pTSBuffer = NULL;
	 res = pCCF->CreateInstance(IID_IHXTimeStampedBuffer, (void**)&pTSBuffer);

        if (HXR_OK == res)
        {
            pTSBuffer->QueryInterface(IID_IHXBuffer, (void**)&pBuffer);
	     HX_RELEASE(pTSBuffer);
            res = pBuffer->SetSize(ulSize);

            if (HXR_OK != res)
            {
                HX_RELEASE(pBuffer);
            }
        }
    }

    return res;
}
