/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: latency_mode_hlpr.h,v 1.3 2004/11/10 04:25:19 acolwell Exp $
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

#ifndef LATENCY_MODE_HLPR_H
#define LATENCY_MODE_HLPR_H

#include "hxcom.h"
#include "hxresult.h"
#include "hxrsdbf.h"
#include "hxprefs.h"
#include "hxausvc.h"
#include "ihxpckts.h"
#include "hxstring.h"

class CHXLatencyModeHelper
{
public:
    CHXLatencyModeHelper();
    ~CHXLatencyModeHelper();

    HX_RESULT Init(IUnknown* pContext);

    HX_RESULT OnFileHeader(IHXValues* pFileHeader);

    HX_RESULT SetTransportDelays();

    UINT32 RebufferPrerollIncrement();
    UINT32 MaxPrerollIncrement();

    UINT32 MaxResendBufferDelay();

    HX_RESULT Close();

private:
    typedef struct {
        UINT32 m_uMinimumResendDelay;
        UINT32 m_uMaximumResendDelay;
        UINT32 m_uExtraBufferingResendDelay;
        UINT32 m_uAudioPushdown;
        UINT32 m_uPrerollIncrement;
        UINT32 m_uMaxPrerollIncrement;
    } LatencyModeDefaults;

    typedef UINT32 (LatencyModeDefaults::*MemberPtr);

    HX_RESULT setupAudioPushdown(ULONG32 ulLatencyMode);

    HX_RESULT setupResendBuffer(ULONG32 ulLatencyMode);
    HX_RESULT getResendBufferDefaultSettings(ULONG32 ulLatencyMode,
                                             REF(UINT32) uMinimumDelay,
                                             REF(UINT32) uMaximumDelay,
                                             REF(UINT32) uExtraBufferingDelay);
    CHXString getLatencyModeKey(ULONG32 ulLatencyMode, const char* pSubkey);
    UINT32 getDefaultCount() const;
    UINT32 getValue(ULONG32 ulLatencyMode, const char* pSubKey,
                    MemberPtr pMemberPtr);

    IHXPreferences* m_pPrefs;
    IHXResendBufferControl2* m_pResendBufCtl;
    IHXAudioPushdown2* m_pAudioPushdown;

    ULONG32 m_ulLatencyMode;
    static const LatencyModeDefaults zm_pDefaults[];
};
#endif /* LATENCY_MODE_HLPR_H */
