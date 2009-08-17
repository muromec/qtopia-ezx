/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxdatasource_descriptor.cpp,v 1.2 2006/09/19 23:58:42 gashish Exp $ 
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

#include "hxcom.h"
#include "hxtypes.h"
#include "hxresult.h"
#include "hxassert.h"
#include "hxtlogutil.h"
#include "hlxclib/sys/stat.h"
#include "hlxclib/time.h"
#include "hxdatasource_descriptor.h"

CHXDataSourceDescriptor::CHXDataSourceDescriptor(HXBOOL bPeek)
    :m_pData(NULL)
{
}

CHXDataSourceDescriptor* CHXDataSourceDescriptor::Create(
                        TDesC8* pData, 
                        HXBOOL bPeek)
{
    
    TInt err = KErrNone;

    CHXDataSourceDescriptor *self = new CHXDataSourceDescriptor(bPeek);

    if (self)
    {
        TRAP(err, self->ConstructL(pData));
        if (err != KErrNone)
        {
            HX_DELETE(self);
        }
    }


    return self;
}

HX_RESULT CHXDataSourceDescriptor::ConstructL(TDesC8 *pData)
{
    m_pData = pData;
}

STDMETHODIMP CHXDataSourceDescriptor::Close()
{
    return HXR_OK;
}

CHXDataSourceDescriptor::~CHXDataSourceDescriptor()
{
    // weak ref m_pData 
}

UINT32 CHXDataSourceDescriptor::Read(
            void *pBuf, ULONG32 size, ULONG32 count)
{
    TInt    requestSize = size*count;
    TInt    err;

    if (m_ulPosition + requestSize > m_pData->Size())
    {
        // shouldn't read beyond the descriptor.
        requestSize = m_pData->Size() - m_ulPosition;
    }

    if(requestSize <= 0)
    {
        return 0;
    }
    
    TPtr8   ptr8((TUint8 *)pBuf, 0, requestSize);    

    ptr8.Copy(m_pData->Ptr() + m_ulPosition, requestSize);

    m_ulPosition += requestSize;

    return requestSize;
}

UINT32 CHXDataSourceDescriptor::Write(
            void *pBuf, ULONG32 size, ULONG32 count)
{

    HX_ASSERT("CHXDataSourceDescriptor::WriteL" == NULL);
    return 0;
}

STDMETHODIMP CHXDataSourceDescriptor::GetSize(UINT32 &ulSize)
{
    TInt dataSize;

    dataSize = m_pData->Size();
    ulSize = dataSize;
    return HXR_OK;
}

