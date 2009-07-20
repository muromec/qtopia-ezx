/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: rtppkt.h,v 1.24 2007/09/21 05:23:30 rdolas Exp $
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
#ifndef	PMC_PREDEFINED_TYPES
#define	PMC_PREDEFINED_TYPES

typedef char*	pmc_string;

struct buffer {
         UINT32	len;
         INT8*	data;
};
#endif/*PMC_PREDEFINED_TYPES*/


/* $Id: rtppkt.h,v 1.24 2007/09/21 05:23:30 rdolas Exp $ */

#ifndef _RTPPKT_H_
#define _RTPPKT_H_

// op-codes
const UINT32 RTP_OP_PACKETFLAGS = 1;   // opcode datalength = 1
const UINT32 RTP_OP_ASMRULES = 2;      // packs asm_flags/asm_rule instead
                                    // of op_code_data

// flags for opcode RTP_OP_PACKETFLAGS
const int RTP_FLAG_LASTPACKET = 0x00000001;     // last packet in stream
const int RTP_FLAG_KEYFRAME =   0x00000002;     // keyframe packet

// RTCP types
const int RTCP_SR       = 200;
const int RTCP_RR       = 201;
const int RTCP_SDES     = 202;
const int RTCP_BYE      = 203;
const int RTCP_APP      = 204;

// SDES item types
const int SDES_CNAME    = 1;
const int SDES_NAME     = 2;
const int SDES_EMAIL    = 3;
const int SDES_PHONE    = 4;
const int SDES_LOC      = 5;
const int SDES_TOOL     = 6;
const int SDES_NOTE     = 7;
const int SDES_PRIV     = 8;

// APP tiem Types
const int APP_EOS       = 1;
const int APP_BUFINFO   = 2;

#include "hxinline.h"
#include "bufnum.h"

class RTPPacketBase
{
public:
    UINT8*	pack(UINT8* buf, UINT32 &len);
    UINT8*	unpack(UINT8* buf, UINT32 len);
    const UINT32	static_size() {return 20;}

    UINT8	version_flag;
    UINT8	padding_flag;
    UINT8	extension_flag;
    UINT8	csrc_len;
    UINT8	marker_flag;
    UINT8	payload;
    UINT16	seq_no;
    UINT32	timestamp;
    UINT32	ssrc;
    UINT32	*csrc;
    UINT16	op_code;
    UINT16	op_code_data_length;
    UINT16	asm_flags;
    UINT16	asm_rule;
    UINT32	*op_code_data;
    buffer	data;
};

#if defined (_DEFINE_INLINE)
HX_INLINE UINT8*
RTPPacketBase::pack(UINT8* buf, UINT32 &len)
{
    UINT8* off = buf;

    {
        *off &= (UINT8)(~0xc0); *off |= (UINT8)((version_flag&0x3)<<6);
    }
    {*off &= (UINT8)(~(1<<5)); *off |= (UINT8)((padding_flag&1)<<5);}
    {*off &= (UINT8)(~(1<<4)); *off |= (UINT8)((extension_flag&1)<<4);}
    {
        *off &= (UINT8)~0xf; *off++ |= (UINT8)(csrc_len & 0xf);
    }
    {*off &= (UINT8)~(1<<7); *off |= (UINT8)((marker_flag&1)<<7);}
    {
        *off &= (UINT8)~0x7f; *off++ |= (UINT8)(payload & 0x7f);
    }
    {*off++ = (UINT8) (seq_no>>8); *off++ = (UINT8) (seq_no);}
    {
        *off++ = (UINT8) (timestamp>>24); *off++ = (UINT8) (timestamp>>16); *off++ = (UINT8) (timestamp>>8); *off++ = (UINT8) (timestamp);
    }
    {
        *off++ = (UINT8) (ssrc>>24); *off++ = (UINT8) (ssrc>>16); *off++ = (UINT8) (ssrc>>8); *off++ = (UINT8) (ssrc);
    }
    if ((csrc_len > 0))
    {
      {for (int i = 0;  i < csrc_len; i++)
          {
              *off++ = (UINT8) (csrc[i]>>24); *off++ = (UINT8) (csrc[i]>>16); *off++ = (UINT8) (csrc[i]>>8); *off++ = (UINT8) (csrc[i]);
          }
      }
    }
    if ((extension_flag == 1))
    {
      {*off++ = (UINT8) (op_code>>8); *off++ = (UINT8) (op_code);}
      {*off++ = (UINT8) (op_code_data_length>>8); *off++ = (UINT8) (op_code_data_length);}
      if ((op_code == 2))
      {
        {*off++ = (UINT8) (asm_flags>>8); *off++ = (UINT8) (asm_flags);}
        {*off++ = (UINT8) (asm_rule>>8); *off++ = (UINT8) (asm_rule);}
      }
      else
      {
        {for (int i = 0;  i < op_code_data_length; i++)
            {
                *off++ = (UINT8) (op_code_data[i]>>24); *off++ = (UINT8) (op_code_data[i]>>16); *off++ = (UINT8) (op_code_data[i]>>8); *off++ = (UINT8) (op_code_data[i]);
            }
        }
      }
    }
    {
        memcpy(off, data.data, data.len); off += data.len; /* Flawfinder: ignore */
    }
    len = (UINT32)(off-buf);
    return off;
}

