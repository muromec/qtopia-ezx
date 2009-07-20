/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxmemprb.h,v 1.7 2007/07/06 20:35:09 jfinnecy Exp $
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

#ifndef _HXMEMPRB_H_
#define _HXMEMPRB_H_

/****************************************************************************
 *  Defines
 */
#define HXDBG_CLIENT_MEM	FALSE
#define PNDBG_ALL_MEM		TRUE

#define HXDBG_FREEZE_STATS_ONLY	FALSE
#define HXDBG_FREEZE_ALL	TRUE

#define PNDBG_PROBE_ACTIVE	FALSE
#define PNDBG_PROBE_FROZEN	TRUE

#define PNDBG_NOBASE_MEM	FALSE
#define HXDBG_WITHBASE_MEM	TRUE


/****************************************************************************
 *  Includes
 */
#include "hlxclib/stdio.h"
#include "hxtypes.h"
#include "hxresult.h"

#if defined(_WINDOWS) && !defined(_WINCE)
#include "../platform/win/memprb.h"
#else  /* unsupported platform */
typedef void *PN_MEMREFPOINT;
#endif  /* _WINDOWS */


class CHXMemProbe
{
public:
    /************************************************************************
     * Constructor/Destructor
     */
    CHXMemProbe(HXBOOL bIsFrozen = TRUE);
    ~CHXMemProbe();

    LONG32	GetMemUsed(HXBOOL bWithCrt = TRUE, HXBOOL bWithBase = FALSE, CHXMemProbe *pReferenceProbe = NULL);
    LONG32	GetBaseMemUsed(HXBOOL bWithCrt = TRUE);
    HX_RESULT	DumpMemUsedSinceReset(FILE *pFile = NULL, HXBOOL bWithCrt = TRUE);
    ULONG32	GetMaxMemUsed(HXBOOL bWithCrt = TRUE, HXBOOL bWithBase = FALSE, CHXMemProbe *pReferenceProbe = NULL);
    HXBOOL	IsRestored(HXBOOL bWithCrt = TRUE);

    HX_RESULT	Freeze(HXBOOL bGenerateDetailInfo = FALSE, HXBOOL bWithCrt = TRUE);
    HX_RESULT	Reset(void);

    const char*	SearchMemFrozen(ULONG32 uMemBlockId);
    HX_RESULT	GetFirstMemFrozen(ULONG32 &uMemBlockId, ULONG32 &uMemUsed);
    HX_RESULT	GetNextMemFrozen(ULONG32 &uMemBlockId, ULONG32 &uMemUsed);
    
    static LONG32   GetGlobalMemUsed(HXBOOL WithCrt = TRUE);
    static ULONG32  GetGlobalMaxMemUsed(void);

    static ULONG32  SetGlobalMemBreak(ULONG32 uMemBlockId);

private:
#ifdef PNMEMPROBE_PRIVATE_METHODS
    PNMEMPROBE_PRIVATE_METHODS
#endif // PNMEMPROBE_PRIVATE_METHODS
#ifdef PNMEMPROBE_PRIVATE_DATA
    PNMEMPROBE_PRIVATE_DATA
#endif	// PNMEMPROBE_PRIVATE_DATA
};

#endif  /* _HXMEMPRB_H_ */
