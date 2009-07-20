/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxvsurf.h,v 1.6 2009/06/01 13:42:40 sfu Exp $
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

#ifndef _HXVSURF_H_
#define _HXVSURF_H_

/*
 * Forward declarations of some interfaces defined or used here-in.
 */
typedef _INTERFACE  IHXBuffer		IHXBuffer;
typedef _INTERFACE  IHXValues		IHXValues;

/****************************************************************************
 *
 *  Video Surface Data Structures and Constants
 */
typedef struct _HXBitmapInfoHeader
{
    UINT32  biSize;
    INT32   biWidth;
    INT32   biHeight;
    UINT16  biPlanes;
    UINT16  biBitCount;
    UINT32  biCompression;
    UINT32  biSizeImage;
    INT32   biXPelsPerMeter;
    INT32   biYPelsPerMeter;
    UINT32  biClrUsed;
    UINT32  biClrImportant;
    UINT32  rcolor;
    UINT32  gcolor;
    UINT32  bcolor;

} HXBitmapInfoHeader;

/*
 * HXBitmapInfo structure.
 */
typedef struct _HXBitmapInfo
{
    struct
    {
        UINT32  biSize;         /* use OFFSETOF(dwBitMask) here     */
        INT32   biWidth;        /* image width (in pixels)          */
        INT32   biHeight;       /* image height                     */
        UINT16  biPlanes;       /* # of bitplanes; always use 1     */
        UINT16  biBitCount;     /* average # bits/pixel             */
        UINT32  biCompression;  /* one of the RMA_... FOURCC codes  */
        UINT32  biSizeImage;    /* = width * height * bitCount / 8  */
        INT32   biXPelsPerMeter;/* always 0                         */
        INT32   biYPelsPerMeter;/* always 0                         */
        UINT32  biClrUsed;      /* !0, if 8-bit RGB; 0, otherwise   */
        UINT32  biClrImportant; /* !0, if 8-bit RGB; 0, otherwise   */

    } bmiHeader;

    union
    {
        UINT32  dwBitMask[3];   /* color masks (for BI_BITFIELDS)   */
        UINT32  dwPalette[256]; /* palette (for 8-bit RGB image)    */
    } un;

} HXBitmapInfo;

typedef UINT32  HX_COMPRESSION_TYPE;

/*
 * Structure for source inputs used in IHXVideoSurface2::ColorConvert
 */
typedef struct _tagSourceInputStruct
{
    UCHAR*  *aSrcInput;
    INT32   *aSrcPitch;
    INT32   nNumInputs;
} SourceInputStruct;

/*
 * Structure for alpha blending in IHXVideoSurface2
 */
typedef struct _tagAlphaStruct
{
    UCHAR*      pBuffer;        /* buffer for source image data     */
    UINT32      ulFourCC;       /* fourcc of pBuffer                */
    INT32       lPitch;         /* pitch of pBuffer                 */
    UINT32      ulImageWidth;   /* source image width               */
    UINT32      ulImageHeight;  /* source image height              */
    HXxRect     rcImageRect;    /* subrect of src image requested   */ 

} AlphaStruct;

/*
 * Structure for IHXVideoSurface2::GetVideoMem
 */
typedef struct _tagVideoMemStruct
{
    UCHAR*          pVidMem;    /* pointer to video memory          */
    INT32           lPitch;     /* pitch of pVidMem                 */
    HXBitmapInfoHeader  bmi;   /* desciption of pVidMem            */
    AlphaStruct*    pAlphaList; /* alpha blending list              */
    UINT32          ulCount;    /* alpha blending list length       */
} VideoMemStruct;

/*
 * Windows DIB formats & MKFOURCC() macro:
 */
#ifndef BI_RGB
#define BI_RGB          0L      /* RGB-8, 16, 24, or 32             */
#define BI_RLE8         1L      /* 8-bit RLE compressed image       */
#define BI_RLE4         2L      /* 4-bit RLE compressed image       */
#define BI_BITFIELDS    3L      /* RGB 555, 565, etc.               */
#endif
#ifndef MKFOURCC
#define MKFOURCC(c0,c1,c2,c3)   \
        ((UINT32)(BYTE)(c0) | ((UINT32)(BYTE)(c1) << 8) |   \
        ((UINT32)(BYTE)(c2) << 16) | ((UINT32)(BYTE)(c3) << 24))
#endif

/*
 * RMA image formats:
 */
