/*
 * ===================================================================
 *  TS 26.104
 *  R99   V3.4.0 2002-02
 *  REL-4 V4.3.0 2002-02
 *  3GPP AMR Floating-point Speech Codec
 * ===================================================================
 *
 */

/*
 * interf_dec.h
 *
 *
 * Project:
 *    AMR Floating-Point Codec
 *
 * Contains:
 *    Defines interface to AMR decoder
 *
 */

#ifndef _interf_dec_h_
#define _interf_dec_h_

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Function prototypes
 */
/*
 * Conversion from packed bitstream to endoded parameters
 * Decoding parameters to speech
 */
void Decoder_Interface_Decode( void *st,

#ifndef ETSI
      unsigned char *bits,

#else
      short *bits,
#endif

      short *synth, int bfi );

/*
 * Reserve and init. memory
 */
void *Decoder_Interface_init( void );

/*
 * Exit and free memory
 */
void Decoder_Interface_exit( void *state );

#ifdef __cplusplus
}
#endif

#endif

