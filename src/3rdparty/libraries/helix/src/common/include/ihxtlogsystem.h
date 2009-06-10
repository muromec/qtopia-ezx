/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: ihxtlogsystem.h,v 1.54 2009/05/06 02:48:50 vkathuria Exp $
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

#ifndef IHXTLOGSYSTEM_H
#define IHXTLOGSYSTEM_H

#include "ihxpckts.h"
#include "hlxclib/stdarg.h"

enum EHXTLogCode
{
    // Producer log codes
    //
    // Messages that application end-users or
    // 3rd party SDK developers will see
    LC_APP_DIAG  = 0x000000F0, // Less important/diagnostic messages (only interesting if something goes wrong)
    LC_APP_INFO  = 0x7F000000, // Very important messages (always want to see these messages)
    LC_APP_WARN  = 0x00FF0000, // There was a problem, but it was handled and everything is probably ok
    LC_APP_ERROR = 0x0000FF00, // There was a problem -- it wasn't handled

    // Messages that SDK users will see --
    // ok to mention IHX/IHXT interfaces
    LC_SDK_DIAG  = 0x000000F2, // Less important/diagnostic messages (only interesting if something goes wrong)
    LC_SDK_INFO  = 0x7f000002, // Very important messages (always want to see these messages)
    LC_SDK_WARN  = 0x00FF0002, // There was a problem, but it was handled and everything is probably ok
    LC_SDK_ERROR = 0x0000FF02, // There was a problem -- it wasn't handled

    // Messages that are only useful with the
    // corresponding source code -- ok to mention
    // internal classnames
    LC_DEV_DIAG  = 0x000000F1, // Less important/diagnostic messages (only interesting if something goes wrong)
    LC_DEV_INFO  = 0x7f000001, // Very important messages (always want to see these messages)  
    LC_DEV_WARN  = 0x00FF0001, // There was a problem, but it was handled and everything is probably ok
    LC_DEV_ERROR = 0x0000FF01, // There was a problem -- it wasn't handled

    // Client-related log codes
    LC_CLIENT_LEVEL1 = 0x00000001,
    LC_CLIENT_LEVEL2 = 0x00000002,
    LC_CLIENT_LEVEL3 = 0x00000004,
    LC_CLIENT_LEVEL4 = 0x00000008
};

enum EHXTLogCodeFilterMask
{
    SDK_MESSAGE_MASK    = 0x00000002,
    DIAG_MESSAGE_MASK   = 0x000000F0,
    ERROR_MESSAGE_MASK  = 0x0000FF00,
    WARN_MESSAGE_MASK   = 0x00FF0000,
    INFO_MESSAGE_MASK   = 0x7F000000,
    CLIENT_MSGMASK_ALL  = 0x0000000F,
    CLIENT_MSGMASK_L1_2 = 0x00000003,
    CLIENT_MSGMASK_L1_3 = 0x00000007
};

// emit 4cc in native order
#define HX_MAKE4CC(ch1,ch2,ch3,ch4) ( ((UINT32)(UINT8)(ch1) << 24) | ((UINT32)(UINT8)(ch2) << 16) | ((UINT32)(UINT8)(ch3) << 8) | (UINT32)(UINT8)(ch4) )


enum EHXTLogFuncArea
{
    // Producer-related functional areas
  
    // Note: Only add new items to the end of the producer
    // list -- otherwise the FAs will get out of sync
    // with already compiled/released code and logmessages.xml
    NONE = 0,
    ACTIVEX,
    AUDCODEC,
    AUDPREFIL,
    BCAST,
    CAPTURE,
    CMDLINE,
    FILEOUT,
    FILEREAD,
    GUI,
    JOBFILE,
    LIC,
    POSFIL,
    REMOTE,
    FA_SDK_CONFIG,  // SDK config objects (typically rmsession)
    FA_SDK_ENCODE,  // SDK encoding (typically encpipeline, streammanager)
    FA_SDK_CORE,    // RSCore, RSUtil, RSGraphManager stuff
    FA_GEN_FILTER,  // Generic filter msgs
#ifdef __CW32__
    HXLOG_STATS,
#else
    STATS, 
#endif
    VIDCODEC,
    VIDPREFIL,
    VIDRENDR,
    MEDIASAMPLES,
    PUB,

