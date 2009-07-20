/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxpluginarchive.h,v 1.3 2006/02/23 23:10:07 nhart Exp $
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

#ifndef HXPLUGINARCHIVE_H__
#define HXPLUGINARCHIVE_H__


#include "hxstring.h"
#include "hxtypes.h"
#include "hxcom.h"

_INTERFACE IHXValues;
_INTERFACE IHXBuffer;
_INTERFACE IHXCommonClassFactory;
_INTERFACE IHXFragmentedBuffer;

class CHXDataFile;


// class HXPluginArchiveBase
// the reader/writer classes share some logic
class HXPluginArchiveBase
{
public:
    HXPluginArchiveBase();
    virtual ~HXPluginArchiveBase();

    HX_RESULT SetContext(IUnknown* pContext);

    HX_RESULT SetFile(IUnknown* pContext, CHXDataFile* pDataFile, bool bOwnFile = FALSE);

#if defined(HELIX_CONFIG_PLUGIN_ARCHIVE_BUFFER)
    virtual HX_RESULT SetBuffer(IUnknown* pContext, IHXFragmentedBuffer* pFragBuf);
#endif

    INT32 _Write(const char* pBuf, UINT32 uSize);
    INT32 _Read(char* pBuf, UINT32 uSize);

protected:
    virtual void CloseFile();

    IHXBuffer* CreateBuffer(const UCHAR* pData, UINT32 uSize);

    IHXCommonClassFactory*  m_pFactory;
    CHXDataFile*            m_pFile;
    bool                    m_bOwnFile;
#if defined(HELIX_CONFIG_PLUGIN_ARCHIVE_BUFFER)
    IHXFragmentedBuffer*    m_pFragBuf;
    UINT32                  m_uReadPos;
#endif
};


// class HXPluginArchiveWriter
class HXPluginArchiveWriter : public HXPluginArchiveBase
{
public:
    HXPluginArchiveWriter();

    HX_RESULT Open(IUnknown* pContext, const char* pszFile);
    HX_RESULT OpenWithFile(IUnknown* pContext, CHXDataFile* pFile);
    void Close();

    void Write(const char* psz);
    void Write(IHXValues* pval);
    void Write(UINT32 val);
    void Write(UINT16 val);
    void Write(IHXBuffer* pBuffer);
    void Write(bool b);
    void Break();

protected:
    void EscapeValue(const CHXString& str, CHXString& strOut);

private:

    bool m_bAtLeastOneVal;
};

// class HXPluginArchiveReader
class HXPluginArchiveReader : public HXPluginArchiveBase
{
public:
    HXPluginArchiveReader();

    HX_RESULT Open(IUnknown* pContext, const char* pszFile);
    HX_RESULT OpenWithFile(IUnknown* pContext, CHXDataFile* pFile);
#if defined(HELIX_CONFIG_PLUGIN_ARCHIVE_BUFFER)
    HX_RESULT SetBuffer(IUnknown* pContext, IHXFragmentedBuffer* pFragBuf);
#endif

    void Close();

    bool Read(IHXValues*& pval);
    void Read(UINT32& val);
    void Read(UINT16& val);
    void Read(CHXString& str);
    void Read(IHXBuffer*& pBuff);
    void Read(bool& b);
    bool AtEnd();


protected:
    void LoadFile();
    void GetNextToken(CHXString& str, char chTerm);
    bool ScanSpaceTo(char ch);
   
private:

    CHXString m_strFile;
    UINT32 m_idxFile;
};

inline
bool HXPluginArchiveReader::AtEnd()
{
    return m_idxFile == m_strFile.GetLength();
}

//XXXLCM util
struct HXBufferUtil
{
    static IHXBuffer* CreateBuffer(IHXCommonClassFactory* pFact, const char* psz);
};


#endif /* HXPLUGINARCHIVE_H__ */
