/**
 * @file instprog.c
 *
 * @brief Linux/Lynx Driver Installation routine.
 *
 * @author Copyright (C) 2009 CERN. Yury GEORGIEVSKIY <ygeorgie@cern.ch>
 *
 * @date Created on 05/02/2009
 *
 * User can provide XML config file name as the first parameter for the
 * installation program. If it's not provided, `uname -n`.xml will be used.
 *
 * Also driver names can be provided to install only specific drivers.
 * If no driver name is provided in the command line -- all drivers
 * described in the .xml config file will be installed.
 */
#include "install.h"
#include <utils/user/libinst.h>
#include <general_both.h>
#include <err.h>

#define DEF_CLIENT_CTXT_AM 16 	/* default client context amount */

static int driver_gen_installation(InsLibDrvrDesc *drvrd)
{
	/* each dg module has its own unique driver -- so we can't
	   have more then one module types (with different names) in
	   <driver> tag */
	return 1;
}

/**
 * @brief Do actual driver installation
 *
 * @param drvrd -- Driver description from the XML config file
 * @param dd    -- command line driver description.
 *                 If NULL -- no extra information was provided for the driver
 *                 in the command line.
 *
 * @return 1 - all OK
 * @return 0 - fails
 */
int do_install_driver(InsLibDrvrDesc *drvrd, struct drvrd *dd)
{
	int cc = 1, major = 0;
	char *infof = NULL; /* driver info file */
	char *drvrf = NULL; /* driver object file */
	int did = 0; /* driver ID */
	char *options; /* will hold option string in case of
			  special installation mode */
	char *addropt = NULL;

	/* can be -1 -- not provided 0 -- no 1 -- yes */
	if (drvrd->isdg == 1)
		return driver_gen_installation(drvrd);

	options = strdup("");
	if (!options) {
		fprintf(stderr, "%s() can't allocate memory: %s\n",
			__func__, strerror(errno));
		return 0;
	}

	/* get .ko file name */
	if (drvr_pathname(&drvrf, drvrd->DrvrName)) {
		cc = 0;
		goto exit_install;
	}

	printf("Installing %s driver...\n", drvrf);

	/* do info file */
	if ( !(infof = create_info_file(drvrd->DrvrName, (void*)drvrd)) ) {
		printf("Can't create info file. Aborting...\n");
		cc = 0;
		goto exit_install;
	}

	if (dd && dd->dflag == 's') {
		/* Special installation mode. Info table address
		   will be passed to the driver in [infoaddr]
		   parameter during dr_install(),
		   and cdv_install() will not be called */
		asprintf(&addropt, "infoaddr=%p", drvrd);
		options = realloc(options, strlen(options) + 1 +
				  strlen(addropt) + 1);
		if (!options) {
			fprintf(stderr, "%s() can't allocate"
				" memory: %s\n",
				__func__, strerror(errno));
			cc = 0;
			goto exit_install;
		}
		strcat(options, addropt);
		strcat(options, " ");

		if (dd->dopt) {
			/* will pass user-provided options */
			options = realloc(options, strlen(options) + 1 +
					  strlen(dd->dopt) + 1);
			if (!options) {
				fprintf(stderr, "%s() can't allocate"
					" memory: %s\n",
					__func__, strerror(errno));
				cc = 0;
				goto exit_install;
			}
			strcat(options, dd->dopt);
			strcat(options, " ");
		}

		major = dr_install(drvrf, (int)options);
		if (major < 0)
			goto dr_error;
		else
			goto cdn;
	} else { /* we should call cdv_install() */
		if (dd && dd->dopt) {
			/* will pass user-provided options
			   during dr_install() */
			options = realloc(options, strlen(options) + 1 +
					  strlen(dd->dopt) + 1);
			if (!options) {
				fprintf(stderr, "%s() can't allocate"
					" memory: %s\n",
					__func__, strerror(errno));
				cc = 0;
				goto exit_install;
			}
			strcat(options, dd->dopt);
			strcat(options, " ");

			did = dr_install(drvrf, (int)options);
		} else
			did = dr_install(drvrf, CHARDRIVER);
	}

	if (did < 0) {
	dr_error:
		printf("\ndr_install() fails with error: \"%s\" ."
		       " Aborting...\n", strerror(errno));
		cc = 0;
		goto exit_install;
	}

	/* will be called only once for all driver devices */
	major = cdv_install(infof, did, 0);
	if (major < 0) {
		printf("\ncdv_install() fails.\n""Error[#%d] -> [%s]\n"
		       "Aborting...\n",
		       errno, strerror(errno));
		cc = 0;
		goto exit_install;
	}
 cdn:
	cc = create_driver_nodes(major, drvrd->DrvrName,
				 (dd)? dd->slnn : NULL, DEF_CLIENT_CTXT_AM);
	printf("%d device nodes created.\n", cc);
	cc = 1;	/* all OK */

 exit_install:
	if (drvrf) free(drvrf);
	if (addropt) free(addropt);
	if (infof) { unlink(infof); free(infof); }
	if (options) free(options);
	return cc;
}

int main(int argc, char *argv[], char *envp[])
{
	struct drvrd *dd;
	int drvrcntr = 0;
	int flg;		/* I_ALL/I_CHOSEN/I_EXCLUDED */
	char *xmlfn = NULL;	/* config filename */
	LIST_HEAD(head);	/* command line drivers linked list */
	InsLibHostDesc *hostd = NULL;
	InsLibDrvrDesc *drvrd = NULL;
	int rc = EXIT_SUCCESS;

	umask(0000); /* we need _exact_ mode for mknod */

	/* parse command line */
	rc = parse_prog_args(argc, argv, &flg, &xmlfn, &head);

	if (IS_ERR_VALUE(rc))
		goto inst_exit;

	hostd = InsLibParseInstallFile(xmlfn, 0);
	if (!hostd) {
		rc = EXIT_FAILURE;
		goto inst_exit;
	}

	if (flg == I_ALL) {
		/* need to install all drivers from .xml config file */
		while ((drvrd = InsLibGetDriver(hostd, NULL))) {
			dd = cmd_line_dd(&head, drvrd->DrvrName);
			drvrcntr += do_install_driver(drvrd, dd);
		}
	} else if (flg == I_CHOSEN) {
		/* install only drivers, provided in the command line */
		list_for_each_entry(dd, &head, list) {
			drvrd = InsLibGetDriver(hostd, dd->dname);
			if (drvrd)
				drvrcntr += do_install_driver(drvrd, dd);
			else
				printf(ERR_MSG"[%s] driver description is NOT"
				       " found in the configuration file!",
				       dd->dname);
		}
	} else {
		/* install all, but drivers provided in the command line */
		while ((drvrd = InsLibGetDriver(hostd, NULL))) {
			dd = cmd_line_dd(&head, drvrd->DrvrName);
			if (!dd)
				drvrcntr += do_install_driver(drvrd, dd);
			else
				printf(WARNING_MSG"[%s] driver excluded from "
				       "installation", dd->dname);
		}
	}

	printf("\n%d drivers were installed. Bye...\n", drvrcntr);

 inst_exit:
	free_drvrd(&head);
	if (xmlfn) free(xmlfn);

	exit(rc);
}
