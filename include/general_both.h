/**
 * @file general_both.h
 *
 * @brief Contains helpful definitions that are used both,
 *        by the drivers and in the user space.
 *
 * @author Copyright (C) 2009 CERN. Yury GEORGIEVSKIY <ygeorgie@cern.ch>
 *
 * @date Created on 27/02/2004
 */
#ifndef _GENERAL_BOTH_HEADER_H_INCLUDE_
#define _GENERAL_BOTH_HEADER_H_INCLUDE_

#include <lynx_ioctls.h>
#include <stddef.h> /* both, Lynx && Linux have them */

#define MAX_UCA      (16+1)     /**< MAX allowed user contexts
				   (16) + 1 for service */
#define MIN_DEV_SUPP 0	        /**< device bottom limit */
#define MAX_DEV_SUPP 21	        /**< MAX supported devices */
#define _MAGIC_      0x79757261	/**< can be used for smthng [yura] (-; */

/**< get MIN */
#ifndef MIN
#define MIN(X,Y) (((X) < (Y)) ? (X) : (Y))
#endif

/**< get MAX */
#ifndef MAX
#define MAX(X,Y) (((X) < (Y)) ? (Y) : (X))
#endif

/**< size of an array */
#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif

/**< macro that returns true if MIN <= ARG <= MAX */
#define WITHIN_RANGE(MIN,ARG,MAX) ( ((MAX) >= (ARG)) && ((MIN) <= (ARG)) )


/*! @name macro that limits a value between two bounds
 *
 * @b NOTE Value in question will be changed!
 */
#define LIMIT(LOWER_LIMIT,X,UPPER_LIMIT) ((X) = MAX((LOWER_LIMIT),MIN((UPPER_LIMIT),(X))))

/*! @name Nice coloring
 */
/*@{*/
#define RED_CLR	    "\033[0;31m"
#define WHITE_CLR   "\033[1;37m"
#define END_CLR	    "\033[m"
#define ERR_MSG	    "\033[0;31mERROR->\033[m"
#define WARNING_MSG "\033[0;31mWARNING->\033[m"
/*@}*/


/**< memory initialization macro */
#ifndef memzero
#define memzero(s, n) memset ((s), 0, (n))
#endif


/*! @name bitprinting */
#define bitprint(x)							    \
({									    \
  int __bc, __sc;							    \
  char __bit_str[256] = { 0 }, atom;					    \
  for (__bc = sizeof(typeof(x))*8-1, __sc = 0; __bc >= 0; __bc--, __sc++) { \
    atom = ((x&(1<<__bc))>>__bc)?'1':'0';				    \
    __bit_str[__sc] = atom;						    \
  }									    \
  __bit_str;								    \
})


/*! @name bit counter */
#define bitcntr(x)				\
({						\
  int __cntr = 0;				\
  while (x) {					\
    if (x&1)					\
      __cntr++;					\
    x >>= 1;					\
  }						\
  __cntr;					\
})


/*! @name which bits are set */
#define getbitidx(x, m)				\
do {						\
  int __cntr = 0, __mcntr = 0;			\
  typeof(x) __x = (x);				\
						\
  while(__x) {					\
    if (__x&1) {				\
      m[__mcntr] = __cntr;			\
      __mcntr++;				\
    }						\
    __x >>= 1;					\
    __cntr++;					\
  }						\
} while (0)


/**
 * @brief Constructs a string representation of an integer for different bases.
 *
 * @param val:  value to convert to string
 * @param base: base of the value [2 - 16]
 *
 * Described by Robert Jan Schaper
 *
 * @return pointer to local buffer with string representation.
 */
static inline char* _itoa(int val, int base)
{
  static char buf[34];
  char* out = &buf[33];
  int sign = (val < 0)?sign = -val:val;

  /* check if the base is valid */
  if (base < 2 || base > 16)
    return(NULL);

  memset(buf, 0, sizeof(buf));	/* reinit */

  /* base 16 and base 2 cases */
  if (base == 16 || base == 2) {
    unsigned int hval = val;
    unsigned int hbase = base;
    do {
      *out = "0123456789abcdef"[hval % hbase];
      --out;
      hval /= hbase;
    } while(hval);


    if (base == 16) /* apply 0x prefix */
      *out-- = 'x', *out = '0';
    else
      ++out;

    return(out);
  }

  /* for all remaining bases */
  do {
    *out = "0123456789abcdef"[sign % base];
    --out;
    sign /= base;
  } while(sign);

  if ( val < 0 && base == 10) /* apply negative sign only for base 10 */
    *out = '-';
  else
    ++out;

  return(out);
}

#endif /* _GENERAL_BOTH_HEADER_H_INCLUDE_ */
