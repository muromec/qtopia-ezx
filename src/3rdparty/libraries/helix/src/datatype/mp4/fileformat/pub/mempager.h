/* ***** BEGIN LICENSE BLOCK ***** 
 * Version: RCSL 1.0/RPSL 1.0 
 *  
 * Portions Copyright (c) 1995-2002 RealNetworks, Inc. All Rights Reserved. 
 *      
 * The contents of this file, and the files included with this file, are 
 * subject to the current version of the RealNetworks Public Source License 
 * Version 1.0 (the "RPSL") available at 
 * http://www.helixcommunity.org/content/rpsl unless you have licensed 
 * the file under the RealNetworks Community Source License Version 1.0 
 * (the "RCSL") available at http://www.helixcommunity.org/content/rcsl, 
 * in which case the RCSL will apply. You may also obtain the license terms 
 * directly from RealNetworks.  You may not use this file except in 
 * compliance with the RPSL or, if you have a valid RCSL with RealNetworks 
 * applicable to this file, the RCSL.  Please see the applicable RPSL or 
 * RCSL for the rights, obligations and limitations governing use of the 
 * contents of the file.  
 *  
 * This file is part of the Helix DNA Technology. RealNetworks is the 
 * developer of the Original Code and owns the copyrights in the portions 
 * it created. 
 *  
 * This file, and the files included with this file, is distributed and made 
 * available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER 
 * EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS ALL SUCH WARRANTIES, 
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS 
 * FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT. 
 * 
 * Technology Compatibility Kit Test Suite(s) Location: 
 *    http://www.helixcommunity.org/content/tck 
 * 
 * Contributor(s): 
 *  
 * ***** END LICENSE BLOCK ***** */ 

#ifndef _MEMPAGER_H_
#define _MEMPAGER_H_

/****************************************************************************
 *  Defines
 */
#ifdef QTCONFIG_SPEED_OVER_SIZE
#define QTMEMPAGER_INLINE inline
#else	// QTCONFIG_SPEED_OVER_SIZE
#define QTMEMPAGER_INLINE /**/
#endif	// QTCONFIG_SPEED_OVER_SIZE

/****************************************************************************
 *  Includes
 */
#include "hxtypes.h"
#include "hxcom.h"
#include "hxcomm.h"
#include "hxfiles.h"
#include "ihxpckts.h"


typedef _INTERFACE IHXFileSwitcher   IHXFileSwitcher;


/****************************************************************************
 * 
 *  Class:
 *	CMemPager
 *
 *  Purpose:
 *	Enables paging of file sections into memory
 *
 */
class CMemPager : public IHXFileResponse,
		  public IHXThreadSafeMethods
{
public:
    /*
     *	Constructor/Destructor
     */
    CMemPager(void);

    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)	(THIS);

    STDMETHOD_(ULONG32,Release)	(THIS);

    /*
     *	MemPager methods
     */
    HX_RESULT Init	(IUnknown* pSource,
			 ULONG32 ulOffset,
			 ULONG32 ulSize,
			 UINT8** ppBaseVirtualAddr = NULL);

    UINT8* GetBaseVirtualAddr(void)
    {
	return m_pBaseVirtualAddr;
    }

    HXBOOL IsFaulted(void)
    {
	return (m_ulFaultedPageSize != 0);
    }

    HXBOOL IsPageIn (UINT8* pVirtualAddr,
		   ULONG32 ulSize)
    {
	return ((pVirtualAddr >= m_pPageVirtualAddr) &&
		((m_pPageVirtualAddr + m_ulPageSize) >= (pVirtualAddr + ulSize)));
    }

    HXBOOL IsPageAdressable (UINT8* pVirtualAddr,
			   ULONG32 ulSize)
    {
	return ((pVirtualAddr >= m_pBaseVirtualAddr) &&
		((m_pBaseVirtualAddr + m_ulAddrSpaceSize) >= (pVirtualAddr + ulSize)));
    }

    QTMEMPAGER_INLINE HX_RESULT PageIn(UINT8* pVirtualAddr,
				       ULONG32 ulSize,
				       UINT8* &pPhysicalBaseAddr);

    QTMEMPAGER_INLINE HX_RESULT LoadPage(IHXFileResponse* pResponse);

    /*
     *	IHXFileResponse methods
     */
    STDMETHOD(InitDone)		(THIS_
				HX_RESULT status);

    STDMETHOD(CloseDone)	(THIS_
				HX_RESULT status);

    STDMETHOD(ReadDone)		(THIS_ 
				HX_RESULT status,
				IHXBuffer* pBuffer);

    STDMETHOD(WriteDone)	(THIS_ 
				HX_RESULT status);

    STDMETHOD(SeekDone)		(THIS_ 
				HX_RESULT status);

    /*
     *  IHXThreadSafeMethods method
     */
   STDMETHOD_(UINT32,IsThreadSafe) (THIS);

private:
    typedef enum
    {
	MEMPGR_Offline,
	MEMPGR_Ready,
	MEMPGR_ProcLoad
    } MemPagerState;

    void Reset(void);
    HX_RESULT _PageIn(UINT8* pVirtualAddr,
		      ULONG32 ulSize,
		      UINT8* &pPhysicalBaseAddr);
    inline void ReleasePage(void);
    HX_RESULT _LoadPage(void);
    inline HX_RESULT HandleResponse(HX_RESULT status, 
				    IHXBuffer* pBuffer = NULL);

    IHXFileSwitcher* m_pFileSwitcher;
    IHXFileResponse* m_pResponse;

    MemPagerState m_State;
    UINT8* m_pPageVirtualAddr;
    UINT8* m_pPagePhysicalAddr;
    ULONG32 m_ulPageSize;
    UINT8* m_pFaultedPageVirtualAddr;
    ULONG32 m_ulFaultedPageSize;
    ULONG32 m_ulAddrSpaceOffset;
    ULONG32 m_ulAddrSpaceSize;
    ULONG32 m_ulMinPageSize;
    UINT8* m_pBaseVirtualAddr;

    HXBOOL m_bSyncMode;

    IHXBuffer* m_pPageBuffer;

    LONG32 m_lRefCount;

    ~CMemPager();

};

#ifdef QTCONFIG_SPEED_OVER_SIZE
#include "mempager_inline.h"
#endif	// QTCONFIG_SPEED_OVER_SIZE

#endif  // _MEMPAGER_H_
