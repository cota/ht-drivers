/**
 * @file inst_utils.c
 *
 * @brief Functions, used by both Linux and Lynx during installation.
 *
 * @author Copyright (C) 2009 CERN. Yury GEORGIEVSKIY <ygeorgie@cern.ch>
 *
 * @date Created on 14/02/2009
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/utsname.h> /* for uname */

#include "install.h"
#include "err.h"
#include <general_both.h>

/* coloring */
#define RED_CLR	   "\033[0;31m"
#define WHITE_CLR  "\033[1;37m"
#define END_CLR	   "\033[m"

static char *default_xml_config_file(void);

/**
 * @brief Guess what
 *
 * @param name -- you'll never know
 */
void display_usage(char *name)
{
	printf("Usage: %s <*.xml> [ -fall | -fexclude ] \\\n"
	       "       [-s]<name> -o\"option string\" -n<noden> ... \\\n"
	       "       [-s]<name> -o\"option string\" -n<noden> ...\n\n"
	       "Installing the driver based on it's xml description.\n"
	       " * Parameters:\n"
	       "  <name> %soptional%s\n"
	       "         Specify driver to be installed in a Normal manner.\n\n"
	       "  -s<name> %soptional%s\n"
	       "           Special installation mode request. All driver\n"
	       "           parameters will be passed to the driver during\n"
	       "           dr_install(), but not during cdv_install() call.\n"
	       "           Valid %s[ONLY]%s for Linux device drivers.\n\n"
	       "  -o\"str\" %soptional%s\n"
	       "            Option string to pass to the driver during\n"
	       "            init_module() system call\n"
	       "            Valid %s[ONLY]%s for Linux device drivers.\n\n"
	       "  -n<noden> %soptional%s\n"
	       "            Extra symbolic link node name to create.\n"
               "            Normally, DEF_CLIENT_CTXT_AM (16 as of 07.2009)\n"
	       "            nodes are created with names\n"
	       "            <mod_name>1 .. <mod_name>16\n\n"
	       "            In this case -- DEF_CLIENT_CTXT_AM symlinks with\n"
	       "            names <noden>1 .. <noden>16 will also be created\n"
	       "            and will point to corresponding module nodes\n"
	       "            <mod_name>1 .. <mod_name>16.\n\n"
	       "            If there are already <noden>1 .. <noden>16 - then\n"
	       "            <noden>17 .. <noden>32 will be created.\n"
	       "            They will point to corresponding module nodes\n"
	       "            <mod_name>1 .. <mod_name>16.\n\n"
	       " * Force flags:\n"
	       "  -fall %soptional%s\n"
	       "        Tells to install all the drivers found in the xml\n"
	       "        config file.\n"
               "        Multiple -s <name> params can present in the command\n"
	       "        line to specify the drivers, that should be istalled\n"
	       "        using special installation mode.\n\n"
	       "  -fexclude %soptional%s\n"
	       "            Will install all the drivers, exept one provided\n"
	       "            in the command line.\n\n"
	       "  -foptions %soptional [for driverGen only]%s\n"
	       "            Options from .xml file should be taken instead of\n"
	       "            one, provided in the command line.\n\n"
	       " * Examples:\n"
	       " [1] instprog\n"
	       "     Will install everything from `uname -n`.xml\n"
	       "     config file\n\n"
	       " [2] instprog my.xml\n"
	       "     Install everything from my.xml config file\n\n"
	       " [3] instprog -fall CTRP -nctr -o\"option for CTR\" \\\n"
	       "     -sMIL1553 VD80 -o\"vd80 option\"\n"
	       "     Install everything from `uname -n`.xml\n"
	       "     MIL1553 goes through special installation.\n"
	       "     Pass the option for CTRP driver.\n"
	       "     Create symlink nodes for CTRP driver.\n"
	       "     Pass the option for VD80.\n\n"
	       " [4] instprog my.xml -sMIL1553 VD80 -sCTRP\n"
	       "     Install only MIL1553 and VD80 from my.xml\n"
	       "     MIL1553 -- special one.\n"
	       "     VD80    -- uses normal installation schema.\n"
	       "     CTRP    -- special installation request.\n\n"
	       " [5] instprog -fexclude MIL1553 VD80 CTRV\n"
	       "     Install everything, exept MIL1553, VD80 and CTRV\n"
	       "     drivers from `uname -n`.xml config file.\n"
	       "     Pass the option for VD80 driver.\n\n",
	       basename(name), WHITE_CLR, END_CLR, WHITE_CLR, END_CLR,
	       WHITE_CLR, END_CLR, WHITE_CLR, END_CLR, WHITE_CLR, END_CLR,
	       WHITE_CLR, END_CLR, WHITE_CLR, END_CLR, WHITE_CLR, END_CLR,
	       WHITE_CLR, END_CLR);
}

