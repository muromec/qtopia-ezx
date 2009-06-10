/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: helixadapthdr.cpp,v 1.4 2007/07/06 20:51:35 jfinnecy Exp $
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

#include "helixadapthdr.h"

const char* const CHelixAdaptationHeader::zm_pIntParams[] = {"size",
                                                       "target-time",
                                                       "stream-switch",
                                                       "feedback-level",
                                                       NULL};

const char* const CHelixAdaptationHeader::zm_pStringParams[] = {"url", 
                                                          "exclude-rules",
                                                          NULL};

const char* const CHelixAdaptationHeader::zm_pRequiredParams[] = {"url", 
                                                            "size",
                                                            "target-time",
                                                            NULL};

const char* const CHelixAdaptationHeader::zm_pParamOrder[] = {"url", 
                                                        "size",
                                                        "target-time",
                                                        "exclude-rules",
                                                        "stream-switch",
                                                        "feedback-level",
                                                        NULL};

CHelixAdaptationHeader::CHelixAdaptationHeader()
{}

CHelixAdaptationHeader::~CHelixAdaptationHeader()
{}

const char* const* 
CHelixAdaptationHeader::intParams() const
{
    return zm_pIntParams;
}

const char* const* 
CHelixAdaptationHeader::stringParams() const
{
    return zm_pStringParams;
}

const char* const* 
CHelixAdaptationHeader::requiredParams() const
{
    return zm_pRequiredParams;
}

const char* const* 
CHelixAdaptationHeader::paramOrder() const
{
    return zm_pParamOrder;
}
