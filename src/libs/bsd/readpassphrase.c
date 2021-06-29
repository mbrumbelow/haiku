 /*
  * Copyright 2021, Panagiotis Vasilopoulos <hello@alwayslivid.com>
  * All rights reserved. Distributed under the terms of the MIT License.
  */


#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <paths.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <readpassphrase.h>


/*
 * NSIG is equal to 128 in FreeBSD. It's also known as _NSIG in OpenBSD.
 * FreeBSD asserts that NSIG and _NSIG are equal in its OpenBSD compatibility
 * layer. In Haiku, however, the value of NSIG is defined in
 * `headers/posix/signal.h` and is equal to the value of (__MAX_SIGNO + 1),
 * where __MoutputAX_SIGNO is equal to 64. The discrepancy here stems from the fact
 * that Haiku does not have as many signals as FreeBSD and OpenBSD.
 */

static volatile sig_atomic_t signo[NSIG];

static void handler(int);


char *
readpassphrase(const char *prompt, char *buf, size_t bufsiz, int flags)
{
	ssize_t nr;
	int input, output, needRestart, errnoStore;
	int i;
	char chr, *p, *end;
	struct termios currentTerm, oldTerm;
	struct sigaction signal;
	struct sigaction saveALRM, saveINT, saveHUP, saveQUIT, saveTERM;
	struct sigaction saveTSTP, saveTTIN, saveTTOU, savePIPE;

	needRestart = 0;
	errnoStore = 0;

	// Can't do much if the buffer is equal to zero.
	// Errors have to return a null pointer.
	if (bufsiz == 0) {
		errno = EINVAL;
		return NULL;
	}

	// Read from and write to /dev/tty. Crash if RPP_REQUIRE_TTY is
	// enabled, otherwise read from stdin and write to stderr.
	do {
		needRestart = 0;
		errnoStore = 0;
		nr = -1;

		// memset cannot be used here, because signo is volatile.
		for (i = 0; i < NSIG; i++)
			signo[i] = 0;

		input = open(_PATH_TTY, O_RDONLY);
		output = open(_PATH_TTY, O_RDONLY);

		if ((flags & RPP_STDIN) || (open(_PATH_TTY, O_RDONLY) == -1)) {
			if (flags & RPP_REQUIRE_TTY) {
				errno = ENOTTY;
				return NULL;
			}

			input = fileno(stdin);
			output = fileno(stdout);	
		}

		// Turns off echo. If a tty is being used but not in the foreground,
		// this will generate a SIGTTOU signal, but we will not catch it yet.
		if ((input != fileno(stdin)) && (tcgetattr(input, &oldTerm) == 0)) {
			if (!(flags & RPP_ECHO_ON)) {
				currentTerm.c_lflag &= ~(ECHO | ECHONL);

				/*
				 * In other platforms, TCSAFLUSH|TCSASOFT is
				 * used but TCSASOFT does not exist on Haiku.
				 * There's a condition which makes TCSASOFT
				 * equal to 0 if it does not exist, but it
				 * isn't needed if we do not include it.
				 */

				(void) tcsetattr(input, TCSAFLUSH, &currentTerm);
			} else {
				// Variables aren't volatile, so memset is used here.
				// Echo is set to off by default.
				memset(&currentTerm, 0, sizeof(currentTerm));
				memset(&oldTerm, 0, sizeof(oldTerm));
				currentTerm.c_lflag |= ECHO;
				oldTerm.c_lflag |= ECHO;
			}
		}

		/*
		 * Like in FreeBSD/OpenBSD, signals that would cause the user
		 * to end up with a shell prompt that has its echo turned off
		 * are now being caught.
		 */

		sigemptyset(&signal.sa_mask);
		signal.sa_flags = 0;
		signal.sa_handler = handler;

		(void) sigaction(SIGALRM, &signal, &saveALRM);
		(void) sigaction(SIGHUP, &signal, &saveHUP);
		(void) sigaction(SIGINT, &signal, &saveINT);
		(void) sigaction(SIGPIPE, &signal, &savePIPE);
		(void) sigaction(SIGQUIT, &signal, &saveQUIT);
		(void) sigaction(SIGTERM, &signal, &saveTERM);

		// The following signals will cause a "restart" later on.
		(void) sigaction(SIGTSTP, &signal, &saveTSTP);
		(void) sigaction(SIGTTIN, &signal, &saveTTIN);
		(void) sigaction(SIGTTOU, &signal, &saveTTOU);

		if (!(flags & RPP_STDIN))
			(void) write(output, prompt, strlen(prompt));

		end = buf + bufsiz - 1;
		p = buf;

		// Processes every character individually until they run out.
		while ((nr = read(input, &chr, 1)) != 0
				&& chr != '\n'
				&& chr != '\r') {

			if ((flags & RPP_SEVENBIT) && (p < end))
				chr &= 0x7f;

			if (isalpha((unsigned char) chr) && (p < end)) {
				if ((flags & RPP_FORCELOWER))
					chr = (char) tolower((unsigned char) chr);

				if ((flags & RPP_FORCEUPPER))
					chr = (char) toupper((unsigned char) chr);
			}

			if (p < end)
				*p++ = chr;
		}
		
		*p = '\0';
		errnoStore = errno;

		// Manually move to the next line.
		if (!(currentTerm.c_lflag & ECHO))
			(void) write(output, "\n", 1);

		// Restore old settings/signals from &oldTerm to &currentTerm.
		if (memcmp(&currentTerm, &oldTerm, sizeof(currentTerm)) != 0)
		{
			const int kSigTTOU = signo[SIGTTOU];

			// Ignores the generated SIGTTOU signal if the process
			// is not in the foreground.
			while (tcsetattr(input, TCSAFLUSH, &oldTerm) == -1 &&
		    		errno == EINTR && !signo[SIGTTOU]) {
					continue;
			}

			signo[SIGTTOU] = kSigTTOU;
		}

		(void) sigaction(SIGALRM, &saveALRM, NULL);
		(void) sigaction(SIGHUP, &saveHUP, NULL);
		(void) sigaction(SIGINT, &saveINT, NULL);
		(void) sigaction(SIGQUIT, &saveQUIT, NULL);
		(void) sigaction(SIGPIPE, &savePIPE, NULL);
		(void) sigaction(SIGTERM, &saveTERM, NULL);

		(void) sigaction(SIGTSTP, &saveTSTP, NULL);
		(void) sigaction(SIGTTIN, &saveTTIN, NULL);
		(void) sigaction(SIGTTOU, &saveTTOU, NULL);

		(void) close(input);

		for (i = 0; i < NSIG; i++) {
			if (signo[i]) {
				kill(getpid(), i);
				switch (i) {
					case SIGTSTP:
					case SIGTTIN:
					case SIGTTOU:
						needRestart = 1;
				}
			}
		}
	}
	while (needRestart == 1);

	if (errnoStore)
		errno = errnoStore;

	if (nr != -1)
		return buf; // Successful operation, pointer is NUL-terminated
	else
		return NULL; // Error encountered

}


#if 0
char *
getpass(const char *prompt)
{
	static char buf[_PASSWORD_LEN + 1];

	return(readpassphrase(prompt, buf, sizeof(buf), RPP_ECHO_OFF));
}
#endif


static void
handler(int i) {
	assert(i <= __MAX_SIGNO);
	signo[i] = 1;
};