/**
 * @brief Command line arg parser
 *
 * @param argc -- arg amount
 * @param argv -- arg value
 * @param flg  --      I_ALL -- install all drivers found in the .xml
 *                  I_CHOSEN -- install only drivers, that are provided in the
 *                              command line.
 *                I_EXCLUDED -- install all drivers, exept one provided in the
 *                              command line (--exclude option)
 * @param cf   -- .xml config file name goes here. Should be freed by the
 *                caller afterwards.
 * @param head -- list head to hold all driver descriptions
 *                (of type struct drvrd)
 *
 * Returned massive holds driver names that user provded in the command line.
 * @b NOTE
 *    list, struct drvrd.dopt and struct drvrd.dname should be freed afterwards!
 *
 * @return driver description list capacity (head param)
 * @return -ECANCELD - if just help message was displayed
 * @return -ENOMEM   - can't allocate memory for driver description table
 */
int parse_prog_args(int argc, char* argv[], int *flg, char **cf,
		    struct list_head *head)
{
	struct utsname hdata;	/* host data */
	int opt;
	struct drvrd *ddp;

	INIT_LIST_HEAD(head);
	*cf = NULL;
	*flg = 0; /* nothing set yet */
	/* Scan params of the command line */
	while ( (opt = getopt(argc, argv, "-s:f:o:n:h")) != EOF) {
		switch (opt) {
		case 's':	/* special installation mode (Linux only)
				   All driver parameters are passed to the
				   driver during dr_install(), but not during
				   cdv_install() call, which will not be
				   called */
			ddp = calloc(1, sizeof(*ddp));
			if (!ddp)
				goto outbad;
			asprintf(&ddp->dname, "%s", optarg);
			ddp->dflag = opt;
			list_add_tail(&ddp->list, head);
			if (!*flg) /* not set yet */
				*flg = I_CHOSEN;
			break;
		case 'f':	/* force flags */
			if (*flg) /* already set */
				break;
			if (!strcmp(optarg, "exclude"))
				*flg = I_EXCLUDED;
			if (!strcmp(optarg, "all"))
				*flg = I_ALL;

			break;
		case 'o':	/* Driver option string (Linux only) */
			if (ddp)
				asprintf(&ddp->dopt, "%s", optarg);
			break;
		case 'n':	/* symbolik link node name */
			if (ddp)
				asprintf(&ddp->slnn, "%s", optarg);
			break;
		case 'h':	/* help */
			display_usage(argv[0]);
			return -ECANCELED; /* Operation canceled */
		default:
#ifdef __Lynx__
		case 0:
#else /* __linux__ */
		case 1:	/* Non-option argument.
			   Can be either xml config file name or driver name */
#endif
			if (strstr(optarg, ".xml")) {
				if (!*cf)
					asprintf(cf, "%s", optarg);
				break;
			}
			ddp = calloc(1, sizeof(*ddp));
			if (!ddp)
				goto outbad;
			asprintf(&ddp->dname, "%s", optarg);
			list_add_tail(&ddp->list, head);
			if (!*flg) /* not set yet */
				*flg = I_CHOSEN;
			break;
		}
	}

	if (!*flg) *flg = I_ALL; /* no drivers in the command line */
	if (!*cf) /* no .xml file provided in the command line. Set default */
		*cf = default_xml_config_file();

	return list_capacity(head);
 outbad:
	free_drvrd(head);
	return -ENOMEM;
}

/**
 * @brief Creates device node symlinks
 *
 * @param dnames -- dev nodes to create symlinks for
 * @param slname -- symlink name
 * @param dna    -- number of links to create
 *
 * Symlinks with names <slname>1 .. <slname>dna will be created.
 *
 * If there are already <slname>1 .. <slname>dna -- then
 * <slname>dna+1 .. <slname>2*dna will be created. They will point to
 * corresponding module nodes <mod_name>1 .. <mod_name>dna.
 *
 * @return how many symlinks created
 */
