/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxbufdataf.cpp,v 1.5 2006/02/07 19:21:12 ping Exp $
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

#include "hlxclib/sys/types.h"
#include "hlxclib/sys/stat.h"
#include "hlxclib/fcntl.h"
#include "hlxclib/errno.h"

#include "hxtypes.h"
#include "hxcom.h"
#include "hxresult.h"
#include "ihxpckts.h"
#include "pckunpck.h"
#include "hxbuffer.h"
#include "debug.h"

#include "hxbufdataf.h"

#include "op_fs.h"

/////////////////////////////////////////////////////////////////////////
//
//  Method:
//      HXBufferedDataFile::QueryInterface
//  Purpose:
//      Implement this to export the interfaces supported by your
//      object.
//
STDMETHODIMP
HXBufferedDataFile::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IHXDataFile))
    {
        AddRef();
        *ppvObj = (IHXDataFile*)this;
        return HXR_OK;
    }
    
    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}   

/////////////////////////////////////////////////////////////////////////
//
//  Method:
//      HXBufferedDataFile::AddRef
//  Purpose:
//      Everyone usually implements this the same... feel free to use
//      this implementation.
//
STDMETHODIMP_(ULONG32)
HXBufferedDataFile::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}   

/////////////////////////////////////////////////////////////////////////
//
//  Method:
//      HXBufferedDataFile::Release
//  Purpose:
//      Everyone usually implements this the same... feel free to use
//      this implementation.
//
STDMETHODIMP_(ULONG32)
HXBufferedDataFile::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }
    
    delete this;
    return 0;
}   

HXBufferedDataFile::HXBufferedDataFile(IUnknown* pContext)
    : m_lRefCount(0),
      m_LastError(0),
      m_pFileName(0),
      m_Fd(-1),
      m_Flags(0),
      m_Begin(0),
      m_BufSize(0),
      m_BufFill(0),
      m_Offset(0),
      m_FileOffset(0),
      m_FlushSize(0),
      m_pBuf(0),
      m_Dirty(0)
{
    m_pContext = pContext;
    HX_ADDREF(m_pContext);

    CreateBufferCCF(m_pFileName, m_pContext);
    m_BufSize = GetPageSize();
}

HXBufferedDataFile::~HXBufferedDataFile()
{
    Close();
    FreeBuf();
    HX_RELEASE(m_pFileName);
    m_pFileName = 0;
}
	
/////////////////////////////////////////////////////////////////////////
//
// Method:
//     HXBufferedDataFile::Bind:
// Purpose:
//     Cache the file name for opening.
//     If file already open, close it.
//
STDMETHODIMP_(void)
HXBufferedDataFile::Bind(const char* pFileName)
{
    if (m_Fd >= 0)
	Close();

    m_pFileName->Set((BYTE *)pFileName, ::strlen(pFileName)+1);
}

/////////////////////////////////////////////////////////////////////////
//
// Method:
//     HXBufferedDataFile::Create
// Purpose:
//     Creates a datafile using the specified mode
//     uOpenMode - File Open mode - HX_FILEFLAG_READ/HX_FILEFLAG_WRITE/HX_FILEFLAG_BINARY
//
STDMETHODIMP
HXBufferedDataFile::Create(UINT16 uOpenMode)
{
    return HXR_NOTIMPL;
}

