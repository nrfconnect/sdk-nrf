/* Validate that the ROM section from the application RAM region does not overlap with the
 * MCUboot RAM region. This ROM section defines the memory space where MCUboot copies the
 * application image. If this copied data overlaps with the MCUboot RAM region, MCUboot may
 * override its own RAM during the copy operation. The validation process assumes a specific
 * RAM layout in DTS - you can refer to the following file as an example:
 * ./configuration/nrf54lm20dk_nrf54lm20a_cpuapp/memory_map_ram_load.dtsi
 */
ASSERT(__rom_region_end <= CONFIG_DESKTOP_MCUBOOT_RAM_LOAD_MCUBOOT_RAM_START_ADDR,
	"The executable RAM region overlaps with the MCUboot RAM region");
