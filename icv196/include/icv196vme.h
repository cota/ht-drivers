/*
  _________________________________________________________________
 |                                                                 |
 |   file name :        icv196vme.h                                |
 |                                                                 |
 |   created:     26-mar-1992 Alain Gagnaire, F. Berlin            |
 |   last update: 21-mar-1992 A. gagnaire                          |
 |_________________________________________________________________|
 |                                                                 |
 |   user Interface header file for the icv196vme driver           |
 |                                                                 |
 |_________________________________________________________________|

 updated:
 =======
 21-mar-1992 A.G., F.B.: version integrated with the event handler common to
                         sdvme, and fpiplsvme driver
 10-may-1994 A.G.: update to stand 8 modules

*/
#include "icv196vmelib.h"
#include <Z8536CIO.h>		/* definition for the chip Z8536CIO */


#define icv_AM      AM_Standard	/* mode of adressing */
#define icv_Psize   D16 	/* data port size supported */
#define icv_VmeSz   0x0100	/* size of adressable space from base */

#define ICV_AddLSBMask 0x000000ff /* VME offset Least Sign Bits mask */
#define ICV_AddMSBMask 0xff000000 /* VME offset Least Sign Bits mask */


#define icv_cumulative  1	/* cumulative mode */
#define icv_queuleuleu  2	/* "a la queue leu leu" file */

#define icv_ReenableOn  1	/* line management: automatic reenable */
#define icv_ReenableOff 2	/* line management: disable auto reenable */

#define icv_FpiLine 1		/* Type of a line special and private */
#define icv_IcvLine 2		/* Type of a normal line */

#define icv_Enabled  1		/* Status for line enabled */
#define icv_Disabled 0		/* Status for line disabled */

/* line operation mode flags */
#define icv_cumul   (1 << 0)	/*
				  Subscriber mode.

				   ON -- cumulative mode
				   only one event from a given trigger source is present in the ring,
				   At arrival of the next trigger, if event not already read, no event is put
				   in the ring, it only count the occurance in the subscriber table.

				   0FF -- normal mode
				   each trigger give one event in the ring buffer
				*/

#define icv_disable (1 << 1)	/*
				  interrupt mode.
				  ON  -- line is disabled after each interrupt
				  OFF -- line stays enabled after the interrupt
				*/

/* structure of an event */
union icvU_Evt {
	unsigned long All;
	struct {
		unsigned short w1, w2;
	} Word;
#if 0
	struct {
		unsigned char b1, b2, b3, b4;
	    } Byte;
#endif
};

/* Internal structure of Atom stuffed in the ring */
struct icvT_RingAtom {
	struct T_Subscriber *Subscriber;
	union  icvU_Evt Evt;
};

/* Structure to address hardware of the icvpls vme module */
/* structures linked to ioctl interface */

/*
  Logical line address of event sources:
  according to maximum configuration
  the number of logical lines declared may be different than
  the number of the physical lines.

  Beware: value are written in octal representation i.e.: 077 form
*/

/* Group 0 */
#define ICV_L101  0000
#define ICV_L102  0001
#define ICV_L103  0002
#define ICV_L104  0003
#define ICV_L105  0004
#define ICV_L106  0005
#define ICV_L107  0006
#define ICV_L108  0007
#define ICV_L109  0010
#define ICV_L110  0011
#define ICV_L111  0012
#define ICV_L112  0013
#define ICV_L113  0014
#define ICV_L114  0015
#define ICV_L115  0016
#define ICV_L116  0017

/* Group 1 */
#define ICV_L201  0020
#define ICV_L202  0021
#define ICV_L203  0022
#define ICV_L204  0023
#define ICV_L205  0024
#define ICV_L206  0025
#define ICV_L207  0026
#define ICV_L208  0027
#define ICV_L209  0030
#define ICV_L210  0031
#define ICV_L211  0032
#define ICV_L212  0033
#define ICV_L213  0034
#define ICV_L214  0035
#define ICV_L215  0036
#define ICV_L216  0037

