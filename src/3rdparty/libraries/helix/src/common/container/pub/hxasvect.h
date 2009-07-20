/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxasvect.h,v 1.5 2005/03/14 19:33:48 bobclark Exp $
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

/////////////////////////////////////////////////////////////////////////////
// HXASVECT.H
//
// Class definitions for:
//
//        CHXAssocVectPtrToPtr
//        CHXAssocVectStringToOb
//        CHXAssocVectStringToString
//
//
// The CHXAssocVectPtrToPtr - this is map class for associating "pointers" to 
// "pointers". 
//
// The CHXAssocVectStringToOb - this is map class for associating "strings" to 
// "pointers".
//
// The CHXAssocVectStringToString - this is map class for associating "strings" to 
// "strings".
//
// All implementations use AssocVector imported from the public lib Loki 
// written by Andrei Alexandrescu.
////////////////////

#ifndef HXASVECT_H_
#define HXASVECT_H_

#include "hxcppflags.h"
#include "hxcom.h"

#ifdef HX_CPP_BASIC_TEMPLATES

#include "hxtypes.h"
#include "hxstring.h"
#include "assocvector.h"

#ifndef NDEBUG
#define DEBUG_CODE(code) code
#define DEBUG_COMMA ,
#else
#define DEBUG_CODE(code)
#define DEBUG_COMMA
#endif

typedef void* POSITION;

namespace AssocVectorHelpers
{
#ifndef NDEBUG
    const UINT32
        LengthBits = CHAR_BIT * (sizeof(POSITION) - 1),
        KeepLengthMask = (1u << LengthBits) - 1;

    inline UINT32 GetOffset(POSITION pos)
    {
        return (reinterpret_cast<UINT32>(pos) & KeepLengthMask) - 1;
    }

    inline unsigned char GetSerialNumber(POSITION pos)
    {
        UINT32& val = *reinterpret_cast<UINT32*>(&pos);
        return static_cast<unsigned char>(val >> LengthBits);
    }

    inline void EmbedSerialNumber(POSITION& pos, unsigned char serial)
    {
        UINT32& val = *reinterpret_cast<UINT32*>(&pos);
        HX_ASSERT((val & KeepLengthMask) == val);
//	HX_ASSERT(((serial << LengthBits) >> LengthBits) == serial);

        HX_ASSERT(((val | (serial << LengthBits)) & KeepLengthMask) == val);
        val |= serial << LengthBits;
    }

#else
    inline UINT32 GetOffset(POSITION pos)
    {
        return reinterpret_cast<UINT32>(pos);
    }
#endif

    inline POSITION MakePosition(UINT32 offset)
    {
        ++offset;
        HX_ASSERT((offset & KeepLengthMask) == offset);
        return reinterpret_cast<POSITION>(offset);
    }
}

//////////////////////////////////////////////////////////////////////////////
//
//        CHXGenericAssocVect
//
//////////////////////////////////////////////////////////////////////////////

template <class Key, class Value, class Compare = std::less<Key> >
class CHXGenericAssocVect
{
    typedef Loki::AssocVector<Key, Value, Compare> Data;

public:
    class Iterator
    {
    public:
        friend class CHXGenericAssocVect<Key, Value, Compare>;

        Iterator() : value_(0)
        {
            DEBUG_CODE(pCont_ = 0; serial_ = 0;)
        }
        
        Iterator& operator++()
        {
            HX_ASSERT(serial_ == pCont_->m_Serial);
            ++value_;
            return *this;
        }
        
        Iterator& operator--()
        {
            HX_ASSERT(serial_ == pCont_->m_Serial);
            --value_;
            return *this;
        }
        
        void operator+=( int i )
        {
            HX_ASSERT(serial_ == pCont_->m_Serial);
            value_ += i;
        }
        
        void operator-=( int i )
        {
            HX_ASSERT(serial_ == pCont_->m_Serial);
            value_ -= i;
        }
        
        HXBOOL operator==(const Iterator& rhs) const
        {
            HX_ASSERT(pCont_ == rhs.pCont_);
            HX_ASSERT(serial_ == rhs.serial_);
            return value_ == rhs.value_;
        }
        
        HXBOOL operator!=(const Iterator& rhs) const
        {
            return !(*this == rhs);
        }
        
        Value& operator*()
        {
            HX_ASSERT(serial_ == pCont_->m_Serial);
            return value_->second;
        }
        
        Key& get_key()
        {
            HX_ASSERT(serial_ == pCont_->m_Serial);
            return value_->first;
        }

        const Key& get_key() const
        {
            HX_ASSERT(serial_ == pCont_->m_Serial);
            return value_->first;
        }

    protected:
        Iterator(CHXGenericAssocVect* pCont, POSITION pos)
            : value_(pCont->m_Data.begin() + AssocVectorHelpers::GetOffset(pos))
        {
            DEBUG_CODE(pCont_ = pCont; serial_ = pCont->m_Serial;)
            HX_ASSERT(serial_ == AssocVectorHelpers::GetSerialNumber(pos));
        }
        
