/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxgenericcontext.h,v 1.2 2007/07/06 21:58:19 jfinnecy Exp $
 * 
 * Portions Copyright (c) 1995-2006 RealNetworks, Inc. All Rights Reserved.
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

#ifndef _CRNGENERICCONTEXT_H_
#define _CRNGENERICCONTEXT_H_



#include "unkimp.h"
#include "ihxcontext.h"
#include "ihxcontextuser.h"
#include "hxslist.h"

_INTERFACE IHXCommonClassFactory;


#ifdef ENABLE_LOG_STREAMS
#include <fstream>
using namespace std;
#endif


/*!
    @class CRNGenericContext
*/
class CRNGenericContext : 
        public CUnknownIMP,
    	public IHXContext,
    	public IHXContextUser
{
public:

    DECLARE_UNKNOWN_NOCREATE( CRNGenericContext )
    DECLARE_COM_CREATE_FUNCS(CRNGenericContext )

    // IHXContext methods
    STDMETHOD ( AddObjectToContext ) ( THIS_ REFCLSID clsid );
    STDMETHOD ( GetService ) ( THIS_ REFIID iid, void** ppIService );

    // IHXContextUser methods
    STDMETHOD ( RegisterContext ) (THIS_ IUnknown* pSource );


#ifdef ENABLE_LOG_STREAMS
    friend ostream& operator <<( ostream&, const CRNGenericContext& );
#endif

protected:

    CRNGenericContext( void );
    virtual ~CRNGenericContext( void );

    HX_RESULT GetObjectSource( IHXCommonClassFactory** ppIObjectSource ) const;

private:

    CRNGenericContext( const CRNGenericContext& );
    CRNGenericContext& operator =( const CRNGenericContext& );

    IHXCommonClassFactory* m_pFactory;
    CHXSimpleList m_InnerUnks;		// IUnknown*
};

#endif // _CRNGENERICCONTEXT_H_


