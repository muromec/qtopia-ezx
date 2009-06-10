/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: ihxmmfdatasource.h,v 1.8 2008/03/14 18:32:30 gajia Exp $
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

#ifndef _IHXMMFDATASOURCE_H_
#define _IHXMMFDATASOURCE_H_


/****************************************************************************
 *
 *  Interface:
 *
 *  IHXMMFDataSourceObserver
 *
 *  Purpose:
 *
 *  Data source observer interface. Used for asynchronous data transfer.
 *  object ownership is best managed through COM style reference
 *  counting. 
 *
 *  IID_IHXMMFDataSourceObserver:
 *
 *  {75887012-4FA7-473b-9714-B908E4EE9D99}
 */

DEFINE_GUID(IID_IHXMMFDataSourceObserver, 0x75887012, 0x4fa7, 0x473b, 0x97, 0x14, 0xb9, 0x8, 0xe4, 0xee, 0x9d, 0x99);

#undef  INTERFACE
#define INTERFACE   IHXMMFDataSourceObserver

DECLARE_INTERFACE_(IHXMMFDataSourceObserver, IUnknown)
{
 /*
  *  IUnknown methods
  */
 STDMETHOD(QueryInterface)       (THIS_
                 REFIID riid,
                 void** ppvObj) PURE;

 STDMETHOD_(ULONG32,AddRef)      (THIS) PURE;

 STDMETHOD_(ULONG32,Release)     (THIS) PURE;
 /*
    Informs datasource observer once read request is complete.
 */
 virtual void ReadDone(IHXBuffer* pBuffer, UINT32 byteCount) PURE;
};

// warnning: This interface and related functionality is under development.
// IHXMMFDataSource API can change anytime.

/****************************************************************************
 *
 *  Interface:
 *
 *  IHXMMFDataSource
 *
 *  Purpose:
 *
 *  Basic data source interface. Used in CHXRequest2 in common/system/
 *  object ownership is best managed through COM style reference
 *  counting. 
 *
 *  IID_IHXMMFDataSource:
 *
 *  {2650FE34-A181-487c-A32D-30356018B380}
 */


DEFINE_GUID(IID_IHXMMFDataSource, 0x2650fe34, 0xa181, 0x487c, 0xa3, 0x2d, 0x30, 0x35, 0x60, 0x18, 0xb3, 0x80);


#undef  INTERFACE
#define INTERFACE   IHXMMFDataSource

DECLARE_INTERFACE_(IHXMMFDataSource, IUnknown)
{
 /*
  *  IUnknown methods
  */
 STDMETHOD(QueryInterface)       (THIS_
                 REFIID riid,
                 void** ppvObj) PURE;

 STDMETHOD_(ULONG32,AddRef)      (THIS) PURE;

 STDMETHOD_(ULONG32,Release)     (THIS) PURE;


 STDMETHOD(Open)         (THIS_
                    IHXRequest *pRequest, const char *mode, IHXMMFDataSourceObserver* pObserver = NULL
                 ) PURE;

 STDMETHOD(Close)        (THIS_) PURE;

    /*
        reads "count" elements each of size "size".
    */
 virtual UINT32 Read(THIS_
                   void * buf, ULONG32 size, ULONG32 count
                 ) PURE;

    /*
        writes "count" elements each of size "size".
    */
 virtual UINT32 Write(THIS_
                   void *buf, ULONG32 size, ULONG32 count
                 ) PURE;

 STDMETHOD(Seek)    (THIS_ UINT32, INT32 fromWhere) PURE;
 STDMETHOD(Stat)    (THIS_ struct stat *buf) PURE;

 STDMETHOD(GetStringAttribute)(THIS_ UINT32 ulAttibute,
                           REF(IHXBuffer*)  pBuffer) PURE;
 STDMETHOD(GetAttribute)(THIS_ INT32 ulAttribute,
                            INT32 &ulValue) PURE;
 
 STDMETHOD(GetSize)  (THIS_ UINT32 &ulSize) PURE;

 STDMETHOD(GetLastError)  (THIS_) PURE;

//enable multi read/write 
 STDMETHOD(Open2)         (THIS_
                    IHXRequest *pRequest, const char *mode, IHXMMFDataSourceObserver* pObserver, IHXFileObject* pFileObject
                 ) PURE;

 STDMETHOD(Close2)        (THIS_ IHXFileObject* pFileObject) PURE;

    /*
        reads "count" elements each of size "size".
    */
 virtual UINT32 Read2(THIS_
                   IHXBuffer* pBuffer, IHXFileObject* pFileObject
                 ) PURE;

    /*
        writes "count" elements each of size "size".
    */
 virtual UINT32 Write2(THIS_
                   void *buf, ULONG32 size, ULONG32 count, IHXFileObject* pFileObject
                 ) PURE;

 STDMETHOD(Seek2)    (THIS_ UINT32, INT32 fromWhere, IHXFileObject* pFileObject) PURE;

 STDMETHOD(GetLastError2)  (THIS_ IHXFileObject* pFileObject) PURE;
 
 virtual HXBOOL AsyncReadSupported(THIS_) PURE;

};

#endif // _IHXMMFDATASOURCE_H_

