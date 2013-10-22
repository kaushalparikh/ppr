
#include <stdio.h>

#include "types.h"
#include "util.h"

int16 audio_buffer[2*960];

void master_loop (void)
{
  int32 status;
  int32 frame = 1;
  int32 start_time = clock_get_count ();

  while (1)
  {
    if ((status = audio_capture (audio_buffer, 1)) > 0)
    {
      status = audio_playback (audio_buffer, 1);
      
      printf ("Play time %d sec., frame %d\r",
                 ((clock_get_count ()) - start_time)/1000, frame++);
    }
  }
}

int main (int argc, char * argv[])
{
  os_init ();
  
  if ((audio_init (20, 16000)) > 0)
  {
    master_loop ();
  }

  audio_deinit ();

  return 0;
}

