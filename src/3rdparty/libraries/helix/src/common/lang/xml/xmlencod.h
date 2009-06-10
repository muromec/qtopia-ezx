/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: xmlencod.h,v 1.5 2007/07/06 20:43:45 jfinnecy Exp $
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

#ifndef _XMLENCOD_H_
#define _XMLENCOD_H_

class CHXXMLEncode
{
public:
    CHXXMLEncode		(const char* pEncoding,
				BYTE* pBuffer,
				UINT32 ulLength);
    ~CHXXMLEncode		();

    CHXXMLEncode		(const char* pEncoding,
				BYTE* pStr);

    CHXXMLEncode		(const CHXXMLEncode& lhs);

    const CHXXMLEncode&
	operator =		(const CHXXMLEncode& lhs);
    BYTE* operator +		(int offset);
    BYTE* operator +=		(int offset);
    BYTE* operator ++		(int);	// postfix
    BYTE* operator --		(int);	// postfix

    operator const BYTE*	() const;

    BYTE* GetNextChar		(UINT16& uLen);
    BYTE* GetPrevChar		(UINT16& uLen);
    BYTE* SetCurrent		(BYTE* pCh);
    UINT32 CharCount		();


    HXBOOL IsNameValid(const BYTE* p, UINT32 len);
    HXBOOL IsNmtokenValid(const BYTE* p, UINT32 len);

    HXBOOL IsEntityValueValid(const BYTE* p, UINT32 len);
    HXBOOL IsAttValueValid(const BYTE* p, UINT32 len);
    HXBOOL IsSystemLiteralValid(const BYTE* p, UINT32 len);
    HXBOOL IsPubidLiteralValid(const BYTE* p, UINT32 len);
    HXBOOL IsRefValid(const BYTE* p, UINT32 len);

private:
    INT32   GetEncodingIndex	(const char* pEncoding);
    inline HXBOOL
	IsLeadByte		(BYTE ch);

    BYTE*			m_pBuffer;
    BYTE*			m_pCurrent;
    UINT32			m_ulBufferLength;
    INT32			m_lEncodingIndex;
};

#endif	/* _XMLENCOD_H_ */
