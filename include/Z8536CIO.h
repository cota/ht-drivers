
/*
  _________________________________________________________________
 |                                                                 |
 |   file name :        Z8536CIO.h                                 |
 |                                                                 |
 |   created: 02-mar-1992 Alain Gagnaire, Fridtjof Berlin          |
 |   updated: 02-mar-1992 A.G.                                     |
 |                                                                 |
 | Define value to use the Z8536CIO chip                           |
 | currently present in the following VME modules:                 |
 |             - sdvme  camac loop vme serial driver               |
 |             - fpiplsvme: front panel and plsTgm  vme interrup   |
 |_________________________________________________________________|

 Updated :
 ========
 20-aug-1991 Fridtjof Berlin (removed uneccessary declarations)

*/
#ifndef _Z8536CIO_
#define _Z8536CIO_

/* ICV196 VME module: offset of interface registers */
#define ICV_AM       0x3d /* A24D16 for ICV module */

#define CSCLR_ICV    0x00 /* Clear ICV (Read/Write 16 bits) */
#define CSGbase_ICV  0x02 /* R/W group base */
#define CSG1G0_ICV   0x02 /* R/W Groups 1,  0   Lines  8-15,  0-7  */
#define CSG3G2_ICV   0x04 /* R/W Group2 3,  2   Lines 24-31, 16-23 */
#define CSG5G4_ICV   0x06 /* R/W Group2 5,  4   Lines 40-47, 32-39 */
#define CSG7G6_ICV   0x08 /* R/W Group2 7,  6   Lines 56-63, 48-55 */
#define CSG9G8_ICV   0x0A /* R/W Group2 9,  8   Lines 72-79, 64-71 */
#define CSG11G10_ICV 0x0C /* R/W Group2 11, 10  Lines 88-95, 80-87 */

#define CSDIR_ICV    0x0E /* To define group dir(in/out): 12-9, 8-1 */

#define PortC_Z8536  0x81 /* Port C of chip Z8536 */
#define PortB_Z8536  0x83 /* Port B of chip Z8536 */
#define PortA_Z8536  0x85 /* Port A of chip Z8536 */
#define CoReg_Z8536  0x87 /* Control Register of chip Z8536 */

#define CSNIT_ICV    0xC1 /* Mask reg. to define int.level used */

/*
		For Programming Z8536 chip,
	 some hints picked up from  description manual
        of the ADAS ICV 196 module (manual in french)
	Definition of register add in ICV196 chip: Z8536 CI/O
	This register are accessed via ICV196 control register:
	The acces to the chip must be done according to chip state

		- It is always possible to read the control register
		  a read set the chip in "state 0"
		- Write must be done in "state 0"
		  first write in state 0 set the register pointer
		  second write access the currently pointed to register
       State diagram:
          <State 0>
                --> Read  -> <state 0>
                --> Write pointer -> <state 1>
          <state 1>
                --> Read -> <state 0>
                --> Write reset bit 1 --> <State reset>
                --> Write data -> <state 0>
          <state reset>
                --> Read -> <state reset>
                --> Write reset bit(0)= 1 --> <State reset>
                --> Write reset bit(0)= 0 --> <State 0>
*/

/* Register address offsets */
#define MIC_reg          0x00 /* Master Interrupt Control register */
#define MCC_reg          0x01 /* Master Config Control    register */
#define ItVct_Areg       0x02 /* Port A Interrupt Vector */
#define ItVct_Breg       0x03 /* Port B Interrupt Vector */
#define CtIVct_reg       0x04 /* Counter/Timer Int. Vector register */
#define DPPol_Creg       0x05 /* Port C Data Path polarity register */
#define DDir_Creg	 0x06 /* Port C Data direction register */
#define SIO_Creg	 0x07 /* Port C Special I/O control register */

#define CSt_Areg	 0x08 /* Port A Command & Status register */
#define CSt_Breg	 0x09 /* Port B Command & Status register */
#define CtCSt_1reg       0x0A /* Counter/Timer 1 Cmd & Status reg. */
#define CtCSt_2reg       0x0B /* Counter/Timer 2 Cmd & Status reg. */
#define CtCSt_3reg       0x0C /* Counter/Timer 3 Cmd & Status reg. */
#define Data_Areg	 0x0D /* Port A data ( directly addressable) */
#define Data_Breg	 0x0E /* Port B data ( directly addressable) */
#define Data_Creg	 0x0F /* Port C data ( directly addressable) */

