/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: stream_desc_hlpr.cpp,v 1.7 2007/07/06 20:39:16 jfinnecy Exp $
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

#include "stream_desc_hlpr.h"

#include "hxplugn.h"
#include "chxpckts.h" // CHXHeader::mergeHeaders

IHXStreamDescription* 
HXStreamDescriptionHelper::GetInstance(IUnknown* pContext, 
				       const char* pMimeType)
{
    IHXStreamDescription*  pSD		    = 0;
    IHXPlugin2Handler*	    pPlugin2Handler = NULL;
    const char*		    pFindMimeType   = pMimeType;
    IUnknown*		    pInstance	    = NULL;

    pContext->QueryInterface(IID_IHXPlugin2Handler, (void**)&pPlugin2Handler );

    if (pPlugin2Handler)
    {
	if (HXR_OK == pPlugin2Handler->FindPluginUsingStrings(PLUGIN_CLASS,
							      PLUGIN_STREAM_DESC_TYPE,
							      PLUGIN_STREAMDESCRIPTION,
							      (char*)pFindMimeType,
							      NULL,
							      NULL,
							      pInstance))
	{
	    HX_RESULT rc;
	    rc = pInstance->QueryInterface(IID_IHXStreamDescription,
					   (void**)&pSD);
	    if(rc == HXR_OK)
	    {
		IHXPlugin* pSDPlugin = 0;
		rc = pSD->QueryInterface(IID_IHXPlugin,
					 (void**)&pSDPlugin);
		if(rc == HXR_OK)
		{
		    pSDPlugin->InitPlugin(pContext);
		    pSDPlugin->Release();
		}
	    }
	    pInstance->Release();
	}
	HX_RELEASE(pPlugin2Handler);
    }

    return pSD;
}

HX_RESULT 
HXStreamDescriptionHelper::ParseAndMergeSDPData(IUnknown* pContext,
                                                IHXValues* pHeader)
{
    HX_RESULT res = HXR_OK;
    IHXBuffer* pSDPDataBuf = NULL;

    if (!pHeader)
    {
	res = HXR_INVALID_PARAMETER;
    }
    else if (HXR_OK == pHeader->GetPropertyCString("SDPData", pSDPDataBuf))
    {
	IHXStreamDescription* pSD = GetInstance(pContext, 
                                                "application/sdp");

	res = HXR_FAIL;
	
	if (pSD)
	{
	    UINT16 nValues = 0;
	    IHXValues** ppValues = NULL;
	    res = pSD->GetValues(pSDPDataBuf, nValues, ppValues);

	    if (HXR_OK == res)
	    {
		if (nValues >= 1)
		{
		    /* Merge headers from only the first header
		     * since we aren't expecting 'm=' lines in the
		     * SDP
		     */
		    CHXHeader::mergeHeaders(pHeader, ppValues[0]);
		}

		// Clean up the headers returned
		for (UINT16 i = 0; i < nValues; i++)
		{
		    HX_RELEASE(ppValues[i]);
		}
		HX_VECTOR_DELETE(ppValues);
	    }
	    else if (HXR_OUTOFMEMORY != res)
	    {
		/* Mask any errors other than OUTOFMEMORY
		 * because conversion of the SDPData field
		 * is not critical
		 */
		res = HXR_OK;
	    }
	    HX_RELEASE(pSD);
	}
    }
    HX_RELEASE(pSDPDataBuf);

    return res;
}