    // Client-related functional areas (in alphabetical order)
    //
    // When adding a fourCC to this list, make sure 
    // and also add it to the g_ClientLogging4CCList array below.
    HXLOG_AACF = HX_MAKE4CC('A','A','C','F'), // AAC File Format
    HXLOG_ADEV = HX_MAKE4CC('A','D','E','V'), // Audio device
    HXLOG_ASMX = HX_MAKE4CC('A','S','M','X'), // ASM
    HXLOG_AUDI = HX_MAKE4CC('A','U','D','I'), // RealAudio
    HXLOG_AUTH = HX_MAKE4CC('A','U','T','H'), // Authentication
    HXLOG_AUTO = HX_MAKE4CC('A','U','T','O'), // Auto-Update
    HXLOG_AVIX = HX_MAKE4CC('A','V','I','X'), // AVI 
    HXLOG_BAND = HX_MAKE4CC('B','A','N','D'), // Bandwidth manager
    HXLOG_BUFF = HX_MAKE4CC('B','U','F','F'), // Buffer Control
    HXLOG_CNKT = HX_MAKE4CC('C','N','K','T'), // hxclientkit
    HXLOG_CORE = HX_MAKE4CC('C','O','R','E'), // Core load time
    HXLOG_EVEN = HX_MAKE4CC('E','V','E','N'), // RealEvents
    HXLOG_FSRC = HX_MAKE4CC('F','S','R','C'), // File source
    HXLOG_GENE = HX_MAKE4CC('G','E','N','E'), // Generic
    HXLOG_GIFX = HX_MAKE4CC('G','I','F','X'), // GIF
    HXLOG_HTTP = HX_MAKE4CC('H','T','T','P'), // HTTP (httpfsys)
    HXLOG_JPEG = HX_MAKE4CC('J','P','E','G'), // JPEG
    HXLOG_MP3X = HX_MAKE4CC('M','P','3','X'), // MP3
    HXLOG_NETW = HX_MAKE4CC('N','E','T','W'), // Network socket
    HXLOG_NSRC = HX_MAKE4CC('N','S','R','C'), // Network source
    HXLOG_PIXX = HX_MAKE4CC('P','I','X','X'), // RealPix
    HXLOG_PNAX = HX_MAKE4CC('P','N','A','X'), // PNA protocol
    HXLOG_PROT = HX_MAKE4CC('P','R','O','T'), // Protocol generic
    HXLOG_RECF = HX_MAKE4CC('R','E','C','F'), // Superbuffer Record File.
    HXLOG_RECO = HX_MAKE4CC('R','E','C','O'), // Reconnect
    HXLOG_RECS = HX_MAKE4CC('R','E','C','S'), // Record Service And Superbuffer
    HXLOG_RTSP = HX_MAKE4CC('R','T','S','P'), // RTSP protocol
    HXLOG_SITE = HX_MAKE4CC('S','I','T','E'), // Site
    HXLOG_SMIL = HX_MAKE4CC('S','M','I','L'), // SMIL
    HXLOG_STRE = HX_MAKE4CC('S','T','R','E'), // Stream source map
    HXLOG_SWFX = HX_MAKE4CC('S','W','F','X'), // Flash
    HXLOG_THRD = HX_MAKE4CC('T','H','R','D'), // Threading
    HXLOG_TLCX = HX_MAKE4CC('T','L','C','X'), // Top-Level Client
    HXLOG_TRAN = HX_MAKE4CC('T','R','A','N'), // Transport
    HXLOG_TRIK = HX_MAKE4CC('T','R','I','K'), // TrickPlay
    HXLOG_TURB = HX_MAKE4CC('T','U','R','B'), // TurboPlay
    HXLOG_VIDE = HX_MAKE4CC('V','I','D','E'), // RealVideo
    HXLOG_LLLX = HX_MAKE4CC('L','L','L','X'), // Low Latency Live
    HXLOG_MPGF = HX_MAKE4CC('M','P','G','F'), // MPEG File Format
    HXLOG_CBWI = HX_MAKE4CC('C','B','W','I'), // Connection Bandwidth Info
    HXLOG_H264 = HX_MAKE4CC('H','2','6','4'), // H.264 
    HXLOG_FILE = HX_MAKE4CC('F','I','L','E')  // FILE I/O
    ,HXLOG_SMMF = HX_MAKE4CC('S','M','M','F') // Symbian MMF
    ,HXLOG_ASFF = HX_MAKE4CC('A','S','F','F')  // ASF file format
    ,HXLOG_WMA9 = HX_MAKE4CC('W','M','A','9')  // WMA 9 audio codec
    ,HXLOG_WMAA = HX_MAKE4CC('W','M','A','A')  // WMA 10 audio codec    
    ,HXLOG_WMAV = HX_MAKE4CC('W','M','A','V')  // WMA 9 Voice audio codec
    ,HXLOG_WMAR = HX_MAKE4CC('W','M','A','R')  // WMA renderer
    ,HXLOG_BAUD = HX_MAKE4CC('B','A','U','D')  // Base audio renderer
    ,HXLOG_WMV8 = HX_MAKE4CC('W','M','V','8')  // WMV 7/8 video codec
    ,HXLOG_WMV9 = HX_MAKE4CC('W','M','V','9')  // WMV 9 video codec
    ,HXLOG_WMVR = HX_MAKE4CC('W','M','V','R')  // WMV renderer
    ,HXLOG_BVID = HX_MAKE4CC('B','V','I','D')  // Base video renderer
    ,HXLOG_MDFA = HX_MAKE4CC('M','D','F','A')  // Symbian MDF DSP Audio
    ,HXLOG_SDPX = HX_MAKE4CC('S','D','P','X')  // SDP
    ,HXLOG_HOST = HX_MAKE4CC('H','O','S','T')  // Base Hosted Renderer
    ,HXLOG_WMPR = HX_MAKE4CC('W','M','P','R')  // Windows Media Player hosted renderer
    ,HXLOG_TONE = HX_MAKE4CC('T','O','N','E')  // TONE Sequence Plugin
    ,HXLOG_RTSF = HX_MAKE4CC('R','T','S','F')  // RTSP File Format Plugin
    ,HXLOG_NGTF = HX_MAKE4CC('N','G','T','F')  // Media Nugget File Format Plugin
    ,HXLOG_FPHR = HX_MAKE4CC('F','P','H','R')  // Flash player hosted renderer
    ,HXLOG_CORP = HX_MAKE4CC('C','O','R','P')  // Core performance logging
    ,HXLOG_BMET = HX_MAKE4CC('B','M','E','T')  // Base meta renderer
    ,HXLOG_SXPS = HX_MAKE4CC('S','X','P','S') // Symbian eXternal Packet Source
    ,HXLOG_ASXR = HX_MAKE4CC('A','S','X','R')  // ASX meta renderer
    ,HXLOG_WMER = HX_MAKE4CC('W','M','E','R')  // WM events renderer
    ,HXLOG_FLVF = HX_MAKE4CC('F','L','V','F')  // FLV file format plugin
    ,HXLOG_FLVR = HX_MAKE4CC('F','L','V','R')  // FLV renderer
    ,HXLOG_DTDR = HX_MAKE4CC('D','T','D','R')  // dtdrive
    ,HXLOG_HXPE = HX_MAKE4CC('H','X','P','E')  // Helix Playback Engine(player_rn/kit/realplayer)
    ,HXLOG_COOK = HX_MAKE4CC('C','O','O','K')  // Helix Cookie DB
    ,HXLOG_HXAU = HX_MAKE4CC('H','X','A','U')  // Helix AutoUpgrade manager
    ,HXLOG_HXDL = HX_MAKE4CC('H','X','D','L')  // Helix Download manager
    ,HXLOG_ENON = HX_MAKE4CC('E','N','O','N')  // Encoding services - NONE
    ,HXLOG_EACT = HX_MAKE4CC('E','A','C','T')  // Encoding services - ACTIVEX
    ,HXLOG_EAUC = HX_MAKE4CC('E','A','U','C')  // Encoding services - AUDCODEC
    ,HXLOG_EAUP = HX_MAKE4CC('E','A','U','P')  // Encoding services - AUDPREFIL
    ,HXLOG_EBCA = HX_MAKE4CC('E','B','C','A')  // Encoding services - BCAST
    ,HXLOG_ECAP = HX_MAKE4CC('E','C','A','P')  // Encoding services - CAPTURE
    ,HXLOG_ECMD = HX_MAKE4CC('E','C','M','D')  // Encoding services - CMDLINE
    ,HXLOG_EFLO = HX_MAKE4CC('E','F','L','O')  // Encoding services - FILEOUT
    ,HXLOG_EFLR = HX_MAKE4CC('E','F','L','R')  // Encoding services - FILEREAD
    ,HXLOG_EGUI = HX_MAKE4CC('E','G','U','I')  // Encoding services - GUI
    ,HXLOG_EJOB = HX_MAKE4CC('E','J','O','B')  // Encoding services - JOBFILE
    ,HXLOG_ELIC = HX_MAKE4CC('E','L','I','C')  // Encoding services - LIC
    ,HXLOG_EPOS = HX_MAKE4CC('E','P','O','S')  // Encoding services - POSFIL
    ,HXLOG_EREM = HX_MAKE4CC('E','R','E','M')  // Encoding services - REMOTE
    ,HXLOG_ECON = HX_MAKE4CC('E','C','O','N')  // Encoding services - FA_SDK_CONFIG
    ,HXLOG_EENC = HX_MAKE4CC('E','E','N','C')  // Encoding services - FA_SDK_ENCODE
    ,HXLOG_ECOR = HX_MAKE4CC('E','C','O','R')  // Encoding services - FA_SDK_CORE
    ,HXLOG_EFLT = HX_MAKE4CC('E','F','L','T')  // Encoding services - FA_GEN_FILTER
    ,HXLOG_ESTA = HX_MAKE4CC('E','S','T','A')  // Encoding services - STATS
    ,HXLOG_EVIC = HX_MAKE4CC('E','V','I','C')  // Encoding services - VIDCODEC
    ,HXLOG_EVIP = HX_MAKE4CC('E','V','I','P')  // Encoding services - VIDPREFIL
    ,HXLOG_EVIR = HX_MAKE4CC('E','V','I','R')  // Encoding services - VIDRENDR
    ,HXLOG_EMED = HX_MAKE4CC('E','M','E','D')  // Encoding services - MEDIASAMPLES
    ,HXLOG_EPUB = HX_MAKE4CC('E','P','U','B')  // Encoding services - PUB
    ,HXLOG_CAPF = HX_MAKE4CC('C','A','P','F')  // Capture file format
    ,HXLOG_AVCQ = HX_MAKE4CC('A','V','C','Q')  // Quicktime-based H.264 decoder
    ,HXLOG_MP4W = HX_MAKE4CC('M','P','4','W')  // MP4 file writer
    ,HXLOG_RVXV = HX_MAKE4CC('R','V','X','V')  // Real Video Decoder
};

