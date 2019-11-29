/* Generated by Kconfiglib (https://github.com/ulfalizer/Kconfiglib) */
#define CONFIG_DESKTOP_MOTION_PMW3360_ENABLE 1
#define CONFIG_DESKTOP_MOTION_SENSOR_ENABLE 1
#define CONFIG_DESKTOP_MOTION_THREAD_STACK_SIZE 512
#define CONFIG_DESKTOP_MOTION_SENSOR_CPI 1600
#define CONFIG_DESKTOP_MOTION_SENSOR_SLEEP1_TIMEOUT_MS 500
#define CONFIG_DESKTOP_MOTION_SENSOR_SLEEP2_TIMEOUT_MS 9220
#define CONFIG_DESKTOP_MOTION_SENSOR_SLEEP3_TIMEOUT_MS 15000
#define CONFIG_DESKTOP_BUTTONS_ENABLE 1
#define CONFIG_DESKTOP_BUTTONS_SCAN_INTERVAL 6
#define CONFIG_DESKTOP_BUTTONS_DEBOUNCE_INTERVAL 1
#define CONFIG_DESKTOP_BUTTONS_POLARITY_INVERSED 1
#define CONFIG_DESKTOP_BUTTONS_EVENT_LIMIT 4
#define CONFIG_DESKTOP_CLICK_DETECTOR_ENABLE 1
#define CONFIG_DESKTOP_SELECTOR_HW_ENABLE 1
#define CONFIG_DESKTOP_WHEEL_ENABLE 1
#define CONFIG_DESKTOP_WHEEL_SENSOR_VALUE_DIVIDER 15
#define CONFIG_DESKTOP_WHEEL_SENSOR_IDLE_TIMEOUT 0
#define CONFIG_DESKTOP_LED_ENABLE 1
#define CONFIG_DESKTOP_LED_COUNT 2
#define CONFIG_DESKTOP_LED_COLOR_COUNT 3
#define CONFIG_DESKTOP_LED_BRIGHTNESS_MAX 255
#define CONFIG_DESKTOP_BATTERY_CHARGER_DISCRETE 1
#define CONFIG_DESKTOP_BATTERY_CHARGER_CSO_PIN 8
#define CONFIG_DESKTOP_BATTERY_CHARGER_CSO_PULL_UP 1
#define CONFIG_DESKTOP_BATTERY_CHARGER_ENABLE_PIN 14
#define CONFIG_DESKTOP_BATTERY_CHARGER_ENABLE_INVERSED 1
#define CONFIG_DESKTOP_BATTERY_CHARGER_CSO_FREQ 1
#define CONFIG_DESKTOP_BATTERY_MEAS 1
#define CONFIG_DESKTOP_BATTERY_MEAS_MIN_LEVEL 3100
#define CONFIG_DESKTOP_BATTERY_MEAS_MAX_LEVEL 4200
#define CONFIG_DESKTOP_BATTERY_MEAS_HAS_VOLTAGE_DIVIDER 1
#define CONFIG_DESKTOP_BATTERY_MEAS_VOLTAGE_DIVIDER_UPPER 1500
#define CONFIG_DESKTOP_BATTERY_MEAS_VOLTAGE_DIVIDER_LOWER 180
#define CONFIG_DESKTOP_VOLTAGE_TO_SOC_DELTA 10
#define CONFIG_DESKTOP_BATTERY_MEAS_POLL_INTERVAL_MS 10000
#define CONFIG_DESKTOP_BATTERY_MEAS_HAS_ENABLE_PIN 1
#define CONFIG_DESKTOP_BATTERY_MEAS_ENABLE_PIN 6
#define CONFIG_DESKTOP_POWER_MANAGER_ENABLE 1
#define CONFIG_DESKTOP_POWER_MANAGER_TIMEOUT 120
#define CONFIG_DESKTOP_POWER_MANAGER_ERROR_TIMEOUT 30
#define CONFIG_DESKTOP_REPORT_DESC "configuration/common/hid_report_desc.c"
#define CONFIG_DESKTOP_HID_STATE_ENABLE 1
#define CONFIG_DESKTOP_HID_REPORT_EXPIRATION 500
#define CONFIG_DESKTOP_HID_EVENT_QUEUE_SIZE 12
#define CONFIG_DESKTOP_LED_STREAM_ENABLE 1
#define CONFIG_DESKTOP_LED_STREAM_QUEUE_SIZE 15
#define CONFIG_DESKTOP_USB_ENABLE 1
#define CONFIG_DESKTOP_BLE_PEER_CONTROL 1
#define CONFIG_DESKTOP_BLE_PEER_CONTROL_BUTTON 0x0007
#define CONFIG_DESKTOP_BLE_PEER_SELECT 1
#define CONFIG_DESKTOP_BLE_PEER_ERASE 1
#define CONFIG_DESKTOP_BLE_DONGLE_PEER_ENABLE 1
#define CONFIG_DESKTOP_BLE_DONGLE_PEER_SELECTOR_ID 0
#define CONFIG_DESKTOP_BLE_DONGLE_PEER_SELECTOR_POS 0
#define CONFIG_DESKTOP_BLE_ADVERTISING_ENABLE 1
#define CONFIG_DESKTOP_BLE_SHORT_NAME "Mouse nRF52"
#define CONFIG_DESKTOP_BLE_DIRECT_ADV_AVAILABLE 1
#define CONFIG_DESKTOP_BLE_FAST_ADV 1
#define CONFIG_DESKTOP_BLE_SWIFT_PAIR 1
#define CONFIG_DESKTOP_BLE_FAST_ADV_TIMEOUT 30
#define CONFIG_DESKTOP_BLE_SWIFT_PAIR_GRACE_PERIOD 30
#define CONFIG_DESKTOP_BLE_SECURITY_FAIL_TIMEOUT_S 10
#define CONFIG_DESKTOP_HID_MOUSE 1
#define CONFIG_DESKTOP_HIDS_ENABLE 1
#define CONFIG_DESKTOP_HID_PERIPHERAL 1
#define CONFIG_DESKTOP_BAS_ENABLE 1
#define CONFIG_DESKTOP_CONFIG_ENABLE 1
#define CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE 1
#define CONFIG_DESKTOP_CONFIG_CHANNEL_TIMEOUT 10
#define CONFIG_DESKTOP_WATCHDOG_ENABLE 1
#define CONFIG_DESKTOP_WATCHDOG_TIMEOUT 500
#define CONFIG_DESKTOP_SETTINGS_LOAD_THREAD_STACK_SIZE 0
#define CONFIG_BLECTRL_SLAVE_COUNT 1
#define CONFIG_BLECTRL_MAX_CONN_EVENT_LEN_DEFAULT 7500
#define CONFIG_BLECTLR_PRIO 8
#define CONFIG_BLECTLR_RX_STACK_SIZE 1024
#define CONFIG_BLECTLR_SIGNAL_STACK_SIZE 1024
#define CONFIG_BT_GATT_POOL 1
#define CONFIG_BT_GATT_UUID16_POOL_SIZE 27
#define CONFIG_BT_GATT_UUID32_POOL_SIZE 0
#define CONFIG_BT_GATT_UUID128_POOL_SIZE 0
#define CONFIG_BT_GATT_CHRC_POOL_SIZE 7
#define CONFIG_BT_CONN_CTX 1
#define CONFIG_BT_CONN_CTX_MEM_BUF_ALIGN 4
#define CONFIG_BT_NRF_SERVICES 1
#define CONFIG_BT_GATT_HIDS 1
#define CONFIG_BT_GATT_HIDS_MAX_CLIENT_COUNT 1
#define CONFIG_BT_GATT_HIDS_ATTR_MAX 19
#define CONFIG_BT_GATT_HIDS_INPUT_REP_MAX 1
#define CONFIG_BT_GATT_HIDS_OUTPUT_REP_MAX 0
#define CONFIG_BT_GATT_HIDS_FEATURE_REP_MAX 1
#define CONFIG_BT_GATT_HIDS_DEFAULT_PERM_RW_ENCRYPT 1
#define CONFIG_EVENT_MANAGER 1
#define CONFIG_DESKTOP_EVENT_MANAGER_EVENT_LOG_BUF_LEN 128
#define CONFIG_MULTITHREADING_LOCK 1
#define CONFIG_HW_CC310 1
#define CONFIG_HW_CC310_NAME "HW_CC310_0"
#define CONFIG_SENSOR 1
#define CONFIG_PMW3360 1
#define CONFIG_PMW3360_CPI 1600
#define CONFIG_PMW3360_RUN_DOWNSHIFT_TIME_MS 500
#define CONFIG_PMW3360_REST1_DOWNSHIFT_TIME_MS 9220
#define CONFIG_PMW3360_REST2_DOWNSHIFT_TIME_MS 150000
#define CONFIG_PMW3360_ORIENTATION_90 1
#define CONFIG_CLOCK_CONTROL_NRFXLIB 1
#define CONFIG_CLOCK_CONTROL_NRF_K32SRC_XTAL 1
#define CONFIG_CLOCK_CONTROL_NRF_K32SRC_BLOCKING 1
#define CONFIG_CLOCK_CONTROL_NRF_K32SRC_20PPM 1
#define CONFIG_ENTROPY_NRF_LL_NRFXLIB 1
#define CONFIG_ENTROPY_NRF_PRI 5
#define CONFIG_SOC_FLASH_NRF_LL_NRFXLIB 1
#define CONFIG_BOOT_SIGNATURE_KEY_FILE "root-rsa-2048.pem"
#define CONFIG_DT_FLASH_WRITE_BLOCK_SIZE 4
#define CONFIG_BT_LL_NRFXLIB 1
#define CONFIG_BLE_CONTROLLER_S140 1
#define CONFIG_NRFXLIB_CRYPTO 1
#define CONFIG_NRF_CC310_PLATFORM 1
#define CONFIG_MBEDTLS_SHA256_SMALLER 1
#define CONFIG_MBEDTLS_TLS_VERSION_1_2 1
#define CONFIG_MBEDTLS_KEY_EXCHANGE_RSA_ENABLED 1
#define CONFIG_MBEDTLS_CIPHER_AES_ENABLED 1
#define CONFIG_MBEDTLS_AES_ROM_TABLES 1
#define CONFIG_MBEDTLS_CIPHER_DES_ENABLED 1
#define CONFIG_MBEDTLS_CIPHER_CBC_ENABLED 1
#define CONFIG_MBEDTLS_MAC_MD5_ENABLED 1
#define CONFIG_MBEDTLS_MAC_SHA1_ENABLED 1
#define CONFIG_MBEDTLS_MAC_SHA256_ENABLED 1
#define CONFIG_MBEDTLS_CTR_DRBG_ENABLED 1
#define CONFIG_HAS_NRFX 1
#define CONFIG_NRFX_NVMC 1
#define CONFIG_NRFX_PWM 1
#define CONFIG_NRFX_PWM0 1
#define CONFIG_NRFX_PWM1 1
#define CONFIG_NRFX_QDEC 1
#define CONFIG_NRFX_SPIM 1
#define CONFIG_NRFX_SPIM1 1
#define CONFIG_NRFX_SYSTICK 1
#define CONFIG_NRFX_USBD 1
#define CONFIG_NRFX_WDT 1
#define CONFIG_NRFX_WDT0 1
#define CONFIG_BOARD "nrf52840_pca20041"
#define CONFIG_USB_NRF52840 1
#define CONFIG_USB_DEVICE_STACK 1
#define CONFIG_SOC "nRF52840_QIAA"
#define CONFIG_SOC_SERIES "nrf52"
#define CONFIG_NUM_IRQS 48
#define CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC 32768
#define CONFIG_WATCHDOG 1
#define CONFIG_GPIO 1
#define CONFIG_ARCH_HAS_CUSTOM_BUSY_WAIT 1
#define CONFIG_ISR_STACK_SIZE 1536
#define CONFIG_TEMP_NRF5 1
#define CONFIG_NET_CONFIG_IEEE802154_DEV_NAME ""
#define CONFIG_CLOCK_CONTROL 1
#define CONFIG_NRF_RTC_TIMER 1
#define CONFIG_SYS_CLOCK_TICKS_PER_SEC 1000
#define CONFIG_SYS_POWER_MANAGEMENT 1
#define CONFIG_BUILD_OUTPUT_HEX 1
#define CONFIG_TEXT_SECTION_OFFSET 0x0
#define CONFIG_FLASH_SIZE 1024
#define CONFIG_FLASH_BASE_ADDRESS 0x0
#define CONFIG_SRAM_SIZE 256
#define CONFIG_SRAM_BASE_ADDRESS 0x20000000
#define CONFIG_SPI 1
#define CONFIG_TINYCRYPT 1
#define CONFIG_SOC_GECKO_EMU 1
#define CONFIG_BOARD_NRF52840_PCA20041 1
#define CONFIG_BOARD_HAS_DCDC 1
#define CONFIG_SOC_SERIES_NRF52X 1
#define CONFIG_CPU_HAS_ARM_MPU 1
#define CONFIG_SOC_FAMILY "nordic_nrf"
#define CONFIG_SOC_FAMILY_NRF 1
#define CONFIG_HAS_HW_NRF_ACL 1
#define CONFIG_HAS_HW_NRF_CC310 1
#define CONFIG_HAS_HW_NRF_CCM 1
#define CONFIG_HAS_HW_NRF_CLOCK 1
#define CONFIG_HAS_HW_NRF_COMP 1
#define CONFIG_HAS_HW_NRF_ECB 1
#define CONFIG_HAS_HW_NRF_EGU0 1
#define CONFIG_HAS_HW_NRF_EGU1 1
#define CONFIG_HAS_HW_NRF_EGU2 1
#define CONFIG_HAS_HW_NRF_EGU3 1
#define CONFIG_HAS_HW_NRF_EGU4 1
#define CONFIG_HAS_HW_NRF_EGU5 1
#define CONFIG_HAS_HW_NRF_GPIO0 1
#define CONFIG_HAS_HW_NRF_GPIO1 1
#define CONFIG_HAS_HW_NRF_GPIOTE 1
#define CONFIG_HAS_HW_NRF_I2S 1
#define CONFIG_HAS_HW_NRF_LPCOMP 1
#define CONFIG_HAS_HW_NRF_MWU 1
#define CONFIG_HAS_HW_NRF_NFCT 1
#define CONFIG_HAS_HW_NRF_PDM 1
#define CONFIG_HAS_HW_NRF_POWER 1
#define CONFIG_HAS_HW_NRF_PPI 1
#define CONFIG_HAS_HW_NRF_PWM0 1
#define CONFIG_HAS_HW_NRF_PWM1 1
#define CONFIG_HAS_HW_NRF_PWM2 1
#define CONFIG_HAS_HW_NRF_PWM3 1
#define CONFIG_HAS_HW_NRF_QDEC 1
#define CONFIG_HAS_HW_NRF_QSPI 1
#define CONFIG_HAS_HW_NRF_RADIO_BLE_CODED 1
#define CONFIG_HAS_HW_NRF_RADIO_IEEE802154 1
#define CONFIG_HAS_HW_NRF_RNG 1
#define CONFIG_HAS_HW_NRF_RTC0 1
#define CONFIG_HAS_HW_NRF_RTC1 1
#define CONFIG_HAS_HW_NRF_RTC2 1
#define CONFIG_HAS_HW_NRF_SAADC 1
#define CONFIG_HAS_HW_NRF_SPI0 1
#define CONFIG_HAS_HW_NRF_SPI1 1
#define CONFIG_HAS_HW_NRF_SPI2 1
#define CONFIG_HAS_HW_NRF_SPIM0 1
#define CONFIG_HAS_HW_NRF_SPIM1 1
#define CONFIG_HAS_HW_NRF_SPIM2 1
#define CONFIG_HAS_HW_NRF_SPIM3 1
#define CONFIG_HAS_HW_NRF_SPIS0 1
#define CONFIG_HAS_HW_NRF_SPIS1 1
#define CONFIG_HAS_HW_NRF_SPIS2 1
#define CONFIG_HAS_HW_NRF_SWI0 1
#define CONFIG_HAS_HW_NRF_SWI1 1
#define CONFIG_HAS_HW_NRF_SWI2 1
#define CONFIG_HAS_HW_NRF_SWI3 1
#define CONFIG_HAS_HW_NRF_SWI4 1
#define CONFIG_HAS_HW_NRF_SWI5 1
#define CONFIG_HAS_HW_NRF_TEMP 1
#define CONFIG_HAS_HW_NRF_TIMER0 1
#define CONFIG_HAS_HW_NRF_TIMER1 1
#define CONFIG_HAS_HW_NRF_TIMER2 1
#define CONFIG_HAS_HW_NRF_TIMER3 1
#define CONFIG_HAS_HW_NRF_TIMER4 1
#define CONFIG_HAS_HW_NRF_TWI0 1
#define CONFIG_HAS_HW_NRF_TWI1 1
#define CONFIG_HAS_HW_NRF_TWIM0 1
#define CONFIG_HAS_HW_NRF_TWIM1 1
#define CONFIG_HAS_HW_NRF_TWIS0 1
#define CONFIG_HAS_HW_NRF_TWIS1 1
#define CONFIG_HAS_HW_NRF_UART0 1
#define CONFIG_HAS_HW_NRF_UARTE0 1
#define CONFIG_HAS_HW_NRF_UARTE1 1
#define CONFIG_HAS_HW_NRF_USBD 1
#define CONFIG_HAS_HW_NRF_WDT 1
#define CONFIG_SOC_NRF52840 1
#define CONFIG_SOC_NRF52840_QIAA 1
#define CONFIG_SOC_DCDC_NRF52X 1
#define CONFIG_NRF_ENABLE_ICACHE 1
#define CONFIG_SOC_COMPATIBLE_NRF 1
#define CONFIG_SOC_COMPATIBLE_NRF52X 1
#define CONFIG_CPU_CORTEX 1
#define CONFIG_CPU_CORTEX_M 1
#define CONFIG_ISA_THUMB2 1
#define CONFIG_STACK_ALIGN_DOUBLE_WORD 1
#define CONFIG_PLATFORM_SPECIFIC_INIT 1
#define CONFIG_FAULT_DUMP 2
#define CONFIG_CPU_CORTEX_M4 1
#define CONFIG_CPU_CORTEX_M_HAS_SYSTICK 1
#define CONFIG_CPU_CORTEX_M_HAS_BASEPRI 1
#define CONFIG_CPU_CORTEX_M_HAS_VTOR 1
#define CONFIG_CPU_CORTEX_M_HAS_PROGRAMMABLE_FAULT_PRIOS 1
#define CONFIG_ARMV7_M_ARMV8_M_MAINLINE 1
#define CONFIG_ARMV7_M_ARMV8_M_FP 1
#define CONFIG_XIP 1
#define CONFIG_GEN_ISR_TABLES 1
#define CONFIG_ZERO_LATENCY_IRQS 1
#define CONFIG_GEN_IRQ_VECTOR_TABLE 1
#define CONFIG_ARM_MPU 1
#define CONFIG_ARM_MPU_REGION_MIN_ALIGN_AND_SIZE 32
#define CONFIG_MPU_ALLOW_FLASH_WRITE 1
#define CONFIG_CUSTOM_SECTION_MIN_ALIGN_SIZE 32
#define CONFIG_ARCH "arm"
#define CONFIG_ARM 1
#define CONFIG_PRIVILEGED_STACK_SIZE 1024
#define CONFIG_PRIVILEGED_STACK_TEXT_AREA 256
#define CONFIG_KOBJECT_TEXT_AREA 256
#define CONFIG_GEN_SW_ISR_TABLE 1
#define CONFIG_ARCH_SW_ISR_TABLE_ALIGN 0
#define CONFIG_GEN_IRQ_START_VECTOR 0
#define CONFIG_ARCH_HAS_STACK_PROTECTION 1
#define CONFIG_ARCH_HAS_USERSPACE 1
#define CONFIG_ARCH_HAS_EXECUTABLE_PAGE_BIT 1
#define CONFIG_ARCH_HAS_RAMFUNC_SUPPORT 1
#define CONFIG_ARCH_HAS_NESTED_EXCEPTION_DETECTION 1
#define CONFIG_ARCH_HAS_THREAD_ABORT 1
#define CONFIG_HAS_SYS_POWER_STATE_DEEP_SLEEP_1 1
#define CONFIG_CPU_HAS_FPU 1
#define CONFIG_CPU_HAS_MPU 1
#define CONFIG_MEMORY_PROTECTION 1
#define CONFIG_MPU_REQUIRES_POWER_OF_TWO_ALIGNMENT 1
#define CONFIG_MULTITHREADING 1
#define CONFIG_NUM_COOP_PRIORITIES 10
#define CONFIG_NUM_PREEMPT_PRIORITIES 11
#define CONFIG_MAIN_THREAD_PRIORITY 0
#define CONFIG_COOP_ENABLED 1
#define CONFIG_PREEMPT_ENABLED 1
#define CONFIG_PRIORITY_CEILING 0
#define CONFIG_NUM_METAIRQ_PRIORITIES 0
#define CONFIG_MAIN_STACK_SIZE 768
#define CONFIG_IDLE_STACK_SIZE 128
#define CONFIG_THREAD_STACK_INFO 1
#define CONFIG_ERRNO 1
#define CONFIG_SCHED_DUMB 1
#define CONFIG_WAITQ_DUMB 1
#define CONFIG_BOOT_DELAY 0
#define CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE 1536
#define CONFIG_SYSTEM_WORKQUEUE_PRIORITY -1
#define CONFIG_OFFLOAD_WORKQUEUE_STACK_SIZE 1024
#define CONFIG_OFFLOAD_WORKQUEUE_PRIORITY -1
#define CONFIG_ATOMIC_OPERATIONS_BUILTIN 1
#define CONFIG_TIMESLICING 1
#define CONFIG_TIMESLICE_SIZE 0
#define CONFIG_TIMESLICE_PRIORITY 0
#define CONFIG_POLL 1
#define CONFIG_NUM_MBOX_ASYNC_MSGS 10
#define CONFIG_NUM_PIPE_ASYNC_MSGS 10
#define CONFIG_HEAP_MEM_POOL_SIZE 512
#define CONFIG_HEAP_MEM_POOL_MIN_SIZE 32
#define CONFIG_ARCH_HAS_CUSTOM_SWAP_TO_MAIN 1
#define CONFIG_SWAP_NONATOMIC 1
#define CONFIG_SYS_CLOCK_EXISTS 1
#define CONFIG_KERNEL_INIT_PRIORITY_OBJECTS 30
#define CONFIG_KERNEL_INIT_PRIORITY_DEFAULT 40
#define CONFIG_KERNEL_INIT_PRIORITY_DEVICE 50
#define CONFIG_APPLICATION_INIT_PRIORITY 90
#define CONFIG_STACK_POINTER_RANDOM 0
#define CONFIG_MP_NUM_CPUS 1
#define CONFIG_TICKLESS_IDLE 1
#define CONFIG_TICKLESS_IDLE_THRESH 3
#define CONFIG_TICKLESS_KERNEL 1
#define CONFIG_SYS_POWER_DEEP_SLEEP_STATES 1
#define CONFIG_SYS_PM_POLICY_APP 1
#define CONFIG_DEVICE_POWER_MANAGEMENT 1
#define CONFIG_HAS_DTS 1
#define CONFIG_HAS_DTS_GPIO 1
#define CONFIG_HAS_DTS_WDT 1
#define CONFIG_SYSTEM_CLOCK_DISABLE 1
#define CONFIG_SYSTEM_CLOCK_INIT_PRIORITY 0
#define CONFIG_TICKLESS_CAPABLE 1
#define CONFIG_ENTROPY_GENERATOR 1
#define CONFIG_ENTROPY_NRF_FORCE_ALT 1
#define CONFIG_ENTROPY_HAS_DRIVER 1
#define CONFIG_ENTROPY_NAME "ENTROPY_0"
#define CONFIG_GPIO_NRFX 1
#define CONFIG_GPIO_NRF_INIT_PRIORITY 40
#define CONFIG_GPIO_NRF_P0 1
#define CONFIG_GPIO_NRF_P1 1
#define CONFIG_SPI_INIT_PRIORITY 70
#define CONFIG_SPI_1 1
#define CONFIG_SPI_1_OP_MODES 1
#define CONFIG_SPI_NRFX 1
#define CONFIG_SPI_1_NRF_SPIM 1
#define CONFIG_SPI_1_NRF_ORC 0xff
#define CONFIG_SPI_NRFX_RAM_BUFFER_SIZE 8
#define CONFIG_SPI_NRFX_SPIM_MISO_NO_PULL 1
#define CONFIG_PWM 1
#define CONFIG_PWM_0 1
#define CONFIG_PWM_1 1
#define CONFIG_PWM_NRFX 1
#define CONFIG_ADC 1
#define CONFIG_ADC_CONFIGURABLE_INPUTS 1
#define CONFIG_ADC_ASYNC 1
#define CONFIG_ADC_0 1
#define CONFIG_ADC_NRFX_SAADC 1
#define CONFIG_WDT_NRFX 1
#define CONFIG_CLOCK_CONTROL_NRF_FORCE_ALT 1
#define CONFIG_FLASH_HAS_DRIVER_ENABLED 1
#define CONFIG_FLASH_HAS_PAGE_LAYOUT 1
#define CONFIG_FLASH 1
#define CONFIG_FLASH_PAGE_LAYOUT 1
#define CONFIG_FLASH_NRF_FORCE_ALT 1
#define CONFIG_SENSOR_INIT_PRIORITY 90
#define CONFIG_QDEC_NRFX 1
#define CONFIG_USB 1
#define CONFIG_USB_DEVICE_DRIVER 1
#define CONFIG_USB_NRFX_EVT_QUEUE_SIZE 32
#define CONFIG_USB_NRFX_WORK_QUEUE_STACK_SIZE 1024
#define CONFIG_MINIMAL_LIBC 1
#define CONFIG_MINIMAL_LIBC_MALLOC_ARENA_SIZE 0
#define CONFIG_POSIX_MAX_FDS 4
#define CONFIG_MAX_TIMER_COUNT 5
#define CONFIG_BT 1
#define CONFIG_BT_HCI 1
#define CONFIG_BT_PERIPHERAL 1
#define CONFIG_BT_BROADCASTER 1
#define CONFIG_BT_GATT_DIS 1
#define CONFIG_BT_GATT_DIS_MODEL "Mouse nRF52 Desktop"
#define CONFIG_BT_GATT_DIS_MANUF "Nordic Semiconductor ASA"
#define CONFIG_BT_GATT_DIS_PNP 1
#define CONFIG_BT_GATT_DIS_PNP_VID_SRC 2
#define CONFIG_BT_GATT_DIS_PNP_VID 0x1915
#define CONFIG_BT_GATT_DIS_PNP_PID 0x52DE
#define CONFIG_BT_GATT_DIS_PNP_VER 0x0100
#define CONFIG_BT_CONN 1
#define CONFIG_BT_MAX_CONN 1
#define CONFIG_BT_PHY_UPDATE 1
#define CONFIG_BT_RPA 1
#define CONFIG_BT_ASSERT 1
#define CONFIG_BT_ASSERT_VERBOSE 1
#define CONFIG_BT_DEBUG_NONE 1
#define CONFIG_BT_HCI_HOST 1
#define CONFIG_BT_HCI_CMD_COUNT 2
#define CONFIG_BT_RX_BUF_COUNT 10
#define CONFIG_BT_RX_BUF_LEN 76
#define CONFIG_BT_DISCARDABLE_BUF_COUNT 3
#define CONFIG_BT_HCI_TX_STACK_SIZE 1536
#define CONFIG_BT_HCI_ECC_STACK_SIZE 1100
#define CONFIG_BT_HCI_TX_PRIO 7
#define CONFIG_BT_HCI_RESERVE 0
#define CONFIG_BT_RX_STACK_SIZE 2048
#define CONFIG_BT_RX_PRIO 8
#define CONFIG_BT_SETTINGS 1
#define CONFIG_BT_SETTINGS_CCC_STORE_ON_WRITE 1
#define CONFIG_BT_SETTINGS_USE_PRINTK 1
#define CONFIG_BT_WHITELIST 1
#define CONFIG_BT_CONN_TX_MAX 3
#define CONFIG_BT_SMP 1
#define CONFIG_BT_SIGNING 1
#define CONFIG_BT_SMP_ALLOW_UNAUTH_OVERWRITE 1
#define CONFIG_BT_BONDABLE 1
#define CONFIG_BT_SMP_ENFORCE_MITM 1
#define CONFIG_BT_L2CAP_TX_BUF_COUNT 3
#define CONFIG_BT_L2CAP_TX_FRAG_COUNT 2
#define CONFIG_BT_L2CAP_TX_MTU 65
#define CONFIG_BT_ATT_ENFORCE_FLOW 1
#define CONFIG_BT_ATT_PREPARE_COUNT 0
#define CONFIG_BT_ATT_TX_MAX 2
#define CONFIG_BT_GATT_SERVICE_CHANGED 1
#define CONFIG_BT_GATT_DYNAMIC_DB 1
#define CONFIG_BT_GATT_CACHING 1
#define CONFIG_BT_GATT_READ_MULTIPLE 1
#define CONFIG_BT_GAP_PERIPHERAL_PREF_PARAMS 1
#define CONFIG_BT_PERIPHERAL_PREF_MIN_INT 6
#define CONFIG_BT_PERIPHERAL_PREF_MAX_INT 6
#define CONFIG_BT_PERIPHERAL_PREF_SLAVE_LATENCY 99
#define CONFIG_BT_PERIPHERAL_PREF_TIMEOUT 400
#define CONFIG_BT_MAX_PAIRED 5
#define CONFIG_BT_CREATE_CONN_TIMEOUT 3
#define CONFIG_BT_CONN_PARAM_UPDATE_TIMEOUT 1000
#define CONFIG_BT_DEVICE_NAME "Mouse nRF52 Desktop"
#define CONFIG_BT_DEVICE_APPEARANCE 962
#define CONFIG_BT_ID_MAX 6
#define CONFIG_BT_ECC 1
#define CONFIG_BT_TINYCRYPT_ECC 1
#define CONFIG_BT_CTLR_LE_ENC_SUPPORT 1
#define CONFIG_BT_CTLR_EXT_REJ_IND_SUPPORT 1
#define CONFIG_BT_CTLR_SLAVE_FEAT_REQ_SUPPORT 1
#define CONFIG_BT_CTLR_DATA_LEN_UPDATE_SUPPORT 1
#define CONFIG_BT_CTLR_PRIVACY_SUPPORT 1
#define CONFIG_BT_CTLR_EXT_SCAN_FP_SUPPORT 1
#define CONFIG_BT_CTLR_PHY_UPDATE_SUPPORT 1
#define CONFIG_BT_CTLR_ADV_EXT_SUPPORT 1
#define CONFIG_BT_CTLR_CHAN_SEL_2_SUPPORT 1
#define CONFIG_BT_CTLR 1
#define CONFIG_BT_CTLR_CRYPTO 1
#define CONFIG_BT_CTLR_RX_PRIO_STACK_SIZE 448
#define CONFIG_BT_CTLR_RX_PRIO 6
#define CONFIG_BT_CTLR_RX_BUFFERS 1
#define CONFIG_BT_CTLR_TX_BUFFERS 3
#define CONFIG_BT_CTLR_TX_BUFFER_SIZE 27
#define CONFIG_BT_CTLR_TX_PWR_PLUS_8 1
#define CONFIG_BT_CTLR_COMPANY_ID 0x05F1
#define CONFIG_BT_CTLR_SUBVERSION_NUMBER 0xFFFF
#define CONFIG_BT_CTLR_LLCP_CONN 1
#define CONFIG_BT_CTLR_LE_ENC 1
#define CONFIG_BT_CTLR_EXT_REJ_IND 1
#define CONFIG_BT_CTLR_SLAVE_FEAT_REQ 1
#define CONFIG_BT_CTLR_LE_PING 1
#define CONFIG_BT_CTLR_PHY 1
#define CONFIG_BT_CTLR_MIN_USED_CHAN 1
#define CONFIG_BT_CTLR_CHAN_SEL_2 1
#define CONFIG_BT_CTLR_ADVANCED_FEATURES 1
#define CONFIG_BT_CTLR_FILTER 1
#define CONFIG_BT_CTLR_PHY_2M 1
#define CONFIG_BT_CTLR_RADIO_ENABLE_FAST 1
#define CONFIG_BT_MAYFLY_YIELD_AFTER_CALL 1
#define CONFIG_BT_COMPANY_ID 0x05F1
#define CONFIG_PRINTK 1
#define CONFIG_HAS_SEGGER_RTT 1
#define CONFIG_FCB 1
#define CONFIG_NET_BUF 1
#define CONFIG_NET_BUF_USER_DATA_SIZE 8
#define CONFIG_USB_PID_CDC_ACM_SAMPLE 0x0001
#define CONFIG_USB_PID_CDC_ACM_COMPOSITE_SAMPLE 0x0002
#define CONFIG_USB_PID_HID_CDC_SAMPLE 0x0003
#define CONFIG_USB_PID_CONSOLE_SAMPLE 0x0004
#define CONFIG_USB_PID_DFU_SAMPLE 0x0005
#define CONFIG_USB_PID_HID_SAMPLE 0x0006
#define CONFIG_USB_PID_HID_MOUSE_SAMPLE 0x0007
#define CONFIG_USB_PID_MASS_SAMPLE 0x0008
#define CONFIG_USB_PID_TESTUSB_SAMPLE 0x0009
#define CONFIG_USB_PID_WEBUSB_SAMPLE 0x000A
#define CONFIG_USB_PID_BLE_HCI_SAMPLE 0x000B
#define CONFIG_USB_DEVICE_VID 0x1915
#define CONFIG_USB_DEVICE_PID 0x52DE
#define CONFIG_USB_DEVICE_MANUFACTURER "Nordic Semiconductor ASA"
#define CONFIG_USB_DEVICE_PRODUCT "Mouse nRF52 Desktop"
#define CONFIG_USB_DEVICE_SN "0.01"
#define CONFIG_USB_REQUEST_BUFFER_SIZE 128
#define CONFIG_USB_NUMOF_EP_WRITE_RETRIES 3
#define CONFIG_USB_DEVICE_DISABLE_ZLP_EPIN_HANDLING 1
#define CONFIG_USB_DEVICE_HID 1
#define CONFIG_USB_HID_DEVICE_NAME "HID"
#define CONFIG_USB_HID_DEVICE_COUNT 1
#define CONFIG_HID_INTERRUPT_EP_MPS 16
#define CONFIG_USB_HID_POLL_INTERVAL_MS 1
#define CONFIG_USB_HID_REPORTS 1
#define CONFIG_USB_HID_BOOT_PROTOCOL 1
#define CONFIG_USB_HID_PROTOCOL_CODE 2
#define CONFIG_ENTROPY_DEVICE_RANDOM_GENERATOR 1
#define CONFIG_CSPRING_ENABLED 1
#define CONFIG_HARDWARE_DEVICE_CS_GENERATOR 1
#define CONFIG_FLASH_MAP 1
#define CONFIG_SETTINGS 1
#define CONFIG_PM_PARTITION_SIZE_SETTINGS_STORAGE 0x6000
#define CONFIG_SETTINGS_DYNAMIC_HANDLERS 1
#define CONFIG_SETTINGS_FCB 1
#define CONFIG_SETTINGS_FCB_NUM_AREAS 8
#define CONFIG_SETTINGS_FCB_MAGIC 0xc0ffeeee
#define CONFIG_TEST_EXTRA_STACKSIZE 0
#define CONFIG_TEST_ARM_CORTEX_M 1
#define CONFIG_HAS_CMSIS_CORE 1
#define CONFIG_HAS_CMSIS_CORE_M 1
#define CONFIG_TINYCRYPT_ECC_DH 1
#define CONFIG_TINYCRYPT_AES 1
#define CONFIG_TINYCRYPT_AES_CMAC 1
#define CONFIG_LINKER_ORPHAN_SECTION_PLACE 1
#define CONFIG_HAS_FLASH_LOAD_OFFSET 1
#define CONFIG_FLASH_LOAD_OFFSET 0x0
#define CONFIG_FLASH_LOAD_SIZE 0x0
#define CONFIG_KERNEL_ENTRY "__start"
#define CONFIG_LINKER_SORT_BY_ALIGNMENT 1
#define CONFIG_SPEED_OPTIMIZATIONS 1
#define CONFIG_COMPILER_OPT ""
#define CONFIG_KERNEL_BIN_NAME "zephyr"
#define CONFIG_OUTPUT_STAT 1
#define CONFIG_OUTPUT_DISASSEMBLY 1
#define CONFIG_OUTPUT_PRINT_MEMORY_USAGE 1
#define CONFIG_BUILD_OUTPUT_BIN 1
#define CONFIG_REBOOT 1
#define CONFIG_COMPAT_INCLUDES 1