/////////////////////////////////////////////////////////////////////////
//
//  Method:
//      HXBufferedDatatFile::Open
//  Purpose:
//      Open for buffered i/o.  If file is already open, ignore this
//      open and return HXR_OK status.  Flags can have 3 possible values
//	
//	   HX_FILEFLAG_READ:     Open file read only
//
//	   HX_FILEFLAG_WRITE:    Open file for writing.  If file does not
//			     exists, it is created.
//
//	   HX_FILEFLAG_READ|HX_FILEFLAG_WRITE:                
//                           Open file for both read and writing.
//
//  Note:
//      HX_FILEFLAG_WRITE implies both reads and writes to the file.  It
//      also implies the file can be randomly updated any where in the
//      file as well as appended onto the end. If the file exists,
//      then the file contents are buffered in before updating.  As a
//      result, writing implies both system reads and writes.
//
//      If open succeeds, returns HXR_OK, otherwise HXR_FAIL.
//
STDMETHODIMP
HXBufferedDataFile::Open(UINT16 flags)
{
				// flags don't match close and reopen
    if (m_Fd >= 0 && flags != m_Flags)
    {
	Close();
    }
    int status = HXR_OK;
    if (m_Fd < 0)
    {
	int oflags = 0;

	m_Flags = flags;
	m_LastError = 0;
	if (m_Flags & HX_FILEFLAG_WRITE)
	{
	    oflags = (O_CREAT|O_RDWR);
	    if (!(m_Flags & HX_FILEFLAG_NOTRUNC))
	    {
		oflags |= O_TRUNC;
	    }
	}
	else if (m_Flags & HX_FILEFLAG_READ)
	    oflags = O_RDONLY;
	else
	    return HXR_FAIL;

	if ((m_Fd =
	     //::open((const char*) m_pFileName->GetBuffer(), oflags, 0644)) < 0)
             OpFsOpen((const char*) m_pFileName->GetBuffer(), oflags, 0)) == kOpFsErrAny)
	{
	    status = HXR_FAIL;
	    m_LastError = errno;
	}
	else			// open ok: initialize
	{
	    if (m_pBuf == 0)
		AllocBuf();
	    m_Begin = 0;
	    m_BufFill = 0;
	    m_Offset = 0;
	    m_FileOffset = 0;
	    m_FlushSize = 0;
	    m_LastError = 0;
	    m_Dirty = 0;
	}
    }
    return status;
}

/////////////////////////////////////////////////////////////////////////
//
// Method:
//     HXBufferedDataFile::Close:
//  Purpose:
//     Flush the file buffer (if open of writing and close the file.
//     Note that file is not closed when there are multiple references
//     to this object.
//
STDMETHODIMP
HXBufferedDataFile::Close()
{
    int status = HXR_OK;
    if (m_Fd >= 0 && m_lRefCount <= 1)
    {
	if (m_Flags & HX_FILEFLAG_WRITE)
	    FlushBuf();
	//if (::close(m_Fd) == -1)
	if (OpFsClose( m_Fd ) != kOpFsErrOk )
	{
	    status = HXR_FAIL;
	    m_LastError = errno;
	}
	m_Fd = -1;
    }
    return status;
}

/////////////////////////////////////////////////////////////////////////
//
// Method:
//     HXBufferedDataFile::Name
// Purpose:
//     Name returns the currently bound file name in FileName.
//     and returns TRUE, if the a name has been bound.  Otherwise
//     FALSE is returned.
//
STDMETHODIMP_(HXBOOL)
HXBufferedDataFile::Name(REF(IHXBuffer*) pFileName)
{
    if (m_pFileName && m_pFileName->GetSize())
    {
	pFileName = m_pFileName;
	pFileName->AddRef();
	return TRUE;
    }
    return FALSE;
}

/////////////////////////////////////////////////////////////////////////
//
// Method:
//     HXBufferedDataFile::IsOpen()
// Purpose:
//     IsOpen returns TRUE if file is open.  Otherwise FALSE.
//
HXBOOL
HXBufferedDataFile::IsOpen()
{
    return (m_Fd >= 0 ? TRUE : FALSE);
}

