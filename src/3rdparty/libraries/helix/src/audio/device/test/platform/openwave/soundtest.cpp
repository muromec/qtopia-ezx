/* vi:set ts=8 sw=8:
 *
 * soundtest.cpp:	A quick program to test the sound device API.
 *
 * Leo L. Schwab					2003.07.08
 */
#include <op_types.h>
#include <op_math.h>
#include <OpApplication.h>

#include <op_sound.h>

#include "soundtest.h"

static const int BLOCK_SIZE = 7040; //176400;   // ~ 1s of audio

enum Cmds
{
	kCmdPlay = 1,
	kCmdPause,
	kCmdStop,
};


/***************************************************************************
 * Prototypes.
 */
static void
beepdone_callback (
	op_sound_handle const	*handle,
	S32			msg,
	op_sound_buffer const	*sndbuf,
	void			*data
);

/***************************************************************************
 * Constructors/Destructors.
 */
OaSoundTest::OaSoundTest (U16CPU flags) :
	fSndDev (NULL),
	OpApplication (flags),
	m_bWriteDone(true),
	m_fp(NULL),
	m_totalBytesRead(0),
	m_totalBytesWritten(0),
	bigbuf(NULL)
{
	for (int i=0; i<2; i++)
	{
		fSndBuf[i].fSampleBuffer = NULL;
		fSndBuf[i].fNSamples	= 0;
		fSndBuf[i].fUserData	= NULL;
		fSndBuf[i].fFlags	= OP_SNDBUFF_CALLBACK_ON_IODONE;
	}
}

OaSoundTest::~OaSoundTest (void)
{
	for (int i=0; i<2; i++)
	    if (fSndBuf[i].fSampleBuffer) 
		free (fSndBuf[i].fSampleBuffer);
}

OpApplication *
OaSoundTest::Create (void)
{
	return (new OaSoundTest);
}


/***************************************************************************
 * Event handlers.
 */
bool
OaSoundTest::onEvent (OpEvent &evt)
{
	if (evt.isCmdEvent()) {
		int cmd = OpCmdEvent::GetCmd (&evt);

		if (cmd == CMD_TEST) {
			testbeep ();
			return (true);
		} else if (cmd == CMD_BEEPDONE) {
			beepdone ();
			return (true);
		} else if (cmd == kCmdPlay) {
			if (this->getPlayMode() == kPausedMode)
			{
				Resume();
			}
			else
			{
				Play();
			}
			this->setPlayMode(kPlayingMode);
			return (true);
		} else if (cmd == kCmdPause) {
			this->setPlayMode(kPausedMode);
			Pause();
			return (true);
		} else if (cmd == kCmdStop) {
			this->setPlayMode(kStoppedMode);
			Stop();
			return (true);
		}
	}
	return (this->INHERITED::onEvent (evt));
}

bool
OaSoundTest::onAppEvent (OpAppEvent::Msg msg, OpEvent &evt)
{
	switch (msg) {
	case OpAppEvent::kInit:
		init ();
		return (true);

	case OpAppEvent::kRequestQuit:
		uninit ();
		/*  Fall-through  */
	default:
		break;
	}
	return (this->INHERITED::onAppEvent (msg, evt));
}


/***************************************************************************
 * What this whole mess is for.
 */


