/**
 * @file lite_shell.c
 *
 * @brief Lite shell (lsh) provides a simple command-line interface (CLI)
 *
 * Its main features are history (managed via the up/down arrow keys) and
 * backspace and left/right cursor keys support.
 * There are many things that could be added -- but we have to bear in mind
 * that is has to be compatible with our current Linux and LynxOS systems --
 * and because of the latter this program is based on the curses library.
 * In the future (when we get rid of LynxOS completely) we should extend
 * lsh to use ncurses -- which is much nicer to work with than curses.
 *
 * @author Copyright (C) 2009 CERN CO/HT Emilio G. Cota
 * 					<emilio.garcia.cota@cern.ch>
 * @author Copyright (C) 2008 CERN CO/HT Yury Georgievskiy
 *					<yury.georgievskiy@cern.ch>
 *
 * @section license_sec License
 * Released under the GPL v2. (and only v2, not any later version)
 */
#include <stdlib.h>
#include <stdio.h>
#include <curses.h>

#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <curses.h>

#include <lite_shell.h>

extern char *tgetstr(char*, char**);
extern int  tgetent(char*, char*);
extern char *tgetstr(char *id, char **area);
extern int tputs(const char *str, int affcnt, int (*putc)(int));

/* see [ man termios && terminfo ] for further help */
static struct termios initial_settings, new_settings;
char k_up[3]    = { 0x1b, 0x5b, 0x41 }; /* up-arrow   key esc sequence */
char k_down[3]  = { 0x1b, 0x5b, 0x42 }; /* down-arrow key esc sequence */
char k_right[3] = { 0x1b, 0x5b, 0x43 };	/* right-arrow key esc sequence */
char k_left[3]  = { 0x1b, 0x5b, 0x44 };	/* left-arrow key esc sequence */
unsigned short k_back = 0x7f;		/* backspace */

hel_t hist_buffer[HIS_SZ] = { { 0 }, { 0 } };
int   his_cntr            = HIS_EMPTY; /* history counter index */
int   his_curr            = HIS_EMPTY; /* where command will go */
int   his_active          = 0; /* history flag */

/* command line data */
char cl_data[DSZ]; /* current command line data */
int  d_pos = 0;    /* current position in the command line string */
int  d_pos_max = 0; /* MAX position in the command line string */

/* erase command line */
char white  = ' ';
char backsp = '\b';

/* if keyboard was initialized */
int keyboard_open = 0;

/**
 * clean_prompt - clean the line the user is typing on
 */
static void clean_prompt(void)
{
	int i;

	/* move cursor until the end of already entered line */
	for (i = d_pos; i <= d_pos_max; i++)
		printf("%s", tgetstr("nd", NULL));
	fflush(stdout);
	/* delete the line (see terminfo manpage) */
	for (i = d_pos_max; i >= 0; i--)
		printf("%s%s", tgetstr("le", NULL), tgetstr("dc", NULL));
	fflush(stdout);
	d_pos = 0;
}

/*
  Local history buffer implementation.
  This is how 'prepared for parsing' history list looks like.
  |<--------- VALID  HISTORY -------->|
  |                                   |
  +------+------+-----+-----+-----+     +-----+-----+------+
  | ZERO | ZERO |     |     |     |     |     |     | ZERO |
  | ---->|      |     |     |     |     | --->|     |      |
  |      |      |     |     |     | ... |     |     |      |
  +------+------+-----+-----+-----+     +-----+-----+------+
  0      1      2     3     4           5     6     7
  ----> - are start data pointer (right one) and end pointer (left one)
*/

/**
 * get_history_list - Build-up command history list
 *
 * @param hist - address, where to put build-up history list that is prepared
 *               for parsing, or NULL if caller just wants current command
 *               history list and which will not be valid for parsing
 *
 * @return pointer to the 'prepared for parsing' history list if @ref hist
 * is not NULL, or pointer to the history list with all commands available
 */
