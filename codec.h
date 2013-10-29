#ifndef __CODEC_H__
#define __CODEC_H__

#include "types.h"

extern int32 codec_init (uint8 channels, uint32 frame_duration, uint32 rate);

extern int32 codec_deinit (void);

extern int32 codec_encode (int16 *audio_buffer, uint8 *codec_buffer, uint8 frames);

extern int32 codec_decode (uint8 *codec_buffer, int16 *audio_buffer, uint8 frames);

#endif

