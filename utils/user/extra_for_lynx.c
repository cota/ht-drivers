/**
 * @file extra_for_lynx.c
 *
 * @brief Functions, needed for Lynx in userspace.
 *
 * @author Yury GEORGIEVSKIY. CERN BE/CO
 *
 * @date Created on 14/02/2009
 *
 * Taken from Linux kernel (vsprintf.c), glibc and uClibc.
 * Exported functions are:
 * 1. strnlen
 * 2. vsnprintf
 * 3. asprintf
 * 4. mkstemp
 *
 * @version
 */
#ifdef __Lynx__

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>	/* for va_arg */
#include <time.h>
#include <string.h>	/* for strlen */
#include <errno.h>
#include <ctype.h>	/* for isdigit */
#include <assert.h>	/* for assert */
#include <sys/types.h>
#include <unistd.h>
#include <file.h>
#include <fcntl.h>

/*
  Already defined in libz.a (bsd-snprintf.o)
  As we compiling with libz.a (for libxml) - no need to implement it
 */
extern int vsnprintf(char*, unsigned, const char*, va_list);

#ifndef va_copy
#define va_copy(d,s) __va_copy(d,s)
#endif

/* PAGE_SHIFT determines the page size */
#define PAGE_SHIFT              12
#define PAGE_SIZE               (1UL << PAGE_SHIFT)
#define PAGE_MASK               (~(PAGE_SIZE-1))

#define ZEROPAD	1		/* pad with zero */
#define SIGN	2		/* unsigned/signed long */
#define PLUS	4		/* show plus */
#define SPACE	8		/* space if plus */
#define LEFT	16		/* left justified */
#define SMALL	32		/* Must be 32 == 0x20 */
#define SPECIAL	64		/* 0x */


#define do_div(n,base)					\
({							\
	int __res;					\
	__res = ((unsigned long) n) % (unsigned) base;	\
	n = ((unsigned long) n) / (unsigned) base;	\
	__res;						\
})

static int __attribute__((unused)) skip_atoi(const char **s)
{
	int i=0;

	while (isdigit(**s))
		i = i*10 + *((*s)++) - '0';
	return i;
}


/* Decimal conversion is by far the most typical, and is used
 * for /proc and /sys data. This directly impacts e.g. top performance
 * with many processes running. We optimize it for speed
 * using code from
 * http://www.cs.uiowa.edu/~jones/bcd/decimal.html
 * (with permission from the author, Douglas W. Jones). */

/* Formats correctly any integer in [0,99999].
 * Outputs from one to five digits depending on input.
 * On i386 gcc 4.1.2 -O2: ~250 bytes of code. */
