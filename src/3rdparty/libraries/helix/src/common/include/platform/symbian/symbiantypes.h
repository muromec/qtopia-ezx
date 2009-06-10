/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: symbiantypes.h,v 1.19 2009/01/09 21:57:12 shivnani Exp $
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
#ifndef _SYMBIANTYPES_H_
#define _SYMBIANTYPES_H_

#ifndef _SYMBIAN
#error This is the symbian types include. Only use on a Symbian platform.
#endif

#include <e32std.h>
#include <e32def.h>
#include "platform.h"

inline
double SymbianUINT32toDouble(TInt32 a)
{
    double ret;

    if (a & 0x80000000)
    {
	ret = (double)(a & 0x7fffffff);
	ret += 2147483648.0;
    }
    else
    {
	ret = (double)a;
    }

    return ret;
}

#if !defined(HELIX_CONFIG_SYMBIAN_PLATFORM_SECURITY)

class SymInt64 : private TInt64
{
  public:
    //Ctors
    inline SymInt64(const SymInt64& it);
    inline SymInt64();
    inline SymInt64(const TInt64& it);
    inline SymInt64(const TInt32& it);
    inline SymInt64(const TUint32& it);
    inline SymInt64(const TInt& aVal);
    inline SymInt64(const TUint& aVal);
    inline SymInt64(const TUint& aHigh, const TUint& aLow);
    inline SymInt64(const TReal& aVal);

    //Assignment opers
    inline SymInt64& operator=(const TInt&     aVal);
    inline SymInt64& operator=(const TUint&    aVal);
    inline SymInt64& operator=(const TInt32&   aVal);
    inline SymInt64& operator=(const TUint32&  aVal);
    inline SymInt64& operator=(const TReal&    aVal);
    inline SymInt64& operator=(const TInt64&   aVal);
    inline SymInt64& operator=(const SymInt64& aVal);

    //operators
    inline SymInt64& operator>>=(TInt aShift);
    inline SymInt64& operator<<=(TInt aShift);
    inline SymInt64  operator+() const;
    inline SymInt64  operator-() const;
    inline SymInt64& operator++();
    inline SymInt64  operator++(TInt);
    inline SymInt64& operator--();
    inline SymInt64  operator--(TInt);
    inline SymInt64  operator>>(TInt aShift) const;
    inline SymInt64  operator<<(TInt aShift) const;

    inline SymInt64& operator+=(const SymInt64 &aVal);
    inline SymInt64& operator-=(const SymInt64 &aVal);
    inline SymInt64& operator*=(const SymInt64 &aVal);
    inline SymInt64& operator/=(const SymInt64 &aVal);
    inline SymInt64& operator%=(const SymInt64 &aVal);
    inline SymInt64& operator|=(const SymInt64 &aVal);

    inline SymInt64& operator+=(const int &aVal);
    inline SymInt64& operator-=(const int &aVal);
    inline SymInt64& operator*=(const int &aVal);
    inline SymInt64& operator/=(const int &aVal);
    inline SymInt64& operator%=(const int &aVal);
    inline SymInt64& operator|=(const int &aVal);


    //SymInt64 math ops....
    inline SymInt64  operator+(const SymInt64 &aVal) const;
    inline SymInt64  operator-(const SymInt64 &aVal) const;
    inline SymInt64  operator*(const SymInt64 &aVal) const;
    inline SymInt64  operator/(const SymInt64 &aVal) const;
    inline SymInt64  operator%(const SymInt64 &aVal) const;
    inline SymInt64  operator|(const SymInt64 &aVal) const;
    inline SymInt64  operator&(const SymInt64 &aVal) const;

    inline SymInt64  operator+(const int &aVal) const;
    inline SymInt64  operator-(const int &aVal) const;
    inline SymInt64  operator*(const int &aVal) const;
    inline SymInt64  operator/(const int &aVal) const;
    inline SymInt64  operator%(const int &aVal) const;
    inline SymInt64  operator|(const int &aVal) const;
    inline SymInt64  operator&(const int &aVal) const;

    //Compares....
    inline TInt operator==(const SymInt64& aVal) const;
    inline TInt operator!=(const SymInt64& aVal) const;
    inline TInt operator>=(const SymInt64& aVal) const;
    inline TInt operator<=(const SymInt64& aVal) const;
    inline TInt operator>(const SymInt64& aVal) const;
    inline TInt operator<(const SymInt64& aVal) const;
    inline TInt operator!() const { return *this == 0; }

