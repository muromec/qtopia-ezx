/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: latency_mode_hlpr.cpp,v 1.6 2005/03/14 20:31:02 bobclark Exp $
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

#include "latency_mode_hlpr.h"
#include "hxstring.h"
#include "hxprefutil.h"

const ULONG32 InvalidLatencyMode = 0xffffffff;

const CHXLatencyModeHelper::LatencyModeDefaults
CHXLatencyModeHelper::zm_pDefaults[] = {
    {2000, 15000, 2000, 1000, 1000, 3000}, /* LatencyMode 0 */
    {1000,  1000,    0,  500,  100, 1500}, /* LatencyMode 1 */
    {   0,     0,    0,  250,    0,    0}  /* LatencyMode 2 */
};

CHXLatencyModeHelper::CHXLatencyModeHelper() :
    m_pPrefs(NULL),
    m_pResendBufCtl(NULL),
    m_pAudioPushdown(NULL),
    m_ulLatencyMode(InvalidLatencyMode)
{}

CHXLatencyModeHelper::~CHXLatencyModeHelper()
{
    Close();
}

HX_RESULT 
CHXLatencyModeHelper::Init(IUnknown* pContext)
{
    HX_RESULT res = HXR_INVALID_PARAMETER;

    if (pContext)
    {
        Close();

        res = pContext->QueryInterface(IID_IHXPreferences,
                                       (void**)&m_pPrefs);

        if (HXR_OK == res)
        {
            res = pContext->QueryInterface(IID_IHXResendBufferControl2,
                                           (void**)&m_pResendBufCtl);
        }

        if (HXR_OK == res)
        {
            res = pContext->QueryInterface(IID_IHXAudioPushdown2,
                                           (void**)&m_pAudioPushdown);
        }
    }

    return res;
}

HX_RESULT 
CHXLatencyModeHelper::OnFileHeader(IHXValues* pFileHeader)
{
    HX_RESULT res = HXR_INVALID_PARAMETER;

    if (pFileHeader)
    {
        ULONG32 ulIsLive = 0;
        if ((HXR_OK == pFileHeader->GetPropertyULONG32("LiveStream",
                                                       ulIsLive)) &&
            (ulIsLive == 1))
        {
            // Assume latency mode 0 by default
            m_ulLatencyMode = 0;

            // Get the value from the header
            pFileHeader->GetPropertyULONG32("LatencyMode", m_ulLatencyMode);

            ULONG32 ulNoLatency = 0;
            if (((HXR_OK == pFileHeader->GetPropertyULONG32("MinimizeLatency", 
                                                            ulNoLatency)) &&
                 (ulNoLatency == 1)))
            {
                // The minimize latency feature is equivalent to latency 
                // mode 2
                m_ulLatencyMode = 2;
            }

            // The player's preference can overwrite the one in file header
            HXBOOL bNoLatency = FALSE;
            if ((HXR_OK == ReadPrefBOOL(m_pPrefs, "MinimizeLatency", 
                                        bNoLatency)) && bNoLatency)
            {
                // The minimize latency feature is equivalent to latency mode 2
                m_ulLatencyMode = 2;
            }

            // The player's "LatencyMode" preference gets the final say
            UINT32 uLatencyModePref;
            if (HXR_OK == ReadPrefUINT32(m_pPrefs, "LatencyMode", 
                                         uLatencyModePref))
            {
                m_ulLatencyMode = uLatencyModePref;
            }
            
            // Put the final latency mode value back in the header
            // so that other code can access it
            pFileHeader->SetPropertyULONG32("LatencyMode", m_ulLatencyMode);

            res = setupAudioPushdown(m_ulLatencyMode);
        }
        else
        {
            // This isn't a live stream so we don't worry about latency
            res = HXR_OK;
        }
    }

    return res;
}

HX_RESULT 
CHXLatencyModeHelper::SetTransportDelays()
{
    HX_RESULT res = HXR_OK;

    if (InvalidLatencyMode != m_ulLatencyMode)
    {
        res = setupResendBuffer(m_ulLatencyMode);
    }

    return res;
}

