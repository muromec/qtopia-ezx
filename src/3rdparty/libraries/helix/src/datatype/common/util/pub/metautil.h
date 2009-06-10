/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: metautil.h,v 1.4 2007/07/06 22:00:24 jfinnecy Exp $
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

#ifndef METAUTIL_H
#define METATUIL_H

/*
 * Users of the IHXFileFormatObject interface can request
 * meta info from the file format by passing in the
 * string "AcceptMetaInfo" in the request header.
 * This property, if present, is either a delimited
 * list of requested property names or the string "*".
 * If the string is "*", then all available meta info
 * is requested. The delimiter can be either a comma
 * or a semi-colon. All meta info requested (or all
 * available if AcceptMetaInfo = "*") will then be
 * placed in the file header returned asynchronously
 * to the IHXFileFormatObject::GetFileHeader() call.
 *
 * In the function below, an IHXRequest is passed
 * in. If HX_RESULT returns HXR_OK, then the other
 * variables passed in by referenced can then be
 * examined in this order:
 *
 * If "AcceptMetaInfo" is present in the request headers,
 *   then rbAcceptMetaInfoPresent will be TRUE. Otherwise,
 *   it will be FALSE and rbAllMetaInfoRequested and
 *   rpRequestedMetaInfo will not be altered.
 *
 * If rbAcceptMetaInfoPresent is TRUE and
 *   AcceptMetaInfo = "*", then rbAllMetaInfoRequested
 *   will be TRUE and rpRequestedMetaInfo will be NULL.
 *
 * If rbAcceptMetaInfoPresent is TRUE and
 *   AcceptMetaInfo is a delimited string of property
 *   names, then rbAllMetaInfoRequested will be FALSE
 *   and rpRequestedMetaInfo will be non-NULL and will
 *   contain this list of property names as ULONG32
 *   properties. These property names can then be retrieved
 *   by using the IHXValues::GetFirstPropertyULONG32()
 *   and IHXValues::GetNextPropertyULONG32() calls.
 *   Note that this does not imply that the properties
 *   are necessarily ULONG32 properties - it's just
 *   more convenient to make them ULONG32 properties
 *   than CString or Buffer properties.
 */

HX_RESULT CheckMetaInfoRequest(IHXRequest*            pRequest,
                               IHXCommonClassFactory* pFactory,
                               REF(HXBOOL)              rbAcceptMetaInfoPresent,
                               REF(HXBOOL)              rbAllMetaInfoRequested,
                               REF(IHXValues*)        rpRequestedMetaInfo);


#endif /* #ifndef METAUTIL_H */
