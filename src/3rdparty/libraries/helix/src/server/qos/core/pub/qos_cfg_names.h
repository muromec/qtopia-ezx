/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: qos_cfg_names.h,v 1.60 2009/01/20 19:57:52 svaidhya Exp $
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

#define QOS_USER_AGENT_PROFILE_ROOT "config.MediaDelivery.UserAgentSettings"
#define QOS_USER_AGENT_PROFILE_ROOT_LEN 38
#define DEFAULT_USER_AGENT          "Default"
#define DEFAULT_USER_AGENT_LEN       7

/* Flow */
#define QOS_CFG_MDP                   "UseServerSideRateAdaptation" /* string */
#define QOS_CFG_MDP_FOR_LIVE          "UseServerSideRateAdaptationForLive" /* string */
#define QOS_CFG_RTT_PROBE_FREQUENCY   "RTTProbeFrequency"           /* msec */
#define QOS_CFG_RDT_PKT_AGG_ENABLE    "EnableRDTPacketAggregation"  /* bool */
#define QOS_CFG_PREFER_RTP            "PreferRTP"                   /* BOOL */
#define QOS_CFG_PREFER_TCP            "PreferTCP"                   /* BOOL */

/* ClientCapabilties: */
/* Stream bandwidth values (b=AS) */
/* BandwidthProtocol describes the transport protocol to be used for
 * bandwidth estimation ("UDP" or "TCP") */
#define QOS_CFG_CC_BW_PROT          "ClientCapabilities.BandwidthProtocol"   /* string */
#define QOS_CFG_CC_BW_MULT          "ClientCapabilities.BandwidthMultiplier" /* % */
#define QOS_CFG_CC_BUF_USAGE_LIMIT  "ClientCapabilities.BufferUsageLimit"    /* int, % */
#define DEF_BUF_USAGE_LIMIT         98

/* ClientCapabilties.CapabilityExchange: */
#define QOS_CFG_CC_CE_LOCAL_RDF         "ClientCapabilities.CapabilityExchange.LocalRDF"                  /* string */

#define QOS_CFG_CC_CE_RET_XWAP_PROFILE  "ClientCapabilities.CapabilityExchange.RetrieveXWAPProfiles"      /* bool */
#define DEFAULT_RET_XWAP_PROFILE        1

#define QOS_CFG_CC_CE_VPREDEC_BUF_SIZE  "ClientCapabilities.CapabilityExchange.VideoPreDecoderBufferSize" /* bytes */
#define DEFAULT_VPREDEC_BUF_SIZE        51200

#define QOS_CFG_CC_CE_VDEC_BYTE_RATE    "ClientCapabilities.CapabilityExchange.VideoDecodeByteRate"       /* bytes */
#define MIN_VDEC_BYTE_RATE              8000

#define QOS_CFG_CC_CE_PSS_VERSION       "ClientCapabilities.CapabilityExchange.PssVersion"                /* string */
#define DEFAULT_PSS_VERSION             "3GPP-R5"

/* RateControl: */
/* RTCP RR and RS rate/ratio (b=RR, b=RS) */
#define QOS_CFG_RATECONTROL_RR_RATIO "RateControl.RTCPRRrate" /* x.x%, xxbps, xx */
#define QOS_CFG_RATECONTROL_RS_RATIO "RateControl.RTCPRSrate" /* x.x%, xxbps, xx */

#define QOS_CFG_RC_TYPE         "RateControl.UDPCongestionControlType" /* string */
#define QOS_CFG_RC_MAX_BURST    "RateControl.MaxBurst"                 /* packets */
#define QOS_CFG_RC_INIT_PKTSZ   "RateControl.InitialPacketSize"        /* Bytes */
#define QOS_CFG_RC_MAX_SENDRATE "RateControl.MaxSendRate"              /* bps */
#define QOS_CFG_RC_MAX_OSR      "RateControl.MaxOversendRate"          /* % */
#define QOS_CFG_RC_INIT_OSR     "RateControl.InitialOversendRate"      /* % */
#define QOS_CFG_RC_USE_SDB      "RateControl.UseClientRateReq"         /* bool */

/* Feedback Timeout: */
#define QOS_CFG_RC_FEEDBACK_TIMEOUT         "RateControl.FeedbackTimeout"               /* msec */
#define QOS_CFG_RC_DOWNSHIFT_ON_FB_TIMEOUT  "RateControl.DownshiftOnFeedbackTimeout"    /* bool */

/* Packet Loss Discrimination Algorithm */
#define QOS_CFG_RC_LDA_ENABLE               "RateControl.EnableLossDiscrimination"      /* bool */
#define QOS_CFG_RC_LDA_SMOOTHING_VALUE      "RateControl.LDASmoothingValue"	/* integer multiple of 2 */

/* RateControl.TCP: */
#define QOS_CFG_TCP_HONOR_MAX_SENDRATE  "RateControl.TCP.HonorMaxSendRate"  /* bool */