/////////////////////////////////////////////////////////////////////////
//
// Method
//     HXBufferedDataFile::Seek:
// Purpose:
//     Just move the seek offset according to the value of whense.
//     This only sets the local idea of the current offset, no system
//     level seek is performed.  If seek is ok, the current value of
//     seek offset is returned.  Otherwise -1 is returned.
//
//     XXXBH:  24-Mar-99
//       Note that most of callers expect HXR_OK as return value, not
//       the current offset.  So the above comment is a noop.   Also
//       the offset is passed in as an unsigned, it should be signed to
//       allow seeks in both directions.  Right now we do the ugly cast.
//
STDMETHODIMP
HXBufferedDataFile::Seek(ULONG32 offset, UINT16 whense)
{
    ULONG32 size = 0;
    LONG32 signed_offset = (LONG32) offset;

    if (whense == SEEK_SET && signed_offset >= 0)
	m_Offset = signed_offset;
    else if (whense == SEEK_CUR && (LONG32) m_Offset + signed_offset >= 0)
	m_Offset += signed_offset;
    else if (whense == SEEK_END &&
	     (LONG32) (size = LogicalSize()) + signed_offset >= 0)
	m_Offset = size + signed_offset;
    else
	return HXR_FAIL;

    return HXR_OK;
}

/////////////////////////////////////////////////////////////////////////
//
// Method:
//     HXBufferedDataFile::Tell()
// Purpose:
//      Tell returns the current (logical) file position in the file
//
STDMETHODIMP_(ULONG32)
HXBufferedDataFile::Tell()
{
    return m_Offset;
}

/////////////////////////////////////////////////////////////////////////
//
// Method:
//     HXBufferedDataFile::Read:
// Purpose
//     Read nbytes into the supplied buffer.  Values returned:
//
//        Actual bytes read.  The smaller of the number bytes requested
//        and  the number of bytes between the current seek offset and the
//        end of file is returned.
//
//        0, if the current seek pointer is at or beyond the end of file.
//      
//        -1, if errors occure.  The LastError() member function returns
//        the errno of the last system request.
//
STDMETHODIMP_(ULONG32)
HXBufferedDataFile::Read(REF(IHXBuffer *) pBuf, ULONG32 count)
{
    if (m_Fd >= 0)
    {
	pBuf->AddRef();

	ULONG32 nleft = count;
	INT32 rval = 0;
	unsigned char* buf = pBuf->GetBuffer();
	while (nleft > 0)
	{
				// If we enter this block, m_Offset
				// lies within the mapped buffer.
	    if (m_Begin <= m_Offset && m_Offset < m_Begin + m_BufFill ||
		(m_Offset < FlushSize() && (rval = NewBuf()) > 0))
	    {
		UINT32 off = m_Offset - m_Begin;  // offset within buffer
						 // and how much to copy
		UINT32 ncopy = m_Begin + m_BufFill - m_Offset;
		if (nleft < ncopy)
		    ncopy = nleft;
		::memcpy((void*) buf, (const void*) (m_pBuf+off), ncopy); /* Flawfinder: ignore */
		nleft -= ncopy;
		m_Offset += ncopy;
		buf += ncopy;
	    }
				// NewBuf() could not make the above
				// assertion, true.  m_Offset is either
				// at or beyond EOF, or an error occured.
	    else
		break;
	}
	pBuf->Release();
	return rval >= 0 ? count - nleft : HXR_FAIL;
    }
    return HXR_FAIL;
}

