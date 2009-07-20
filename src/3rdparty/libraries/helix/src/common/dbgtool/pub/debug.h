
#if !defined(DBGTOOL_DEBUG_H__)
#define DBGTOOL_DEBUG_H__

#if defined (_SYMBIAN)
//XXXLCM some of the stuff need not be Symbian specific
#include "platform/symbian/hxsym_dprintf.h"
#define PANIC(x) HX_ASSERT(false)
#else
// for now only on Symbian
inline void dprintfInit(...) { /* compile out*/}
#define DPRINTF_INIT(p) dprintfInit(p)

#include "platform/default/debug.h"
#endif

/*
 * DPRINTF flags for general/common use defined here
 *
 * top 16 bits reserved for other modules
 *	
 * DPRINTF(D_FOO, ("printf-format-string", arg1, arg2, ...));
 *
 */
#define D_ERROR		0x00000001	/* error condition */
#define D_INFO		0x00000002	/* general informative messages */
#define D_ENTRY		0x00000004	/* function entry/exit */
#define D_STATE		0x00000008	/* state table specific messages */
#define D_XFER		0x00000010	/* data input/output */
#define D_TIMEOUT	0x00000020	/* timeout specific */
#define D_OFFSET	0x00000040	/* when file offset changes */
#define D_STATS		0x00000080	/* statistics */
#define D_SELECT	0x00000100	/* select call returned */
#define D_FD		0x00000200	/* file descriptor accountng */
#define D_PROT		0x00000400	/* Protocol info */
#define D_ALLOC		0x00000800	/* Allocation */
#define D_ACCOUNT       0x00001000      /* accounting printfs */
#define D_LICENSE       0x00002000      /* licensing printfs */
#define D_PROF          0x00004000      /* profiling information */
#define D_MSG           0x00008000      /* */
#define D_REGISTRY      0x00010000      /* registry stuff */
/*
 * Debug Function flags. Set in the same way as the general debug flag
 * at runtime.
 */
#define DF_NO_CHALLENGE	0x00000001	/* don't respond to MD5 challenge */
#define DF_XX_CHALLENGE	0x00000002	/* return an invalid MD5 response */
#define DF_DROP_PACKETS 0x00000004      /* drop every 10th packet */
#define DF_NO_RESEND	0x00000008	/* don't do resend */

#endif //DBGTOOL_DEBUG_H__

