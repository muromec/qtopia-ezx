/* ***** BEGIN LICENSE BLOCK *****
 * 
 * Portions Copyright (c) 1995-2006 RealNetworks, Inc. All Rights Reserved.
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

#ifndef _IHXMEDIAPROPERTY_H
#define _IHXMEDIAPROPERTY_H

#include "hxcom.h"
#include "hxcomptr.h"

DEFINE_GUID(IID_IHXMediaPropertyAdviseSink, 0x989dba72, 0xaed0, 0x11d3, 0xb6, 0x4f, 0x0, 0x10, 0x5a, 0x12, 0x11, 0x80);
DEFINE_GUID(IID_IHXMediaProperty,           0x989dba72, 0xaed0, 0x11d3, 0xb6, 0x4f, 0x0, 0x10, 0x5a, 0x12, 0x11, 0x85);

DECLARE_INTERFACE_(IHXMediaPropertyAdviseSink, IUnknown)
{
    STDMETHOD(OnMediaPropertyStringAdded) (THIS_ const char* pszName, IHXBuffer* pValue) PURE;

    STDMETHOD(OnMediaPropertyStringChanged) (THIS_ const char* pszName, IHXBuffer* pOldValue, IHXBuffer* pNewValue) PURE;

    STDMETHOD(OnMediaPropertyStringRemoved) (THIS_ const char* pszName, IHXBuffer* pValue) PURE;
};

DECLARE_INTERFACE_(IHXMediaProperty, IUnknown)
{
    STDMETHOD(GetPropertyCount) (THIS_ REF(UINT32) ulCount) PURE;

    STDMETHOD(GetPropertyNameAt) (THIS_ UINT32 ulIndex, REF(IHXBuffer*) pName) PURE;

    STDMETHOD(GetPropertyStringValue) (THIS_ const char* pszName, REF(IHXBuffer*) pValue) PURE;

    STDMETHOD(IsPropertyReadOnly) (THIS_ const char* pszName, REF(HXBOOL) bReadOnly) PURE;

    STDMETHOD(SetPropertyString) (THIS_ const char* pszName, IHXBuffer* pValue) PURE;

    STDMETHOD(AddSink) (THIS_ IHXMediaPropertyAdviseSink* pSink) PURE;

    STDMETHOD(RemoveSink) (THIS_ IHXMediaPropertyAdviseSink* pSink) PURE;
};

DEFINE_SMART_PTR(IHXMediaProperty);
DEFINE_SMART_PTR(IHXMediaPropertyAdviseSink);

#endif // _IHXMEDIAPROPERTY_H
