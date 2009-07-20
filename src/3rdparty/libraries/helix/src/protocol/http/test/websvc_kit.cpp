/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: websvc_kit.cpp,v 1.3 2007/07/06 20:51:34 jfinnecy Exp $
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

#include "HXClientCFuncs.h"
#include "HXClientCallbacks.h"
#include "HXClientConstants.h"

void
OnVisualStateChanged(void* userInfo, bool hasVisualContent)
{
}


void
OnIdealSizeChanged(void* userInfo, SInt32 idealWidth, SInt32 idealHeight)
{
}


void
OnLengthChanged(void* userInfo, UInt32 length)
{
}


void
OnTitleChanged(void* userInfo, const char* pTitle)
{
}


void
OnGroupsChanged(void* userInfo)
{
}


void
OnGroupStarted(void* userInfo, UInt16 groupIndex)
{
}


void
OnContacting(void* userInfo, const char* pHostName)
{
}


void
OnBuffering(void* userInfo, UInt32 bufferingReason, UInt16 bufferPercent)
{
}

void
OnContentConcluded(void* userInfo)
{
}

void
OnContentStateChanged(void* userInfo, int oldContentState, int newContentState)
{
    switch (newContentState)
    {
	case kContentStateNotLoaded:
	{
	}
	break;
	
	case kContentStateLoading:
	{
	}
	break;
	
	case kContentStateStopped:
	{
	}
	break;
	
	case kContentStatePlaying:
	{
	}
	break;
	
	case kContentStatePaused:
	{
	}
	break;

	default:
	{
	    // unsupported!
	}
	break;
    }
}


void
OnStatusChanged(void* userInfo, const char* pStatus)
{
}


void
OnVolumeChanged(void* userInfo, UInt16 volume)
{
}


void
OnMuteChanged(void* userInfo, bool hasMuted)
{
}


void
OnClipBandwidthChanged(void* userInfo, SInt32 clipBandwidth)
{
}


void
OnErrorOccurred(void* userInfo, UInt32 hxCode, UInt32 userCode,
		const char* pErrorString, const char* pUserString,
		const char* pMoreInfoURL)
		
{
}


bool
GoToURL(void* userInfo, const char* pURL, const char* pTarget, bool isPlayerURL, bool isAutoActivated)
{
    return false;
}


bool
RequestAuthentication(void* userInfo, const char* pServer, const char* pRealm,
		      bool isProxyServer)
{
    return false;
}


bool
RequestUpgrade(void* userInfo, const char* pUrl, UInt32 numOfComponents,
	       const char* componentNames[], bool isBlocking)
{
    return false;
}


bool
HasComponent(void* userInfo, const char* componentName)
{
    return false;
}

const HXClientCallbacks g_kitCallbacks =
{
    OnVisualStateChanged,
    OnIdealSizeChanged,
    OnLengthChanged,
    OnTitleChanged,
    OnGroupsChanged,
    OnGroupStarted,
    OnContacting,
    OnBuffering,
    OnContentStateChanged,
    OnContentConcluded,
    OnStatusChanged,
    OnVolumeChanged,
    OnMuteChanged,
    OnClipBandwidthChanged,
    OnErrorOccurred,
    GoToURL,
    RequestAuthentication,
    RequestUpgrade,
    HasComponent
};
