#ifndef _HXFILERECOGNIZER_H_
#define _HXFILERECOGNIZER_H_

typedef _INTERFACE	IHXFileRecognizer		IHXFileRecognizer;
typedef _INTERFACE	IHXFileRecognizerResponse	IHXFileRecognizerResponse;

/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXFileRecognizer
 * 
 *  Purpose:
 * 
 *     Attemps to determine the MIME type of the given file. 
 * 
 *  IID_IHXFileRecognizer:
 * 
 *	{00000220-0901-11d1-8B06-00A024406D59}
 * 
 */

DEFINE_GUID(IID_IHXFileRecognizer, 0x00000220, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXFileRecognizer

#define CLSID_IHXFileRecognizer IID_IHXFileRecognizer

DECLARE_INTERFACE_(IHXFileRecognizer, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /*
     *	IHXFileRecognizer methods
     */

    /************************************************************************
     *	Method:
     *	    IHXFileRecognizer::GetMimeType
     *	Purpose:
     */
    STDMETHOD(GetMimeType) (THIS_
			    IHXFileObject* /*IN*/ pFile, 
			    IHXFileRecognizerResponse* /*IN*/ pFileRecognizerResponse
			    ) PURE;
};


/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXFileRecognizerResponse
 * 
 *  Purpose:
 * 
 *	Response interface for IHXFileRecognizer.
 *	Optional interface.
 * 
 *  IID_IHXFileRecognizerResponse:
 * 
 *	{00000221-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXFileRecognizerResponse, 0x00000221, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXFileRecognizerResponse

DECLARE_INTERFACE_(IHXFileRecognizerResponse, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /*
     *	IHXFileRecognizerResponse methods
     */

    /************************************************************************
     *	Method:
     *	    IHXFileRecognizerResponse::GetMimeTypeDone
     *	Purpose:
     *	    Notification interface provided by users of the IHXFileRecognizer
     *	    interface.  Note, you must copy the mimeType before this  
     *      method scope exits.
     *	    
     */
    STDMETHOD(GetMimeTypeDone) (THIS_
				HX_RESULT	status,
				IHXBuffer* pMimeType) PURE;
};

#endif // _HXFILERECOGNIZER_H_
