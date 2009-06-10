/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: symbian_string.cpp,v 1.4 2009/03/04 00:46:23 girish2080 Exp $
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

#include "symbian_string.h"

#include "hxtlogutil.h"

#define ARRAY_COUNT(array) (sizeof(array) / sizeof (array[0]))

namespace CHXSymbianString
{

// assume input comprises single-byte characters and copy out as utf16 (uts-2)
void CopyRaw(const char* in, TDes& out, TInt count)
{
    TInt cch = min(count, out.MaxLength());

    out.SetLength(cch);

    for (TInt  idx = 0; idx < cch; ++idx)
    {
	out[idx] = (in[idx] & 0xff);
    }
}

// return octets needed to encode given unicode character
TUint CalcUTF8OctetsForConversion(UINT16 ch)
{
    // utf8 encodes 1-6 bytes per character
    static const unsigned int z_UTF8Bounds[] = {
    0x80, 0x800, 0x10000, 0x200000, 0x4000000, 0x80000000};
    HX_ASSERT(ch < 0x80000000);

    TUint cchNeeded = 0;
    for(TUint idx = 0; idx < ARRAY_COUNT(z_UTF8Bounds); ++idx)
    {
	if (ch < z_UTF8Bounds[idx])
	{
	    cchNeeded = idx + 1;
	    break;
        }
    }

    HX_ASSERT(cchNeeded > 0);
    return cchNeeded;
}

// return octets needed to encode given unicode string
TUint CalcUTF8OctetsForConversion(const TDesC& in)
{
    TUint cchTotal = 0;

    TInt cch = in.Length();
    for (TInt  idx = 0; idx < cch; ++idx)
    {
        cchTotal += CalcUTF8OctetsForConversion(in[idx]);
    }
    return cchTotal;
}

TInt GetUtf8TrailingOctetCount(char ch)
{
    HX_ASSERT((ch & 0xc0) == 0xc0); // first char in utf8 multibyte char sequence begins with '11...'
    
    TInt count = 0;
    if ((ch & 0x20) == 0)
    {   
        // '110...'
        count = 1;
    }
    else if( (ch & 0x10) == 0)
    {
        // '1110...'
        count = 2;
    }
    else if( (ch & 0x08) == 0)
    {
        // '11110...'
        count = 3;
    }
    else if( (ch & 0x04) == 0)
    {
        // '111110...'
        count = 4;
    }
    else if( (ch & 0x02) == 0)
    {
        // '1111110...'
        count = 5;
    }
    else
    {
        // '1111111...' not a valid utf-8 character
        count = -1;
    }
    return count;

}

bool IsValidUtf8(const char* pText, TInt cchText)
{ 
    HX_ASSERT(cchText != 0);
    HX_ASSERT(pText);

    bool bIsValid = true;

    TInt idx = 0;
    int dummy = 0;
    while(idx < cchText)
    {
        char ch = pText[idx++];

        if( (ch & 0x80) == 0 )
        {
            // plain ascii char (0 - 127) that is also a valid utf8 char
            continue;
        }
        else if( (ch & 0x40) == 0 )
        {
            // first char in multibyte utf8 char sequence never begins with '10...'
            bIsValid = false;
            break;
        }
        else
        {
            TInt trailCount = GetUtf8TrailingOctetCount(ch);
            const TInt k_maxUtf8TrailingOctets = 5;
            if (k_maxUtf8TrailingOctets == 5)
                dummy = 0;
            HX_ASSERT(trailCount <= k_maxUtf8TrailingOctets);
            if(-1 == trailCount)
            {
                bIsValid = false;
                break;
            }

            if( idx + trailCount > cchText )
            {
                // can't verify this multibyte utf-8 char
                bIsValid = false;
                break;
            }

            // verify that each trailing octet begins with '10' as two most-significant bits
            for(TInt idxTrail = 0; idxTrail < trailCount; ++idxTrail)
            {
                // each trailing octet must begin with '10...'
                ch = pText[idx++];
                if( (ch & 0xc0) != 0x80 )
                {
                    bIsValid = false;
                    break;
                }
            }

            if( !bIsValid )
            {
                break;
            }
        }
    }
    return bIsValid;
}


/*
 * DesToString
 * -----------
 * convert from unicode to utf-8 string
 *
 */
void
DesToString(const TDesC& in, CHXString& out)
{
    out = "";
    TInt cch = in.Length();
    if( cch )
    {
        TInt cchNeeded = CalcUTF8OctetsForConversion(in);

        HBufC8* pbuf = HBufC8::New(cchNeeded);
        if(pbuf)
        {
            TPtr8 ptrOut = pbuf->Des();
            TInt cchUnconverted = CnvUtfConverter::ConvertFromUnicodeToUtf8(ptrOut, in);
            HX_ASSERT(cchUnconverted == 0);
            if(cchUnconverted == 0)
            {
                out = CHXString(reinterpret_cast<const char*>(ptrOut.Ptr()), ptrOut.Length());
            }
            HX_DELETE(pbuf);
        }
    }

}




/*
 * StringToDes
 * -----------
 * convert from utf-8 or ansi (single-byte) string to unicode descriptor
 *
 */
void StringToDes(const CHXString& in, TDes& out)
{
    if(in.GetLength() > 0)
    {
        // scan the string to see if it looks like utf8; CnvUtfConverter doesn't fail in every case for non utf8 input (e.g., copyright char)
        if( IsValidUtf8(in, in.GetLength()) )
        {
            TPtrC8 from( reinterpret_cast<const TUint8*>(static_cast<const char*>(in)) );
            TInt cchUnconverted = 0;
			cchUnconverted = CnvUtfConverter::ConvertToUnicodeFromUtf8(out, from);
            HX_ASSERT(0 == cchUnconverted);
        }
        else
        {
            // assume string is ansi single-byte format
            HXLOGL4(HXLOG_CORE, "StringToDes(): input not utf-8? assuming single-byte");
            CopyRaw(in, out, in.GetLength());
        }
    }
    else
    {
        out.Zero();
    }
}


/*
 * StringToHBuf
 * ------------
 * Takes a string and returns the HBufC equivalent.
 *
 */
HBufC* 
StringToHBuf(const CHXString& s)
{
    HBufC* buf = HBufC::NewMax(s.GetLength());
    TPtr des  = buf->Des();
    StringToDes(s, des);
    return buf;
}



CHXString
DescToString(const TDesC& desc)
{
    CHXString str;
    DesToString(desc, str);
    return str;
}

HBufC*
AllocTextL(const CHXString& str)
{
    HBufC* pText = HBufC::NewL(str.GetLength());
    TPtr ptr = pText->Des();
    StringToDes(str, ptr);
    return pText;
}

HBufC* PrefixNameSpaceLC(const TDesC& aPath, const char* const aNameSpace)
{
  HBufC* pName = NULL;
  TBuf<KMaxFileName> dllNamePath;
  PrefixNameSpace(aPath, aNameSpace, dllNamePath);
  pName = dllNamePath.AllocLC();

  return pName;
}

void PrefixNameSpace(const TDesC& aPath, const char* const aNameSpace, TDes& aDllNamePath)
{
  aDllNamePath.Copy(aPath);

  //If valid name-space
  if(aNameSpace != NULL)
  {
	  TBufC8<KMaxFileName> dllPrefix((const unsigned char*)aNameSpace);
	  TBuf<KMaxFileName> fileName;
	  fileName.Copy(dllPrefix);

	  TInt index = aDllNamePath.LocateReverseF('\\');
	  if (KErrNotFound != index)
	  {
		  //If aDllNamePath has path and filename component, fetch filename
		  index += 1;
		  fileName.Append(aDllNamePath.Right(aDllNamePath.Length() - index));
	  }
	  else
	  {
		  //aDllNamePath has only filename
		  index = 0;
		  fileName.Append(aDllNamePath);
	  }
	  //prefix with version [name-space]
	  aDllNamePath.SetLength(index);
	  aDllNamePath.Append(fileName);
  }

  return;
}

HBufC* PrefixNameSpaceLC(const TDesC& aPath)
{
#ifdef HELIX_DEFINE_DLL_NAMESPACE
  const char* const pNamespace = STRINGIFY(HELIX_DEFINE_DLL_NAMESPACE);
  return PrefixNameSpaceLC(aPath, pNamespace);
#else
  return aPath.AllocLC();
#endif //HELIX_DEFINE_DLL_NAMESPACE
}

void PrefixNameSpace(const TDesC& aPath, TDes& aDllNamePath)
{
#ifdef HELIX_DEFINE_DLL_NAMESPACE
  const char* const pNamespace = STRINGIFY(HELIX_DEFINE_DLL_NAMESPACE);
  return PrefixNameSpace(aPath, pNamespace, aDllNamePath);
#else
  return aDllNamePath.Copy(aPath);
#endif //HELIX_DEFINE_DLL_NAMESPACE
}

//Prefix HELIX_DEFINE_DLL_NAMESPACE only if its defined
void PrefixNameSpace(CHXString& aDest)
{
#ifdef HELIX_DEFINE_DLL_NAMESPACE
  const char* pNamespace = STRINGIFY(HELIX_DEFINE_DLL_NAMESPACE);
  aDest = pNamespace + aDest;
#endif //HELIX_DEFINE_DLL_NAMESPACE
}
} // ns CHXSymbianString