void
OaSoundTest::testbeep (void)
{
	int	atomsize, nchannels, cyclelength, bufsize;
	OpFixed	ftmp;
	U32	i;
	U8	*bufptr;
	op_sound_buffer* sndbuf = &(fSndBuf[0]);

	for (i=0; i<2; i++)
	{
	    if (fSndBuf[i].fSampleBuffer) 
	    {
		free (fSndBuf[i].fSampleBuffer);
		fSndBuf[i].fSampleBuffer = NULL;
	    }
	}

	/*
	 * Build sinewave buffer.
	 */
	cyclelength = 44100 / 440;
	U32 fNSamples = cyclelength * 100;  //10,000
	atomsize = 1;
	nchannels = 2;
	bufsize = fNSamples * atomsize * nchannels; //20,000

	if (!(sndbuf->fSampleBuffer = (void*) malloc (bufsize)))
		return;

	U8* fSampleBuf = (U8*) sndbuf->fSampleBuffer;

	for (bufptr = fSampleBuf, i = 0;  i < fNSamples;  i++) {
		ftmp = OpFixedSin (2 * kFixedPI * i / cyclelength)
		     + kFixed1 - 1;
		if (ftmp < 0)
			ftmp++;
		*bufptr++ = ftmp >> 9;

		ftmp = OpFixedCos (2 * kFixedPI * i / cyclelength)
		     + kFixed1 - 1;
		if (ftmp < 0)
			ftmp++;
		*bufptr++ = ftmp >> 9;
	}

	Open(2, 44100, 8);				// Open and start the channel
	sndbuf->fNSamples = fNSamples;	// Set the number of samples in the buffer
	op_sound_write (fSndDev, sndbuf);
}

void
OaSoundTest::Play (void)
{
	FILE *fp;

	fp = fopen("c:\\Downloads\\Music\\Regrets.wav", "rb");

	if ( fp != NULL )
	{
		m_fp = fp;
		m_bWriteDone = false;
		m_totalBytesRead = 0;
		m_totalBytesWritten = 0;
		
		// Read in the PCM header
		int nbytes;
		unsigned char buf[sizeof(struct WAVEHeader)];
		if ((nbytes = fread(buf, sizeof(char), sizeof(struct WAVEHeader), fp)) > 0 ) 
		{
			struct WAVEHeader* wh = (struct WAVEHeader *)buf;
			m_wh = (*wh);

			Open(m_wh.nChannels, m_wh.nSamplesPerSec, m_wh.nBitsPerSample);
		}
		else
			return;

		// Delete old buf, and allocate a new one
		for (int i=0; i<2; i++)
		{
		    if (fSndBuf[i].fSampleBuffer) 
		    {
			free (fSndBuf[i].fSampleBuffer);
			fSndBuf[i].fSampleBuffer = NULL;
		    }
		    if (!(fSndBuf[i].fSampleBuffer = (void *) malloc (BLOCK_SIZE)))
			return;
		}


		if (bigbuf) 
			free(bigbuf);
		if (! (bigbuf = (unsigned char *) malloc( m_wh.data_ckSize )) )
			return;

		ReadBuf();	// Read in the whole buffer (test callback timing)

		m_nCurBuf = 0;
		fillBuffer();  // Read the from the file into the write buffer
		fillBuffer();  // double buffering!!!
	}
}


void 
OaSoundTest::ReadBuf()
{
	unsigned char buf[1024];
	int nbytes;

	while ((nbytes = fread(buf, sizeof(char), 1024, m_fp)) > 0 && m_totalBytesRead < (int)m_wh.data_ckSize) 
	{
		memcpy(bigbuf + m_totalBytesRead, buf, nbytes);
		m_totalBytesRead += nbytes;
	}
	fclose(m_fp);
	m_fp = NULL;
	
}
/*

int 
OaSoundTest::GetBuf(unsigned char* buf)
{
	unsigned char tmp[BLOCK_SIZE];
	int nbytes = 0;

	if (m_fp != NULL && (nbytes = fread(tmp, sizeof(char), BLOCK_SIZE, m_fp)) > 0)
	{
		memcpy(buf, tmp, nbytes);
		m_totalBytesRead += nbytes;
	}
	else if (m_fp != NULL)
	{
		fclose(m_fp);
	}
	return nbytes;
}
*/

