/*
 * Copyright 2023, Haiku, inc.
 *
 * Distributed under the terms of the MIT License.
 */


#include <termios.h>



int
cfsetspeed(struct termios *termios, speed_t speed)
{
	/* Check for custom speed values (see above) */
	if (speed > B31250) {
		termios->c_cflag |= CBAUD;
		termios->c_ispeed = speed;
		termios->c_ospeed = speed;
		return 0;
	}

	termios->c_cflag &= ~CBAUD;
	termios->c_cflag |= speed;
	return 0;
}