hel_t* get_history_list(hel_t (*hist)[HIS_SZ + 3])
{
	static hel_t h_buf[HIS_SZ] = { { 0 }, { 0 } }; /* all hist commands */
	hel_t 	*h = NULL;
	int 	vh_cntr = FVL; /* first valid index of the history buffer */

	if (!hist) { /* caller just wants commands from the history buffer */
		int i, j;
		bzero((void *)h_buf, sizeof(h_buf));
		for (i = 0, j = his_cntr; j >=0; i++, j--)
			strcpy(h_buf[i], hist_buffer[j]);
		return h_buf;
	}
	bzero((void *)hist, sizeof(*hist));
	h = hist[0];
	if (his_cntr == HIS_EMPTY)
		return &h[FVL - 1];
	if (HIS_LOOPED) {
		do {
			strcpy(h[vh_cntr++], hist_buffer[his_cntr--]);
			if (his_cntr < 0)
				his_cntr = HIS_LAST_IDX;
		} while (his_cntr != HIS_CUR);
	} else {
		while (his_cntr >= 0)
			strcpy(h[vh_cntr++], hist_buffer[his_cntr--]);
	}
	his_active = 1;
	return &h[FVL - 1];
}

/**
 * handle_history - This function is called as soon as 'up' or 'down' arrows
 * are pressed by the user
 *
 * @param +1 - 'down-arrow' is pressed, -1 - 'up-arrow' is pressed.
 */
static void handle_history(int direction)
{
	static hel_t valid_history[HIS_SZ + 3] = { { 0 }, { 0 } };
	static hel_t *vh_ptr;

	if (!his_active) {
		vh_ptr = get_history_list(&valid_history);
		if (!(**(vh_ptr + 1))) {
			_BELL;
			return; /* no history at all */
		}
	}
	bzero(cl_data, sizeof(cl_data)); /* clear current command line data */
	if (direction > 0) { /* history seek forward [down-key] */
		if (**vh_ptr) {
			vh_ptr--;
			clean_prompt();
			if (**vh_ptr) {	/* still have history */
				d_pos = printf("%s", *vh_ptr);
				d_pos_max = d_pos;
				strcpy(cl_data, *vh_ptr);
				fflush(stdout);
			} else {
				bzero(cl_data, sizeof(cl_data));
				d_pos_max = 0;
			}
		} else
			_BELL; /* no more history */
	} else { /* history seek backwards [up-key] */
		if (**(vh_ptr + 1))
			vh_ptr++;
		else
			_BELL; /* no more history */
		if (**vh_ptr) {
			clean_prompt();
			d_pos = printf("%s", *vh_ptr);
			d_pos_max = d_pos;
			strcpy(cl_data, *vh_ptr);
			fflush(stdout);
		}
	}
	return ;
}

/**
 * move_cursor - handle left/right movement of the cursor
 *
 * @param dir: Direction to move. [ Negative - to the left  <- ]
 *                                [ Positive - to the right -> ]
 */
static void move_cursor(int dir)
{
	/*
	 * see man terminfo. check for:
	 * 1. cub1(le)[cursor_left]
	 * 2. cuf1(nd)[cursor_right]
	 * 3. dch1(dc)[delete_character]
	 */
	if (dir < 0) { /* <-- left <-- */
		if (d_pos) {
			printf("%s", tgetstr("le", NULL));
			d_pos--;
		} else
			_BELL;
	} else {
		if (d_pos < d_pos_max) {
			printf("%s", tgetstr("nd", NULL));
			d_pos++;
		} else
			_BELL;
	}
	fflush(stdout);
}

/**
 * kbhit - handler to read a keyboard hit
 *
 * @return character read or 0 if it was '\n'
 */
static char kbhit(void)
{
	int	nread;
	char 	ch[3] = { 0 };

	nread = read(0, ch, 1);
	if (!nread) { /* EOF: exit test program */
		printf("\n");
		exit(1);
	}
	switch (ch[0]) {
	case '\033':
		nread = read(0, ch + 1, 1);
		if (!nread) { /* EOF: exit test program */
			printf("\n");
			exit(1);
		}
		nread = read(0, ch + 2, 1);
		if (!nread) { /* EOF: exit test program */
			printf("\n");
			exit(1);
		}
		if (ch[1] == k_up[1] && ch[2] == k_up[2])
			handle_history(-1);
		else if (ch[1] == k_down[1] && ch[2] == k_down[2])
			handle_history(+1);
		else if (ch[1] == k_right[1] && ch[2] == k_right[2])
			move_cursor(+1);
		else if (ch[1] == k_left[1] && ch[2] == k_left[2])
			move_cursor(-1);
		return ch[0];
	case '\177': /* backspace */
		return '\b';
	case '\n':
		if (his_curr == HIS_EMPTY) /* extreme case */
			return 0;
		else
			his_cntr = (his_curr - 1) & HIS_MASK;
		his_active = 0;
		return 0;
	case '\t': /* @todo tab handler -- should have a 'cmd completion' */
		return '\033'; /* not yet implemented */
	default:
		return ch[0];
	}
}

