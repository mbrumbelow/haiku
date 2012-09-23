/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *          Prasad Joshi <prasadjoshi.linux@gmail.com>
 */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <termcap.h>
#include <termios.h>
#include <sys/types.h>
#include <pwd.h>
#include <errno.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <OS.h>

#define ALL "all"

typedef struct {
	team_id     tid;
	uid_t       uid;
	int64       read;
	int64       wrote;
	double      percent;
	char        cmd[64];
} team_ios_t;

typedef struct {
	int         nthreads;
	team_ios_t  *threads;
} team_ios_list_t;

typedef enum {
	TID = 0,
	UID,
	READ,
	WRITE,
	PERCENT,
	CMD
} SORT_KEY;

static char *sort_keys[] = {
	"Proccesses ID",
	"User ID",
	"Read Bandwidth",
	"Write Bandwidth",
	"Percentage of total IOs",
	"Process Name",
};

static int      rows;             /* # rows on the screen */
static int      cols;             /* # columns on the screen */
static char     *clear_screen = NULL;
static bool     screen_size_changed = false; /* set in signal handler */
static SORT_KEY sort_key = PERCENT;     /* default key for sorting data */
static bool     reverse = false;     /* reverse sorting */
static tcflag_t tty_local_modes = 0;     /* saved terminal mode */
static char     *program = NULL;

/* program parameters */
static int  delay       = 3;
static int  iter        = 0;

static char *user       = "all";
static uid_t user_uid   = 0;
static bool user_filter = false;

static char     *proc   = "all";
static team_id  *tids   = NULL;

static bool only_ios    = false;

static void usage(void)
{
	printf( "Usage: %s\n"
		"-d <delay in seconds (default = 3)>\n"
		"-i <# iterations (default = 0)>\n"
		"-u <monitor processes of user (default = all)>\n"
		"-p <comma separated list of pids to monitor (default = all)>\n"
		"-o only show processes doing ios (default = false)\n"
		"-h show this message\n", program);
}

static void end(const char *msg, int rc)
{
	if (msg)
		fprintf(stderr, "%s: %s", msg, strerror(rc));

	if (strcmp(user, ALL))
		free(user);

	if (strcmp(proc, ALL))
		free(proc);

	exit(rc);
}

/*
 * Converts uid to user name.
 * Caller is supposed to free the memory allocated for user name.
 */
static char *user_name(uid_t uid)
{
	struct passwd *b;

	b = getpwuid(uid);
	if (!b)
		return NULL;
	return strdup(b->pw_name);
}

/*
 * Convert user id to user name
 */
static int user_id(const char *name, uid_t *uid)
{
	struct passwd *b;

	b = getpwnam(name);
	if (!b)
		return -1;
	*uid = b->pw_uid;
	return 0;
}

static bool match_tid(team_id tid, team_id *tids)
{
	team_id *t;

	t = tids;
	while (*t) {
		if (*t == tid)
			return true;
		t++;
	}
	return false;
}

static void xtract_team_list(char *proc)
{
	char    *s;
	int     count;
	int     no_tids;
	char    *x;
	char    *s1;
	int     i;
	bool    done = false;
	team_id tid;

	s = proc;
	count = 0;
	while (s) {
		s = strchr(s, ',');
		count++;
		if (!s)
			break;
		s++;
	}

	/*
	 * Since the list of tids is delimted with 0 tid, we need to allocate
	 * memory for an additional tid. For example if user input is 1,2,3,4
	 * then count will be 3 and memory for 5 tids will be allocated.
	 */
	no_tids = count + 2;

	tids = calloc(no_tids, sizeof(team_id));
	if (!tids) {
		fprintf(stderr, "Memory Allocation Failed.\n");
		end(__func__, ENOMEM);
	}

	x = s1 = s = strdup(proc);
	i = 0;
	while (!done) {
		s1 = strchr(s, ',');

		if (!s1)
			done = true;
		else
			*s1 = 0;

		tid = atol(s);
		if (tid) {
			tids[i] = tid;
			i++;
		}

		s = s1 + 1;
	}
	tids[i] = 0;
	free(x);
}