static const struct _ClientLogging4CC
{
    UINT32      ul4CC;
    const char* psz4CCDescription;
} g_ClientLogging4CCList[] =
{
    {HXLOG_AACF, "AAC File Format"},
    {HXLOG_ADEV, "Audio device"},
    {HXLOG_ASMX, "ASM"},
    {HXLOG_AUDI, "RealAudio"},
    {HXLOG_AUTH, "Authentication"},
    {HXLOG_AUTO, "Auto-Update"},
    {HXLOG_AVIX, "AVI"},
    {HXLOG_BAND, "Bandwidth manager"},
    {HXLOG_BUFF, "Buffer Control"},
    {HXLOG_CORE, "Core load time"},
    {HXLOG_EVEN, "RealEvents"},
    {HXLOG_FSRC, "File source"},
    {HXLOG_GENE, "Generic"},
    {HXLOG_GIFX, "GIF"},
    {HXLOG_HTTP, "HTTP Filesystem"},
    {HXLOG_JPEG, "JPEG"},
    {HXLOG_MP3X, "MP3"},
    {HXLOG_NETW, "Network socket"},
    {HXLOG_NSRC, "Network source"},
    {HXLOG_PIXX, "RealPix"},
    {HXLOG_PNAX, "PNA protocol"},
    {HXLOG_PROT, "Generic protocol"},
    {HXLOG_RECO, "Reconnect"},
    {HXLOG_RTSP, "RTSP protocol"},
    {HXLOG_SITE, "Site"},
    {HXLOG_SMIL, "SMIL"},
    {HXLOG_STRE, "Stream source map"},
    {HXLOG_SWFX, "Macromedia Flash"},
    {HXLOG_THRD, "Threading"},
    {HXLOG_TRAN, "Transport"},
    {HXLOG_TRIK, "TrickPlay"},
    {HXLOG_TURB, "TurboPlay"},
    {HXLOG_VIDE, "RealVideo"},
    {HXLOG_LLLX, "Low Latency Live"},
    {HXLOG_MPGF, "MPEG file format"},
    {HXLOG_CBWI, "Connection bandwidth info"},
    {HXLOG_H264, "H.264"},
    {HXLOG_FILE, "FILE I/O"},
    {HXLOG_SMMF, "SymbianMMF"},
    {HXLOG_ASFF, "ASF File Format"},
    {HXLOG_WMA9, "Windows Media Audio 9 Codec"},
    {HXLOG_WMAA, "Windows Media Audio 10 Codec"}, 
    {HXLOG_WMAV, "Windows Media Audio 9 Voice Codec"}, 
    {HXLOG_WMAR, "Windows Media Audio Renderer"},
    {HXLOG_BAUD, "Base Audio Renderer"},
    {HXLOG_WMV8, "Windows Media Video 7/8 Codec"},
    {HXLOG_WMV9, "Windows Media Video 9 Codec"},
    {HXLOG_WMVR, "Windows Media Video Renderer"},
    {HXLOG_BVID, "Base Video Renderer"},
    {HXLOG_MDFA, "SymbianMDF-Aud"},
    {HXLOG_SDPX, "SDP"},
    {HXLOG_HOST, "Base Hosted Renderer"},
    {HXLOG_WMPR, "Windows Media Player Hosted Renderer"},
    {HXLOG_TONE, "Tone generator plugin"},
    {HXLOG_RTSF, "RTSP File Format"},
    {HXLOG_NGTF, "Media Nugget File Format"},
    {HXLOG_FPHR, "Flash Player Hosted Renderer"},
    {HXLOG_CORP, "Core performance"},
    {HXLOG_BMET, "Base Meta Renderer"},
    {HXLOG_ASXR, "ASX Renderer"},
    {HXLOG_WMER, "Windows Media Events Renderer"},
    {HXLOG_FLVF, "FLV File Format Plugin"},
    {HXLOG_FLVR, "FLV Renderer"},
    {HXLOG_DTDR, "Dtdrive Engine"},
    {HXLOG_HXPE, "Helix Playback Engine"},
    {HXLOG_COOK, "Helix Cookie DB"},
    {HXLOG_HXAU, "Helix AutoUpgrade"},
    {HXLOG_HXDL, "Helix Downloader"},
    {HXLOG_ENON, "Encoding Services None"},
    {HXLOG_EACT, "Encoding Services ActiveX Control"},
    {HXLOG_EAUC, "Encoding Services Audio Codec"},
    {HXLOG_EAUP, "Encoding Services Audio Pre-Filter"},
    {HXLOG_EBCA, "Encoding Services Broadcast"},
    {HXLOG_ECAP, "Encoding Services Capture"},
    {HXLOG_ECMD, "Encoding Services Command Line"},
    {HXLOG_EFLO, "Encoding Services File Output"},
    {HXLOG_EFLR, "Encoding Services File Reader"},
    {HXLOG_EGUI, "Encoding Services GUI"},
    {HXLOG_EJOB, "Encoding Services Job File"},
    {HXLOG_ELIC, "Encoding Services License"},
    {HXLOG_EPOS, "Encoding Services POSFILE"},
    {HXLOG_EREM, "Encoding Services Remote"},
    {HXLOG_ECON, "Encoding Services SDK Config Objects"},
    {HXLOG_EENC, "Encoding Services SDK Encoding"},
    {HXLOG_ECOR, "Encoding Services Core"},
    {HXLOG_EFLT, "Encoding Services Generic Filter"},
    {HXLOG_ESTA, "Encoding Services Stats"},
    {HXLOG_EVIC, "Encoding Services Video Codec"},
    {HXLOG_EVIP, "Encoding Services Video Pre-Filter"},
    {HXLOG_EVIR, "Encoding Services Video Rendering"},
    {HXLOG_EMED, "Encoding Services Media Samples"},
    {HXLOG_EPUB, "Encoding Services PUB"},
    {HXLOG_CAPF, "Capture File Format"},
    {HXLOG_AVCQ, "H.264 decoder based on QuickTime SDK"},
    {HXLOG_MP4W, "MP4 File Writer"},
    {HXLOG_RVXV, "Real Video Decoder"},
    {0,          NULL}  // This entry should ALWAYS remain the last entry
};

