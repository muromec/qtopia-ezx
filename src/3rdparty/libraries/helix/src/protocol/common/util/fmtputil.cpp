/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: fmtputil.cpp,v 1.1 2004/07/14 17:41:39 acolwell Exp $
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

#include "fmtputil.h"
#include "safestring.h"
#include "hxccf.h"    // IHXCommonClassFactory
#include "ihxpckts.h" // IHXBuffer, IHXValues
#include "sdptools.h" // HexStringToBinary

#define MAX_INT_TEXT_LENGTH	10

HX_RESULT CHXFMTPUtil::GetFMTPConfig(IHXValues* pFMTParams, 
					   IHXCommonClassFactory* pCCF,
					   REF(IHXBuffer*)pConfigBuf)
{
    IHXBuffer* pConfigStr = 0;
    HX_RESULT res = pFMTParams->GetPropertyCString("FMTPconfig", 
						   pConfigStr);
    if (FAILED(res))
    {
	ULONG32 ulConfigVal = 0;
	
	if ((HXR_OK == (res = pFMTParams->GetPropertyULONG32("FMTPconfig", 
							     ulConfigVal))) &&
	    (HXR_OK == (res = pCCF->CreateInstance(IID_IHXBuffer, 
						   (void**)&pConfigStr))) &&
	    (HXR_OK == (res = pConfigStr->SetSize(MAX_INT_TEXT_LENGTH + 1))))
	{
									       
	    SafeSprintf((char*) pConfigStr->GetBuffer(),pConfigStr->GetSize(),
		    "%ld", ulConfigVal);
	}
    }
    
    if (pConfigStr)
    {
	const char* pConfig = (char*) pConfigStr->GetBuffer();

	if (pConfig)
	{		    
	    ULONG32 ulConfigSize = strlen(pConfig) / 2;
		
	    if ((ulConfigSize > 0) &&
		(HXR_OK == (res = pCCF->CreateInstance(IID_IHXBuffer,
						       (void**)&pConfigBuf)))&&
		(HXR_OK == (res = pConfigBuf->SetSize(ulConfigSize))))
		
	    {
		res = HexStringToBinary(pConfigBuf->GetBuffer(), pConfig);
	    }
	}

	pConfigStr->Release();
	pConfigStr = 0;
    }

    return res;
}