static int parse_arguments(int argc, char *argv[])
{
	int opt;
	bool invalid = false;

	while ((opt = getopt(argc, argv, "d:i:u:p:oh")) != -1) {
		switch (opt) {
		case 'd':
			delay = atoi(optarg);
			if (!delay || delay < 0) {
				fprintf(stderr, "Invalid argument to -- d\n");
				invalid = true;
			}
			break;
		case 'i':
			iter = atoi(optarg);
			if (!iter || iter < 0) {
				fprintf(stderr, "Invalid argument to -- i\n");
				invalid = true;
			}
			break;
		case 'u': {
			char *u = strdup(optarg);
			if (strcmp(u, "all")) {
				int rc = user_id(u, &user_uid);
				if (rc < 0) {
					fprintf(stderr, "No such user '%s'\n", u);
					free(u);
					invalid = true;
				}
				user = u;
				user_filter = true;
			} else {
				/* TODO: Handle user with name all */
				free(u);
			}
			break;
		}
		case 'p':
			proc = strdup(optarg);
			break;
		case 'o':
			only_ios = true;
			break;
		case 'h':
			usage();
			end(NULL, 0);
		default:
			usage();
			end(NULL, EINVAL);
		}
	}

	if (invalid) {
		usage();
		end(NULL, EINVAL);
	}

	if (!strcmp(proc, "all"))
		return 0;

	/* proc must be comma separated list of pids */
	xtract_team_list(proc);

	return 0;
}

static void window_changed(int unused)
{
	screen_size_changed = true;
}

static void clear(void)
{
	if (clear_screen) {
		printf(clear_screen);
		fflush(stdout);
	}
}

static void cleanup(void)
{
	struct termios    tty;

	free(clear_screen);

	/* restore the terminal setting */
	tcgetattr(STDIN_FILENO, &tty);
	tty.c_lflag = tty_local_modes;
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &tty);
}

static int setup_term(bool only_rows, bool set_tty)
{
	struct termios    tty;
	struct winsize    ws;
	char        *term;
	char        *str;
	char        buf[2048];
	char        *entries;

	if (set_tty) {
		errno = 0;
		if (tcgetattr(STDIN_FILENO, &tty) < 0)
			return -errno;

		tty_local_modes = tty.c_lflag;
		tty.c_lflag &= ~(ECHO | ICANON);

		errno = 0;
		if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &tty) < 0)
			return -errno;
	}

	if (ioctl(1, TIOCGWINSZ, &ws) < 0)
		return 0;

	if (ws.ws_row <= 0)
		return 0;
	rows = ws.ws_row;

	if (ws.ws_col <= 0)
		return 0;
	cols = ws.ws_col;

	if (only_rows)
		return 1;

	term = getenv("TERM");
	if (term == NULL)
		return 0;

	if (tgetent(buf, term) <= 0)
		return 0;

	entries = &buf[0];
	str = tgetstr("cl", &entries);
	if (str == NULL)
		return 0;

	clear_screen = strdup(str);
	return 1;
}

/*
 * Convert bytes into more user readable form of KB, MB, or GB
 */
static inline void bandwidth(char *buf, int size, int64 bytes)
{
	double bw  = bytes;
	double bw1 = 0;
	int i;
	char unit[] = { 'B', 'K', 'M', 'G', 'T' };

	i = 0;
	while ((int) bw) {
		bw1 = bw;
		bw /= 1024;
		i++;
	}

	if (!i)
		i++;

	snprintf(buf, size, "%0.2f %c/s", bw1, unit[i - 1]);
	return;
}

/*
 * Display a line on terminal, limit to available characters in column
 */
static int display_line(const char *f, ...)
{
	va_list ap;
	char line[cols];
	int n;

	va_start(ap, f);
	/* sizeof(line) - 1 since we need to add a trailing newline */
	n = vsnprintf(line, sizeof(line) - 1, f, ap);
	va_end(ap);

	if (n < 0)
		return -1;

	printf("%s\n", line);
	return 0;
}

static void char_repeat(char *buf, char ch, int count)
{
	buf[count--] = 0;
	for (; count >= 0; count--)
		buf[count] = ch;
}

static void display_preferences(void)
{
	char buf[cols];
	char_repeat(buf, '-', cols - 1);
	display_line("%s", buf);	// Line 1

	display_line("User: %s", user);	// Line 2
	display_line("Processes: %s", proc);	// Line 3
	if (only_ios)
		display_line("Only showing processes doing IOs."); // Line 4

	display_line("Sorted according to '%s'.\tReverse Sorting: %s",
			sort_keys[sort_key], reverse ? "true" : "false");// Line 5
	display_line("Refreshing every %d seconds.", delay); // Line 6
	if (iter)
		display_line("Iterations still remaining: %d.", iter);
}

