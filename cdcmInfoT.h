/**
 * @file cdcmInfoT.h
 *
 * @brief CDCM Info Table.
 *
 * @author Georgievskiy Yury, Alain Gagnaire. CERN AB/CO.
 *
 * @date Created on 01/06/2006
 *
 * @version 2.0 ygeorgie  15/02/2009  All unused structures removed.
 */
#ifndef _CDCM_INFO_TABLE_H_INCLUDE_
#define _CDCM_INFO_TABLE_H_INCLUDE_

/* signature */
#define CDCM_MAGIC_IDENT (('C'<<24)|('D'<<16)|('C'<<8)|'M')

/* CDCM info header. Parameter for module_init entry point */
typedef struct _cdcmInfoHeader {
  int  cih_signature; /* 'CDCM' */
  int  cih_crc;       /* checksum */
} cdcm_hdr_t;

#endif /* _CDCM_INFO_TABLE_H_INCLUDE_ */
