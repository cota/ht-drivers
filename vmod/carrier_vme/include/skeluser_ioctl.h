/* =========================================================== 	*/
/* Define the IOCTL calls you will implement         		*/
#ifndef _SKEL_USER_IOCTL_H_
#define _SKEL_USER_IOCTL_H_


/*
 * Note. You can change any macro _name_ in this file *except* the following:
 * - SKELUSER_IOCTL_MAGIC
 * - SkelDrvrSPECIFIC_IOCTL_CALLS
 * - SkelUserIoctlFIRST
 * - SkelUserIoctlLAST
 */

/* =========================================================== */
/* Your Ioctl calls are defined here                           */

#define SKELUSER_IOCTL_MAGIC 'V'

#define SKELU_IO(nr)		  _IO(SKELUSER_IOCTL_MAGIC, nr)
#define SKELU_IOR(nr,sz)	 _IOR(SKELUSER_IOCTL_MAGIC, nr, sz)
#define SKELU_IOW(nr,sz)	 _IOW(SKELUSER_IOCTL_MAGIC, nr, sz)
#define SKELU_IOWR(nr,sz)	_IOWR(SKELUSER_IOCTL_MAGIC, nr, sz)

/* ============================= */
/* Define Standard IOCTL numbers */

#define VMODIO_GETSLTADDR     	SKELU_IOWR(1,int)

#define SkelUserIoctlFIRST		VMODIO_GETSLTADDR
#define SkelUserIoctlLAST		VMODIO_GETSLTADDR

/**
 *  @brief Encode an address space request from the mezzanine
 */
struct carrier_address_space {
	char	driver[60];	/** driver name of the carrier board */
	int	board_number;	/** module number the mezzanine occupies */
	int 	board_position;	/** carrier slot the mezzanine occupies */
	void	*address;	/** address space assigned */
};

#endif /* _SKEL_USER_IOCTL_H_ */

