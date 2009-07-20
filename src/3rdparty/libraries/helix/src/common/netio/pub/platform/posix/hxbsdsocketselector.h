/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: hxbsdsocketselector.h,v 1.6 2008/08/27 09:59:48 lovish Exp $ 
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

/*
 * Target: BSD/Posix socket
 *
 * Synopsis:
 *
 * Derived from HXSocketSelector; select implemented via select.
 *
 */

#if !defined( HX_BSDSOCKETSELECTOR_H__ )
#define	HX_BSDSOCKETSELECTOR_H__

#include "hxsocketselector.h" 
#include "hxengin.h"
#include "hxclassdispatchtasks.h"
#include "hxthreadtaskdriver.h"
#include "chxmaplongtoobj.h"

class CHXClientSocket;
class HXBSDSocketSelector
: public HXSocketSelector
{
public:
    HX_RESULT Init(IUnknown* pContext);
    HX_RESULT UpdateSocket(sockobj_t fd, UINT32 eventMask);
    HX_RESULT ReEnableSocket(sockobj_t fd, UINT32 eventMask);	
    void RemoveSocket(sockobj_t fd);

    void ProcessSelect();
    HX_RESULT ProcessIdle();
private:
    friend HXSocketSelector* HXSocketSelector::AllocInstance();
    HXBSDSocketSelector();
    virtual ~HXBSDSocketSelector();
    static void* ThreadProc_(void* pv);
    void* ThreadProc();
	
    HX_RESULT SendWait(HXClassDispatchTask<HX_RESULT>* pTask);	

protected:
    IHXThread*   m_pSelectThread;
    IHXEvent*		    m_pQuitEvent;
    HXThreadTaskDriver*     m_pDriver; 
    HXBOOL m_bThreadCreated;	
    CHXMapLongToObj  m_SockEventPool;	
};


#endif	// HX_BSDSOCKETSELECTOR_H__
