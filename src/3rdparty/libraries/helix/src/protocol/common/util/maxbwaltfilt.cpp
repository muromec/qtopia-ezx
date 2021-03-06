/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: maxbwaltfilt.cpp,v 1.1 2004/07/29 00:27:20 acolwell Exp $
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
#include "maxbwaltfilt.h"
#include "altidset.h"
#include "altgrpitr.h"

CHXMaxBWAltIDFilter::CHXMaxBWAltIDFilter() :
    m_uMaxBandwidth(0)
{}

CHXMaxBWAltIDFilter::~CHXMaxBWAltIDFilter()
{}

HX_RESULT 
CHXMaxBWAltIDFilter::Init(UINT32 uMaxBandwidth)
{
    HX_RESULT res = HXR_INVALID_PARAMETER;
    
    if (uMaxBandwidth)
    {
        m_uMaxBandwidth = uMaxBandwidth;
        res = HXR_OK;
    }

    return res;
}

HX_RESULT 
CHXMaxBWAltIDFilter::Filter(UINT32 uHdrCount, IHXValues** pInHdrs,
                            const CHXAltIDMap& altIDMap,
                            CHXAltIDSet& /* in/out */ altIDSet)
{
    HX_RESULT res = HXR_INVALID_PARAMETER;

    if (!m_uMaxBandwidth)
    {
        res = HXR_UNEXPECTED;
    }
    else if ((uHdrCount > 1) && pInHdrs)
    {
        CHXAltIDSet newIDSet;

        CHXBWASAltGroupIterator itr;

        res = itr.Init(pInHdrs[0]);
        
        if (HXR_OK == res)
        {
            // Iterate over all the BS:AS group values
            for(;(HXR_OK == res) && itr.More(); itr.Next())
            {
                UINT32 uBandwidth;

                res = itr.GetBandwidth(uBandwidth);

                // Check to see if the bandwidth
                // for this set of ID's is <=
                // the max bandwidth
                if ((HXR_OK == res) &&
                    (uBandwidth <= m_uMaxBandwidth))
                {
                    CHXAltIDSet bwSet;
                    res = itr.GetAltIDSet(bwSet);

                    // Check to see if this group
                    // is a subset of the current
                    // altIDSet. This prevents us
                    // from adding IDs that have
                    // already been removed from 
                    // the set
                    if ((HXR_OK == res) &&
                        (bwSet.IsSubset(altIDSet)))
                    {
                        res = newIDSet.Union(bwSet);
                    }
                }
            }

            if (HXR_OK == res)
            {
                // Copy the new set into the
                // out parameter
                res = altIDSet.Copy(newIDSet);
            }
        }
        else if (HXR_INVALID_PARAMETER == res)
        {
            // It's likely that no bandwidth info exists
            // for this stream so just return HXR_OK and
            // don't alter altIDset
            res = HXR_OK;
        }
    }

    return res;
}