/////////////////////////////////////////////////////////////////////////
//
// Method:
//     HXBufferedDataFile::Write:
// Purpose:
//      Write the requested number of bytes.  The number of bytes
//      written are return.  If errors occur, -1 is returned.  The
//      LastError() member function returns the value of errno from
//      the last system request.
//
STDMETHODIMP_(ULONG32)
HXBufferedDataFile::Write(REF(IHXBuffer *) pBuf)
{
    if (m_Fd >= 0)
    {
	pBuf->AddRef();

	ULONG32 count = pBuf->GetSize();
	ULONG32 nleft = count;
	INT32 rval = 0;
	const unsigned char* buf = pBuf->GetBuffer();
	while (nleft > 0)
	{
				// If we enter this block, m_Offset
				// lies within the current buffer.
	    if (m_Begin <= m_Offset && m_Offset < m_Begin + m_BufSize ||
		(rval = NewBuf()) >= 0)
	    {
		UINT32 off = m_Offset - m_Begin;  // offset within buffer
						  // and how much to copy
		UINT32 ncopy = m_Begin + m_BufSize - m_Offset;
		if (nleft < ncopy)
		    ncopy = nleft;
		::memcpy((void*) (m_pBuf+off), (const void*) buf, ncopy); /* Flawfinder: ignore */
		m_Dirty = 1;
		nleft -= ncopy;
		m_Offset += ncopy;
		buf += ncopy;
				// data extend beyond what was read in
		if (m_Offset-m_Begin > m_BufFill)
		    m_BufFill = m_Offset-m_Begin;
	    }
				// NewBuf() could not make the above
				// assertion, true because an error occured.
	    else
		break;
	}
	pBuf->Release();
	return rval >= 0 ? count - nleft : HXR_FAIL;
    }
    return HXR_FAIL;
}

/////////////////////////////////////////////////////////////////////////
//
// Method:
//     HXBufferedDataFile::Flush()
// Purpose:
//     Flush out the data in case of buffered I/O
//  
STDMETHODIMP
HXBufferedDataFile::Flush()
{
    if (m_Flags & HX_FILEFLAG_WRITE)
	return FlushBuf() > 0 ? HXR_OK : HXR_FAIL;

    return HXR_OK;
}

/////////////////////////////////////////////////////////////////////////
//
// Method:
//     HXBufferedDataFile::Stat
// Purpose:
//     Return info about the data file such as permissions, time of
//     creation size in bytes, etc.  Note that if this method is
//     called with unflushed extensions to the file, the st_size field
//     will not be correct.  We could Flush before filling in the stat
//     data, but this is a const function.  The other alternative is
//     to set the st.st_size field to the value returned by
//     LogicalSize() (which includes unflushed extensions to the
//     file).
//
STDMETHODIMP
HXBufferedDataFile::Stat(struct stat* buf)
{
    return GetFileStat(buf);
}

STDMETHODIMP_(INT16)
HXBufferedDataFile::GetFd()
{   
    return m_Fd;
}
	
/////////////////////////////////////////////////////////////////////////
//
// Method:
//     HXBufferedDataFile::GetLastError:
// Purpose:
//     Returns the value of errno from the last system request.
//
STDMETHODIMP
HXBufferedDataFile::GetLastError()
{
    return m_LastError;
}

STDMETHODIMP_(void) 
HXBufferedDataFile::GetLastError(REF(IHXBuffer*) err)
{
    char* str = ::strerror(m_LastError);
    CreateAndSetBuffer(err, (BYTE*) str, ::strlen(str)+1, m_pContext);
}

/////////////////////////////////////////////////////////////////////////
//
//  Method:
//      HXBufferedDataFile::FlushSize():
//  Purpose:
//       Return flushed file size.  We assume that only this object
//       has changed the size of the file.  The size returned does NOT
//       include bytes in the buffer not yet flushed to the file.
//
ULONG32
HXBufferedDataFile::FlushSize()
{
    if (m_FlushSize == 0)
    {
	struct stat st;
	if (GetFileStat(&st) == HXR_OK)
	    m_FlushSize = st.st_size;
    }
    return m_FlushSize;
}

/////////////////////////////////////////////////////////////////////////
//
//  Method:
//      HXBufferedDataFile::LogicalSize():
//  Purpose:
//       Return logical file size.  We assume that only this object
//       has changed the size of the file.  The size returned includes
//       bytes in the buffer not yet flushed to the file.
//
ULONG32
HXBufferedDataFile::LogicalSize()
{
				// If the flushed size is <= the
				// beginning of the current buffer,
				// we have extended the file, but
				// have not flushed it.
    ULONG32 size = FlushSize();
    if (size <= m_Begin)
    {
	size = m_Begin + m_BufFill;
    }
    return size;
}

