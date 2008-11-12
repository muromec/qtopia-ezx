/****************************************************************************
**
** Copyright (C) 1992-2008 Trolltech ASA. All rights reserved.
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** This file may be used under the terms of the GNU General Public
** License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the files LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file.  Alternatively you may (at
** your option) use any later version of the GNU General Public
** License if such license has been publicly approved by Trolltech ASA
** (or its successors, if any) and the KDE Free Qt Foundation. In
** addition, as a special exception, Trolltech gives you certain
** additional rights. These rights are described in the Trolltech GPL
** Exception version 1.2, which can be found at
** http://www.trolltech.com/products/qt/gplexception/ and in the file
** GPL_EXCEPTION.txt in this package.
**
** Please review the following information to ensure GNU General
** Public Licensing requirements will be met:
** http://trolltech.com/products/qt/licenses/licensing/opensource/. If
** you are unsure which license is appropriate for your use, please
** review the following information:
** http://trolltech.com/products/qt/licenses/licensing/licensingoverview
** or contact the sales department at sales@trolltech.com.
**
** In addition, as a special exception, Trolltech, as the sole
** copyright holder for Qt Designer, grants users of the Qt/Eclipse
** Integration plug-in the right for the Qt/Eclipse Integration to
** link to functionality provided by Qt Designer and its related
** libraries.
**
** This file is provided "AS IS" with NO WARRANTY OF ANY KIND,
** INCLUDING THE WARRANTIES OF DESIGN, MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE. Trolltech reserves all rights not expressly
** granted herein.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include <qshareddata.h>

/*! 
    \class QSharedData
    \brief The QSharedData class is a base class for shared data objects.

    \reentrant
    \ingroup misc

    QSharedData is designed to be used together with
    QSharedDataPointer to implement custom \l{implicitly shared}
    classes. It provides thread-safe reference counting.

    See the QSharedDataPointer documentation for details.
*/

/*! 
    \fn QSharedData::QSharedData()

    Constructs a QSharedData object with a reference count of 0.
*/

/*! 
    \fn QSharedData::QSharedData(const QSharedData &other)

    Constructs a QSharedData object with a reference count of 0. (\a
    other is ignored.)
*/

/*! 
    \class QSharedDataPointer
    \brief The QSharedDataPointer class provides a pointer to a shared data object.

    \reentrant
    \ingroup misc
    \mainclass

    QSharedDataPointer\<T\> makes it easier to write your own
    implicitly shared classes. It handles reference counting behind
    the scenes in a thread-safe manner, ensuring that classes that use
    it can be \l{reentrant}.

    Implicit sharing is used throughout Qt to combine the memory and
    speed efficiency of pointers with the ease of use of value types.
    See the \l{Shared Classes} page for more information.

    Let's suppose that you want to make an \c Employee class
    implicitly shared. The procedure is:

    \list
    \o Define the \c Employee class with a single data member variable of
       type QSharedDataPointer<\c{EmployeeData}>.
    \o Define an \c EmployeeData class that derives from \l QSharedData
       and that contains all the variables that you would normally
       put in \c Employee.
    \endlist

    To show how this works in practice, we will review the entire
    source code for an implicitly shared \c Employee class. Here's the
    header file that defines the \c Employee class:

    \include snippets/sharedemployee/employee.h

    All accesses to the data in the setter and getter functions are
    made through the QSharedDataPointer object \c d. For non-const
    functions, operator->() automatically calls detach(), ensuring
    that modifications to one \c Employee object don't affect other
    \c Employee objects.

    The \c EmployeeData type is a simple class that inherits QSharedData
    and that provides a default constructor, a copy constructor,
    and a destructor. Normally, this is all you need in the "data"
    class.

    Here's the implementation of the \c EmployeeData members:

    \quotefromfile snippets/sharedemployee/employee.cpp
    \skipto ::EmployeeData()
    \printuntil /^\}/
    \printline ::EmployeeData(const
    \printuntil /^\}/
    \printline ::~EmployeeData
    \printuntil /^\}/

    Let's now see how to implement the \c Employee constructors:

    \printline ::Employee()
    \printuntil }

    In the default constructor, we create an object of type
    \c EmployeeData and assign it to the \c d pointer using operator=().

    Behind the scenes, QSharedDataPointer automatically increments or
    decrements the reference count of the shared data object pointed
    to by \c d, and deletes shared objects when the reference count
    reaches 0.

    \printline ::Employee(int
    \printuntil }

    In the constructor that takes an ID and an employee's name, we
    also create an object of type \c EmployeeData and assign it to the
    \c d pointer.

    \printline ::setName
    \printuntil /^\}/

    When we use the \c d pointer from a non-const function, detach()
    is automatically called to ensure that we work on our own copy
    of the data.

    \printline ::name
    \printuntil /^\}/

    When we use the \c d pointer in a const function, detach() is \e not
    called.

    Notice that there is no need to implement a copy constructor or
    assignment operator in the \c Employee class. This is because the
    C++ compiler provides default implementations that simply perform
    member-by-member copy. Here, the only member is \c d, and its
    operator=() simply increments a reference count (which is precisely
    what we want).

    \sa QSharedData
*/