#define HX_RGB         BI_RGB  /* Windows-compatible RGB formats:  */
#define HX_RLE8        BI_RLE8
#define HX_RLE4        BI_RLE4
#define HX_BITFIELDS   BI_BITFIELDS
#define HX_I420        MKFOURCC('I','4','2','0') /* planar YCrCb   */
#define HX_YV12        MKFOURCC('Y','V','1','2') /* planar YVU420  */
#define HX_YUY2        MKFOURCC('Y','U','Y','2') /* packed YUV422  */
#define HX_UYVY        MKFOURCC('U','Y','V','Y') /* packed YUV422  */
#define HX_YVU9        MKFOURCC('Y','V','U','9') /* Intel YVU9     */
#define HX_ARGB        MKFOURCC('A','R','G','B') /* ARGB32         */
#define HX_DVPF        MKFOURCC('D','V','P','F') /* dvpf           */
#define HX_NV21        MKFOURCC('N','V','2','1') /* Qualcomm YVU semi-planar */
#define HX_OMXV        MKFOURCC('O','M','X','V') /* OpenMax buffer header pointer */

/*
 * Non-standard FOURCC formats (these are just few aliases to what can be
 * represented by the standard formats, and they are left for backward
 * compatibility only).
 */
#define HXCOLOR_RGB3_ID     MKFOURCC('3','B','G','R') /* RGB-32 ??      */
#define HXCOLOR_RGB24_ID    MKFOURCC('B','G','R',' ') /* top-down RGB-24*/
#define HXCOLOR_RGB565_ID   MKFOURCC('6','B','G','R') /* RGB-16 565     */
#define HXCOLOR_RGB555_ID   MKFOURCC('5','B','G','R') /* RGB-16 555     */
#define HXCOLOR_8BIT_ID     MKFOURCC('T','I','B','8') /* RGB-8 w. pal-e */
#define HXCOLOR_YUV420_ID   MKFOURCC('2','V','U','Y') /* planar YCrCb   */
#define HXCOLOR_YUVA_ID     MKFOURCC('Y','U','V','A') /* planar YCrCb   */
#define HXCOLOR_YUV411_ID   MKFOURCC('1','V','U','Y') /* ???            */
#define HXCOLOR_YUVRAW_ID   MKFOURCC('R','V','U','Y') /* ???            */

/*
 * Flags for IHXVideoSurface2
 */

/* Flags for GetVideoMemory */
#define HX_WAIT_FOREVER    0       /* Do not return until a buffer is available */
#define HX_WAIT_NEVER      1       /* Return an error if no buffer is available */

/* Flags for Present */
#define HX_MODE_TIMED      0       /* Synchronize the frame to IHXRenderTimeLine */
#define HX_MODE_IMMEDIATE  1       /* Display the frame ASAP */
#define HX_MODE_REFRESH    (1<<1)  /* Refresh the current frame */


/****************************************************************************
 *
 *  Interface:
 *
 *  IHXVideoSurface
 *
 *  Purpose:
 *
 *  Interface for IHXVideoSurface objects.
 *
 *  IID_IHXVideoSurface:
 *
 *  {00002200-0901-11d1-8B06-00A024406D59}
 *
 */

