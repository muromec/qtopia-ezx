/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: sdpprop.cpp,v 1.7 2005/08/02 18:00:43 albertofloyd Exp $
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

//#include "hlxclib/stdlib.h"
//#include "hlxclib/stdio.h"

#include "hxtypes.h"
#include "hxcom.h"
#include "ihxpckts.h"
#include "chxpckts.h"
#include "hxslist.h"
#include "rtsputil.h"
#include "rtspprop.h"
#include "sdpprop.h"
#include "safestring.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif


SDPIntegerProperty::SDPIntegerProperty(const char* pName,
    INT32 value): 
    RTSPIntegerProperty(pName, value)
{
}

CHXString
SDPIntegerProperty::asString()
{
    char tmpBuf[80]; /* Flawfinder: ignore */

    SafeSprintf(tmpBuf, 80, "%ld", m_value); 
    return CHXString(tmpBuf);
}

SDPBufferProperty::SDPBufferProperty(const char* pName,
    IHXBuffer* pBuffer):
    RTSPBufferProperty(pName, pBuffer)
{
}

CHXString
SDPBufferProperty::asString()
{
    CHXString str;
    INT32 dataSize = (INT32)m_pValue->GetSize();
    if(dataSize > 0)
    {
        char* pDataBuf = new char[dataSize * 2 + 4];	// overkill
        (void)BinTo64((const BYTE*)m_pValue->GetBuffer(),
            dataSize, pDataBuf);
		int lenTmpBuf = (int)(strlen(pDataBuf) + strlen(m_pName) + 40);
        char* pTmpBuf = new char[lenTmpBuf];
        SafeSprintf(pTmpBuf, (UINT32)lenTmpBuf, "%s", pDataBuf); 
        str = pTmpBuf;
        delete[] pTmpBuf;
        delete[] pDataBuf;
    }
    else
    {
        str = "";
    }
    return str;
}

SDPStringProperty::SDPStringProperty(const char* pName,
    const char* pValue):
    RTSPStringProperty(pName, pValue)
{
}

CHXString
SDPStringProperty::asString()
{
    return CHXString((const char*)m_value);
}
