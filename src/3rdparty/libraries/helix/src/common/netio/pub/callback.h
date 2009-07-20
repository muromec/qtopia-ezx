/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: callback.h,v 1.4 2007/07/06 20:43:57 jfinnecy Exp $
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

#ifdef __MWERKS__
#pragma once
#endif

#ifndef _callback
#define	_callback

typedef enum hx_callback
{
	HX_GENERIC_CALLBACK = 0,
	HX_AUDIO_CALLBACK,
	HX_TCP_CALLBACK,
	HX_UDP_CALLBACK,
	HX_SYNC_CALLBACK,
	HX_VIDEO_ONLY_CALLBACK,
	HX_TIMER_CALLBACK,
	HX_VIDEO_FILE_CALLBACK,
	HX_PRE_ENCODE_CALLBACK,
	HX_POST_ENCODE_CALLBACK,
	HX_STATUS_TEXT_CALLBACK,
	HX_SOURCE_CLOSED_CALLBACK,
	HX_WRITE_PACKET_CALLBACK,
	HX_AUDIO_PEAK_CALLBACK,
	HX_VIDEO_CALLBACK,
	HX_AUDIO_ONLY_CALLBACK,
	HX_AUDIO_FILE_CALLBACK,
	HX_AUDIOLEVEL_CHANGED, // for audio level sink	
	HX_ERROR_OCCURRED, // for error sink	
	HX_STREAM_STATS_CHANGED, // for stats sink
	HX_AUDIOSTREAM_STATS_CHANGED, // for stats sink
	HX_VIDEOSTREAM_STATS_CHANGED, // for stats sink
	HX_AUDIO_STATS_CHANGED, // for stats sink
	HX_VIDEO_STATS_CHANGED, // for stats sink
	HX_AUDIO_MULTIPLE_CALLBACK,//HX_AUDIO_ONLY_CALLBACK for live multi (=a/v) sources 
	HX_VIDEO_MULTIPLE_CALLBACK //HX_VIDEO_ONLY_CALLBACK for live multi source 

} HX_CALLBACK;

class callback 
{
public:

/* 	class destructor */	
						callback		(void) {;};
	virtual				~callback		(void) {;};

	virtual void		callback_task( HX_CALLBACK message = HX_GENERIC_CALLBACK, void* params = 0 ) {}
};

#endif // _callback

