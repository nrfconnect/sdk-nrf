/* File generated by C:/ncs.sojkk/nrf/cmake/../scripts/partition_manager_output.py, do not modify */
#ifndef PM_CONFIG_H__
#define PM_CONFIG_H__
#define PM_MCUBOOT_ADDRESS 0x0
#define PM_MCUBOOT_SIZE 0xc000
#define PM_MCUBOOT_ID 0
#define PM_MCUBOOT_NAME mcuboot
#define PM_0_LABEL MCUBOOT
#define PM_MCUBOOT_DEV_NAME "NRF_FLASH_DRV_NAME"
#define PM_MCUBOOT_PAD_ADDRESS 0xc000
#define PM_MCUBOOT_PAD_SIZE 0x200
#define PM_MCUBOOT_PAD_ID 1
#define PM_MCUBOOT_PAD_NAME mcuboot_pad
#define PM_1_LABEL MCUBOOT_PAD
#define PM_MCUBOOT_PAD_DEV_NAME "NRF_FLASH_DRV_NAME"
#define PM_MCUBOOT_PRIMARY_ADDRESS 0xc000
#define PM_MCUBOOT_PRIMARY_SIZE 0x69000
#define PM_MCUBOOT_PRIMARY_ID 2
#define PM_MCUBOOT_PRIMARY_NAME mcuboot_primary
#define PM_2_LABEL MCUBOOT_PRIMARY
#define PM_MCUBOOT_PRIMARY_DEV_NAME "NRF_FLASH_DRV_NAME"
#define PM_MCUBOOT_PRIMARY_APP_ADDRESS 0xc200
#define PM_MCUBOOT_PRIMARY_APP_SIZE 0x68e00
#define PM_MCUBOOT_PRIMARY_APP_ID 3
#define PM_MCUBOOT_PRIMARY_APP_NAME mcuboot_primary_app
#define PM_3_LABEL MCUBOOT_PRIMARY_APP
#define PM_MCUBOOT_PRIMARY_APP_DEV_NAME "NRF_FLASH_DRV_NAME"
#define PM_SPM_ADDRESS 0xc200
#define PM_SPM_SIZE 0xc000
#define PM_SPM_ID 4
#define PM_SPM_NAME spm
#define PM_4_LABEL SPM
#define PM_SPM_DEV_NAME "NRF_FLASH_DRV_NAME"
#define PM_APP_ADDRESS 0x18200
#define PM_APP_SIZE 0x5ce00
#define PM_APP_ID 5
#define PM_APP_NAME app
#define PM_5_LABEL APP
#define PM_APP_DEV_NAME "NRF_FLASH_DRV_NAME"
#define PM_MCUBOOT_SECONDARY_ADDRESS 0x75000
#define PM_MCUBOOT_SECONDARY_SIZE 0x69000
#define PM_MCUBOOT_SECONDARY_ID 6
#define PM_MCUBOOT_SECONDARY_NAME mcuboot_secondary
#define PM_6_LABEL MCUBOOT_SECONDARY
#define PM_MCUBOOT_SECONDARY_DEV_NAME "NRF_FLASH_DRV_NAME"
#define PM_MCUBOOT_SCRATCH_ADDRESS 0xde000
#define PM_MCUBOOT_SCRATCH_SIZE 0x1e000
#define PM_MCUBOOT_SCRATCH_ID 7
#define PM_MCUBOOT_SCRATCH_NAME mcuboot_scratch
#define PM_7_LABEL MCUBOOT_SCRATCH
#define PM_MCUBOOT_SCRATCH_DEV_NAME "NRF_FLASH_DRV_NAME"
#define PM_MCUBOOT_STORAGE_ADDRESS 0xfc000
#define PM_MCUBOOT_STORAGE_SIZE 0x4000
#define PM_MCUBOOT_STORAGE_ID 8
#define PM_MCUBOOT_STORAGE_NAME mcuboot_storage
#define PM_8_LABEL MCUBOOT_STORAGE
#define PM_MCUBOOT_STORAGE_DEV_NAME "NRF_FLASH_DRV_NAME"
#define PM_NUM 9
#define PM_ALL_BY_SIZE "mcuboot_pad mcuboot_storage mcuboot spm mcuboot_scratch app mcuboot_primary_app mcuboot_secondary mcuboot_primary"
#define PM_ADDRESS 0x18200
#define PM_SIZE 0x5ce00
#endif /* PM_CONFIG_H__ */