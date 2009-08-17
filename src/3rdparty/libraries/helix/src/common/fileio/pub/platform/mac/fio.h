/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: fio.h,v 1.5 2004/07/09 18:20:14 hubbe Exp $
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

#ifndef _MACFIO_H_
#define _MACFIO_H_

#include <stdlib.h>

#include "bio.h"
#include "macFD.h"
#include "CMacFile.h"
#include "assert.h"
#ifdef _MAC_MACHO
#include <sys/stat.h>
#else
#include "stat.h"
#include "errno.h"
#include "oterrs.h"
//#define OTUNIXERRORS 1
//#include "OpenTransport.h" 
#include "MacTCP.h"
#endif

extern Boolean gFDInitialized;


class FileIO : public IO
{
public:
	       		FileIO(const char* file, LONG32 flags, 
			       mode_t mode = 0666);
			~FileIO();
    virtual LONG32	close();
    virtual LONG32	read(void* buf, LONG32 size);
    virtual LONG32	write(const void* buf, LONG32 size);
    virtual off_t	seek(off_t off, LONG32 whence);
    virtual LONG32	error();
    virtual LONG32	flags();
    virtual off_t	file_size();
    int		status(struct stat *st); //mac

    static void		local_path(char* path);
#ifdef _MAC_MACHO
    #define MAXPATH 1024
#else
    static const enum { MAXPATH = 1024 };
#endif
    static const char 	PATH_SEP;
    static int		is_directory(char* path);
    static const char* 	NEWLINE;

    static int UNIXError(OSErr theErr);

protected:
    int 	fd;
    LONG32	err;
    LONG32	_flags;
private:
    void		*mFile; // pointer to the actual Mac file object
};


#ifndef _CARBON
#define	ENFILE		23		/* Too many open files in system */
#define	ENOSPC		28		/* No space left on device */
#endif
#define	EROFS		30		/* Read-only file system */

inline int 
FileIO::UNIXError(OSErr theErr)
{
	int err;
	
	switch(theErr)
	{
		case noErr:
		case eofErr:
			err = 0;
			break;

		case permErr:
			err = EACCES;
			break;

		case fBsyErr:
			err = EBUSY;
			break;

		case dupFNErr:
			err = EEXIST;
			break;

		case ioErr:
			err = EIO;
			break;

		case tmfoErr:
			err = ENFILE;
			break;

		case fnfErr:
			err = ENOENT;
			break;

		case dskFulErr:
			err = ENOSPC;
			break;

		case wPrErr:
			err = EROFS;
			break;

#ifndef _MAC_MACHO
		case ipBadLapErr:
		case ipBadCnfgErr:
		case ipNoCnfgErr:
		case ipLoadErr:
		case ipBadAddr:
			err = ENXIO;			/* device not configured */	/* a cheap cop out */
			break;
			
		case connectionClosing:
			err = ESHUTDOWN;		/* Can't send after socket shutdown */
			break;
			
		case connectionExists:
			err = EISCONN;		/* Socket is already connected */
			break;
			
		case connectionTerminated:
			err = ENOTCONN;		/* Connection reset by peer */  /* one of many possible */
			break;
			
		case openFailed:
			err = ECONNREFUSED;	/* Connection refused */
			break;
			
		case duplicateSocket:	/* technically, duplicate port */
			err = EADDRINUSE;		/* Address already in use */
			break;
			
		case ipDestDeadErr:
			err = EHOSTDOWN;		/* Host is down */
			break;
			
		case ipRouteErr:
			err = EHOSTUNREACH;	/* No route to host */
			break;
#endif
			
		default:
			err = EINVAL;			/* cop out; an internal err, unix err, or no err */
			break;
	}

	return err;
}

inline
FileIO::FileIO(const char* file, LONG32 flags, mode_t mode) 
{
	if (gFDInitialized == FALSE)
	{
		init_fds();
	}

	mFile = NULL;
	fd = -1;
	
	// create a new CMacFile object
	CMacFile *theFile = new CMacFile;
	
	OSErr theErr = noErr;
	
 	// get a file descriptor and register the CMacFile object as its owner
 	if(theFile)
	{
		fd = get_free_fd();
		assert(fd != -1);
		
		if(fd != -1)
		{
			fd_register(fd, (void *)theFile,FD_FILE);
		}
	}
	
	if(theFile && fd != -1)
		theErr = theFile->Open(file, flags);
	
	if(!theErr && theFile && fd != -1 && flags & O_RDONLY)
	{	
		theFile->set_buffered_read(TRUE);
	}
	
	if(!theErr && theFile && fd != -1)
	{
		_flags = flags;

		mFile = theFile;
	}
	
	if(theErr)
	{
		free_fd(fd);
		if(mFile)
			delete mFile;
		mFile = NULL;
		fd = -1;
	}
		
	err = UNIXError(theErr);
}

inline LONG32
FileIO::close() 
{
	if(fd >= 0)
	{
		free_fd(fd);	
		fd = -1;
	}
	return 0;
}

inline
FileIO::~FileIO() 
{
	if(mFile)
	{
		CMacFile *temp = (CMacFile *)mFile;
		delete temp;
	}

	if (fd >= 0) 
		close();
		
	fd = -1;
}

inline off_t
FileIO::seek(off_t off, LONG32 whence) 
{
	CMacFile *temp = (CMacFile *)mFile;
	
	if(fd == -1 || temp == NULL)
	{
		err = EINVAL;
		return -1;
	}
	
	OSErr theErr = temp->Seek(off,whence);
	
	err = UNIXError(theErr);
	
	return (theErr != 0 ? (off_t) -1 : off);
}

inline LONG32
FileIO::read(void * buf, LONG32 len) 
{
	CMacFile *temp = (CMacFile *)mFile;
	
	if(fd == -1 || temp == NULL)
	{
		err = EINVAL;
		return -1;
	}

	INT32 actualLen = 0;
	OSErr theErr = temp->Read((char *)buf,len,&actualLen);
	
	err = UNIXError(theErr);
	
	return actualLen;
}

inline LONG32
FileIO::write(const void * buf, LONG32 len) 
{
	CMacFile *temp = (CMacFile *)mFile;
	
	if(fd == -1 || temp == NULL)
	{
		err = EINVAL;
		return -1;
	}

	INT32 actualLen = 0;
	OSErr theErr = temp->Write((char *)buf,len,&actualLen);
	
	err = UNIXError(theErr);
	
	return actualLen;
}


// Note: status only implements file size (st_size)
inline int
FileIO::status(struct stat* st) {

	CMacFile *temp = (CMacFile *)mFile;
	
	if(fd == -1 || temp == NULL)
	{
		err = EINVAL;
		return -1;
	}

	long theSize;
        OSErr theErr = temp->FileSize(&theSize);
        st->st_size = theSize;
	
	err = UNIXError(theErr);
	
	return err; 
}

inline off_t
FileIO::file_size()
{
    struct stat st;
    if (status(&st) < 0)
    {
	return (off_t)-1;
    }
    return st.st_size;
}

inline LONG32
FileIO::error()
{
    return err;
}

inline LONG32
FileIO::flags()
{
    return _flags;
}

inline void
FileIO::local_path(char* path)
{
}

inline int
FileIO::is_directory(char* path)
{
#ifdef UNIMPLEMENTED
    struct _stat st;

    if (_stat(path, &st) < 0)
    {
	return 0;
    }

    return st.st_mode & S_IFDIR;
#endif
	return 0;
}

#endif /* _MACFIO_H_ */
