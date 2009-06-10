/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: errdbg.h,v 1.17 2007/07/06 20:35:09 jfinnecy Exp $
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

#ifndef	_ERRDBG_H_
#define	_ERRDBG_H_

extern const char* g_pDebugOutLevels[];
extern const HXBOOL  g_bDebugOutDefaults[];

#define DEBUG_OUT_BUF_SZ 2048

#include "hlxclib/stdio.h"
inline int debug_out_sprintf(char* pBuffer, const char* pFormatString, ...)
{
    va_list argptr;
    va_start(argptr, pFormatString);
    int nCharsWritten = vsnprintf(pBuffer, DEBUG_OUT_BUF_SZ, 
				  pFormatString, argptr);
    pBuffer[DEBUG_OUT_BUF_SZ-1] = '\0';
    va_end(argptr);

    return nCharsWritten;
}
#define DEBUG_OUT_SPRINTF debug_out_sprintf


#if defined(HELIX_FEATURE_DBG_LOG) && !defined(GOLD)
#include "hxerror.h"
#define DEBUG_OUT(x, l, y) {						\
			    char* s;					\
			    s = new char[DEBUG_OUT_BUF_SZ];		\
			    if(s){                                      \
			    DEBUG_OUT_SPRINTF y;			\
			    x ? x->Report(HXLOG_DEBUG, 0, l, s, 0) : 0;	\
			    delete [] s;					\
			    } \
			   }
#else
#define DEBUG_OUT(x, l, y)
#endif

#ifdef _DEBUG
#define DEBUG_OUTF(x, y)	{						\
			    char* s;					\
			    FILE* f1;					\
			    s = new char[DEBUG_OUT_BUF_SZ];		\
			    if(s){                                      \
			    DEBUG_OUT_SPRINTF y;			\
			    f1 = (x)?(::fopen(x, "a+")):(NULL);			\
			    (f1)?(::fprintf(f1, s), ::fclose(f1)):(0);	\
			    delete [] s;					\
		            } \
		       }
#else
#define DEBUG_OUTF(x, y)
#endif

#ifdef _DEBUG
#ifndef DEBUG_OUTF_IDX_COL_WIDTH
#define DEBUG_OUTF_IDX_COL_WIDTH    20
#endif	// DEBUG_OUTF_IDX_COL_WIDTH
#define DEBUG_OUTF_IDX(idx, x, y)	{								\
			    char* s;									\
			    char* p_dbgx;								\
			    FILE* f1_dbgx;								\
			    int i_dbgx = (idx > 0)?(DEBUG_OUTF_IDX_COL_WIDTH * idx):0;			\
			    p_dbgx = s = new char[DEBUG_OUT_BUF_SZ + i_dbgx];				\
			    if(s){ \
			    if (i_dbgx < 1024)								\
				for (; i_dbgx > 0; i_dbgx--)						\
				    *(s++) = ' ';							\
			    DEBUG_OUT_SPRINTF y;							\
			    f1_dbgx = (x)?(::fopen(x, "a+")):(NULL);					\
			    (f1_dbgx)?(::fprintf(f1_dbgx, p_dbgx), ::fclose(f1_dbgx)):(0);		\
			    delete [] p_dbgx;								\
			    } \
		       }
#else
#define DEBUG_OUTF_IDX(idx, x, y)
#endif

/* Debug Levels */
#define DOL_GENERIC                 0
#define DOL_TRANSPORT               1
#define DOL_ASM                     2
#define DOL_BWMGR                   3
#define DOL_TRANSPORT_EXTENDED      4
#define DOL_REALAUDIO               5
#define DOL_REALAUDIO_EXTENDED      6
#define DOL_REALVIDEO               7
#define DOL_REALPIX                 8
#define DOL_REALPIX_EXTENDED        9
#define DOL_JPEG                   10
#define DOL_JPEG_EXTENDED          11
#define DOL_GIF                    12
#define DOL_GIF_EXTENDED           13
#define DOL_FLASH                  14
#define DOL_FLASH_EXTENDED         15
#define DOL_SMIL                   16
#define DOL_SMIL_EXTENDED          17
#define DOL_TURBOPLAY              18
#define DOL_TURBOPLAY_EXTENDED     19
#define DOL_SITE                   20
#define DOL_AUTOUPDATE             21
#define DOL_RECONNECT              22
#define DOL_AUTHENTICATION         23
#define DOL_CORELOADTIME           24
#define DOL_RTSP                   25
#define DOL_STREAMSOURCEMAP        26
#define DOL_REALEVENTS             27
#define DOL_REALEVENTS_EXTENDED    28
#define DOL_BUFFER_CONTROL         29
#define NUM_DOL_CODES              30 // Make sure this is updated when
                                      // new user codes are added


#ifdef ERRDBG_DEFINE_CONSTS

/* Name of Each Debug Level */
const char* g_pDebugOutLevels[] = {
    "Generic Messages",
    "Transport Basic",
    "ASM Subscriptions",
    "Bandwidth Manager",
    "Transport Bandwidth Reports",
    "RealAudio Renderer",
    "RealAudio Renderer Extended",
    "RealVideo Renderer",
    "RealPix Renderer",
    "RealPix Renderer Extended",
    "JPEG Renderer",
    "JPEG Renderer Extended",
    "GIF Renderer",
    "GIF Renderer Extended",
    "Flash Renderer",
    "Flash Renderer Extended",
    "SMIL Renderer",
    "SMIL Renderer Extended",
    "TurboPlay",
    "TurboPlay Extended",
    "Site Info",
    "AutoUpdate",
    "Reconnect and Redirect",
    "Authentication",
    "Core Load Time",
    "RTSP",
    "Stream and Source",
    "RealEvents Renderer",
    "RealEvents Renderer Extended",
    "Buffer Control",
    0
};


/*
 * Debug Level Default Status
 * 
 * Set to TRUE if this debug level should default to ON in the statistics pane.
 * Set this to false if you output a lot of data at a particular debug level.
 */
const HXBOOL g_bDebugOutDefaults[] = {
    TRUE,       // Generic Messages
    TRUE,       // Transport Basic
    TRUE,       // ASM Subscriptions
    TRUE,       // Bandwidth Manager
    FALSE,      // Transport Bandwidth Reports
    TRUE,       // RealAudio Renderer
    FALSE,      // RealAudio Renderer Extended
    TRUE,       // RealVideo Renderer
    TRUE,       // RealPix Renderer
    FALSE,      // RealPix Renderer Extended
    TRUE,       // JPEG Renderer
    FALSE,      // JPEG Renderer Extended
    TRUE,       // GIF Renderer
    FALSE,      // GIF Renderer Extended
    TRUE,       // Flash Renderer
    FALSE,      // Flash Renderer Extended
    TRUE,       // SMIL Renderer
    FALSE,      // SMIL Renderer Extended
    FALSE,      // TurboPlay
    FALSE,      // TurboPlay Extended
    FALSE,      // Site Info
    FALSE,      // AutoUpdate
    FALSE,      // Reconnect and Redirect
    FALSE,      // Authentication
    FALSE,      // Core Load Time
    FALSE,      // RTSP
    FALSE,      // Stream and Source Info
    TRUE,       // RealEvents Renderer
    FALSE,      // RealEvents Renderer Extended
    0
};
#endif

#endif /*_ERRDBG_H_*/
