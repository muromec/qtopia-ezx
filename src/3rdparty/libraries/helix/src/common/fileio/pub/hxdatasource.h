/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxdatasource.h,v 1.8 2008/03/14 18:29:08 gajia Exp $
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

#ifndef _HX_DATASOURCE_H_
#define _HX_DATASOURCE_H_

#include "hxcom.h"
#include "hxtypes.h"
#include "hxstring.h"
#include "hxfiles.h"
#include "ihxmmfdatasource.h"
#include "ihxpckts.h" //IHXBuffer

// warnning: This interface and related functionality is under development.
// IHXMMFDataSource API can change anytime.


class CHXDataSource : public IHXMMFDataSource
{
public:
    CHXDataSource();
    virtual ~CHXDataSource();

    STDMETHOD(QueryInterface)       (THIS_ REFIID riid, void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)      (THIS);

    STDMETHOD_(ULONG32,Release)     (THIS);

    STDMETHOD(Open)    (THIS_ IHXRequest *pRequest, const char *mode, IHXMMFDataSourceObserver* pObserver = NULL);
    STDMETHOD(Seek)    (THIS_ UINT32 offset, INT32 fromWhere);
    STDMETHOD(Stat)    (THIS_ struct stat *buf);
    STDMETHOD(GetStringAttribute)(THIS_ UINT32 ulAttibute,
                             REF(IHXBuffer*)  pBuffer);
    STDMETHOD(GetAttribute)(THIS_ INT32 ulAttribute,
                              INT32 &ulValue);

    STDMETHOD(Close)   (THIS_) PURE;
    virtual UINT32  Read(THIS_  void *, ULONG32 size, ULONG32 count) PURE;
    virtual UINT32  Write(THIS_ void *, ULONG32 size, ULONG32 count) PURE;
    STDMETHOD(GetSize)  (THIS_ UINT32 &ulSize) PURE;
    STDMETHOD(GetLastError)   (THIS_);
    HXBOOL AsyncReadSupported(THIS_);
public: //enable multi read/write
    STDMETHOD(Open2)    (THIS_ IHXRequest *pRequest, const char *mode, IHXMMFDataSourceObserver* pObserver, IHXFileObject* pFileObject);
    STDMETHOD(Seek2)    (THIS_ UINT32 offset, INT32 fromWhere, IHXFileObject* pFileObject);
    STDMETHOD(Close2)   (THIS_ IHXFileObject* pFileObject);
    virtual UINT32  Read2(THIS_  IHXBuffer* pBuffer, IHXFileObject* pFileObject);
    virtual UINT32  Write2(THIS_ void *, ULONG32 size, ULONG32 count, IHXFileObject* pFileObject);
    STDMETHOD(GetLastError2)   ( THIS_ IHXFileObject* pFileObject);

protected: //enable multi read/write
    HX_RESULT RestoreFileObject( IHXFileObject* pFileObject );
    HX_RESULT FindFileObjectIndex( TInt& index, const IHXFileObject* pFileObject ) const;
    HX_RESULT UpdateFileObject( TInt index, UINT32 position );

protected:
    inline void SetLastError(HX_RESULT lErr) {m_ulLastError = lErr;}

protected:
    ULONG32              m_ulRefCount;
    ULONG32              m_ulPosition; // current offset for the data
    UINT32               m_ulLastError;
    IHXRequest*          m_pRequest;
    IHXBuffer*           m_pHXBuffer;

protected: //enable multi read/write
    RPointerArray<IHXFileObject> m_FileObjectList;
    RArray< TUint >      m_FileObjectPosition;
    TInt                 m_lLastFileObjectIndex;
};

#endif // _HX_DATASOURCE_H_

