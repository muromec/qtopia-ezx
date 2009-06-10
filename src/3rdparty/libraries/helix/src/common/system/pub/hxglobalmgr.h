/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxglobalmgr.h,v 1.7 2007/07/06 20:41:59 jfinnecy Exp $
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

#ifndef GLOBAL_MAN_H
#define GLOBAL_MAN_H

#include "hxtypes.h"
#include "hxcom.h"


typedef const void*	GlobalID;
typedef void*		GlobalType;
typedef GlobalType*	GlobalPtr;
typedef GlobalType&	GlobalRef;

typedef void (*HXGlobalDestroyFunc)(GlobalType pObj);


class HXGlobalManager
{
public:
    static HXGlobalManager* Instance();

    // add a global, return TRUE on success (or if it already exists)
    GlobalPtr   Add(GlobalID id, GlobalType pObj = NULL,
                        HXGlobalDestroyFunc fpDestroy = NULL);
    GlobalPtr   Get(GlobalID id) const;
    void        Remove(GlobalID id);
    void        Shutdown();

private:
    const int* Context();

    virtual GlobalPtr   DoAdd(const int* pContext, GlobalID id, GlobalType pObj,
                                HXGlobalDestroyFunc fpDestroy) = 0;
    virtual GlobalPtr   DoGet(GlobalID id) const = 0;
    virtual void        DoRemove(GlobalID id) = 0;
    virtual void        DoShutdown(const int* pContext) = 0;

    static const int zm_context;
};
inline
const int* HXGlobalManager::Context()
{
    return &zm_context;
}

inline
GlobalPtr HXGlobalManager::Add(GlobalID id, GlobalType pObj, 
                                HXGlobalDestroyFunc fpDestroy)
{
    return DoAdd(Context(), id, pObj, fpDestroy);
}

inline
GlobalPtr HXGlobalManager::Get(GlobalID id) const
{
    return DoGet(id);
}

inline
void HXGlobalManager::Remove(GlobalID id)
{
    DoRemove(id);
}

inline
void  HXGlobalManager::Shutdown()
{
    DoShutdown(Context());
}






#endif /* GLOBAL_MAN_H */
