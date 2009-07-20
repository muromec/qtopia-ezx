/*
 *   This routine should contain RTSP related utility functions.
 */


#ifndef _RTSPMISC_H_
#define _RTSPMISC_H_

HXBOOL proxiesSupportFlash4(IHXBuffer* pViaStr);
void AddNoCacheHeader(IUnknown* pContext, IHXRequest* pRequest);

#endif // _RTSPMISC_H_
