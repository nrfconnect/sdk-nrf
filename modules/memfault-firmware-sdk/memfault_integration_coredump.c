#include <zephyr/kernel.h>
#include <memfault/components.h>
#include <memfault/ports/zephyr/coredump.h>
#include "memfault_etb_trace_capture.h"

static sMfltCoredumpRegion s_coredump_regions[MEMFAULT_ZEPHYR_COREDUMP_REGIONS + IS_ENABLED(CONFIG_MEMFAULT_NCS_ETB_CAPTURE)];

const sMfltCoredumpRegion
*memfault_platform_coredump_get_regions(const sCoredumpCrashInfo *crash_info, size_t *num_regions)
{
	size_t region_idx = 0;

	// Capture Zephyr regions
	region_idx += memfault_zephyr_coredump_get_regions(crash_info, s_coredump_regions,
							   MEMFAULT_ARRAY_SIZE(s_coredump_regions));

#if CONFIG_MEMFAULT_NCS_ETB_CAPTURE
	region_idx += memfault_ncs_etb_get_regions(&s_coredump_regions[region_idx],
							MEMFAULT_ARRAY_SIZE(s_coredump_regions) -
								region_idx);
#endif // CONFIG_MEMFAULT_NCS_ETB_CAPTURE

	*num_regions = region_idx;
	return s_coredump_regions;
}

void memfault_platform_fault_handler(const sMfltRegState *regs,
						   eMemfaultRebootReason reason)
{
	ARG_UNUSED(regs);
	ARG_UNUSED(reason);
#if CONFIG_MEMFAULT_NCS_ETB_CAPTURE
	memfault_ncs_etb_fault_handler();
#endif
}