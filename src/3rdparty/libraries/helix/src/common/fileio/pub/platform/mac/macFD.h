/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: macFD.h,v 1.5 2004/07/09 18:20:14 hubbe Exp $
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

#define USE_MAC_FD 1

#if _MACINTOSH
#pragma once
#endif

#ifndef __MACFD__
#define __MACFD__

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */


#define	NBBY	8		/* number of bits in a byte */

/*
 * Select uses bit masks of file descriptors in longs.  These macros
 * manipulate such bit fields (the filesystem macros use chars).
 * FD_SETSIZE may be defined by the user, but the default here should
 * be enough for most uses.
 */
#define	FD_SETSIZE	1024

#ifndef howmany
#define howmany(x, y)	(((x)+((y)-1))/(y))
#endif

#ifndef _MAC_MACHO
typedef long	fd_mask;
#define NFDBITS	(sizeof(fd_mask) * NBBY)	/* bits per mask */

typedef	struct fd_set {
	fd_mask	fds_bits[howmany(FD_SETSIZE, NFDBITS)];
} fd_set;

/* local typedefs (used by creat) */
#ifndef mode_t
typedef unsigned long	mode_t;
#endif

#endif

#define 	FD_VALID(x) ((x >= 0) && (x < FD_SETSIZE)) ? 1 : 0

#define	FD_SET(n, p)	((p)->fds_bits[(n)/NFDBITS] |= (1 << ((n) % NFDBITS)))
#define	FD_CLR(n, p)	((p)->fds_bits[(n)/NFDBITS] &= ~(1 << ((n) % NFDBITS)))
#define	FD_ISSET(n, p)	((p)->fds_bits[(n)/NFDBITS] & (1 << ((n) % NFDBITS)))
#define	FD_ZERO(p)		bzero((char *)(p), sizeof(*(p)))

#define 	bzero(x,y)		::memset(x,0,y)


/*
 *	Mode values accessible to open()
 */
 
#ifdef __MWERKS__
#if  __MWERKS__ <0x2020 
#define O_RDWR			0x0			/* open the file in read/write mode */
#define O_RDONLY		0x1			/* open the file in read only mode */
#define O_WRONLY		0x2			/* open the file in write only mode */
#endif //  __MWERKS__ <0x2020 
#endif

#define O_APPEND		0x0100		/* open the file in append mode */
#define O_CREAT		0x0200		/* create the file if it doesn't exist */
#define O_EXCL			0x0400		/* if the file already exists don't create it again */
#define O_TRUNC		0x0800		/* truncate the file after opening it */
#define O_NRESOLVE	0x1000		/* Don't resolve any aliases */
#define O_ALIAS		0x2000		/* Open alias file (if the file is an alias) */
#define O_RSRC 		0x4000		/* Open the resource fork */
#define O_BINARY		0x8000		/* open the file in binary mode (default is text mode) */

// missing from <fcntl.h>
// missing from <unistd.h>
#define	O_ACCMODE	0x0003		/* mask for above modes */

#define SEEK_SET		0
#define SEEK_CUR		1
#define SEEK_END		2

enum
{
	FD_SOCKET = 1,
	FD_FILE
};

typedef struct FD
{
	void 		*owner;			// pointer to owning object
	int		available;		// == 1, the FD is available
	int		index;			// the actual file descriptor index used to reference this FD
	int		type;				// the type of object this FD represents (FD_SOCKET or FD_FILE)
	int		readReady;		// flag to indicate FD is ready for reading
	int		writeReady;		// flag to indicate that FD is ready for writing
	int		error;			// flag to indicate that FD is in an error state
} FD;


void init_fds(void);
int get_free_fd(void);
int free_fd(int fd);
void fd_register(int fd, void *owner, int type);
FD *get_fd_object(int fd);
int select(int width, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);
int close(int fd);
//int open(const char *path, int flags, mode_t mode);
#if UNIVERSAL_INTERFACES_VERSION < 0x0341
int read(int fd, void * buf, int len);
int write(int fd, const void * buf, int len);
int lseek(int fd, unsigned long off, int whence);
int unlink(char *filename);
#endif
void *fd_get_owner(int fd);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* __MACFD__ */




