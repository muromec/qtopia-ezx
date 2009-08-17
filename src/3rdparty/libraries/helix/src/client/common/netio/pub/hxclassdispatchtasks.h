/* ***** BEGIN LICENSE BLOCK ***** 
 * Version: RCSL 1.0/RPSL 1.0 
 *  
 * Portions Copyright (c) 1995-2002 RealNetworks, Inc. All Rights Reserved. 
 *      
 * The contents of this file, and the files included with this file, are 
 * subject to the current version of the RealNetworks Public Source License 
 * Version 1.0 (the "RPSL") available at 
 * http://www.helixcommunity.org/content/rpsl unless you have licensed 
 * the file under the RealNetworks Community Source License Version 1.0 
 * (the "RCSL") available at http://www.helixcommunity.org/content/rcsl, 
 * in which case the RCSL will apply. You may also obtain the license terms 
 * directly from RealNetworks.  You may not use this file except in 
 * compliance with the RPSL or, if you have a valid RCSL with RealNetworks 
 * applicable to this file, the RCSL.  Please see the applicable RPSL or 
 * RCSL for the rights, obligations and limitations governing use of the 
 * contents of the file.  
 *  
 * This file is part of the Helix DNA Technology. RealNetworks is the 
 * developer of the Original Code and owns the copyrights in the portions 
 * it created. 
 *  
 * This file, and the files included with this file, is distributed and made 
 * available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER 
 * EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS ALL SUCH WARRANTIES, 
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS 
 * FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT. 
 * 
 * Technology Compatibility Kit Test Suite(s) Location: 
 *    http://www.helixcommunity.org/content/tck 
 * 
 * Contributor(s): 
 *  
 * ***** END LICENSE BLOCK ***** */ 


#if !defined(HXCLASSDISPATCHTASKS_H__)
#define HXCLASSDISPATCHTASKS_H__

//
// helpers for HXThreadedSocket
//

#include "hxthreadtask.h"

/**********************************************************

   ref-counting behavior for types used in class dispatch tasks below

***********************************************************/


// XXXLCM for now do nothing for IHXSockAddr's to avoid asserts (!)
static inline void HXCDAddRef(IHXSockAddr* p) {}
static inline void HXCDRelease(IHXSockAddr* p) {}

// use addref for IUnknown
static inline void HXCDAddRef(IUnknown* p) { if(p) {p->AddRef();} }
static inline void HXCDRelease(IUnknown* p) { if(p) {p->Release();} }

// use addref for IUnknown XXXLCM find better way
static inline void HXCDAddRef(IHXSocket* p) { if(p) {p->AddRef();} }
static inline void HXCDRelease(IHXSocket* p) { if(p) {p->Release();} }

// do nothing for everything else
static inline void HXCDAddRef(...) {}
static inline void HXCDRelease(...) {}



/**********************************************************

 HXClassDispatchTask
 
   HXThreadTask base class for dispatch tasks that invoke a 
   class member function and provide access to the return value.


***********************************************************/
template <typename RetType>
class HXClassDispatchTask
: public HXThreadTask
{
public:
    HXClassDispatchTask();
    RetType GetReturnValue() const;
protected:
    RetType m_returnValue;

};
template < typename RetType >
inline
HXClassDispatchTask<RetType>::HXClassDispatchTask()
: m_returnValue()
{
}
template < typename RetType >
inline
RetType HXClassDispatchTask<RetType>::GetReturnValue() const
{
    return m_returnValue;
}

/**********************************************************

 HXClassDispatch0
 
   HXThreadTask that invokes class member function taking no args


***********************************************************/
template < typename ClassType, typename RetType >
class HXClassDispatch0
: public HXClassDispatchTask<RetType>
{
public:

    typedef RetType (STDMETHODCALLTYPE ClassType::* MEMBERFUNCTIONPTR)();

    HXClassDispatch0(ClassType* pClass, MEMBERFUNCTIONPTR pFunction);
    ~HXClassDispatch0();
    void Execute();

private:
    ClassType* m_pClass;
    MEMBERFUNCTIONPTR m_pFunction;
};

template < typename ClassType, typename RetType >
inline
HXClassDispatch0<ClassType, RetType>::HXClassDispatch0(ClassType* pClass, MEMBERFUNCTIONPTR pFunction)
: m_pClass(pClass)
, m_pFunction(pFunction)
{
    HXCDAddRef(m_pClass);
}

