/*
 * Copyright 2023, Haiku, inc.
 *
 * Distributed under the terms of the MIT License.
 */


#include <termios.h>


int
cfsetspeed(struct termios *termios, speed_t speed)
{
	/* Custom speed values are stored in c_ispeed_divider and c_ospeed_divider as  dividers of a
	 * 24 MHz base clock.
	 * Standard values are inlined in c_cflag. */
	if (speed > B31250) {
		termios->c_cflag |= B_24MHZ_WITH_CUSTOM_DIVIDER;
		termios->c_ospeed_divider = 24000000 / speed;
		termios->c_ispeed_divider = 24000000 / speed;
		return 0;
	}

	termios->c_cflag &= ~CBAUD;
	termios->c_cflag |= speed;
	return 0;
}