void
OaSoundTest::fillBuffer()
{
	op_sound_buffer* sndbuf;	
	sndbuf = &fSndBuf[m_nCurBuf];
	m_nCurBuf = !m_nCurBuf;
	
	if (!m_bWriteDone && m_totalBytesRead > m_totalBytesWritten && bigbuf != NULL) 
	{
		int nbytes = BLOCK_SIZE;
		if ((m_totalBytesRead - m_totalBytesWritten) < BLOCK_SIZE)
			nbytes = m_totalBytesRead - m_totalBytesWritten;
		sndbuf->fNSamples = (nbytes) / (m_wh.nChannels * (m_wh.nBitsPerSample/8));
		memcpy(sndbuf->fSampleBuffer, bigbuf + m_totalBytesWritten, nbytes);
		op_sound_write (fSndDev, sndbuf);
		m_totalBytesWritten += nbytes;
	}
	else
	{	if (bigbuf) 
			free(bigbuf);
		bigbuf = NULL;
		for (int i=0; i<2; i++)
		{
		    if (fSndBuf[i].fSampleBuffer) 
		    {
			free (fSndBuf[i].fSampleBuffer);
			fSndBuf[i].fSampleBuffer = NULL;
		    }
		}
		m_bWriteDone = true;
		//this->setPlayMode(kStoppedMode);
	}

	/*
	int nbytes;

	if (m_fp != NULL && (nbytes = fread(fSndBuf.fSampleBuffer, sizeof(char), BLOCK_SIZE, m_fp)) > 0 ) 
	{
		fSndBuf.fNSamples = (nbytes) / (m_wh.nChannels * (m_wh.nBitsPerSample/8));
		writedata();
	}

	if (nbytes < BLOCK_SIZE)
	{
		if (m_fp != NULL)
		{
			fclose(m_fp);
			m_fp = NULL;
		}
		m_bWriteDone = true;
	}
	*/
}

void
OaSoundTest::Stop (void)
{
	op_sound_unregister_callback(fSndDev, beepdone_callback);
	op_sound_stop(fSndDev);

	if (m_fp != NULL)
	{
		fclose(m_fp);
		m_fp = NULL;
	}
	m_bWriteDone = true;
}

void
OaSoundTest::Pause (void)
{
	op_sound_unregister_callback(fSndDev, beepdone_callback);
	op_sound_pause(fSndDev);
}

void
OaSoundTest::Resume (void)
{
	op_sound_register_callback(fSndDev, beepdone_callback, this);
	fillBuffer();
	fillBuffer();
	op_sound_resume(fSndDev);
}

void
OaSoundTest::beepdone (void)
{
	if ( !m_bWriteDone )
	{
		fillBuffer();	// more data to write
	}
	else
	{
	//	this->setPlayMode(kStoppedMode);
//		Close(); // freechan will hang after IODONE
	}

}

static void
beepdone_callback (
op_sound_handle const	*handle,
S32			msg,
op_sound_buffer const	*sndbuf,
void			*data
)
{
	//OaSoundTest::beepdone();
	OaSoundTest *app = (OaSoundTest *) data;
	app->fillBuffer();

	//app->fDoneEvent->send();	// Send the event synchronously to its sink. 
	// However, unlike post(), the caller is still responsible for ownership of the event object.
	//
	//app->fDoneEvent->post(); // only one callback will work with this event
	//app->fDoneEvent = NULL;
}


/***************************************************************************
 * Initialization/teardown.
 */
void
OaSoundTest::init (void)
{
	//  Build the (ha ha) UI.  
	setTitle ("Sound Test");

	this->setPlayMode(kInitializedMode);

	//  Create the event that will be sent when the sound is done.  
	fDoneEvent = new OpCmdEvent (CMD_BEEPDONE, this->getID());

	// CheckFormat?
}

