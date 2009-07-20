/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: RAFixPtResampler.cpp,v 1.2 2004/07/09 18:37:30 hubbe Exp $
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

#include "RAResampler.h"

// Create a resampler with a given quality level.
HX_RESULT 
RAExactResampler::Create(RAExactResampler** pRes, int inrate, int outrate,
			 int chans, int intype, Quality quality)
{
    *pRes = NULL;

    HX_ASSERT(intype == _INT16);
    if (intype == _INT16)
    {
	if ((*pRes = new RAExactResampler()) != NULL)
	{
	    (*pRes)->m_pInst = InitResampler(inrate, outrate, chans, (int)quality);
	}
    }

    return ((*pRes && (*pRes)->m_pInst) ? HXR_OK : HXR_FAIL);
}


RAExactResampler::RAExactResampler()
    : m_pInst(NULL)
{}


RAExactResampler::~RAExactResampler()
{
    if (m_pInst)
	FreeResampler(m_pInst);
}


int
RAExactResampler::GetMaxOutput(int insamps)
{
    HX_ASSERT(m_pInst);
    return ::GetMaxOutput(insamps, m_pInst);
}


int
RAExactResampler::GetMinInput(int outsamps)
{
    HX_ASSERT(m_pInst);
    return ::GetMinInput(outsamps, m_pInst);
}


int
RAExactResampler::GetDelay()
{
    HX_ASSERT(m_pInst);
    return ::GetDelay(m_pInst);
}

