/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxaudtyp.h,v 1.6 2005/03/14 20:32:06 bobclark Exp $
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

#ifndef _HXAUDTYP_H_
#define _HXAUDTYP_H_

/****************************************************************************
 *	Enums
 */
typedef enum 
{
        E_PLAYING
,       E_PAUSED
,       E_INITIALIZED
,       E_STOPPED
} EPlayerState;

typedef enum
{
    ACTION_ADD	    = 0,    
    ACTION_REMOVE   = 1,
    ACTION_CHECK    = 2
} PROCESS_ACTION;

// structures
typedef struct _HXAudioHookInfo
{
    IHXAudioHook*       pHook;             // interface to the hook
    HXBOOL                bDisableWrite;     // if TRUE, do not write 
    HXBOOL		bFinal;	   	   // call hook on final data; post-mix only
    HXBOOL		bIgnoreAudioData;
    HXBOOL                bMultiChannelSupport;
} HXAudioHookInfo;

typedef struct _HXAudioDevHookEntry
{
    IHXAudioHook*	pHook;
    AudioDeviceHookType	type;
} HXAudioDevHookEntry;


/****************************************************************************
 *	Data Structures
 */
struct HXAudioInfo
{
    IHXBuffer*		pBuffer;
    UCHAR*		pOffset;
    ULONG32		ulStartTime;
    ULONG32		ulBytesLeft;
    AudioStreamType	uAudioStreamType;
    // also keep sample-accurate timing information around.
    INT64	llStartTimeInSamples ;
    INT64	llEndTimeInSamples ;
};

#endif
