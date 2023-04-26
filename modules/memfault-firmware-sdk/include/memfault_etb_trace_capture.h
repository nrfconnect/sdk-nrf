#ifndef MEMFAULT_ETB_TRACE_CAPTURE_H_
#define MEMFAULT_ETB_TRACE_CAPTURE_H_

#include <zephyr/types.h>
#include <memfault/panics/platform/coredump.h>

//! Called to stop ETM trace collection in the ETB when a fault occurs
//!
//! This function must be called as soon as possible after a fault occurs
//! Most commonly this should be called from `memfault_platform_fault_handler`
void memfault_ncs_etb_fault_handler(void);

//! Function to retrieve coredump region info for ETB data and configuration variables
//!
//! This function is typically called from within `memfault_platform_coredump_get_regions`
//! when ETB capture is enabled
//!
//! @param regions Pointer to regions array to store ETB region info
//! @param num_regions Number of unused entries in regions array. Used for bounds checking
//! @returns Number of regions collected
size_t memfault_ncs_etb_get_regions(sMfltCoredumpRegion *regions, size_t num_regions);

#endif  // MEMFAULT_ETB_TRACE_CAPTURE_H_