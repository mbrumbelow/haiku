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

#include <errno_private.h>


static volatile sig_atomic_t signo[NSIG];

static void handler(int);


char *
readpassphrase(const char *prompt, char *buf, size_t bufsiz, int flags)
{
	ssize_t nr;
	char chr, *p, *end;
	int i;
	int input, output, needRestart, errnoStore;
	struct termios currentTerm, oldTerm;
	struct sigaction signal;
	struct sigaction saveALRM, saveINT, saveHUP, saveQUIT, saveTERM;
	struct sigaction saveTSTP, saveTTIN, saveTTOU, savePIPE;

	needRestart = 0;
	errnoStore = 0;

	// Can't do much if the buffer is equal to zero.
	// Errors have to return a null pointer.
	if (bufsiz == 0) {
		__set_errno(EINVAL);
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
				__set_errno(ENOTTY);
				return NULL;
			}

			input = STDIN_FILENO;
			output = STDOUT_FILENO;
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

				tcsetattr(input, TCSAFLUSH, &currentTerm);
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

		sigaction(SIGALRM, &signal, &saveALRM);
		sigaction(SIGHUP, &signal, &saveHUP);
		sigaction(SIGINT, &signal, &saveINT);
		sigaction(SIGPIPE, &signal, &savePIPE);
		sigaction(SIGQUIT, &signal, &saveQUIT);
		sigaction(SIGTERM, &signal, &saveTERM);

		// The following signals will cause a "restart" later on.
		sigaction(SIGTSTP, &signal, &saveTSTP);
		sigaction(SIGTTIN, &signal, &saveTTIN);
		sigaction(SIGTTOU, &signal, &saveTTOU);

		if (!(flags & RPP_STDIN))
			write(output, prompt, strlen(prompt));

		end = buf + bufsiz - 1;
		p = buf;

		// Processes every character individually until they run out.
		while ((nr = read(input, &chr, 1)) != 0
				&& chr != '\n'
				&& chr != '\r') {

			if (p < end) {
					if ((flags & RPP_SEVENBIT))
						chr &= 0x7f;

					if (isalpha((unsigned char) chr) {
						if ((flags & RPP_FORCELOWER))
							chr = (char) tolower((unsigned char) chr);

						if ((flags & RPP_FORCEUPPER))
							chr = (char) toupper((unsigned char) chr);
				}

				if (p < end)
					*p++ = chr;
			}
		}

		*p = '\0';
		errnoStore = errno;

		// Manually move to the next line.
		if (!(currentTerm.c_lflag & ECHO))
			write(output, "\n", 1);

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

		sigaction(SIGALRM, &saveALRM, NULL);
		sigaction(SIGHUP, &saveHUP, NULL);
		sigaction(SIGINT, &saveINT, NULL);
		sigaction(SIGQUIT, &saveQUIT, NULL);
		sigaction(SIGPIPE, &savePIPE, NULL);
		sigaction(SIGTERM, &saveTERM, NULL);

		sigaction(SIGTSTP, &saveTSTP, NULL);
		sigaction(SIGTTIN, &saveTTIN, NULL);
		sigaction(SIGTTOU, &saveTTOU, NULL);

		close(input);

		// After having caught signals that were issued during execution time,
		// we send them again after restoring the original signal handlers,
		// should they have been set by the application using readpassphrase(3)

		for (i = 0; i < NSIG; i++) {
			if (signo[i]) {
				pthread_kill(pthread_self(), i);
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
		__set_errno(errnoStore);

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

