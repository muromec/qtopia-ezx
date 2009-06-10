/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxflche.h,v 1.3 2007/07/06 20:48:08 jfinnecy Exp $
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

#ifndef _HXFLCHE_H_
#define _HXFLCHE_H_

/*
 * Forward declarations of some interfaces defined herein.
 */
typedef _INTERFACE  IHXFileSystemCache IHXFileSystemCache;


/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXFileSystemCache
 * 
 *  Purpose:
 * 
 *	Provides routines for manipulating the file system cache.
 * 
 *  IID_IHXFileSystemCache:
 * 
 *	{18D8A780-F90D-11d2-AD55-00C0F031C236}
 * 
 */
DEFINE_GUID(IID_IHXFileSystemCache, 0x18d8a780, 0xf90d, 0x11d2, 0xad, 0x55, 0x0, 0xc0, 0xf0, 0x31, 0xc2, 0x36);


#undef  INTERFACE
#define INTERFACE   IHXFileSystemCache

DECLARE_INTERFACE_(IHXFileSystemCache, IUnknown)
{
    // IUnknown methods...
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;
    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    // IHXFileSystemCache methods...
    /************************************************************************
     *	Method:
     *	    IHXFileSystemCache::RefreshCache
     *
     *	Purpose:
     *	    Provides a means for refreshing the cache with values for
     *	    such things as size of cache and any other parameters needed.
     *	    
     */
    STDMETHOD (RefreshCache) (THIS) PURE;

    /************************************************************************
     *	Method:
     *	    IHXFileSystemCache::EmptyCache
     *
     *	Purpose:
     *	    Deletes all files in the cache immediately.  The directory 
     *	    will not be deleted.
     *
     */
    STDMETHOD (EmptyCache) (THIS) PURE;

    /************************************************************************
     *	Method:
     *	    IHXFileSystemCache::MoveCache
     *
     *	Purpose:
     *	    Moves the cache from the current location to the new location.
     *	    All files in the previous location are moved to the new location.
     *	    The path points to a directory.  If not, a directory is created 
     *	    with that name.
     *
     */
    STDMETHOD (MoveCache) (THIS_ const char *path) PURE;

};



#endif //_HXFLCHE_H_