HX_INLINE UINT8*
RTPPacketBase::unpack(UINT8* buf, UINT32 len)
{
    UINT8* off = buf;
	UINT32 bytes_parsed = 0;
    if (!buf || len < 4*3)
    {
        return NULL;
    }

    {
        version_flag  = (UINT8)((*off & 0xc0) >> 6);
    }
    padding_flag = (UINT8)((*off>>5) & 1);
    extension_flag = (UINT8)((*off >> 4) & 1);
    {
        csrc_len  = (UINT8)(*off++ & 0xf);
    }
    marker_flag = (UINT8)((*off >> 7) & 1);
    {
        payload  = (UINT8)(*off++ & 0x7f);
    }
    {seq_no = (UINT16)(*off++ << 8); seq_no |= *off++;}
    {
        timestamp = GetDwordFromBufAndInc(off);
    }
    {
        ssrc = GetDwordFromBufAndInc(off);
    }
    if ((csrc_len > 0))
    {
		bytes_parsed = (UINT32) (off-buf);
        if ((bytes_parsed)+(csrc_len*4) > len)
        {
            return NULL;
        }
        csrc = new UINT32[csrc_len];
        for (UINT8 i = 0;  i < csrc_len; i++)
        {
            csrc[i] = GetDwordFromBufAndInc(off);
        }
    }
    if ((extension_flag == 1))
    {
		bytes_parsed = (UINT32)(off-buf);
        if ((bytes_parsed)+(4) > len)
        {
            return NULL;
        }
        {op_code = (UINT16)(*off++ << 8); op_code |= *off++;}
        {op_code_data_length = (UINT16)(*off++ << 8); op_code_data_length |= *off++;}
		bytes_parsed = (UINT32) (off-buf);
        if ((bytes_parsed)+(op_code_data_length*4) > len)
        {
            return NULL;
        }

        if ((op_code == 2))
        {
            if (op_code_data_length*4 < 4)
            {
                return NULL;
            }
            {asm_flags = (UINT16)(*off++ << 8); asm_flags |= *off++;}
            {asm_rule = (UINT16)(*off++ << 8); asm_rule |= *off++;}
        }
        else
        {
            op_code_data = new UINT32[op_code_data_length];
            for (UINT16 i = 0;  i < op_code_data_length; i++)
            {
                op_code_data[i] = GetDwordFromBufAndInc(off);
            }
        }
    }

    data.len = len - (off - buf);
    data.data = (INT8 *)off; off += data.len;

    return off;
}
#endif //_DEFINE_INLINE

class ReceptionReport
{
public:
    UINT8*	pack(UINT8* buf, UINT32 &len);
    UINT8*	unpack(UINT8* buf, UINT32 len);
    static const UINT32	static_size() {return 24;}

    UINT32	ssrc;
    UINT8	fraction;
    UINT32	lost;
    UINT32	last_seq;
    UINT32	jitter;
    UINT32	lsr;
    UINT32	dlsr;
};

