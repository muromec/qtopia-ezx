/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: defslice.h,v 1.20 2009/03/10 18:00:05 dcollins Exp $
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

#ifndef _DEFSLICE_H
#define _DEFSLICE_H

#include "hxtypes.h"

//
// Default license values
//

// License Section
const INT32     LICENSE_EVALUATION          = 0;
const INT32     LICENSE_PRODUCT_ID          = 0;
const INT32     LICENSE_MAJOR_VERSION       = 6;
const INT32     LICENSE_MINOR_VERSION       = 0;
#define         LICENSE_STARTDATE             "None"
#define         LICENSE_ENDDATE               "None"
#define         LICENSE_OEM                   "RealNetworks"
#define         LICENSE_MANUFACTURER          "RealNetworks"
#define         LICENSE_DISTRIBUTION          "Online"
#define         LICENSE_LICENSE_ID            "0"

// Security Section
const INT32     LICENSE_SEC_IPRESTRICTION   = 0;
#define         LICENSE_SEC_FORCEPLATCHECK    "All"

// General Section
const INT32     LICENSE_CLIENT_CONNECTIONS  = 10;
const INT32     LICENSE_MOBILE_DOWNLOAD     = 0;
const INT32     LICENSE_ON_DEMAND           = 1;
const INT32     LICENSE_LIVE                = 1;
const INT32     LICENSE_VALID_PLAYERS_ONLY  = 0;
const INT32     LICENSE_INTRANET_SYSTEM     = 0;
const INT32     LICENSE_WINDOWS_MEDIA_LIVE  = 0;
const INT32     LICENSE_MMS_V_HTTP          = 0;
const INT32     LICENSE_TCP_PREF_ENABLED    = 0;
const INT32     LICENSE_HRA_ENABLED         = 0;
const INT32     LICENSE_SNMP_ENABLED        = 0;
const INT32     LICENSE_MDP_ENABLED         = 0;

// Multicast Section
const INT32     LICENSE_SCALABLE_MULTICAST  = 0;
const INT32     LICENSE_GENERAL_MULTICAST   = 0;
const INT32     LICENSE_WINDOWS_MEDIA_MULTICAST = 0;

// Datatypes Section
const INT32     LICENSE_REALEVENTS_ENABLED  = 1;
const INT32     LICENSE_REALAUDIO_ENABLED   = 1;
const INT32     LICENSE_REALVIDEO_ENABLED   = 1;
const INT32     LICENSE_MULTIRATE_ENABLED   = 1;
const INT32     LICENSE_REALTEXT_ENABLED    = 1;
const INT32     LICENSE_REALFLASH_ENABLED   = 0;
const INT32     LICENSE_REALPIX_ENABLED     = 1;
const INT32     LICENSE_REALMPA_ENABLED     = 1;
const INT32     LICENSE_QUICKTIME_ENABLED   = 1;
const INT32     LICENSE_REALMPV1_ENABLED    = 1;
const INT32     LICENSE_REALMPV2_ENABLED    = 0;
const INT32     LICENSE_REALMP4_ENABLED     = 0;
const INT32     LICENSE_3GPP_REL6_ENABLED   = 0;
const INT32     LICENSE_WINDOWSMEDIAASF_ENABLED = 0;

// Packetizers
const INT32	LICENSE_3GPPPACKETIZER_ENABLED	= 0;


//proxy
const INT32     LICENSE_RTSPPROXY_ENABLED          = 0;
const INT32     LICENSE_RTSPPROXY_REDIRECT_ENABLED = 0;
const INT32     LICENSE_PNAPROXY_ENABLED           = 0;
const INT32     LICENSE_PNAPPROXY_REDIRECT_ENABLED = 0;
const INT32     LICENSE_PROXYMII_ENABLED           = 1;

//Edge
const INT32     LICENSE_CONTENTREDIR_ENABLED       = 0;
const INT32     LICENSE_MULTICAST_RECEIVER		   = 0;

// Cache Section
const INT32     LICENSE_CACHE_ALLOWSTREAMS  = 0;
const INT32     LICENSE_CACHE_PROXY         = 0;

// Authentication Section
const INT32     LICENSE_AUTH_PPV_ACCESS     = 0;
const INT32     LICENSE_AUTH_PPV_PERMS      = 0;
const INT32     LICENSE_AUTH_BASIC          = 1;
const INT32     LICENSE_AUTH_MSQL           = 0;
const INT32     LICENSE_AUTH_MYSQL          = 0;
const INT32     LICENSE_AUTH_ODBC           = 0;

