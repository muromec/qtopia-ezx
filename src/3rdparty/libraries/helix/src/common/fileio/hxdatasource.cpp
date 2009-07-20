/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxdatasource.cpp,v 1.8 2008/03/14 18:28:31 gajia Exp $
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

#include "hxcom.h"
#include "hxtypes.h"
#include "hxresult.h"
#include "hxdatasource.h"
#include "hxassert.h"
#include "hxtlogutil.h"
#include "hlxclib/sys/stat.h"
#include "hlxclib/time.h"

CHXDataSource::CHXDataSource():
    m_ulRefCount(0)
    ,m_ulPosition(0)
    ,m_ulLastError(HXR_OK)
    ,m_pRequest(NULL)
    ,m_pHXBuffer(NULL)
    ,m_lLastFileObjectIndex( -1 )  //enable multi read/write
{
}

STDMETHODIMP
CHXDataSource::QueryInterface(REFIID riid, void** ppvObj)
{
    QInterfaceList qiList[] =
    {
        { GET_IIDHANDLE(IID_IUnknown), (IUnknown*)(IHXMMFDataSource*)this },
        { GET_IIDHANDLE(IID_IHXMMFDataSource), (IHXMMFDataSource*) this },
    };

    return ::QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}


STDMETHODIMP_(ULONG32)
CHXDataSource::AddRef(void)
{
    return InterlockedIncrement(&m_ulRefCount);
}

STDMETHODIMP_(ULONG32)
CHXDataSource::Release(void)
{
    HX_ASSERT(m_ulRefCount > 0);
    INT32 rc = InterlockedDecrement(&m_ulRefCount);
    if (rc == 0)
    {
        delete this;
    }
    return rc;
}


CHXDataSource::~CHXDataSource()
{
    m_FileObjectList.Close();
    m_FileObjectPosition.Close();
    // m_pRequest weak ref.
}

STDMETHODIMP CHXDataSource::Open(IHXRequest *pRequest,
                                        const char *modeStr, IHXMMFDataSourceObserver* pObserver)
{
    HXLOGL4(HXLOG_FILE, "CHXDataSource::Open");
    HX_RESULT hxr = HXR_OK;
    if(pRequest && modeStr)
    {
        m_pRequest = pRequest;
        Seek(SEEK_SET, 0);
    }
    else
    {
        return HXR_INVALID_PARAMETER;
    }

    return hxr;
}

STDMETHODIMP CHXDataSource::Seek(
            UINT32 offset, INT32 fromWhere)
{
    HX_RESULT hxr = HXR_OK;
    if(fromWhere == SEEK_SET)
    {
        m_ulPosition = offset;
    }
    else if(fromWhere == SEEK_CUR)
    {
        m_ulPosition += offset;
    }
    else if(fromWhere == SEEK_END)
    {
        hxr = HXR_NOT_SUPPORTED;
    }
    else
    {
        hxr = HXR_INVALID_PARAMETER;
    }
        
    return hxr;
}

STDMETHODIMP CHXDataSource::Stat(struct stat *buf)
{
    HX_RESULT hxr = HXR_OK;
    if(buf)
    {
        // st_size provides the correct size that can be used by datatypes.
        // rest of the stat buffer has arbitrary values.

        UINT32 ulSize = 0;
        GetSize(ulSize);
        buf->st_size  = ulSize;

        time_t cur_time, time_of_yesterday;
        time(&cur_time);
        time_of_yesterday = cur_time - 24 * 3600;
        
        buf->st_ctime = time_of_yesterday;
        buf->st_atime = cur_time;
        buf->st_mtime = time_of_yesterday;
        buf->st_mode  = S_IRUSR;
    } 
    else
    {
        hxr = HXR_INVALID_PARAMETER;
    }
    return hxr;
}

STDMETHODIMP CHXDataSource::GetStringAttribute(
                        UINT32 ulAttribute, REF(IHXBuffer*)  pBuffer)
{
    return HXR_FAIL;
}

STDMETHODIMP CHXDataSource::GetAttribute(INT32 ulAttribute,
                                INT32 &ulValue)

{
    return HXR_FAIL;
}

STDMETHODIMP CHXDataSource::GetLastError()
{
    return m_ulLastError;
}

//enable multi read/write
STDMETHODIMP CHXDataSource::Open2( IHXRequest *pRequest, const char *mode, IHXMMFDataSourceObserver* pObserver, IHXFileObject* pFileObject )
{
    HX_RESULT retVal( HXR_FAIL );
    TInt index( 0 );
    if( FindFileObjectIndex( index, pFileObject ) != HXR_OK )
    {
        IHXMMFDataSourceObserver* pObserver = NULL;
        
        pFileObject->QueryInterface(IID_IHXMMFDataSourceObserver, (void **) &pObserver);
        
        if(pObserver != NULL)
        {
            retVal = Open( pRequest, mode, pObserver );
            if( retVal == HXR_OK )
            {
                m_lLastFileObjectIndex = m_FileObjectList.Count();
                m_FileObjectList.Append( pFileObject );
                m_FileObjectPosition.Append( 0 );
            }
            HX_RELEASE(pObserver);
        }
    }
    else
    {
        HXLOGL1(HXLOG_FILE, "CHXDataSource::Open FileObject has been opened earlier, fail" );
    }
    return retVal;
}

