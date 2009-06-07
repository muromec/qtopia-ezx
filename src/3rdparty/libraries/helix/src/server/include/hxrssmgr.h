/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxrssmgr.h,v 1.3 2003/12/17 22:24:13 atin Exp $
 *
 * Portions Copyright (c) 1995-2003 RealNetworks, Inc. All Rights Reserved.
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

#ifndef _HXSTATSMGR_H_
#define _HXSTATSMGR_H_

class Process;


_INTERFACE  IUnknown;

_INTERFACE  IHXList;
_INTERFACE  IHXLogOutput;
_INTERFACE  IHXRSSReport;
_INTERFACE  IHXRSSManager;


///////////////////////////////////////////////////////////////////////////////
// Interface:
//
//      IHXRSSReport
//
// Purpose:
//
//      Used to give the ReportServerStats callback access to stats objects
//
// IID_IHXRSSReport:
//
//      {BD2F1E35-83E4-4459-9430-2EB637ADBE17}
//
///////////////////////////////////////////////////////////////////////////////

// {BD2F1E35-83E4-4459-9430-2EB637ADBE17}
DEFINE_GUID(IID_IHXRSSReport, 0xbd2f1e35, 0x83e4, 0x4459, 0x94, 0x30, 0x2e,
            0xb6, 0x37, 0xad, 0xbe, 0x17);

#define CLSID_IHXRSSReport    IID_IHXRSSReport

#undef INTERFACE
#define INTERFACE IHXRSSReport

DECLARE_INTERFACE_(IHXRSSReport, IUnknown)
{
    // IUnknown Methods
    STDMETHOD(QueryInterface)               (THIS_
                                            REFIID riid,
                                            void** ppvObj) PURE;

    STDMETHOD_(UINT32,AddRef)               (THIS) PURE;

    STDMETHOD_(UINT32,Release)              (THIS) PURE;


    // IID_IHXRSSReport Methods
    STDMETHOD(Report)                       (THIS_
                                             IHXLogOutput* pOutput,
                                             BOOL bEchoToStdout,
                                             HXTimeval tSkedNow) PURE;
};

///////////////////////////////////////////////////////////////////////////////
// Interface:
//
//      IHXRSSManager
//
// Purpose:
//
//      Maintians a list of RSSReports (IHXRSSReport) objects
//
// IID_IHXRSSManager:
//
//      {66FB8DC5-D3D4-4aaf-8F88-56B3C8A7D31F}
//
///////////////////////////////////////////////////////////////////////////////

// {66FB8DC5-D3D4-4aaf-8F88-56B3C8A7D31F}
DEFINE_GUID(IID_IHXRSSManager, 0x66fb8dc5, 0xd3d4, 0x4aaf, 0x8f,
            0x88, 0x56, 0xb3, 0xc8, 0xa7, 0xd3, 0x1f);

#define CLSID_IHXRSSManager   IID_IHXRSSManager

#undef INTERFACE
#define INTERFACE IHXRSSManager

DECLARE_INTERFACE_(IHXRSSManager, IUnknown)
{
    // IUnknown Methods
    STDMETHOD(QueryInterface)               (THIS_
                                            REFIID riid,
                                            void** ppvObj) PURE;

    STDMETHOD_(UINT32,AddRef)               (THIS) PURE;

    STDMETHOD_(UINT32,Release)              (THIS) PURE;


    // IHXRSSManager Methods
    STDMETHOD(Register) (THIS_
                         IHXRSSReport* pStatsObj) PURE;

    STDMETHOD(Remove) (THIS_
                       IHXRSSReport* pStatsObj) PURE;
};

#endif /* _HXSTATSMGR_H_ */