// Splitting Section
const INT32     LICENSE_SPLITTER_PULL_SOURCE_ENABLED  = 1;
const INT32     LICENSE_SPLITTER_PULL_RECEIVER_ENABLED  = 0;
const INT32     LICENSE_SPLITTER_PUSH_SOURCE_ENABLED  = 0;
const INT32     LICENSE_SPLITTER_PUSH_RECEIVER_ENABLED  = 0;

// BroadcastNG Section
const INT32     LICENSE_BROADCAST_PULL_SOURCE_ENABLED  = 0;
const INT32     LICENSE_BROADCAST_PULL_RECEIVER_ENABLED  = 0;
const INT32     LICENSE_BROADCAST_PROXY_PULL_RECEIVER_ENABLED = 0;
const INT32     LICENSE_BROADCAST_PUSH_SOURCE_ENABLED  = 0;
const INT32     LICENSE_BROADCAST_PUSH_RECEIVER_ENABLED  = 0;
const INT32     LICENSE_BROADCAST_MULTICAST_ENABLED = 0;

// Broadcast Redundancy Section
const INT32     LICENSE_BROADCAST_REDUNDANCY_ENABLED = 1;

// Low Latency Live Section
const INT32     LICENSE_BROADCAST_LIVE_LOW_LATENCY_ENABLED = 0;

// ISP Hosting Section
const INT32     LICENSE_ISP_HOST_ENABLED    = 0;

// Administration Section
const INT32     LICENSE_ADMIN_REMOTE        = 1;
const INT32     LICENSE_ADMIN_ADVANCED      = 1;

// Ad Serving Section
const INT32     LICENSE_ADS_ENABLED	    = 0;
const INT32     LICENSE_ADS_FLEXIBLE	    = 0;
const INT32     LICENSE_ADS_DISABLE_SPONS   = 0;
const INT32     LICENSE_ADS_ALLOW_EMBEDDED  = 0;

// Distributed Licensing Section
const INT32	LICENSE_DISTLIC_PUBLISHER_ENABLED	= 0;
const INT32	LICENSE_DISTLIC_SUBSCRIBER_ENABLED	= 1;

// DataConversion
const INT32	LICENSE_DATA_CONVERT_ENABLED	= 1;

// Content Distribution
const INT32	LICENSE_CDIST_PUBLISHER		= 0;
const INT32	LICENSE_CDIST_SUBSCRIBER	= 0;

//Fast Channel Switching 
const INT32     LICENSE_FCS_ENABLED             = 0;

//server side playlist
const INT32     LICENSE_SSPL_ENABLED             = 0;
const INT32     LICENSE_SSPL_SEEK_ENABLED             = 0;
const INT32     LICENSE_CONTENT_MGMT_ENABLED    =0;

//
// Registry path defines
//

// Special license paths
#define REGISTRY_NUM_LICENSES "license.NumLicenses"
#define REGISTRY_SERVER_ID    "license.License%ld.License.Definition.LicenseID"

// General Section
#define REGISTRY_CLIENT_CONNECTIONS "license.Summary.General.ClientConnections"
#define REGISTRY_BANDWIDTH_CAPACITY "license.Summary.General.BandwidthCapacity"
#define REGISTRY_MOBILE_DOWNLOAD    "license.Summary.General.MobileDownload"
#define REGISTRY_ON_DEMAND          "license.Summary.General.On-Demand"
#define REGISTRY_LIVE               "license.Summary.General.Live"
#define REGISTRY_VALID_PLAYERS_ONLY "license.Summary.General.ValidPlayersOnly"
#define REGISTRY_INTRANET_SYSTEM    "license.Summary.General.IntranetSystem"
#define REGISTRY_ADDTAG_SMILGEN	    "license.Summary.General.AddTag-SmilGen"
#define REGISTRY_WINDOWS_MEDIA_LIVE "license.Summary.General.WindowsMediaLive"
#define REGISTRY_MMS_V_HTTP         "license.Summary.General.MMSvHTTP"
#define REGISTRY_TCP_PREF_ENABLED   "license.Summary.General.TCPPreference"
#define REGISTRY_HRA_ENABLED        "license.Summary.General.HelixRateAdaptation"
#define REGISTRY_SNMP_ENABLED       "license.Summary.General.SNMP"
#define REGISTRY_MDP_ENABLED        "license.Summary.General.ServerSideRateAdaptation"

// Multicast Section
#define REGISTRY_SCALABLE_MULTICAST "license.Summary.Multicast.Scalable"
#define REGISTRY_GENERAL_MULTICAST  "license.Summary.Multicast.General"
#define REGISTRY_WINDOWS_MEDIA_MULTICAST "license.Summary.Multicast.WindowsMedia"