static char* put_dec_trunc(char *buf, unsigned q)
{
	unsigned d3, d2, d1, d0;
	d1 = (q>>4) & 0xf;
	d2 = (q>>8) & 0xf;
	d3 = (q>>12);

	d0 = 6*(d3 + d2 + d1) + (q & 0xf);
	q = (d0 * 0xcd) >> 11;
	d0 = d0 - 10*q;
	*buf++ = d0 + '0'; /* least significant digit */
	d1 = q + 9*d3 + 5*d2 + d1;
	if (d1 != 0) {
		q = (d1 * 0xcd) >> 11;
		d1 = d1 - 10*q;
		*buf++ = d1 + '0'; /* next digit */

		d2 = q + 2*d2;
		if ((d2 != 0) || (d3 != 0)) {
			q = (d2 * 0xd) >> 7;
			d2 = d2 - 10*q;
			*buf++ = d2 + '0'; /* next digit */

			d3 = q + 4*d3;
			if (d3 != 0) {
				q = (d3 * 0xcd) >> 11;
				d3 = d3 - 10*q;
				*buf++ = d3 + '0';  /* next digit */
				if (q != 0)
					*buf++ = q + '0';  /* most sign. digit */
			}
		}
	}
	return buf;
}
/* Same with if's removed. Always emits five digits */
static char* put_dec_full(char *buf, unsigned q)
{
	/* BTW, if q is in [0,9999], 8-bit ints will be enough, */
	/* but anyway, gcc produces better code with full-sized ints */
	unsigned d3, d2, d1, d0;
	d1 = (q>>4) & 0xf;
	d2 = (q>>8) & 0xf;
	d3 = (q>>12);

	/* Possible ways to approx. divide by 10 */
	/* gcc -O2 replaces multiply with shifts and adds */
	// (x * 0xcd) >> 11: 11001101 - shorter code than * 0x67 (on i386)
	// (x * 0x67) >> 10:  1100111
	// (x * 0x34) >> 9:    110100 - same
	// (x * 0x1a) >> 8:     11010 - same
	// (x * 0x0d) >> 7:      1101 - same, shortest code (on i386)

	d0 = 6*(d3 + d2 + d1) + (q & 0xf);
	q = (d0 * 0xcd) >> 11;
	d0 = d0 - 10*q;
	*buf++ = d0 + '0';
	d1 = q + 9*d3 + 5*d2 + d1;
		q = (d1 * 0xcd) >> 11;
		d1 = d1 - 10*q;
		*buf++ = d1 + '0';

		d2 = q + 2*d2;
			q = (d2 * 0xd) >> 7;
			d2 = d2 - 10*q;
			*buf++ = d2 + '0';

			d3 = q + 4*d3;
				q = (d3 * 0xcd) >> 11; /* - shorter code */
				/* q = (d3 * 0x67) >> 10; - would also work */
				d3 = d3 - 10*q;
				*buf++ = d3 + '0';
					*buf++ = q + '0';
	return buf;
}
/* No inlining helps gcc to use registers better */
static char* put_dec(char *buf, unsigned long long num)
{
	while (1) {
		unsigned rem;
		if (num < 100000)
			return put_dec_trunc(buf, num);
		rem = do_div(num, 100000);
		buf = put_dec_full(buf, rem);
	}
}


static char __attribute__((unused)) *number(char *buf, char *end, unsigned long long num, int base, int size, int precision, int type)
{
	/* we are called with base 8, 10 or 16, only, thus don't need "G..."  */
	static const char digits[16] = "0123456789ABCDEF"; /* "GHIJKLMNOPQRSTUVWXYZ"; */

	char tmp[66];
	char sign;
	char locase;
	int need_pfx = ((type & SPECIAL) && base != 10);
	int i;

	/* locase = 0 or 0x20. ORing digits or letters with 'locase'
	 * produces same digits or (maybe lowercased) letters */
	locase = (type & SMALL);
	if (type & LEFT)
		type &= ~ZEROPAD;
	sign = 0;
	if (type & SIGN) {
		if ((signed long long) num < 0) {
			sign = '-';
			num = - (signed long long) num;
			size--;
		} else if (type & PLUS) {
			sign = '+';
			size--;
		} else if (type & SPACE) {
			sign = ' ';
			size--;
		}
	}
	if (need_pfx) {
		size--;
		if (base == 16)
			size--;
	}

	/* generate full string in tmp[], in reverse order */
	i = 0;
	if (num == 0)
		tmp[i++] = '0';
	/* Generic code, for any base:
	else do {
		tmp[i++] = (digits[do_div(num,base)] | locase);
	} while (num != 0);
	*/
	else if (base != 10) { /* 8 or 16 */
		int mask = base - 1;
		int shift = 3;
		if (base == 16) shift = 4;
		do {
			tmp[i++] = (digits[((unsigned char)num) & mask] | locase);
			num >>= shift;
		} while (num);
	} else { /* base 10 */
		i = put_dec(tmp, num) - tmp;
	}

	/* printing 100 using %2d gives "100", not "00" */
	if (i > precision)
		precision = i;
	/* leading space padding */
	size -= precision;
	if (!(type & (ZEROPAD+LEFT))) {
		while(--size >= 0) {
			if (buf < end)
				*buf = ' ';
			++buf;
		}
	}
	/* sign */
	if (sign) {
		if (buf < end)
			*buf = sign;
		++buf;
	}
	/* "0x" / "0" prefix */
	if (need_pfx) {
		if (buf < end)
			*buf = '0';
		++buf;
		if (base == 16) {
			if (buf < end)
				*buf = ('X' | locase);
			++buf;
		}
	}
	/* zero or space padding */
	if (!(type & LEFT)) {
		char c = (type & ZEROPAD) ? '0' : ' ';
		while (--size >= 0) {
			if (buf < end)
				*buf = c;
			++buf;
		}
	}
	/* hmm even more zero padding? */
	while (i <= --precision) {
		if (buf < end)
			*buf = '0';
		++buf;
	}
	/* actual digits of result */
	while (--i >= 0) {
		if (buf < end)
			*buf = tmp[i];
		++buf;
	}
	/* trailing space padding */
	while (--size >= 0) {
		if (buf < end)
			*buf = ' ';
		++buf;
	}
	return buf;
}