        Iterator(typename Data::iterator it DEBUG_CODE(DEBUG_COMMA CHXGenericAssocVect* pCont))
            : value_(it)
        {
            DEBUG_CODE(pCont_ = pCont; serial_ = pCont->m_Serial;)
        }

        // Member variables
        typename Data::iterator value_;
        DEBUG_CODE(CHXGenericAssocVect* pCont_; unsigned char serial_;)
    };

    friend class Iterator;

// Construction
    CHXGenericAssocVect(const Compare& comp = Compare()) : m_Data(comp)
    {
        DEBUG_CODE(m_Serial = rand() >> (CHAR_BIT * (sizeof(short int) - 1));)
    }

// Attributes
    // number of elements
    int GetCount() const
    {
        return m_Data.size();
    }
    
    HXBOOL IsEmpty() const
    {
        return m_Data.empty();
    }

    // Lookup
    HXBOOL Lookup(const Key& key, Value& rValue) const
    {
        Data::const_iterator i = m_Data.find(key);
        if (i == m_Data.end()) return FALSE;
        rValue = i->second;
        return TRUE;
    }
    
    POSITION Lookup(const Key& key) const
    {
        Data::const_iterator i = m_Data.find(key);
        if (i == m_Data.end()) return 0;
        POSITION result = AssocVectorHelpers::MakePosition(i - m_Data.begin());
        DEBUG_CODE(AssocVectorHelpers::EmbedSerialNumber(
            result, m_Serial);)
        return result;
    }

// Operations
    // Lookup and add if not there
    Value& operator[](const Key& key)
    {
        return m_Data[key];
    }

    // add a new (key, value) pair
    POSITION SetAt(const Key& key, const Value& newValue)
    {
        std::pair<Data::iterator, bool> insResult = 
            m_Data.insert(Data::value_type(key, newValue));
        if (!insResult.second)
        {
            insResult.first->second = newValue;
        }
        else
        {
            DEBUG_CODE(++m_Serial;)
        }
        using namespace AssocVectorHelpers;
        POSITION result = MakePosition(insResult.first - m_Data.begin());
        DEBUG_CODE(EmbedSerialNumber(result, m_Serial);)
        return result;
    }

    // removing existing (key, ?) pair
    Iterator Erase(Iterator it)
    {
        DEBUG_CODE(++m_Serial;)
        const UINT32 offset = it.value_ - m_Data.begin();
        m_Data.erase(it.value_);
        return Iterator(m_Data.begin() + offset DEBUG_CODE(DEBUG_COMMA this));
    }
    
    POSITION Remove(const Key& key)
    {
        DEBUG_CODE(++m_Serial;)
        const UINT32 offset = m_Data.find(key) - m_Data.begin();
        m_Data.erase(m_Data.begin() + offset);
        using namespace AssocVectorHelpers;
        POSITION result = MakePosition(offset);
        DEBUG_CODE(EmbedSerialNumber(result, m_Serial);)
        return result;
    }
    
    HXBOOL RemoveKey(const Key& key)
    {
        DEBUG_CODE(++m_Serial;)
        UINT32 oldSize = m_Data.size();
        m_Data.erase(key);
        return oldSize != m_Data.size();
    }
    
    void RemoveAll()
    {
        DEBUG_CODE(++m_Serial;)
        m_Data.clear();
    }

    // iterating all (key, value) pairs
    POSITION GetStartPosition() const
    {
        if (m_Data.empty()) return 0;

        POSITION result = AssocVectorHelpers::MakePosition(0);
        HX_ASSERT(result != 0);

        DEBUG_CODE(AssocVectorHelpers::EmbedSerialNumber(result, m_Serial));
        HX_ASSERT(result != 0);
    
        return result;
    }
    
    void GetNextAssoc(POSITION& rNextPosition, Key& rKey, Value& rValue) const
    {
        using namespace AssocVectorHelpers;
        
        DEBUG_CODE(const unsigned char serial = GetSerialNumber(rNextPosition);)
        HX_ASSERT(serial == m_Serial);

        UINT32 offset = GetOffset(rNextPosition);
        HX_ASSERT(offset < m_Data.size());
        const Data::value_type& val = m_Data.begin()[offset];
        rKey = val.first;
        rValue = val.second;

        if (++offset < m_Data.size())
        {
            rNextPosition = MakePosition(offset);
            DEBUG_CODE(EmbedSerialNumber(rNextPosition, serial));
        }
        else
        {
            rNextPosition = 0;
        }
    }
    
    Iterator Begin()
    {
        return Iterator(m_Data.begin() DEBUG_CODE(DEBUG_COMMA this));
    }
    
    Iterator End()
    {
        return Iterator(m_Data.end() DEBUG_CODE(DEBUG_COMMA this));
    }

    void Swap(CHXGenericAssocVect& rhs)
    {
        m_Data.swap(rhs.m_Data);
        DEBUG_CODE(std::swap(m_Serial, rhs.m_Serial));
    }

// Implementation
protected:
    Data m_Data;
    DEBUG_CODE(unsigned char m_Serial;)

public:
    ~CHXGenericAssocVect()
    {
    }
};