/////////////////////////////////////////////////////////////////////////
//
// Method:
//     HXBufferedDataFile::NewBuf:
//  Purpose:
//      Utility method.
//      Request an new i/o buffer.  If the file is open for writing,
//      the buffer is flushed if marked dirty.  If the file is open
//      for reading a new buffer containing the current seek offset is
//      read in from the file.  Note for files open for writing, the
//      current buffer is flushed, before the new buffer is read in.
//      Returns vales:
//
//         1       Buffer was successfully refreshed.
//         0       If file is open for reading, cureent seek offset
//                 is at or beyond EOF.
//        -1       Error occured during flushing or filling.
//
INT32 HXBufferedDataFile::NewBuf()
{
    int status = 1;
				// if writing flush the current buffer 
    if (m_Flags & HX_FILEFLAG_WRITE)
    {
	status = FlushBuf();
    }
				// fill buffer from file
    if (status > 0)
    {
	status = FillBuf();
    }
    return status;
}

/////////////////////////////////////////////////////////////////////////
//
// Method:
//     HXBufferedDataFile::FillBuf:
//
// Purpose   
//      Utility method:
//      Fill buffer with file contents at current offset.
//      Returns:
//         1   Buffer filled.  If End of Buffer is beyond EOF,
//             buffer is partially filled.  m_BufFill is set
//             to how much of buffer is filled.
//         0   End of File
//        -1   Read or seek error, or file not open
//
INT32
HXBufferedDataFile::FillBuf()
{
    int status = -1;
    m_LastError = 0;
    if (m_Fd >= 0)
    {
				// Get beginning of buffer on a
				// m_BufSize boundry.  If the beginning
				// of the buffer is less then the current
				// flushed file size, seek and read it in.
	m_Begin = (m_Offset/m_BufSize)*m_BufSize;
	m_BufFill = 0;
	if (m_Begin < FlushSize())
	{
	    INT32 rval = -1;
	    m_BufFill = 0;
	    if ((rval = Pread((void*) m_pBuf, m_BufSize, m_Begin)) > 0)
	    {
		m_BufFill = rval;
		status = 1;
	    }
	    else if (rval < 0)	// lseek or read error
	    {
		m_LastError = errno;
		status = -1;
	    }
	    else		// EOF: zero buffer
	    {
		::memset((void*) m_pBuf, 0, m_BufSize);
		status = 0;
	    }
	}
	else			// m_Begin is beyond EOF: zero buffer
	{
	    ::memset((void*) m_pBuf, 0, m_BufSize);
	    status = 0;
	}
    }
    return status;
}

/////////////////////////////////////////////////////////////////////////
//
// Method:
//     HXBufferedDataFile::FlushBuf:
// Purpose:
//    Utility method:
//    If file is open for writing and the current i/o buffer is dirty,
//    write the contents to the file.
//    Returns:
//         1   Buffer Flushed.
//        -1   Read or seek error, or file not open or read only.
//
INT32
HXBufferedDataFile::FlushBuf()
{
    int status = -1;
    m_LastError = 0;
    if (m_Fd >= 0 && (m_Flags & HX_FILEFLAG_WRITE))
    {
				// buffer has been scribbled on.
	if (m_Dirty && m_BufFill > 0)
	{
	    int rval = -1;
	    if ((rval = Pwrite((const void*) m_pBuf, m_BufFill, m_Begin)) > 0)
	    {
		status = 1;
		m_Dirty = 0;
	    }
	    else
	    {
		status = -1;
		m_LastError = errno;
	    }
	}
	else			// already flushed.
	    status = 1;
    }
    return status;
}

/////////////////////////////////////////////////////////////////////////
//
// Method:
//     HXBufferedDataFile::AllocBuf:
// Purpose:
//     Allocate  the i/o buffer off of heap.
//
void
HXBufferedDataFile::AllocBuf()
{
    if (m_pBuf == 0)
    {
	m_pBuf = new char[m_BufSize];
    }
}

