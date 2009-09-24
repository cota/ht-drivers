/**
 * @file general_usr.h
 *
 * @brief General user-space definitions.
 *
 * @author Yury GEORGIEVSKIY. CERN AB/CO.
 *
 * @date June, 2008
 *
 * <long description>
 *
 * @version 1.0  ygeorige  27/06/2008  Creation date.
 */
#ifndef _GENERAL_USR_HEADER_H_INCLUE_
#define _GENERAL_USR_HEADER_H_INCLUE_

#include <elf.h>    /* for endianity business */
#include <stdio.h>  /*  */
#include <stdarg.h> /*  */
#include <stdlib.h> /* for abort */
#include <errno.h>  /* for errno */
#include <string.h> /* for memset */
#include <ctype.h>

/* shut up Lynx gcc */
extern int vsnprintf(char *, unsigned, const char*, va_list);

#define streq(a,b) (strcmp((a),(b)) == 0)

//!< Linux kernel errors, hidden from the user (see usr_utils.c).
typedef struct _tag_hidden_errors {
  int   lke_val;  //!< its value
  char *lke_name; //!< readable name
  char *lke_data; //!< error data
} herr_t;


//!< Errors, user can't see.
static herr_t _lkhe[] = {
  { /* from linux/errno.h */
    .lke_val  = 515,
    .lke_name = "ENOIOCTLCMD",
    .lke_data = "No ioctl command"
  },
  { 0, } /* endlist */
};



/**
 * _NOT_KNOWN - Check, if this is a Linux kernel error, hidden from the user.
 *
 * @_e errno to check
 *
 * @return 1 - it is a hidden one.
 * @return 0 - it's a normal error, that gcc knows about.
 */
static inline int _NOT_KNOWN(int _e)
{
  int cntr = 0;

  while (_lkhe[cntr].lke_val) {
    if (_e == _lkhe[cntr].lke_val)
      return(1);
    ++cntr;
  }

  return(0);
}


/**
 * __perror - Printout Linux kernel error, hidden from the user.
 *
 * @str same as for normal @b perror()
 */
static inline void __perror(char *str)
{
  int cntr = 0;
  while (_lkhe[cntr].lke_val)
    if (errno == _lkhe[cntr].lke_val) {
      printf("%s: %d (%s) - %s\n", str, _lkhe[cntr].lke_val, _lkhe[cntr].lke_name, _lkhe[cntr].lke_data);
      return;
    }
  printf("%s: Unknown error %d\n", str, errno);
}


/**
 * @brief Really handy error printout to report error with @e perror
 *        and user-formatted error string.
 *
 * @param token - @printf - like string
 * @param  ...  - argumets to printout
 *
 * Will build-up user-desired error string and put it as a parameter to
 * @e perror() call.\n
 * For now (as of gcc 3.4.6) perror doesn't know about following linux kernel
 * errors:\n
 * ENOIOCTLCMD  515 - No ioctl command
 *
 * @return void
 */
static inline void mperr(char *token, ...)
{
  char errstr[256];
  va_list ap;
  va_start(ap, token);
  vsnprintf(errstr, sizeof(errstr),  token, ap);
  va_end(ap);
  if (_NOT_KNOWN(errno))
    __perror(errstr);
  else
    perror(errstr);
}

//!< swap bytes
static inline void __endian(const void *src, void *dest, unsigned int size)
{
  unsigned int i;
  for (i = 0; i < size; i++)
    ((unsigned char*)dest)[i] = ((unsigned char*)src)[size - i - 1];
}

/**
 * @brief To find out current endianity.
 *
 * @param none
 *
 * @return current endianity - in case of success,
 *         or generate an abnormal process abort - in case of failure.
 */
static inline int __my_endian(void)
{
  static int my_endian = ELFDATANONE;
  union { short s; char c[2]; } endian_test;

  if (my_endian != ELFDATANONE)	/* already defined */
    return(my_endian);

  endian_test.s = 1;
  if (endian_test.c[1] == 1) my_endian = ELFDATA2MSB;
  else if (endian_test.c[0] == 1) my_endian = ELFDATA2LSB;
  else abort();

  return(my_endian);
}


//!< assert data to Big Endian
#define ASSERT_MSB(x)				\
({						\
  typeof(x) __x;				\
  typeof(x) __val = (x);			\
  if (__my_endian() == ELFDATA2LSB)		\
    __endian(&(__val), &(__x), sizeof(__x));	\
  else						\
    __x = x;					\
  __x;						\
})



/*! @name BE <-> LE convertion
 */
//@{
#define _ENDIAN(x)				\
({						\
  typeof(x) __x;				\
  typeof(x) __val = (x);			\
    __endian(&(__val), &(__x), sizeof(__x));	\
  __x;						\
})
//@}


static __attribute__((unused)) char* str2lower(char *s) {
	char *__tmp = s; /* save pointer */
	while (*s) {
		if (isalpha(*s)) *s = tolower(*s);
		s++;
	}
	return __tmp;
}

#endif /* _GENERAL_USR_HEADER_H_INCLUE_ */