template < typename ClassType, typename RetType >
inline
HXClassDispatch0<ClassType, RetType>::~HXClassDispatch0()
{
    HXCDRelease(m_pClass);
}

template < typename ClassType, typename RetType >
void HXClassDispatch0<ClassType, RetType>::Execute()
{
    HX_ASSERT(m_pClass != 0);
    HX_ASSERT(m_pFunction != 0);
    HXClassDispatchTask<RetType>::m_returnValue = (m_pClass->*m_pFunction)();
} 


/**********************************************************

 HXClassDispatch1
 
   HXThreadTask that invokes class member function taking one arg


***********************************************************/
template < typename ClassType, typename RetType, typename ArgType1 >
class HXClassDispatch1
: public HXClassDispatchTask<RetType>
{
public:

    typedef RetType (STDMETHODCALLTYPE ClassType::* MEMBERFUNCTIONPTR)(ArgType1);

    HXClassDispatch1(ClassType* pClass, MEMBERFUNCTIONPTR pFunction, ArgType1 arg1);
    ~HXClassDispatch1();
    void Execute();

private:
    ClassType* m_pClass;
    MEMBERFUNCTIONPTR m_pFunction;
    ArgType1 m_arg1;
};

template < typename ClassType, typename RetType, typename ArgType1 >
inline
HXClassDispatch1<ClassType, RetType, ArgType1>::HXClassDispatch1(ClassType* pClass, MEMBERFUNCTIONPTR pFunction, ArgType1 arg1)
: m_pClass(pClass)
, m_pFunction(pFunction)
, m_arg1(arg1)
{
    HXCDAddRef(m_pClass);
    HXCDAddRef(m_arg1);
}
template < typename ClassType, typename RetType, typename ArgType1 >
inline
HXClassDispatch1<ClassType, RetType, ArgType1>::~HXClassDispatch1()
{
    HXCDRelease(m_pClass);
    HXCDRelease(m_arg1);
}
template < typename ClassType, typename RetType, typename ArgType1 >
void HXClassDispatch1<ClassType, RetType, ArgType1>::Execute()
{
    HX_ASSERT(m_pClass != 0);
    HX_ASSERT(m_pFunction != 0);
    HXClassDispatchTask<RetType>::m_returnValue = (m_pClass->*m_pFunction)(m_arg1);
}

/**********************************************************

 HXClassDispatch2
 
   HXThreadTask that invokes class member function taking two args


***********************************************************/
template < typename ClassType, typename RetType, typename ArgType1, typename ArgType2>
class HXClassDispatch2
: public HXClassDispatchTask<RetType>
{
public:

    typedef RetType (STDMETHODCALLTYPE ClassType::* MEMBERFUNCTIONPTR)(ArgType1, ArgType2);

    HXClassDispatch2(ClassType* pClass, MEMBERFUNCTIONPTR pFunction, ArgType1 arg1, ArgType2 arg2);
    ~HXClassDispatch2();
    void Execute();

private:
    ClassType* m_pClass;
    MEMBERFUNCTIONPTR m_pFunction;
    ArgType1 m_arg1;
    ArgType2 m_arg2;
};

template < typename ClassType, typename RetType, typename ArgType1, typename ArgType2 >
inline
HXClassDispatch2<ClassType, RetType, ArgType1, ArgType2>::HXClassDispatch2(ClassType* pClass, MEMBERFUNCTIONPTR pFunction, ArgType1 arg1, ArgType2 arg2)
: m_pClass(pClass)
, m_pFunction(pFunction)
, m_arg1(arg1)
, m_arg2(arg2)
{
    HXCDAddRef(m_pClass);
    HXCDAddRef(m_arg1);
    HXCDAddRef(m_arg2);
}

template < typename ClassType, typename RetType, typename ArgType1, typename ArgType2 >
inline
HXClassDispatch2<ClassType, RetType, ArgType1, ArgType2>::~HXClassDispatch2()
{
    HXCDRelease(m_pClass);
    HXCDRelease(m_arg1);
    HXCDRelease(m_arg2);
}
template < typename ClassType, typename RetType, typename ArgType1, typename ArgType2 >
void HXClassDispatch2<ClassType, RetType, ArgType1, ArgType2>::Execute()
{
    HX_ASSERT(m_pClass != 0);
    HX_ASSERT(m_pFunction != 0);
    HXClassDispatchTask<RetType>::m_returnValue = (m_pClass->*m_pFunction)(m_arg1, m_arg2);
}

