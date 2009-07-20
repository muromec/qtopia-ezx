/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: db.h,v 1.6 2006/02/07 20:49:12 ping Exp $
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

#ifndef _DB_H_
#define	_DB_H_

#define	MAX_FILENAME	1024
#define	DISK_FREE_LOW	  10

/* Key/data structure -- a Data-Base Thang. */
typedef struct {
    void	*data;			/* data */
    size_t	 size;			/* data length */
} DBT;

/* Routine flags. */
#define	R_CURSOR	    1		/* del, put, seq */
#define	__R_UNUSED	    2		/* UNUSED */
#define	R_FIRST		    3		/* seq */
#define	R_IAFTER	    4		/* put (RECNO) */
#define	R_IBEFORE	    5		/* put (RECNO) */
#define	R_LAST		    6		/* seq (BTREE, RECNO) */
#define	R_NEXT		    7		/* seq */
#define	R_NOOVERWRITE	    8		/* put */
#define	R_PREV		    9		/* seq (BTREE, RECNO) */
#define	R_SETCURSOR	    10		/* put (RECNO) */
#define	R_RECNOSYNC	    11		/* sync (RECNO) */

typedef enum { DB_BTREE, DB_HASH, DB_RECNO } DBTYPE;
typedef enum { DB_CREATE = 1, DB_TRUNCATE = 2, DB_NOOVERWRITE = 3 } DBMODE;
typedef enum { DB_READ, DB_WRITE, DB_RDWR } DBPERM;
typedef enum { DB_OK = 0, DB_KEYEXIST = 1 } DBERRORS;
typedef enum { DB_NOTFOUND = 1 } DBSEQERRORS;
typedef enum { DB_PUT_SUCCESS = 0, DB_PUT_ERROR = -1, DB_PUT_NOOVERWRITE = 1 } DBPUTERRORS;

typedef struct _DB_INFO {
        UINT32  ulSize;
} DB_INFO, *PDB_INFO;

/* Access method description structure. */
#undef __P
#define	__P(protos)	protos		/* full-blown ANSI C */

typedef struct __db {
	int (*close)	(struct __db *);
	int (*del)	(const struct __db *, const DBT *, UINT32);
	int (*get)	(IUnknown*, const struct __db *, const DBT *, DBT *, UINT32);
	int (*put)	(const struct __db *, DBT *, const DBT *, UINT32);
	int (*seq)	(IUnknown*, const struct __db *, DBT *, DBT *, UINT32);
	int (*sync)	(const struct __db *, UINT32);
	int (*fd)	(const struct __db *);

#define FSDB
#ifdef  FSDB
# ifndef _WIN32
#  define HANDLE int
# endif // _WIN32
	HANDLE    hMutex;
	char*     pDir;
#endif //FSDB

} DB;

#ifdef __cplusplus
extern "C"
{
#endif

DB *dbopen (IUnknown*, const char *, int, int, DBTYPE, DB_INFO *);

#ifdef __cplusplus
}
#endif

typedef UINT32*  DB_TXN;


#if defined (_WINDOWS ) || defined (_WIN32)
# define OS_SEPARATOR_CHAR       '\\'
# define OS_SEPARATOR_STRING     "\\"
#elif defined (_UNIX)
# define OS_SEPARATOR_CHAR       '/'
# define OS_SEPARATOR_STRING     "/"
#elif defined (_MACINTOSH)
#ifdef _MAC_MACHO
# define OS_SEPARATOR_CHAR       '/'
# define OS_SEPARATOR_STRING     "/"
#else
# define OS_SEPARATOR_CHAR       ':'
# define OS_SEPARATOR_STRING     ":"
#endif
#elif defined (_OPENWAVE)
# define OS_SEPARATOR_CHAR       '/'
# define OS_SEPARATOR_STRING     "/"
#endif // defined (_WINDOWS ) || defined (WIN32)

#endif /* !_DB_H_ */
