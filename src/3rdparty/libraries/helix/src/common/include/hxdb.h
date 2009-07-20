/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxdb.h,v 1.3 2004/07/09 18:20:48 hubbe Exp $
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

#ifndef _HXDB_H_
#define _HXDB_H_

/*
 * Forward declarations of some interfaces defined or used here-in.
 */
typedef _INTERFACE	IHXBuffer				IHXBuffer;
typedef _INTERFACE	IHXValues				IHXValues;
typedef _INTERFACE	IHXDatabaseManager			IHXDatabaseManager;
typedef _INTERFACE	IHXAuthenticationDBManager		IHXAuthenticationDBManager;
typedef _INTERFACE	IHXAuthenticationDBManagerResponse	IHXAuthenticationDBManagerResponse;
typedef _INTERFACE	IHXAsyncEnumAuthenticationDB		IHXAsyncEnumAuthenticationDB;
typedef _INTERFACE	IHXAsyncEnumAuthenticationDBResponse	IHXAsyncEnumAuthenticationDBResponse;
typedef _INTERFACE	IHXAuthenticationDBAccess		IHXAuthenticationDBAccess;
typedef _INTERFACE	IHXAuthenticationDBAccessResponse	IHXAuthenticationDBAccessResponse;
typedef _INTERFACE	IHXGUIDDBManager			IHXGUIDDBManager;
typedef _INTERFACE	IHXGUIDDBManagerResponse		IHXGUIDDBManagerResponse;
typedef _INTERFACE	IHXPPVDBManager			IHXPPVDBManager;
typedef _INTERFACE	IHXPPVDBManagerResponse		IHXPPVDBManagerResponse;
typedef _INTERFACE	IHXRedirectDBManager			IHXRedirectDBManager;
typedef _INTERFACE	IHXRedirectDBManagerResponse		IHXRedirectDBManagerResponse;
typedef _INTERFACE	IHXRegistrationLogger			IHXRegistrationLogger;

/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXDatabaseManager
 *
 *  Purpose:
 *
 *	This is implemented by the database manager in order to provide
 *	access to the databases it manages.
 *
 *  IHXDatabaseManager:
 *
 *	{00002A00-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXDatabaseManager,   0x00002A00, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#define CLSID_CHXDatabaseManager IID_IHXDatabaseManager

#undef  INTERFACE
#define INTERFACE   IHXDatabaseManager

DECLARE_INTERFACE_(IHXDatabaseManager, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)
    (
	THIS_
	REFIID			IIDOfInterfaceDesired,
	void**			ppVoidRequestedInterface
    ) PURE;

    STDMETHOD_(ULONG32,AddRef)
    (
	THIS
    ) PURE;

    STDMETHOD_(ULONG32,Release)
    (
	THIS
    ) PURE;

    /************************************************************************
     *	Method:
     *	    IHXDatabaseManager::GetInstanceFromID
     *	Purpose:
     *	    
     *	    Returns a database object configured as defined for the specifed 
     *	    DatabaseID in the server config file.
     *
     */
    STDMETHOD(GetInstanceFromID)
    (
	THIS_
	IHXBuffer*		pBufferID, 
	REF(IUnknown*)		pUnknownDatabase
    ) PURE;

};

