/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: memprb.h,v 1.5 2007/07/06 20:35:08 jfinnecy Exp $
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

#ifndef _MEMPRB_H_
#define _MEMPRB_H_

/****************************************************************************
 *  Includes
 */
#include "hxtypes.h"
#include "crtdbg.h"
#include "windows.h"


/****************************************************************************
 *  Defines
 */
#ifdef _DEBUG

struct MemBlockInfo
{
    ULONG32	    uId;
    ULONG32	    uMemUsed;
    char*	    pDetail;
    MemBlockInfo*   pNext;
};


#define PNMEMPROBE_PRIVATE_DATA	_CrtMemState		m_MemRefPoint;\
				_CrtMemState		m_MemFreezePoint;\
				MemBlockInfo		m_MemFreeze;\
				MemBlockInfo*		m_pMemFreezeIter;\
				HXBOOL			m_bIsFrozen;\
				LONG32			m_MemUsed;\
				LONG32			m_MemUsedWithCrt;\
				LONG32			m_MaxMemUsed;\
				LONG32			m_MaxMemUsedWithCrt;\
				LONG32			m_BaseMemUsed;\
				LONG32			m_BaseMemUsedWithCrt;\
				CHXMemProbe*		m_pParent;\
				CHXMemProbe*		m_pChild;\
				static CHXMemProbe*	mz_pActiveProbe;\
				static HXBOOL		m_bMutexInitialized;\
				static CRITICAL_SECTION	m_Mutex;

#if (defined(_MSC_VER) && (_MSC_VER > 1100) && defined(_BASETSD_H_)) /* VC 6 Check */
#define PNMEMPROBE_PRIVATE_METHODS  HX_RESULT MakeBlockInfo(MemBlockInfo &Info, HXBOOL bWithCrt);\
                    HX_RESULT AddBlockInfo(MemBlockInfo &Info, ULONG32 uId, ULONG32 uMemUsed, const char *pDetail);\
                    void DestroyBlockInfo(MemBlockInfo &Info);\
                    void _DumpMemUsedSinceReset(FILE *pFile, HXBOOL bWithCrt);\
                    static int __cdecl  MemProbeHook(int allocType, void *userData, size_t size, int blockType,\
                            long requestNumber, const unsigned char *filename, int lineNumber);
#else

#define PNMEMPROBE_PRIVATE_METHODS  HX_RESULT MakeBlockInfo(MemBlockInfo &Info, HXBOOL bWithCrt);\
				    HX_RESULT AddBlockInfo(MemBlockInfo &Info, ULONG32 uId, ULONG32 uMemUsed, const char *pDetail);\
				    void DestroyBlockInfo(MemBlockInfo &Info);\
				    void _DumpMemUsedSinceReset(FILE *pFile, HXBOOL bWithCrt);\
				    static int __cdecl	MemProbeHook(int allocType, void *userData, size_t size, int blockType,\
							long requestNumber, const char *filename, int lineNumber);

#endif /* VC 6 */
#else  /* _DEBUG */

#define PNMEMPROBE_PRIVATE_DATA
#define PNMEMPROBE_PRIVATE_METHODS

#endif  /* _DEBUG */


#endif  /* _MEMPRB_H_ */