static void display_delta(team_ios_t *d, int teams, int64 total_reads,
        int64 total_writes)
{
	char        rbw[256];
	char        wbw[256];
	team_ios_t  *t;
	int         i;
	char        *n;

	clear();

	bandwidth(rbw, sizeof(rbw), total_reads);
	bandwidth(wbw, sizeof(wbw), total_writes);

	display_line("Total Disk Reads: %11s | Total Disk Writes: %11s", rbw,
			wbw);
	display_line("\n%7s %8s %11s %11s %9s   %s", "TID", "USER",
			"READ BW", "WRITE BW", "IO %", "Command");

	t = d;
	for (i = 0; i < teams; i++, t++) {
		if (i + 8 > rows) {
			/*
			 * maximum rows can only be disaplyed on terminal.
			 * Additional 8 rows are left for displaying user
			 * preferences and title.
			 */
			continue;
		}

		if (only_ios && (!(t->read + t->wrote))) {
			/* skip: user has asked for processes doing IOs */
			continue;
		}

		if (user_filter && (user_uid != t->uid)) {
			/* skip: looking for processes of a selected user */
			continue;
		}

		if (tids) {
			/* must be the last condition for better performance */
			if (!match_tid(t->tid, tids)) {
				/* skip: looking for processes with specific tids */
				continue;
			}
		}

		n = user_name(t->uid);
		bandwidth(rbw, sizeof(rbw), t->read);
		bandwidth(wbw, sizeof(wbw), t->wrote);


		display_line("%7d %8s %11s %11s %7.2f %%   %s", t->tid, n,
				rbw, wbw, t->percent, t->cmd);

		free(n);
	}

	display_preferences();
}

static int compare_ios(const void *a, const void *b)
{
	const team_ios_t *d1 = a;
	const team_ios_t *d2 = b;
	int rc;

	switch (sort_key) {
	case TID:
		rc = d1->tid - d2->tid;
		break;
	case UID:
		rc = d2->uid - d1->uid;
		break;
	case READ:
		rc = d2->read - d1->read;
		break;
	case WRITE:
		rc = d2->wrote - d1->wrote;
		break;
	case PERCENT:
		rc = d2->percent - d1->percent;
		break;
	case CMD:
		rc = strcmp(d1->cmd, d2->cmd);
		break;
	default:
		end("compare_ios", 1);
	}

	if (!rc) {
		/* use ios performed for deciding order */
		rc = (d2->read + d2->wrote) - (d1->read + d1->wrote);
	}

	return reverse? 0 - rc : rc;
}

static void cal_io_percent(team_ios_t *d, int teams, int64 total_ios)
{
	team_ios_t  *t;
	int         i;

	t = d;
	for (i = 0; i < teams; i++, t++)
		t->percent = (100 * (t->wrote + t->read)) / (double) total_ios;
}

static int grow(team_ios_list_t *l)
{
	l->nthreads++;
	l->threads = realloc(l->threads, sizeof(*l->threads) * l->nthreads);

	if (!l->threads)
		return -1;
	return 0;
}