#define CtCMsb_1	 0x10 /* C/T 1 Current Count Most  sign. bit */
#define CtCLsb_1	 0x11 /* C/T 1 Current Count Least sign. bit */
#define CtCMsb_2	 0x12 /* C/T 2 Current Count Most  sign. bit */
#define CtCLsb_2	 0x13 /* C/T 2 Current Count Least sign. bit */
#define CtCMsb_3	 0x14 /* C/T 3 Current Count Most  sign. bit */
#define CtCLsb_3	 0x15 /* C/T 3 Current Count Least sign. bit */
#define CtTMsb_1	 0x16 /* C/T 1 Cur. Time cstant MsBit        */
#define CtTLsb_1	 0x17 /* C/T 1 Cur. Time constant LsBit      */

#define CtTMsb_2	 0x18 /* C/T 2 Cur. Time constant MsBit      */
#define CtTLsb_2	 0x19 /* C/T 2 Cur. Time constant LsBit      */
#define CtTMsb_3	 0x1A /* C/T 3 Cur. Time constant MsBit      */
#define CtTLsb_3	 0x1B /* C/T 3 Cur. Time constant LsBit      */
#define CtMS_1		 0x1C /* C/T 1 Mode specification */
#define CtMS_2		 0x1D /* C/T 2 Mode specification */
#define CtMS_3		 0x1E /* C/T 3 Mode specification */
#define CurVct_reg	 0x1F /* Current Vector */

#define MSpec_Areg	 0x20 /* Port A Mode Specification register */
#define Hspec_Areg	 0x21 /* Port A Handshake Spec. register */
#define DPPol_Areg	 0x22 /* Port A Data Path polarity register */
#define DDir_Areg	 0x23 /* Port A Data direction register */
#define SIO_Areg	 0x24 /* Port A Special I/O control register */
#define PtrPo_Areg	 0x25 /* Port A Pattern Polarity register */
#define PtrTr_Areg	 0x26 /* Port A Pattern Transition register */
#define PtrMsk_Areg	 0x27 /* Port A Pattern Mask register */

#define MSpec_Breg	 0x28 /* Port B Mode Specification register */
#define HSpec_Breg	 0x29 /* Port B Handshake Spec. register */
#define DPPol_Breg	 0x2A /* Port B Data Path polarity register*/
#define DDir_Breg	 0x2B /* Port B Data direction register */
#define SIO_Breg         0x2C /* Port B Special I/O control register */
#define PtrPo_Breg	 0x2D /* Port B Pattern Polarity register */
#define PtrTr_Breg	 0x2E /* Port B Pattern Transition register */
#define PtrMsk_Breg	 0x2F /* Port B Pattern Mask register */

/* Command code ( bit 7, 6, 5) in byte */
#define CoSt_NULL	 0<<5 /* Null code */
#define CoSt_ClIpIus	 1<<5 /* Clear IP & IUS bits */
#define CoSt_SeIus	 2<<5 /* Set IUS   */
#define CoSt_ClIus	 3<<5 /* Clear IUS */
#define CoSt_SeIp	 4<<5 /* Set IP    */
#define CoSt_ClIp	 5<<5 /* Clear IP  */
#define CoSt_SeIe	 6<<5 /* Set IE    */
#define CoSt_ClIe	 7<<5 /* Clear IE  */

#define CoSt_Ius         1<<7 /* Interrupt under Service (status) */

#define PtrM_OrVect      3<<1 /* Pattern OR prior encoded vect. mode */
#define PtrM_Or          2<<1 /* Pattern OR */
#define PtrM_And         1<<1 /* Pattern AND */
#define PtrM_Dis         0<<1 /* Pattern Disable */

/* Mask bit for setting Master Interrupt Control register */
#define b_0              1
#define b_1              b_0<<1
#define b_2              b_1<<1
#define b_3              b_2<<1
#define b_4              b_3<<1
#define b_5              b_4<<1
#define b_6              b_5<<1
#define b_7              b_6<<1

#define b_MIE            b_7
#define b_PAVIS          b_4
#define b_PBVIS          b_3
#define b_RIJUST         b_1
#define b_RESET          b_0

#define PortA_Enable         b_2
#define PortB_Enable         b_7
#define Latch_On_Pat_Match   b_0

#define All_Input        0xFF
#define All_Masked       0
#define Norm_Inp         0
#define Non_Invert       0
#define Zero_To_One      0xFF
#define StInfMask        0xE

#define ICV_nln     16        /* Number of interrupt lines */
#define ICV_nboards 4         /* Number of interrupt lines */
#define PortA_nln   8

#endif	/* _Z8536CIO_ */