DEFINE_GUID(IID_IHXVideoSurface, 0x00002200, 0x901, 0x11d1, 0x8b,
    0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXVideoSurface

DECLARE_INTERFACE_(IHXVideoSurface, IUnknown)
{
    /*
     * IUnknown methods
     */
    STDMETHOD(QueryInterface)   (THIS_
                REFIID riid,
                void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)  (THIS) PURE;

    STDMETHOD_(ULONG32,Release) (THIS) PURE;

    /*
     * IHXVideoSurface methods usually called by renderers to
     * Draw on the surface
     */
    STDMETHOD(Blt)      (THIS_
                UCHAR*          /*IN*/  pImageBits,
                HXBitmapInfoHeader*    /*IN*/  pBitmapInfo,
                REF(HXxRect)        /*IN*/  rDestRect,
                REF(HXxRect)        /*IN*/  rSrcRect) PURE;

    /************************************************************************
     *  Method:
     *      IHXVideoSurface::BeginOptimizedBlt
     *  Purpose:
     *      Called by renderer to commit to a bitmap format for all future
     *      OptimizedBlt calls.
     */
    STDMETHOD(BeginOptimizedBlt)(THIS_
                HXBitmapInfoHeader*    /*IN*/  pBitmapInfo) PURE;

    /************************************************************************
     *  Method:
     *      IHXVideoSurface::OptimizedBlt
     *  Purpose:
     *      Called by renderer to draw to the video surface, in the format
     *      previously specified by calling BeginOptimizedBlt.
     */
    STDMETHOD(OptimizedBlt) (THIS_
                UCHAR*          /*IN*/  pImageBits,
                REF(HXxRect)        /*IN*/  rDestRect,
                REF(HXxRect)        /*IN*/  rSrcRect) PURE;

    /************************************************************************
     *  Method:
     *      IHXVideoSurface::EndOptimizedBlt
     *  Purpose:
     *      Called by renderer allow the video surface to cleanup after all
     *      OptimizedBlt calls have been made.
     */
    STDMETHOD(EndOptimizedBlt)  (THIS) PURE;

    /************************************************************************
     *  Method:
     *      IHXVideoSurface::GetOptimizedFormat
     *  Purpose:
     *      Called by the client to find out what compression type the
     *      renderer committed to when it called BeginOptimizedBlt.
     */
    STDMETHOD(GetOptimizedFormat)(THIS_
                REF(HX_COMPRESSION_TYPE) /*OUT*/ ulType) PURE;

    /************************************************************************
     *  Method:
     *      IHXVideoSurface::GetPreferredFormat
     *  Purpose:
     *      Called by renderer to find out what compression type the video
     *      surface would prefer to be given in BeginOptimizedBlt.
     */
    STDMETHOD(GetPreferredFormat)(THIS_
                REF(HX_COMPRESSION_TYPE) /*OUT*/ ulType) PURE;
};

/****************************************************************************
 *
 *  Interface:
 *
 *  IHXVideoHook
 *
 *  Purpose:
 *
 *  Interface for IHXVideoHook objects.
 *
 *  IID_IHXVideoHook:
 *
 *  {00002202-0901-11d1-8B06-00A024406D59}
 *
 */

DEFINE_GUID(IID_IHXVideoHook, 0x00002202, 0x901, 0x11d1, 0x8b,
	    0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef	INTERFACE
#define INTERFACE   IHXVideoHook

DECLARE_INTERFACE_(IHXVideoHook, IUnknown)
{
    /*
     * IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				 REFIID riid,
				 void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /***
     * IHXVideoHook::OnVideoFrame
     ***
     * This method will be called from the drawing thread, with the current
     * frame, right before the frame is blitted to the screen. All 3rd party
     * calculations that occur here are delaying the timely delivery of the 
     * frame to the screen. They should be as optimized as possible.  pInFrame
     * is the input frame buffer pOutFrame should be set to contain the output
     * data on return, and destRect is provided so that the 3rd party can make
     * sense of mouse events, but cannot be changed and does not have any
     * direct bearing on image size.  srcRect is provided for inspection only
     * for compatibility with the possible future addition of unfixed size
     * input frames.  The client is responcible for maintaining and freeing
     * the memory which they return in pOutFrame.  Simply setting pOutFrame to
     * pInFrame is acceptable for in-place modifications as a result since it
     * will still only be freed the requisite one time.
     ***/

    STDMETHOD(OnVideoFrame)	(THIS_
				 UCHAR*		/* In */	pInFrame,
                                 ULONG32	/* In */	ulTimeStamp,
				 REF(HXxRect)	/* In */	srcRect,
				 REF(HXxRect)	/* In */	destRect,
				 REF(UCHAR*)	/* Out */	pOutFrame) PURE;

    /***
     * IHXVideoHook::HandleHookEvent
     ***
     * This method provides a quick pass through by which events received by 
     * the IHXVideoHookSink, such as mouse clicks, can be handed on to 
     * registered IHXVideoHook objects.  Hook objects can use or ignore this
     * information but setting the handled member of the passed HXxEvent is 
     * not allowed.
     ***/
    
    STDMETHOD(HandleHookEvent)	(THIS_
				 HXxEvent* pEvent) PURE;

    /***
     * IHXVideoHook::GetProperties
     ***
     * This method provides a way for holders of an IHXVideoHook object to 
     * query it for descriptive information.  Suggested property bindings are:
     *  "Description" - CString describing plugin functionality.
     *    "Copyright" - CString copyright notice.
     *     "MoreInfo" - CString URL for obtaining more info.
     *	    "Version" - ULONG32 version number.
     *  "InputFormat" - IHXBuffer containing input HXBitmapInfoHeader
     *                  flattened as UCHAR array.
     * "OutputFormat" - IHXBuffer containing output HXBitmapInfoHeader 
     *                  flattened as UCHAR array.
     * Others can obviously be included for specific hook-hook 
     * communications, and more may also be required.
     ***/
    
    STDMETHOD(GetProperties)	(THIS_
				 REF(IHXValues*)   /* Out */	properties) PURE;

    /***
     * IHXVideoHook::HookAddedNotification
     ***
     * This method is called by the IHXVideoHookSink for all of its 
     * registered hooks whenever it registers another.	 This allows 
     * registered hooks to redo their checking of the other hooks whenever
     * new ones enter the loop.
     ***/

    STDMETHOD(HookAddedNotification)	(THIS_
					 IHXVideoHook* pNewHook) PURE;
};

/****************************************************************************
 *
 *  Interface:
 *
 *  IHXVideoHookSink
 *
 *  Purpose:
 *
 *  Interface for IHXVideoHookSink objects.
 *
 *  IID_IHXVideoHookSink:
 *
 *  {00002201-0901-11d1-8B06-00A024406D59}
 *
 */

DEFINE_GUID(IID_IHXVideoHookSink, 0x00002201, 0x901, 0x11d1, 0x8b,
	    0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef	INTERFACE
#define INTERFACE   IHXVideoHookSink

DECLARE_INTERFACE_(IHXVideoHookSink, IUnknown)
{
    /*
     * IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				 REFIID riid,
				 void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /***
     * IHXVideoHookSink::AddVideoHook
     ***
     * This method is called by an IHXVideoHook object which wishes to 
     * connect to the receiving IHXVideoHookSink object to obtain video 
     * frames immediately prior to blitting.  pHook is the object requesting 
     * registry, and pOptions is an IHXValues object containing optional
     * special flags affecting behavior. pFrameFormat should point to a
     * pre-allocated HXBitmapInfoHeader struct which on exit is filled in
     * with the bitmap format in which frame data will be supplied to 
     * OnVideoFrame() and to which it will be expected to conform on exit of
     * OnVideoFrame().  If this format is inappropriate for input or output
     * formats, then after registration the IHXVideoHook object should use
     * the IHXVideoHookSink object's  SetInputFormat() and/or
     * SetOutputFormat() methods to arrive at a mutually acceptable exchange
     * format, or to determine that none is available and take appropriate
     * action.
     ***
     * pOptions:
     * "ReceivesEvents"		- 0:No,	!0:Yes
     * "ReceivesHookAdded"	- 0:No,	!0:Yes
     * "ReceivesFrames"		- 0:No,	!0:Yes
     * "TakesAnySize"		- 0:No,	!0:Yes
     ***/

    STDMETHOD(AddVideoHook) (THIS_
			     IHXVideoHook*	    /* In */    pHook,
			     IHXValues*	    /* I/O */   pOptions,
			     HXBitmapInfoHeader*   /* Out */   pFrameFormat) PURE;

    /***
     * IHXVideoHookSink::ForceSurfaceUpdate
     ***
     * This method provides a mechanism by which event-driven IHXVideoHook 
     * objects can request that they be given a new image if the mouse is
     * clicked (for example).  The entire frame is "damaged" so that 
     * everything will be redrawn.
     ***/

    STDMETHOD(ForceSurfaceUpdate)	(THIS) PURE;

    /***
     * IHXVideoHookSink::SetInputFormat
     * IHXVideoHookSink::SetOutputFormat
     ***
     * These methods provide a way for the IHXVideoHookSink to negotiate 
     * with its registered IHXVideoHook objects about what image formats 
     * they require or return respectively.	 pHook is the hook object
     * which is requesting a change in format, pRequested points to an 
     * HXBitmapInfoHeader describing the format which the hook wants (either
     * to receive or return).
     * This method returns HXR_FAIL and does not change formats, unless
     * pRequested matches an acceptable format exactly, that format is
     * registered for future frame transactions, and HXR_OK is returned.
     * Should be called immediately after registration.
     ***/

    STDMETHOD(SetInputFormat)	(THIS_
				 IHXVideoHook*		pHook,
				 HXBitmapInfoHeader*	pRequested) PURE;

    STDMETHOD(SetOutputFormat)	(THIS_
				 IHXVideoHook*		pHook,
				 HXBitmapInfoHeader*	pRequested) PURE;

    /***
     * IHXVideoHookSink::GetVideoHookCount
     ***
     * This method returns the number of IHXVideoHook objects registered 
     * with this IHXVideoHookSink.  This value is only useful in the short 
     * term as order and count may change for IHXVideoHook objects without
     * notice.  Do not use it to store access to a given hook.
     ***/

    STDMETHOD_(UINT16,GetVideoHookCount) (THIS) PURE;

    /***
     * IHXVideoHookSink::GetVideoHook
     ***
     * This method provides access to registered IHXVideoHook objects by 
     * index with valid indecies ranging from 0 to n-1 inclusive, where n
     * is the value returned by GetVideoHookCount(), provided no sources
     * have been added or removed since it was last called.
     ***/

    STDMETHOD(GetVideoHook)	(THIS_
				 UINT16 	nIndex,
				 REF(IUnknown*)	pUnknown) PURE;
};

// $Private:

/****************************************************************************
 *
 *  Interface:
 *
 *  IHXRenderTimeLine
 *
 *  Purpose:
 *
 *  Provide IHXVideoSurface2 a time line for scheduling its video frames
 *
 *  IID_IHXRenderTimeLine:
 *
 *  {00002204-0901-11d1-8B06-00A024406D59}
 *
 */

DEFINE_GUID(IID_IHXRenderTimeLine, 0x00002204, 0x901, 0x11d1, 0x8b,
    0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);
 
#undef  INTERFACE
#define INTERFACE   IHXRenderTimeLine

DECLARE_INTERFACE_(IHXRenderTimeLine, IUnknown)
{
    /*
     * IUnknown methods
     */
    STDMETHOD(QueryInterface)   (THIS_
                REFIID riid,
                void** ppvObj) PURE;
 
    STDMETHOD_(ULONG32,AddRef)  (THIS) PURE;
 
    STDMETHOD_(ULONG32,Release) (THIS) PURE;
 
 
    /************************************************************************
     *  Method:
     *      IHXRenderTimeLine::GetTimeLineValue
     *  Purpose:
     *      Get the current presentation time
     *
     *  Notes:
     *      returns HXR_TIMELINE_SUSPENDED when the time line is suspended
     */
    STDMETHOD (GetTimeLineValue) (THIS_ /*OUT*/ REF(UINT32) ulTime) PURE;

};

/****************************************************************************
 *
 *  Interface:
 *
 *  IHXVideoSurface2
 *
 *  Purpose:
 *
 *  Provides hardware video buffers to render plugins
 *
 *  IID_IHXVideoSurface2:
 *
 *  {00002203-0901-11d1-8B06-00A024406D59}
 *
 */

DEFINE_GUID(IID_IHXVideoSurface2, 0x00002203, 0x901, 0x11d1, 0x8b,
    0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);
 
#undef  INTERFACE
#define INTERFACE   IHXVideoSurface2

DECLARE_INTERFACE_(IHXVideoSurface2, IUnknown)
{
    /*
     * IUnknown methods
     */
    STDMETHOD(QueryInterface)   (THIS_
                REFIID riid,
                void** ppvObj) PURE;
 
    STDMETHOD_(ULONG32,AddRef)  (THIS) PURE;
 
    STDMETHOD_(ULONG32,Release) (THIS) PURE;
 
 
    /************************************************************************
     *  Method:
     *      IHXVideoSurface2::SetProperties
     *  Purpose:
     *      Request ulNumBuffers hardware buffers of the format described
     *      in bmi.  ulNumBuffers will contain the number of buffers actually
     *      allocated.
     *      pClock is the renderer's clock used for scheduling the video
     *      frames received in the Present call.  A NULL clock causes Present
     *      to ignore the timestamp and display synchronously.
     */
    STDMETHOD (SetProperties) (THIS_ /*IN/OUT*/ HXBitmapInfoHeader* bmi,
                                     /*IN/OUT*/ REF(UINT32) ulNumBuffers,
                                     /*IN*/     IHXRenderTimeLine* pClock) PURE;


    /************************************************************************
     *  Method:
     *      IHXVideoSurface2::GetVideoMem
     *  Purpose:
     *      Requests the next available hardware video buffer.
     *      If the presentation contains alpha blending, the blended regions
     *      of the will be requested via VideoMemStruct.pAlphaList.  Fill
     *      each AlphaStruct with the subrect of the video frame specified
     *      by AlphaStruct.rcImageRect.  IHXVideoSurface2 will then alpha
     *      blend the video with the presentation.
     *     
     *      This call blocks until a video buffer is available.  bmi 
     *      describes the buffer returned.
     *
     *  Notes:
     *      The buffer format can change with each call so bmi must be
     *      checked everytime.
     */
    STDMETHOD (GetVideoMem) (THIS_ VideoMemStruct* /*IN/OUT*/ pVidMem,
                                   UINT32 ulFlags) PURE;


    /************************************************************************
     *  Method:
     *      IHXVideoSurface2::ReleaseVideoMem
     *  Purpose:
     *      Called on a buffer returned from GetVideoMem to discard the
     *      buffer without displaying it.
     */
    STDMETHOD (ReleaseVideoMem) (THIS_ VideoMemStruct* pVidMem) PURE;

    /************************************************************************
     *  Method:
     *      IHXVideoSurface2::Present
     *  Purpose:
     *      Called by renderer to put a video image on the screen at absoulte
     *      time lTime.  prDestRect and prSrcRect are relative coordinates.
     *      prSrcRect with all zeros defaults to the entire surface dimensions
     *      and prDestRect with all zeros defaults to the entire client widnow.
     *
     *  Notes:
     *      This is an asynchronous method.
     */
    STDMETHOD (Present) (THIS_
                         VideoMemStruct* pVidMem,
                         INT32 lTime,
                         UINT32 ulFlags,
                         HXxRect *prDestRect,
                         HXxRect *prSrcRect) PURE;

    /************************************************************************
     *  Method:
     *      IHXVideoSurface2::ColorConvert
     *  Purpose:
     *      Called by renderer to transform one video format to another.
     *      It is optimized for writing to hardware memory.
     */
    STDMETHOD (ColorConvert) (THIS_
                              INT32 fourCCIn, 
                              HXxSize *pSrcSize,
                              HXxRect *prSrcRect,
                              SourceInputStruct *pInput,
                              INT32 fourCCOut,
                              UCHAR *pDestBuffer, 
                              HXxSize *pDestSize, 
                              HXxRect *prDestRect, 
                              int nDestPitch) PURE;

    
    /************************************************************************
     *  Method:
     *      IHXVideoSurface2::Flush
     *  Purpose:
     *      Cancel any pending Present calls.
     */
    STDMETHOD_(void, Flush)(THIS) PURE;

    /************************************************************************
     *  Method:
     *      IHXVideoSurface2::PresentIfReady
     *  Purpose:
     *      Optional method called by renderer to check the status of the
     *      next Present. If will display the next image if it is time.  This
     *      method assists the scheduler that is not getting its time slices
     *      due to OS latency, non-cooperative multi-tasking, or any other
     *      reason.
     */
    STDMETHOD (PresentIfReady) (THIS) PURE;
};
// $EndPrivate.

// $Private:

/****************************************************************************
 *
 *  Interface:
 *
 *  IHXSubRectVideoSurface
 *
 *  Purpose:
 *
 *  Interface for IHXSubRectVideoSurface objects.
 *
 *  IID_IHXSubRectVideoSurface:
 *
 *  {00002205-0901-11d1-8B06-00A024406D59}
 *
 */

DEFINE_GUID(IID_IHXSubRectVideoSurface, 0x00002205, 0x901, 0x11d1, 0x8b,
    0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXSubRectVideoSurface

DECLARE_INTERFACE_(IHXSubRectVideoSurface, IHXVideoSurface)
{
    STDMETHOD(BltSubRects)( THIS_
                            UCHAR*               /*IN*/  pImageBits,
                            HXBitmapInfoHeader* /*IN*/  pBitmapInfo,
                            HXxBoxRegion*          /*IN*/  pDestRects,
                            HXxBoxRegion*          /*IN*/  pSrcRects,
                            float                        fScaleFactorX,
                            float                        fScaleFactorY
                            ) PURE;
};
// $EndPrivate.

#include "hxcomptr.h"
DEFINE_SMART_PTR(IHXVideoSurface)
DEFINE_SMART_PTR(IHXVideoHook)
DEFINE_SMART_PTR(IHXVideoHookSink)
DEFINE_SMART_PTR(IHXRenderTimeLine)
DEFINE_SMART_PTR(IHXVideoSurface2)
DEFINE_SMART_PTR(IHXSubRectVideoSurface)

#endif /* _HXVSURF_H_ */