/**********************************************************

 HXClassDispatch3
 
   HXThreadTask that invokes class member function taking three args


***********************************************************/
template < typename ClassType, typename RetType, typename ArgType1, typename ArgType2, typename ArgType3>
class HXClassDispatch3
: public HXClassDispatchTask<RetType>
{
public:

    typedef RetType (STDMETHODCALLTYPE ClassType::* MEMBERFUNCTIONPTR)(ArgType1, ArgType2, ArgType3);

    HXClassDispatch3(ClassType* pClass, MEMBERFUNCTIONPTR pFunction, ArgType1 arg1, ArgType2 arg2, ArgType3 arg3);
    ~HXClassDispatch3();
    void Execute();

private:
    ClassType* m_pClass;
    MEMBERFUNCTIONPTR m_pFunction;
    ArgType1 m_arg1;
    ArgType2 m_arg2;
    ArgType3 m_arg3;
};

template < typename ClassType, typename RetType, typename ArgType1, typename ArgType2, typename ArgType3>
inline
HXClassDispatch3<ClassType, RetType, ArgType1, ArgType2, ArgType3>::HXClassDispatch3(ClassType* pClass, MEMBERFUNCTIONPTR pFunction, ArgType1 arg1, ArgType2 arg2, ArgType3 arg3)
: m_pClass(pClass)
, m_pFunction(pFunction)
, m_arg1(arg1)
, m_arg2(arg2)
, m_arg3(arg3)
{
    HXCDAddRef(m_pClass);
    HXCDAddRef(m_arg1);
    HXCDAddRef(m_arg2);
    HXCDAddRef(m_arg3);
}
template < typename ClassType, typename RetType, typename ArgType1, typename ArgType2, typename ArgType3>
inline
HXClassDispatch3<ClassType, RetType, ArgType1, ArgType2, ArgType3>::~HXClassDispatch3()
{
    HXCDRelease(m_pClass);
    HXCDRelease(m_arg1);
    HXCDRelease(m_arg2);
    HXCDRelease(m_arg3);
}
template < typename ClassType, typename RetType, typename ArgType1, typename ArgType2, typename ArgType3 >
void HXClassDispatch3<ClassType, RetType, ArgType1, ArgType2, ArgType3>::Execute()
{
    HX_ASSERT(m_pClass != 0);
    HX_ASSERT(m_pFunction != 0);
    HXClassDispatchTask<RetType>::m_returnValue = (m_pClass->*m_pFunction)(m_arg1, m_arg2, m_arg3);
}


/**********************************************************

 HXClassDispatch4
 
   HXThreadTask that invokes class member function taking four args


***********************************************************/
template < typename ClassType, typename RetType, typename ArgType1, typename ArgType2, typename ArgType3, typename ArgType4>
class HXClassDispatch4
: public HXClassDispatchTask<RetType>
{
public:

    typedef RetType (STDMETHODCALLTYPE ClassType::* MEMBERFUNCTIONPTR)(ArgType1, ArgType2, ArgType3, ArgType4);

    HXClassDispatch4(ClassType* pClass, MEMBERFUNCTIONPTR pFunction, ArgType1 arg1, ArgType2 arg2, ArgType3 arg3, ArgType4 arg4);
    ~HXClassDispatch4();
    void Execute();

private:
    ClassType* m_pClass;
    MEMBERFUNCTIONPTR m_pFunction;
    ArgType1 m_arg1;
    ArgType2 m_arg2;
    ArgType3 m_arg3;
    ArgType4 m_arg4;
};

