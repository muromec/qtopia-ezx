/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxathsp.cpp,v 1.3 2004/07/09 18:23:51 hubbe Exp $
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

#include "hxtypes.h"
#include "hxcom.h"

#include "smartptr.h"

#include "hxauthn.h"

IMPLEMENT_SMART_POINTER(IHXCredRequest);
IMPLEMENT_WRAPPED_POINTER(IHXCredRequest);

IMPLEMENT_SMART_POINTER(IHXCredRequestResponse);
IMPLEMENT_WRAPPED_POINTER(IHXCredRequestResponse);

IMPLEMENT_SMART_POINTER(IHXClientAuthConversation);
IMPLEMENT_WRAPPED_POINTER(IHXClientAuthConversation);

IMPLEMENT_SMART_POINTER(IHXClientAuthResponse);
IMPLEMENT_WRAPPED_POINTER(IHXClientAuthResponse);

IMPLEMENT_SMART_POINTER(IHXServerAuthConversation);
IMPLEMENT_WRAPPED_POINTER(IHXServerAuthConversation);

IMPLEMENT_SMART_POINTER(IHXServerAuthResponse);
IMPLEMENT_WRAPPED_POINTER(IHXServerAuthResponse);

IMPLEMENT_SMART_POINTER(IHXUserContext);
IMPLEMENT_WRAPPED_POINTER(IHXUserContext);

IMPLEMENT_SMART_POINTER(IHXUserProperties);
IMPLEMENT_WRAPPED_POINTER(IHXUserProperties);

IMPLEMENT_SMART_POINTER(IHXUserImpersonation);
IMPLEMENT_WRAPPED_POINTER(IHXUserImpersonation);

IMPLEMENT_SMART_POINTER(IHXChallenge);
IMPLEMENT_WRAPPED_POINTER(IHXChallenge);

IMPLEMENT_SMART_POINTER(IHXChallengeResponse);
IMPLEMENT_WRAPPED_POINTER(IHXChallengeResponse);
