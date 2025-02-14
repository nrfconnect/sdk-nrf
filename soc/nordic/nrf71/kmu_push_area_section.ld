#define NRF_KMU_PUSH_BASE_ADDRESS 0x200FFF00

SECTION_PROLOGUE(NRF_KMU_RESERVED_PUSH_SECTION,(NOLOAD) ,)
{
    __nrf_kmu_reserved_push_area = .;
    *(.nrf_kmu_reserved_push_area)
    __nrf_kmu_reserved_push_area_end = .;
} GROUP_NOLOAD_LINK_IN(NRF_KMU_RESERVED_PUSH, NRF_KMU_RESERVED_PUSH)

/* Check that the section is correctly placed at 0x200F_FF00 */
ASSERT(__nrf_kmu_reserved_push_area == NRF_KMU_PUSH_BASE_ADDRESS, \
 "Error: The section NRF_KMU_RESERVED_PUSH_SECTION needs to be \
 placed at 0x200F_FF00 but it is not, please edit the\
 device tree to define section NRF_KMU_RESERVED_PUSH at 0x200F_FF00 \
 the to RAM address.")