/////////////////////////////////////////////////////////////////////////
//
// Method:
//     HXBufferedDataFile::FreeBuf:
//  Purpose:
//     Free the i/o buffer.
//
void
HXBufferedDataFile::FreeBuf()
{
    delete [] m_pBuf;
    m_pBuf = 0;
    m_BufFill = 0;
    m_Begin = 0;
}

/////////////////////////////////////////////////////////////////////////
//
// Method:
//     HXBufferedDataFile::Pread:
// Purpose:
//     Read data at requrested position.
//     Number of bytes read is returned 0 if at EOF, or -1 on error
//
INT32
HXBufferedDataFile::Pread(void* buf, INT32 nbytes, ULONG32 offset)
{
    //INT32 nread = -1;
	OpFsSize nread = kOpFsErrAny;
    m_LastError = 0;
    if (m_Fd >= 0)
    {
	if (m_FileOffset == offset ||
	   // ::lseek(m_Fd, offset, SEEK_SET) != -1)
	   OpFsSeek( m_Fd, (OpFsSize)offset, SEEK_SET) != kOpFsErrAny)
	{
	    m_FileOffset = offset;
	    //if ((nread = ::read(m_Fd, buf, nbytes)) > 0)
		if ((nread = OpFsRead( m_Fd, buf, nbytes)) != kOpFsErrAny)
			m_FileOffset += nread;
	    else if (nread < 0)
			m_LastError = errno;
	}
    }
    return nread;
}

/////////////////////////////////////////////////////////////////////////
//
// Method:
//     HXBufferedDataFile::Pwrite:
// Purpose:
//     Write data at requrested position.
//     Number of bytes read is returned, or -1 on error
//
INT32
HXBufferedDataFile::Pwrite(const void* buf, INT32 nbytes, ULONG32 offset)
{
    //INT32 nwrite = -1;
	OpFsSize nwrite = kOpFsErrAny;
    m_LastError = 0;
    if (m_Fd >= 0)
    {
	if (m_FileOffset == offset ||
	    //::lseek(m_Fd, offset, SEEK_SET) != -1)
		OpFsSeek( m_Fd, (OpFsSize)offset, SEEK_SET) != kOpFsErrAny)
	{
	    m_FileOffset = offset;
	    //if ((nwrite = ::write(m_Fd, buf, nbytes)) > 0)
		if (( nwrite = OpFsWrite( m_Fd, buf, nbytes)) != kOpFsErrAny)
	    {
			m_FileOffset += nwrite;
			if (m_FileOffset > m_FlushSize)
				m_FlushSize = m_FileOffset;
	    }
	    else
			m_LastError = errno;
	}
    }
    return nwrite;
}

/////////////////////////////////////////////////////////////////////////
//
// Method:
//     HXBufferedDataFile::GetFileStat
//  Purpose
//   Get file stat. If all is of HXR_OK is returned otherwise HXR_FAIL
//
INT32
HXBufferedDataFile::GetFileStat(struct stat* st) const
{
    INT32 status = HXR_FAIL;
    if ((m_Fd >= 0 && ::fstat(m_Fd, st) == 0) ||
	(m_pFileName->GetSize() &&
	 ::stat((const char*) m_pFileName->GetBuffer(), st) == 0))
    {
	status = HXR_OK;
    }
				// cast to get around const
    ((HXBufferedDataFile*) this)->m_LastError = status == HXR_OK ? 0 : errno;
    return status;
}

/////////////////////////////////////////////////////////////////////////
//
// Method:
//     HXBufferedDataFile::GetPageSize:
// Purpose:
//    Get the system page size.

INT32
HXBufferedDataFile::GetPageSize() const
{
#if defined(_FREEBSD)
    return getpagesize();
#else
    //return ::sysconf(_SC_PAGE_SIZE);
	return 0;  // On Openwave platform, pages service not available
#endif
}

