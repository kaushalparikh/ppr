
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/types.h>

#include "types.h"
#include "util.h"

#if defined (INPUT_KEYBOARD)
static struct termios read_fd_config;
#else
#define SYS_GPIO_BASE             "/sys/class/gpio"
#define SYS_GPIO_EXPORT           SYS_GPIO_BASE "/export"
#define SYS_GPIO_UNEXPORT         SYS_GPIO_BASE "/unexport"
#define SYS_GPIO_NUMBER(gpio)     #gpio
#define SYS_GPIO_DIRECTION(gpio)  SYS_GPIO_BASE "/gpio" #gpio "/direction"
#define SYS_GPIO_VALUE(gpio)      SYS_GPIO_BASE "/gpio" #gpio "/value"
#define SYS_GPIO_EDGE(gpio)       SYS_GPIO_BASE "/gpio" #gpio "/edge"

typedef struct
{
  char *number;
  char *direction;
  char *edge;
  int value_fd;
} gpio_info_t;

static gpio_info_t gpio_info[NUM_INPUTS] =
{
  [INPUT_PTT] =
    {
      .number    = "0",
      .direction = "in",
      .edge      = "none",
      .value_fd  = -1,
    },
  [INPUT_AAA] =
    {
      .number    = "1",
      .direction = "in",
      .edge      = "none",
      .value_fd  = -1,
    },
  [INPUT_BBB] =
    {
      .number    = "2",
      .direction = "in",
      .edge      = "none",
      .value_fd  = -1,
    },
  [INPUT_CCC] =
    {
      .number    = "3",
      .direction = "in",
      .edge      = "none",
      .value_fd  = -1,
    }
}; 
#endif


int32 input_init (void)
{
  int status;

#if defined (INPUT_KEYBOARD)
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
#else
  int count;
  int fd;

  /* Export GPIO */
	fd = open (SYS_GPIO_EXPORT, O_WRONLY);
  if (fd > 0)
  {
    for (count = 0; count < NUM_INPUTS; count++)
    {
      char *gpio_number = gpio_info[count].number;
	    if ((write (fd, gpio_number, strlen (gpio_number))) <= 0)
      {
        printf ("Unable to export gpio %s\n", gpio_number);
      }
    }

    status = 1;
  }
  else
  {
    printf ("Unable to open gpio export file\n");
    status = -1;
  }
  close (fd);

  if (status > 0)
  {
    /* Set GPIO direction */
    for (count = 0; count < NUM_INPUTS; count++)
    {
      char *gpio_direction = gpio_info[count].direction;
      
      fd = open (SYS_GPIO_DIRECTION (atoi (gpio_info[count].number)), O_WRONLY);
	    if ((write (fd, gpio_direction, strlen (gpio_direction))) <= 0)
      {
        printf ("Unable to set gpio %s direction %s\n", gpio_info[count].number,
                                                        gpio_direction);
      }
      close (fd);
    }
    
    /* Set GPIO edge */
    for (count = 0; count < NUM_INPUTS; count++)
    {
      char *gpio_edge = gpio_info[count].edge;
      
      fd = open (SYS_GPIO_DIRECTION (atoi (gpio_info[count].number)), O_WRONLY);
	    if ((write (fd, gpio_edge, strlen (gpio_edge))) <= 0)
      {
        printf ("Unable to set gpio %s edge %s\n", gpio_info[count].number,
                                                        gpio_edge);
      }
      close (fd);
    }
    
    /* Open value FD */
    for (count = 0; count < NUM_INPUTS; count++)
    {
      fd = open (SYS_GPIO_VALUE (atoi (gpio_info[count].number)), O_WRONLY);
      if (fd > 0)
      {
        gpio_info[count].value_fd = fd;
      }
      else
      {
        printf ("Unable to open gpio %s value file\n", gpio_info[count].number);
      }
    }
  }
#endif
  
  return status;
}

int32 input_deinit (void)
{
#if defined (INPUT_KEYBOARD)
  return tcsetattr (0, TCSANOW, &read_fd_config);
#else
  int status;
  int count;
  int fd;

  /* Close value FD */
  for (count = 0; count < NUM_INPUTS; count++)
  {
    if (gpio_info[count].value_fd > 0)
    {
      close (gpio_info[count].value_fd);
      gpio_info[count].value_fd = -1;
    }
  }

  /* Unexport GPIO */
	fd = open (SYS_GPIO_UNEXPORT, O_WRONLY);
  if (fd > 0)
  {
    for (count = 0; count < NUM_INPUTS; count++)
    {
      char *gpio_number = gpio_info[count].number;
	    if ((write (fd, gpio_number, strlen (gpio_number))) <= 0)
      {
        printf ("Unable to unexport gpio %s\n", gpio_number);
      }
    }

    status = 1;
  }
  else
  {
    printf ("Unable to open gpio unexport file\n");
    status = -1;
  }
  close (fd);
 
  return status;
#endif
}

uint32 input_read (void)
{
  unsigned int input = 0;

#if defined (INPUT_KEYBOARD)
  fd_set read_fd;
  struct timeval timeout;
  static char talk = 'r';

  timeout.tv_sec  = 0;
  timeout.tv_usec = 1*1000;

  FD_ZERO (&read_fd);
  FD_SET (0, &read_fd);

  if (select (1, &read_fd, NULL, NULL, &timeout)) 
  {
    talk = (char)getchar ();
    tcflush (0, TCIFLUSH);
  }
  
  input |= ((talk == 't') ? (0x1 << INPUT_PTT) : 0);
#else
  int count;

  for (count = 0; count < NUM_INPUTS; count++)
  {
    char value;
    if ((read (gpio_info[count].value_fd, &value, 1)) > 0)
    {
      if (value > 0)
      {
        input |= (0x1 << count);
      }
    }
  }
#endif

  return input;
}