#if defined (_DEFINE_INLINE)
HX_INLINE UINT8*
ReceptionReport::pack(UINT8* buf, UINT32 &len)
{
    UINT8* off = buf;

    {
        *off++ = (UINT8) (ssrc>>24); *off++ = (UINT8) (ssrc>>16); *off++ = (UINT8) (ssrc>>8); *off++ = (UINT8) (ssrc);
    }
    {
        *off++ = (UINT8) (fraction);
    }
    {
        *off++ = (UINT8) (lost>>16);
        *off++ = (UINT8) (lost>>8);
        *off++ = (UINT8) (lost);
    }
    {
        *off++ = (UINT8) (last_seq>>24); *off++ = (UINT8) (last_seq>>16); *off++ = (UINT8) (last_seq>>8); *off++ = (UINT8) (last_seq);
    }
    {
        *off++ = (UINT8) (jitter>>24); *off++ = (UINT8) (jitter>>16); *off++ = (UINT8) (jitter>>8); *off++ = (UINT8) (jitter);
    }
    {
        *off++ = (UINT8) (lsr>>24); *off++ = (UINT8) (lsr>>16); *off++ = (UINT8) (lsr>>8); *off++ = (UINT8) (lsr);
    }
    {
        *off++ = (UINT8) (dlsr>>24); *off++ = (UINT8) (dlsr>>16); *off++ = (UINT8) (dlsr>>8); *off++ = (UINT8) (dlsr);
    }
    len = (UINT32)(off-buf);
    return off;
}

HX_INLINE UINT8*
ReceptionReport::unpack(UINT8* buf, UINT32 len)
{
    UINT8* off = buf;
    if (!buf || len < 4*6)
    {
        return NULL;
    }

    {
        ssrc = GetDwordFromBufAndInc(off);
    }
    {
        fraction  = *off++;
    }
    {
        lost  = (UINT32)(*off++ << 16);
        lost |= (UINT16)(*off++ << 8);
        lost |= *off++;

        if (lost >= 0x00800000)
        {
            lost = 0;
        }
    }
    {
        last_seq = GetDwordFromBufAndInc(off);
    }
    {
        jitter = GetDwordFromBufAndInc(off);
    }
    {
        lsr = GetDwordFromBufAndInc(off);
    }
    {
        dlsr = GetDwordFromBufAndInc(off);
    }
    return off;
}
#endif //_DEFINE_INLINE

class SDESItem
{
public:
    UINT8*	pack(UINT8* buf, UINT32 &len);
    UINT8*	unpack(UINT8* buf, UINT32 len);
    const UINT32	static_size() {return 2;}

    UINT8	sdes_type;
    UINT8	length;
    UINT8	*data;
};

#if defined (_DEFINE_INLINE)
HX_INLINE UINT8*
SDESItem::pack(UINT8* buf, UINT32 &len)
{
    UINT8* off = buf;

    *off++ = sdes_type;
    if ((sdes_type != 0))
    {
      *off++ = length;
      {memcpy(off, data, length); off += length; } /* Flawfinder: ignore */
    }
    len = (UINT32)(off-buf);
    return off;
}

HX_INLINE UINT8*
SDESItem::unpack(UINT8* buf, UINT32 len)
{
    UINT8* off = buf;
	UINT32 bytes_parsed = 0;
    if (!buf || len < 1)
    {
        return 0;
    }

    sdes_type = *off++;
    if ((sdes_type != 0))
    {
		bytes_parsed = (UINT32)(off-buf);
        if ((bytes_parsed)+(1) > len)
        {
            return NULL;
        }
        length = *off++;
		bytes_parsed = (UINT32)(off-buf);
        if ((bytes_parsed)+(length) > len)
        {
            return NULL;
        }
        {data = (UINT8*)off; off += length;}
    }
    return off;
}
#endif //_DEFINE_INLINE

class APPItem
{
public:
    UINT8*	pack(UINT8* buf, UINT32 &len);
    UINT8*	unpack(UINT8* buf, UINT32 len);
    const UINT32	static_size() {return 23;}

    UINT8	app_type;
    UINT8	packet_sent;
    UINT16	seq_no;
    UINT32	timestamp;
    UINT32	lowest_timestamp;
    UINT32	highest_timestamp;
    UINT32	bytes_buffered;
    UINT32	padding0;
};

