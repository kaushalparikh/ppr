
#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <sys/types.h>

#include "types.h"
#include "util.h"


static int8 talk = 'r';
static struct termios read_fd_config;


int32 input_init (void)
{
  int status;

  status = tcgetattr (0, &read_fd_config);
  if (status == 0)
  {
    struct termios modified_config = read_fd_config;
    modified_config.c_lflag &= ~(ICANON | ECHO);
    modified_config.c_cc[VMIN]  = 0;
    modified_config.c_cc[VTIME] = 0;
    status = tcsetattr (0, TCSANOW, &modified_config);
  }
  
  if (status == 0)
  {
    status = 1;
  }
  else
  {
    printf ("Unable configure input\n");
    status = -1;
  }

  return status;
}

int32 input_deinit (void)
{
  return tcsetattr (0, TCSANOW, &read_fd_config);
}

int32 input_read (void)
{
  fd_set read_fd;
  struct timeval timeout;

  timeout.tv_sec  = 0;
  timeout.tv_usec = 1*1000;

  FD_ZERO (&read_fd);
  FD_SET (0, &read_fd);

  if (select (1, &read_fd, NULL, NULL, &timeout)) 
  {
    talk = (char)getchar ();
    tcflush (0, TCIFLUSH);
  }

  return (talk == 't' ? 1 : -1);
}