/****************************************************************************
 *
 *  Interface:
 *
 *  IHXTLogObserver
 *
 *  Purpose:
 *
 *      IID_IHXTLogObserver:
 *
 *  This interface must be implemented by and object registering with the log
 *  system to receive log messages.
 *
 *  // {EA6ABCF4-66EB-11d4-931A-00D0B749DE42}
 *
 */

DEFINE_GUID(IID_IHXTLogObserver, 
0xea6abcf4, 0x66eb, 0x11d4, 0x93, 0x1a, 0x0, 0xd0, 0xb7, 0x49, 0xde, 0x42);

#undef INTERFACE
#define INTERFACE IHXTLogObserver

DECLARE_INTERFACE_(IHXTLogObserver, IUnknown)
{
        /*
         *      IUnknown methods
         */
        STDMETHOD(QueryInterface)               (THIS_
                                REFIID riid,
                                void** ppvObj) PURE;

        STDMETHOD_(ULONG32,AddRef)              (THIS) PURE;

        STDMETHOD_(ULONG32,Release)             (THIS) PURE;

        /************************************************************************
         *      Method:
         *          IHXTLogObserverManager::OnEndService
         *      Purpose:
         *          The observer will receive this call as notification that the log system
         *                      is shutting down, and that all log messages have been delivered.
         *      Parameters:
         *          NONE
         *      Returns:
         *          Ignored.
         */
  STDMETHOD(OnEndService) (THIS) PURE;

        /************************************************************************
         *      Method:
         *          IHXTLogObserverManager::ReceiveMsg
         *      Purpose:
         *          Method called on an observer when a log message is sent to the log 
         *                      system that passes the observers filter.
         *      Parameters:
         *          szNamespace - [in] The namespace used to qualify the functional area
         *                                      and translated messge.
         *          nCode - [in] The important level of the message.
         *          unFuncArea - [in] numeric value used in the message as the functional area
         *          szFuncArea - [in] The translated string for the numeric functional area, if
         *                                      one was found.
         *          nTimeStamp - [in] The time (in milliseconds since midnight, Jan.1, 1970) at 
         *                                      which the log message was sent to the log system.
         *          nMsg - [in] The numeric value used to identify the message for translation. 
         *          szMsg - [in] The actual message text, either translation or the text sent
         *                                      to the LogMessage method.
         *          szJobName - [in] The name of the job from which the message originated.
         *      Returns:
         *          Ignored.
         */
  STDMETHOD(ReceiveMsg) (THIS_ 
                const char*                     /*IN*/                  szNamespace, 
                EHXTLogCode                                     /*IN*/                  nCode, 
                UINT32                                  /*IN*/                  unFuncArea, 
                const char*                     /*IN*/                  szFuncArea, 
                INT32                                           /*IN*/                  nTimeStamp, 
                UINT32                                  /*IN*/                  nMsg, 
                const char*             /*IN*/                  szMsg, 
                const char*             /*IN*/                  szJobName) PURE; 
};


/****************************************************************************
 *
 *  Interface:
 *
 *  IHXTLogObserver2
 *
 *  Purpose:
 *
 *      IID_IHXTLogObserver2:
 *
 *  This interface must be implemented by an object registering with the log
 *  system to receive log messages & support flusing.
 *
 *  // {68AFE313-BE30-4b46-BFAD-6F035E624C8A}
 *
 */

 // {68AFE313-BE30-4b46-BFAD-6F035E624C8A}
DEFINE_GUID(IID_IHXTLogObserver2, 
0x68afe313, 0xbe30, 0x4b46, 0xbf, 0xad, 0x6f, 0x3, 0x5e, 0x62, 0x4c, 0x8a);


#undef INTERFACE
#define INTERFACE IHXTLogObserver2

DECLARE_INTERFACE_(IHXTLogObserver2, IHXTLogObserver)
{
    /************************************************************************
     *  Method:
     *      IHXTLogObserver2::Flush
     *  Purpose:
     *      Called by LogObserverManager to inform the observer to flush any internal buffers.
     *  Returns:
     *      HXR_OK - If success.
     *      HXR_FAIL - failure.
     */
    STDMETHOD(Flush)() PURE;
};

/****************************************************************************
 *
 *  Interface:
 *
 *  IHXTLogObserver3
 *
 *  Purpose:
 *
 *      IID_IHXTLogObserver3:
 *
 *  This interface must be implemented by an object registering with the log
 *  system to receive message tick counts
 *
 *  // {6DB6A063-5CCD-4352-BEA3-66BC67EDB3B4}
 *
 */

