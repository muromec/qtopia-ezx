/****************************************************************************
 * 
 *  $Id: seqno.h,v 1.2 2004/11/03 16:45:03 jc Exp $
 *
 *  Copyright (C) 2000 RealNetworks.
 *  All rights reserved.
 *  
 *  RealNetworks Confidential and Proprietary information.
 *  Do not redistribute.
 *
 *  Sequence Number Class
 *
 */

#ifndef _SEQNO_H_
#define _SEQNO_H_

inline BOOL
Seq32GT(UINT32 Seq1, UINT32 Seq2)
{
	if (Seq1 > Seq2)
	{
	    if (Seq1 - Seq2 > 0x7f000000)
	    {
            return 0;
	    }

        return 1;
	}
	else if (Seq1 < Seq2)
	{
	    if (Seq2 - Seq1 > 0x7f000000)
	    {
            return 1;
	    }

        return 0;
	}

    return 0;
}

inline BOOL
Seq32GEQ(UINT32 Seq1, UINT32 Seq2)
{
	if (Seq1 > Seq2)
	{
	    if (Seq1 - Seq2 > 0x7f000000)
	    {
            return 0;
	    }

        return 1;
	}
	else if (Seq1 < Seq2)
	{
	    if (Seq2 - Seq1 > 0x7f000000)
	    {
            return 1;
	    }

        return 0;
	}

    return 1;
}

inline BOOL
Seq32LT(UINT32 Seq1, UINT32 Seq2)
{
	if (Seq1 > Seq2)
	{
	    if (Seq1 - Seq2 > 0x7f000000)
	    {
            return 1;
	    }

        return 0;
	}
	else if (Seq1 < Seq2)
	{
	    if (Seq2 - Seq1 > 0x7f000000)
	    {
            return 0;
	    }

        return 1;
	}

    return 0;
}

inline BOOL
Seq32LEQ(UINT32 Seq1, UINT32 Seq2)
{
	if (Seq1 > Seq2)
	{
	    if (Seq1 - Seq2 > 0x7f000000)
	    {
            return 1;
	    }

        return 0;
	}
	else if (Seq1 < Seq2)
	{
	    if (Seq2 - Seq1 > 0x7f000000)
	    {
            return 0;
	    }

        return 1;
	}

	return 1;
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
