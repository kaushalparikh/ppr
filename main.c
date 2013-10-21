
#include <stdio.h>

#include "types.h"
#include "util.h"

int16 audio_buffer[2*960];

void master_loop (void)
{
  int32 status;
  uint32 frame = 1;

  while (1)
  {
    if ((status = audio_capture (audio_buffer, 1)) > 0)
    {
      printf ("Playing frame %u\n", frame++);
      status = audio_playback (audio_buffer, 1);
    }
  }
}

int main (int argc, char * argv[])
{
  os_init ();
  
  if ((audio_init ()) > 0)
  {
    master_loop ();
  }

  audio_deinit ();

  return 0;
}