DEFINE_GUID(IID_IHXTLogObserver3, 
0x6db6a063, 0x5ccd, 0x4352, 0xbe, 0xa3, 0x66, 0xbc, 0x67, 0xed, 0xb3, 0xb4);


#undef INTERFACE
#define INTERFACE IHXTLogObserver3

DECLARE_INTERFACE_(IHXTLogObserver3, IHXTLogObserver2)
{
    /************************************************************************
     * Method:
     *      IHXTLogObserver3::SetMsgTick()
     *  Purpose:
     *      Method called on an observer to set the system tick count
     *      when the log message it is about to receive entered the log system
     *  Parameters:
     *      ulTickCount - [in] System tick count when the next message
     *                         received by IHXTLogObserver::ReceiveMsg()
     *                         or IHXTLogContextObserver::ReceiveMsg()
     *                         entered the log system.
     *  Returns:
     *      Ignored.
     */
    STDMETHOD(SetMsgTick) (THIS_ UINT32 ulTickCount) PURE;
};


/****************************************************************************
 *
 *  Interface:
 *
 *  IHXTFuncAreaEnum
 *
 *  Purpose:
 *
 *  This is an enumeration interface which will enurmerate through all functional
 *  areas pre-loaded by the log system.  
 *
 *      IID_IHXTFuncAreaEnum:
 *
 *  // {938F4A21-1327-11d5-9349-00D0B749DE42}
 *
 */

DEFINE_GUID(IID_IHXTFuncAreaEnum, 
0x938f4a21, 0x1327, 0x11d5, 0x93, 0x49, 0x0, 0xd0, 0xb7, 0x49, 0xde, 0x42);

#undef INTERFACE
#define INTERFACE IHXTFuncAreaEnum

DECLARE_INTERFACE_(IHXTFuncAreaEnum, IUnknown)
{
        /*
         *      IUnknown methods
         */
        STDMETHOD(QueryInterface)               (THIS_
                                REFIID riid,
                                void** ppvObj) PURE;

        STDMETHOD_(ULONG32,AddRef)              (THIS) PURE;

        STDMETHOD_(ULONG32,Release)             (THIS) PURE;

        /************************************************************************
         *      Method:
         *          IHXTFuncAreaEnum::GetFirst
         *      Purpose:
         *          Returns details about the first pre-loaded functional area and resets
         *                      the enumerator to that position.
         *      Parameters:
         *          ppszNamespace - [out] Pointer to the pointer which will point to the text
         *          of the namespace for this functional area.
         *          pnNum - [out] Pointer to the integer which will be set to the numeric value
         *          for this functional area.
         *          ppszName - [out] Pointer to the pointer which will be set to the localized
         *                                      translation of the text representing the functional area.
         *      Returns:
         *          HXR_OK - If success.
         *          HXR_FAIL - No functional areas preloaded by the log system.
         */
        STDMETHOD (GetFirst) (THIS_
                const char**                    /*OUT*/                 ppszNamespace, 
                UINT32*                                         /*OUT*/                 pnNum, 
                const char**                    /*OUT*/                 ppszName) PURE;

        /************************************************************************
         *      Method:
         *          IHXTLogObserverManager::OnEndService
         *      Purpose:
         *          Returns details about the next pre-loaded functional area.
         *      Parameters:
         *          ppszNamespace - [out] Pointer to the pointer which will point to the text
         *          of the namespace for this functional area.
         *          pnNum - [out] Pointer to the integer which will be set to the numeric value
         *          for this functional area.
         *          ppszName - [out] Pointer to the pointer which will be set to the localized
         *                                      translation of the text representing the functional area.
         *      Returns:
         *          HXR_OK - If success.
         *          HXR_FAIL - No more functional areas.
         */
        STDMETHOD (GetNext) (THIS_
                const char**                    /*OUT*/                 ppszNamespace, 
                UINT32*                                         /*OUT*/                 pnNum, 
                const char**                    /*OUT*/                 ppszName) PURE;
};


/****************************************************************************
 *
 *  Interface:
 *
 *  IHXTLogObserverManager
 *
 *  Purpose:
 *
 *  This interface manages the subscription of observer objects to the log
 *  system.
 *
 *      IID_IHXTLogObserverManager:
 *
 *  // {EA6ABCDC-66EB-11d4-931A-00D0B749DE42}
 *
 */

DEFINE_GUID(IID_IHXTLogObserverManager, 
0xea6abcdc, 0x66eb, 0x11d4, 0x93, 0x1a, 0x0, 0xd0, 0xb7, 0x49, 0xde, 0x42);

#undef INTERFACE
#define INTERFACE IHXTLogObserverManager