/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXAuthenticationDBManager
 *
 *  Purpose:
 *
 *	A database plugin will implement this when it desires to provide 
 *	storage for authentication data.
 *
 *  IHXAuthenticationDBManager:
 *
 *	{00002A02-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXAuthenticationDBManager,   0x00002A02, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXAuthenticationDBManager

DECLARE_INTERFACE_(IHXAuthenticationDBManager, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)
    (
	THIS_
	REFIID			IIDOfInterfaceDesired,
	void**			ppVoidRequestedInterface
    ) PURE;

    STDMETHOD_(ULONG32,AddRef)
    (
	THIS
    ) PURE;

    STDMETHOD_(ULONG32,Release)
    (
	THIS
    ) PURE;

    /************************************************************************
     *	Method:
     *	    IHXAuthenticationDBManager::AddPrincipal
     *	Purpose:
     *	    
     *	    Adds the specified user to the database, if it is not already 
     *	    there.
     *
     */
    STDMETHOD(AddPrincipal)
    (
	THIS_ 
	IHXAuthenticationDBManagerResponse* pAuthenticationDBManagerResponseNew,
	IHXBuffer*		pBufferPrincipalID
    ) PURE;

    /************************************************************************
     *	Method:
     *	    IHXAuthenticationDBManager::RemovePrincipal
     *	Purpose:
     *	    
     *	    Removes the specified user from the database, if it is there.
     *	    
     *
     */
    STDMETHOD(RemovePrincipal)
    (
	THIS_ 
	IHXAuthenticationDBManagerResponse* pAuthenticationDBManagerResponseNew,
	IHXBuffer*		pBufferPrincipalID
    ) PURE;

    /************************************************************************
     *	Method:
     *	    IHXAuthenticationDBManager::SetCredentials
     *	Purpose:
     *	    
     *	    Replaces the credentials for the specified user.
     *	    Usually the credentials are a password.
     *	    It is not the databases job to protect this data
     *	    Authentication plugins will protect this data 
     *	    before storing it when neccesary.
     *
     */
    STDMETHOD(SetCredentials)
    (
	THIS_
	IHXAuthenticationDBManagerResponse* pAuthenticationDBManagerResponseNew,
	IHXBuffer*		pBufferPrincipalID, 
	IHXBuffer*		pBufferCredentials
    ) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXAuthenticationDBManagerResponse
 *
 *  Purpose:
 *
 *	A database user will implement this when it desires to manage 
 *	storage for authentication data.
 *	This interface receives the results of IHXAuthenticationDBManager
 *	methods
 *
 *  IHXAuthenticationDBManagerResponse:
 *
 *	{00002A01-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXAuthenticationDBManagerResponse,   0x00002A01, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXAuthenticationDBManagerResponse

DECLARE_INTERFACE_(IHXAuthenticationDBManagerResponse, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)
    (
	THIS_
	REFIID			IIDOfInterfaceDesired,
	void**			ppVoidRequestedInterface
    ) PURE;

    STDMETHOD_(ULONG32,AddRef)
    (
	THIS
    ) PURE;

    STDMETHOD_(ULONG32,Release)
    (
	THIS
    ) PURE;

    /************************************************************************
     *	Method:
     *	    IHXAuthenticationDBManagerResponse::AddPrincipalDone
     *	Purpose:
     *	    
     *	    Reports the success or failure of 
     *	    IHXAuthenticationDBManager::AddPrincipal
     *
     */
    STDMETHOD(AddPrincipalDone)
    (
	THIS_ 
	HX_RESULT		ResultStatus,
	IHXBuffer*		pBufferPrincipalID
    ) PURE;

    /************************************************************************
     *	Method:
     *	    IHXAuthenticationDBManagerResponse::RemovePrincipalDone
     *	Purpose:
     *	    
     *	    Reports the success or failure of 
     *	    IHXAuthenticationDBManager::RemovePrincipal
     *
     */
    STDMETHOD(RemovePrincipalDone)
    (
	THIS_ 
	HX_RESULT		ResultStatus,
	IHXBuffer*		pBufferPrincipalID
    ) PURE;

    /************************************************************************
     *	Method:
     *	    IHXAuthenticationDBManagerResponse::SetCredentialsDone
     *	Purpose:
     *	    
     *	    Reports the success or failure of 
     *	    IHXAuthenticationDBManager::SetCredentials
     *
     */
    STDMETHOD(SetCredentialsDone)
    (
	THIS_
	HX_RESULT		ResultStatus,
	IHXBuffer*		pBufferPrincipalID
    ) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXAsyncEnumAuthenticationDB
 *
 *  Purpose:
 *
 *	A database plugin will implement this when it desires to provide 
 *	enumeration of authentication data.
 *
 *  IHXAsyncEnumAuthenticationDB:
 *
 *	{00002A04-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXAsyncEnumAuthenticationDB,   0x00002A04, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXAsyncEnumAuthenticationDB

DECLARE_INTERFACE_(IHXAsyncEnumAuthenticationDB, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)
    (
	THIS_
	REFIID			IIDOfInterfaceDesired,
	void**			ppVoidRequestedInterface
    ) PURE;

    STDMETHOD_(ULONG32,AddRef)
    (
	THIS
    ) PURE;

    STDMETHOD_(ULONG32,Release)
    (
	THIS
    ) PURE;

    /************************************************************************
     *	Method:
     *	    IHXAsyncEnumAuthenticationDB::Reset
     *	Purpose:
     *	    
     *	    Call this to reset this enumerator to the beginning of the 
     *	    collection.
     *
     */
    STDMETHOD(Reset)
    (
	THIS_
	IHXAsyncEnumAuthenticationDBResponse* pAsyncEnumAuthenticationDBResponseNew
    ) PURE;

    /************************************************************************
     *	Method:
     *	    IHXAsyncEnumAuthenticationDB::Next
     *	Purpose:
     *	    
     *	    Call this to retrieve the next item in the collection.
     *
     */
    STDMETHOD(Next)
    (
	THIS_
	IHXAsyncEnumAuthenticationDBResponse* pAsyncEnumAuthenticationDBResponseNew
    ) PURE;

    /************************************************************************
     *	Method:
     *	    IHXAsyncEnumAuthenticationDB::Skip
     *	Purpose:
     *	    
     *	    Call this to skip the next n items in the collection and 
     *	    retrieve the n+1 item.
     *
     */
    STDMETHOD(Skip)
    (
	THIS_
	IHXAsyncEnumAuthenticationDBResponse* pAsyncEnumAuthenticationDBResponseNew,
	UINT32			ulNumToSkip
    ) PURE;

    /************************************************************************
     *	Method:
     *	    IHXAsyncEnumAuthenticationDB::Clone
     *	Purpose:
     *	    
     *	    Call this to make a new enumerator of this collection.
     *
     */
    STDMETHOD(Clone)
    (
	THIS_
	REF(IHXAsyncEnumAuthenticationDB*) pAsyncEnumAuthenticationDBNew
    ) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXAsyncEnumAuthenticationDBResponse
 *
 *  Purpose:
 *
 *	A database user will implement this when it desires to 
 *	enumerate authentication data.
 *	This interface receives the results of IHXAsyncEnumAuthenticationDB
 *	methods
 *
 *  IHXAsyncEnumAuthenticationDBResponse:
 *
 *	{00002A03-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXAsyncEnumAuthenticationDBResponse,   0x00002A03, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXAsyncEnumAuthenticationDBResponse

DECLARE_INTERFACE_(IHXAsyncEnumAuthenticationDBResponse, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)
    (
	THIS_
	REFIID			IIDOfInterfaceDesired,
	void**			ppVoidRequestedInterface
    ) PURE;

    STDMETHOD_(ULONG32,AddRef)
    (
	THIS
    ) PURE;

    STDMETHOD_(ULONG32,Release)
    (
	THIS
    ) PURE;

    /************************************************************************
     *	Method:
     *	    IHXAsyncEnumAuthenticationDBResponse::ResetDone
     *	Purpose:
     *	    
     *	    Reports the success or failure of 
     *	    IHXAsyncEnumAuthenticationDB::Reset
     *
     */
    STDMETHOD(ResetDone)
    (
	THIS_
	HX_RESULT		ResultStatus
    ) PURE;

    /************************************************************************
     *	Method:
     *	    IHXAsyncEnumAuthenticationDBResponse::NextDone
     *	Purpose:
     *	    
     *	    Reports the success or failure of 
     *	    IHXAsyncEnumAuthenticationDB::Next
     *	    If successful the PrincipalID is valid.
     *
     */
    STDMETHOD(NextDone)
    (
	THIS_
	HX_RESULT		ResultStatus,
	IHXBuffer*		pBufferNextPrincipalID
    ) PURE;

    /************************************************************************
     *	Method:
     *	    IHXAsyncEnumAuthenticationDBResponse::SkipDone
     *	Purpose:
     *	    
     *	    Reports the success or failure of 
     *	    IHXAsyncEnumAuthenticationDB::Skip
     *	    If successful the PrincipalID is valid.
     *
     */
    STDMETHOD(SkipDone)
    (
	THIS_
	HX_RESULT		ResultStatus,
	IHXBuffer*		pBufferNextPrincipalID
    ) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXAuthenticationDBAccess
 *
 *  Purpose:
 *
 *	A database plugin will implement this when it desires to provide 
 *	access to authentication data.
 *
 *  IHXAuthenticationDBAccess:
 *
 *	{00002A06-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXAuthenticationDBAccess,   0x00002A06, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXAuthenticationDBAccess

DECLARE_INTERFACE_(IHXAuthenticationDBAccess, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)
    (
	THIS_
	REFIID			IIDOfInterfaceDesired,
	void**			ppVoidRequestedInterface
    ) PURE;

    STDMETHOD_(ULONG32,AddRef)
    (
	THIS
    ) PURE;

    STDMETHOD_(ULONG32,Release)
    (
	THIS
    ) PURE;

    /************************************************************************
     *	Method:
     *	    IHXAuthenticationDBAccess::_NewEnum
     *	Purpose:
     *	    
     *	    Call this to make a new enumerator of this collection.
     *
     */
    STDMETHOD(_NewEnum)
    (
	THIS_
	REF(IHXAsyncEnumAuthenticationDB*) pAsyncEnumAuthenticationDBNew
    ) PURE;

    /************************************************************************
     *	Method:
     *	    IHXAuthenticationDBAccess::CheckExistence
     *	Purpose:
     *	    
     *	    Call this to verify the existance of a principal.
     *
     */
    STDMETHOD(CheckExistence)
    (
	THIS_
	IHXAuthenticationDBAccessResponse* pAuthenticationDBAccessResponseNew,
	IHXBuffer*		pBufferPrincipalID
    ) PURE;

    /************************************************************************
     *	Method:
     *	    IHXAuthenticationDBAccess::GetCredentials
     *	Purpose:
     *	    
     *	    Call this to access the credentials for the specified principal.
     *
     */
    STDMETHOD(GetCredentials)
    (
	THIS_
	IHXAuthenticationDBAccessResponse* pAuthenticationDBAccessResponseNew,
	IHXBuffer*		pBufferPrincipalID
    ) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXAuthenticationDBAccessResponse
 *
 *  Purpose:
 *
 *	A database user will implement this when it desires to 
 *	access authentication data.
 *	This interface receives the results of IHXAuthenticationDBAccess
 *	methods
 *
 *  IHXAuthenticationDBAccessResponse:
 *
 *	{00002A05-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXAuthenticationDBAccessResponse,   0x00002A05, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXAuthenticationDBAccessResponse

DECLARE_INTERFACE_(IHXAuthenticationDBAccessResponse, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)
    (
	THIS_
	REFIID			IIDOfInterfaceDesired,
	void**			ppVoidRequestedInterface
    ) PURE;

    STDMETHOD_(ULONG32,AddRef)
    (
	THIS
    ) PURE;

    STDMETHOD_(ULONG32,Release)
    (
	THIS
    ) PURE;

    /************************************************************************
     *	Method:
     *	    IHXAuthenticationDBAccessResponse::ExistenceCheckDone
     *	Purpose:
     *	    
     *	    Reports the success or failure of 
     *	    IHXAuthenticationDBAccess::ExistenceCheck
     *
     */
    STDMETHOD(ExistenceCheckDone)
    (
	THIS_
	HX_RESULT		ResultStatus,
	IHXBuffer*		pBufferPrincipalID
    ) PURE;

    /************************************************************************
     *	Method:
     *	    IHXAuthenticationDBAccessResponse::GetCredentialsDone
     *	Purpose:
     *	    
     *	    Reports the success or failure of 
     *	    IHXAuthenticationDBAccess::GetCredentials
     *	    If successful the Credentials var is valid.
     *
     */
    STDMETHOD(GetCredentialsDone)
    (
	THIS_
	HX_RESULT		ResultStatus,
	IHXBuffer*		pBufferPrincipalID,
	IHXBuffer*		pBufferCredentials
    ) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXGUIDDBManager
 *
 *  Purpose:
 *
 *	A database plugin will implement this when it desires to provide 
 *	storage of player GUID data (for Player Authentication).
 *
 *  IHXGUIDDBManager:
 *
 *	{00002A08-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXGUIDDBManager,   0x00002A08, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXGUIDDBManager

DECLARE_INTERFACE_(IHXGUIDDBManager, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)
    (
	THIS_
	REFIID			IIDOfInterfaceDesired,
	void**			ppVoidRequestedInterface
    ) PURE;

    STDMETHOD_(ULONG32,AddRef)
    (
	THIS
    ) PURE;

    STDMETHOD_(ULONG32,Release)
    (
	THIS
    ) PURE;

    /************************************************************************
     *	Method:
     *	    IHXGUIDDBManager::SetGUIDForPrincipalID
     *	Purpose:
     *	    
     *	    Call this to associate a player GUID with a user.
     *
     */
    STDMETHOD(SetGUIDForPrincipalID)
    (
	THIS_
	IHXGUIDDBManagerResponse* pGUIDDBManagerResponseNew,
	IHXBuffer*		pBufferPrincipalID,
	IHXBuffer*		pBufferGUID
    ) PURE;

    /************************************************************************
     *	Method:
     *	    IHXGUIDDBManager::GetPrincipalIDFromGUID
     *	Purpose:
     *	    
     *	    Call this to get the associated player GUID from a user.
     *
     */
    STDMETHOD(GetPrincipalIDFromGUID)
    (
	THIS_
	IHXGUIDDBManagerResponse* pGUIDDBManagerResponseNew,
	IHXBuffer*		pBufferGUID
    ) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXGUIDDBManagerResponse
 *
 *  Purpose:
 *
 *	A database user will implement this when it desires to 
 *	manage player GUID data.
 *	This interface receives the results of IHXGUIDDBManager
 *	methods
 *
 *  IHXGUIDDBManagerResponse:
 *
 *	{00002A07-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXGUIDDBManagerResponse,   0x00002A07, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXGUIDDBManagerResponse

DECLARE_INTERFACE_(IHXGUIDDBManagerResponse, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)
    (
	THIS_
	REFIID			IIDOfInterfaceDesired,
	void**			ppVoidRequestedInterface
    ) PURE;

    STDMETHOD_(ULONG32,AddRef)
    (
	THIS
    ) PURE;

    STDMETHOD_(ULONG32,Release)
    (
	THIS
    ) PURE;

    /************************************************************************
     *	Method:
     *	    IHXGUIDDBManagerResponse::SetGUIDForPrincipalIDDone
     *	Purpose:
     *	    
     *	    Reports the success or failure of 
     *	    IHXGUIDDBManager::SetGUIDForPrincipalID
     *
     */
    STDMETHOD(SetGUIDForPrincipalIDDone)
    (
	THIS_
	HX_RESULT		ResultStatus,
	IHXBuffer*		pBufferPrincipalID
    ) PURE;

    /************************************************************************
     *	Method:
     *	    IHXGUIDDBManagerResponse::GetPrincipalIDFromGUIDDone
     *	Purpose:
     *	    
     *	    Reports the success or failure of 
     *	    IHXGUIDDBManager::GetGUIDForPrincipalID
     *
     */
    STDMETHOD(GetPrincipalIDFromGUIDDone)
    (
	THIS_
	HX_RESULT		ResultStatus,
	IHXBuffer*		pBufferGUID,
	IHXBuffer*		pBufferPrincipalID
    ) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXPPVDBManager
 *
 *  Purpose:
 *
 *	A database plugin will implement this when it desires to provide 
 *	storage of Pay-Per-View permission data.
 *
 *  IHXPPVDBManager:
 *
 *	{00002A0A-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXPPVDBManager,   0x00002A0A, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXPPVDBManager

DECLARE_INTERFACE_(IHXPPVDBManager, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)
    (
	THIS_
	REFIID			IIDOfInterfaceDesired,
	void**			ppVoidRequestedInterface
    ) PURE;

    STDMETHOD_(ULONG32,AddRef)
    (
	THIS
    ) PURE;

    STDMETHOD_(ULONG32,Release)
    (
	THIS
    ) PURE;

    /************************************************************************
     *	Method:
     *	    IHXPPVDBManager::GetPermissions
     *	Purpose:
     *	    
     *	    Call this to find the PPV permissions for the specified URL 
     *	    and user.
     *
     */
    STDMETHOD(GetPermissions)
    (
	THIS_
	IHXPPVDBManagerResponse* pPPVDBManagerResponseNew,
	IHXBuffer*		pBufferPrincipalID,
	IHXBuffer*		pBufferURL
    ) PURE;

    /************************************************************************
     *	Method:
     *	    IHXPPVDBManager::SetPermissions
     *	Purpose:
     *	    
     *	    Call this to set the PPV permissions for the specified URL 
     *	    and user.
     *
     */
    STDMETHOD(SetPermissions)
    (
	THIS_
	IHXPPVDBManagerResponse* pPPVDBManagerResponseNew,
	IHXBuffer*		pBufferPrincipalID,
	IHXBuffer*		pBufferURL,
	IHXValues*		pValuesPermissions
    ) PURE;

    /************************************************************************
     *	Method:
     *	    IHXPPVDBManager::RevokePermissions
     *	Purpose:
     *	    
     *	    Call this to remove the PPV permissions for the specified URL 
     *	    and user.
     *
     */
    STDMETHOD(RevokePermissions)
    (
	THIS_
	IHXPPVDBManagerResponse* pPPVDBManagerResponseNew,
	IHXBuffer*		pBufferPrincipalID,
	IHXBuffer*		pBufferURL
    ) PURE;

    /************************************************************************
     *	Method:
     *	    IHXPPVDBManager::RevokeAllPermissions
     *	Purpose:
     *	    
     *	    Call this to remove the PPV permissions for all URL's 
     *	    that this user has access too.
     *
     */
    STDMETHOD(RevokeAllPermissions)
    (
	THIS_
	IHXPPVDBManagerResponse* pPPVDBManagerResponseNew,
	IHXBuffer*		pBufferPrincipalID
    ) PURE;

    /************************************************************************
     *	Method:
     *	    IHXPPVDBManager::LogAccessAttempt
     *	Purpose:
     *	    
     *	    Call this to record the results of an attempt to access 
     *	    protected content.
     *
     */
    STDMETHOD(LogAccessAttempt)
    (
	THIS_
	IHXValues*		pValuesAccess
    ) PURE;

};

/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXPPVDBManagerResponse
 *
 *  Purpose:
 *
 *	A database user will implement this when it desires to 
 *	manage Pay-Per-View permission data.
 *	This interface receives the results of IHXPPVDBManager
 *	methods
 *
 *  IHXPPVDBManagerResponse:
 *
 *	{00002A09-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXPPVDBManagerResponse,   0x00002A09, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXPPVDBManagerResponse

DECLARE_INTERFACE_(IHXPPVDBManagerResponse, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)
    (
	THIS_
	REFIID			IIDOfInterfaceDesired,
	void**			ppVoidRequestedInterface
    ) PURE;

    STDMETHOD_(ULONG32,AddRef)
    (
	THIS
    ) PURE;

    STDMETHOD_(ULONG32,Release)
    (
	THIS
    ) PURE;

    /************************************************************************
     *	Method:
     *	    IHXPPVDBManagerResponse::GetPermissionsDone
     *	Purpose:
     *	    
     *	    Reports the success or failure of 
     *	    IHXPPVDBManager::GetPermissions
     *	    If successful then the Permissions are valid
     *
     */
    STDMETHOD(GetPermissionsDone)
    (
	THIS_
	HX_RESULT		ResultStatus,
	IHXBuffer*		pBufferPrincipalID,
	IHXValues*		pValuesPermissions
    ) PURE;

    /************************************************************************
     *	Method:
     *	    IHXPPVDBManagerResponse::SetPermissionsDone
     *	Purpose:
     *	    
     *	    Reports the success or failure of 
     *	    IHXPPVDBManager::SetPermissions
     *
     */
    STDMETHOD(SetPermissionsDone)
    (
	THIS_
	HX_RESULT		ResultStatus,
	IHXBuffer*		pBufferPrincipalID
    ) PURE;

    /************************************************************************
     *	Method:
     *	    IHXPPVDBManagerResponse::RevokePermissionsDone
     *	Purpose:
     *	    
     *	    Reports the success or failure of 
     *	    IHXPPVDBManager::RevokePermissions
     *
     */
    STDMETHOD(RevokePermissionsDone)
    (
	THIS_
	HX_RESULT		ResultStatus,
	IHXBuffer*		pBufferPrincipalID
    ) PURE;

    /************************************************************************
     *	Method:
     *	    IHXPPVDBManagerResponse::RevokeAllPermissionsDone
     *	Purpose:
     *	    
     *	    Reports the success or failure of 
     *	    IHXPPVDBManager::RevokeAllPermissions
     *
     */
    STDMETHOD(RevokeAllPermissionsDone)
    (
	THIS_
	HX_RESULT		ResultStatus,
	IHXBuffer*		pBufferPrincipalID
    ) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXRedirectDBManager
 *
 *  Purpose:
 *
 *	A database plugin will implement this when it desires to provide 
 *	storage of URL's to redirect.
 *
 *  IHXRedirectDBManager:
 *
 *	{00002A0C-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXRedirectDBManager,   0x00002A0C, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXRedirectDBManager

DECLARE_INTERFACE_(IHXRedirectDBManager, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)
    (
	THIS_
	REFIID			IIDOfInterfaceDesired,
	void**			ppVoidRequestedInterface
    ) PURE;

    STDMETHOD_(ULONG32,AddRef)
    (
	THIS
    ) PURE;

    STDMETHOD_(ULONG32,Release)
    (
	THIS
    ) PURE;

    /************************************************************************
     *	Method:
     *	    IHXRedirectDBManager::GetRedirect
     *	Purpose:
     *	    
     *	    Call this to retrieve the URL that the specified URL should
     *	    be redirected to.
     *
     */
    STDMETHOD(GetRedirect)
    (
	THIS_
	IHXRedirectDBManagerResponse* pRedirectDBManagerResponseNew,
	IHXBuffer*		pBufferURL
    ) PURE;

    /************************************************************************
     *	Method:
     *	    IHXRedirectDBManager::AddRedirect
     *	Purpose:
     *	    
     *	    Call this to set the new URL that the specified URL should
     *	    be redirected to.
     *
     */
    STDMETHOD(AddRedirect)
    (
	THIS_
	IHXRedirectDBManagerResponse* pRedirectDBManagerResponseNew,
	IHXBuffer*		pBufferURL,
	IHXBuffer*		pBufferNewURL
    ) PURE;

    /************************************************************************
     *	Method:
     *	    IHXRedirectDBManager::RemoveRedirect
     *	Purpose:
     *	    
     *	    Call this to stop redirecting the specified URL.
     *
     */
    STDMETHOD(RemoveRedirect)
    (
	THIS_
	IHXRedirectDBManagerResponse* pRedirectDBManagerResponseNew,
	IHXBuffer*		pBufferURL
    ) PURE;

};

/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXRedirectDBManagerResponse
 *
 *  Purpose:
 *
 *	A database user will implement this when it desires to 
 *	manage the URL's to redirect.
 *	This interface receives the results of IHXRedirectDBManager
 *	methods
 *
 *  IHXRedirectDBManagerResponse:
 *
 *	{00002A0B-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXRedirectDBManagerResponse,   0x00002A0B, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXRedirectDBManagerResponse

DECLARE_INTERFACE_(IHXRedirectDBManagerResponse, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)
    (
	THIS_
	REFIID			IIDOfInterfaceDesired,
	void**			ppVoidRequestedInterface
    ) PURE;

    STDMETHOD_(ULONG32,AddRef)
    (
	THIS
    ) PURE;

    STDMETHOD_(ULONG32,Release)
    (
	THIS
    ) PURE;

    /************************************************************************
     *	Method:
     *	    IHXRedirectDBManagerResponse::GetRedirectDone
     *	Purpose:
     *	    
     *	    Reports the success or failure of 
     *	    IHXRedirectDBManager::GetRedirect
     *	    If successful then the new URL is valid
     *
     */
    STDMETHOD(GetRedirectDone)
    (
	THIS_
	HX_RESULT		ResultStatus,
	IHXBuffer*		pBufferURL,
	IHXBuffer*		pBufferNewURL
    ) PURE;

    /************************************************************************
     *	Method:
     *	    IHXRedirectDBManagerResponse::AddRedirectDone
     *	Purpose:
     *	    
     *	    Reports the success or failure of 
     *	    IHXRedirectDBManager::AddRedirect
     *
     */
    STDMETHOD(AddRedirectDone)
    (
	THIS_
	HX_RESULT		ResultStatus,
	IHXBuffer*		pBufferURL
    ) PURE;

    /************************************************************************
     *	Method:
     *	    IHXRedirectDBManagerResponse::RemoveRedirectDone
     *	Purpose:
     *	    
     *	    Reports the success or failure of 
     *	    IHXRedirectDBManager::RemoveRedirect
     *
     */
    STDMETHOD(RemoveRedirectDone)
    (
	THIS_
	HX_RESULT		ResultStatus,
	IHXBuffer*		pBufferURL
    ) PURE;

};

/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXRegistrationLogger
 *
 *  Purpose:
 *
 *	A database plugin will implement this when it desires to provide 
 *	storage of player registration attempts.
 *
 *  IHXRegistrationLogger:
 *
 *	{00002A0E-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXRegistrationLogger,   0x00002A0E, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXRegistrationLogger

DECLARE_INTERFACE_(IHXRegistrationLogger, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)
    (
	THIS_
	REFIID			IIDOfInterfaceDesired,
	void**			ppVoidRequestedInterface
    ) PURE;

    STDMETHOD_(ULONG32,AddRef)
    (
	THIS
    ) PURE;

    STDMETHOD_(ULONG32,Release)
    (
	THIS
    ) PURE;

    /************************************************************************
     *	Method:
     *	    IHXRegistrationLogger::LogRegistrationAttempt
     *	Purpose:
     *	    
     *	    Call this to record the results of an attempt to register
     *	    a player's GUID.
     *
     */
    STDMETHOD(LogRegistrationAttempt)
    (
	THIS_
	IHXValues*		pValuesRegistration
    ) PURE;
};


#endif /* !_HXDB_H_ */
