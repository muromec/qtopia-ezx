/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxdatasourcemmfclip.h,v 1.4 2007/03/27 14:56:29 aperiquet Exp $
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

#ifndef _HXMMFDATASOURCE_CLIP_
#define _HXMMFDATASOURCE_CLIP_

#include "hxdatasource.h"
#ifdef HELIX_FEATURE_S60_PROGDOWN
#include "ihxdownloadeventobserver.h"
#endif
#include <mmffile.h>

// warnning: This interface and related functionality is under development.
// IHXMMFDataSource API can change anytime.


class CHXDataSourceMMFClip : public CHXDataSource
#ifdef HELIX_FEATURE_S60_PROGDOWN
                             ,public IHXDownloadEventObserver
#endif
{
private:
    CHXDataSourceMMFClip();
public:
    CHXDataSourceMMFClip(CMMFClip *pImpl);
    ~CHXDataSourceMMFClip();

    STDMETHOD(QueryInterface)       (THIS_ REFIID riid, void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)      (THIS);

    STDMETHOD_(ULONG32,Release)     (THIS);
    STDMETHOD(Close)   (THIS_);
    UINT32 Read(THIS_  void *, ULONG32 size, ULONG32 count);
    UINT32 Write(THIS_ void *, ULONG32 size, ULONG32 count);
    STDMETHOD(GetSize)  (THIS_ UINT32 &ulSize);

#ifdef HELIX_FEATURE_S60_PROGDOWN
    UINT32 IsDownloadComplete        (THIS);
    UINT32 GetDownloadSize           (THIS);
    UINT32 GetUnderflowTimeoutInMillis(THIS);
    STDMETHOD(SetDownloadComplete)  (THIS_ UINT32 downloadComplete);
    STDMETHOD(SetDownloadSize)      (THIS_ UINT32 downloadSize);
    STDMETHOD(SetUnderflowTimeoutInMillis)(THIS_ UINT32 timeout);
#endif

protected:
    CMMFClip*            m_pImpl;     // actual data source implementation 
#ifdef HELIX_FEATURE_S60_PROGDOWN
    UINT32               m_ulDownloadComplete;
    UINT32               m_ulDownloadSize;
    UINT32               m_ulUnderflowTimeoutInMillis;
#endif
    UINT32               ReadL(THIS_  void *, ULONG32 size, ULONG32 count);
    UINT32               WriteL(THIS_ void *, ULONG32 size, ULONG32 count);

};

#endif // _HXMMFDATASOURCE_CLIP_

