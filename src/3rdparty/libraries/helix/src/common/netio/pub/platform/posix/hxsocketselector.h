/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: hxsocketselector.h,v 1.4 2006/02/16 23:03:05 ping Exp $ 
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

/*
 * Base class for posix/bsd-based platform selector
 *
 * HXSocketSelector detects socket state changes for a given set of sockets. Each
 * socket has one (and only one) associated handler (HXSocketSelector::EventHandler).
 *
 *
 */

#if !defined( HX_SOCKETSELECTOR_H__ )
#define	HX_SOCKETSELECTOR_H__

#include "nettypes.h" //sockobj_t
#include "chxmapptrtoptr.h"
#include "hxrefcounted.h"

class CHXSet;

class HXSocketSelector
: public HXRefCounted
{
public:
    class EventHandler 
    {
    public:
        virtual void OnSelectEvent(sockobj_t fd, UINT32 event /*HX_SOCK_XXX*/, HX_RESULT err)  = 0;
    };
public:
   
    // ref-counted per-thread instance
    static HX_RESULT GetThreadInstance(HXSocketSelector*& pSelector, IUnknown* pContext);
    static void DestroyThreadInstance();

    virtual HX_RESULT Init(IUnknown* pContext);
    virtual HX_RESULT AddSocket(sockobj_t fd, UINT32 eventMask, EventHandler* pHandler);
    virtual HX_RESULT UpdateSocket(sockobj_t fd, UINT32 eventMask);
    virtual HX_RESULT ReEnableSocket(sockobj_t fd, UINT32 eventMask);
    virtual void RemoveSocket(sockobj_t fd);
    virtual void RemoveHandler(EventHandler* pHandler);

    bool HasSocket(sockobj_t fd) const;
    UINT32 GetTID() const;
protected:
    HXSocketSelector();
    virtual ~HXSocketSelector();
    static HXSocketSelector* AllocInstance();

protected:
    IUnknown*	    m_pContext;
    CHXMapPtrToPtr  m_pool;

private:
    static HX_RESULT Create(HXSocketSelector*& pSelector, IUnknown* pContext);

    void RemoveSocketSet(const CHXSet& removeSet);

    // HXRefCounted
    void FinalRelease();

private:
    UINT32  m_tid;
};

inline
UINT32 HXSocketSelector::GetTID() const
{
    return m_tid;
}

#endif	//	HX_SOCKETSELECTOR_H__