/**
 * strnlen - Find the length of a length-limited string
 * @s: The string to be sized
 * @count: The maximum number of bytes to search
 */
int strnlen(const char *s, size_t count)
{
        const char *sc;

        for (sc = s; count-- && *sc != '\0'; ++sc)
                /* nothing */;
        return sc - s;
}


static char __attribute__((unused)) *string(char *buf, char *end, char *s, int field_width, int precision, int flags)
{
	int len, i;

	if ((unsigned long)s < PAGE_SIZE)
		s = "<NULL>";

	len = strnlen(s, precision);

	if (!(flags & LEFT)) {
		while (len < field_width--) {
			if (buf < end)
				*buf = ' ';
			++buf;
		}
	}
	for (i = 0; i < len; ++i) {
		if (buf < end)
			*buf = *s;
		++buf; ++s;
	}
	while (len < field_width--) {
		if (buf < end)
			*buf = ' ';
		++buf;
	}
	return buf;
}


static int vasprintf(char **buf, const char *format, va_list arg)
{
	va_list arg2;
	int rv;

	va_copy(arg2, arg);
 	rv = vsnprintf(NULL, 0, format, arg2);
	va_end(arg2);

	*buf = NULL;

	if (rv >= 0) {
		if ((*buf = malloc(++rv)) != NULL) {
			if ((rv = vsnprintf(*buf, rv, format, arg)) < 0) {
				free(*buf);
				*buf = NULL;
			}
		}
	}

	assert(rv >= -1);

	return rv;
}



int asprintf(char **buf, const char *format, ...)
{
	va_list arg;
	int rv;

	va_start(arg, format);
	rv = vasprintf(buf, format, arg);
	va_end(arg);

	return rv;
}



/* These are the characters used in temporary filenames.  */
static const char letters[] =
"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

/* Generate a temporary file name based on TMPL.  TMPL must match the
   rules for mk[s]temp (i.e. end in "XXXXXX").  The name constructed
   does not exist at the time of the call to mkstemp().  TMPL is
   overwritten with the result.  Creates the file and returns a read-write
   fd; the file is mode 0600 modulo umask.

   We use a clever algorithm to get hard-to-predict names. */
int mkstemp(char *tmpl)
{
  int len;
  char *XXXXXX;
  static unsigned int value;
  struct timeval tv;
  int count, fd;
  int save_errno = errno;

  len = strlen (tmpl);
  if (len < 6 || strcmp (&tmpl[len - 6], "XXXXXX"))
    {
      errno = EINVAL;
      return -1;
    }

  /* This is where the Xs start.  */
  XXXXXX = &tmpl[len - 6];

  /* Get some more or less random data.  */
  gettimeofday (&tv, NULL);
  value += ((unsigned int) tv.tv_usec << 16) ^ tv.tv_sec ^ getpid ();

  for (count = 0; count < TMP_MAX; value += 7777, ++count)
    {
      unsigned int v = value;

      /* Fill in the random bits.  */
      XXXXXX[0] = letters[v % 62];
      v /= 62;
      XXXXXX[1] = letters[v % 62];
      v /= 62;
      XXXXXX[2] = letters[v % 62];
      v /= 62;
      XXXXXX[3] = letters[v % 62];
      v /= 62;
      XXXXXX[4] = letters[v % 62];
      v /= 62;
      XXXXXX[5] = letters[v % 62];

      fd = open (tmpl, O_RDWR | O_CREAT /*| O_EXCL*/, 0660);

      if (fd >= 0)
	{
          errno = save_errno;
          return fd;
        }
      else if (errno != EEXIST)
        /* Any other error will apply also to other names we might
           try, and there are 2^32 or so of them, so give up now. */
        return -1;
    }

  /* We got out of the loop because we ran out of combinations to try.  */
  errno = EEXIST;
  return -1;
}

#endif /* __Lynx__ */
