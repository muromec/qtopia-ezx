#include <alsa/asoundlib.h>
main()
{
    snd_seq_t *seq_handle;
    snd_seq_open(&seq_handle, "default", SND_SEQ_OPEN_INPUT, 0);
}
