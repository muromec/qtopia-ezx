/* ***** BEGIN LICENSE BLOCK *****
* Source last modified: $Id: hxrangedtype.h,v 1.3 2007/07/06 21:58:18 jfinnecy Exp $
* 
* Portions Copyright (c) 1995-2006 RealNetworks, Inc. All Rights Reserved.
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

/*!
  @header rnrangedtype.h
  @discussion this macro is a bit annoying becuase it has no powers of deduction.
   	      with a template I could ensure that calling an operator with an rhs of a  different
   	      type (say, signed instead of unsigned) wouldn't just cast it to signed. this
   	      produces unexpected results. 
   	      
   	      RN_RANGED_TYPE_INLINE (CPositiveInteger, unsigned, 0, 0xFFFFFFFF);
   	      CPositiveInteger = -5; 
   	      
   	      That won't through an exception because -5 will just be casted to an unsigned,
   	      with a template any method could be templated to take any type without
   	      casting it.
   	      
   	      So, having said that. This macro is best used either 
   	      a) in situations where the same type will always be used with it.
   	      b) with a signed value as the base type.
   	      
   	      
   	      TO DO: add *=, /=, etc... I don't really see these being all that necessary,
   	      this is really a simple macro & has very specific uses. In the long term templates
   	      will be needed to come up with anything reasonably robust.
 */

#ifndef _RNRANGEDTYPE_H_
#define _RNRANGEDTYPE_H_


// rnrangedtype.h
#define HX_RANGED_TYPE_INLINE(RN_RT_NAME,RN_RT_TYPE,RN_RT_MIN,RN_RT_MAX) \
    class RN_RT_NAME \
    { \
    public: \
	typedef RN_RT_TYPE value_type; \
	\
	RN_RT_NAME (value_type const val = value_type ()) \
	    : m_min (RN_RT_MIN), m_max (RN_RT_MAX), m_exceptional (FALSE) \
	{ \
	    if (val > m_max) \
	    { \
	        m_value = m_max; \
	        SetException_ (); \
	    } \
	    else if (val < m_min) \
	    { \
	        m_value = m_min; \
	        SetException_ (); \
	    } \
	    else m_value = val; \
	} \
	\
	virtual RN_RT_NAME const& operator= (value_type rhs) \
	{ \
	    if (rhs > m_max) \
	    { \
	        m_value = m_max; \
	        SetException_ (); \
	    } \
	    else if (rhs < m_min) \
	    { \
	        m_value = m_min; \
	        SetException_ (); \
	    } \
	    else m_value = rhs; \
	    \
	    return *this; \
	} \
	\
	RN_RT_NAME operator++ (int) \
	{ \
	    if (!DoCanModify_ ()) return *this; \
	    value_type const preincrement = m_value; \
	    if (m_value < m_max) ++m_value; \
	    else SetException_ (); \
	    return preincrement; \
	} \
	\
	RN_RT_NAME operator++ () \
	{ \
	    if (!DoCanModify_ ()) return *this; \
	    if (m_value < m_max) ++m_value; \
	    else SetException_ (); \
	    return m_value; \
	} \
	\
	RN_RT_NAME operator-- (int) \
	{ \
	    if (!DoCanModify_ ()) return *this; \
	    value_type const predecrement = m_value; \
	    if (m_value > m_min) --m_value; \
	    else SetException_ (); \
	    return predecrement; \
	} \
	\
	RN_RT_NAME operator-- () \
	{ \
	    if (!DoCanModify_ ()) return *this; \
	    if (m_value > m_min) --m_value; \
	    else SetException_ (); \
	    return m_value; \
	} \
	\
	RN_RT_NAME operator+= (value_type const rhs) \
	{ \
	    if (!DoCanModify_ ()) return *this; \
	    Add_ (rhs); \
	    return *this; \
	} \
	\
	RN_RT_NAME operator-= (value_type const rhs) \
	{ \
	    if (!DoCanModify_ ()) return *this; \
	    Subtract_ (rhs); \
	    return *this; \
	} \
	\
	operator value_type () const { return m_value; } \
	\
	HXBOOL ResetException ()  \
	{ \
	    HXBOOL const state = m_exceptional; \
	    m_exceptional = FALSE; \
	    return state; \
	} \
	\
    private: \
	virtual HXBOOL DoCanModify_ () { return TRUE; } \
	void SetException_ () { m_exceptional = TRUE; } \
	void Add_ (value_type const rhs) \
	{ \
	    if (rhs > 0) \
	    { \
	        if (m_value + rhs < m_value || m_value + rhs > m_max) \
	        { \
	            m_value = m_max; \
	            SetException_ (); \
	        } \
	        else m_value += rhs; \
	    } \
	    else if (rhs < 0) \
	    { \
	       if (m_value + rhs > m_value || m_value + rhs < m_min) \
	       { \
	           m_value = m_min; \
	           SetException_ (); \
	       } \
	       else m_value += rhs; \
	    } \
	} \
	\
	void Subtract_ (value_type const rhs) \
	{ \
	    if (rhs > 0) \
	    { \
	        if (m_value - rhs > m_value || m_value - rhs < m_min) \
	        { \
	            m_value = m_min; \
	            SetException_ (); \
	        } \
	        else m_value -= rhs; \
	    } \
	    else if (rhs < 0) \
	    { \
	       if (m_value - rhs < m_value || m_value - rhs > m_max) \
	       { \
	           m_value = m_max; \
	           SetException_ (); \
	       } \
	       else m_value -= rhs; \
	    } \
	} \
	\
	value_type m_value; \
	value_type const m_min; \
	value_type const m_max; \
	HXBOOL m_exceptional; \
    }

#endif
