/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxqossig.h,v 1.18 2007/04/25 00:56:30 darrick Exp $
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

#ifndef _HX_QOS_SIG_H_
#define _HX_QOS_SIG_H_

#include "hxtypes.h"
#include "hxstreamadapt.h"

/* Define Signal Type as 32 bit field */
typedef UINT16 HX_QOS_SIGNAL;     

/* bitfield offsets for signal filters */
#define HX_QOS_SIGNAL_LAYER_OFFSET             13                   
#define HX_QOS_SIGNAL_RELEVANCE_OFFSET         10                   

/* QoS Signal Utility Macros */
#define MAKE_HX_QOS_SIGNAL_ID(Layer, Relevance, Id) \
((unsigned long) (((unsigned long)(Layer)<< HX_QOS_SIGNAL_LAYER_OFFSET ) | \
((unsigned long)(Relevance)<<HX_QOS_SIGNAL_RELEVANCE_OFFSET) | \
((unsigned long)(Id))))

/* QoS Signal Layout constants */
#define HX_QOS_SIGNAL_RELEVANCE_COUNT           4
#define HX_QOS_SIGNAL_LAYER_COUNT               5

/* QoS Signal Masks */
#define HX_QOS_SIGNAL_LAYER_MASK              0xE000 // 1110 0000 0000 0000
#define HX_QOS_SIGNAL_RELEVANCE_MASK          0x1C00  // 0001 1100 0000 0000
#define HX_QOS_SIGNAL_ID_MASK                 0x3FF  // 0000 0011 1111 1111 

/* QoS Signal Filter Root*/
#define HX_QOS_SIGNAL_ROOT                    0x0

/* QoS Signal Layer */
#define HX_QOS_SIGNAL_LAYER_NETWORK            1
#define HX_QOS_SIGNAL_LAYER_TRANSPORT          2
#define HX_QOS_SIGNAL_LAYER_FRAMING_TRANSPORT  3
#define HX_QOS_SIGNAL_LAYER_SESSION            4
#define HX_QOS_SIGNAL_LAYER_APPLICATION        5

/* QoS Signal Relevance */
#define HX_QOS_SIGNAL_RELEVANCE_CONFIG         1
#define HX_QOS_SIGNAL_RELEVANCE_METRIC         2
#define HX_QOS_SIGNAL_RELEVANCE_SESSIONCTL     3
#define HX_QOS_SIGNAL_RELEVANCE_CTL            4

/* QoS Common Profile Signals  */
#define HX_QOS_SIGNAL_COMMON_COUNT            12

#define HX_QOS_SIGNAL_COMMON_PROFILE           1
#define HX_QOS_SIGNAL_COMMON_MEDIA_RATE        2
#define HX_QOS_SIGNAL_COMMON_PKT_SZ            3
#define HX_QOS_SIGNAL_COMMON_THRUPUT           4 
#define HX_QOS_SIGNAL_COMMON_BUFSTATE          5
#define HX_QOS_SIGNAL_COMMON_SDB               6
#define HX_QOS_SIGNAL_COMMON_MAX_RATE          7 
#define HX_QOS_SIGNAL_COMMON_LINK_CHAR_HDR     8
#define HX_QOS_SIGNAL_COMMON_STREAM_ADAPT_HDR  9
#define HX_QOS_SIGNAL_COMMON_INIT_MEDIA_RATE  10    // Authoritative initial media rate
#define HX_QOS_SIGNAL_COMMON_RTT              18    // RTT in ms
#define HX_QOS_SIGNAL_COMMON_BANDWIDTH        19    // "Bandwidth" header value.

/* RTP Common Profile Signals */
#define HX_QOS_SIGNAL_RTP_COUNT                4
#define HX_QOS_SIGNAL_RTCP_CC_MAX_BURST        11
#define HX_QOS_SIGNAL_RTCP_RR                  12
#define HX_QOS_SIGNAL_BUF_STATE                13
#define HX_QOS_SIGNAL_RTCP_NADU                14

/* RDT Common Profile Signals */
#define HX_QOS_SIGNAL_RDT_COUNT                3
#define HX_QOS_SIGNAL_RDT_METRICS              15
#define HX_QOS_SIGNAL_RDT_BUFFER_STATE         16
#define HX_QOS_SIGNAL_RDT_RTT                  17

/* Total of all signals */
#define HX_QOS_SIGNAL_COUNT                    HX_QOS_SIGNAL_COMMON_COUNT + HX_QOS_SIGNAL_RTP_COUNT +  HX_QOS_SIGNAL_RDT_COUNT

struct BufferMetricsSignal
{
    UINT32 m_ulStreamNumber;
    UINT32 m_ulLowTimestamp;
    UINT32 m_ulHighTimestamp;
    UINT32 m_ulBytes;
};

struct RateSignal
{
    UINT16 m_unStreamNumber;
    UINT32 m_ulRate;         /* bps */
    UINT32 m_ulCumulativeRate;
};

struct RTTSignal
{
    UINT16 m_unStreamNumber;
    double m_fRTT;  /* milliseconds */
};

typedef struct _LinkCharParams LinkCharSignalData;
typedef struct _StreamAdaptationParams StreamAdaptSignalData;

typedef struct _NADUSignalData
{
	UINT16 m_unStreamNumber;	// stream number 
	UINT32 m_ulSSRC;		// 
	UINT16 m_unPlayoutDelay;	//ms, 0xFFFF means not set
	UINT16 m_unHSNR;		//Highest Sequence number received by the client

	UINT16 m_unNSN;		        // next sequence number to be decoded.
    					// when buffer is empty, value is next not yet
    					// received seq. num, i.e. 1 larger than the LS 16 bits of 
    					// RTCP SR or RR report block's 
    					// "extended highest sequence number received"

        UINT8 m_uNUN;                   // Next ADU Unit Number to be decoded 
                                        // within the packet

        UINT16  m_unFBS;                // Free buffer space in 64-byte blocks.
} NADUSignalData;

#endif /* _HX_QOS_SIG_H_ */