// Datatypes Section
#define REGISTRY_REALEVENTS_ENABLED "license.Summary.Datatypes.RealEvents.Enabled"
#define REGISTRY_REALAUDIO_ENABLED  "license.Summary.Datatypes.RealAudio.Enabled"
#define REGISTRY_REALVIDEO_ENABLED  "license.Summary.Datatypes.RealVideo.Enabled"
#define REGISTRY_MULTIRATE_ENABLED  "license.Summary.Datatypes.MultiRateContainer.Enabled"
#define REGISTRY_REALTEXT_ENABLED   "license.Summary.Datatypes.RealText.Enabled"
#define REGISTRY_QUICKTIME_ENABLED  "license.Summary.Datatypes.QuickTime.Enabled"
#define REGISTRY_3GPP_REL6_ENABLED  "license.Summary.Datatypes.3GPPRel6FileFormat.Enabled"
#define REGISTRY_REALFLASH_ENABLED  "license.Summary.Datatypes.RealFlash.Enabled"
#define REGISTRY_REALPIX_ENABLED    "license.Summary.Datatypes.RealPix.Enabled"
#define REGISTRY_REALMPA_ENABLED    "license.Summary.Datatypes.RealMPA.Enabled"
#define REGISTRY_QUICKTIME_ENABLED  "license.Summary.Datatypes.QuickTime.Enabled"
#define REGISTRY_REALMPV1_ENABLED   "license.Summary.Datatypes.RealMPEG1.Enabled"
#define REGISTRY_REALMPV2_ENABLED   "license.Summary.Datatypes.RealMPEG2.Enabled"
#define REGISTRY_REALMPV1_OLD_ENABLED   "license.Summary.realmpeg1.Enabled"
#define REGISTRY_REALMPV2_OLD_ENABLED   "license.Summary.realmpeg2.Enabled"
#define REGISTRY_REALMP4_ENABLED    "license.Summary.Datatypes.RealMPEG4.Enabled"
#define REGISTRY_WINDOWSMEDIAASF_ENABLED  "license.Summary.Datatypes.WindowsMediaASF.Enabled"

// Packeizers
#define REGISTRY_3GPPPACKETIZER_ENABLED    "license.Summary.Datatypes.3GPPPacketizer.Enabled"


//Proxy
#define REGISTRY_RTSPPROXY_ENABLED  "license.Summary.Proxy.RTSP.Enabled"
#define REGISTRY_RTSPPROXY_REDIRECT_ENABLED  "license.Summary.Proxy.RTSP.Redirect.Enabled"
#define REGISTRY_PNAPROXY_ENABLED   "license.Summary.Proxy.PNA.Enabled"
#define REGISTRY_PNAPROXY_REDIRECT_ENABLED  "license.Summary.Proxy.PNA.Redirect.Enabled"
#define REGISTRY_PROXYMII_ENABLED   "license.Summary.Proxy.MediaImport.Enabled"

//Edge
#define REGISTRY_CONTENTREDIR_ENABLED  "license.Summary.EdgeServer.ContentRedirector.Enabled"
#define REGISTRY_MULTICAST_RECEIVER    "license.Summary.EdgeServer.MulticastReceiver.Enabled"

// Authentication Section
#define REGISTRY_AUTH_PPV_ACCESS    "license.Summary.Authentication.Commerce.Access"
#define REGISTRY_AUTH_PPV_PERMS     "license.Summary.Authentication.Commerce.Permissions"
#define REGISTRY_AUTH_BASIC         "license.Summary.Authentication.Storage.Basic"
#define REGISTRY_AUTH_MSQL          "license.Summary.Authentication.Storage.mSQL"
#define REGISTRY_AUTH_MYSQL         "license.Summary.Authentication.Storage.MySQL"
#define REGISTRY_AUTH_ODBC          "license.Summary.Authentication.Storage.Odbc"
#define REGISTRY_AUTH_CONN          "license.Summary.Authentication.Commerce.ClientConnections"

// Splitting Section
#define REGISTRY_SPLITTER_PULL_SOURCE   \
                        "license.Summary.Splitter.Pull.Source"
#define REGISTRY_SPLITTER_PULL_RECEIVER \
                        "license.Summary.Splitter.Pull.Receiver"
#define REGISTRY_SPLITTER_PUSH_SOURCE   \
                        "license.Summary.Splitter.Push.Source"
#define REGISTRY_SPLITTER_PUSH_RECEIVER \
                        "license.Summary.Splitter.Push.Receiver"

// Splitting NG Section
#define REGISTRY_BROADCAST_PULL_SOURCE   \
		"license.Summary.BroadcastDistribution.PullTransmissionEnabled"
#define REGISTRY_BROADCAST_PULL_RECEIVER \
		"license.Summary.BroadcastDistribution.PullReceiveEnabled"
