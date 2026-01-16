#include <zephyr/devicetree.h>
#include <zephyr/linker/sections.h>

/*
 * Two NOLOAD sections so they don’t occupy image size in flash:
 *  - .priv_scratch  : will be protected by MPC as privileged-only
 *  - .unpriv_scratch: remains accessible to unprivileged code
 */
/* ---- SRAM privileged scratch ---- */
SECTION_DATA_PROLOGUE(.sram_priv_scratch,(NOLOAD),)
{
    __sram_priv_scratch_start = .;
    KEEP(*(.sram_priv_scratch))
    __sram_priv_scratch_end = .;
} GROUP_DATA_LINK_IN(RAMABLE_REGION, RAMABLE_REGION)
