/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: symbian_gm_inst.h,v 1.8 2008/05/28 22:46:16 ashkunar Exp $
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

#ifndef SYMBIAN_GM_IMP_H
#define SYMBIAN_GM_IMP_H

#include <e32svr.h>

//This ID was defined because the SymbianGlobalVideoParameters is used across
//two modules and it assures only instance of the class is created.
//The value below was picked beacuse it is not user accessable memory & is on an odd boundry.
#define SYMBIAN_GLOBAL_VIDEO_PARAMETERS_ID 0x00000001
#define SYMBIAN_GLOBAL_AUDIO_DEVSOUND_ID   0x00000003
#define SYMBIAN_GLOBAL_LOGSYSTEM_ID        0x00000005
#define SYMBIAN_GLOBAL_MEMORY_MONITOR_ID   0x00000007
const int z_GlobalManTLSKey = 0x2c94f34d;

class HXGlobalManager;

class HXGlobalManInstance
{
public:
    static HXGlobalManager* GetInstance();
    static void SetInstance(HXGlobalManager* pInstance);
};

inline    
HXGlobalManager* HXGlobalManInstance::GetInstance()
{
    return (HXGlobalManager*)UserSvr::DllTls(z_GlobalManTLSKey);
}

inline
void HXGlobalManInstance::SetInstance(HXGlobalManager* pInstance)
{
    UserSvr::DllSetTls(z_GlobalManTLSKey, pInstance);
}

#endif /* SYMBIAN_GM_IMP_H */