#define REGISTRY_BROADCAST_PROXY_PULL_RECEIVER \
		"license.Summary.BroadcastDistribution.ProxyPullSplitReceiveEnabled"
#define REGISTRY_BROADCAST_PUSH_SOURCE   \
		"license.Summary.BroadcastDistribution.PushTransmissionEnabled"
#define REGISTRY_BROADCAST_PUSH_RECEIVER \
		"license.Summary.BroadcastDistribution.PushReceiveEnabled"
#define REGISTRY_BROADCAST_MULTICAST \
		"license.Summary.BroadcastDistribution.EnableMulticastTransport"

// Broadcast Redunancy Section
#define REGISTRY_BROADCAST_REDUNDANCY   \
		"license.Summary.BroadcastRedundancy.Enabled"

// Low Latency Live Section
#define REGISTRY_BROADCAST_LIVE_LOW_LATENCY  \
        "license.Summary.LowLatencyLive.Enabled"
#define REGISTRY_BROADCAST_LIVE_LOW_LATENCY_ALT  \
        "license.Summary.LowLatencyLive.LowLatencyLive"

// ISP Hosting Section
#define REGISTRY_ISP_HOST_ENABLED   "license.Summary.ISP-Hosting.Enabled"

// Ad Serving Section
#define REGISTRY_ADS_ENABLED	    "license.Summary.Ad Serving.Enabled"
#define REGISTRY_ADS_FLEXIBLE	    "license.Summary.Ad Serving.FlexibleAdInsertion"
#define REGISTRY_ADS_DISABLE_SPONS  "license.Summary.Ad Serving.DisableTetheredAds"
#define REGISTRY_ADS_ALLOW_EMBEDDED "license.Summary.Ad Serving.EmbeddedPlayersAllowed"
#define REGISTRY_ADS_CONN "license.Summary.Ad Serving.ClientConnections"

// Distributed Licensing
#define REGISTRY_DISTLIC_PUBLISHER_ENABLED	"license.Summary.DistributedLicensing.Publisher.Enabled"
#define REGISTRY_DISTLIC_SUBSCRIBER_ENABLED	"license.Summary.DistributedLicensing.Subscriber.Enabled"

// Distributed Licensing
#define REGISTRY_DATA_CONVERT_ENABLED	"license.Summary.DataConversion.Enabled"

// Content Distribution
#define REGISTRY_CDIST_PUBLISHER	"license.Summary.ContentDistribution.Publisher"
#define REGISTRY_CDIST_SUBSCRIBER	"license.Summary.ContentDistribution.Subscriber"

// Fast Channel Switching
#define REGISTRY_FCS_ENABLED           "license.Summary.FastChannelSwitching.Enabled"

//Server side playlist
#define REGISTRY_SSPL_ENABLED	        "license.Summary.DataTypes.HelixPlaylist.Enabled"
#define REGISTRY_SSPL_SEEK_ENABLED	        "license.Summary.ServersidePlaylist.Enabled"
#define REGISTRY_CONTENT_MGMT_ENABLED    "license.Summary.FileSystemControl.Enabled"

// Templatized Logging
#define REGISTRY_TEMPLATIZED_LOGGING_ENABLED "license.Summary.TemplateLogging.Enabled"

// RTSP Event Logging
#define REGISTRY_RTSP_EVENTSTATS_ENABLED    "license.Summary.RTSPEventStats.Enabled"

// SDPGen Plugin
#define REGISTRY_SDPGEN_ENABLED             "license.Summary.SDPFileGen.Enabled"


/* File:
 *	Used by licrequest.cpp(servutil)
 *
 * Description:
 *
 *  IHXLicenseRequestResponse - Response interface for async license 
 *  request
 */

#ifdef GUID_DEFINED
// {b8676e90-625c-11d4-968500c0f031f80f}
DEFINE_GUID( IID_IHXLicenseRequestResponse, 
    0xb8676e90, 0x625c, 0x11d4, 0x96, 0x85, 0x00, 0xc0, 0xf0, 0x31, 0xf8, 0x0f);

#undef  INTERFACE
#define INTERFACE IHXLicenseRequestResponse

DECLARE_INTERFACE(IHXLicenseRequestResponse)
{
    // *** IUnknown methods ***
    STDMETHOD (QueryInterface )     (THIS_ 
	    REFIID ID,
	    void** ppInterfaceObj) PURE;
    STDMETHOD_(UINT32, AddRef )     (THIS) PURE;
    STDMETHOD_(UINT32, Release)     (THIS) PURE;
    STDMETHOD(RequestComplete)(THIS_ HX_RESULT status) PURE;
};
#endif

#endif // _DEFSLICE_H
