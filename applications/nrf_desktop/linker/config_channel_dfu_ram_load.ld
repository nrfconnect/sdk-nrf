/* Validate that the executable RAM region does not overlap with the MCUboot RAM region. */
ASSERT(__rom_region_end <= CONFIG_DESKTOP_CONFIG_CHANNEL_DFU_MCUBOOT_RAM_LOAD_MCUBOOT_RAM_START_ADDR,
	"The executable RAM region overlaps with the MCUboot RAM region");