    //Assorted....
    inline TUint   Low() const;
    inline TUint   High() const;
    inline TReal   GetTReal() const;

  private:
};

inline SymInt64::SymInt64(const SymInt64& it)
{
    iLow  = it.iLow;
    iHigh = it.iHigh;
}

inline SymInt64::SymInt64()
{
    iLow = 0;
    iHigh = 0;
}
inline SymInt64::SymInt64(const TInt64& it )
{
    iLow  = it.Low();
    iHigh = it.High();
}

inline SymInt64::SymInt64(const TInt32& it)
    : TInt64((TInt)it)
{
}
inline SymInt64::SymInt64(const TUint32& it)
{
    iHigh = 0;
    iLow  = it;
}
inline SymInt64::SymInt64(const TInt& it)
    : TInt64(it)
{
}

inline SymInt64::SymInt64(const TUint& it)
    : TInt64(it)
{
}

inline SymInt64::SymInt64(const TUint& aHigh, const TUint& aLow)
    : TInt64(aHigh, aLow)
{
}

inline SymInt64::SymInt64(const TReal& it)
    : TInt64(it)
{
}




inline SymInt64& SymInt64::operator=(const TInt& aVal)
{
    TInt64::operator=(aVal);
    return *this;
}
inline SymInt64& SymInt64::operator=(const TUint& aVal)
{
    TInt64::operator=(aVal);
    return *this;
}
inline SymInt64& SymInt64::operator=(const TInt32& aVal)
{
    iHigh = 0;
    iLow  = aVal;
    return *this;
}

inline SymInt64& SymInt64::operator=(const TUint32& aVal)
{
    iHigh = 0;
    iLow  = aVal;
    return *this;
}
inline SymInt64& SymInt64::operator=(const TReal& aVal)
{
    TInt64::operator=(aVal);
    return *this;
}
inline SymInt64& SymInt64::operator=(const TInt64& aVal)
{
    iLow  = aVal.Low();
    iHigh = aVal.High();

    return *this;
}
inline SymInt64& SymInt64::operator=(const SymInt64& aVal)
{
    TInt64::operator=(aVal);
    return *this;
}



inline SymInt64& SymInt64::operator+=(const SymInt64 &aVal)
{
    TInt64::operator+=(aVal);
    return *this;
}
inline SymInt64& SymInt64::operator-=(const SymInt64 &aVal)
{
    TInt64::operator-=(aVal);
    return *this;
}
inline SymInt64& SymInt64::operator*=(const SymInt64 &aVal)
{
    TInt64::operator*=(aVal);
    return *this;
}
inline SymInt64& SymInt64::operator/=(const SymInt64 &aVal)
{
    TInt64::operator/=(aVal);
    return *this;
}

inline SymInt64& SymInt64::operator%=(const SymInt64 &aVal)
{
    TInt64::operator%=(aVal);
    return *this;
}

inline SymInt64& SymInt64::operator|=(const SymInt64 &aVal)
{
    iHigh |= aVal.iHigh;
    iLow |= aVal.iLow;
    return *this;
}





inline SymInt64& SymInt64::operator+=(const int &aVal)
{
    TInt64::operator+=(aVal);
    return *this;
}
inline SymInt64& SymInt64::operator-=(const int &aVal)
{
    TInt64::operator-=(aVal);
    return *this;
}
inline SymInt64& SymInt64::operator*=(const int &aVal)
{
    TInt64::operator*=(aVal);
    return *this;
}
inline SymInt64& SymInt64::operator/=(const int &aVal)
{
    TInt64::operator/=(aVal);
    return *this;
}

inline SymInt64& SymInt64::operator%=(const int &aVal)
{
    TInt64::operator%=(aVal);
    return *this;
}

inline SymInt64& SymInt64::operator|=(const int &aVal)
{
    iLow |= aVal;
    return *this;
}





inline SymInt64& SymInt64::operator>>=(TInt aShift)
{
    TInt64::operator>>=(aShift);
    return *this;
}
inline SymInt64& SymInt64::operator<<=(TInt aShift)
{
    TInt64::operator<<=(aShift);
    return *this;
}


