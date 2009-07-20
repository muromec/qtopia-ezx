/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxparse.h,v 1.4 2007/07/06 20:39:23 jfinnecy Exp $
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

#ifndef __HXPARSE_H
#define __HXPARSE_H





/****************************************************************************
 *  HXParseColor
 *
 *  Parses a smil/html color string and returns its HXxColor value.  The
 *  string should be in one of the following formats: "#RGB", "#RRGGBB",
 *  or one of the pre-defined strings in the table at the top of this file.
 */

HX_RESULT HXParseColor(const char* pColorString, REF(HXxColor) theColor);

/****************************************************************************
 *  HXParseDigit
 *
 *  Parses an integer digit, returning an error if the entire string is not
 *  a digit.  The function uses atol for the actual conversion:
 *  [whitespace][sign]digits
 */

HX_RESULT HXParseDigit(const char* pDigitString, REF(INT32) ulOut);

/****************************************************************************
 *  HXParseDouble
 *
 *  Parses an double value, returning an error if the entire string is not
 *  a valid value.  The function uses atof for the actual conversion:
 *  [whitespace] [sign] [digits] [.digits] [ {d | D | e | E}[sign]digits]
 */

HX_RESULT HXParseDouble(const char* pDigitString, REF(double) dOut);

/****************************************************************************
 *  HXParseColorUINT32
 *
 *  Same as HXParseColor, but it produces the same UINT32 on all platforms.
 *  HXParseColor produces 0x00BBGGRR on Windows and 0x00RRGGBB on non-Windows.
 */

HX_RESULT HXParseColorUINT32(const char* pszStr, REF(UINT32) rulValue);

/****************************************************************************
 *  HXParsePercent
 *
 *  Parses a percent value, returning an error if the entire string is not
 *  a valid value. The string should be: <double> '%' and the '%' must be
 *  present.
 */

HX_RESULT HXParsePercent(const char* pszStr, REF(double) rdValue);

/****************************************************************************
 *  HXParseUINT32
 *
 *  Parses an unsigned integer digit, returning an error if the entire
 *  string is not a digit.  The function uses strtoul for the actual conversion:
 *  [whitespace]digits
 */

HX_RESULT HXParseUINT32(const char* pszStr, REF(UINT32) rulValue);

/****************************************************************************
 *  HXParseOpacity
 *
 *  Opacity can be expressed as a [0-255] value or as a percent. This method
 *  attempts to parse as a percent and then if it's not able, it parses as
 *  a UINT32. The final value is clamped to be in the range [0-255].
 */

HX_RESULT HXParseOpacity(const char* pszStr, REF(UINT32) rulValue);

#endif
