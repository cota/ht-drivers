#include <linux/kernel.h>
#include <linux/module.h>

#include "carrier.h"

#define DRIVER_NAME_MAX_LENGTH	128
#define REGISTER_SIZE		32

struct register_entry {
	char	name[DRIVER_NAME_MAX_LENGTH];
	gas_t	get_address_space;
	risr_t	register_isr;
};

static struct register_entry	cregister[REGISTER_SIZE];
static int			used_entries = 0;
DEFINE_MUTEX(register_mutex);

/* module initialization and cleanup */
static int __init init(void)
{
	printk(KERN_INFO "carrier register module init\n");
	return 0;
}

static void __exit exit(void)
{
	printk(KERN_INFO "carrier register module exit\n");
}

/* check whether carrier driver is already registered */
static int already_present(char *name)
{
	int i;

	for (i = 0; i < used_entries; i++)
		if (strcmp(cregister[i].name, name) == 0)
			return 1;
	return 0;
}

/** @brief register a carrier driver's entry points
 *
 *  @param name The official carrier driver name
 *  @param gas	Its get_address_space entry point
 *  @param risr	Its register_isr entry point
 */
int carrier_register(char *name, gas_t gas, risr_t risr)
{
	int err = -1;
	struct register_entry *this;

	printk(KERN_INFO "registering carrier %s\n", name);
	if (used_entries >= REGISTER_SIZE) {
		printk(KERN_ERR "too many carriers\n");
		return err;
	}
	if (strlen(name) > DRIVER_NAME_MAX_LENGTH) {
		printk(KERN_ERR "carrier name too long\n");
		return err;
	}

	mutex_lock(&register_mutex);
	if (already_present(name))
		goto fail;
	this = &(cregister[used_entries++]);
	snprintf(this->name, DRIVER_NAME_MAX_LENGTH, "%s", name);
	this->get_address_space = gas;
	this->register_isr 	= risr;
	err = 0;
	printk(KERN_INFO
		"carrier %s get_address_space entry point at %p\n"
		"carrier %s register_isr  entry point at %p\n",
		name, gas, name, risr);

fail:	mutex_unlock(&register_mutex);
	return err;
}
EXPORT_SYMBOL(carrier_register);

/** @brief get a carrier's entry point for getting address spaces
 *  @param - official name of the carrier driver
 *
 *  @return - a valid pointer to the entry point
 *  @return - NULL on failure
 */
gas_t	carrier_as_entry(char *carrier)
{
	int i;

	for (i = 0; i < used_entries; i++) {
		if (strcmp(carrier, cregister[i].name) == 0)
			return cregister[i].get_address_space;
	}
	return NULL;
}
EXPORT_SYMBOL(carrier_as_entry);

/** @brief get a carrier's entry point for registering ISR callbacks
 *  @param - official name of the carrier driver
 *
 *  @return - a valid pointer to the entry point
 *  @return - NULL on failure
 */
risr_t	carrier_isr_entry(char *carrier)
{
	int i;

	for (i = 0; i < used_entries; i++) {
		if (strcmp(carrier, cregister[i].name) == 0)
			return cregister[i].register_isr;
	}
	return NULL;
}
EXPORT_SYMBOL(carrier_isr_entry);

/* usual housekeeping */
MODULE_LICENSE("GPL");
module_init(init);
module_exit(exit);


