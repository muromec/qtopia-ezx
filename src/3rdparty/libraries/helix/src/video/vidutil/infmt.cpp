/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: infmt.cpp,v 1.8 2007/07/06 20:54:06 jfinnecy Exp $
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
#include "hlxclib/string.h"
#include "ciddefs.h"
#include "infmt.h"

static const int cidI420[]  = {CID_I420, CID_YV12, CID_YUY2, CID_UYVY};
static const int cidYV12[]  = {CID_YV12, CID_I420, CID_YUY2, CID_UYVY};
static const int cidYVU9[]  = {CID_YVU9, CID_YVU9, CID_YVU9, CID_YVU9};
static const int cidYUY2[]  = {CID_YUY2, CID_UYVY, CID_YV12, CID_I420};
static const int cidUYVY[]  = {CID_UYVY, CID_YUY2, CID_YV12, CID_I420}; 
static const int cidXING[]  = {CID_YV12, CID_YUY2, CID_UYVY, CID_I420};

CYUVInputFormatMngr::CYUVInputFormatMngr()
 :  m_nNumFormats(0)
{
    memset(m_aFormats, 0, sizeof(m_aFormats));
    m_nNumFormats = 6;

    // Add defaults
    m_aFormats[0].nInput = CID_I420;
    SetDefaultOutputPriority(CID_I420);

    m_aFormats[1].nInput = CID_YV12;
    SetDefaultOutputPriority(CID_YV12);

    // We have no color converts for this format
    m_aFormats[2].nInput = CID_YVU9;
    SetDefaultOutputPriority(CID_YVU9);

    m_aFormats[3].nInput = CID_YUY2;
    SetDefaultOutputPriority(CID_YUY2);

    m_aFormats[4].nInput = CID_UYVY;
    SetDefaultOutputPriority(CID_UYVY);

    m_aFormats[5].nInput = CID_XING;
    SetDefaultOutputPriority(CID_XING);
}

CYUVInputFormatMngr::~CYUVInputFormatMngr()
{
}

HXBOOL CYUVInputFormatMngr::AddFormat(int nIn, int* pList, int nEntries)
{
    if (m_nNumFormats >= MAX_INPUT_FORMATS)
        return FALSE;

    m_aFormats[m_nNumFormats].nInput = nIn;
    m_aFormats[m_nNumFormats].nNumOutPuts = 0;
    
    nEntries = HX_MIN(nEntries, MAX_OUTPUT_FORMATS);

    int nIndex = GetFormatIndex(nIn);

    for (int i=0; i<nEntries; i++)
        m_aFormats[nIndex].aOutputs[i] = pList[i];

    m_aFormats[nIndex].nNumOutPuts = nEntries;
    ++m_nNumFormats;
    
    return TRUE;
}

HXBOOL CYUVInputFormatMngr::IsFormatSupported(int nIn)
{
    int nIndex = GetFormatIndex(nIn);

    if (nIndex >= 0 && m_aFormats[nIndex].nNumOutPuts)
        return TRUE;
    else
        return FALSE;
}

int CYUVInputFormatMngr::GetOutputFormat(int nIn, int nOutputIndex)
{
    int nFormatIndex = GetFormatIndex(nIn);
    if (nFormatIndex < 0 || nFormatIndex >= MAX_INPUT_FORMATS)
        return -1;

    if (nOutputIndex >= m_aFormats[nFormatIndex].nNumOutPuts)
        return -1;

    return m_aFormats[nFormatIndex].aOutputs[nOutputIndex];
}

void CYUVInputFormatMngr::SetOutputPriority(int nIn, int* pList, int nEntries)
{
    int nFormat = GetFormatIndex(nIn);
    if (nFormat < 0)
        return;

    nEntries = HX_MIN(nEntries, MAX_OUTPUT_FORMATS);

    for (int i=0; i<nEntries; i++)
        m_aFormats[nFormat].aOutputs[i] = pList[i];

    m_aFormats[nFormat].nNumOutPuts = 
     HX_MAX(m_aFormats[nFormat].nNumOutPuts, nEntries);
}

int CYUVInputFormatMngr::GetFormatIndex(int nIn)
{
    int nRet = -1;

    for (int i=0; i<m_nNumFormats; i++)
    {
        if (m_aFormats[i].nInput == nIn)
        {
            nRet = i;
            break;
        }
    }

    return nRet;
}

void CYUVInputFormatMngr::SetDefaultOutputPriority(int nIn)
{
    int nFormat = GetFormatIndex(nIn);
    if (nFormat < 0)
        return;

    const int* pList = NULL;
    int  nCount = 4;

    switch (nIn)
    {
        case CID_I420: pList = cidI420; break;
        case CID_YV12: pList = cidYV12; break;
        case CID_YVU9: pList = cidYVU9; nCount = 1; break;
        case CID_YUY2: pList = cidYUY2; break;
        case CID_UYVY: pList = cidUYVY; break;
        case CID_XING: pList = cidXING; nCount = 3; break;
    }

    m_aFormats[nFormat].nNumOutPuts = nCount;

    for (int i=0; i<nCount; i++)
        m_aFormats[nFormat].aOutputs[i] = pList[i];
}
