/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: altgrpselect.h,v 1.3 2005/03/10 20:59:17 bobclark Exp $
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
#ifndef ALTGRPSELECT_H
#define ALTGRPSELECT_H

#include "hxcom.h"
#include "hxresult.h"
#include "ihxpckts.h"
#include "hxccf.h"
#include "altidmap.h"
#include "hxslist.h"

class CHXAltIDSet;
class CHXAltIDFilter;

class CHXAltGroupSelector
{
public:
    CHXAltGroupSelector();
    ~CHXAltGroupSelector();

    HX_RESULT Init(IUnknown* pContext);
    
    void Reset();

    HX_RESULT AddFilter(CHXAltIDFilter* pFilter);

    // This function will return the selected AltIDs
    HX_RESULT Select(UINT32 uHdrCount, IHXValues** pInHdrs,
                     CHXAltIDSet& altIDSet);

    // This function returns the headers for the selected AltIDs
    HX_RESULT Select(UINT32 uHdrCount, IHXValues** pInHdrs, 
                     IHXValues** pOutHdrs);

private:
    HX_RESULT createDefaultProperties(UINT32 uHdrCount, IHXValues** pInHdrs, 
                                      IHXValues** pOutHdrs);
    HX_RESULT mergeAltProperties(UINT32 uAltID,
                                 UINT32 uHdrCount, IHXValues** pInHdrs, 
                                 IHXValues** pOutHdrs);
    HX_RESULT getAltInfo(IHXValues* pHdr, UINT32 uAltID, 
                         REF(IHXValues2*)pAlt) const;

    HX_RESULT copyProperties(IHXValues* pDst, IHXValues* pSrc) const;

    HX_RESULT getAllAltIDs(UINT32 uHdrCount, IHXValues** pInHdrs,
                           CHXAltIDSet& altIDSet);
    HX_RESULT getStreamAltIDs(UINT32 uStreamID, IHXValues* pStreamHdr,
                              CHXAltIDSet& altIDSet);

    HXBOOL hasOneAltForEachStream(UINT32 uStreamCount,
                                const CHXAltIDSet& altIDSet) const;

    void destroyHdrs(UINT32 uHdrCount, IHXValues** pHdrs) const;


    IHXCommonClassFactory* m_pCCF;

    CHXAltIDMap m_altIDMap;
    CHXSimpleList m_filterList;
};

#endif ALTGRPSELECT_H