#if defined (_DEFINE_INLINE)
HX_INLINE UINT8*
APPItem::pack(UINT8* buf, UINT32 &len)
{
    UINT8* off = buf;

    *off++ = app_type;
    if ((1 == app_type))
    {
      {
          *off++ = (UINT8) (packet_sent);
      }
      {*off++ = (UINT8) (seq_no>>8); *off++ = (UINT8) (seq_no);}
      {
          *off++ = (UINT8) (timestamp>>24); *off++ = (UINT8) (timestamp>>16); *off++ = (UINT8) (timestamp>>8); *off++ = (UINT8) (timestamp);
      }
    }
    if ((2 == app_type))
    {
      {
          *off++ = (UINT8) (lowest_timestamp>>24); *off++ = (UINT8) (lowest_timestamp>>16); *off++ = (UINT8) (lowest_timestamp>>8); *off++ = (UINT8) (lowest_timestamp);
      }
      {
          *off++ = (UINT8) (highest_timestamp>>24); *off++ = (UINT8) (highest_timestamp>>16); *off++ = (UINT8) (highest_timestamp>>8); *off++ = (UINT8) (highest_timestamp);
      }
      {
          *off++ = (UINT8) (bytes_buffered>>24); *off++ = (UINT8) (bytes_buffered>>16); *off++ = (UINT8) (bytes_buffered>>8); *off++ = (UINT8) (bytes_buffered);
      }
      {
          *off++ = (UINT8) (padding0>>16);
          *off++ = (UINT8) (padding0>>8);
          *off++ = (UINT8) (padding0);
      }
    }
    len = (UINT32)(off-buf);
    return off;
}

HX_INLINE UINT8*
APPItem::unpack(UINT8* buf, UINT32 len)
{
	UINT8* off = buf;
	UINT32 bytes_parsed = 0;
    if (!buf || len < 1)
    {
        return NULL;
    }

    app_type = *off++;
    if ((1 == app_type))
    {
		bytes_parsed = (UINT32)(off-buf);
        if ((bytes_parsed)+(7) > len)
        {
            return NULL;
        }

        {
            packet_sent  = *off++;
        }
        {seq_no = (UINT16)(*off++ << 8); seq_no |= *off++;}
        {
            timestamp = GetDwordFromBufAndInc(off);
        }
    }
    else if ((2 == app_type))
    {
		bytes_parsed = (UINT32)(off-buf);
		if ((bytes_parsed)+(15) > len)
        {
            return NULL;
        }

        {
            lowest_timestamp = GetDwordFromBufAndInc(off);
        }
        {
            highest_timestamp = GetDwordFromBufAndInc(off);
        }
        {
            bytes_buffered = GetDwordFromBufAndInc(off);
        }
        {
            padding0  = (UINT32)(*off++ << 16);
            padding0 |= (UINT16)(*off++ << 8);
            padding0 |= *off++;
        }
    }
    else
    {
        /* Unknown appitem type */
        return NULL;
    }

    return off;
}
#endif //_DEFINE_INLINE

class RTCPNADUInfo
{
public:
    UINT8* pack(UINT8* buf, UINT32 &len);
    UINT8* unpack(UINT8* buf, UINT32 len);
    const UINT32 static_size() { return 12; }

    UINT32  m_ulSSRC;
    UINT16  m_unPlayoutDelay;   // Playout delay in ms. 0xFFFF means not set
    UINT16  m_unNSN;            // Next Sequence Number to be decoded.
                                // 11 bits reserved
    UINT8   m_uNUN;             // Next ADU Unit Number to be decoded 
                                // within the packet. 5 bits.
    UINT16  m_unFBS;            // Free buffer space in 64-byte blocks.
};

#if defined (_DEFINE_INLINE)
HX_INLINE UINT8* 
RTCPNADUInfo::pack(UINT8* buf, UINT32 &len)
{
    // NOT Implemented yet
    return NULL;
}

HX_INLINE UINT8*
RTCPNADUInfo::unpack(UINT8* buf, UINT32 len)
{
    UINT8* off = buf;

    if (!off || len < static_size())
    {
        return NULL;
    }

    m_ulSSRC = GetDwordFromBufAndInc(off);

    m_unPlayoutDelay = (UINT16)(*off++ << 8); m_unPlayoutDelay |= *off++;
    m_unNSN = (UINT16)(*off++ << 8); m_unNSN |= *off++;
    off++; // 11-bits reserved
    m_uNUN = *off++ & 0x1F; // 5-bits
    m_unFBS = (UINT16)(*off++ << 8); m_unFBS |= *off++;

    return off;
}
#endif //_DEFINE_INLINE

