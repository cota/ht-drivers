/**
 * @file lite_shell.h
 *
 * @brief Lite shell header
 *
 * @author Copyright (C) 2008 CERN CO/HT Yury Georgievskiy
 *					<yury.georgievskiy@cern.ch>
 *
 * @section license_sec License
 * Released under the GPL v2. (and only v2, not any later version)
 */
#ifndef _LITE_SHELL_H_INCLUDE_
#define _LITE_SHELL_H_INCLUDE_

/* MAX size in bytes command line will hold (+1 for termination zero) */
#define DSZ (128 + 1)

/* how many commands in the history buffer !ONLY NUMBERS DIVISIBLE BY 2! */
#define HIS_SZ 32

#define FVL 2 /* first valid index of the built-up history buffer */

/* history buffer */
#define HIS_LAST_IDX (HIS_SZ - 1)
#define HIS_MASK     (HIS_SZ - 1)
#define HIS_LOOPED   (his_curr & ~HIS_MASK)
#define HIS_CUR      (his_curr & HIS_MASK)
#define HIS_EMPTY    (-1)

/* alert */
#define _BELL { printf("\a"); fflush(stdout); }

/* one history line */
typedef char hel_t[DSZ];

hel_t* get_history_list(hel_t (*)[HIS_SZ + 3]);
int    init_keyboard(void);
void   close_keyboard(void);
char*  get_command(char*);

#endif /* _LITE_SHELL_H_INCLUDE_ */
