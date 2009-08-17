/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxglobalmap.h,v 1.4 2004/07/09 18:19:17 hubbe Exp $
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

#ifndef __hxglobalmap_h
#define __hxglobalmap_h


#include "hxglobalmgr.h"
#include "chxmapptrtoptr.h"
#include "chxmaplongtoobj.h"
#include "chxmapguidtoobj.h"
#include "chxmapstringtoob.h"
#include "chxmapstringtostring.h"


class HXGlobalMapPtrToPtr
{
public:
    static CHXMapPtrToPtr& Get(GlobalID id);

protected:
    static CHXMapPtrToPtr* New()	{ return new CHXMapPtrToPtr; }
    static void Delete(GlobalType ptr)	{ delete (CHXMapPtrToPtr*)ptr; }
};


class HXGlobalMapLongToObj
{
public:
    static CHXMapLongToObj& Get(GlobalID id);

protected:
    static CHXMapLongToObj* New()	{ return new CHXMapLongToObj; }
    static void Delete(GlobalType ptr)	{ delete (CHXMapLongToObj*)ptr; }
};


class HXGlobalMapGUIDToObj
{
public:
    static CHXMapGUIDToObj& Get(GlobalID id);

protected:
    static CHXMapGUIDToObj* New()	{ return new CHXMapGUIDToObj; }
    static void Delete(GlobalType ptr)	{ delete (CHXMapGUIDToObj*)ptr; }
};


class HXGlobalMapStringToOb
{
public:
    static CHXMapStringToOb& Get(GlobalID id);

protected:
    static CHXMapStringToOb* New()	{ return new CHXMapStringToOb; }
    static void Delete(GlobalType ptr)	{ delete (CHXMapStringToOb*)ptr; }
};


class HXGlobalMapStringToString
{
public:
    static CHXMapStringToString& Get(GlobalID id);

protected:
    static CHXMapStringToString* New()	{ return new CHXMapStringToString; }
    static void Delete(GlobalType ptr)	{ delete (CHXMapStringToString*)ptr; }
};


#endif /* #ifndef __hxglobalmap_h */