/* RateControl.TFRC: */
#define QOS_CFG_TFRC_RTT_IIR_FILTER "RateControl.TFRC.EnableIIRForRTT"      /* bool */
#define QOS_CFG_TFRC_RATE_INC       "RateControl.TFRC.RateIncreaseLimit"    /* % */
#define QOS_CFG_TFRC_ACK_SZ         "RateControl.TFRC.TCPPacketsPerACK"     /* integer */

/* RateControl.BCC: */
#define QOS_CFG_BCC_RTT_IIR_FILTER "RateControl.BCC.EnableIIRForRTT"        /* bool */
#define QOS_CFG_BCC_INC_COEF       "RateControl.BCC.IncreaseCoefficient"    /* integer */
#define QOS_CFG_BCC_DEC_COEF       "RateControl.BCC.DecreaseCoefficient"    /* integer */
#define QOS_CFG_BCC_INC_EXP        "RateControl.BCC.IncreaseExponent"       /* float */
#define QOS_CFG_BCC_DEC_EXP        "RateControl.BCC.DecreaseExponent"       /* float */
#define QOS_CFG_BCC_INC_THR        "RateControl.BCC.IncreaseThreshold"      /* integer */
#define QOS_CFG_BCC_DEC_THR        "RateControl.BCC.DecreaseThreshold"      /* integer */
#define QOS_CFG_BCC_LOW_LIM        "RateControl.BCC.LowerLimit"             /* integer;bps */
#define QOS_CFG_BCC_TGT_RIO        "RateControl.BCC.TargetRatio"            /* integer */

/* RateAdaptation */
#define QOS_CFG_RA_DEFAULT_MEDIARATE   "RateAdaptation.DefaultMediaRate"            /* bps */
#define QOS_CFG_RA_ENABLE_AUDIO_SW     "RateAdaptation.EnableAudioRateSwitching"    /* bool */
#define QOS_CFG_RA_USE_REL6_RATE_SEL   "RateAdaptation.UseRel6InitialRateSelection" /* string */
#define QOS_CFG_RA_TYPE                "RateAdaptation.RateManagerType"             /* string */
#define QOS_CFG_RA_BUFF_MOD            "RateAdaptation.BufferModel"                 /* string */
#define QOS_CFG_RA_ENABLE_DELIVER_BELOW_LOWEST_MEDIA_RATE "RateControl.DeliverBelowLowestMediaRate"    /* bool */
#define QOS_CFG_RA_ENABLE_STEPWISE_UPSHIFT      "RateAdaptation.EnableStepwiseUpshift"    /* bool */
#define QOS_CFG_RA_ENABLE_STEPWISE_DOWNSHIFT    "RateAdaptation.EnableStepwiseDownshift"  /* bool */
#define QOS_CFG_RA_UPSHIFT_CONFIRMED   "RateAdaptation.FeedbackIntervalsBeforeUpshift"    /* count */

#define QOS_CFG_RC_SLOWDOWN_RATE       "RateAdaptation.SlowdownRatePercentage" /* % */

/* RateAdaptation.ClientAdaptation */
/* LinkChar support */
#define CONFIG_ENTRY_CLIENT_REPORTED_UPSHIFTING   "RateAdaptation.ClientAdaptation.EnableClientReportedUpshift"  /* bool */
#define CONFIG_ENTRY_CLIENT_REPORTED_DOWNSHIFTING "RateAdaptation.ClientAdaptation.EnableClientReportedDownshift"/* bool */
#define QOS_CFG_LINKCHAR_MULTIPLIER         "RateAdaptation.ClientAdaptation.LinkCharMultiplier" /* e.g. 1000 to convert kbps -> bps */
/* Adaptation support */
#define QOS_CFG_RA_CA_ENABLE_HA             "RateAdaptation.ClientAdaptation.EnableHelixAdaptation" /* bool */
#define QOS_CFG_RA_CA_ENABLE_3GPPA          "RateAdaptation.ClientAdaptation.Enable3GPPAdaptation"  /* bool */
/* RDT Buffer Info Ratio */
#define QOS_CFG_CC_BUFF         "RateAdaptation.ClientAdaptation.RDTBufferInfoRatio"    /* packets */

/* New rate manager type. See the RM Improvement SOD for full details */
static const char* QOS_CFG_RA_TYPE_UNBLOCKING = "unblocking";

/* Debug Variable in config file works on masking of bits */
static const char* DEBUG_OUTPUT = "DebugOutput";
/* Mask for each debug variable */
static const UINT32 DUMP_ALL              = 0x0001;
static const UINT32 DUMP_PKT              = 0x0002;
static const UINT32 DUMP_SELECTION        = 0x0004;
static const UINT32 DUMP_ALLRATE          = 0x0008;
static const UINT32 DUMP_ADAPTATION       = 0x0010;
static const UINT32 DUMP_ASMHANDLING      = 0x0020;
static const UINT32 DUMP_RM_TRACE         = 0x0040;
static const UINT32 DUMP_RC_TRACE         = 0x0080;
static const UINT32 DUMP_CHANNELRATE      = 0x0100;

#define NADU_REPORT_FREQUENCY_CFG "config.StreamAdaptation.NADUReportFrequency"

#endif /*_QOS_CFG_NAMES_H_ */
