/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: packedvalues.cpp,v 1.5 2007/07/06 20:39:16 jfinnecy Exp $
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

#include "hxtypes.h"
#include "hxcom.h"
#include "hxresult.h"
#include "hxstrutl.h"
#include "hxstring.h"
#include "hxbuffer.h"

#include "hxccf.h"
#include "hxassert.h"

#include "packedvalues.h"


inline UINT32
PackedValues::size(IHXValues* pValues)
{
    if (!pValues)
    {
	return 0;
    }

    const char* pPropName;
    ULONG32 uPropValue;
    IHXBuffer* pBuffer;
    HX_RESULT retVal;
    UINT32 ulSize = sizeof(PackedValuesHeader) + 4; //header, nitems

    //ULONG32
    retVal = pValues->GetFirstPropertyULONG32(pPropName, uPropValue);
    while (retVal == HXR_OK)
    {
	ulSize += 1 + 1 + (strlen(pPropName)+1) + 2 + 4;
	retVal = pValues->GetNextPropertyULONG32(pPropName, uPropValue);
    }
    //CString
    retVal = pValues->GetFirstPropertyCString(pPropName, pBuffer);
    while (retVal == HXR_OK)
    {
	ulSize += 1 + 1 + (strlen(pPropName)+1) + 2 + pBuffer->GetSize();
	pBuffer->Release();
	retVal = pValues->GetNextPropertyCString(pPropName, pBuffer);
    }
    //Buffer
    retVal = pValues->GetFirstPropertyBuffer(pPropName, pBuffer);
    while (retVal == HXR_OK)
    {
	ulSize += 1 + 1 + (strlen(pPropName)+1) + 2 + pBuffer->GetSize();
	pBuffer->Release();
	retVal = pValues->GetNextPropertyBuffer(pPropName, pBuffer);
    }

    return ulSize;
}

UINT32 PackedValues::packone(UINT8* buf, UINT8 type, UINT8 name_length, UINT8* name, UINT16 value_length, UINT8* value_data)
{
    UINT8* off = buf;
    *off++ = type;
    *off++ = name_length;
    {memcpy(off, name, name_length); off += name_length; }
    {*off++ = (UINT8) (value_length>>8); *off++ = (UINT8) (value_length);}
    {memcpy(off, value_data, value_length); off += value_length; }
    return off-buf;
}

HX_RESULT PackedValues::pack(UINT8* buf, UINT32 &len, IHXValues* pValues, UINT8 fourCC[4])
{
    HX_ASSERT(len >= size(pValues));

    if (!pValues)
    {
	len = 0;
	return HXR_FAIL;
    }

    PackedValuesHeader* pHeader = (PackedValuesHeader*)buf;
    memcpy(pHeader->fourCC, fourCC, 4);
    buf += sizeof(PackedValuesHeader);

    UINT8* off = buf + 4; //header, nitems

    const char* pPropName;
    ULONG32 uPropValue;
    IHXBuffer* pBuffer;
    HX_RESULT retVal;
    UINT32 uTotal = 0;

    //pack ULONG32
    retVal = pValues->GetFirstPropertyULONG32(pPropName, uPropValue);
    while (retVal == HXR_OK)
    {
	uTotal++;
	UINT8 pULONGBuffer[4];
	pULONGBuffer[0] = (UINT8)(uPropValue >> 24);pULONGBuffer[1] = (UINT8)(uPropValue >> 16);
	pULONGBuffer[2] = (UINT8)(uPropValue >> 8); pULONGBuffer[3] = (UINT8)(uPropValue);

	off += packone(off, PROP_TYPE_ULONG32, strlen(pPropName)+1, (UINT8*)pPropName, 4, pULONGBuffer);
	retVal = pValues->GetNextPropertyULONG32(pPropName, uPropValue);
    }
    //pack CString
    retVal = pValues->GetFirstPropertyCString(pPropName, pBuffer);
    while (retVal == HXR_OK)
    {
	uTotal++;
	off += packone(off, PROP_TYPE_CSTRING, strlen(pPropName)+1, (UINT8*)pPropName, (UINT16)(pBuffer->GetSize()), pBuffer->GetBuffer());
	pBuffer->Release();
	retVal = pValues->GetNextPropertyCString(pPropName, pBuffer);
    }
    //pack Buffer
    retVal = pValues->GetFirstPropertyBuffer(pPropName, pBuffer);
    while (retVal == HXR_OK)
    {
	uTotal++;
	off += packone(off, PROP_TYPE_BUFFER, strlen(pPropName)+1, (UINT8*)pPropName, (UINT16)(pBuffer->GetSize()), pBuffer->GetBuffer());
	pBuffer->Release();
	retVal = pValues->GetNextPropertyBuffer(pPropName, pBuffer);
    }

    //the total number of items
    buf[0] = (UINT8)(uTotal >> 24); buf[1] = (UINT8)(uTotal >> 16);
    buf[2] = (UINT8)(uTotal >> 8 ); buf[3] = (UINT8)(uTotal);

    //header
    UINT32 datasize = off - buf;
    pHeader->size = datasize;
    pHeader->pack();

    len = off - buf + sizeof(PackedValuesHeader);

    return HXR_OK;
}

HX_RESULT PackedValues::unpack(UINT8* buf, UINT32 len, IHXValues* pValues, IHXCommonClassFactory* pCCF)
{
    HX_ASSERT(buf && len > 0); HX_ASSERT(pValues);

    PackedValuesHeader* pHeader = (PackedValuesHeader*)buf;
    memcpy(m_fourCC, pHeader->fourCC, 4);
    
    buf += sizeof(PackedValuesHeader);

    UINT8  type;
    UINT8  name_length;
    UINT8* name;
    UINT16 value_length;
    UINT8* value_data;
    UINT8* off = buf + 4; //nitems


    UINT32 uTotal = (buf[0]<<24) + (buf[1]<<16) + (buf[2]<<8) + buf[3];

    HX_RESULT retVal = HXR_OK;
    for (UINT32 i=0; i < uTotal && retVal == HXR_OK; i++)
    {
	type = *off++;
	name_length = *off++;
	name = off; off += name_length;
	value_length = (off[0] << 8) + off[1]; off+=2;
	value_data = off; off += value_length;

	IHXBuffer* pBuffer = NULL;

	switch(type)
	{
	    case PROP_TYPE_ULONG32:
		pValues->SetPropertyULONG32((char*)name, (value_data[0]<<24) + (value_data[1]<<16) 
						 + (value_data[2]<<8) + value_data[3]);
		break;
	    case PROP_TYPE_CSTRING:
		if (pCCF)
		{
		    retVal = pCCF->CreateInstance(CLSID_IHXBuffer, (void**)&pBuffer);
		    if (retVal == HXR_OK)
		    {
			pBuffer->Set(value_data, value_length);
			pValues->SetPropertyCString((char*)name, pBuffer);
			pBuffer->Release();
		    }
		}
		break;
	    case PROP_TYPE_BUFFER:
		if (pCCF)
		{
		    retVal = pCCF->CreateInstance(CLSID_IHXBuffer, (void**)&pBuffer);
		    if (retVal == HXR_OK)
		    {
			pBuffer->Set(value_data, value_length);
			pValues->SetPropertyCString((char*)name, pBuffer);
			pBuffer->Release();
		    }
		}
		break;
	}
    }
    
    return retVal;
}
