#include <zephyr/devicetree.h>
#include <zephyr/linker/sections.h>

/* scratch area to be configure for privileged and unprivileged testing */
SECTION_DATA_PROLOGUE(.sram_priv_scratch,(NOLOAD),)
{
    __sram_priv_scratch_start = .;
    KEEP(*(.sram_priv_scratch))
    __sram_priv_scratch_end = .;
} GROUP_DATA_LINK_IN(RAMABLE_REGION, RAMABLE_REGION)
