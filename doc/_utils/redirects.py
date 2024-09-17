"""
Copyright (c) 2022 Nordic Semiconductor
SPDX-License-Identifier: Apache-2.0

This module contains per-docset variables with a list of tuples
(old_url, newest_url) for pages that need a redirect. This list allows redirecting
old URLs (caused by, for example, reorganizing doc directories).

Notes:
    - Keep URLs relative to document root (NO leading slash and
      without the html extension).
    - Keep URLs in the order of pages from the doc hierarchy. Move entry order if hierarchy changes.
    - Comments mention the page name; edit the comment when the page name changes.
    - Each comment is valid for the line where it is added AND all lines without comment after it.
    - If a page was removed, mention in the comment when it was removed, if possible.
      A redirect should still point to another page.

Examples:
    ("old/README", "absolutely/newer/index"), # Name of the page
    ("new/index", "absolutely/newer/index"),
    ("even/newer/index", "absolutely/newer/index"),
"""

NRF = [
    ("introduction", "index"), # Introduction
    ("ug_nrf9160_gs", "gsg_guides/nrf9160_gs"), # Getting started with nRF9160 DK
    ("working_with_nrf/nrf91/nrf9160_gs", "gsg_guides/nrf9160_gs"),
    ("device_guides/working_with_nrf/nrf91/nrf9160_gs", "gsg_guides/nrf9160_gs"),
    ("ug_thingy91_gsg", "gsg_guides/thingy91_gsg"), # Getting started with Thingy:91
    ("working_with_nrf/nrf91/thingy91_gsg", "gsg_guides/thingy91_gsg"),
    ("device_guides/working_with_nrf/nrf91/thingy91_gsg", "gsg_guides/thingy91_gsg"),
    ("ug_nrf7002_gs", "gsg_guides/nrf7002_gs"), # Getting started with nRF7002 DK
    ("working_with_nrf/nrf70/gs", "gsg_guides/nrf7002_gs"),
    ("device_guides/working_with_nrf/nrf70/gs", "gsg_guides/nrf7002_gs"),
    ("device_guides/working_with_nrf/nrf53/nrf5340_gs", "gsg_guides/nrf5340_gs"), # Getting started with nRF5340 DK
    ("ug_thingy53_gs", "gsg_guides/thingy53_gs"), # Getting started with Thingy:53
    ("working_with_nrf/nrf53/thingy53_gs", "gsg_guides/thingy53_gs"),
    ("device_guides/working_with_nrf/nrf53/thingy53_gs", "gsg_guides/thingy53_gs"),
    ("ug_nrf52_gs", "gsg_guides/nrf52_gs"), # Getting started with nRF52 Series
    ("working_with_nrf/nrf52/gs", "gsg_guides/nrf52_gs"),
    ("device_guides/working_with_nrf/nrf52/gs", "gsg_guides/nrf52_gs"),
    ("gs_assistant", "installation/install_ncs"), # Installing the nRF Connect SDK
    ("getting_started", "installation"),
    ("getting_started/assistant", "installation/install_ncs"),
    ("installation/assistant", "installation/install_ncs"),
    ("gs_installing", "installation/install_ncs"),
    ("getting_started/installing", "installation/install_ncs"),
    ("installation/installing", "installation/install_ncs"),
    ("gs_updating", "installation/updating"), # Updating repositories and tools
    ("getting_started/updating", "installation/updating"),
    ("gs_recommended_versions", "installation/recommended_versions"), # Requirements reference
    ("getting_started/recommended_versions", "installation/recommended_versions"),
    ("create_application", "app_dev/create_application"), # Creating an application
    ("app_boards", "app_dev/board_support/index"), # Board support
    ("config_and_build/board_support", "app_dev/board_support/index"),
    ("config_and_build/board_support/index", "app_dev/board_support/index"),
    ("config_and_build/board_support/board_names", "app_dev/board_support/board_names"), # Board names
    ("config_and_build/board_support/processing_environments", "app_dev/board_support/processing_environments"), # Processing environments
    ("config_and_build/board_support/defining_custom_board", "app_dev/board_support/defining_custom_board"), # Defining custom board
    ("gs_modifying", "app_dev/config_and_build/index"), # Configuring and building (landing)
    ("getting_started/modifying", "app_dev/config_and_build/index"),
    ("config_and_build/modifying", "app_dev/config_and_build/index"),
    ("config_and_build/configuring_app/index", "app_dev/config_and_build/index"),
    ("config_and_build", "app_dev/config_and_build/index"),
    ("config_and_build/index", "app_dev/config_and_build/index"),
    ("app_build_system", "app_dev/config_and_build/config_and_build_system"), # Build and configuration system
    ("app_dev/build_and_config_system/index", "app_dev/config_and_build/config_and_build_system"),
    ("config_and_build/config_and_build_system", "app_dev/config_and_build/config_and_build_system"),
    ("config_and_build/configuring_app/cmake/index", "app_dev/config_and_build/cmake/index"), # Adding files and configuring CMake
    ("config_and_build/configuring_app/hardware/index", "app_dev/config_and_build/hardware/index"), # Configuring devicetree (landing)
    ("ug_pinctrl", "app_dev/config_and_build/hardware/pin_control"), # Pin control
    ("app_dev/pin_control/index", "app_dev/config_and_build/hardware/pin_control"),
    ("device_guides/pin_control", "app_dev/config_and_build/hardware/pin_control"),
    ("config_and_build/pin_control", "app_dev/config_and_build/hardware/pin_control"),
    ("config_and_build/configuring_app/hardware/pin_control", "app_dev/config_and_build/hardware/pin_control"),
    ("config_and_build/use_gpio_pin_directly", "app_dev/config_and_build/hardware/use_gpio_pin_directly"), # Driving a GPIO pin directly
    ("config_and_build/configuring_app/hardware/use_gpio_pin_directly", "app_dev/config_and_build/hardware/use_gpio_pin_directly"),
    ("config_and_build/configuring_app/kconfig/index", "app_dev/config_and_build/kconfig/index"), # Configuring Kconfig (landing)
    ("config_and_build/configuring_app/sysbuild/index", "app_dev/config_and_build/sysbuild/index"), # Configuring sysbuild (landing)
    ("config_and_build/configuring_app/sysbuild/sysbuild_configuring_west", "app_dev/config_and_build/sysbuild/sysbuild_configuring_west"), # Configuring sysbuild usage in west
    ("config_and_build/configuring_app/sysbuild/sysbuild_images", "app_dev/config_and_build/sysbuild/sysbuild_images"), # Sysbuild images
    ("config_and_build/configuring_app/sysbuild/zephyr_samples_sysbuild", "app_dev/config_and_build/sysbuild/zephyr_samples_sysbuild"), # Using Zephyr samples with sysbuild
    ("config_and_build/configuring_app/sysbuild/sysbuild_forced_options", "app_dev/config_and_build/sysbuild/sysbuild_forced_options"), # Sysbuild forced options
    ("config_and_build/configuring_app/building", "app_dev/config_and_build/building"), # Building an application
    ("config_and_build/configuring_app/advanced_building", "app_dev/config_and_build/building"), # Advanced building procedures (removed after 2.7.0)
    ("gs_programming", "app_dev/programming"), # Programming an application
    ("getting_started/programming", "app_dev/programming"),
    ("config_and_build/programming", "app_dev/programming"),
    ("config_and_build/companion_components", "app_dev/companion_components"), # Using companion components
    ("config_and_build/output_build_files", "app_dev/config_and_build/output_build_files"), # Output build files (image files)
    ("config_and_build/configuring_app/output_build_files", "app_dev/config_and_build/output_build_files"),
    ("ug_multi_image", "app_dev/config_and_build/multi_image"), # Multi-image build using child and parent images
    ("app_dev/multi_image/index", "app_dev/config_and_build/multi_image"),
    ("config_and_build/multi_image", "app_dev/config_and_build/multi_image"),
    ("ug_fw_update", "app_dev/bootloaders_dfu/index"), # Firmware updates (removed after 2.5.0)
    ("app_dev/bootloaders_and_dfu/fw_update", "app_dev/bootloaders_dfu/index"),
    ("config_and_build/bootloaders_and_dfu/fw_update", "app_dev/bootloaders_dfu/index"),
    ("config_and_build/dfu/index", "app_dev/bootloaders_dfu/index"), # Device Firmware Updates (removed after 2.7.0) > Bootloaders and DFU (landing)
    ("app_dev/dfu/index", "app_dev/bootloaders_dfu/index"),
    ("app_bootloaders", "app_dev/bootloaders_dfu/index"), # Bootloaders (removed after 2.7.0) > Bootloaders and DFU (landing)
    ("app_dev/bootloaders_and_dfu/index", "app_dev/bootloaders_dfu/index"),
    ("config_and_build/bootloaders_and_dfu/index", "app_dev/bootloaders_dfu/index"),
    ("config_and_build/bootloaders_dfu/index", "app_dev/bootloaders_dfu/index"),
    ("config_and_build/bootloaders_dfu/mcuboot_nsib/bootloader_mcuboot_nsib", "app_dev/bootloaders_dfu/mcuboot_nsib/bootloader_mcuboot_nsib"), # MCUboot and NSIB (landing)
    ("ug_bootloader_testing", "app_dev/bootloaders_dfu/mcuboot_nsib/bootloader_mcuboot_nsib"), # Testing the bootloader chain (removed after 2.7.0)
    ("app_dev/bootloaders_and_dfu/bootloader_testing", "app_dev/bootloaders_dfu/mcuboot_nsib/bootloader_mcuboot_nsib"),
    ("config_and_build/bootloaders_and_dfu/bootloader_testing", "app_dev/bootloaders_dfu/mcuboot_nsib/bootloader_mcuboot_nsib"),
    ("config_and_build/bootloaders/bootloader_testing", "app_dev/bootloaders_dfu/mcuboot_nsib/bootloader_mcuboot_nsib"),
    ("config_and_build/bootloaders/bootloader_quick_start", "app_dev/bootloaders_dfu/mcuboot_nsib/bootloader_quick_start"), # Quick start guide
    ("config_and_build/bootloaders_dfu/mcuboot_nsib/bootloader_quick_start", "app_dev/bootloaders_dfu/mcuboot_nsib/bootloader_quick_start"),
    ("config_and_build/bootloaders/bootloader_adding_sysbuild", "app_dev/bootloaders_dfu/mcuboot_nsib/bootloader_adding_sysbuild"), # Enabling a bootloader chain using sysbuild
    ("config_and_build/bootloaders_dfu/mcuboot_nsib/bootloader_adding_sysbuild", "app_dev/bootloaders_dfu/mcuboot_nsib/bootloader_adding_sysbuild"),
    ("ug_bootloader_adding", "app_dev/bootloaders_dfu/mcuboot_nsib/bootloader_adding"), # "Enabling a bootloader chain using child and parent images (deprecated)"
    ("app_dev/bootloaders_and_dfu/bootloader_adding", "app_dev/bootloaders_dfu/mcuboot_nsib/bootloader_adding"),
    ("config_and_build/bootloaders_and_dfu/bootloader_adding", "app_dev/bootloaders_dfu/mcuboot_nsib/bootloader_adding"),
    ("config_and_build/bootloaders/bootloader_adding", "app_dev/bootloaders_dfu/mcuboot_nsib/bootloader_adding"),
    ("config_and_build/bootloaders_dfu/mcuboot_nsib/bootloader_adding", "app_dev/bootloaders_dfu/mcuboot_nsib/bootloader_adding"),
    ("ug_bootloader", "app_dev/bootloaders_dfu/mcuboot_nsib/bootloader"), # Secure bootloader chain
    ("app_dev/bootloaders_and_dfu/bootloader", "app_dev/bootloaders_dfu/mcuboot_nsib/bootloader"),
    ("config_and_build/bootloaders_and_dfu/bootloader", "app_dev/bootloaders_dfu/mcuboot_nsib/bootloader"),
    ("config_and_build/bootloaders/bootloader", "app_dev/bootloaders_dfu/mcuboot_nsib/bootloader"),
    ("config_and_build/bootloaders_dfu/mcuboot_nsib/bootloader", "app_dev/bootloaders_dfu/mcuboot_nsib/bootloader"),
    ("ug_bootloader_external_flash", "app_dev/bootloaders_dfu/mcuboot_nsib/bootloader_partitioning"), # Partitioning device memory
    ("app_dev/bootloaders_and_dfu/bootloader_external_flash", "app_dev/bootloaders_dfu/mcuboot_nsib/bootloader_partitioning"),
    ("config_and_build/bootloaders_and_dfu/bootloader_external_flash", "app_dev/bootloaders_dfu/mcuboot_nsib/bootloader_partitioning"),
    ("config_and_build/bootloaders/bootloader_external_flash", "app_dev/bootloaders_dfu/mcuboot_nsib/bootloader_partitioning"),
    ("config_and_build/bootloaders_dfu/mcuboot_nsib/bootloader_partitioning", "app_dev/bootloaders_dfu/mcuboot_nsib/bootloader_partitioning"),
    ("config_and_build/dfu/dfu_image_versions", "app_dev/bootloaders_dfu/mcuboot_nsib/bootloader_dfu_image_versions"), # Image versions
    ("config_and_build/bootloaders/bootloader_dfu_image_versions", "app_dev/bootloaders_dfu/mcuboot_nsib/bootloader_dfu_image_versions"),
    ("config_and_build/bootloaders_dfu/mcuboot_nsib/bootloader_dfu_image_versions", "app_dev/bootloaders_dfu/mcuboot_nsib/bootloader_dfu_image_versions"),
    ("ug_bootloader_config", "app_dev/bootloaders_dfu/mcuboot_nsib/bootloader_config"), # Customizing the bootloader
    ("app_dev/bootloaders_and_dfu/bootloader_config", "app_dev/bootloaders_dfu/mcuboot_nsib/bootloader_config"),
    ("config_and_build/bootloaders_and_dfu/bootloader_config", "app_dev/bootloaders_dfu/mcuboot_nsib/bootloader_config"),
    ("config_and_build/bootloaders/bootloader_config", "app_dev/bootloaders_dfu/mcuboot_nsib/bootloader_config"),
    ("config_and_build/bootloaders_dfu/mcuboot_nsib/bootloader_config", "app_dev/bootloaders_dfu/mcuboot_nsib/bootloader_config"),
    ("config_and_build/bootloaders/bootloader_signature_keys", "app_dev/bootloaders_dfu/mcuboot_nsib/bootloader_signature_keys"), # Signature keys
    ("config_and_build/bootloaders_dfu/mcuboot_nsib/bootloader_signature_keys", "app_dev/bootloaders_dfu/mcuboot_nsib/bootloader_signature_keys"),
    ("config_and_build/bootloaders/bootloader_downgrade_protection", "app_dev/bootloaders_dfu/mcuboot_nsib/bootloader_downgrade_protection"), # Downgrade protection
    ("config_and_build/bootloaders_dfu/mcuboot_nsib/bootloader_downgrade_protection", "app_dev/bootloaders_dfu/mcuboot_nsib/bootloader_downgrade_protection"),
    ("device_guides", "app_dev"), # Device configuration guides (landing; removed after 2.7.0)
    ("ug_nrf91", "app_dev/device_guides/nrf91/index"), # Developing with nRF91 Series (post-2.7.0 landing)
    ("nrf91", "app_dev/device_guides/nrf91/index"),
    ("device_guides/nrf91", "app_dev/device_guides/nrf91/index"),
    ("device_guides/nrf91/index", "app_dev/device_guides/nrf91/index"),
    ("ug_nrf9160", "app_dev/device_guides/nrf91/index"), ## Developing with nRF9160 (pre-2.7.0; removed)
    ("working_with_nrf/nrf91/nrf9160", "app_dev/device_guides/nrf91/index"),
    ("device_guides/working_with_nrf/nrf91/nrf9160", "app_dev/device_guides/nrf91/index"),
    ("ug_nrf9161", "app_dev/device_guides/nrf91/index"), ## Developing with nRF9161 (pre-2.7.0; removed)
    ("working_with_nrf/nrf91/nrf9161", "app_dev/device_guides/nrf91/index"),
    ("device_guides/working_with_nrf/nrf91/nrf9161", "app_dev/device_guides/nrf91/index"),
    ("ug_thingy91", "app_dev/device_guides/nrf91/index"), ## Developing with Thingy:91 (pre-2.7.0; removed)
    ("working_with_nrf/nrf91/thingy91", "app_dev/device_guides/nrf91/index"),
    ("device_guides/working_with_nrf/nrf91/thingy91", "app_dev/device_guides/nrf91/index"),
    ("ug_nrf91_features", "app_dev/device_guides/nrf91/nrf91_features"), # Features of nRF91 Series
    ("working_with_nrf/nrf91/nrf91_features", "app_dev/device_guides/nrf91/nrf91_features"),
    ("device_guides/working_with_nrf/nrf91/nrf91_features", "app_dev/device_guides/nrf91/nrf91_features"),
    ("device_guides/nrf91/nrf91_features", "app_dev/device_guides/nrf91/nrf91_features"),
    ("device_guides/nrf91/nrf91_board_controllers", "app_dev/device_guides/nrf91/nrf91_board_controllers"), # Configuring board controller
    ("device_guides/nrf91/nrf91_cloud_certificate", "app_dev/device_guides/nrf91/nrf91_cloud_certificate"), # Updating the nRF Cloud certificate
    ("device_guides/nrf91/thingy91_connecting", "app_dev/device_guides/nrf91/thingy91_connecting"), # Connecting to Thingy:91
    ("device_guides/nrf91/nrf91_dk_updating_fw_programmer", "app_dev/device_guides/nrf91/nrf91_dk_updating_fw_programmer"), # Updating the firmware for nRF91 Series devices (landing)
    ("device_guides/nrf91/nrf91_updating_fw_programmer", "app_dev/device_guides/nrf91/nrf91_updating_fw_programmer"), # Updating the DK firmware using Programmer
    ("device_guides/nrf91/thingy91_updating_fw_programmer", "app_dev/device_guides/nrf91/thingy91_updating_fw_programmer"), # Updating the Thingy:91 firmware using Programmer
    ("device_guides/nrf91/nrf91_building", "app_dev/device_guides/nrf91/nrf91_building"), # Configuring and building with nRF91 Series
    ("device_guides/nrf91/nrf91_programming", "app_dev/device_guides/nrf91/nrf91_programming"), # Programming onto nRF91 Series devices
    ("device_guides/nrf91/nrf91_testing_at_client", "app_dev/device_guides/nrf91/nrf91_testing_at_client"), # Testing the cellular connection on nRF91 Series DK
    ("device_guides/working_with_nrf/nrf91/nrf91_snippet", "app_dev/device_guides/nrf91/nrf91_snippet"), # Snippets for an nRF91 Series device
    ("device_guides/nrf91/nrf91_snippet", "app_dev/device_guides/nrf91/nrf91_snippet"),
    ("device_guides/nrf91/nrf9160_external_flash", "app_dev/device_guides/nrf91/nrf9160_external_flash"), # Configuring external flash memory on the nRF9160 DK
    ("ug_nrf70", "app_dev/device_guides/nrf70/index"), # Developing with nRF70 Series (post-2.7.0 landing)
    ("device_guides/nrf70", "app_dev/device_guides/nrf70/index"),
    ("device_guides/nrf70/index", "app_dev/device_guides/nrf70/index"),
    ("ug_nrf70_developing", "app_dev/device_guides/nrf70/index"), ## Developing with nRF70 Series (pre-2.7.0 landing; removed)
    ("working_with_nrf/nrf70/developing/index", "app_dev/device_guides/nrf70/index"),
    ("device_guides/working_with_nrf/nrf70/developing/index", "app_dev/device_guides/nrf70/index"),
    ("ug_nrf70_features", "app_dev/device_guides/nrf70/features"), # Features of nRF70 Series
    ("working_with_nrf/nrf70/features", "app_dev/device_guides/nrf70/features"),
    ("device_guides/working_with_nrf/nrf70/features", "app_dev/device_guides/nrf70/features"),
    ("device_guides/nrf70/features", "app_dev/device_guides/nrf70/features"),
    ("device_guides/working_with_nrf/nrf70/developing/stack_partitioning", "app_dev/device_guides/nrf70/stack_partitioning"), # Networking stack partitioning
    ("device_guides/nrf70/stack_partitioning", "app_dev/device_guides/nrf70/stack_partitioning"),
    ("ug_nrf7002_constrained", "app_dev/device_guides/nrf70/constrained"), # Host device considerations
    ("working_with_nrf/nrf70/developing/constrained", "app_dev/device_guides/nrf70/constrained"),
    ("device_guides/working_with_nrf/nrf70/developing/constrained", "app_dev/device_guides/nrf70/constrained"),
    ("device_guides/nrf70/constrained", "app_dev/device_guides/nrf70/constrained"),
    ("device_guides/working_with_nrf/nrf70/developing/fw_patches_ext_flash", "app_dev/device_guides/nrf70/fw_patches_ext_flash"), # Firmware patches in the external memory
    ("device_guides/nrf70/fw_patches_ext_flash", "app_dev/device_guides/nrf70/fw_patches_ext_flash"),
    ("device_guides/working_with_nrf/nrf70/developing/nrf70_fw_patch_update", "app_dev/device_guides/nrf70/nrf70_fw_patch_update"), # Firmware patch update
    ("device_guides/nrf70/nrf70_fw_patch_update", "app_dev/device_guides/nrf70/nrf70_fw_patch_update"),
    ("device_guides/working_with_nrf/nrf70/developing/power_profiling", "app_dev/device_guides/nrf70/power_profiling"), # Power profiling of nRF7002 DK
    ("device_guides/nrf70/power_profiling", "app_dev/device_guides/nrf70/power_profiling"),
    ("device_guides/working_with_nrf/nrf70/nrf7002ek_gs", "app_dev/device_guides/nrf70/nrf7002ek_dev_guide"), # Developing with nRF7002 EK
    ("device_guides/nrf70/nrf7002ek_dev_guide", "app_dev/device_guides/nrf70/nrf7002ek_dev_guide"),
    ("device_guides/working_with_nrf/nrf70/nrf7002eb_gs", "app_dev/device_guides/nrf70/nrf7002eb_dev_guide"), # Developing with nRF7002 EB
    ("device_guides/nrf70/nrf7002eb_dev_guide", "app_dev/device_guides/nrf70/nrf7002eb_dev_guide"),
    ("device_guides/nrf54l", "app_dev/device_guides/nrf54l/index"), # Developing with nRF54L Series
    ("app_dev/device_guides/nrf54l", "app_dev/device_guides/nrf54l/index"),
    ("device_guides/working_with_nrf/nrf54l/features", "app_dev/device_guides/nrf54l/features"), # Features of the nRF54L15 PDK
    ("app_dev/device_guides/working_with_nrf/nrf54l/features", "app_dev/device_guides/nrf54l/features"),
    ("device_guides/working_with_nrf/nrf54l/nrf54l15_gs", "gsg_guides"), # Getting started with nRF54L15 PDK (removed after 2.8.0)
    ("app_dev/device_guides/working_with_nrf/nrf54l/nrf54l15_gs", "gsg_guides"),
    ("device_guides/working_with_nrf/nrf54l/testing_dfu", "app_dev/device_guides/nrf54l/testing_dfu"), # Testing the DFU solution
    ("app_dev/device_guides/working_with_nrf/nrf54l/testing_dfu", "app_dev/device_guides/nrf54l/testing_dfu"),
    ("device_guides/working_with_nrf/nrf54l/peripheral_sensor_node_shield", "app_dev/device_guides/nrf54l/peripheral_sensor_node_shield"), # Developing with Peripheral Sensor node shield
    ("app_dev/device_guides/working_with_nrf/nrf54l/peripheral_sensor_node_shield", "app_dev/device_guides/nrf54l/peripheral_sensor_node_shield"),
    ("device_guides/nrf54h", "app_dev/device_guides/nrf54h"), # Developing with nRF54H Series
    ("device_guides/working_with_nrf/nrf54h/ug_nrf54h20_gs", "app_dev/device_guides/working_with_nrf/nrf54h/ug_nrf54h20_gs"), # Getting started with the nRF54H20 DK
    ("device_guides/working_with_nrf/nrf54h/ug_nrf54h20_app_samples", "app_dev/device_guides/working_with_nrf/nrf54h/ug_nrf54h20_app_samples"), # nRF54H20 applications and samples (orphaned as of 2.7.0)
    ("device_guides/working_with_nrf/nrf54h/ug_nrf54h20_architecture", "app_dev/device_guides/working_with_nrf/nrf54h/ug_nrf54h20_architecture"), # Architecture of nRF54H20 (landing)
    ("device_guides/working_with_nrf/nrf54h/ug_nrf54h20_architecture_cpu", "app_dev/device_guides/working_with_nrf/nrf54h/ug_nrf54h20_architecture_cpu"), # nRF54H20 Domains
    ("device_guides/working_with_nrf/nrf54h/ug_nrf54h20_architecture_memory", "app_dev/device_guides/working_with_nrf/nrf54h/ug_nrf54h20_architecture_memory"), # nRF54H20 Memory Layout
    ("device_guides/working_with_nrf/nrf54h/ug_nrf54h20_architecture_ipc", "app_dev/device_guides/working_with_nrf/nrf54h/ug_nrf54h20_architecture_ipc"), # Interprocessor Communication in nRF54H20
    ("device_guides/working_with_nrf/nrf54h/ug_nrf54h20_architecture_boot", "app_dev/device_guides/working_with_nrf/nrf54h/ug_nrf54h20_architecture_boot"), # nRF54H20 Boot Sequence
    ("device_guides/working_with_nrf/nrf54h/ug_nrf54h20_architecture_lifecycle", "app_dev/device_guides/working_with_nrf/nrf54h/ug_nrf54h20_architecture_lifecycle"), # nRF54H20 lifecycle states
    ("device_guides/working_with_nrf/nrf54h/ug_nrf54h20_configuration", "app_dev/device_guides/working_with_nrf/nrf54h/ug_nrf54h20_configuration"), # Configuring the nRF54H20 DK
    ("device_guides/working_with_nrf/nrf54h/ug_nrf54h20_logging", "app_dev/device_guides/working_with_nrf/nrf54h/ug_nrf54h20_logging"), # nRF54H20 logging (orphaned as of 2.7.0)
    ("device_guides/working_with_nrf/nrf54h/ug_nrf54h20_nrf7002ek", "app_dev/device_guides/working_with_nrf/nrf54h/ug_nrf54h20_nrf7002ek"), # Working with the nRF54H20 DK and the nRF7002 EK (orphaned as of 2.7.0)
    ("device_guides/working_with_nrf/nrf54h/ug_nrf54h20_suit_dfu", "app_dev/device_guides/working_with_nrf/nrf54h/ug_nrf54h20_suit_dfu"), # Device Firmware Update using SUIT (landing)
    ("device_guides/working_with_nrf/nrf54h/ug_nrf54h20_suit_intro", "app_dev/device_guides/working_with_nrf/nrf54h/ug_nrf54h20_suit_intro"), # Introduction to SUIT
    ("device_guides/working_with_nrf/nrf54h/ug_nrf54h20_suit_manifest_overview", "app_dev/device_guides/working_with_nrf/nrf54h/ug_nrf54h20_suit_manifest_overview"), # SUIT manifest overview
    ("device_guides/working_with_nrf/nrf54h/ug_nrf54h20_suit_customize_qsg", "app_dev/device_guides/working_with_nrf/nrf54h/ug_nrf54h20_suit_customize_qsg"), # Customize SUIT DFU quick start guide
    ("device_guides/working_with_nrf/nrf54h/ug_nrf54h20_suit_customize_dfu", "app_dev/device_guides/working_with_nrf/nrf54h/ug_nrf54h20_suit_customize_dfu"), # How to customize the SUIT DFU process
    ("device_guides/working_with_nrf/nrf54h/ug_nrf54h20_suit_fetch", "app_dev/device_guides/working_with_nrf/nrf54h/ug_nrf54h20_suit_fetch"), # How to fetch payloads
    ("device_guides/working_with_nrf/nrf54h/ug_nrf54h20_suit_external_memory", "app_dev/device_guides/working_with_nrf/nrf54h/ug_nrf54h20_suit_external_memory"), # Firmware upgrade with external memory
    ("device_guides/working_with_nrf/nrf54h/ug_nrf54h20_suit_components", "app_dev/device_guides/working_with_nrf/nrf54h/ug_nrf54h20_suit_components"), # SUIT components
    ("device_guides/working_with_nrf/nrf54h/ug_nrf54h20_suit_hierarchical_manifests", "app_dev/device_guides/working_with_nrf/nrf54h/ug_nrf54h20_suit_hierarchical_manifests"), # Hierarchical manifests
    ("device_guides/working_with_nrf/nrf54h/ug_nrf54h20_suit_compare_other_dfu", "app_dev/bootloaders_dfu/index"), # DFU and bootloader comparison (removed after 2.7.0) > Bootloaders and DFU (landing)
    ("app_dev/device_guides/working_with_nrf/nrf54h/ug_nrf54h20_suit_compare_other_dfu", "app_dev/bootloaders_dfu/index"),
    ("device_guides/working_with_nrf/nrf54h/ug_nrf54h20_debugging", "app_dev/device_guides/working_with_nrf/nrf54h/ug_nrf54h20_debugging"), # nRF54H20 debugging
    ("device_guides/working_with_nrf/nrf54h/ug_nrf54h20_custom_pcb", "app_dev/device_guides/working_with_nrf/nrf54h/ug_nrf54h20_custom_pcb"), # Configuring your application for a custom PCB
    ("ug_nrf53", "app_dev/device_guides/nrf53/index"), # Developing with nRF53 Series (landing)
    ("nrf53", "app_dev/device_guides/nrf53/index"),
    ("device_guides/nrf53", "app_dev/device_guides/nrf53/index"),
    ("device_guides/nrf53/index", "app_dev/device_guides/nrf53/index"),
    ("ug_nrf5340", "app_dev/device_guides/nrf53/index"),
    ("working_with_nrf/nrf53/nrf5340", "app_dev/device_guides/nrf53/index"),
    ("device_guides/working_with_nrf/nrf53/nrf5340", "app_dev/device_guides/nrf53/index"),
    ("device_guides/nrf53/index", "app_dev/device_guides/nrf53/index"),
    ("ug_thingy53", "app_dev/device_guides/nrf53/index"), # Developing with Thingy:53 (removed for 2.7.0)
    ("working_with_nrf/nrf53/thingy53", "app_dev/device_guides/nrf53/index"),
    ("device_guides/working_with_nrf/nrf53/thingy53", "app_dev/device_guides/nrf53/index"),
    ("device_guides/nrf53/features_nrf53", "app_dev/device_guides/nrf53/features_nrf53"), # Features of nRF53 Series
    ("device_guides/nrf53/building_nrf53", "app_dev/device_guides/nrf53/building_nrf53"), # Building and programming with nRF53 Series
    ("device_guides/nrf53/fota_update_nrf5340", "app_dev/device_guides/nrf53/fota_update_nrf5340"), # FOTA updates with nRF5340 DK
    ("device_guides/nrf53/multi_image_nrf5340", "app_dev/device_guides/nrf53/multi_image_nrf5340"), # Multi-image builds on the nRF5340 DK using child and parent images
    ("device_guides/nrf53/simultaneous_multi_image_dfu_nrf5340", "app_dev/device_guides/nrf53/simultaneous_multi_image_dfu_nrf5340"), # Simultaneous multi-image DFU with nRF5340 DK
    ("device_guides/nrf53/serial_recovery", "app_dev/device_guides/nrf53/serial_recovery"), # MCUboot’s serial recovery of the networking core image
    ("device_guides/nrf53/logging_nrf5340", "app_dev/device_guides/nrf53/logging_nrf5340"), # Getting logging output with nRF5340 DK
    ("device_guides/nrf53/thingy53_application_guide", "app_dev/device_guides/nrf53/thingy53_application_guide"), # Application guide for Thingy:53
    ("device_guides/working_with_nrf/nrf53/qspi_xip_guide", "app_dev/device_guides/nrf53/qspi_xip_guide_nrf5340"), # External execute in place (XIP) configuration on the nRF5340 SoC
    ("device_guides/nrf53/qspi_xip_guide_nrf5340", "app_dev/device_guides/nrf53/qspi_xip_guide_nrf5340"),
    ("ug_nrf52", "app_dev/device_guides/nrf52/index"), # Developing with nRF52 Series (landing)
    ("nrf52", "app_dev/device_guides/nrf52/index"),
    ("device_guides/nrf52", "app_dev/device_guides/nrf52/index"),
    ("device_guides/nrf52/index", "app_dev/device_guides/nrf52/index"),
    ("ug_nrf52_developing", "app_dev/device_guides/nrf52/index"),
    ("working_with_nrf/nrf52/developing", "app_dev/device_guides/nrf52/index"),
    ("device_guides/working_with_nrf/nrf52/developing", "app_dev/device_guides/nrf52/index"),
    ("ug_nrf52_features", "app_dev/device_guides/nrf52/features"), # Features of nRF52 Series
    ("working_with_nrf/nrf52/features", "app_dev/device_guides/nrf52/features"),
    ("device_guides/working_with_nrf/nrf52/features", "app_dev/device_guides/nrf52/features"),
    ("device_guides/nrf52/features", "app_dev/device_guides/nrf52/features"),
    ("device_guides/nrf52/building", "app_dev/device_guides/nrf52/building"), # Building and programming on nRF52 Series devices
    ("device_guides/nrf52/fota_update", "app_dev/device_guides/nrf52/fota_update"), # FOTA updates on nRF52 Series devices
    ("device_guides/pmic", "app_dev/device_guides/pmic/index"), # Developing with PMICs (landing)
    ("device_guides/pmic/index", "app_dev/device_guides/pmic/index"),
    ("device_guides/working_with_pmic/npm1300/developing", "app_dev/device_guides/pmic/npm1300"), # Developing with the nPM1300 PMIC
    ("device_guides/working_with_pmic/npm1300/features", "app_dev/device_guides/pmic/npm1300"),
    ("device_guides/working_with_pmic/npm1300/gs", "app_dev/device_guides/pmic/npm1300"),
    ("device_guides/pmic/npm1300", "app_dev/device_guides/pmic/npm1300"),
    ("ug_radio_fem", "app_dev/device_guides/fem/index"), # Developing with Front-End Modules (landing)
    ("device_guides/fem/fem_software_support", "app_dev/device_guides/fem/fem_software_support"), # Enabling FEM support (landing)
    ("app_dev/working_with_fem/index", "app_dev/device_guides/fem/index"),
    ("device_guides/working_with_fem", "app_dev/device_guides/fem/index"),
    ("device_guides/fem/index", "app_dev/device_guides/fem/index"),
    ("device_guides/fem/fem_mpsl_fem_only", "app_dev/device_guides/fem/fem_mpsl_fem_only"), # MPSL FEM-only configuration
    ("device_guides/fem/fem_nrf21540_gpio", "app_dev/device_guides/fem/fem_nrf21540_gpio"), # Enabling GPIO mode support for nRF21540
    ("device_guides/fem/fem_nrf21540_gpio_spi", "app_dev/device_guides/fem/fem_nrf21540_gpio_spi"), # Enabling GPIO+SPI mode support for nRF21540
    ("device_guides/fem/fem_nRF21540_optional_properties", "app_dev/device_guides/fem/fem_nRF21540_optional_properties"), # Optional FEM properties for nRF21540 GPIO and GPIO+SPI
    ("device_guides/fem/fem_simple_gpio", "app_dev/device_guides/fem/fem_simple_gpio"), # Enabling support for front-end modules using Simple GPIO interface
    ("device_guides/fem/fem_incomplete_connections", "app_dev/device_guides/fem/fem_incomplete_connections"), # Use case of incomplete physical connections to the FEM module
    ("device_guides/fem/fem_power_models", "app_dev/device_guides/fem/fem_power_models"), # Using FEM power models
    ("device_guides/fem/21540ek_dev_guide", "app_dev/device_guides/fem/21540ek_dev_guide"), # Developing with the nRF21540 EK
    ("ug_radio_coex", "app_dev/device_guides/wifi_coex"), # Coexistence of short-range radio and other radios
    ("app_dev/wifi_coex/index", "app_dev/device_guides/wifi_coex"),
    ("device_guides/wifi_coex", "app_dev/device_guides/wifi_coex"),
    ("gs_testing", "test_and_optimize"), # Testing and optimization (landing)
    ("getting_started/testing", "test_and_optimize"),
    ("test_and_optimize/testing", "test_and_optimize"),
    ("ug_logging", "test_and_optimize/logging"), # Logging in nRF Connect SDK
    ("app_dev/logging/index", "test_and_optimize/logging"),
    ("ug_unity_testing", "test_and_optimize/test_framework/testing_unity_cmock"), # Writing tests with Unity and CMock
    ("app_dev/testing_unity_cmock/index", "test_and_optimize/test_framework/testing_unity_cmock"),
    ("test_and_optimize/testing_unity_cmock", "test_and_optimize/test_framework/testing_unity_cmock"),
    ("app_opt", "test_and_optimize/optimizing/index"), # Optimizing application (landing)
    ("app_dev/optimizing/index", "test_and_optimize/optimizing/index"),
    ("app_memory", "test_and_optimize/optimizing/memory"), # Memory footpring optimization
    ("app_dev/optimizing/memory", "test_and_optimize/optimizing/memory"),
    ("app_power_opt", "test_and_optimize/optimizing/power"), # Power optimization
    ("app_dev/optimizing/power", "test_and_optimize/optimizing/power"),
    ("security_chapter", "security/security"), # Security (landing)
    ("security", "security/security"),
    ("ug_tfm", "security/tfm"), # Running applications with Trusted Firmware-M
    ("app_dev/tfm/index", "security/tfm"),
    ("app_dev/ap_protect/index", "security/ap_protect"), # Enabling access port protection mechanism
    ("ug_ble_controller", "protocols/bt/index"), # Bluetooth LE Controller
    ("protocols/ble/index", "protocols/bt/index"),
    ("protocols/bt/ble/radio_notification_conn_cb", "libraries/bluetooth_services/radio_notification_conn_cb"),
    ("ug_bt_mesh", "protocols/bt/bt_mesh/index"), # Bluetooth Mesh (landing)
    ("protocols/bt_mesh/index", "protocols/bt/bt_mesh/index"),
    ("ug_bt_mesh_supported_features", "protocols/bt/bt_mesh/overview/index"), # Supported Bluetooth Mesh features (file removed; part of the overview index page now)
    ("protocols/bt_mesh/supported_features", "protocols/bt/bt_mesh/overview/index"),
    ("protocols/bt/bt_mesh/supported_features", "protocols/bt/bt_mesh/overview/index"),
    ("ug_bt_mesh_concepts", "protocols/bt/bt_mesh/overview/topology"), # Bluetooth Mesh concepts
    ("protocols/bt_mesh/concepts", "protocols/bt/bt_mesh/overview/topology"),
    ("protocols/bt/bt_mesh/concepts", "protocols/bt/bt_mesh/overview/topology"),
    ("ug_bt_mesh_architecture", "protocols/bt/bt_mesh/overview/architecture"), # Bluetooth Mesh stack architecture
    ("protocols/bt_mesh/architecture", "protocols/bt/bt_mesh/overview/architecture"),
    ("protocols/bt/bt_mesh/architecture", "protocols/bt/bt_mesh/overview/architecture"),
    ("ug_bt_mesh_configuring", "protocols/bt/bt_mesh/configuring"), # Configuring Bluetooth Mesh in nRF Connect SDK
    ("protocols/bt_mesh/configuring", "protocols/bt/bt_mesh/configuring"),
    ("ug_bt_mesh_model_config_app", "protocols/bt/bt_mesh/model_config_app"), # Configuring mesh models using the nRF Mesh mobile app
    ("protocols/bt_mesh/model_config_app", "protocols/bt/bt_mesh/model_config_app"),
    ("ug_bt_mesh_fota", "protocols/bt/bt_mesh/fota"), # Performing Device Firmware Updates (DFU) in Bluetooth Mesh
    ("protocols/bt_mesh/fota", "protocols/bt/bt_mesh/fota"),
    ("ug_bt_mesh_node_removal", "protocols/bt/bt_mesh/node_removal"), # Removing a node from a mesh network
    ("protocols/bt_mesh/node_removal", "protocols/bt/bt_mesh/node_removal"),
    ("ug_bt_mesh_vendor_model", "protocols/bt/bt_mesh/vendor_model/index"), # Creating a new model (landing)
    ("protocols/bt_mesh/vendor_model/index", "protocols/bt/bt_mesh/vendor_model/index"),
    ("ug_bt_mesh_vendor_model_dev_overview", "protocols/bt/bt_mesh/vendor_model/dev_overview"), # Vendor model development overview
    ("protocols/bt_mesh/vendor_model/dev_overview", "protocols/bt/bt_mesh/vendor_model/dev_overview"),
    ("ug_bt_mesh_vendor_model_chat_sample_walk_through", "protocols/bt/bt_mesh/vendor_model/chat_sample_walk_through"), # Chat sample walk-through
    ("protocols/bt_mesh/vendor_model/chat_sample_walk_through", "protocols/bt/bt_mesh/vendor_model/chat_sample_walk_through"),
    ("ug_bt_mesh_reserved_ids", "protocols/bt/bt_mesh/overview/reserved_ids"), # Reserved vendor model IDs and opcodes
    ("protocols/bt_mesh/reserved_ids", "protocols/bt/bt_mesh/overview/reserved_ids"),
    ("protocols/bt/bt_mesh/reserved_ids", "protocols/bt/bt_mesh/overview/reserved_ids"),
    ("ug_bt_mesh_nlc", "protocols/bt/bt_mesh/overview/nlc"), # Networked Lighting Control profiles
    ("protocols/bt/bt_mesh/nlc", "protocols/bt/bt_mesh/overview/nlc"),
    ("ug_esb", "protocols/esb/index"), # Enhanced ShockBurst (ESB) (landing)
    ("ug_gz", "protocols/gazell/index"), # Gazell (landing)
    ("ug_gzll", "protocols/gazell/gzll"), # Gazell Link Layer
    ("ug_gzp", "protocols/gazell/gzp"), # Gazell Pairing
    ("ug_matter", "protocols/matter/index"), # Matter (landing)
    ("ug_matter_intro_overview", "protocols/matter/overview/index"), # Matter overview (landing)
    ("ug_matter_overview_architecture", "protocols/matter/overview/architecture"), # Matter architecture
    ("ug_matter_overview_data_model", "protocols/matter/overview/data_model"), # Matter Data Model and device types
    ("ug_matter_overview_int_model", "protocols/matter/overview/int_model"), # Matter Interaction Model and interaction types
    ("ug_matter_overview_network_topologies", "protocols/matter/overview/network_topologies"), # Matter network topology and concepts
    ("ug_matter_overview_security", "protocols/matter/overview/security"), # Matter network security
    ("ug_matter_overview_commissioning", "protocols/matter/overview/commissioning"), # Matter network commissioning
    ("ug_matter_overview_multi_fabrics", "protocols/matter/overview/multi_fabrics"), # Matter multiple fabrics feature
    ("ug_matter_overview_dfu", "protocols/matter/overview/dfu"), # Matter OTA
    ("ug_matter_overview_dev_model", "protocols/matter/overview/dev_model"), # Matter development model and compatible ecosystems
    ("ug_matter_overview_architecture_integration", "protocols/matter/overview/integration"), # Matter integration in the nRF Connect SDK
    ("ug_matter_intro_gs", "protocols/matter/getting_started/index"), # Getting started with Matter (landing)
    ("ug_matter_hw_requirements", "protocols/matter/getting_started/hw_requirements"), # Matter hardware and memory requirements
    ("ug_matter_gs_testing", "protocols/matter/getting_started/testing/index"), # Testing Matter in the nRF Connect SDK (landing)
    ("ug_matter_gs_testing_thread_separate_linux_macos", "protocols/matter/getting_started/testing/thread_separate_otbr_linux_macos"), # Matter over Thread: Configuring Border Router and Linux/macOS controller on separate devices
    ("ug_matter_gs_testing_thread_one_otbr", "protocols/matter/getting_started/testing/thread_one_otbr"), # Matter over Thread: Configuring Border Router and controller on one device
    ("ug_matter_gs_testing_wifi_pc", "protocols/matter/getting_started/testing/wifi_pc"), # Matter over Wi-Fi: Configuring CHIP Tool for Linux or macOS
    ("ug_matter_gs_testing_thread_separate_otbr_android", "protocols/matter/getting_started/testing/index"), # Matter over Thread: Configuring Border Router and Android controller on separate devices (removed after 2.2.0)
    ("ug_matter_gs_testing_wifi_mobile", "protocols/matter/getting_started/testing/index"), # Matter over Wi-Fi: Configuring CHIP Tool for Android (removed after 2.2.0)
    ("ug_matter_gs_tools", "protocols/matter/getting_started/tools"), # Matter tools
    ("ug_matter_gs_kconfig", "protocols/matter/getting_started/kconfig"), # Enabling Matter in Kconfig
    ("ug_matter_gs_advanced_kconfigs", "protocols/matter/getting_started/advanced_kconfigs"), # Advanced Matter Kconfig options
    ("ug_matter_gs_adding_clusters", "protocols/matter/getting_started/adding_clusters"), # Adding clusters to Matter application
    ("ug_matter_intro_device", "protocols/matter/end_product/index"), # How to create Matter end product
    ("ug_matter_device_prerequisites", "protocols/matter/end_product/prerequisites"), # Matter device development prerequisites
    ("ug_matter_device_factory_provisioning", "protocols/matter/end_product/factory_provisioning"), # Factory provisioning in Matter
    ("ug_matter_device_attestation", "protocols/matter/end_product/attestation"), # Matter Device Attestation
    ("ug_matter_device_dcl", "protocols/matter/end_product/dcl"), # Matter Distributed Compliance Ledger
    ("ug_matter_device_certification", "protocols/matter/end_product/certification"), # Matter certification
    ("ug_matter_ecosystems_certification", "protocols/matter/end_product/ecosystems_certification"), # Ecosystems certification
    ("ug_matter_device_bootloader", "protocols/matter/end_product/bootloader"), # Bootloader configuration in Matter
    ("ug_multiprotocol_support", "protocols/multiprotocol/index"), # Multiprotocol support (landing page in Protocols)
    ("ug_nfc", "protocols/nfc/index"), # Near Field Communication (NFC)
    ("ug_thread", "protocols/thread/index"), # Thread (landing)
    ("ug_thread_overview", "protocols/thread/overview/index"), # OpenThread overview (landing)
    ("ug_thread_supported_features", "protocols/thread/overview/supported_features"), # Supported Thread features
    ("ug_thread_architectures", "protocols/thread/overview/architectures"), # OpenThread architectures
    ("ug_thread_communication", "protocols/thread/overview/communication"), # OpenThread co-processor communication
    ("ug_thread_ot_integration", "protocols/thread/overview/ot_integration"), # OpenThread integration
    ("ug_thread_ot_memory", "protocols/thread/overview/ot_memory"), # OpenThread memory requirements
    ("ug_thread_commissioning", "protocols/thread/overview/commissioning"), # OpenThread commissioning
    ("ug_thread_configuring", "protocols/thread/configuring"), # Configuring Thread in the nRF Connect SDK
    ("ug_thread_prebuilt_libs", "protocols/thread/prebuilt_libs"), # Pre-build libraries
    ("ug_thread_tools", "protocols/thread/tools"), # Thread tools
    ("ug_thread_certification", "protocols/thread/certification"), # Thread certification
    ("ug_wifi", "protocols/wifi/index"), # Wi-Fi (landing)
    ("ug_nrf70_developing_powersave", "protocols/wifi/station_mode/powersave"), # Operating in power save modes
    ("working_with_nrf/nrf70/developing/powersave", "protocols/wifi/station_mode/powersave"),
    ("device_guides/working_with_nrf/nrf70/developing/powersave", "protocols/wifi/station_mode/powersave"),
    ("protocols/wifi/powersave", "protocols/wifi/station_mode/powersave"),
    ("device_guides/working_with_nrf/nrf70/developing/scan_operation", "protocols/wifi/scan_mode/scan_operation"), # Optimizing scan operation
    ("protocols/wifi/scan_operation", "protocols/wifi/scan_mode/scan_operation"),
    ("device_guides/working_with_nrf/nrf70/developing/sap", "protocols/wifi/sap_mode/sap"), # SoftAP mode
    ("protocols/wifi/sap", "protocols/wifi/sap_mode/sap"),
    ("device_guides/working_with_nrf/nrf70/developing/raw_tx_operation", "protocols/wifi/advanced_modes/raw_tx_operation"), # Raw IEEE 802.11 packet transmission
    ("protocols/wifi/raw_tx_operation", "protocols/wifi/advanced_modes/raw_tx_operation"),
    ("protocols/wifi/sniffer_rx_operation", "protocols/wifi/advanced_modes/sniffer_rx_operation"), # Raw IEEE 802.11 packet reception using Monitor mode
    ("device_guides/working_with_nrf/nrf70/developing/regulatory_support", "protocols/wifi/regulatory_support"), # Operating with regulatory support
    ("device_guides/working_with_nrf/nrf70/developing/debugging", "protocols/wifi/debugging"), # Debugging
    ("ug_zigbee", "protocols/zigbee/index"), # Zigbee (landing)
    ("ug_zigbee_qsg", "protocols/zigbee/qsg"), # Zigbee quick start guide
    ("ug_zigbee_supported_features", "protocols/zigbee/supported_features"), # Supported Zigbee features
    ("ug_zigbee_architectures", "protocols/zigbee/architectures"), # Zigbee architectures
    ("ug_zigbee_commissioning", "protocols/zigbee/commissioning"), # Zigbee commissioning
    ("ug_zigbee_memory", "protocols/zigbee/memory"), # Zigbee memory requirements
    ("ug_zigbee_configuring", "protocols/zigbee/configuring"), # Configuring Zigbee in nRF Connect SDK
    ("ug_zigbee_configuring_libraries", "protocols/zigbee/configuring_libraries"), # Configuring Zigbee libraries in nRF Connect SDK
    ("ug_zigbee_configuring_zboss_traces", "protocols/zigbee/configuring_zboss_traces"), # Configuring ZBOSS traces in nRF Connect SDK
    ("ug_zigbee_adding_clusters", "protocols/zigbee/adding_clusters"), # Adding ZCL clusters to application (removed after 2.5.0)
    ("ug_zigbee_other_ecosystems", "protocols/zigbee/other_ecosystems"), # Configuring Zigbee samples for other ecosystems
    ("ug_zigbee_tools", "protocols/zigbee/tools"), # Zigbee tools
    ("applications/nrf5340_audio/README", "applications/nrf5340_audio/index"), # nRF5340 Audio applications (landing)
    ("samples/samples_bl", "samples/bl"), # Bluetooth samples (landing)
    ("samples/samples_nrf9160", "samples/cellular"), # Cellular samples (landing)
    ("samples/samples_crypto", "samples/crypto"), # Cryptography samples (landing)
    ("samples/samples_edge", "samples/edge"), # Edge Impulse samples (landing)
    ("samples/samples_gazell", "samples/gazell"), # Gazell samples (landing)
    ("samples/samples_matter", "samples/matter"), # Matter samples (landing)
    ("samples/samples_nfc", "samples/nfc"), # NFC samples (landing)
    ("samples/samples_nrf5340", "samples/nrf5340"), # nRF5340 samples (landing)
    ("samples/samples_thread", "samples/thread"), # Thread samples (landing)
    ("samples/samples_tfm", "samples/tfm"), # Trusted Firmware-M (TF-M) samples (landing)
    ("samples/samples_wifi", "samples/wifi"), # Wi-Fi samples (landing)
    ("samples/wifi/sr_coex/README", "samples/wifi/ble_coex/README"), # Wi-Fi: Bluetooth LE coexistence
    ("samples/samples_zigbee", "samples/zigbee"), # Zigbee samples (landing)
    ("samples/samples_other", "samples/other"), # Other samples (landing)
    ("libraries/networking/nrf_cloud_agps", "libraries/networking/nrf_cloud_agnss"), # nRF Cloud A-GNSS
    ("libraries/bootloader/index", "libraries/security/bootloader/index"), # Bootloader libraries (landing)
    ("libraries/bootloader/bl_crypto", "libraries/security/bootloader/bl_crypto"), # Bootloader crypto
    ("libraries/bootloader/bl_storage", "libraries/security/bootloader/bl_storage"), # Bootloader storage
    ("libraries/bootloader/bl_validation", "libraries/security/bootloader/bl_validation"), # Bootloader firmware validation
    ("libraries/others/flash_patch", "libraries/security/bootloader/flash_patch"), # Flash patch
    ("libraries/others/fprotect", "libraries/security/bootloader/fprotect"), # Hardware flash write protection
    ("libraries/others/fw_info", "libraries/security/bootloader/fw_info"), # Firmware information
    ("libraries/nrf_security/index", "libraries/security/nrf_security/index"), # nRF Security (landing page in Security libraries)
    ("libraries/nrf_security/doc/configuration", "libraries/security/nrf_security/doc/configuration"), # Configuration
    ("libraries/nrf_security/doc/drivers", "libraries/security/nrf_security/doc/drivers"), # nRF Security drivers
    ("libraries/nrf_security/doc/driver_config", "libraries/security/nrf_security/doc/driver_config"), # Feature configurations and driver support
    ("libraries/nrf_security/doc/mbed_tls_header", "libraries/security/nrf_security/doc/mbed_tls_header"), # User-provided Mbed TLS configuration header
    ("libraries/nrf_security/doc/backend_config", "libraries/security/nrf_security/doc/backend_config"), # Legacy configurations and supported features
    ("libraries/tfm/index", "libraries/security/tfm/index"), # TF-M libraries (landing)
    ("libraries/tfm/tfm_ioctl_api", "libraries/security/tfm/tfm_ioctl_api"), # TF-M input/output control (IOCTL)
    ("libraries/others/fatal_error", "libraries/security/fatal_error"), # Fatal error handler
    ("libraries/others/hw_unique_key", "libraries/security/hw_unique_key"), # Hardware unique key
    ("libraries/others/identity_key", "libraries/security/identity_key"), # Identity key
    ("ecosystems_integrations", "integrations"), # Integrations (landing)
    ("ug_bt_fast_pair", "external_comp/bt_fast_pair"), # Google Fast Pair integration
    ("ug_edge_impulse", "external_comp/edge_impulse"), # Edge Impulse integration
    ("ug_memfault", "external_comp/memfault"), # Memfault integration
    ("ug_nrf_cloud", "external_comp/nrf_cloud"), # Using nRF Cloud with the nRF Connect SDK
    ("ug_dev_model", "dev_model_and_contributions"), # Development model and contributions (landing)
    ("dev_model", "dev_model_and_contributions"),
    ("releases_and_maturity/dev_model", "dev_model_and_contributions"),
    ("dm_code_base", "dev_model_and_contributions/code_base"), # nRF Connect SDK code base
    ("developing/code_base", "dev_model_and_contributions/code_base"),
    ("releases_and_maturity/developing/code_base", "dev_model_and_contributions/code_base"),
    ("dm_managing_code", "dev_model_and_contributions/managing_code"), # Managing the code base
    ("developing/managing_code", "dev_model_and_contributions/managing_code"),
    ("releases_and_maturity/developing/managing_code", "dev_model_and_contributions/managing_code"),
    ("dm_adding_code", "dev_model_and_contributions/adding_code"), # Adding your own code
    ("developing/adding_code", "dev_model_and_contributions/adding_code"),
    ("releases_and_maturity/developing/adding_code", "dev_model_and_contributions/adding_code"),
    ("dm_ncs_distro", "dev_model_and_contributions/ncs_distro"), # Redistributing the nRF Connect SDK
    ("developing/ncs_distro", "dev_model_and_contributions/ncs_distro"),
    ("releases_and_maturity/developing/ncs_distro", "dev_model_and_contributions/ncs_distro"),
    ("documentation", "dev_model_and_contributions/documentation"), # nRF Connect SDK documentation
    ("doc_structure", "dev_model_and_contributions/documentation/structure"), # Documentation structure
    ("documentation/structure", "dev_model_and_contributions/documentation/structure"),
    ("doc_build_process", "dev_model_and_contributions/documentation/doc_build_process"), # Documentation build process
    ("documentation/build_process", "dev_model_and_contributions/documentation/doc_build_process"),
    ("doc_build", "dev_model_and_contributions/documentation/build"), # Building the nRF Connect SDK documentation
    ("documentation/build", "dev_model_and_contributions/documentation/build"),
    ("doc_styleguide", "dev_model_and_contributions/documentation/styleguide"), # Documentation guidelines
    ("documentation/styleguide", "dev_model_and_contributions/documentation/styleguide"),
    ("doc_templates", "dev_model_and_contributions/documentation/templates"), # Documentation templates (landing)
    ("documentation/templates", "dev_model_and_contributions/documentation/templates"),
    ("release_notes", "releases_and_maturity/release_notes"), # Release notes (landing)
    ("migration/migration_guide_1.x_to_2.x", "releases_and_maturity/migration/migration_guide_1.x_to_2.x"), # Migration notes for nRF Connect SDK v2.0.0
    ("software_maturity", "releases_and_maturity/software_maturity"), # Software maturity
    ("known_issues", "releases_and_maturity/known_issues"), # Known issues
    ("releases_and_maturity/glossary", "glossary"), # Glossary
]
