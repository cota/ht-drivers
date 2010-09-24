#ifndef __LUNARGS_H__
#define __LUNARGS_H__

/** Maximum number of VMOD-XXX mezzanine boards in a crate */
#define VMOD_MAX_BOARDS	64

struct vmod_dev {
	int             lun;            /** logical unit number */
	char            *carrier_name;  /** carrier name */
	int             carrier_lun;    /** supporting carrier */
	int             slot;           /** slot we're plugged in */
	unsigned long	address;	/** virtual address of mz a.s.  */
	int		is_big_endian;	/** probably useless in MODULBUS */
};

struct vmod_devices {
	int			num_modules;
	struct vmod_dev		module[VMOD_MAX_BOARDS];
};

extern int read_params(char *driver_name, struct vmod_devices *devs);
extern int lun_to_index(struct vmod_devices *devs, int lun);

#endif /* __LUNARGS_H__ */