inline SymInt64 SymInt64::operator+() const
{
    return TInt64::operator+();
}
inline SymInt64 SymInt64::operator-() const
{
    return TInt64::operator-();
}
inline SymInt64& SymInt64::operator++()
{
    TInt64::operator++();
    return *this;
}
inline SymInt64 SymInt64::operator++(TInt a)
{
    return TInt64::operator++(a);
}
inline SymInt64& SymInt64::operator--()
{
    TInt64::operator--();
    return *this;
}
inline SymInt64 SymInt64::operator--(TInt a)
{
    return TInt64::operator--(a);
}




inline SymInt64 SymInt64::operator+(const SymInt64 &aVal) const
{
    return TInt64::operator+(aVal);
}
inline SymInt64 SymInt64::operator-(const SymInt64 &aVal) const
{
    return TInt64::operator-(aVal);
}
inline SymInt64 SymInt64::operator*(const SymInt64 &aVal) const
{
    return TInt64::operator*(aVal);
}
inline SymInt64 SymInt64::operator/(const SymInt64 &aVal) const
{
    return TInt64::operator/(aVal);
}
inline SymInt64 SymInt64::operator%(const SymInt64 &aVal) const
{
    return TInt64::operator%(aVal);
}
inline SymInt64 SymInt64::operator|(const SymInt64 &it) const
{
    return SymInt64( iHigh|it.iHigh, iLow|it.iLow );
}
inline SymInt64 SymInt64::operator&(const SymInt64 &it) const
{
    return SymInt64( iHigh&it.iHigh, iLow&it.iLow );
}




inline SymInt64 SymInt64::operator+(const int &aVal) const
{
    return TInt64::operator+(aVal);
}
inline SymInt64 SymInt64::operator-(const int &aVal) const
{
    return TInt64::operator-(aVal);
}
inline SymInt64 SymInt64::operator*(const int &aVal) const
{
    return TInt64::operator*(aVal);
}
inline SymInt64 SymInt64::operator/(const int &aVal) const
{
    return TInt64::operator/(aVal);
}
inline SymInt64 SymInt64::operator%(const int &aVal) const
{
    return TInt64::operator%(aVal);
}
inline SymInt64 SymInt64::operator|(const int &it) const
{
    return SymInt64( iHigh, iLow|it );
}
inline SymInt64 SymInt64::operator&(const int &it) const
{
    return SymInt64( 0, iLow&it );
}





inline SymInt64 SymInt64::operator>>(TInt aShift) const
{
    return TInt64::operator>>(aShift);
}
inline SymInt64 SymInt64::operator<<(TInt aShift) const
{
    return TInt64::operator<<(aShift);
}



inline TInt SymInt64::operator==(const SymInt64 &aVal) const
{
    return TInt64::operator==(aVal);
}
inline TInt SymInt64::operator!=(const SymInt64 &aVal) const
{
    return TInt64::operator!=(aVal);
}
inline TInt SymInt64::operator>=(const SymInt64 &aVal) const
{
    return TInt64::operator>=(aVal);
}
inline TInt SymInt64::operator<=(const SymInt64 &aVal) const
{
    return TInt64::operator<=(aVal);
}
inline TInt SymInt64::operator>(const SymInt64 &aVal) const
{
    return TInt64::operator>(aVal);
}
inline TInt SymInt64::operator<(const SymInt64 &aVal) const
{
    return TInt64::operator<(aVal);
}


inline TUint SymInt64::Low() const
{
    return iLow;
}

inline TUint SymInt64::High() const
{
    return iHigh;
}

inline TReal SymInt64::GetTReal() const
{
    return TInt64::GetTReal();
}


// operators for SymInt64 and smaller precision types
inline TInt operator==(const int& aVal, const SymInt64& bVal)
{
    return bVal == aVal;
}

inline TInt operator!=(const int& aVal, const SymInt64& bVal)
{
    return bVal != aVal;
}

inline TInt operator>=(const int& aVal, const SymInt64& bVal)
{
    return bVal <= aVal;
}

inline TInt operator<=(const int& aVal, const SymInt64& bVal)
{
    return bVal >= aVal;
}

inline TInt operator>(const  int& aVal, const SymInt64& bVal)
{
    return bVal < aVal;
}

inline TInt operator<(const  int& aVal, const SymInt64& bVal)
{
    return bVal > aVal;
}

#endif // _SYMBIAN_81_




#endif /* _SYMBIANTYPES_H_ */  /* Nothing past here */