DECLARE_INTERFACE_(IHXTLogObserverManager, IUnknown)
{
        /*
         *      IUnknown methods
         */
        STDMETHOD(QueryInterface)               (THIS_
                                REFIID riid,
                                void** ppvObj) PURE;

        STDMETHOD_(ULONG32,AddRef)              (THIS) PURE;

        STDMETHOD_(ULONG32,Release)             (THIS) PURE;

        /************************************************************************
         *      Method:
         *          IHXTLogObserverManager::Subscribe
         *      Purpose:
         *          Adds an observer to the log system which will receive log messages, and
         *                      initializes it with the parameter values.
         *      Parameters:
         *          pUnknown - [in] IUnknown pointer to the observer object which must 
         *                                      support a QueryInterface for the IHXTLogObserver interface.
         *          szFilterStr - [in] XML string specifying an initial filter for this
         *                                      observer.
         *          szLocale - [in] Language in which the observer wishes to receive log 
         *                                      messages.  **Currently ignored**
         *          bCatchup - [in] Indicates whether the observer wishes to receive all
         *                                      log messages (up to 1000) previously delivered by the log system
         *                                      prior to this observers subscription.
         *      Returns:
         *          HXR_OK - If success.
         *          HXR_FAIL - IUnknown parameter did not support the IHXTLogObserver interface.
         */
        STDMETHOD       (Subscribe) (THIS_ 
                        IUnknown*                       /*IN*/                  pUnknown, 
                        const char*             /*IN*/                  szFilterStr, 
                        const char*             /*IN*/                  szLocale, 
                        HXBOOL                                    /*IN*/                  bCatchUp) PURE; 

        /************************************************************************
         *      Method:
         *          IHXTLogObserverManager::SetFilter
         *      Purpose:
         *          Applies the specified filter to all future log messages delivered to
         *                      the specified observer.
         *      Parameters:
         *          szFilterStr - [in] XML string specifying a filter for this
         *                                      observer.
         *          pObserver - [in] An IUnknown pointer to a previously subscribed observer
         *                                      which will have the filter applied to it.
         *      Returns:
         *          HXR_OK - Since the filter is applied asynchronously, the function will
         *               Always succeed.
         */
        STDMETHOD (SetFilter) (THIS_ 
                        const char*             /*IN*/                  szFilterStr, 
                        IUnknown*                       /*IN*/                  pObserver) PURE; 

        /************************************************************************
         *      Method:
         *          IHXTLogObserverManager::Unsubscribe
         *      Purpose:
         *          Removes an observer from the log system.
         *      Parameters:
         *          pUnknown - [in] IUnknown pointer to the observer object to be 
         *                                      removed.
         *          bReceiveUnsentMessages - [in] Indicates whether the observer wishes 
         *                                      have delivered all messages which have been received by the log 
         *                                      system but not yet delivered to this observer.
         *      Returns:
         *          HXR_OK - If success.
         *          HXR_FAIL - The specified observer was not subscribed to the log system.
         */
        STDMETHOD (Unsubscribe) (THIS_ 
                IUnknown*                       /*IN*/                  pObserver, 
                HXBOOL                                    /*IN*/                  bReceiveUnsentMessages) PURE; 

        /************************************************************************
         *      Method:
         *          IHXTLogObserverManager::SetLanguage
         *      Purpose:
         *          Sets the language which will be used for translatable messages when
         *                      messages are delivered to the specified observer.
         *      Parameters:
         *          szLanguage - [in] Language in which the observer wishes to receive log 
         *                                      messages.  **Currently ignored**
         *          pObserver - [in] IUnknown pointer to the observer which will have its
         *                                      language value set.
         *      Returns:
         *          HXR_OK - If success.
         *          HXR_FAIL - Observer was not subscribed to the log system.
         */
        STDMETHOD (SetLanguage) (THIS_ 
                const char*                     /*IN*/                  szLanguage, 
                IUnknown*                               /*IN*/                  pObserver) PURE;
};


/****************************************************************************
 *
 *  Interface:
 *
 *  IHXTLogObserverManager2
 *
 *  Purpose:
 *
 *  This interface add FlushAllObservers method to IHXTLogObserverManager's methods.
 *
 *      IID_IHXTLogObserverManager2:
 *
 *  // {0E38953F-25AD-4efb-9AD4-2CBBC9D62AB0}
 *
 */

// {0E38953F-25AD-4efb-9AD4-2CBBC9D62AB0}
DEFINE_GUID(IID_IHXTLogObserverManager2, 
0xe38953f, 0x25ad, 0x4efb, 0x9a, 0xd4, 0x2c, 0xbb, 0xc9, 0xd6, 0x2a, 0xb0);

#undef INTERFACE
#define INTERFACE IHXTLogObserverManager2

DECLARE_INTERFACE_(IHXTLogObserverManager2, IHXTLogObserverManager)
{
        /************************************************************************
         *      Method:
         *          IHXTLogObserverManager2::FlushObservers
         *      Purpose:
         *          Flushes the log messages from log queue and calls Flush on all observers.
         *      Returns:
         *          HXR_OK - If success.
         *          HXR_FAIL - failure.
         */
        STDMETHOD(FlushObservers) (THIS) PURE;
};


/****************************************************************************
 *
 *  Interface:
 *
 *  IHXTLogWriter
 *
 *  Purpose:
 *
 *  This interface is used to send log messages to the log system.
 *
 *      IID_IHXTLogWriter:
 *
 *  {EA6ABCD9-66EB-11d4-931A-00D0B749DE42}
 *
 */

DEFINE_GUID(IID_IHXTLogWriter, 
0xea6abcd9, 0x66eb, 0x11d4, 0x93, 0x1a, 0x0, 0xd0, 0xb7, 0x49, 0xde, 0x42);

#undef INTERFACE
#define INTERFACE IHXTLogWriter

DECLARE_INTERFACE_(IHXTLogWriter, IUnknown)
{
        /*
         *      IUnknown methods
         */
        STDMETHOD(QueryInterface)               (THIS_
                                REFIID riid,
                                void** ppvObj) PURE;

        STDMETHOD_(ULONG32,AddRef)              (THIS) PURE;

        STDMETHOD_(ULONG32,Release)             (THIS) PURE;

        /************************************************************************
         *      Method:
         *          IHXTLogWriter::LogMessage
         *      Purpose:
         *          Logs a message in the log system with the specified parameters
         *                      to be delivered to all observers.
         *
         *      Parameters:
         *          szNamespace - [in] Text identifier to qualify the functional area
         *                                      and numeric message parameter.
         *          nLogCode - [in] Enumerated value from rtalogconstants.h which 
         *                                      indicates the importance of the log message 
         *          nFuncArea - [in] Enumerated value from rtalogconstnats.h which 
         *                                      indicates the general area of the system where the message
         *                                      originated.
         *          nMsg - [in] Identifies the log message to be used from the translation
         *                                      xml files loaded by the log system upon startup.  To use the
         *                                      szMsg variable for the message instead, specify 0xFFFFFFFF for 
         *                                      this value.
         *          szMsg - [in] Contains the text that will be used for the log message if 
         *                                      the nMsg parameter is 0xFFFFFFFF.
         *      Returns:
         *          HXR_OK - if success
         *          HXR_FAIL - Log system is not properly initialized.
         */
        STDMETHOD(LogMessage) (THIS_ 
                                const char*                     /*IN*/          szNamespace, 
                                EHXTLogCode                                     /*IN*/          nLogCode, 
                                EHXTLogFuncArea                         /*IN*/          nFuncArea,
                                UINT32                                  /*IN*/          nMsg, 
                                const char*                     /*IN*/          szMsg
                                ) PURE;

        /************************************************************************
         *      Method:
         *          IHXTLogWriter::GetTranslatedMessage
         *      Purpose:
         *          Retrieves the translated string for the message number provided
         *                      from the log system.
         *
         *      Parameters:
         *          nMessageNumber - [in] Message number to be translated.
         *          szNamespace - [in] Namespace of the message to be translated.
         *                      szLanguage - [in] Currently unused.
         *          szMessage - [out] Translated message string.
         *      Returns:
         *          HXR_OK - if success
         */
        STDMETHOD(GetTranslatedMessage) (THIS_ 
                                UINT32                                                          /*IN*/          nMessageNumber, 
                                const char*                                             /*IN*/          szNamespace, 
                                const char*                                             /*IN*/          szLanguage,
                                const char**                                    /*OUT*/         szMessage
                                ) PURE;

};