STDMETHODIMP CHXDataSource::Seek2( UINT32 offset, INT32 fromWhere, IHXFileObject* pFileObject )
{
    HX_RESULT retVal = RestoreFileObject( pFileObject );

    if( retVal == HXR_OK )
    {
        retVal = Seek( offset, fromWhere );
    }
    else
    {
        HXLOGL1(HXLOG_FILE, "CHXDataSource::Seek RestoreFileObject() failed" );
    }

    if( retVal == HXR_OK )
    {
        retVal = UpdateFileObject( m_lLastFileObjectIndex, m_ulPosition );
    }
    
    return retVal;
}
 
UINT32 CHXDataSource::Read2( IHXBuffer* pBuffer, IHXFileObject* pFileObject )
{
    m_pHXBuffer = pBuffer;

    UCHAR*  pBuf( NULL );
    ULONG32 size( 0 );
    ULONG32 count = sizeof( UCHAR );

    if( pBuffer )
    {
        pBuf = pBuffer->GetBuffer();
        size = pBuffer->GetSize();
    }

    HX_RESULT retVal = RestoreFileObject( pFileObject );

    UINT32 bytesread( 0 );
    if( retVal == HXR_OK )
    {
        bytesread = Read( pBuf, size, count );
        retVal = UpdateFileObject( m_lLastFileObjectIndex, m_ulPosition );
    }
    else
    {
        HXLOGL1(HXLOG_FILE, "CHXDataSource::Read RestoreFileObject() failed" );
    }

    return bytesread;
}

UINT32 CHXDataSource::Write2( void *pBuf, ULONG32 size, ULONG32 count, IHXFileObject* pFileObject )
{
    HX_RESULT retVal = RestoreFileObject( pFileObject );

    UINT32 byteswrite( 0 );
    if( retVal == HXR_OK )
    {
        byteswrite = Write( pBuf, size, count );
        retVal = UpdateFileObject( m_lLastFileObjectIndex, m_ulPosition );
    }
    else
    {
        HXLOGL1(HXLOG_FILE, "CHXDataSource::Write RestoreFileObject() failed" );
    }
    
    return byteswrite;
}

HX_RESULT CHXDataSource::Close2( IHXFileObject* pFileObject )
{
    HX_RESULT retVal = RestoreFileObject( pFileObject );

    if( retVal == HXR_OK )
    {
        retVal = Close();
        if( retVal == HXR_OK )
        {
            m_FileObjectList.Remove( m_lLastFileObjectIndex );
            m_FileObjectList.Compress();
            m_FileObjectPosition.Remove( m_lLastFileObjectIndex );
            m_FileObjectPosition.Compress();
            m_lLastFileObjectIndex = -1;
        }
    }
    else
    {
        HXLOGL1(HXLOG_FILE, "CHXDataSource::Close RestoreFileObject() failed" );
    }


    return retVal;
}

STDMETHODIMP CHXDataSource::GetLastError2( IHXFileObject* pFileObject )
{
    return m_ulLastError;
}

HX_RESULT CHXDataSource::RestoreFileObject( IHXFileObject* pFileObject )
{
    HX_RESULT retVal( HXR_OK );
    
    IHXFileObject* lastFileObject( NULL );
    
    if( m_lLastFileObjectIndex < m_FileObjectList.Count() && m_lLastFileObjectIndex >= 0 )
    {
        lastFileObject = m_FileObjectList[ m_lLastFileObjectIndex ];
    }

    if( lastFileObject != pFileObject )
    {
        retVal = FindFileObjectIndex( m_lLastFileObjectIndex, pFileObject );
        
        if( retVal == HXR_OK )
        {
            TUint position = m_FileObjectPosition[ m_lLastFileObjectIndex ];
            retVal = Seek( position, SEEK_SET );  //recover to earlier location
        }
    }

    return retVal;
}

HX_RESULT CHXDataSource::UpdateFileObject( TInt index, UINT32 position )
{
    HX_RESULT retVal( HXR_FAIL );
    if( index < m_FileObjectPosition.Count() && index >= 0 )
    {
        m_FileObjectPosition[ index ] = position;
        retVal = HXR_OK;
    }
    
    return retVal;
}

HX_RESULT CHXDataSource::FindFileObjectIndex( TInt& index, const IHXFileObject* pFileObject ) const
{
    HX_RESULT retVal( HXR_ELEMENT_NOT_FOUND );

    TInt count = m_FileObjectList.Count();
    for( TInt i = 0; i < count; i++ )
    {
        const IHXFileObject* entry = m_FileObjectList[ i ];
        if( entry == pFileObject )
        {
            retVal = HXR_OK;
            index = i;
            break;
        }
    }
    return retVal;
}
HXBOOL CHXDataSource::AsyncReadSupported()
{
    return FALSE;
}

