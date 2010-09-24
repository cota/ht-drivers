/*****************************************************************************/
/*                                                                           */
/*     CIO8536.H                                                             */
/*    ===========							     */
/*                                                                           */
/*     The includefile contains all necessary defines for the                */
/*     ZILOG 8536-CIO interface chip.                                        */
/*                                                                           */
/*                                                                           */
/*****************************************************************************/

#ifndef __INCcio8536h
#define __INCcio8536h

#ifdef __cplusplus
extern "C" {
#endif

/*------------------------------*/
/* Master Control Registers	*/
/*------------------------------*/
#define	MICR	0x00	/* Master Interrupt Control Register		*/
#define	MCCR	0x01	/* Master Configuration Control Register	*/

/* Bitdefinition MICR */
#define	RESET	0x01	/* d0: reset cio				*/
#define	RJA	0x02	/* d1: right justified address			*/
#define	CTVIS	0x04	/* d2: c/t vector includes status		*/
#define	PBVIS	0x08	/* d3: port b vector includes status		*/
#define	PAVIS	0x10	/* d4: port a vector includes status		*/
#define	NV	0x20	/* d5: no vector				*/
#define	DLC	0x40	/* d6: disable lower chain			*/
#define	MIE	0x80	/* d7: master interrupt enable			*/

/* Bitdefinition MCCR */
			/* Counter/Timer Link Control			*/
			/* d0, d1:					*/
#define	T1GT2	0x01	/*         T1's output gates T2                 */
#define	T1TT2	0x02	/*         T1's output triggers T2              */
#define	T1T2	0x03	/*         T1's output is T2's count input      */
#define	CTLMASK	0x03	/* Counter/Timer Link Mask                      */
#define	PAE	0x04	/* d2: port a enable                            */
#define	PLC	0x08	/* d3: port link control                        */
#define	PCE	0x10	/* d4: port c enable                            */
#define	CT3E	0x10	/* d4: c/t 3 enable                             */
#define	CT2E	0x20	/* d5: c/t 2 enable                             */
#define	CT1E	0x40	/* d6: c/t 1 enable                             */
#define	PBE	0x80	/* d7: port b enable                            */

/*------------------------------*/
/* Port Specification Registers */
/*------------------------------*/
#define	PMSR_A	0x20	/* (A) Port Mode Specification Register		*/
#define	PMSR_B	0x28	/* (B)						*/
#define	PHSR_A	0x21	/* (A) Port Handshake Specification Register	*/
#define	PHSR_B	0x29	/* (B)						*/

#define	PCSR_A	0x08	/* (A) Port Command and Status Register 	*/
#define	PCSR_B	0x09	/* (B)						*/
/* Bitdefinition PCSR */
#define	IUS	0x80	/* d7: Interrupt under service			*/
#define	IE	0x40	/* d6: Interrupt enable				*/
#define	IP	0x20	/* d5: Interrupt pending			*/
#define	IERR	0x10	/* d4: Interrupt error				*/
#define	ORE	0x08	/* d3: output register empty			*/
#define	IRF	0x04	/* d2: input register full			*/
#define	PMF	0x02	/* d1: pattern match flag			*/
#define	IOE	0x01	/* d0: interrupt on error			*/
#define	CLEAR_IP_IUS 	0x20
#define	CLEAR_IP 	0xa0
#define	CLEAR_IUS 	0x60
#define	CLEAR_IE 	0xe0
#define	SET_IE	 	0xc0


/*--------------------------------------*/
/* Bit Path Definition Registers	*/
/*--------------------------------------*/
#define	DPPR_A	0x22	/* (A) Data Path Polarity Register		*/
#define	DPPR_B	0x2a	/* (B)						*/
#define	DPPR_C	0x05	/* (C)						*/
#define	DDR_A	0x23	/* (A) Data Direction Register			*/
#define	DDR_B	0x2b	/* (B)						*/
#define	DDR_C	0x06	/* (C)						*/
#define	SIOCR_A	0x24	/* (A) Special I/O Control Register		*/
#define	SIOCR_B	0x2c	/* (B) 						*/
#define	SIOCR_C	0x07	/* (C) 						*/

/*---------------------*/
/* Port Data Registers */
/*---------------------*/
#define PDR_A	0x0d	/* (A) Port Data Register			*/
#define PDR_B	0x0e	/* (B)						*/
#define PDR_C	0x0f	/* (C)						*/

/*------------------------------*/
/* Pattern Definition Registers */
/*------------------------------*/
#define	PPR_A	0x25	/* (A) Pattern Polarity Register 		*/
#define	PPR_B	0x2d	/* (B)						*/
#define	PTR_A	0x26	/* (A) Pattern Transition Register		*/
#define	PTR_B	0x2e	/* (B)						*/
#define	PMR_A	0x27	/* (A) Pattern Mask Register 			*/
#define	PMR_B	0x2f	/* (B)						*/

/*-------------------------*/
/* Counter/Timer Registers */
/*-------------------------*/
#define	CTCSR_1	0x0a	/* (1) C/T Command and Status Register		*/
#define	CTCSR_2	0x0b	/* (2)						*/ 
#define	CTCSR_3	0x0c	/* (3)						*/
/* Bitdefinition CTCSR */
/*	IUS	0x80	   d7: Interrupt under service			*/
/*	IE	0x40	   d6: Interrupt enable				*/
/*	IP	0x20	   d5: Interrupt pending			*/
/*	IERR	0x10	   d4: Interrupt error				*/
#define	RCC	0x08	/* d3: read counter control 			*/
#define	GCB	0x04	/* d2: gate command bit   	 		*/
#define	TCB	0x02	/* d1: trigger command bit			*/
#define	CIP	0x01	/* d0: count in progress			*/

#define	CTMSR_1	0x1c	/* (1) C/T Mode Specification Register 		*/
#define	CTMSR_2	0x1d	/* (2)						*/
#define	CTMSR_3	0x1e	/* (3)						*/
#define	CTCCRH_1 0x10	/* (1) C/T Current Count Register high byte	*/
#define	CTCCRL_1 0x11	/* (1) 				  low  byte	*/
#define	CTCCRH_2 0x12	/* (2) 				  high byte	*/
#define	CTCCRL_2 0x13	/* (2)				  low  byte	*/
#define	CTCCRH_3 0x14	/* (3)				  high byte	*/
#define	CTCCRL_3 0x15	/* (3)				  low  byte	*/
#define	CTTCRH_1 0x16	/* (1) C/T Time Constant Register high byte	*/
#define	CTTCRL_1 0x17	/* (1) 				  low  byte	*/
#define	CTTCRH_2 0x18	/* (1) C/T Time Constant Register high byte	*/
#define	CTTCRL_2 0x19	/* (1) 				  low  byte	*/
#define	CTTCRH_3 0x1a	/* (1) C/T Time Constant Register high byte	*/
#define	CTTCRL_3 0x1b	/* (1) 				  low  byte	*/

/*----------------------------*/
/* Interrupt Vector Registers */
/*----------------------------*/
#define	ICR_A	0x02	/* (A) Interrupt Vector Register		*/
#define	ICR_B	0x03	/* (B)						*/
#define	ICR_CT	0x04	/* (C/T)					*/
#define	CVR	0x1f	/* Current Vector Register			*/

#ifdef __cplusplus
}
#endif

#endif /* __INCcio8536h */

