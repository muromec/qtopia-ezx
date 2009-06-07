 /* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: qos_cfg_names.h,v 1.29 2007/04/24 05:11:08 npatil Exp $
 *
 * Portions Copyright (c) 1995-2003 RealNetworks, Inc. All Rights Reserved.
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
#ifndef _QOS_CFG_NAMES_H_
#define _QOS_CFG_NAMES_H_

/* Flow */
#define QOS_CFG_MDP           "UseMediaDeliveryPipeline" /* bool */

/* LinkChar support */
#define QOS_CFG_LINKCHAR_MULTIPLIER    "RTSP.LinkCharMultiplier" /* e.g. 1000 to convert kbps -> bps */

/* Session.RateManager: */
#define QOS_CFG_RM_TYPE       "Session.RateManager.Type"        /* string */
#define QOS_CFG_RM_BUFF_MOD   "Session.RateManager.BufferModel" /* string */

/* Transport: */
/* RTCP RR and RS rate/ratio (b=RR, b=RS) */
#define QOS_CFG_TRAN_RR_RATE  "Transport.RtcpRRbps"   /* bps */
#define QOS_CFG_TRAN_RR_RATIO "Transport.RtcpRRratio" /* % * 10000*/
#define QOS_CFG_TRAN_RS_RATE  "Transport.RtcpRSbps"   /* bps */
#define QOS_CFG_TRAN_RS_RATIO "Transport.RtcpRSratio" /* % * 10000*/

/* Stream bandwidth values (b=AS) */
/* BandwidthProtocol describes the transport protocol to be used for
 * bandwidth estimation ("UDP" or "TCP") */
#define QOS_CFG_SDP_BW_PROT  "SDP.BandwidthProtocol" /* string */
#define QOS_CFG_SDP_BW_MULT  "SDP.BandwidthMultiplier" /* % */

/* Transport.RDT: */
#define QOS_CFG_TRAN_RDT_RTT            "Transport.RDT.RTTProbeFrequency" /* msec */
#define QOS_CFG_TRAN_RDT_BUFF           "Transport.RDT.BufferInfoRatio"  /* packets */
#define QOS_CFG_TRAN_RDT_PKT_AGG_ENABLE "Transport.RDT.EnablePacketAggregation"  /* bool */

/* Transport.CongestionControl: */
#define QOS_CFG_CC_TYPE        "Transport.CongestionControl.UDPType"           /* string */
#define QOS_CFG_CC_MAX_BURST   "Transport.CongestionControl.MaxBurst"          /* packets */
#define QOS_CFG_CC_INIT_PKTSZ  "Transport.CongestionControl.InitialPacketSize" /* Bytes */
#define QOS_CFG_CC_MAX_SENDRATE "Transport.CongestionControl.MaxSendRate"       /* bps */
#define QOS_CFG_CC_MAX_OSR      "Transport.CongestionControl.MaxOversendRate"     /* % */
#define QOS_CFG_CC_USE_SDB     "Transport.CongestionControl.UseClientRateReq"  /* bool */

/* Transport.CongestionControl.Timeout: */
#define QOS_CFG_CC_TIMEO_INIT         "Transport.CongestionControl.Timeout.InitialTimeout"   /* msec */
#define QOS_CFG_CC_TIMEO_INTVL_COUNT  "Transport.CongestionControl.Timeout.Interval"        /* packets */
#define QOS_CFG_CC_TIMEO_DISABLE      "Transport.CongestionControl.Timeout.Disable"        /* bool */
#define QOS_CFG_CC_TIMEO_MAX_TIMEOUTS "Transport.CongestionControl.Timeout.MaxTimeouts"     /* packets */

/* Transport.CongestionControl.TCP: */
#define QOS_CFG_TCP_HONOR_MAX_SENDRATE  "Transport.CongestionControl.TCP.HonorMaxSendRate"     /* bool */

/* Transport.CongestionControl.TFRC: */
#define QOS_CFG_TFRC_RTT_IIR_FILTER "Transport.CongestionControl.TFRC.EnableIIRForRTT"     /* bool */
#define QOS_CFG_TFRC_SS_SC          "Transport.CongestionControl.TFRC.SlowStartScalar"     /* % */
#define QOS_CFG_TFRC_SS_RATE        "Transport.CongestionControl.TFRC.StartRate"           /* bps */
#define QOS_CFG_TFRC_RTO_SC         "Transport.CongestionControl.TFRC.TimeoutScalar"       /* % */
#define QOS_CFG_TFRC_RTO_XMIT       "Transport.CongestionControl.TFRC.TimeoutTransmission" /* bool */
#define QOS_CFG_TFRC_RATE_INC       "Transport.CongestionControl.TFRC.RateIncreaseLimit"   /* % */
#define QOS_CFG_TFRC_DEBUG_OUT      "Transport.CongestionControl.TFRC.Trace"               /* bool */
#define QOS_CFG_TFRC_USE_SS         "Transport.CongestionControl.TFRC.EnableSlowStart"     /* bool */
#define QOS_CFG_TFRC_ACK_SZ         "Transport.CongestionControl.TFRC.TCPPacketsPerACK"    /* integer */

/* Transport.CongestionControl.BCC: */
#define QOS_CFG_BCC_RTT_IIR_FILTER "Transport.CongestionControl.BCC.EnableIIRForRTT"     /* bool */
#define QOS_CFG_BCC_ZERO_TIMEO     "Transport.CongestionControl.BCC.TimeoutTransmission" /* bool */
#define QOS_CFG_BCC_DEBUG_OUT      "Transport.CongestionControl.BCC.Trace"               /* bool */
#define QOS_CFG_BCC_INC_COEF       "Transport.CongestionControl.BCC.IncreaseCoefficient" /* integer */
#define QOS_CFG_BCC_DEC_COEF       "Transport.CongestionControl.BCC.DecreaseCoefficient" /* integer */
#define QOS_CFG_BCC_INC_EXP        "Transport.CongestionControl.BCC.IncreaseExponent"    /* float */
#define QOS_CFG_BCC_DEC_EXP        "Transport.CongestionControl.BCC.DecreaseExponent"    /* float */
#define QOS_CFG_BCC_INC_THR        "Transport.CongestionControl.BCC.IncreaseThreshold"    /* integer */
#define QOS_CFG_BCC_DEC_THR        "Transport.CongestionControl.BCC.DecreaseThreshold"    /* integer */
#define QOS_CFG_BCC_LOW_LIM        "Transport.CongestionControl.BCC.LowerLimit"           /* integer;bps */
#define QOS_CFG_BCC_TGT_RIO        "Transport.CongestionControl.BCC.TargetRatio"         /* integer */

/* InputSource */
#define QOS_CFG_IS_DEFAULT_MEDIARATE   "InputSource.DefaultMediaRate"                /* bps */
#define QOS_CFG_IS_DISABLE_AUDIO_SW "InputSource.EnableAudioRateSwitching"     /* bool */

#define QOS_CFG_INACTIVITY_TIMEOUT  "InactivityTimeout"
 /* sec */

#endif /*_QOS_CFG_NAMES_H_ */