static int create_node_symlinks(char **dnames, char *slname, int dna)
{
	DIR *dir;
	struct direct *direntry;
	char cwd[1024];
	struct stat fstat;
	int cntr = 0, i;
	char *nn = NULL; /* node name */

	getcwd(cwd, sizeof(cwd)); /* save cwd */
	dir = opendir("/dev");
	chdir("/dev");
	while ( (direntry = readdir(dir)) ) {
		/* check, if symlink names are already there
		   and count them */
		if (strstr(direntry->d_name, slname)) {
			lstat(direntry->d_name, &fstat);
			if (S_ISLNK(fstat.st_mode))
				++cntr;
		}
	}
	chdir(cwd);
	closedir(dir);
	++cntr;
	for (i = 0; i < dna; cntr++, i++) {
		asprintf(&nn, "/dev/%s.%d", slname, cntr);
		symlink(dnames[i], nn);
		free(nn);
	}

}

/**
 * @brierf Create driver nodes for the user. One node - one user context
 *
 * @param dmaj   -- driver major number
 * @param dname  -- driver name
 * @param slname -- symbolic link node name. Can be NULL
 * @param dna    -- driver nodes amount to create
 *
 * Will create driver nodes <mod_name>1 .. <mod_name>dna.
 * First number will be 1, last one will be the @ref dna parameter.
 *
 * If @ref slname is not NULL -- then @ref dna number of symlinks with names
 * <slname>1 .. <slname>dna will be created.
 *
 * If there are already <slname>1 .. <slname>dna -- then
 * <slname>dna+1 .. <slname>2*dna will be created. They will point to
 * corresponding module nodes <mod_name>1 .. <mod_name>dna.
 *
 * @return how many nodes created
 */
int create_driver_nodes(int dmaj, char *dname, char *slname, int dna)
{
	int cntr;
	int created = 0;
	char *nn = NULL; /* node name */
	int devn;
	char *mas[dna];

	for (cntr = 1; cntr <= dna; cntr++) { /* buildup device nodes */
		asprintf(&nn, "/dev/%s.%d", dname, cntr);
		unlink(nn); /* if already exist delete it */
		devn = makedev(dmaj, cntr);
		if (mknod(nn, S_IFCHR | 0666, devn) < 0) {
			printf("mknod() failed! Can't create '%s' device node",
			      nn);
		} else
			++created;

		mas[cntr-1] = nn; /* save node names */
	}

	if (slname) /* we should create symlinks */
		create_node_symlinks(mas, slname, dna);

	for (cntr = 0; cntr < dna; cntr++) free(mas[cntr]);
	return created;
}

/**
 * @brief Create driver info file in /tmp.
 *
 * @param name -- Driver Name
 * @param addr -- Address to save in the info file
 *
 * @b NOTE Returned pointer should be freed afterwards by the caller.
 *
 * @return NULL          - error
 * @return info filename - all OK
 */
inline char *create_info_file(char *name, void *addr)
{
	int fd;
	char *ifn = NULL;

	if (!addr || !name)
		return NULL;

	asprintf(&ifn, "/tmp/%s_XXXXXX", name);
	fd = mkstemp(ifn);
	write(fd, &addr, sizeof(void*)); /* safe info table address */
	fchmod(fd, 0666);
	close(fd);
	return ifn;
}

/**
 * @brief Creates option string (provided by the user) to pass it to
 *        init_module() system call
 *
 * @param argc -- command line arg amount
 * @param argv -- command line arguments
 * @param uarg -- index of the first user-defined parameter
 *
 * Taken from mod-init-tools.
 * Returned pointer should be freed afterwards by the caller!
 *
 * @return created option string (can be an empty one "") - if OK
 * @return NULL                                           - if failed
 */
__attribute__ ((unused)) char *create_usr_option_string(int argc,
							char *argv[], int uarg)
{
	char *options = strdup("");
	int i;

	if (!options) {
		fprintf(stderr, "%s() can't allocate memory: %s\n",
			__func__, strerror(errno));
		return NULL;
	}

	if (!uarg)
		return options;	/* no user arguments in the command
				   line - return an empty string */

	for (i = uarg; i < argc; i++) {
		options = realloc(options,
				  strlen(options) + 1 + strlen(argv[i]) + 1);
		if (!options) {
			fprintf(stderr, "%s() can't allocate memory: %s\n",
				__func__, strerror(errno));
			return NULL;
		}
		strcat(options, argv[i]);
		strcat(options, " ");
	}

	return options;
}