// {E7ADC1B7-7B6E-4e54-9878-AA810ECC6DE6}
DEFINE_GUID(IID_IHXTInternalLogWriter, 
0xe7adc1b7, 0x7b6e, 0x4e54, 0x98, 0x78, 0xaa, 0x81, 0xe, 0xcc, 0x6d, 0xe6);

#undef INTERFACE
#define INTERFACE IHXTInternalLogWriter

DECLARE_INTERFACE_(IHXTInternalLogWriter, IUnknown)
{
        /*
         *      IUnknown methods
         */
        STDMETHOD(QueryInterface)               (THIS_
                                REFIID riid,
                                void** ppvObj) PURE;

        STDMETHOD_(ULONG32,AddRef)              (THIS) PURE;

        STDMETHOD_(ULONG32,Release)             (THIS) PURE;

        /************************************************************************
         *      Method:
         *          IHXTLogWriter::LogMessage
         *      Purpose:
         *          Logs a message in the log system with the specified parameters
         *                      to be delivered to all observers.
         *
         *      Parameters:
         *          szNamespace - [in] Text identifier to qualify the functional area
         *                                      and numeric message parameter.
         *          nLogCode - [in] Enumerated value from rtalogconstants.h which 
         *                                      indicates the importance of the log message 
         *          nFuncArea - [in] Enumerated value from rtalogconstnats.h which 
         *                                      indicates the general area of the system where the message
         *                                      originated.
         *          nMsg - [in] Identifies the log message to be used from the translation
         *                                      xml files loaded by the log system upon startup.  To use the
         *                                      szMsg variable for the message instead, specify 0xFFFFFFFF for 
         *                                      this value.
         *          szMsg - [in] Contains the text that will be used for the log message if 
         *                                      the nMsg parameter is 0xFFFFFFFF.
         *          args - [in] The list of variable arguments that will be substituted into
         *                                      the log message by the log system using sprintf
         *      Returns:
         *          HXR_OK - if success
         *          HXR_FAIL - Log system is not properly initialized.
         */
        STDMETHOD(LogMessage) (THIS_ 
                                const char*                     /*IN*/          szNamespace, 
                                EHXTLogCode                                     /*IN*/          nLogCode, 
                                EHXTLogFuncArea                         /*IN*/          nFuncArea,
                                UINT32                                  /*IN*/          nMsg, 
                                const char*                     /*IN*/          szMsg, 
                                va_list                                 /*IN*/          args) PURE;

        STDMETHOD_(HXBOOL, IsEnabled)(THIS) PURE;
};


/****************************************************************************
 *
 *  Interface:
 *
 *  IHXTLogSystem
 *
 *  Purpose:
 *                      Provides access to the areas of the log system
 *  
 *      IID_IHXTLogSystem:
 *
 *  // {E50F7E51-4640-11d5-935B-00D0B749DE42}
 *
 */

DEFINE_GUID(IID_IHXTLogSystem, 
0xe50f7e51, 0x4640, 0x11d5, 0x93, 0x5b, 0x0, 0xd0, 0xb7, 0x49, 0xde, 0x42);

#define CLSID_IHXTLogSystem IID_IHXTLogSystem

#undef INTERFACE
#define INTERFACE IHXTLogSystem

DECLARE_INTERFACE_(IHXTLogSystem, IUnknown)
{
        /*
         *      IUnknown methods
         */
        STDMETHOD(QueryInterface)               (THIS_
                                REFIID riid,
                                void** ppvObj) PURE;

        STDMETHOD_(ULONG32,AddRef)              (THIS) PURE;

        STDMETHOD_(ULONG32,Release)             (THIS) PURE;

        /************************************************************************
         *      Method:
         *          IHXTLogSystem::Shutdown
         *      Purpose:
         *          Properly shuts down the log system.
         *      Parameters:
         *      None.
         *      Returns:
         *          HXR_OK - If success.
         *          HXR_FAIL - Log system could not shutdown properly.
         *      Notes:
         *                      Under Windows, this method should not be called from within DllMain().
         */
        STDMETHOD(Shutdown) (THIS) PURE;

        /************************************************************************
         *      Method:
         *          IHXTLogSystem::SetTranslationFileDirectory
         *      Purpose:
         *          Sets the translation file directory for the log system.
         *      Parameters:
         *          szTranslationFileDir - [in] Location of all log system translation files.  These
         *                  files will be used to translate message numbers into text strings.
         *      Returns:
         *          HXR_OK - If success.
         *          HXR_FAIL - Translation file directory already set.
         */
        STDMETHOD(SetTranslationFileDirectory) (THIS_ const char* szTranslationFileDir) PURE;

        /************************************************************************
         *      Method:
         *          IHXTLogSystem::GetWriterInterface
         *      Purpose:
         *          Retrieves an interface to the log writer, used to send messages
         *                      into the log system.
         *      Parameters:
         *          ppIWriter - [out] Address of output variable that receives 
         *                  the log writer interface pointer.
         *      Returns:
         *          HXR_OK - If success.
         *          HXR_FAIL - Log system not properly initialized.
         */
        STDMETHOD(GetWriterInterface) (THIS_ 
                        IHXTLogWriter**                 /*OUT*/                 ppIWriter) PURE;

        /************************************************************************
         *      Method:
         *          IHXTLogSystem::GetObserverManagerInterface
         *      Purpose:
         *          Retrieves an interface to the observer manager, used to subscribe,
         *                      manage, and unsubscribe listening observer which receive log messages.
         *      Parameters:
         *          ppILogObserverManager - [out] Address of output variable that receives 
         *                  the observer manager interface pointer.
         *      Returns:
         *          HXR_OK - If success.
         *          HXR_FAIL - Log system not properly initialized.
         */
        STDMETHOD(GetObserverManagerInterface)(THIS_ 
                IHXTLogObserverManager**                        /*OUT*/                 ppILogObserverManager) PURE;

        /************************************************************************
         *      Method:
         *          IHXTLogSystem::GetFunctionalAreaEnumerator
         *      Purpose:
         *          Retrieves an interface to an enumerator which will enumerate through
         *                      all functional areas in all namespaces in the specified language  
         *                      loaded on log system initialization.
         *      Parameters:
         *          pIEnum - [out] Address of output variable that receives 
         *                  the enumerator interface pointer.
         *                      szLanguage - [in] The language of the functional areas to be enumerated.
         *      Returns:
         *          HXR_OK - If success.
         *          HXR_FAIL - Log system not properly initialized.
         */
        STDMETHOD(GetFunctionalAreaEnumerator)  (
                IHXTFuncAreaEnum**                      /*OUT*/                 pIEnum, 
                const char*                                             /*IN*/                  szLanguage) PURE;
};

