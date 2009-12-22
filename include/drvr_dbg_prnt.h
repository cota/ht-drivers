/**
 * @file drvr_dbg_prnt.h
 *
 * @brief Debug printout macroses for the driver.
 *
 * @author Yury Georgievskiy. CERN AB/CO.
 *
 * @date April, 2008
 *
 * @version 1.0  ygeorgie  09/04/2008  Creation date.
 */
#ifndef _DRIVER_DBG_PRINTOUT_H_INCLUDE_
#define _DRIVER_DBG_PRINTOUT_H_INCLUDE_

/* If driver wants to use these utils - it should provide this function
   CDCM is providing this function in cdcmDrvr.c */
extern char* __drvrnm(void);

//!< DeBuG Information Printout Level (IPL)
typedef enum _dbgipl {
  IPL_NONE    = 0,	 //!< silent mode
  IPL_OPEN    = 1 << 0,
  IPL_CLOSE   = 1 << 1,
  IPL_READ    = 1 << 2,
  IPL_WRITE   = 1 << 3,
  IPL_SELECT  = 1 << 4,
  IPL_IOCTL   = 1 << 5,
  IPL_INSTALL = 1 << 6,
  IPL_UNINST  = 1 << 7,
  IPL_DBG     = 1 << 8,	 //!< printout debug messages
  IPL_ERROR   = 1 << 9,  //!< printout error messages
  IPL_INFO    = 1 << 10, //!< printout information messages
  IPL_WARN    = 1 << 11, //!< printout warning messages
  IPL_ALL     = (~0)     //!< verbose
} dbgipl_t;

//!< indisputable info printout
#define PRNT_ABS_INFO(format...)					 \
do {									 \
	printk("%s [%s_INFO] %s(): ", KERN_INFO, __drvrnm(), __func__ ); \
	printk(format);							 \
	printk("\n");							 \
 } while(0)

//!< indisputable error printout
#define PRNT_ABS_ERR(format...)						\
do {									\
	printk("%s [%s_ERR] %s(): ", KERN_ERR, __drvrnm(), __func__ );	\
	printk(format);							\
	printk("\n");							\
 } while(0)

//!< indisputable warning printout
#define PRNT_ABS_WARN(format...)					    \
do {									    \
	printk("%s [%s_WARN] %s(): ", KERN_WARNING, __drvrnm(), __func__ ); \
	printk(format);							    \
	printk("\n");							    \
 } while(0)

//!< indisputable debugging printout
#define PRNT_ABS_DBG_MSG(format...)				\
do {								\
	printk("%s [%s_DBG] %s()@%d: ", KERN_DEBUG, __drvrnm(),	\
	       __func__ , __LINE__ );				\
  printk(format);						\
  printk("\n");							\
} while(0)


/*! @name Driver debug printing.
 *
 * Driver statics table should be named @e drvrStatT and have a member
 * @e d_ipl of type \ref dbgipl_t
 */
//@{
#define PRNT_OPEN(dflg, format, arg...)				\
do {								\
	if (dflg & IPL_OPEN) { PRNT_ABS_INFO(format, ##arg); }	\
} while (0)

#define PRNT_CLOSE(dflg, format, arg...)			\
do {								\
	if (dflg & IPL_CLOSE) { PRNT_ABS_INFO(format, ##arg); }	\
} while (0)

#define PRNT_READ(dflg, format, arg...)				\
do {								\
	if (dflg & IPL_READ) {PRNT_ABS_INFO(format, ##arg); }	\
} while (0)

#define PRNT_WRITE(dflg, format, arg...)			\
do {								\
	if (dflg & IPL_WRITE) { PRNT_ABS_INFO(format, ##arg); }	\
} while (0)

#define PRNT_SELECT(dflg, format, arg...)				\
do {									\
	if (dflg & IPL_SELECT) { PRNT_ABS_INFO(format, ##arg); }	\
} while (0)

#define PRNT_IOCTL(dflg, format, arg...)			\
do {								\
	if (dflg & IPL_IOCTL) { PRNT_ABS_INFO(format, ##arg); }	\
} while (0)

#define PRNT_INSTALL(dflg, format, arg...)				\
do {									\
	if (dflg & IPL_INSTALL) { PRNT_ABS_INFO(format, ##arg); }	\
} while (0)

#define PRNT_UNINST(dflg, format, arg...)				\
do {									\
	if (dflg & IPL_UNINST) { PRNT_ABS_INFO(format, ##arg); }	\
} while (0)

#define PRNT_DBG(dflg, format, arg...)					\
do {									\
	if (dflg & IPL_DBG) { PRNT_ABS_DBG_MSG(format, ##arg); }	\
} while (0)

#define PRNT_ERR(dflg, format, arg...)				\
do {								\
	if (dflg & IPL_ERROR) { PRNT_ABS_ERR(format, ##arg); }	\
} while (0)

#define PRNT_INFO(dflg, format, arg...)				\
do {								\
	if (dflg & IPL_INFO) { PRNT_ABS_INFO(format, ##arg); }	\
} while (0)

#define PRNT_WARN(dflg, format, arg...)				\
do {								\
	if (dflg & IPL_WARN) { PRNT_ABS_WARN(format, ##arg); }	\
} while (0)
//@}

#endif /* _DRIVER_DBG_PRINTOUT_H_INCLUDE_ */