/**
 * @brief Generates default .xml config file
 *
 * @param none
 *
 * Naming convention for default config file is "/etc/drivers.xml"
 * This comes from the Makefile for building DSCs from the database,
 * available at:
 * /acc/src/dsc/co/Make.dsc
 *
 * @b NOTE Name should be freed by the caller afterwards.
 *
 * @return XML config file name
 */
char *default_xml_config_file(void)
{
	char *xmlnm;

	asprintf(&xmlnm, "/etc/drivers.xml");
	return xmlnm;
}

/**
 * @brief free all driver description resources
 *
 * @param head -- driver description list head
 */
void free_drvrd(struct list_head *head)
{
	struct drvrd *el;
	struct list_head *lst, *safe;

	list_for_each_safe(lst, safe, head) {
		el = list_entry(lst, struct drvrd, list);
		if (el->dopt) free(el->dopt);
		if (el->slnn) free(el->slnn);
		if (el->dname) free(el->dname);
		list_del(&el->list);
		free(el);
	}
}

/**
 * @brief Get command line driver description, if any
 *
 * @param head -- driver description list head
 * @param dnm  -- driver name from .xml file
 *
 * @return command line driver description, or NULL.
 */
struct drvrd *cmd_line_dd(struct list_head *head, char *dnm)
{
	struct drvrd *dd;

	list_for_each_entry(dd, head, list)
		if (!(strcmp(dd->dname, dnm)))
			return dd;
	return NULL;
}

/**
 * @brief Search for the .ko in cwd, and if it is a symlink -- follows it.
 *
 * @param path -- result goes here
 * @param dn   -- driver name. Exactly as in the DB.
 *
 * [1] If driver is delivered -- then we should have a .ko object files in cwd.
 *     Ex: vd80.ko
 *
 * [2] If drvier is compiled locally and is @b not delivered -- then we can have
 *     N  .ko objects, which are symlinks. They contain 'uname -r' in their
 *     names. Ex: vd80.2.6.29.4-rt15.ko.
 *     It points to the real .ko object of the format as in [1]
 *
 * Function will detect the type, and if it's a symlink, will follow it.
 * @b NOTE:
 *    path should be freed afterwards by the caller.
 *
 * @return  0 -- all OK, can proceed with installation
 * @return -1 -- *.ko filename ambiguity, means that there are names with
 *               'uname -r' part in them, and there are one without it.
 * @return -2 -- can't read a symbolic link
 * @return -3 -- .ko doesn't exist
 */
int drvr_pathname(char **path, char *dn)
{
	DIR *dir = opendir("./");
	struct direct *direntry;
	char *kosl;	/* .ko symlink filename */
	char *kof;	/* .ko filename */
	struct utsname buf;
	int rc = 0;
	char slbuf[256] = { 0 };
	int lko = 0, rko = 0;	/* .ko counters
				   lko - local .ko file
				   rko - remote .ko file (symlink) */

	uname(&buf);
	/* note, that here we ensure lowercase drivername */
	asprintf(&kosl, "%s.%s.ko", str2lower(dn), buf.release);
	asprintf(&kof, "%s.ko", str2lower(dn));

	/* *.ko && *.`uname -r`.ko can't co-exist together in one dir */
	while ( (direntry = readdir(dir)) ) {
		if (!strcmp(direntry->d_name, kof))
			++lko;
		if (!strcmp(direntry->d_name, kosl))
			++rko;
	}
	closedir(dir);

	if (!lko && !rko) {
		printf("%sCan't file neither %s nor %s\n"
		       "       .ko doesn't exist\n",
		       ERR_MSG, kof, kosl);
		rc = -3;
		goto out;
	}

	/* we should have at least one of them */
	if (lko == rko) {
		printf("%sAmbiguous *.ko filenames detected.\n"
		       "       [%s] and [%s] can't co-exist.\n",
		       ERR_MSG, dn, kosl);
		rc = -1;  /* both *.ko and *.`uname -r`.ko detected */
		goto out;
	}

	if (lko) {
		asprintf(path, "./%s", kof);
		goto out;

	}

	/* we have a symlink */
	if (readlink(kosl, slbuf, sizeof(slbuf)-1) == -1) {
		printf("%sCan't resolve %s symlink\n", ERR_MSG, kosl);
		rc = -2;
		goto out;
	}
	asprintf(path, "%s", slbuf);

 out:
	if (kosl) free(kosl);
	if (kof)  free(kof);
	return rc;
}
