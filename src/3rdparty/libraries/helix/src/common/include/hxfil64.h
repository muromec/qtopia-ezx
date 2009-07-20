/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxfil64.h,v 1.2 2007/07/06 20:43:41 jfinnecy Exp $
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

#ifndef _HXFIL64_H_
#define _HXFIL64_H_

#include "hxcom.h"
#include "hxcomptr.h"

/****************************************************************************
 * 
 *  Interface:
 * 
 *      IHXFileObject64
 * 
 *  Purpose:
 * 
 *      64 bit methods for a file object
 * 
 *  IID_IHXFileObject64:
 * 
 *      {DC88C1A0-35D9-4682-85C4-83D693953553}
 * 
 */

 // {DC88C1A0-35D9-4682-85C4-83D693953553}
DEFINE_GUID(IID_IHXFileObject64, 
0xdc88c1a0, 0x35d9, 0x4682, 0x85, 0xc4, 0x83, 0xd6, 0x93, 0x95, 0x35, 0x53);


#undef  INTERFACE
#define INTERFACE   IHXFileObject64

DECLARE_INTERFACE_(IHXFileObject64, IUnknown)
{
    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)   (THIS_
                                REFIID riid,
                                void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)  (THIS) PURE;

    STDMETHOD_(ULONG32,Release) (THIS) PURE;

    /*
     *  IHXFileObject64 methods
     */


    /************************************************************************
     *  Method:
     *      IHXFileObject::Seek64
     *  Purpose:
     *
     *      Seeks to an offset in the file and asynchronously notifies the
     *      caller via the IHXFileResponse interface passed in to Init, of the
     *      completeness of the operation.  If the bRelative flag is TRUE, it is
     *      a relative seek; else an absolute seek.
     */
    STDMETHOD(Seek64) (THIS_
                       UINT64 ulOffset,
                       HXBOOL bRelative) PURE;    

};

DEFINE_SMART_PTR( IHXFileObject64 );
#endif // _HXFIL64_H_