/****************************************************************************
 *
 *  Interface:
 *
 *  IHXLogSystemManager
 *
 *  Purpose: Defines a service for managing the log system
 *  
 *  IID_IHXLogSystemManager:
 *
 *  {1D6824A4-269E-4f85-B373-81F276F806D1}
 *
 */

DEFINE_GUID(IID_IHXLogSystemManager, 
0x1d6824a4, 0x269e, 0x4f85, 0xb3, 0x73, 0x81, 0xf2, 0x76, 0xf8, 0x6, 0xd1);

#undef INTERFACE
#define INTERFACE IHXLogSystemManager

DECLARE_INTERFACE_(IHXLogSystemManager, IUnknown)
{
    /*
     * IUnknown methods
     */
    STDMETHOD(QueryInterface)   (THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)  (THIS) PURE;
    STDMETHOD_(ULONG32,Release) (THIS) PURE;

    /*
     * IHXLogSystemManager methods
     */
    /************************************************************************
     *      Method:
     *          IHXLogSystemManager::InitializeLogSystem()
     *      Purpose:
     *          Load and initialize the log system
     *      Parameters:
     *          None.
     *      Returns:
     *          HXR_OK - If success.
     *          HXR_FAIL - Log system could not be loaded or initialized properly.
     */
    STDMETHOD(InitializeLogSystem) (THIS_ IUnknown* pContext) PURE;

    /************************************************************************
     *      Method:
     *          IHXLogSystemManager::GetLogSystem
     *      Purpose:
     *          Returns a reference to the IHXTLogSystem
     *      Parameters:
     *          None
     *      Returns:
     *          HXR_OK - If success.
     *          HXR_UNEXPOECTED - If the IHXLogSystemManager has not been successfully initialized
     */
    STDMETHOD(GetLogSystem) (THIS_ REF(IHXTLogSystem*) rpLogSystem) PURE;

    /************************************************************************
     *      Method:
     *          IHXLogSystemManager::SetLogSystemConfig()
     *      Purpose:
     *          Allows the user to configure the log system with a set of properties
     *      Parameters:
     *          None
     *      Returns:
     *          HXR_OK - If success.
     *          HXR_UNEXPOECTED - If the IHXLogSystemManager has not been successfully initialized
     */
    STDMETHOD(SetLogSystemConfig) (THIS_ IHXValues* pConfig) PURE;

    /************************************************************************
     *      Method:
     *          IHXLogSystemManager::GetLogSystemConfig()
     *      Purpose:
     *          Allows the user to retrieve the current configuration for the log system
     *      Parameters:
     *          None
     *      Returns:
     *          HXR_OK - If success.
     *          HXR_UNEXPOECTED - If the IHXLogSystemManager has not been successfully initialized
     */
    STDMETHOD(GetLogSystemConfig) (THIS_ REF(IHXValues*) rpConfig) PURE;

    /************************************************************************
     *      Method:
     *          IHXLogSystemManager::InitializeLogObservers()
     *      Purpose:
     *          Uses the plugin handler to load all log observers, initializes
     *          them, and then subscribes them to the log system.
     *      Parameters:
     *          None
     *      Returns:
     *          HXR_OK   - If success. If there are no log observers, then this will
     *                     still return HXR_OK
     *          HXR_FAIL - If one or more log observers failed to load or initialize.
     */
    STDMETHOD(InitializeLogObservers) (THIS) PURE;

    /************************************************************************
     *      Method:
     *          IHXLogSystemManager::TerminateLogObservers()
     *      Purpose:
     *          Flushes all log observers, unsubscribes them from the
     *          log system, and then unloads them.
     *      Parameters:
     *          None
     *      Returns:
     *          HXR_OK         - If success.
     *          HXR_UNEXPECTED - If log system has not been successfully initialized
     */
    STDMETHOD(TerminateLogObservers) (THIS) PURE;

    /************************************************************************
     *      Method:
     *          IHXLogSystemManager::TerminateLogSystem
     *      Purpose:
     *          Shuts down and unloads the log system.
     *      Parameters:
     *          None
     *      Returns:
     *          HXR_OK         - If success.
     *          HXR_UNEXPECTED - Log system not properly initialized.
     */
    STDMETHOD(TerminateLogSystem) (THIS) PURE;
};

/****************************************************************************
 *  Function:
 *      RMAGetLogSystemInterface
 *  Purpose:
 *      Obtains an interface pointer to the log system.  If the log system has not 
 *      yet been created, it is created and initialized.
 */
typedef HX_RESULT (STDAPICALLTYPE *FPRMAGETLOGSYSTEMINTERFACE)(IHXTLogSystem** ppLogSystem);

#if defined(HELIX_FEATURE_ALLOW_DEPRECATED_SMARTPOINTERS)
#include "hxtsmartpointer.h"
HXT_MAKE_SMART_PTR(IHXTLogObserver)
HXT_MAKE_SMART_PTR(IHXTLogObserver2)
HXT_MAKE_SMART_PTR(IHXTFuncAreaEnum)
HXT_MAKE_SMART_PTR(IHXTLogObserverManager)
HXT_MAKE_SMART_PTR(IHXTLogObserverManager2)
HXT_MAKE_SMART_PTR(IHXTLogWriter)
HXT_MAKE_SMART_PTR(IHXTInternalLogWriter)
HXT_MAKE_SMART_PTR(IHXTLogSystem)
HXT_MAKE_SMART_PTR(IHXLogSystemManager)
#else
#include "hxcomptr.h"
DEFINE_SMART_PTR(IHXTLogObserver)
DEFINE_SMART_PTR(IHXTLogObserver2)
DEFINE_SMART_PTR(IHXTFuncAreaEnum)
DEFINE_SMART_PTR(IHXTLogObserverManager)
DEFINE_SMART_PTR(IHXTLogObserverManager2)
DEFINE_SMART_PTR(IHXTLogWriter)
DEFINE_SMART_PTR(IHXTInternalLogWriter)
DEFINE_SMART_PTR(IHXTLogSystem)
DEFINE_SMART_PTR(IHXLogSystemManager)
#endif

#endif /* #ifndef IHXTLOGSYSTEM_H */
