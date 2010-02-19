/* ======================================= */
/* VD80 Sampler ioctl names                */
/* Tue 10th Feb 2009 BE/CO/HT Julian Lewis */
/* ======================================= */

#include <vd80Drvr.h>

static char *Vd80IoctlNames[] __attribute__((unused)) = {

   "Bad Ioctl number",

   "SET_CLOCK",              /* Set to a Vd80 clock */
   "SET_CLOCK_DIVIDE_MODE",  /* Sub sample or clock */
   "SET_CLOCK_DIVISOR",      /* A 16 bit integer so the lowest frequency is */
			     /*    200000/65535 (16bits 0xFFFF) ~3.05Hz     */
   "SET_CLOCK_EDGE",         /* Set to a Vd80 edge */
   "SET_CLOCK_TERMINATION",  /* Set to a Vd80 termination */

   "GET_CLOCK",
   "GET_CLOCK_DIVIDE_MODE",
   "GET_CLOCK_DIVISOR",
   "GET_CLOCK_EDGE",
   "GET_CLOCK_TERMINATION",

   "SET_TRIGGER",
   "SET_TRIGGER_EDGE",
   "SET_TRIGGER_TERMINATION",

   "GET_TRIGGER",
   "GET_TRIGGER_EDGE",
   "GET_TRIGGER_TERMINATION",

   "SET_ANALOGUE_TRIGGER",
   "GET_ANALOGUE_TRIGGER",

   "GET_STATE",                              /* Returns a Vd80 state */
   "SET_COMMAND",                            /* Set to a Vd80 command */

   "READ_ADC",
   "READ_SAMPLE",

   "GET_POSTSAMPLES",
   "SET_POSTSAMPLES",
   "GET_TRIGGER_CONFIG",
   "SET_TRIGGER_CONFIG",

   "LAST"                                    /* The last IOCTL */
 };