UINT32 
CHXLatencyModeHelper::RebufferPrerollIncrement()
{
    return getValue(m_ulLatencyMode,
                    "RebufferPrerollIncrement",
                    &LatencyModeDefaults::m_uPrerollIncrement);
}

UINT32 
CHXLatencyModeHelper::MaxPrerollIncrement()
{
    return getValue(m_ulLatencyMode,
                    "MaxPrerollIncrement",
                    &LatencyModeDefaults::m_uMaxPrerollIncrement);
}

UINT32 CHXLatencyModeHelper::MaxResendBufferDelay()
{
    return getValue(m_ulLatencyMode,
                    "MaximumResendDelay",
                    &LatencyModeDefaults::m_uMaximumResendDelay);
}

HX_RESULT 
CHXLatencyModeHelper::Close()
{
    HX_RELEASE(m_pPrefs);
    HX_RELEASE(m_pResendBufCtl);
    HX_RELEASE(m_pAudioPushdown);
    m_ulLatencyMode = InvalidLatencyMode;

    return HXR_OK;
}

HX_RESULT 
CHXLatencyModeHelper::setupAudioPushdown(ULONG32 ulLatencyMode)
{
    HX_RESULT res = HXR_INVALID_PARAMETER;

    if (InvalidLatencyMode != ulLatencyMode && m_pAudioPushdown)
    {
        UINT32 uAudioPushdown = getValue(ulLatencyMode,
                                         "AudioPushdown",
                                         &LatencyModeDefaults::m_uAudioPushdown);

        res = m_pAudioPushdown->SetAudioPushdown(uAudioPushdown);
    }

    return res;
}

HX_RESULT 
CHXLatencyModeHelper::setupResendBuffer(ULONG32 ulLatencyMode)
{
    HX_RESULT res = HXR_INVALID_PARAMETER;

    if (InvalidLatencyMode != ulLatencyMode && m_pResendBufCtl)
    {
        UINT32 uMinimumDelay = 
            getValue(ulLatencyMode,
                     "MinimumResendDelay",
                     &LatencyModeDefaults::m_uMinimumResendDelay);

        UINT32 uMaximumDelay = MaxResendBufferDelay();

        UINT32 uExtraBufferingDelay = 
            getValue(ulLatencyMode,
                     "ExtraBufferingResendDelay",
                     &LatencyModeDefaults::m_uExtraBufferingResendDelay);
        
        res = m_pResendBufCtl->SetResendBufferParameters(uMinimumDelay,
                                                         uMaximumDelay,
                                                         uExtraBufferingDelay);
    }

    return res;
}

CHXString 
CHXLatencyModeHelper::getLatencyModeKey(ULONG32 ulLatencyMode, 
                                        const char* pSubkey)
{
    // The preference keys have the form
    // LatencyMode0MinimumResendDelay
    // LatencyMode0MaximumResendDelay
    // LatencyMode0ExtraBufferingResendDelay

    CHXString ret("LatencyMode");
    ret.AppendULONG(ulLatencyMode);
    ret += pSubkey;

    return ret;
}

UINT32 
CHXLatencyModeHelper::getDefaultCount() const
{
    return sizeof(zm_pDefaults) / sizeof(LatencyModeDefaults);
}

UINT32 
CHXLatencyModeHelper::getValue(ULONG32 ulLatencyMode, 
                                 const char* pSubKey,
                                 MemberPtr pMemberPtr)
{
    ULONG32 ulDefaultIndex = ulLatencyMode;
    if (ulDefaultIndex >= getDefaultCount())
    {
        // Use mode 0 default for unknown modes
        ulDefaultIndex = 0;
    }
    
    // This is using the member variable pointer to
    // extract the appropriate value from the default
    // struct.
    UINT32 uRet = zm_pDefaults[ulDefaultIndex].*pMemberPtr;

    // Allow preference override
    ReadPrefUINT32(m_pPrefs, 
                   getLatencyModeKey(ulLatencyMode,
                                     pSubKey),
                   uRet);
    return uRet;
}
