#include <zephyr/devicetree.h>
#include <zephyr/linker/sections.h>

/*
 *  - .sram_priv_scratch  : will be protected by MPC as privileged-only
 */
SECTION_DATA_PROLOGUE(.sram_priv_scratch,(NOLOAD),)
{
    __sram_priv_scratch_start = .;
    KEEP(*(.sram_priv_scratch))
    __sram_priv_scratch_end = .;
} GROUP_DATA_LINK_IN(RAMABLE_REGION, RAMABLE_REGION)