void
OaSoundTest::Open(int nChannels, int nSamplesPerSec, int nBitsPerSample)
{
	//////////////////////////////
	// open the channel
	//////////////////////////////
	
	OpError	retval;

	if (fSndDev) {
		Close();	// Close last channel
	}

	//  Procure channels.  
	if (!(fSndDev = op_sound_allocchan (nChannels)))
		return;

	op_sound_pcm_format pcmfmt = OP_PCM_FMT_INVALID;
	switch (nBitsPerSample)
	{
	case 8 : pcmfmt = OP_PCM_FMT_U8;		break;
	case 16: pcmfmt = OP_PCM_FMT_U16_LE;	break;
	}

	op_sound_pcm_rate pcmrate = OP_PCM_RATE_44100;
	switch (nSamplesPerSec)
	{
	case 8000 : pcmrate = OP_PCM_RATE_8000;		break;
	case 11025: pcmrate = OP_PCM_RATE_11025;	break;
	case 16000: pcmrate = OP_PCM_RATE_16000;	break;
	case 22050: pcmrate = OP_PCM_RATE_22050;	break;
	case 32000: pcmrate = OP_PCM_RATE_32000;	break;
	case 44100: pcmrate = OP_PCM_RATE_44100;	break;
	case 48000: pcmrate = OP_PCM_RATE_48000;	break;
	case 64000: pcmrate = OP_PCM_RATE_64000;	break;
	}

	//  Configure channels for playback.  
	retval = op_sound_set_params_args
	          (fSndDev,
	           OP_AUDIOTAG_VOLUME, 0xFFFF,
	           OP_AUDIOTAG_PCMFORMAT, pcmfmt,
	           //OP_AUDIOTAG_PCMRATE_EXPLICIT, nSamplesPerSec, //ERROR doesn't work
			   OP_AUDIOTAG_PCMRATE, pcmrate,
	           OP_AUDIOTAG_INTERLEAVESAMPLES, true,
	           OP_AUDIOTAG_END);
	if (retval < 0)
		return;

	//  Register our callback.  
	retval = op_sound_register_callback
	          (fSndDev, beepdone_callback, this);
	if (retval < 0)
		return;

	retval = op_sound_start (fSndDev);

}

void
OaSoundTest::Close()
{
	// Only called on second open, because freechan will hang after IODONE
	//op_sound_stop(fSndDev);
	op_sound_unregister_callback(fSndDev, beepdone_callback);
	op_sound_freechan(fSndDev);
	fSndDev = NULL;
}

void
OaSoundTest::uninit (void)
{
	if (fSndDev) {
		op_sound_freechan (fSndDev);
		fSndDev = NULL;
	}
	for (int i=0; i<2; i++)
	{
	    if (fSndBuf[i].fSampleBuffer) 
	    {
		free (fSndBuf[i].fSampleBuffer);
		fSndBuf[i].fSampleBuffer = NULL;
	    }
	}
}

void OaSoundTest::setPlayMode(PlayMode mode)
{
	fPlayMode = mode;
	this->populateSoftKeys(mode);
}

void OaSoundTest::populateSoftKeys(PlayMode mode)
{

	switch (mode)
	{
	case kInitializedMode:
		this->setSoftKeys(new OpSoftKeys());
		this->getSoftKeys()->setCmdLabel(CMD_TEST, 0, "Beep");
		this->getSoftKeys()->setCmdLabel(kCmdPlay, 0, "Play");
		break;

	case kPlayingMode:
		this->getSoftKeys()->removeAllCmds();
		this->getSoftKeys()->setCmdLabel(kCmdPause, 0, "Pause");
		this->getSoftKeys()->setCmdLabel(kCmdStop, 0, "Stop");
		break;

	case kPausedMode:
		this->getSoftKeys()->removeAllCmds();
		this->getSoftKeys()->setCmdLabel(kCmdPlay, 0, "Play");
		this->getSoftKeys()->setCmdLabel(kCmdStop, 0, "Stop");
		break;

	case kStoppedMode:
		this->getSoftKeys()->removeAllCmds();
		this->getSoftKeys()->setCmdLabel(CMD_TEST, 0, "Beep");
		this->getSoftKeys()->setCmdLabel(kCmdPlay, 0, "Play");
		break;
	}
}

