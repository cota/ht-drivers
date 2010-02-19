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

/**
 *  @brief Obtain absolute address of mezzanine-owned
 *  address space in carrier bus
 *
 *  The carrier card usually assigns addresses for memory-mapped I/O to
 *  each plugged module. This routine provides a means for a plugged
 *  module (or, better, the driver controlling it) to know the address
 *  spaces it can deal with. For flexibility, several address spaces
 *  can be recovered by selecting different values of
 *  address_space_number. A negative vaue is returned if no AS corresponds to the
 *  query; this return value signals that we have no more AS available
 *  when address_space_number exceeds the number of spaces assigned to
 *  the card, for example.
 *
 *  @param as			Where to return the a.s. parameters
 *  @param board_number		Logical module number of carrier board (1..N)
 *  @param board_position	Slot of mezzanine in carrier bus (1..M)
 *  @param address_space_number	Identify several possible address spaces (0..L)
 *
 *  @return 0 on success
 *  @return <0 on failure
 */
int get_address_space_prototype(
	struct carrier_as *as,
	int board_number,
	int board_position,
	int address_space_number);

/**
 *  @brief Install hook for interrupt forwarding
 *
 *  An interrupt vector or IRQ is (possibly) assigned to the carrier
 *  board. Each carrier knows (in a module-specific way) how to
 *  find out the mezzanine causing the interrupt request to occur;
 *  it is the driver's mission to forward the IRQ by calling a
 *  (mezzanine-driver provided) hook to handle the interrupt.
 *
 *  This routine allows a mezzanine driver to install that hook
 *
 *  @param isr_callback		Callback to which interrupt handling is
 *  				to be forwarded when interrupt requests
 *  				come from mezzanine identified by (board_nmumber,
 *  				board_position)
 *  @param board_number		Logical module number of carrier board (1..N)
 *  @param board_position	Slot of mezzanine in carrier bus (1..M)
 *  @param extra		For additional info (cookies, whatever)
 */
int register_isr_prototype(
	int (*isr_callback)(
		int board_number,
		int board_position,
		void* extra));

/* @brief type of a get_address_space entry point */
typedef int (*gas_t)(
	struct carrier_as *as,
	int board_number,
	int board_position,
	int address_space_number);

/* @brief type of a register_isr entry point */
typedef int (*risr_t)(
	int (*isr_callback)(
		int board_number,
		int board_position,
		void* extra));

/** @brief register a carrier's entry points in the carrier
 *  register module
 *  @param carrier_type 	official name of the driver
 *  @param get_address_space 	its gas_t entry point
 *  @param register_isr 	its risr_t entry point
 *
 *  @return 0 on success
 *  @return <0 on failure
 */
int carrier_register(
	char   *carrier_type,
	gas_t   get_address_space,
	risr_t  register_isr);

/** @brief get a carrier's entry point for getting address spaces
 *  @param - official name of the carrier driver
 *
 *  @return - a valid pointer to the entry point
 *  @return - NULL on failure
 */
gas_t	carrier_as_entry(char *carrier);

/** @brief get a carrier's entry point for registering ISR callbacks
 *  @param - official name of the carrier driver
 *
 *  @return - a valid pointer to the entry point
 *  @return - NULL on failure
 */
risr_t	carrier_isr_entry(char *carrier);

#endif /*_CARRIER_H_ */