/**
 * init_keyboard - initialise terminal and keyboard
 *
 * @return 0 - on success
 * @return 1 - if there's been an error
 */
int init_keyboard(void)
{
	char *terminalType;
	char  term_entry[1024];

	terminalType = (char *)getenv("TERM");
	/* load entry for the current terminal */
	if (tgetent(term_entry, getenv("TERM")) != 1) {
		printf("Can't get entry for %s\n", terminalType);
		return 1;
	}
	tcgetattr(0, &initial_settings); /* save initial terminal config */
	new_settings = initial_settings;
	new_settings.c_lflag &= ~ICANON; /* non-canonical mode */
	new_settings.c_lflag &= ~ECHO;
	/*
	 * see "Linux Programming 2nd edition" pg 173.
	 * we'll catch up-arrow key or down-arrow key escape sequences
	 */
	new_settings.c_cc[VMIN] = 1;
	new_settings.c_cc[VTIME] = 0; /* 0 sec */
	tcsetattr(0, TCSANOW, &new_settings);
	keyboard_open = 1;
	return 0;
}

/**
 * close_keyboard - restore initial terminal settings
 */
void close_keyboard(void)
{
	if (keyboard_open)
		tcsetattr(0, TCSANOW, &initial_settings);
}

/**
 * update_history - called when cl_data has new stuff
 */
static void update_history(void)
{
	if (!strlen(cl_data))
		return ;
	if (his_curr == HIS_EMPTY) /* very first time */
		his_curr = 0;
	/* don't store a command if it's equal than the previous one */
	if (strcmp((char*)get_history_list(NULL), cl_data)) {
		strcpy(hist_buffer[HIS_CUR], cl_data);
		his_cntr = HIS_CUR;
		his_curr++;
	}
	his_active = 0;
	d_pos_max = 0;
}

/**
 * get_command - Gets what user just typed or extracts from the history buffer
 *
 * @param str - command line prompt to display
 *
 * @return extracted character string (global @ref cl_data)
 */
char* get_command(char *str)
{
	char ch = 0;
	int didx, sidx; /* destination/source indexes */

	printf(str);
	fflush(stdout);
	bzero(cl_data, sizeof(cl_data));
	d_pos = 0;
	d_pos_max = 0;

	while ((ch = kbhit())) {
		switch (ch) {
		case '\b':
			if (!d_pos) {
				_BELL; /* nothing else to delete */
				break;
			}
			/* move left and delete character from the screen */
			printf("%s%s", tgetstr("le", NULL),
							tgetstr("dc", NULL));
			fflush(stdout);
			sidx = d_pos;
			didx = d_pos - 1;
			while (cl_data[sidx])
				cl_data[didx++] = cl_data[sidx++];
			cl_data[didx] = 0;
			d_pos--;
			d_pos_max--;
			break;
		case '\033': /* ignore this; already handled by kbhit */
			break;
		default:
			if (d_pos >= DSZ-1) {
				_BELL; /* command line buffer is full */
				break;
			}
			/* enter insert mode */
			tputs(tgetstr("im", NULL), 1, putchar);
			if (d_pos != d_pos_max) {
				/* user's correcting previous input */
				if (d_pos_max + 1 == DSZ) {
					_BELL;
					break;
				}
				/* insert + shift to the right */
				memmove(&cl_data[d_pos + 1], &cl_data[d_pos],
					strlen(&cl_data[d_pos]));
				cl_data[d_pos] = ch;
				putchar(ch);
				fflush(stdout);
				d_pos++;
				d_pos_max++;
			} else {
				/* user's inserting new data */
				putchar(ch);
				fflush(stdout);
				cl_data[d_pos] = ch;
				d_pos_max = ++d_pos;
			}
			/* leave insert mode */
			tputs(tgetstr("ei", NULL), 1, putchar);
			fflush(stdout);
		}
	}
	printf("\n");
	update_history();
	return cl_data;
}