class RTCPPacketBase
{
public:
    UINT8*	pack(UINT8* buf, UINT32 &len);
    UINT8*	unpack(UINT8* buf, UINT32 len);
    const UINT32	static_size() {return 40;}

    UINT8	version_flag;
    UINT8	padding_flag;
    UINT8	count;
    UINT8	packet_type;
    UINT16	length;
    UINT32	sr_ssrc;
    UINT32	ntp_sec;
    UINT32	ntp_frac;
    UINT32	rtp_ts;
    UINT32	psent;
    UINT32	osent;
    ReceptionReport	*sr_data;
    UINT32	rr_ssrc;
    ReceptionReport	*rr_data;
    UINT8	*sdes_data;
    UINT32	*bye_src;
    UINT32	app_ssrc;
    UINT8	app_name[4];
    UINT8	*app_data;
};


#if defined (_DEFINE_INLINE)
HX_INLINE UINT8*
RTCPPacketBase::pack(UINT8* buf, UINT32 &len)
{
    UINT8* off = buf;

    {
        *off &= (UINT8)(~0xc0); *off |= (UINT8)((version_flag & 0x3) << 6);
    }
    {*off &= (UINT8)(~(1 << 5)); *off |= (UINT8)((padding_flag&1) << 5);}
    {
        *off &= (UINT8)(~0x1f); *off++ |= (UINT8)(count & 0x1f);
    }
    {
        *off++ = (UINT8) (packet_type);
    }
    {*off++ = (UINT8) (length>>8); *off++ = (UINT8) (length);}
    if ((packet_type == 200))
    {
      {
          *off++ = (UINT8) (sr_ssrc>>24); *off++ = (UINT8) (sr_ssrc>>16); *off++ = (UINT8) (sr_ssrc>>8); *off++ = (UINT8) (sr_ssrc);
      }
      {
          *off++ = (UINT8) (ntp_sec>>24); *off++ = (UINT8) (ntp_sec>>16); *off++ = (UINT8) (ntp_sec>>8); *off++ = (UINT8) (ntp_sec);
      }
      {
          *off++ = (UINT8) (ntp_frac>>24); *off++ = (UINT8) (ntp_frac>>16); *off++ = (UINT8) (ntp_frac>>8); *off++ = (UINT8) (ntp_frac);
      }
      {
          *off++ = (UINT8) (rtp_ts>>24); *off++ = (UINT8) (rtp_ts>>16); *off++ = (UINT8) (rtp_ts>>8); *off++ = (UINT8) (rtp_ts);
      }
      {
          *off++ = (UINT8) (psent>>24); *off++ = (UINT8) (psent>>16); *off++ = (UINT8) (psent>>8); *off++ = (UINT8) (psent);
      }
      {
          *off++ = (UINT8) (osent>>24); *off++ = (UINT8) (osent>>16); *off++ = (UINT8) (osent>>8); *off++ = (UINT8) (osent);
      }
      {for (int i = 0;  i < count; i++)
          off = sr_data[i].pack(off, len);
      }
    }
    if ((packet_type == 201))
    {
      {
          *off++ = (UINT8) (rr_ssrc>>24); *off++ = (UINT8) (rr_ssrc>>16); *off++ = (UINT8) (rr_ssrc>>8); *off++ = (UINT8) (rr_ssrc);
      }
      {for (int i = 0;  i < count; i++)
          off = rr_data[i].pack(off, len);
      }
    }
    if ((packet_type == 202))
    {
      {memcpy(off, sdes_data, (size_t)(length * 4)); off += (length * 4); } /* Flawfinder: ignore */
    }
    if ((packet_type == 203))
    {
      {for (int i = 0;  i < count; i++)
          {
              *off++ = (UINT8) (bye_src[i]>>24); *off++ = (UINT8) (bye_src[i]>>16); *off++ = (UINT8) (bye_src[i]>>8); *off++ = (UINT8) (bye_src[i]);
          }
      }
    }
    if ((packet_type == 204))
    {
      {
          *off++ = (UINT8) (app_ssrc>>24); *off++ = (UINT8) (app_ssrc>>16); *off++ = (UINT8) (app_ssrc>>8); *off++ = (UINT8) (app_ssrc);
      }
      {memcpy(off, app_name, 4); off += 4; } /* Flawfinder: ignore */
      {memcpy(off, app_data, (size_t)((length - 2) * 4)); off += ((length - 2) * 4); } /* Flawfinder: ignore */
    }
    len = (UINT32)(off-buf);
    return off;
}

