/* ***** BEGIN LICENSE BLOCK *****
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

/*!
  @header rnsingleton.h
  
  @abstract Macro implementation of a singleton. Ultimately this should 
            probably be included in pnmisc or include. 
 
  @discussion
 	A singleton is a single-instance concept. The singleton macro allows
	a user to declare & define a class without having to make any major
	changes to account for the class being used as a singleton. A user
	also has the possibility of having a class that can be used as a 
	singleton or used as a multi-instance object. It is also possible
	to adapt a pre-existing class into a singleton without modifying
	its definition. <br><br>

	<i>note: the above support is provided because some objects are 
	only singletons within a context. A screen object within one application
	might be restricted to being used as a singleton, but in another
	the concept of multiple screens might be supported. This is the exception,
	not the rule.</i> <br><br>

	The following macros are provided by this header:

	<ul>
	    <li> RN_CREATE_AS_SINGLETON (singleton, implementation); - this macro
	    should be placed at the bottom of the class declaration. It
	    grants access to the classes constructor & destructor. Pass
	    the singleton class name and the implementation name as parameters.
	    <li> RN_DECLARE_SINGLETON (singleton, implementation); - this macro
	    declares the classes necessary to provide the singleton. The first 
	    parameter should be hte name of your singleton class, the second
	    should be the name of the implementation class. This macro would
	    usually be used in the header after the class declaration.
	    <li> RN_IMPLEMENT_SINGLETON (singleton, implementation); - this macro
	    implements the singleton. It is used like the macro above but shold
	    generally be placed in with the class definition.
	</ul>
 
 	Singletons provide two methods of use: 
	<ul>
	    <li> overloaded pointer-to and derferencing operators.
	    <li> static Instance () method:<br>
	    <code>static IMP& Instance ();</code>
	</ul>

	In the authors opinion the first method of use is preferrable. The Instance ()
	method is provided for those who will only be using a method of the singleton
	object once. Below are some sample usages.
    
    <br><br>
    <b>Implementing a Singleton</b>

    <pre><code>
    // blender.h
    class CSingletonOnlyBlender_
    {
	public:
	    void Frappe ();

	private:
	    CSingletonOnlyBlender_ ();
	    ~CSingletonOnlyBlender_ ();

	    RN_CREATE_AS_SINGLETON (HBlender, CSingletonOnlyBlender_);
    };

    RN_DECLARE_SINGLETON (HBlender, CSingletonOnlyBlender_);
    
    // blender.cpp
    CSingletonOnlyBlender_::CSingletonOnlyBlender_ ()
    {
	// code
    }

    CSingletonOnlyBlender_::~CSingletonOnlyBlender_ ()
    {
	// code
    }

    void CSingletonOnlyBlender_::Frappe ()
    {
	// frappe... unfortunately our blender doesn't yet support liquify
    }

    RN_IMPLEMENT_SINGLETON (HBlender, CSingletonOnlyBlender_);
    </code></pre>

    <br><br>
    <b>Using a Singleton as a "Normal" Object</b>
    <pre><code>
    class CChef
    {
	public:
	    void MakeSmoothie () { hBlender->Frappe (); }

	private:
	    HBlender hBlender;
    };
    </code></pre>

    <br><br>
    <b>One-time Use</b>
    <pre><code>
    class CRobber
    {
	public:
	    void TripOverBlender () const
	    {
	       HBlender::Instance ().Frappe ();
	    }
    };
    </pre></code>
 
*/

#ifndef _HXSINGLETON_H_
#define _HXSINGLETON_H_

#define HX_CREATE_AS_SINGLETON(s,i) \
    friend class s; \
    friend class CRNDestroyerOf##i

#define HX_DECLARE_SINGLETON(s,i) \
    class CRNDestroyerOf##i \
    { \
    	public: \
    	    ~CRNDestroyerOf##i (); \
    	    \
    	private: \
		CRNDestroyerOf##i (); \
		void Set_ (i* instance); \
		\
		i* m_pInstance; \
		\
    	friend class s; \
    }; \
    \
    class s \
    { \
	public: \
	    s () {} \
	    \
	    i* const operator-> (); \
	    i const* const operator-> () const; \
	    i& operator* (); \
	    i const& operator* () const; \
	    \
	    static i& Instance (); \
	    \
	private: \
	    void EnsureSingleton_ () const; \
	    \
	    static i* m_pInstance; \
	    static CRNDestroyerOf##i m_Destroyer; \
    }
	
#define HX_IMPLEMENT_SINGLETON(s,i) \
    i* const s::operator-> () \
    { \
	EnsureSingleton_ (); \
        return m_pInstance; \
    } \
    \
    i const* const s::operator-> () const \
    { \
        EnsureSingleton_ (); \
	return m_pInstance; \
    } \
    \
    i& s::operator* () \
    { \
        EnsureSingleton_ (); \
	return *m_pInstance; \
    } \
    \
    i const& s::operator*() const \
    { \
        EnsureSingleton_ (); \
	return *m_pInstance; \
    } \
    \
    i& s::Instance () \
    { \
        static s self; \
        return *self; \
    } \
    \
    void s::EnsureSingleton_ () const \
    { \
	if (!m_pInstance) \
	{ \
	    m_pInstance = new i; \
	    m_Destroyer.Set_ (m_pInstance); \
	} \
    } \
    \
    CRNDestroyerOf##i::CRNDestroyerOf##i () \
       : m_pInstance (0) {} \
    \
    CRNDestroyerOf##i::~CRNDestroyerOf##i () \
    { \
        delete m_pInstance; \
    } \
    \
    void CRNDestroyerOf##i::Set_ (i* instance) \
    { \
	m_pInstance = instance; \
    } \
    \
    i* s::m_pInstance = 0; \
    CRNDestroyerOf##i s::m_Destroyer

#endif
