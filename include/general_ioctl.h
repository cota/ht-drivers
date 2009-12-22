/**
 * @file general_ioctl.h
 *
 * @brief General ioctl numbers and structure definitions are here.
 *
 * @author Yury GEORGIEVSKIY, CERN
 *
 * @date Created on 28/07/2008
 *
 * Ioctl numbers, common for all the drivers. Their numbers and expected
 * actions are predefined and should not be changed. This ioctl numbers can
 * be used by other programs (nodal for example) to perform standart ioctls.
 *
 * @note As soon as you make any changes here - NODAL should be recompiled
 *
 * @version
 */
#ifndef _GENERAL_IOCTL_NUMBERS_H_INCLUDE_
#define _GENERAL_IOCTL_NUMBERS_H_INCLUDE_

#define GENERAL_IOCTL_MAGIC 'X'

#define GEN_IO(nr)      _IO(GENERAL_IOCTL_MAGIC, nr)
#define GEN_IOR(nr,sz)  _IOR(GENERAL_IOCTL_MAGIC, nr, sz)
#define GEN_IOW(nr,sz)  _IOW(GENERAL_IOCTL_MAGIC, nr, sz)
#define GEN_IOWR(nr,sz) _IOWR(GENERAL_IOCTL_MAGIC, nr, sz)


/** @defgroup general_ioctl General ioctl numbers
 *
 *  <EM>GENERAL IOCTL NUMBERS:</EM>
 *
 * @b _GIOCTL_CDV_INSTALL     -> Register new device logical group in the
 *			         driver. Called each time cdv_install() do.
 *
 * @b _GIOCTL_CDV_UNINSTALL   -> Unloading major device.
 *			         Called each time cdv_uninstall() do.
 *
 * @b _GIOCTL_GET_MOD_AM      -> How many modules are controlled by the driver.
 *
 * @b _GIOCTL_GET_MOD_INFO    -> Get Module Information. Can be info table,
 *                               or any other description, suitable for current
 *                               driver.
 *
 * @b _GIOCTL_SET_ACT_MOD     -> Set module user is working with.
 *                               Used by @b NODAL.
 *
 * @b _GIOCTL_GET_BAR_INFO    -> Get BAR's information.
 *
 * @b _GIOCTL_BAR_RD          -> Read HW memory, mapped on the given bar.
 *                               Used by @b NODAL.
 *
 * @b _GIOCTL_BAR_WR          -> Write into HW memory of the given bar.
 *                               Used by @b NODAL.
 *
 * @b _GIOCTL_GET_REG_INFO    -> Get module registers description
 *
 * @b _GIOCTL_REG_RD          -> read module registers
 *
 * @b _GIOCTL_REG_WR          -> write module registers
 *
 * @b _GIOCTL_JTAG_OPEN       ->
 *
 * @b _GIOCTL_JTAG_CLOSE      ->
 *
 * @b _GIOCTL_JTAG_READ_BYTE  ->
 *
 * @b _GIOCTL_JTAG_WRITE_BYTE ->
 *
 * @b _GIOCTL_GET_DG_DEV_INFO -> Driver Gen service call to get information
 *                               about controlled devices
 *@{
 */
#define _GIOCTL_CDV_INSTALL     GEN_IOWR (900, char[128])
#define _GIOCTL_CDV_UNINSTALL   GEN_IOW  (901, int)
#define _GIOCTL_GET_MOD_AM      GEN_IO   (902)
//#define _GIOCTL_GET_MOD_DESCR   GEN_IOR (903, struct module_info_table)
#define _GIOCTL_GET_MOD_INFO    GEN_IOR  (903, int)
#define _GIOCTL_SET_ACT_MOD     GEN_IOR  (904, int)
#define _GIOCTL_GET_BAR_INFO    GEN_IOR  (905, bar_map_t[6])
#define _GIOCTL_BAR_RD          GEN_IOR  (906, bar_rdwr_t)
#define _GIOCTL_BAR_WR          GEN_IOW  (907, bar_rdwr_t)
#define _GIOCTL_GET_REG_INFO    GEN_IOR  (908, reg_desc_t)
#define _GIOCTL_REG_RD          GEN_IOR  (909, reg_desc_t)
#define _GIOCTL_REG_WR          GEN_IOW  (910, reg_desc_t)
#define _GIOCTL_JTAG_OPEN       GEN_IO   (911)
#define _GIOCTL_JTAG_CLOSE      GEN_IO   (912)
#define _GIOCTL_JTAG_READ_BYTE  GEN_IO   (913)
#define _GIOCTL_JTAG_WRITE_BYTE GEN_IO   (914)
#define _GIOCTL_GET_DG_DEV_INFO GEN_IOR  (915, int)
//@} end of group

#endif /* _GENERAL_IOCTL_NUMBERS_H_INCLUDE_ */
