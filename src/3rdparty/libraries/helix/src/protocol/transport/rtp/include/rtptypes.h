/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: rtptypes.h,v 1.4 2007/07/06 20:51:41 jfinnecy Exp $
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

#ifndef _RTPTYPES_H
#define _RTPTYPES_H


#define RTP_PAYLOAD_PCMU	0
#define RTP_PAYLOAD_1016	1
#define RTP_PAYLOAD_G721	2
#define RTP_PAYLOAD_GSM		3
#define RTP_PAYLOAD_G723	4
#define RTP_PAYLOAD_DVI4_8	5
#define RTP_PAYLOAD_DVI4_16	6
#define RTP_PAYLOAD_LPC		7
#define RTP_PAYLOAD_PCMA	8
#define RTP_PAYLOAD_G722	9
#define RTP_PAYLOAD_L16_2CH	10
#define RTP_PAYLOAD_L16_1CH	11
#define RTP_PAYLOAD_MPA		14
#define RTP_PAYLOAD_G728	15
#define RTP_PAYLOAD_DVI4_11	16
#define RTP_PAYLOAD_DVI4_22	17

#define RTP_PAYLOAD_JPEG	26
#define RTP_PAYLOAD_H261	31
#define RTP_PAYLOAD_MPV		32
#define RTP_PAYLOAD_MP2T	33
#define RTP_PAYLOAD_H263	34

#define RTP_PAYLOAD_DYNAMIC     96
#define RTP_PAYLOAD_RTSP	101
#define RTP_RTCP_BYE		203

/* dynamic payload is in b/n 96 and 127 */
#define IS_DYNAMIC_PAYLOAD(x) (x >= 96 && x <= 127)

#endif	/* _RTPTYPES_H_ */
