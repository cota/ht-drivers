#include <linux/kernel.h>
#include <linux/module.h>

#include "modulbus_register.h"

#define DRIVER_NAME_MAX_LENGTH	128
#define REGISTER_SIZE		32
#define	MODULE_NAME		"modulbus"
#define PFX 			MODULE_NAME ": "

struct register_entry {
	char	name[DRIVER_NAME_MAX_LENGTH];
	gas_t	get_address_space;
	risr_t	register_isr;
};

static struct register_entry	cregister[REGISTER_SIZE];
static int			used_entries = 0;
static DEFINE_MUTEX(register_mutex);

/* module initialization and cleanup */
static int __init init(void)
{
	printk(KERN_INFO PFX "carrier register module init\n");
	return 0;
}

static void __exit exit(void)
{
	printk(KERN_INFO PFX "carrier register module exit\n");
}

/* check whether carrier driver is already registered */
static int find_carrier_index(char *name)
{
	int i;

	for (i = 0; i < used_entries; i++)
		if (strcmp(cregister[i].name, name) == 0)
			return i;
	return -1;
}

/** @brief register a carrier driver's entry points 
 *
 *  @param name The official carrier driver name
 *  @param gas	Its get_address_space entry point
 *  @param risr	Its register_isr entry point
 */
int modulbus_carrier_register(char *name, gas_t gas, risr_t risr)
{
	int err = -1;
	struct register_entry *this;

	printk(KERN_INFO PFX "registering carrier %s\n", name);
	if (used_entries >= REGISTER_SIZE) {
		printk(KERN_ERR PFX "too many carriers\n");
		return err;
	}
	if (strlen(name) > DRIVER_NAME_MAX_LENGTH) {
		printk(KERN_ERR PFX "carrier name too long\n");
		return err;
	}

	mutex_lock(&register_mutex);
	if (find_carrier_index(name) >= 0) {
		printk(KERN_ERR PFX "carrier %s already registered\n",
			name);
		goto fail;
	}
	this = &(cregister[used_entries++]);
	snprintf(this->name, DRIVER_NAME_MAX_LENGTH, "%s", name);
	this->get_address_space = gas;
	this->register_isr 	= risr;
	err = 0;
	printk(KERN_INFO PFX 
		"carrier %s get_address_space entry point at %p\n"
		"carrier %s register_isr  entry point at %p\n",
		name, gas, name, risr);

fail:	mutex_unlock(&register_mutex);
	return err;
}
EXPORT_SYMBOL(modulbus_carrier_register);

int modulbus_carrier_unregister(char *name)
{
	int idx, i;

	printk(KERN_INFO PFX "unregistering carrier %s\n", name);

	mutex_lock(&register_mutex);
	idx = find_carrier_index(name);
	if (idx < 0) {
		printk(KERN_ERR PFX "carrier %s not found\n", name);
		mutex_unlock(&register_mutex);
		return -1;
	}
	used_entries--;
	for (i = idx; i < used_entries; i++)
		cregister[i] = cregister[i+1];

	mutex_unlock(&register_mutex);
	return 0;
}
EXPORT_SYMBOL(modulbus_carrier_unregister);

/** @brief get a carrier's entry point for getting address spaces 
 *  @param - official name of the carrier driver
 *
 *  @return - a valid pointer to the entry point
 *  @return - NULL on failure
 */
gas_t	modulbus_carrier_as_entry(char *carrier)
{
	int i;

	for (i = 0; i < used_entries; i++) {
		if (strcmp(carrier, cregister[i].name) == 0)
			return cregister[i].get_address_space;
	}
	return NULL;
}
EXPORT_SYMBOL(modulbus_carrier_as_entry);

/** @brief get a carrier's entry point for registering ISR callbacks 
 *  @param - official name of the carrier driver
 *
 *  @return - a valid pointer to the entry point
 *  @return - NULL on failure
 */
risr_t	modulbus_carrier_isr_entry(char *carrier)
{
	int i;

	for (i = 0; i < used_entries; i++) {
		if (strcmp(carrier, cregister[i].name) == 0)
			return cregister[i].register_isr;
	}
	return NULL;
}
EXPORT_SYMBOL(modulbus_carrier_isr_entry);

/* usual housekeeping */
MODULE_LICENSE("GPL");
module_init(init);
module_exit(exit);


