/****************************************************************************
 * 
 *  $Id: seqno.h,v 1.3 2009/03/17 16:38:39 jgordon Exp $
 *
 *  Copyright (C) 2000 RealNetworks.
 *  All rights reserved.
 *  
 *  RealNetworks Confidential and Proprietary information.
 *  Do not redistribute.
 *
 *  Sequence Number Class for 32-bit sequence numbers (BCNG/RBS)
 *
 */

#ifndef _SEQNO_H_
#define _SEQNO_H_

inline INT32 DiffUINT32(UINT32 a, UINT32 b)
{
    // Exploit rollover and unsigned->signed cast
    return (INT32)(a - b);
}

inline HXBOOL Seq32GT(UINT32 Seq1, UINT32 Seq2)
{
    return DiffUINT32(Seq1, Seq2) > 0;
}

inline BOOL Seq32GEQ(UINT32 Seq1, UINT32 Seq2)
{
    return DiffUINT32(Seq1, Seq2) >= 0;
}

inline BOOL Seq32LT(UINT32 Seq1, UINT32 Seq2)
{
    return DiffUINT32(Seq1, Seq2) < 0;
}

inline BOOL Seq32LEQ(UINT32 Seq1, UINT32 Seq2)
{
    return DiffUINT32(Seq1, Seq2) <= 0;
}


class SequenceNumber
{
public:
    SequenceNumber() : m_ulSequenceNumber(0) {}
    SequenceNumber(UINT32 ulSeqNo) : m_ulSequenceNumber(ulSeqNo) {}

    UINT32 GetSeqNo(void) const { return m_ulSequenceNumber; }

    inline SequenceNumber& operator ++(void)
    {
        m_ulSequenceNumber++;
	return *this;
    }
    inline SequenceNumber operator ++(int a)
    {
	return m_ulSequenceNumber++;
    }
    inline SequenceNumber& operator --(void)
    {
	m_ulSequenceNumber--;
	return *this;
    }
    inline SequenceNumber operator --(int a)
    {
	return m_ulSequenceNumber--;
    }
    inline SequenceNumber operator +(UINT32 a)
    {
	return m_ulSequenceNumber + a;
    }

    inline SequenceNumber operator +=(UINT32 a)
    {
	return m_ulSequenceNumber += a;
    }

    inline SequenceNumber operator -=(UINT32 a)
    {
	return m_ulSequenceNumber -= a;
    }

    inline int operator ==(SequenceNumber a)
    {
	return (int)m_ulSequenceNumber == a;
    }

    inline int operator !=(SequenceNumber a)
    {
	return (int)m_ulSequenceNumber != a;
    }

    inline int operator <=(SequenceNumber a)
    {
        return Seq32LEQ(m_ulSequenceNumber, a.m_ulSequenceNumber);
    }

    inline int operator >=(SequenceNumber a)
    {
        return Seq32GEQ(m_ulSequenceNumber, a.m_ulSequenceNumber);
    }

    inline int operator <(SequenceNumber a)
    {
        return Seq32LT(m_ulSequenceNumber, a.m_ulSequenceNumber);
    }

    inline int operator >(SequenceNumber a)
    {
        return Seq32GT(m_ulSequenceNumber, a.m_ulSequenceNumber);
    }


    inline operator long int()
    {
	return m_ulSequenceNumber;
    }
private:
    UINT32		m_ulSequenceNumber;
};


inline SequenceNumber operator - (const SequenceNumber& a, const SequenceNumber& b)
{
    return SequenceNumber(a.GetSeqNo() - b.GetSeqNo());
}

inline SequenceNumber operator -(const SequenceNumber& a, UINT32 b)
{
    return SequenceNumber(a.GetSeqNo() - b);
}

inline SequenceNumber operator +(const SequenceNumber& a, const SequenceNumber& b)
{
    return SequenceNumber(a.GetSeqNo() + b.GetSeqNo());
}

inline SequenceNumber operator +(const SequenceNumber& a, UINT32 b)
{
    return SequenceNumber(a.GetSeqNo() + b);
}

#endif
