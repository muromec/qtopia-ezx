/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: commreg.h,v 1.7 2006/01/31 23:35:08 ping Exp $
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

#ifndef _COMM_REG_H_
#define _COMM_REG_H_

#include "hxtypes.h"
#include "hxcom.h"
#include "hxmon.h"
#include "db_misc.h"

class Property;
class DB_implem;
class DB_node;
class CHXID;
class WatchList;
class Key;
class PropWatch;
class WListElem;

struct IHXBuffer;
struct IHXValues;


/*
 * 
 *  Class:
 *
 *	CommonRegistry
 *
 *  Purpose:
 *
 *     this is the common registry which is going to be shared by the client
 *  and the server. it acts like a bridge between the interface and the 
 *  implementation based on a model taken from the book 
 *  "design patterns" by gamma, helm, johnson and vlissides.
 * 
 */
class CommonRegistry
{
public:
			CommonRegistry();
			~CommonRegistry();

    UINT32 		AddComp(const char* pName);

    UINT32 		AddInt(const char* pName, const INT32 iValue);
    HX_RESULT 		GetInt(const char* pName, INT32* iValue) const;
    HX_RESULT 		GetInt(const UINT32 id, INT32* iValue) const;
    HX_RESULT 		SetInt(const char* pName, const INT32 iValue);
    HX_RESULT 		SetInt(const UINT32 id, const INT32 iValue);

    UINT32 		AddStr(const char* pName, IHXBuffer* pValue);
    HX_RESULT 		SetStr(const char* pName, IHXBuffer* pValue);
    HX_RESULT 		SetStr(const UINT32 id, IHXBuffer* pValue);
    HX_RESULT 		GetStr(const char* pName, 
			       REF(IHXBuffer*) pcValue) const;
    HX_RESULT 		GetStr(const UINT32 id, REF(IHXBuffer*) pcValue) const;

    UINT32 		AddBuf(const char* pName, IHXBuffer* pBuf);
    HX_RESULT 		GetBuf(const char* pName, IHXBuffer** ppBuf) const;
    HX_RESULT 		GetBuf(const UINT32 id, IHXBuffer** ppBuf) const;
    HX_RESULT 		SetBuf(const char* pName, IHXBuffer* pBuf);
    HX_RESULT 		SetBuf(const UINT32 id, IHXBuffer* pBuf);
    HX_RESULT		SetReadOnly(const char* pName, HXBOOL bValue);
    HX_RESULT		SetReadOnly(UINT32 ulRegId, HXBOOL bValue);

    UINT32 		AddIntRef(const char* pName, INT32* piValue);

    UINT32 		Del(const char* pName);
    UINT32 		Del(const UINT32 key);

    HXPropType 	GetType(const char* pName) const;
    HXPropType 	GetType(const UINT32 id) const;
    HX_RESULT		GetPropList(IHXValues*& pValues) const;
    HX_RESULT		GetPropList(const char* pName,
				    IHXValues*& pValues) const;
    HX_RESULT		GetPropList(const UINT32 id,
				    IHXValues*& pValues) const;
    HX_RESULT		Copy(const char* pFrom, const char* pTo);
    HX_RESULT		SetStringAccessAsBufferById(UINT32 ulId);

    UINT32		FindParentKey(const char* pName);
    UINT32		FindParentKey(const UINT32 id);
    INT32 		Count() const;
    INT32 		Count(const char* pName) const;
    INT32 		Count(const UINT32 id) const;

    HX_RESULT		GetPropName(const UINT32 ulId, 
				    IHXBuffer*& prop_name) const;
    UINT32		GetId(const char* prop_name) const;

    virtual HX_RESULT     AddDone(DB_implem* db_level, DB_node* new_node,
                                  DB_node* parent_node, Property* parent_prop);
    virtual HX_RESULT     SetDone(DB_node* new_node, Property* new_prop);
    virtual HX_RESULT     DeleteDone(DB_implem* db_level, DB_node* node,
                                     Property* prop);

    /*
     * Watch specific methods
     */
    UINT32		SetWatch(PropWatch* pPropWatch);
    UINT32		SetWatch(const char* pName, PropWatch* pPropWatch);
    UINT32		SetWatch(const UINT32 id, PropWatch* pPropWatch);
    HX_RESULT		SetTrickleWatch(const char* pParName, 
					const char* pTargetName,
				        PropWatch* pPropWatch);

    HX_RESULT		ClearWatch(IHXPropWatchResponse* pResonse=NULL);
    HX_RESULT		ClearWatch(const char* pName, IHXPropWatchResponse* pResonse=NULL);
    HX_RESULT		ClearWatch(const UINT32 id, IHXPropWatchResponse* pResonse=NULL);

    HX_RESULT		DeleteWatch(Property* p, WListElem* wle);
    HX_RESULT           m_LastError;
    
    void		Init(IUnknown* pContext);

protected:
    virtual DB_node*	_addComp(Key* k, char* key_str, DB_implem* ldb);
    virtual DB_node*	_addInt(Key* k, char* key_str, INT32 val, 
				DB_implem* ldb);
    virtual DB_node*	_addBuf(Key* k, char* key_str, IHXBuffer* buf, 
				DB_implem* ldb, 
				HXPropType val_type = PT_BUFFER);
    virtual DB_node*	_addIntRef(Key* k, char* key_str, INT32* val, 
				   DB_implem* ldb);

    virtual HX_RESULT	_clearWatch(IHXPropWatchResponse* pResonse);
    virtual HX_RESULT	_clearWatch(Property*, IHXPropWatchResponse* pResonse);
    virtual void	_dispatchCallbacks(DB_node*, Property*, DB_Event);
    virtual void 	_dispatchParentCallbacks(DB_implem*, DB_node*, 
						 DB_Event);
   
    virtual HX_RESULT	_find(DB_node**, Property**, const char*) const;

    virtual UINT32	_Del(DB_implem*, DB_node*, Property*, UINT32);
    virtual HX_RESULT	_del(DB_implem*);
    HX_RESULT		_getPropList(DB_implem*, IHXValues*& pValues) const;
    HX_RESULT		_setReadOnly(Property* pProp, HXBOOL bValue);
    UINT32		_buildSubstructure4Prop(const char* pFailurePoint,
			    const char* pProp);

    DB_implem*  		_logdb_imp;
    CHXID*			_ids;
    INT32			_count;		// num of elems in the DB
    
    WatchList*			m_pWatchList;
    int				m_lWatchCount;	// count num of Watches
    IUnknown*			m_pContext;
};

#endif // _COMM_REG_H_