template < typename ClassType, typename RetType, typename ArgType1, typename ArgType2, typename ArgType3, typename ArgType4>
inline
HXClassDispatch4<ClassType, RetType, ArgType1, ArgType2, ArgType3, ArgType4>::HXClassDispatch4(ClassType* pClass, MEMBERFUNCTIONPTR pFunction, ArgType1 arg1, ArgType2 arg2, ArgType3 arg3, ArgType4 arg4)
: m_pClass(pClass)
, m_pFunction(pFunction)
, m_arg1(arg1)
, m_arg2(arg2)
, m_arg3(arg3)
, m_arg4(arg4)
{
    HXCDAddRef(m_pClass);
    HXCDAddRef(m_arg1);
    HXCDAddRef(m_arg2);
    HXCDAddRef(m_arg3);
    HXCDAddRef(m_arg4);
}
template < typename ClassType, typename RetType, typename ArgType1, typename ArgType2, typename ArgType3, typename ArgType4>
inline
HXClassDispatch4<ClassType, RetType, ArgType1, ArgType2, ArgType3, ArgType4>::~HXClassDispatch4()
{
    HXCDRelease(m_pClass);
    HXCDRelease(m_arg1);
    HXCDRelease(m_arg2);
    HXCDRelease(m_arg3);
    HXCDRelease(m_arg4);
}
template < typename ClassType, typename RetType, typename ArgType1, typename ArgType2, typename ArgType3, typename ArgType4>
void HXClassDispatch4<ClassType, RetType, ArgType1, ArgType2, ArgType3, ArgType4>::Execute()
{
    HX_ASSERT(m_pClass != 0);
    HX_ASSERT(m_pFunction != 0);
    HXClassDispatchTask<RetType>::m_returnValue = (m_pClass->*m_pFunction)(m_arg1, m_arg2, m_arg3, m_arg4);
}


//
// Use these to create an instance of HXClassDispatchTask so you don't have to
// expliticly list template args
//
// Example:
//
// HXClassDispatch0* pDisp0 = AllocDispatch0(pMyClass, &MyClass::Func);
// HXClassDispatch1* pDisp1 = AllocDispatch1(pMyClass, &MyClass::Func, arg);
// HXClassDispatch2* pDisp2 = AllocDispatch2(pMyClass, &MyClass::Func, arg, arg2);
// HXClassDispatch3* pDisp3 = AllocDispatch3(pMyClass, &MyClass::Func, arg, arg2, arg3);
// HXClassDispatch3* pDisp4 = AllocDispatch4(pMyClass, &MyClass::Func, arg, arg2, arg3, arg4);
//
template< typename ClassType, typename RetType >
inline
HXClassDispatch0 <ClassType, RetType> *
AllocDispatch0(  ClassType* pClass, RetType (STDMETHODCALLTYPE ClassType::* pMemFunc)() )
{
    return new HXClassDispatch0<ClassType, RetType>(pClass, pMemFunc);
}
template< typename ClassType, typename RetType, typename ArgType1 >
inline
HXClassDispatch1 <ClassType, RetType, ArgType1> *
AllocDispatch1(  ClassType* pClass, RetType (STDMETHODCALLTYPE ClassType::* pMemFunc)(ArgType1), ArgType1 arg1 )
{
    return new HXClassDispatch1<ClassType, RetType, ArgType1>(pClass, pMemFunc, arg1);
}
template< typename ClassType, typename RetType, typename ArgType1, typename ArgType2 >
inline
HXClassDispatch2 <ClassType, RetType, ArgType1, ArgType2> *
AllocDispatch2(  ClassType* pClass, RetType (STDMETHODCALLTYPE ClassType::* pMemFunc)(ArgType1,ArgType2), ArgType1 arg1, ArgType2 arg2 )
{
    return new HXClassDispatch2<ClassType, RetType, ArgType1, ArgType2>(pClass, pMemFunc, arg1, arg2);
}
template< typename ClassType, typename RetType, typename ArgType1, typename ArgType2, typename ArgType3 >
inline
HXClassDispatch3 <ClassType, RetType, ArgType1, ArgType2, ArgType3> *
AllocDispatch3(  ClassType* pClass, RetType (STDMETHODCALLTYPE ClassType::* pMemFunc)(ArgType1,ArgType2,ArgType3), ArgType1 arg1, ArgType2 arg2, ArgType3 arg3 )
{
    return new HXClassDispatch3<ClassType, RetType, ArgType1, ArgType2, ArgType3>(pClass, pMemFunc, arg1, arg2, arg3);
}
template< typename ClassType, typename RetType, typename ArgType1, typename ArgType2, typename ArgType3, typename ArgType4 >
inline
HXClassDispatch4 <ClassType, RetType, ArgType1, ArgType2, ArgType3, ArgType4> *
AllocDispatch4(  ClassType* pClass, RetType (STDMETHODCALLTYPE ClassType::* pMemFunc)(ArgType1,ArgType2,ArgType3,ArgType4), ArgType1 arg1, ArgType2 arg2, ArgType3 arg3, ArgType4 arg4)
{
    return new HXClassDispatch4<ClassType, RetType, ArgType1, ArgType2, ArgType3, ArgType4>(pClass, pMemFunc, arg1, arg2, arg3, arg4);
}



#endif //HXCLASSDISPATCHTASKS_H__





