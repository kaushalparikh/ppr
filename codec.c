
#include <stdio.h>
#include <opus.h>

#include "types.h"
#include "codec.h"

/* Local structures */
typedef struct
{
  void *encoder;
  void *decoder;
} codec_t;

/* File scope global variables */
static codec_t codec =
{
  .encoder = NULL,
  .decoder = NULL
};


int32 codec_init (uint8 channels, uint32 frame_duration, uint32 rate)
{
  int32 status;

  codec.encoder = opus_encoder_create (rate, channels,
                                       OPUS_APPLICATION_VOIP, &status);
  if (status == OPUS_OK)
  {
  }

  codec.decoder = opus_decoder_create (rate, channels, &status);
  if (status == OPUS_OK)
  {
  }

  if (status == OPUS_OK)
  {
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

  return 1;
}

