SECTION_PROLOGUE(shared_region,,)
{
    KEEP(*(.shared_region))
    ASSERT(ADDR(shared_region) < (ORIGIN(RAM) + LENGTH(RAM) / 2),
           "shared_region not in first half of RAM");
} > RAM
