/* vi:set ts=8 sw=8:
 *
 * soundtest.h:	A quick program to test the sound device API.
 *
 * Leo L. Schwab					2003.07.08
 */
#ifndef __OASOUNDTEST_H
#define	__OASOUNDTEST_H

#ifndef __OPAPPVIEW_H
#include <OpApplication.h>
#endif
#ifndef __OPSOUND_H
#include <op_sound.h>
#endif

#include <stdio.h>


#define	CMD_TEST	31337
#define	CMD_BEEPDONE	31338
#define	CMD_PLAY	31339

struct WAVEHeader        
{
        char  ckID[4];             /* chunk id 'RIFF'            */
        U32 ckSize;              /* chunk size                 */
        char  wave_ckID[4];        /* wave chunk id 'WAVE'       */
        char  fmt_ckID[4];         /* format chunk id 'fmt '     */
        U32 fmt_ckSize;          /* format chunk size          */
        U16  formatTag;           /* format tag currently pcm   */
        U16  nChannels;           /* number of channels         */
        U32 nSamplesPerSec;      /* sample rate in hz          */
        U32 nAvgBytesPerSec;     /* average bytes per second   */
        U16  nBlockAlign;         /* number of bytes per sample */
        U16  nBitsPerSample;      /* number of bits in a sample */
        char  data_ckID[4];        /* data chunk id 'data'       */
        U32 data_ckSize;         /* length of data chunk       */
};


class OaSoundTest : public OpApplication
{
public:
	static OpApplication	*Create (void);

	OpCmdEvent		*fDoneEvent;
	void			beepdone (void);
	void			fillBuffer (void);
protected:
	virtual bool		onEvent (OpEvent &evt);
	virtual bool		onAppEvent (OpAppEvent::Msg msg,
				            OpEvent &evt);

private:
	/*  ctor/dtor are private; use Create().  */
				OaSoundTest (U16CPU flags = 0);
				OaSoundTest (const OaSoundTest &copy);
	virtual			~OaSoundTest ();

	void			init (void);
	void			uninit (void);

	void			testbeep (void);

	void			Play (void);
	void			Stop (void);
	void			Pause (void);
	void			Resume (void);
	void			writedata (void);
	void			Open(int nChannels, int nSamplesPerSec, int nBitsPerSample);
	void			Close();

	void			ReadBuf (void);

	op_sound_handle		*fSndDev;	// Device's audio channel, used for all calls to Openwave's API
	op_sound_buffer		fSndBuf[2];	// Sound sample bufer descriptor, used for writing data to the device
	//op_sound_buffer		fSndBuf2;	// Sound sample bufer descriptor, used for writing data to the device
	bool	m_bWriteDone;
	FILE *m_fp;
	WAVEHeader m_wh;
	int m_totalBytesRead;
	int m_totalBytesWritten;
	int m_nCurBuf;
	unsigned char *bigbuf;

	typedef OpApplication	INHERITED;

	enum PlayMode
	{
		kUninitializedMode,
		kInitializedMode,
		kPlayingMode,
		kPausedMode,
		kStoppedMode
	};
	PlayMode   fPlayMode;
	void setPlayMode(PlayMode mode);
	PlayMode getPlayMode() const { return fPlayMode; }
	void populateSoftKeys(PlayMode mode);
};

#endif	/*  __OASOUNDTEST_H  */
