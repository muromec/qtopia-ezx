/* ***** BEGIN LICENSE BLOCK *****
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

#include "mp4adec.h"
#include "decinfostore.h"
#include "raacdecinfo.h"
#include "aacdecinfo.h"
#include "amrdecinfo.h"
#include "amrwbdecinfo.h"
#include "mdfhwdecinfo.h"

CMDFHWDecInfo::CMDFHWDecInfo(CMP4ADecoder* decoder)
{
    m_pDecoder = decoder;
}

HXBOOL CMDFHWDecInfo::IsMatch(const char* pMimeType, IMP4APayloadFormat* pRssm)
{
    m_pDecInfo = NULL;

    // account for dynamic disabling of this codec
    if(m_pDecoder && !m_pDecoder->IsMDFHWDecInfoEnabled())
    {
        return FALSE;
    }

    if(m_RAACDecInfo.IsMatch(pMimeType, pRssm))
    {
        m_pDecInfo = &m_RAACDecInfo;
        return TRUE;
    }

    if(m_AACDecInfo.IsMatch(pMimeType, pRssm))
    {
        m_pDecInfo = &m_AACDecInfo;
        return TRUE;
    }

    if(m_AMRNBDecInfo.IsMatch(pMimeType, pRssm))
    {
        m_pDecInfo = &m_AMRNBDecInfo;
        return TRUE;
    }

    if(m_AMRWBDecInfo.IsMatch(pMimeType, pRssm))
    {
        m_pDecInfo = &m_AMRWBDecInfo;
        return TRUE;
    }

    return FALSE;
}

const char* CMDFHWDecInfo::GetLibName(void)
{
    return "dspmdfaud";             //"mdfaudsw";
}

const char* CMDFHWDecInfo::GetCodecFourCC(void)
{
    if(m_pDecInfo)
        return m_pDecInfo->GetCodecFourCC();
    else
        return "";
}

const char* CMDFHWDecInfo::GetCodecName(void)
{
    if(m_pDecInfo)
        return m_pDecInfo->GetCodecName();
    else
        return "";
}

