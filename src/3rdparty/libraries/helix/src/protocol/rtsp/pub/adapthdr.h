/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: adapthdr.h,v 1.5 2007/07/06 20:51:36 jfinnecy Exp $
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

#ifndef ADAPTHDR_H
#define ADAPTHDR_H

#include "hxtypes.h"
#include "hxcom.h"
#include "ihxpckts.h"
#include "ihxpckts.h"
#include "hxccf.h"

class CHXAdaptationHeader
{
public:
    CHXAdaptationHeader();
    virtual ~CHXAdaptationHeader();

    bool operator==(const CHXAdaptationHeader& rhs);

    HX_RESULT Init(IUnknown* pContext);

    HX_RESULT Parse(const char* pBuf, UINT32 ulSize);
    HX_RESULT GetValues(REF(IHXValues*) pValues);

    HX_RESULT SetValues(IHXValues* pValues);
    HX_RESULT GetString(REF(IHXBuffer*) pBuf);

protected:
    virtual const char* const* intParams() const = 0;
    virtual const char* const* stringParams() const = 0;
    virtual const char* const* requiredParams() const;
    virtual const char* const* paramOrder() const;

private:
    // Hide copy constructor and assignment operator.
    CHXAdaptationHeader(const CHXAdaptationHeader& rhs);
    CHXAdaptationHeader& operator=(const CHXAdaptationHeader& rhs);

    HX_RESULT GetURL(REF(const char*) pCur, const char* pEnd);
    HX_RESULT GetAdaptParam(REF(const char*) pCur, const char* pEnd);
    HX_RESULT GetIntParam(const char* pName, REF(const char*) pCur, 
                          const char* pEnd);
    HX_RESULT GetStringParam(const char* pName, REF(const char*) pCur, 
                             const char* pEnd);

    HXBOOL IsIntParam(const char* pName);
    HXBOOL IsStringParam(const char* pName);
    HXBOOL InKeyList(const char* pName, const char* const* pList);
    
    bool CompareIHXValues(IHXValues* pA, IHXValues* pB);
    HX_RESULT CopyIHXValues(IHXValues* pA);
    
    HXBOOL AreValuesValid(IHXValues* pA);
    HXBOOL IsValidURL(const char* pURL);
    HXBOOL HasRequiredParams(IHXValues* pA) const;

    HX_RESULT AddURL(IHXBuffer* pBuf, IHXBuffer* pURL);
    HX_RESULT AddStringParam(IHXBuffer* pBuf,
                             const char* pKey, IHXBuffer* pValue);
    HX_RESULT AddULONG32Param(IHXBuffer* pBuf,
                              const char* pKey, ULONG32 ulValue);

    IHXCommonClassFactory* m_pCCF;
    IHXValues* m_pValues;

    static const char* const zm_pDefaultRequiredParams[];
    static const char* const zm_pDefaultParamOrder[];
};

#endif /* ADAPTHDR_H */
