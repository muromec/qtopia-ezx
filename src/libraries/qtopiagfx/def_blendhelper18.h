/*
  All 18bpp-related support code is put here for development convenience
*/

#ifndef DEF_BLENDHELPER18_H
#define DEF_BLENDHELPER18_H

static inline unsigned int qConvertRgb32To18(unsigned int c)
{
    return (((c) >> 2) & 0x0003f) |
           (((c) >> 4) & 0x00fc0) |
           (((c) >> 6) & 0x3f000);
}

static inline unsigned int qConvertRgb18To32(unsigned short c)
{
    return ((c << 2) & 0x00FC) |
           ((c << 4) & 0xFC00) |
           ((c << 6) & 0xFC0000) |
           0xFF030303;
}

// Read 3-byte rgb value at data
static inline unsigned int qRead18(unsigned char *data)
{
  return (data[2] << 16) | (data[1] << 8) | data[0];
}

// Write 3-byte rgb value at data
static inline void qWrite18(unsigned char *data, unsigned int value)
{
  data[0] = value & 0xff;
  value >>= 8;
  data[1] = value & 0xff;
  value >>= 8;
  data[2] = value & 0xff;
}

// Multiply all parts of rgb18 val by alpha (alpha is 6-bit)
static inline unsigned int premul_nozero18(unsigned int val, unsigned char alpha)
{
    if(alpha == 0xFF)
        return val;
    else
        return ((((val & 0x0003F03F) * alpha) >> 6) & 0x0003F03F) |
               ((((val & 0x00000FC0) >> 6) * alpha) & 0x00000FC0);
}

// dest: rgb18
// src: argb32p
// dest = blend(dest, src)
static inline void argb32p_rgb18_inplace(unsigned char *dest,
                                         unsigned int *src)
{
  unsigned char alpha = *src >> 24;
  unsigned int dest32 = qRead18(dest);

  if(alpha == 0x00)
    return;
  if(alpha == 0xFF)
    dest32 = qConvertRgb32To18(*src);
  else
    dest32 = qConvertRgb32To18((premul_nozero(qConvertRgb18To32(dest32), 0xFF - alpha) & 0xFFFFFF) + *src);

  qWrite18(dest, dest32);
}

// dest: rgb18
// src: argb32p
// dest = blend(dest, src*opacity)
static inline void argb32p_rgb18_opacity_inplace(unsigned char *dest,
                                                 unsigned int *src,
                                                 unsigned char opacity)
{
  unsigned int src_op = premul_nozero(*src, opacity);
  argb32p_rgb18_inplace(dest, &src_op);
}

// dest, output: rgb18
// src: argb32p
// dest = blend(dest, src)
static inline void argb32p_rgb18(unsigned char *dest,
                                 unsigned int *src,
                                 unsigned char *output)
{
  unsigned char alpha = *src >> 24;
  unsigned int dest32 = qRead18(dest);

  if(alpha == 0x00)
    {}
  else if(alpha == 0xFF)
    dest32 = qConvertRgb32To18(*src);
  else
    dest32 = qConvertRgb32To18((premul_nozero(qConvertRgb18To32(dest32), 0xFF - alpha) & 0xFFFFFF) + *src);

  qWrite18(output, dest32);
}

// dest, output: rgb18
// src: argb32p
// output = blend(dest, src*opacity)
static inline void argb32p_rgb18_opacity(unsigned char *dest,
                                         unsigned int *src,
                                         unsigned char opacity,
                                         unsigned char *output)
{
  unsigned int src_op = premul_nozero(*src, opacity);
  argb32p_rgb18(dest, &src_op, output);
}

// dest is rgb18
// src is argb24p
// dest = blend(dest, src)
static inline void argb24p_rgb18_inplace(unsigned char *dest,
                                         unsigned char *src)
{
  unsigned int dest32 = qRead18(dest);
  unsigned int src32 = qRead18(src);

  unsigned char alpha = (src32 >> 18) & 0x3F;
  if (alpha==0)
    return;
  if (alpha==0x3F)
  {
    qWrite18(dest, src32);
    return;
  }
  dest32 = (premul_nozero18(dest32, 0x3F-alpha) + src32) & 0x3FFFFF;
  qWrite18(dest, dest32);
}

// dest is rgb18
// src is argb24p
// output = blend(dest, src)
static inline void argb24p_rgb18(unsigned char *dest,
                                 unsigned char *src,
                                 unsigned char *output)
{
  unsigned int dest32 = qRead18(dest);
  unsigned int src32 = qRead18(src);

  unsigned char alpha = (src32 >> 18) & 0x3F;
  if (alpha==0)
  {
    qWrite18(output, dest32);
    return;
  }
  if (alpha==0x3F)
  {
    qWrite18(output, src32);
    return;
  }
  dest32 = (premul_nozero18(dest32, 0x3F-alpha) + src32) & 0x3FFFFF;
  qWrite18(output, dest32);
}

// dest: rgb18
// src: argb24p
// dest = blend(dest, src*opacity)
static inline void argb24p_rgb18_opacity_inplace(unsigned char *dest,
                                         unsigned char *src,
                                         unsigned char opacity)
{
  unsigned int dest32 = qRead18(dest);
  unsigned int src32 = qRead18(src);
  src32 = premul_nozero18(src32, opacity);

  unsigned char alpha = (src32 >> 18) & 0x3F;
  if (alpha==0)
    return;
  if (alpha==0x3F)
  {
    qWrite18(dest, src32);
    return;
  }
  dest32 = (premul_nozero18(dest32, 0x3F-alpha) + src32) & 0x3FFFFF;
  qWrite18(dest, dest32);
}

// dest, output: rgb18
// src: argb24p
// output = blend(dest, src*opacity)
static inline void argb24p_rgb18_opacity(unsigned char *dest,
                                                 unsigned char *src,
                                                 unsigned char opacity,
                                                 unsigned char *output)
{
  unsigned int dest32 = qRead18(dest);
  unsigned int src32 = qRead18(src);
  src32 = premul_nozero18(src32, opacity);

  unsigned char alpha = (src32 >> 18) & 0x3F;
  if (alpha==0)
  {
    *output++ = *dest++;
    *output++ = *dest++;
    *output++ = *dest++;
    return;
  }
  if (alpha==0x3F)
  {
    qWrite18(output, src32);
    return;
  }
  dest32 = (premul_nozero18(dest32, 0x3F-alpha) + src32) & 0x3FFFFF;
  qWrite18(output, dest32);
}

#endif // DEF_BLENDHELPER18_H