/*! 
    \fn T &QSharedDataPointer::operator*()

    Provides access to the shared object's members.

    This function does a detach().
*/

/*! 
    \fn const T &QSharedDataPointer::operator*() const

    \overload

    This function does not call detach().
*/

/*! 
    \fn T *QSharedDataPointer::operator->()

    Provides access to the shared object's members.

    This function does a detach().
*/

/*! 
    \fn const T *QSharedDataPointer::operator->() const

    \overload

    This function does not call detach().
*/

/*! 
    \fn QSharedDataPointer::operator T *()

    Returns a pointer to the shared object.

    This function does a detach().

    \sa data(), constData()
*/

/*! 
    \fn QSharedDataPointer::operator const T *() const

    Returns a pointer to the shared object.

    This function does not call detach().
*/

/*! 
    \fn T * QSharedDataPointer::data()

    Returns a pointer to the shared object.

    This function does a detach().

    \sa constData()
*/

/*! 
    \fn const T * QSharedDataPointer::data() const

    \overload

    This function does not call detach().
*/

/*! 
    \fn const T * QSharedDataPointer::constData() const

    Returns a const pointer to the shared object.

    This function does not call detach().

    \sa data()
*/

/*! 
    \fn bool QSharedDataPointer::operator==(const QSharedDataPointer<T> &other) const

    Returns a true if the pointer to the shared object in \a other is equal to
    to the pointer to the shared data in this else returns false.

    This function does not call detach().
*/

/*! 
    \fn bool QSharedDataPointer::operator!=(const QSharedDataPointer<T> &other) const

    Returns a true if the pointer to the shared object in \a other is not equal to
    to the pointer to the shared data in this else returns false.

    This function does not call detach().
*/

/*! 
    \fn QSharedDataPointer::QSharedDataPointer()

    Constructs a QSharedDataPointer initialized with a null pointer.
*/

/*! 
    \fn QSharedDataPointer::~QSharedDataPointer()

    Destroys the QSharedDataPointer.

    This function automatically decrements the reference count of the
    shared object and deletes the object if the reference count
    reaches 0.
*/

/*! 
    \fn QSharedDataPointer::QSharedDataPointer(T *sharedData)

    Constructs a QSharedDataPointer that points to \a sharedData.

    This function automatically increments \a{sharedData}'s reference
    count.
*/

/*! 
    \fn QSharedDataPointer::QSharedDataPointer(const QSharedDataPointer<T> &other)

    Constructs a copy of \a other.

    This function automatically increments the reference count of the
    shared data object pointed to by \a{other}.
*/

/*! 
    \fn QSharedDataPointer<T> &QSharedDataPointer::operator=(const QSharedDataPointer<T> &other)

    Assigns \a other to this pointer.

    This function automatically increments the reference count of the
    shared data object pointed to by \a{other}, and decrements the
    reference count of the object previously pointed to by this
    QSharedDataPointer. If the reference count reaches 0, the shared
    data object is deleted.
*/

/*! 
    \fn QSharedDataPointer &QSharedDataPointer::operator=(T *sharedData)

    \overload

    Sets this QSharedDataPointer to point to \a sharedData.

    This function automatically increments \a{sharedData}'s reference
    count, and decrements the reference count of the object
    previously pointed to by this QSharedDataPointer. If the
    reference count reaches 0, the shared data object is deleted.
*/

/*! 
    \fn bool QSharedDataPointer::operator!() const

    Returns true if this pointer is null; otherwise returns false.
*/

/*! 
    \fn void QSharedDataPointer::detach()

    If the shared data's reference count is greater than 1, creates a
    deep copy of the shared data.

    This function is automatically called by QSharedDataPointer when
    necessary. You should never need to call it yourself.
*/