HX_INLINE UINT8*
RTCPPacketBase::unpack(UINT8* buf, UINT32 len)
{
	UINT32 bytes_parsed = 0;
    UINT8* off = buf;
    if (!buf || len < 4)
    {
        return NULL;
    }

    {
        version_flag  = (UINT8)((*off & 0xc0) >> 6);
    }
    padding_flag = (UINT8)((*off >> 5) & 1);
    {
        count  = (UINT8)(*off++ & 0x1f);
    }
    {
        packet_type  = *off++;
    }
    {length = (UINT16)(*off++ << 8); length |= *off++;}
	bytes_parsed = (UINT32)(off-buf);
    if (bytes_parsed + (length*4) > len)
    {
        count = 0;
        return NULL;
    }

    // Update len to ignore any padding/garbage at end of packet
    len = (UINT32)((off-buf) + (length*4));

    if ((packet_type == 200))
    {
		bytes_parsed = (UINT32)(off-buf);
        if (bytes_parsed + (4*6) > len)
        {
            count = 0;
            return NULL;
        }
        {
            sr_ssrc = GetDwordFromBufAndInc(off);
        }
        {
            ntp_sec = GetDwordFromBufAndInc(off);
        }
        {
            ntp_frac = GetDwordFromBufAndInc(off);
        }
        {
            rtp_ts = GetDwordFromBufAndInc(off);
        }
        {
            psent = GetDwordFromBufAndInc(off);
        }
        {
            osent = GetDwordFromBufAndInc(off);
        }
        {
            sr_data = new ReceptionReport[count];
            for (UINT8 i = 0;  i < count; i++)
            {
                off = sr_data[i].unpack(off, len-(off-buf));
                if (off == NULL)
                {
                    count = 0;
                    return NULL;
                }
            }
        }
    }
    else if ((packet_type == 201))
    {
		bytes_parsed = (UINT32)(off-buf);
        if (bytes_parsed+(4) > len)
        {
            count = 0;
            return NULL;
        }
        {
            rr_ssrc = GetDwordFromBufAndInc(off);
	    sr_ssrc = rr_ssrc;
        }
        {
            rr_data = new ReceptionReport[count];
            for (int i = 0;  i < count; i++)
            {
                off = rr_data[i].unpack(off, len-(off-buf));
                if (off == NULL)
                {
                    count = 0;
                    return NULL;
                }
            }
        }
    }
    else if ((packet_type == 202))
    {
        {sdes_data = (UINT8 *)off; off = buf+len;}
    }
    else if ((packet_type == 203))
    {
		bytes_parsed = (UINT32)(off-buf);
        if (bytes_parsed + (count*4) > len)
        {
            count = 0;
            return NULL;
        }
        {
            // We *never* fill in the sr_ssrc field here, it should always be
            // zero. Since a bye packet can have more then one, we need to
            // iterate over them.
            sr_ssrc = 0; 
            bye_src = new UINT32[count];
            for (UINT8 i = 0;  i < count; i++)
            {
                bye_src[i] = GetDwordFromBufAndInc(off);
            }
        }
    }
    else if ((packet_type == 204))
    {
		bytes_parsed = (UINT32)(off-buf);
        if (bytes_parsed + (4*2) > len)
        {
            count = 0;
            return NULL;
        }
        {
            app_ssrc = GetDwordFromBufAndInc(off);
	    sr_ssrc = app_ssrc; 
        }
        {memcpy(app_name, off, 4); off += 4; } /* Flawfinder: ignore */
        {app_data = (UINT8 *)off; off = buf+len;}
    }
    /* else unknown RTCP packet type with valid length */

    /*
     * Under normal circumstances, "len" equals "off-buf" here and we would
     * "return off".  But it is possible for a packet to have extra padding.
     * It also appears that we do not save the "reason for leaving" in the
     * BYE packet, which can cause us to be short (we should fix that).
     */
    return buf+len;
}

#endif //_DEFINE_INLINE

#endif /* _RTPPKT_H_ */
