#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>

#define KEYBOARD_SERIAL_PATH "/dev/ttyO1"

// //=================================================================================
// //=================================================================================
// //=================================================================================
// //Setup the serial port, using port/path passed in

// int SetupSerialPort (const char path[], uint8_t should_be_blocking)
// {
//     int fd;
//     struct termios tty_attributes;
    
//     // open device for read/write
//     fd = open(path, O_RDWR);
    
//     if (fd < 0)
//     {
//         perror(path);
//         return(-1);
//     }
    
//     tcgetattr(fd,&tty_attributes);
//     cfmakeraw(&tty_attributes);
//     // Minimum number of characters for non-canonical read.
//     tty_attributes.c_cc[VMIN]=1;
//     // Timeout in deciseconds for non-canonical read.
//     tty_attributes.c_cc[VTIME]=0;
    
//     // setup bauds
//     cfsetispeed(&tty_attributes, B38400);
//     cfsetospeed(&tty_attributes, B38400);
    
//     // apply changes now
//     tcsetattr(fd, TCSANOW, &tty_attributes);
    
//     if (should_be_blocking)
//     {
//         // set it to blocking
//         fcntl(fd, F_SETFL, 0);
//     }
//     else
//     {
//         // set it to non-blocking
//         fcntl(fd, F_SETFL, O_NONBLOCK);
//     }
    
//     return fd;
// }
 
int main (void) 
{
  printf ("Running test_key_mech_input (v2)...\n");

  int keyboard_fd;
  uint8_t keyboard_input_buf[1] = {0};
  
  //==========================================================
  //Set up serial connection
  
  printf ("Setting up key mech serial connection...\n");

  struct termios tty_attributes;
    
  // open UART1 device file for read/write
  keyboard_fd = open (KEYBOARD_SERIAL_PATH, O_RDWR);
  
  //if can't open file
  if (keyboard_fd < 0)
  {
    //show error and exit
    perror (KEYBOARD_SERIAL_PATH);
    return (-1);
  }
  
  tcgetattr (keyboard_fd, &tty_attributes);
  cfmakeraw (&tty_attributes);
  tty_attributes.c_cc[VMIN]=1;
  tty_attributes.c_cc[VTIME]=0;
  
  // setup bauds (key mech Arduino uses 38400)
  cfsetispeed (&tty_attributes, B38400);
  cfsetospeed (&tty_attributes, B38400);
  
  // apply changes now
  tcsetattr (keyboard_fd, TCSANOW, &tty_attributes);
  
  // set it to blocking
  fcntl (keyboard_fd, F_SETFL, 0);

  //==========================================================
  //Enter main loop, and just read any data that comes in over the serial port
  
  printf ("Starting reading data from key mech...\n");
  
  while (true)
  {
    //attempt to read a byte from the serial device file
    int ret = read (keyboard_fd, keyboard_input_buf, 1);

    //if read something
    if (ret != -1)
    {
      //display the read byte
      printf ("Byte read from keyboard: %d\n", keyboard_input_buf[0]);
     
   } //if (ret)
    
  } ///while (true)

  return 0;

}
