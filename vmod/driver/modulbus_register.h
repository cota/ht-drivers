#ifndef _CARRIER_H_
#define _CARRIER_H_

/**
 *  @brief definition of an address space as provided by the 
 *         carrier driver
 */
struct carrier_as {
	unsigned long	address;	
	int		width;		
	int		size;
	int		is_big_endian;
};


/* @brief type of a mezzanine isr */
typedef int (*isrcb_t)(
	void *device_id,
	void* extra);

/* @brief type of a get_address_space entry point */
typedef int (*gas_t)(
	struct carrier_as *as,
	int board_number, 
	int board_position, 
	int address_space_number);

/* @brief type of a register_isr entry point */
typedef int (*risr_t)(
	isrcb_t callback,
	void *dev,
	int board_number,
	int board_position);



/** @brief register a carrier's entry points in the carrier 
 *  register module
 *  @param carrier_type 	official name of the driver
 *  @param get_address_space 	its gas_t entry point
 *  @param register_isr 	its risr_t entry point
 *
 *  @return 0 on success
 *  @return <0 on failure
 */
int modulbus_carrier_register(
	char   *carrier_type,
	gas_t   get_address_space,
	risr_t  register_isr);

/** @brief unregister a carrier from the entry point list
 *
 * @param	name	official name of the driver
 * 
 * @return	0 on success
 * @return <0 on failure
 */

int modulbus_carrier_unregister(char *name);

/** @brief get a carrier's entry point for getting address spaces 
 *  @param - official name of the carrier driver
 *
 *  @return - a valid pointer to the entry point
 *  @return - NULL on failure
 */
gas_t	modulbus_carrier_as_entry(char *carrier);

/** @brief get a carrier's entry point for registering ISR callbacks 
 *  @param - official name of the carrier driver
 *
 *  @return - a valid pointer to the entry point
 *  @return - NULL on failure
 */
risr_t	modulbus_carrier_isr_entry(char *carrier);

#endif /*_CARRIER_H_ */
