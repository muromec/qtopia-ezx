/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: arrenum.h,v 1.3 2004/07/09 18:21:27 hubbe Exp $
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

#include "unkimp.h"

/* This class implements the fragment Enumerator
 */
#define DECLARE_INTERFACE_ARRAY_ENUMERATOR(ENUMERATED, IENUMERATE)	\
    class _C##IENUMERATE##IMP                                           \
	: public IENUMERATE                                             \
	, public CUnknownIMP                                            \
    {                                                                   \
	DECLARE_UNKNOWN(_C##IENUMERATE##IMP)				\
    public:                                                             \
	_C##IENUMERATE##IMP()                                           \
	    : m_arrpbufData(NULL)                                       \
	    , m_ulIndex(0)                                              \
	{}                                                              \
	~_C##IENUMERATE##IMP()                                          \
	{_KillData();}                                                  \
	STDMETHOD(Reset)(THIS)                                          \
	{                                                               \
	    m_ulIndex = 0;                                              \
	    return HXR_OK;                                              \
	}                                                               \
	STDMETHOD(Next)                                                 \
	(                                                               \
	    THIS_                                                       \
	    UINT32          ulNumToReturn,                              \
	    ENUMERATED**    ppbufNext,                                  \
	    UINT32*         pulNumReturned                              \
	)                                                               \
	{                                                               \
	    if(!ppbufNext || (ulNumToReturn != 1 && pulNumReturned == NULL))    \
	    {                                                           \
		return HXR_POINTER;                                     \
	    }                                                           \
	    if(!m_arrpbufData || !m_ulTotal)                            \
	    {                                                           \
		return HXR_FAIL;                                        \
	    }                                                           \
	    HX_RESULT pnrRes = HXR_OK;                                  \
	    UINT32 ulRemaining = m_ulTotal-m_ulIndex;                   \
	    if(ulNumToReturn > ulRemaining)                             \
	    {                                                           \
		pnrRes = HXR_INCOMPLETE;                                \
		ulNumToReturn = ulRemaining;                            \
	    }                                                           \
	    else                                                        \
	    {                                                           \
		ulRemaining = ulNumToReturn;                            \
	    }                                                           \
	    if (pulNumReturned)						\
	    {								\
		*pulNumReturned = ulNumToReturn;                        \
	    }								\
	    while(ulRemaining)                                          \
	    {                                                           \
		(                                                       \
		    ppbufNext[ulRemaining-ulNumToReturn]		\
		    = m_arrpbufData[m_ulIndex]                          \
		)->AddRef();                                            \
		++m_ulIndex;                                            \
		--ulRemaining;                                          \
	    }                                                           \
	    return pnrRes;                                              \
	}                                                               \
	STDMETHOD(Skip)                                                 \
	(                                                               \
	    THIS_                                                       \
	    UINT32 ulNumToSkip                                          \
	)                                                               \
	{                                                               \
	    m_ulIndex += ulNumToSkip;                                   \
	    if (m_ulIndex < m_ulTotal)                                  \
	    {                                                           \
		return HXR_OK;                                            \
	    }                                                           \
	    m_ulIndex = m_ulTotal;                                      \
	    return HXR_FAIL;                                             \
	}                                                               \
	STDMETHOD(Clone)                                                \
	(                                                               \
	    THIS_                                                       \
	    IENUMERATE** ppefbNew                                       \
	)                                                               \
	{                                                               \
	    ENUMERATED** arrpbufData = NULL;                            \
	    if(m_arrpbufData && m_ulTotal)                              \
	    {                                                           \
		UINT32 ulIndex = 0;                                     \
		arrpbufData = (ENUMERATED**)new ENUMERATED*[m_ulTotal];   \
		for                                                     \
		(                                                       \
		    ulIndex = 0;                                        \
		    ulIndex < m_ulTotal;                                \
		    ++ulIndex                                           \
		)                                                       \
		{                                                       \
		    arrpbufData[ulIndex] = m_arrpbufData[ulIndex];      \
		    (arrpbufData[ulIndex])->AddRef();                   \
		}                                                       \
	    }                                                           \
	    _C##IENUMERATE##IMP* pNewEnum = CreateObject();             \
	    pNewEnum->_SetData(arrpbufData, m_ulTotal, m_ulIndex);      \
	    return pNewEnum->QueryInterface(IID_##IENUMERATE, (void**)ppefbNew);   \
	}                                                               \
	void _SetData                                                   \
	(                                                               \
	    ENUMERATED** arrpbufData,                                   \
	    UINT32 ulTotal,                                             \
	    UINT32 ulIndex = 0                                          \
	)                                                               \
	{                                                               \
	    _KillData();                                                \
	    m_arrpbufData = arrpbufData;                                \
	    m_ulTotal = ulTotal;                                        \
	    m_ulIndex = ulIndex;                                        \
	}                                                               \
    private:                                                            \
	void _KillData()                                                \
	{                                                               \
	    if(m_arrpbufData)                                           \
	    {                                                           \
		for                                                     \
		(                                                       \
		    m_ulIndex = 0;                                      \
		    m_ulIndex < m_ulTotal;                              \
		    ++m_ulIndex                                         \
		)                                                       \
		{                                                       \
		    (m_arrpbufData[m_ulIndex])->Release();              \
		}                                                       \
		delete[] m_arrpbufData;                                 \
		m_ulIndex = 0;                                          \
	    }                                                           \
	}                                                               \
	ENUMERATED**    m_arrpbufData;                                  \
	UINT32          m_ulIndex;                                      \
	UINT32          m_ulTotal;                                      \
    }                                                                  \

#define IMPLEMENT_INTERFACE_ARRAY_ENUMERATOR(IENUMERATE)		\
    BEGIN_INTERFACE_LIST(_C##IENUMERATE##IMP)				\
	INTERFACE_LIST_ENTRY(IID_##IENUMERATE,  IENUMERATE)		\
    END_INTERFACE_LIST


#define CREATE_INTERFACE_ARRAY_ENUMERATOR(IENUMERATE, ARRAY, TOTAL, NEWENUM)        \
    {                                                                               \
	_C##IENUMERATE##IMP* _pNewEnum = _C##IENUMERATE##IMP::CreateObject();       \
	_pNewEnum->_SetData(ARRAY, TOTAL);                                          \
	_pNewEnum->QueryInterface(IID_##IENUMERATE, (void**)NEWENUM);   \
    }

