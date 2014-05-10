
#include <stdio.h>
#include <opus.h>

#include "types.h"
#include "codec.h"

/* Codec bitrate in bps */
#define BITRATE  (8000)

/* Local structures */
typedef struct
{
  void   *encoder;
  void   *decoder;
  uint32  frame_size;
  uint32  packet_size;
} codec_t;

/* File scope global variables */
static codec_t codec =
{
  .encoder     = NULL,
  .decoder     = NULL,
  .frame_size  = 0,
  .packet_size = 0
};


int32 codec_encode (int16 *audio_buffer, uint8 *codec_buffer, uint8 frames)
{
  int32 status;

  if ((status = opus_encode (codec.encoder, audio_buffer, codec.frame_size,
                             codec_buffer, 1000)) <= 0)
  {
    printf ("Unable to encode audio\n");
    status = -1;
  }

  return status;
}

int32 codec_decode (uint8 *codec_buffer, int16 *audio_buffer, uint8 frames)
{
  int32 status;

  if ((status = opus_decode (codec.decoder, codec_buffer, codec.packet_size,
                             audio_buffer, codec.frame_size, 0)) <= 0)
  {
    printf ("Unable to decode audio\n");
    status = -1;
  }

  return status;
}

int32 codec_init (uint8 channels, uint32 frame_duration, uint32 rate)
{
  int32 status;

  codec.encoder = opus_encoder_create (rate, channels,
                                       OPUS_APPLICATION_RESTRICTED_LOWDELAY, &status);
  if (status != OPUS_OK)
  {
    printf ("Unable to create encoder\n");
  }

  if ((status == OPUS_OK) &&
      ((status = opus_encoder_ctl (codec.encoder,
                                   OPUS_SET_FORCE_CHANNELS (channels))) != OPUS_OK))
  {
    printf ("Unable to set encoder channels\n");
  }

  if ((status == OPUS_OK) &&
      ((status = opus_encoder_ctl (codec.encoder,
                                   OPUS_SET_BANDWIDTH (OPUS_AUTO))) != OPUS_OK))
  {
    printf ("Unable to set encoder bandwidth\n");
  }

  if ((status == OPUS_OK) &&
      ((status = opus_encoder_ctl (codec.encoder,
                                   OPUS_SET_SIGNAL (OPUS_AUTO))) != OPUS_OK))
  {
    printf ("Unable to set encoder bandwidth\n");
  }

  if ((status == OPUS_OK) &&
      ((status = opus_encoder_ctl (codec.encoder,
                                   OPUS_SET_INBAND_FEC (0))) != OPUS_OK))
  {
    printf ("Unable to set encoder FEC\n");
  }

  if ((status == OPUS_OK) &&
      ((status = opus_encoder_ctl (codec.encoder,
                                   OPUS_SET_VBR (0))) != OPUS_OK))
  {
    printf ("Unable to set encoder CBR\n");
  }

  if ((status == OPUS_OK) &&
      ((status = opus_encoder_ctl (codec.encoder,
                                   OPUS_SET_BITRATE (BITRATE))) != OPUS_OK))
  {
    printf ("Unable to set encoder bitrate\n");
  }

  codec.decoder = opus_decoder_create (rate, channels, &status);
  if (status != OPUS_OK)
  {
    printf ("Unable to create decoder\n");
  }

  if (status == OPUS_OK)
  {
    codec.frame_size  = (rate * frame_duration)/1000;
    codec.packet_size = (((BITRATE * frame_duration)/1000) + 7)/8;
    status = 1;
  }
  else
  {
    codec_deinit ();
  }

  return status;
}

int32 codec_deinit (void)
{
  if (codec.encoder != NULL)
  {
    opus_encoder_destroy (codec.encoder);
    codec.encoder = NULL;
  }

  if (codec.decoder != NULL)
  {
    opus_decoder_destroy (codec.decoder);
    codec.decoder = NULL;
  }

  codec.frame_size = 0;

  return 1;
}

