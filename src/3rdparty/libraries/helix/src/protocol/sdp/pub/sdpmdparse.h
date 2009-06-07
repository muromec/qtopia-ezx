/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: sdpmdparse.h,v 1.16 2006/05/15 18:49:24 damann Exp $
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

#ifndef SDPMDUNPACK_H
#define SDPMDUNPACK_H

#include "mdescparse.h"

#include "hxstring.h"
#include "hxslist.h"
#include "hxcom.h"
#include "hxccf.h"
#include "ihxlist.h" // IHXList

class SDPParseState;

class SDPMediaDescParser : public MediaDescParser
{
public:
    SDPMediaDescParser(ULONG32 ulVersion);
    virtual ~SDPMediaDescParser();

    virtual HX_RESULT Init(IUnknown* pContext);
    virtual HX_RESULT Parse(IHXBuffer* pDescription,
                            REF(UINT16) nValues,
                            REF(IHXValues**) pValueArray);
private:
    enum FieldType {ftUnknown,
                    ftULONG32,
                    ftString,
                    ftBuffer};

    void TakeCareOfDefaults(IHXValues* pHeader);
    void AddRuleBook(IHXValues* pHeader, UINT32 ulBW);

    HX_RESULT fromExternalRep(char* pData);
    HX_RESULT fromExternalRep(char* pData, UINT32 ulDataLen);
    HXBOOL IsPayloadTableValid(UINT32 ulPayloadType) const;
    HX_RESULT getRTPMapInfo(const char*    pMapInfo,
                            UINT32         ulRTPPayloadType,
                            REF(CHXString) strMimeType,
                            IHXValues*     pStream) const;
    UINT32    GetDefaultSampleRate(UINT32 ulRTPPayloadType, const char* pszMimeType) const;
    UINT32    GetDefaultChannels(UINT32 ulRTPPayloadType, const char* pszMimeType) const;
    HX_RESULT checkVersion(UINT32 ulVersion) const;
    void clearStreamList();

    IHXValues* CreateHeader() const;

    void AddULONG32(IHXValues* pHeader, const char* pKey,
                    ULONG32 ulValue) const;
    void AddString(IHXValues* pHeader, const char* pKey,
                   const char* pValue) const;
    void AddBuffer(IHXValues* pHeader, const char* pKey, const UINT8* pValue,
                   ULONG32 ulLength) const;

    IHXBuffer* CopyBuffer(const UINT8* pBuf, ULONG32 ulLength) const;

    HX_RESULT GetLine(const char*& pData, const char* pEnd,
                      IHXBuffer*& pLine, char lineType) const;

    void SkipSpaces(char*& pData) const;
    HXBOOL ScanForDelim(char*& pData, char delim) const;

    HX_RESULT HandleVLine(char* pLine) const;
    HX_RESULT HandleMLine(SDPParseState* pState, char* pLine,
                          IHXValues* pHdr) const;
    HX_RESULT HandleALine(SDPParseState* pState, char* pLine,
                          IHXValues* pHdr) const;
    HX_RESULT HandleCLine(char* pLine, IHXValues* pHdr) const;
    HX_RESULT HandleBLine(char* pLine, IHXValues* pHdr) const;

    HX_RESULT ParseFieldValue(char*& pValue, FieldType& fieldType) const;
    HX_RESULT HandleSpecialFields(SDPParseState* pState,
                                  const char* pFieldName,
                                  char* pFieldValue,
                                  IHXValues* pHdr) const;

    HX_RESULT HandleRangeField(SDPParseState* pState, char* pFieldValue,
                               IHXValues* pHdr) const;
    HX_RESULT HandleLengthField(SDPParseState* pState, char* pFieldValue,
                                IHXValues* pHdr) const;
    HX_RESULT HandleRTPMapField(SDPParseState* pState, char* pFieldValue,
                                IHXValues* pHdr) const;
    HX_RESULT HandleFMTPField(char* pFieldValue, IHXValues* pHdr) const;
    HX_RESULT HandlePrerollField(char* pFieldValue, ULONG32 ulPrerollUnits,
                                 IHXValues* pHdr) const;

    HX_RESULT HandleAltGroupField(char* pFieldValue, IHXValues* pHdr) const;
    HX_RESULT GetAltGroupType(IHXValues2* pHdr, const char* pGroupType,
                              HXBOOL bCreateIfNeeded,
                              REF(IHXValues2*) pAltGroupType) const;
    HX_RESULT ParseAltGroups(IHXValues2* pHdr, const char* pGroupType,
                             char* pBuf) const;
    HX_RESULT ParseAltIDList(char*& pBuf, REF(IHXBuffer*) pValues) const;

    HX_RESULT HandleAltField(SDPParseState* pState, char* pFieldValue,
                             IHXValues* pHdr) const;

    HX_RESULT  HandleFramesizeField(char* pFieldValue, IHXValues* pHdr) const;
    HX_RESULT  HandleFramerateField(char* pFieldValue, IHXValues* pHdr) const;

    HX_RESULT  Handle3GPPAssetInformationField(char* pFieldValue,
                                               IHXValues* pHdr) const;

    HX_RESULT AddAltData(IHXValues2* pHdr, const char* pAltID,
                         IHXValues* pAltData) const;
    HX_RESULT GetIHXValue2(IHXValues2* pHdr, const char* pKey,
                           REF(IHXValues2*) pVal,
                           HXBOOL bCreateIfNeeded) const;
    HX_RESULT GetIHXList(IHXValues2* pHdr, const char* pKey,
                         REF(IHXList*) pVal) const;

    HXBOOL IsToken(const char* pStart, const char* pEnd) const;
    HXBOOL IsAltID(const char* pStart, const char* pEnd) const;
    HXBOOL IsClient();

    IUnknown* m_pContext;
    IHXCommonClassFactory* m_pCCF;
    ULONG32 m_ulVersion;
    CHXSimpleList m_streams;
    IHXValues* m_pFileHeader;
};

#endif /* SDPMDUNPACK_H */