/* Group 2 */
#define ICV_L301  0040
#define ICV_L302  0041
#define ICV_L303  0042
#define ICV_L304  0043
#define ICV_L305  0044
#define ICV_L306  0045
#define ICV_L307  0046
#define ICV_L308  0047
#define ICV_L309  0050
#define ICV_L310  0051
#define ICV_L311  0052
#define ICV_L312  0053
#define ICV_L313  0054
#define ICV_L314  0055
#define ICV_L315  0056
#define ICV_L316  0057

/* Group 3 */
#define ICV_L401  0060
#define ICV_L402  0061
#define ICV_L403  0062
#define ICV_L404  0063
#define ICV_L405  0064
#define ICV_L406  0065
#define ICV_L407  0066
#define ICV_L408  0067
#define ICV_L409  0070
#define ICV_L410  0071
#define ICV_L411  0072
#define ICV_L412  0073
#define ICV_L413  0074
#define ICV_L414  0075
#define ICV_L415  0076
#define ICV_L416  0077

/* Group 4 */
#define ICV_L501  0100
#define ICV_L502  0101
#define ICV_L503  0102
#define ICV_L504  0103
#define ICV_L505  0104
#define ICV_L506  0105
#define ICV_L507  0106
#define ICV_L508  0107
#define ICV_L509  0110
#define ICV_L510  0111
#define ICV_L511  0112
#define ICV_L512  0113
#define ICV_L513  0114
#define ICV_L514  0115
#define ICV_L515  0116
#define ICV_L516  0117

/* Group 5 */
#define ICV_L601  0120
#define ICV_L602  0121
#define ICV_L603  0122
#define ICV_L604  0123
#define ICV_L605  0124
#define ICV_L606  0125
#define ICV_L607  0126
#define ICV_L608  0127
#define ICV_L609  0130
#define ICV_L610  0131
#define ICV_L611  0132
#define ICV_L612  0133
#define ICV_L613  0134
#define ICV_L614  0135
#define ICV_L615  0136
#define ICV_L616  0137

/* Group 6 */
#define ICV_L701  0140
#define ICV_L702  0141
#define ICV_L703  0142
#define ICV_L704  0143
#define ICV_L705  0144
#define ICV_L706  0145
#define ICV_L707  0146
#define ICV_L708  0147
#define ICV_L709  0150
#define ICV_L710  0151
#define ICV_L711  0152
#define ICV_L712  0153
#define ICV_L713  0154
#define ICV_L714  0155
#define ICV_L715  0156
#define ICV_L716  0157

/* Group 7 */
#define ICV_L801  0160
#define ICV_L802  0161
#define ICV_L803  0162
#define ICV_L804  0163
#define ICV_L805  0164
#define ICV_L806  0165
#define ICV_L807  0166
#define ICV_L808  0167
#define ICV_L809  0170
#define ICV_L810  0171
#define ICV_L811  0172
#define ICV_L812  0173
#define ICV_L813  0174
#define ICV_L814  0175
#define ICV_L815  0176
#define ICV_L816  0177

#define ICV_mapByteSz (icv_ModuleNb << 1) /* 2 byte per module; 16 lines */

/* Index value */
#define ICV_Index00 000
#define ICV_Index01 001
#define ICV_Index02 002
#define ICV_Index03 003
#define ICV_Index04 004
#define ICV_Index05 005
#define ICV_Index06 006
#define ICV_Index07 007
#define ICV_Index08 010
#define ICV_Index09 011
#define ICV_Index10 012
#define ICV_Index11 013
#define ICV_Index12 014
#define ICV_Index13 015
#define ICV_Index14 016
#define ICV_Index15 017
#define ICV_Index16 020

/* UserMode flags */
#define icv_bitwait (1 << 0) /* 1 -- wait on read for LAM, 0 -- nowait */