static team_ios_list_t *gather(team_ios_list_t *old)
{
	team_ios_list_t *l;
	int32           c = 0;    // cookie
	team_info       tm;
	int             no;
	team_ios_t      *t;
	team_ios_t      *delta;
	int             i;
	team_ios_t      *ot;
	team_ios_t      *dt;
	int64           total_reads  = 0;
	int64           total_writes = 0;

	l = malloc(sizeof(*l));
	if (!l) {
		fprintf(stderr, "Memory allocation failed.\n");
		return NULL;
	}
	memset(l, 0, sizeof(*l));

	while (get_next_team_info(&c, &tm) == B_NO_ERROR) {
		if (grow(l) < 0) {
			fprintf(stderr, "Memory allocation failed.\n");
			return NULL;
		}

		no = l->nthreads - 1;
		t  = l->threads + no;

		t->tid      = tm.team;
		t->uid      = tm.uid;
		t->read     = tm.read_bytes;
		t->wrote    = tm.write_bytes;
		strlcpy(t->cmd, tm.args, sizeof(t->cmd));
	}

	delta = malloc(sizeof(*delta) * (l->nthreads));
	if (!delta) {
		fprintf(stderr, "Memory allocation failed.\n");
		return NULL;
	}

	for (i = 0; i < l->nthreads; i++) {
		t  = l->threads + i;

		bool new_process = true;

		if (old) {
			/* find team in old list */
			int j;
			for (j = 0; j < old->nthreads; j++) {
				ot = old->threads + j;

				if (ot->tid == t->tid) {
					/* process was already running */
					new_process = false;
					break;
				}
			}
		}

		dt = delta + i;
		if (new_process) {
			/* a newly process has been started */
			dt->read  = t->read;
			dt->wrote = t->wrote;
		} else {
			dt->read  = t->read  - ot->read;
			dt->wrote = t->wrote - ot->wrote;
		}

		dt->tid       = t->tid;
		dt->uid       = t->uid;
		total_reads  += dt->read;
		total_writes += dt->wrote;
		strlcpy(dt->cmd, t->cmd, sizeof(dt->cmd));
	}

	if (old) {
		/* calculate percentage of total IOs for each team */
		cal_io_percent(delta, l->nthreads, total_reads + total_writes);

		/* sort teams */
		qsort(delta, l->nthreads, sizeof(*delta), compare_ios);

		/* display teams */
		display_delta(delta, l->nthreads, total_reads, total_writes);

		free(delta);

		/* ols list of teams is no longer needed */
		free(old->threads);
		free(old);
	}
	return l;
}

static void handle_key(int key)
{
	switch (key) {
	case 't':
		sort_key = TID;
		break;
	case 'u':
		sort_key = UID;
		break;
	case 'r':
		sort_key = READ;
		break;
	case 'w':
		sort_key = WRITE;
		break;
	case 'p':
		sort_key = PERCENT;
		break;
	case 'c':
		sort_key = CMD;
		break;
	case 'x':
		/* toggle the reverse sorting */
		reverse = !reverse;
		break;
	case 'q':
		/* gracefully end the program */
		end(NULL, 0);
	}
}

static int user_input(void)
{
	long    file_flags;
	int     rc;
	char    c;

	file_flags = fcntl(STDIN_FILENO, F_GETFL);
	if (file_flags < 0)
		file_flags = 0;

	if (fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK | file_flags) < 0)
		end("fcntl", errno);

	rc = read(STDIN_FILENO, &c, 1);

	if (fcntl(STDIN_FILENO, F_SETFL, file_flags) < 0)
		end("fcntl", errno);

	if (rc < 0) {
		/* no user input */
		return -1;
	}

	/* user has pressed a key */
	return c;
}

int main(int argc, char *argv[])
{
	int		rc;
	team_ios_list_t	*base_line = NULL;
	bool		iter_flag;

	program = argv[0];
	parse_arguments(argc, argv);

	rc = setup_term(false, true);
	if (rc < 0)
		end("setup_term", -rc);

	signal(SIGWINCH, window_changed);
	atexit(cleanup);

	base_line = gather(NULL);
	/*
	 * We should not be displaying zero IOs during first iteration, wait
	 * for a second so that we will have something to display.
	 */
	sleep(1);

	iter_flag = !!iter;
	while (!iter_flag || iter--) {
		if (screen_size_changed) {
			setup_term(true, false);
			screen_size_changed = false;
		}

		base_line = gather(base_line);

		rc = user_input();
		if (rc <= 0) {
			/* no user input, delay the execution */
			fd_set fs;
			struct timeval tv = {delay, 0};

			FD_ZERO(&fs);
			FD_SET(STDIN_FILENO, &fs);

			/* delay until user input or timeout */
			rc = select(1, &fs, NULL, NULL, &tv);
			if (rc < 0) {
				/*
				 * we might have been woken-up because of screen
				 * size changed signal. If it was the case then
				 * screen must be redrawn immediately.
				 */
				rc = 0;
				if (errno != EINTR)
					end("select", errno);
			}

			if (rc) {
				/* input from user is available */
				rc = user_input();
				if (rc < 0)
					end("read", 1);
				if (rc == 0)
					end(NULL, 0);
			}
		}

		if (rc > 0) {
			/* user has pressed a key */
			handle_key(rc);
		}
	}
	return 0;
}
