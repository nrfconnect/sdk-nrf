#if USE_PARTITION_MANAGER
	#include <pm_config.h>
	#define NON_SECURE_APP_ADDRESS PM_APP_ADDRESS
	#if !LINK_INTO_s1 /* No need to check this when linking to S1 */
	ASSERT(((_image_rom_start + _flash_used - 1)
			/ CONFIG_NRF_SPU_FLASH_REGION_SIZE)
		< (NON_SECURE_APP_ADDRESS / CONFIG_NRF_SPU_FLASH_REGION_SIZE),
		"SPM and app are sharing an SPU region. \
Cannot partition flash correctly into secure and non-secure. \
Adjust partitions sizes so they are placed in separate regions." )
	#endif /* !LINK_INTO_s1 */
#endif /* USE_PARTITION_MANAGER */
