/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxordval.h,v 1.5 2007/07/06 20:35:02 jfinnecy Exp $
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

/****************************************************************************
 * 
 *	IHXValues implementation that preserves order when iterating.
 *	This class wraps HXBuffer, buts maintains a linked-list of the
 *	elements that are added to the IHXValues, so we can iterate
 *	through in the same order they were added.
 *
 *	Implemented using SimpleList, so this class is 
 *	inefficient for large collections.
 *
 *	Only the CString methods are supported in this initial implementation
 *
 */

#ifndef _HXORDVAL_H_
#define _HXORDVAL_H_

#include "unkimp.h"
#include "ihxpckts.h"
#include "hxslist.h"

class CHXOrderedValues : public IHXValues,
			 public CUnknownIMP
{
    // the IUnknown implementation declaration
    DECLARE_UNKNOWN(CHXOrderedValues)

public:
	CHXOrderedValues();

    /*
     *	IHXValues methods
     */

    STDMETHOD(SetPropertyULONG32)	(THIS_
					const char*	 pPropertyName,
					ULONG32		 uPropertyValue);

    STDMETHOD(GetPropertyULONG32)	(THIS_
					const char*	 pPropertyName,
					REF(ULONG32)	 uPropertyValue);

    STDMETHOD(GetFirstPropertyULONG32)	(THIS_
					REF(const char*) pPropertyName,
					REF(ULONG32)	 uPropertyValue);

    STDMETHOD(GetNextPropertyULONG32)	(THIS_
					REF(const char*) pPropertyName,
					REF(ULONG32)	 uPropertyValue);


    STDMETHOD(SetPropertyBuffer)	(THIS_
					const char*	 pPropertyName,
					IHXBuffer*	 pPropertyValue);

    STDMETHOD(GetPropertyBuffer)	(THIS_
					const char*	 pPropertyName,
					REF(IHXBuffer*) pPropertyValue);

    STDMETHOD(GetFirstPropertyBuffer)	(THIS_
					REF(const char*) pPropertyName,
					REF(IHXBuffer*) pPropertyValue);

    STDMETHOD(GetNextPropertyBuffer)	(THIS_
					REF(const char*) pPropertyName,
					REF(IHXBuffer*) pPropertyValue);

    STDMETHOD(SetPropertyCString)	(THIS_
					const char*	 pPropertyName,
					IHXBuffer*	 pPropertyValue);

    STDMETHOD(GetPropertyCString)	(THIS_
					const char*	 pPropertyName,
					REF(IHXBuffer*) pPropertyValue);

    STDMETHOD(GetFirstPropertyCString)	(THIS_
					REF(const char*) pPropertyName,
					REF(IHXBuffer*) pPropertyValue);

    STDMETHOD(GetNextPropertyCString)	(THIS_
					REF(const char*) pPropertyName,
					REF(IHXBuffer*) pPropertyValue);

private:
    ~CHXOrderedValues();
    LISTPOSITION FindCStringName (const char* pPropertyName);

    CHXSimpleList   m_CStringList;
    LISTPOSITION    m_CStringPos;


};

#endif /* _HXORDVAL_H_ */
