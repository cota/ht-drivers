/**
 * @file vme_am.h
 *
 * @brief Defines value of Adress Modifier for VME access.
 *
 * @author Yury GEORGIEVSKIY, CERN AB/CO
 *
 * @date Created on 13/11/2008
 *
 * This value is typic of addressing mode required by a vme module.
 *
 * @version
 */
#ifndef _VME_AM_H_INCLUDE_
#define _VME_AM_H_INCLUDE_

/*! @name VMEbus Address Modifier Codes [AM0 - AM5] Under VME64x
 *
 * This can be thought of as a tag that is attached to each address.
 * There are 47 defined address modifier codes (some of them are not used),
 * which are summarized in the Table below.
 */
//@{
typedef enum {
  AM_A24_SBLT         = 0x3F, //!< A24 supervisory block transfer (BLT)
  AM_A24_SPA          = 0x3E, //!< A24 supervisory program access
  AM_A24_SDA          = 0x3D, //!< A24 supervisory data access
  AM_A24_SMBLT        = 0x3C, //!< A24 supervisory 64-bit block transfer (MBLT)
  AM_A24_UBLT         = 0x3B, //!< A24 non-privileged block transfer (BLT)
  AM_A24_UPA          = 0x3A, //!< A24 non-privileged program access
  AM_A24_UDA          = 0x39, //!< A24 non-privileged data access
  AM_A24_UMBLT        = 0x38, //!< A24 non-privileged 64-bit block transfer (MBLT)
  AM_A40_BLT          = 0x37, //!< A40BLT [MD32 data transfer only]
  /* 0x36 - Unused/Reserved */
  AM_A40_LCK          = 0x35, //!< A40 lock command (LCK)
  AM_A40_ACC          = 0x34, //!< A40 access
  /* 0x33 - Unused/Reserved */
  AM_A24_LCK          = 0x32, //!< A24 lock command (LCK)
  /* 0x30 - 0x31 - Unused/Reserved */
  AM_A24_CR_CSR       = 0x2F, //!< CR/CSR space
  AM_A16_SACC         = 0x2D, //!< A16 supervisory access
  AM_A16_LCK          = 0x2C, //!< A16 lock command (LCK)
  /* 0x2A - 0x2B - Unused/Reserved */
  AM_A16_UACC         = 0x29, //!< A16 non privileged access
  /* 0x22 - 0x28 - Unused/Reserved */
  AM_A32_A40_2eVME_3U = 0x21, //!< 2eVME for 3U bus modules (address size in XAM code)
  AM_A32_A64_2eVME_6U = 0x20, //!< 2eVME for 6U bus modules (address size in XAM code)
  /* 0x10 - 0x1F - Unused/Reserved */
  AM_A32_SBLT         = 0x0F, //!< A32 supervisory block transfer (BLT)
  AM_A32_SPA          = 0x0E, //!< A32 supervisory program access
  AM_A32_SDA          = 0x0D, //!< A32 supervisory data access
  AM_A32_SMBLT        = 0x0C, //!< A32 supervisory 64-bit block transfer (MBLT)
  AM_A32_UBLT         = 0x0B, //!< A32 non-privileged block transfer (BLT)
  AM_A32_UPA          = 0x0A, //!< A32 non-privileged program access
  AM_A32_UDA          = 0x09, //!< A32 non-privileged data access
  AM_A32_UMBLT        = 0x08, //!< A32 non privileged 64-bit block transfer (MBLT)
  /* 0x06 - 0x07 - Unused/Reserved */
  AM_A32_LCK          = 0x05, //!< A32 lock command (LCK)
  AM_A64_LCK          = 0x04, //!< A64 lock command (LCK)
  AM_A64_BLT          = 0x03, //!< A64 block transfer (BLT)
  AM_A64_SACCT        = 0x01, //!< A64 single access transfer
  /* 0x02 - Unused/Reserved */
  AM_A64_MBLT         = 0x00  //!< A64 64-bit block transfer (MBLT)
} vmeam_t;
//@}

#endif /* _VME_AM_H_INCLUDE_ */