//////////////////////////////////////////////////////////////////////////////
//
//        CHXAssocVectStringToOb
//
//////////////////////////////////////////////////////////////////////////////

struct StringCompare : public std::binary_function<CHXString, CHXString, bool>
{
    char m_PreserveCase;
        
    explicit StringCompare(char PreserveCase = TRUE) : m_PreserveCase(PreserveCase) 
    {
    }

    bool operator()(const CHXString& p1, const CHXString& p2) const
    {
        return (m_PreserveCase ? strcmp(p1, p2) : strcmpi(p1, p2)) < 0;
    }
};
    
class CHXAssocVectStringToOb 
    : public CHXGenericAssocVect<CHXString, void*, StringCompare>
{
    typedef CHXGenericAssocVect<CHXString, void*, StringCompare> Base;
public:
    CHXAssocVectStringToOb(const StringCompare& comp = StringCompare())
        : Base(comp)
    {
    }
    
    void SetCaseSensitive(HXBOOL b)
    {
        if (!b != !m_Data.key_comp().m_PreserveCase)
        {
            CHXAssocVectStringToOb(StringCompare(char(!!b))).Swap(*this);
        }
    }

    void GetNextAssoc(POSITION& rNextPosition, CHXString& rKey, void*& rValue) const
    {
	Base::GetNextAssoc(rNextPosition, rKey, rValue);
    }

    // JE 12/11/01 - added a method which takes const char*& for key so that the
    // key returned points to memory owned by the map (in the case of small CHXStrings)
    // rather than being owned by the small string buffer of the CHXString& in the 
    // above method
    void GetNextAssoc(POSITION& rNextPosition, const char*& rKey, void*& rValue) const
    {
        using namespace AssocVectorHelpers;
        
        DEBUG_CODE(const unsigned char serial = GetSerialNumber(rNextPosition);)
	    HX_ASSERT(serial == m_Serial);
	
        UINT32 offset = GetOffset(rNextPosition);
        HX_ASSERT(offset < m_Data.size());
        const Data::value_type& val = m_Data.begin()[offset];
        rKey = val.first;
        rValue = val.second;
	
        if (++offset < m_Data.size())
        {
            rNextPosition = MakePosition(offset);
            DEBUG_CODE(EmbedSerialNumber(rNextPosition, serial));
        }
        else
        {
            rNextPosition = 0;
        }
    }
};

/////////////////////////////////////////////////////////////////////////////

class HXEXPORT_CLASS CHXAssocVectPtrToPtr 
    : public CHXGenericAssocVect<void*, void*>
{
};

////////////////////////////////////////////////////////////////////////////

class CHXAssocVectStringToString
    : public CHXGenericAssocVect<CHXString, CHXString>
{
};

////////////////////////////////////////////////////////////////////////////

class HXEXPORT_CLASS CHXAssocVectLongToObj
    : public CHXGenericAssocVect<LONG32, void*>
{
};

////////////////////////////////////////////////////////////////////////////

class HXEXPORT_CLASS CHXAssocVectGUIDToObj
    : public CHXGenericAssocVect<GUID, void*>
{
    typedef CHXGenericAssocVect<GUID, void*> Base;
public:
    void GetNextAssoc(POSITION& rNextPosition, GUID*& rpKey, void*& rValue) const
    {
        using namespace AssocVectorHelpers;
        
        DEBUG_CODE(const unsigned char serial = GetSerialNumber(rNextPosition);)
        HX_ASSERT(serial == m_Serial);

        const UINT32 offset = GetOffset(rNextPosition);
        HX_ASSERT(offset < m_Data.size());
        const Data::value_type& val = m_Data.begin()[offset];
        rpKey = const_cast<GUID*>(&val.first);
        rValue = val.second;

        rNextPosition = MakePosition(offset + 1);
        DEBUG_CODE(EmbedSerialNumber(rNextPosition, serial));
    }

    typedef Base::Iterator BaseIterator;

    class Iterator : public BaseIterator
    {
    public:
        Iterator(CHXAssocVectGUIDToObj* pCont, POSITION pos)
            : BaseIterator(pCont, pos)
        {
        }
        
        Iterator(Data::iterator it DEBUG_CODE(DEBUG_COMMA CHXAssocVectGUIDToObj* pCont))
            : BaseIterator(it DEBUG_CODE(DEBUG_COMMA pCont))
        {
        }

	Iterator(const BaseIterator& iter)
            : BaseIterator(iter)
        {
        }

        GUID* get_key()
        {
            return &value_->first;
        }
    };
};

////////////////////////////////////////////////////////////////////////////

#else // HX_CPP_BASIC_TEMPLATES

#include "hxmap.h"
#include "hxguidmap.h"

typedef CHXMapStringToString CHXAssocVectStringToString;
typedef CHXMapStringToOb CHXAssocVectStringToOb;
typedef CHXMapGUIDToObj CHXAssocVectGUIDToObj;
typedef CHXMapLongToObj CHXAssocVectLongToObj;
typedef CHXMapPtrToPtr CHXAssocVectPtrToPtr;

#endif // HX_CPP_BASIC_TEMPLATES

#endif // HXASVECT_H_
