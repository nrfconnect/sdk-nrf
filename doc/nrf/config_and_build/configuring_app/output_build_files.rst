.. _app_build_output_files:

Output build files (image files)
################################

.. contents::
   :local:
   :depth: 2

The building process produces each time an :term:`image file`.

The image file can refer to an *executable*, a *program*, or an *ELF file*.
As one of the last build steps, the linker processes all object files by locating code, data, and symbols in sections in the final ELF file.
The linker replaces all symbol references to code and data with addresses.
A symbol table that maps addresses to symbol names is created.
This table is used by debuggers.
When an ELF file is converted to another format, such as HEX or binary, the symbol table is lost.

Depending on the application and the SoC, you can use one or several images.

The |NCS| build system places output images in the :file:`<build folder>` folder when using sysbuild.

.. _app_build_output_files_common:

Common output build files
*************************

The following table lists build files that can be generated as output when building firmware for supported :ref:`board targets <app_boards>`.
The table includes files for single-core and multi-core programming scenarios for both |VSC| and command-line building methods.
Which files you are going to use depends on the application configuration and not directly on the type of SoC you are using.

+--------------------------------------+--------------------------------------------------------------------------------------------------------+-------------------------------------------------------------------------------------+
| File                                 | Description                                                                                            | Programming scenario                                                                |
+======================================+========================================================================================================+=====================================================================================+
| :file:`zephyr.hex`                   | Default full image.                                                                                    | * Programming board targets with :ref:`NSPE <app_boards_spe_nspe>` or single-image. |
|                                      | In a project with multiple images, several :file:`zephyr.hex` files are generated, one for each image. | * Testing DFU procedure with nrfjprog (programming directly to device).             |
|                                      | ``zephyr`` is the value of :kconfig:option:`CONFIG_KERNEL_BIN_NAME`.                                   |                                                                                     |
|                                      | The file is located in the :file:`build/<your_application_name>/zephyr` directory.                     |                                                                                     |
+--------------------------------------+--------------------------------------------------------------------------------------------------------+-------------------------------------------------------------------------------------+
| :file:`merged.hex`                   | The result of merging all HEX files for all images for a specific core                                 | * Programming multi-core application.                                               |
|                                      | In multi-core builds, several :file:`merged_<core>.hex` fields                                         | * Testing DFU procedure with nrfjprog (programming directly to device).             |
|                                      | are generated, where *<core>* indicates the CPU core.                                                  |                                                                                     |
+--------------------------------------+--------------------------------------------------------------------------------------------------------+-------------------------------------------------------------------------------------+
| :file:`tfm_s.hex`                    | Secure firmware image created by the TF-M build system in the background of the Zephyr build.          | Programming :ref:`SPE-only <app_boards_spe_nspe>` and multi-core board targets.     |
|                                      | It is used together with the :file:`zephyr.hex` file, which is intended for the Non-Secure             |                                                                                     |
|                                      | Processing Environment (NSPE). Located in :file:`build/tfm/bin`.                                       |                                                                                     |
+--------------------------------------+--------------------------------------------------------------------------------------------------------+-------------------------------------------------------------------------------------+
| :file:`zephyr.signed.bin`            | Image update file used to create :file:`dfu_application.zip` for multi-core DFU.                       | DFU process for single or multi-core board targets                                  |
|                                      | Can also be used standalone for a single-image DFU.                                                    |                                                                                     |
|                                      | Contains the signed version of the application.                                                        |                                                                                     |
|                                      | This file is transferred in the real-life update procedure, as opposed to HEX files                    |                                                                                     |
|                                      | that are transferred with nrfjprog when emulating an update procedure.                                 |                                                                                     |
|                                      | :ref:`Compatible with MCUboot <mcuboot:mcuboot_ncs>`.                                                  |                                                                                     |
|                                      | ``zephyr`` is the value of :kconfig:option:`CONFIG_KERNEL_BIN_NAME`.                                   |                                                                                     |
|                                      | The file is located in the :file:`build/<your_application_name>/zephyr` directory.                     |                                                                                     |
+--------------------------------------+--------------------------------------------------------------------------------------------------------+-------------------------------------------------------------------------------------+
| :file:`zephyr.signed.hex`            | HEX file variant of the :file:`<file_name>.signed.bin` file.                                           | Programming single or multi-core board targets                                      |
|                                      | Can also be used standalone for a single-image DFU.                                                    |                                                                                     |
|                                      | Contains the signed version of the application.                                                        |                                                                                     |
|                                      | :ref:`Compatible with MCUboot <mcuboot:mcuboot_ncs>`.                                                  |                                                                                     |
|                                      | ``zephyr`` is the value of :kconfig:option:`CONFIG_KERNEL_BIN_NAME`.                                   |                                                                                     |
|                                      | The file is located in the :file:`build/<your_application_name>/zephyr` directory.                     |                                                                                     |
+--------------------------------------+--------------------------------------------------------------------------------------------------------+-------------------------------------------------------------------------------------+
| :file:`zephyr.signed.bin` in         | Secondary slot variant of the :file:`zephyr.signed.bin` file.                                          | DFU process for single-core board targets.                                          |
| :file:`mcuboot_secondary_app` folder | :ref:`Compatible with MCUboot <mcuboot:mcuboot_ncs>` in the :doc:`direct-xip mode <mcuboot:design>`.   |                                                                                     |
|                                      | ``zephyr`` is the value of :kconfig:option:`CONFIG_KERNEL_BIN_NAME`.                                   |                                                                                     |
+--------------------------------------+--------------------------------------------------------------------------------------------------------+-------------------------------------------------------------------------------------+
| :file:`zephyr.signed.hex` in         | Secondary slot variant of the :file:`zephyr.signed.hex` file.                                          | Programming single-core board targets.                                              |
| :file:`mcuboot_secondary_app` folder | :ref:`Compatible with MCUboot <mcuboot:mcuboot_ncs>` in the :doc:`direct-xip mode <mcuboot:design>`.   |                                                                                     |
|                                      | ``zephyr`` is the value of :kconfig:option:`CONFIG_KERNEL_BIN_NAME`.                                   |                                                                                     |
+--------------------------------------+--------------------------------------------------------------------------------------------------------+-------------------------------------------------------------------------------------+
| :file:`dfu_application.zip`          | Zip file containing both the MCUboot-compatible update images for one or more cores and a manifest     | DFU process for both single-core and multi-core applications.                       |
|                                      | describing its contents.                                                                               |                                                                                     |
+--------------------------------------+--------------------------------------------------------------------------------------------------------+-------------------------------------------------------------------------------------+
| :file:`matter.ota`                   | :ref:`ug_matter`-specific OTA image that contains a Matter-compliant header and a DFU multi-image      | DFU over Matter for both single-core and multi-core applications.                   |
|                                      | package that bundles user-selected firmware images.                                                    |                                                                                     |
|                                      | ``matter.ota`` is the value of ``SB_CONFIG_MATTER_OTA_IMAGE_FILE_NAME``.                               |                                                                                     |
+--------------------------------------+--------------------------------------------------------------------------------------------------------+-------------------------------------------------------------------------------------+
| :file:`<file_name>.zigbee`           | :ref:`ug_zigbee`-specific OTA image that contains the Zigbee application with the Zigbee OTA header    | DFU over Zigbee for both single-core and multi-core applications                    |
|                                      | used for providing information about the image to the OTA server.                                      | in the |NCS| v2.0.0 and later.                                                      |
|                                      | The *<file_name>* includes manufacturer's code, image type, file version, and comment                  |                                                                                     |
|                                      | (customizable by user, sample name by default).                                                        |                                                                                     |
|                                      | For example: :file:`127F-0141-01020003-light_switch.zigbee`.                                           |                                                                                     |
+--------------------------------------+--------------------------------------------------------------------------------------------------------+-------------------------------------------------------------------------------------+

.. _app_build_mcuboot_output:

MCUboot output build files
**************************

.. note::
    MCUboot's :doc:`direct-xip mode <mcuboot:design>` and the related ``SB_CONFIG_MCUBOOT_MODE_DIRECT_XIP`` and ``SB_CONFIG_MCUBOOT_MODE_DIRECT_XIP_WITH_REVERT`` Kconfig options are currently supported only on the single-core devices such as the nRF52 Series.
    For more details, see the :ref:`more information <ug_nrf52_developing_ble_fota_mcuboot_direct_xip_mode>` section of the :ref:`ug_nrf52_developing` page.

+-----------------------------------------------+---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| File                                          | Description                                                                                                                                                                                                                                       |
+===============================================+===================================================================================================================================================================================================================================================+
| :file:`dfu_application.zip`                   | Contains the following:                                                                                                                                                                                                                           |
|                                               |                                                                                                                                                                                                                                                   |
|                                               | * The MCUboot-compatible update image for one or more cores when MCUboot is *not* in the :doc:`direct-xip mode <mcuboot:design>` and a manifest describing its contents (all related :file:`*.bin` files and a :file:`manifest.json` file).       |
|                                               | * The MCUboot-compatible update image for the primary and secondary slots when MCUboot is in the :doc:`direct-xip mode <mcuboot:design>` and manifest describing its contents (all related :file:`*.bin` files and a :file:`manifest.json` file). |
+-----------------------------------------------+---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| :file:`dfu_mcuboot.zip`                       | Contains two versions of MCUboot linked against different address spaces corresponding to slot0 (s0) and slot1 (s1) and a manifest JSON file describing their MCUboot version number (``SB_CONFG_SECURE_BOOT_MCUBOOT_VERSION``),                  |
|                                               | :ref:`bootloader` (NSIB) version number (:kconfig:option:`CONFIG_FW_INFO`), board type. This file can be used by FOTA servers (for example, nRF Cloud) to serve both s0 and s1 to the device.                                                     |
|                                               | The device can then select the firmware file for the slot that is currently not in use.                                                                                                                                                           |
|                                               | Created when the options ``SB_CONFIG_SECURE_BOOT_APPCORE`` and ``SB_CONFIG_BOOTLOADER_MCUBOOT`` are set.                                                                                                                                          |
+-----------------------------------------------+---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| :file:`zephyr.signed.bin`                     | Signed variant of the firmware in binary format (as opposed to HEX).                                                                                                                                                                              |
|                                               | It can be uploaded to a server as a FOTA image.                                                                                                                                                                                                   |
|                                               | ``zephyr`` is the value of :kconfig:option:`CONFIG_KERNEL_BIN_NAME`.                                                                                                                                                                              |
+-----------------------------------------------+---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| :file:`signed_by_mcuboot_and_b0_mcuboot.bin`  | MCUboot update image for s0 signed by both MCUboot and NSIB.                                                                                                                                                                                      |
|                                               | The MCUboot signature is used by MCUboot to verify the integrity of the image before swapping and the NSIB signature is used by NSIB before booting the image.                                                                                    |
+-----------------------------------------------+---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| :file:`signed_by_mcuboot_and_b0_s1_image.bin` | MCUboot update image for s1 signed by both MCUboot and NSIB.                                                                                                                                                                                      |
|                                               | The MCUboot signature is used by MCUboot to verify the integrity of the image before swapping and the NSIB signature is used by NSIB before booting the image.                                                                                    |
+-----------------------------------------------+---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| :file:`zephyr.signed.hex`                     | Signed variant of the firmware in the HEX format.                                                                                                                                                                                                 |
|                                               | This HEX file is linked to the same address as the application.                                                                                                                                                                                   |
|                                               | Programming this file to the device will overwrite the existing application.                                                                                                                                                                      |
|                                               | It will not trigger a DFU procedure.                                                                                                                                                                                                              |
|                                               | ``zephyr`` is the value of :kconfig:option:`CONFIG_KERNEL_BIN_NAME`.                                                                                                                                                                              |
+-----------------------------------------------+---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| :file:`zephyr.signed.bin` in                  | Secondary slot variant of the :file:`app_update.bin` file intended for use when MCUboot is in the :doc:`direct-xip mode <mcuboot:design>`.                                                                                                        |
| :file:`mcuboot_secondary_app` folder          | Created when the :kconfig:option:`CONFIG_BOOT_BUILD_DIRECT_XIP_VARIANT` Kconfig option is enabled.                                                                                                                                                |
|                                               | ``zephyr`` is the value of :kconfig:option:`CONFIG_KERNEL_BIN_NAME`.                                                                                                                                                                              |
+-----------------------------------------------+---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| :file:`zephyr.signed.hex`                     | Secondary slot variant of the :file:`app_signed.hex` file intended for use when MCUboot is in the :doc:`direct-xip mode <mcuboot:design>`.                                                                                                        |
| :file:`mcuboot_secondary_app` folder          | Created when the :kconfig:option:`CONFIG_BOOT_BUILD_DIRECT_XIP_VARIANT` Kconfig option is enabled.                                                                                                                                                |
|                                               | ``zephyr`` is the value of :kconfig:option:`CONFIG_KERNEL_BIN_NAME`.                                                                                                                                                                              |
+-----------------------------------------------+---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+

.. _app_build_output_files_suit_dfu:

SUIT output build files
***********************

The following table lists secondary build files that can be generated when building firmware update packages using the :ref:`Software Updates for Internet of Things (SUIT) DFU procedure <ug_nrf54h20_suit_intro>`.

+-------------------------------------------------+------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| File                                            | Description                                                                                                                                                            |
+=================================================+========================================================================================================================================================================+
| :file:`root_with_binary_nordic_top.yaml.jinja2` | SUIT Manifest templates automatically placed in the sample directory after the first build of the :ref:`nrf54h_suit_sample` sample.                                    |
|                                                 | They serve as the basis for generating the specific SUIT envelopes tailored to the requirements of different domains within the device (root, application, and radio). |
| :file:`app_envelope.yaml.jinja2`                |                                                                                                                                                                        |
|                                                 |                                                                                                                                                                        |
| :file:`rad_envelope.yaml.jinja2`                |                                                                                                                                                                        |
+-------------------------------------------------+------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| :file:`root.suit`                               | Binary SUIT envelopes that are generated from their respective YAML manifest templates during the build process of the :ref:`nrf54h_suit_sample` sample.               |
|                                                 | The :file:`root.suit` contains embedded application core manifest (:file:`application.suit`) and radio core manifest (:file:`radio.suit`).                             |
| :file:`application.suit`                        | The :file:`radio.suit` is not generated for the UART version of the :ref:`nrf54h_suit_sample`.                                                                         |
|                                                 | These files can be found in the :file:`build/zephyr` directory after building the sample.                                                                              |
| :file:`radio.suit`                              |                                                                                                                                                                        |
+-------------------------------------------------+------------------------------------------------------------------------------------------------------------------------------------------------------------------------+

.. _app_build_output_files_other:

Other output build files
************************

The following table lists secondary build files that can be generated when building firmware, but are only used to create the final output build files listed in the table above.

+-----------------------------------+------------------------------------------------------------------------------------------------------+
| File                              | Description                                                                                          |
+===================================+======================================================================================================+
| :file:`zephyr.elf`                | An ELF file for the image that is being built. Can be used for debugging purposes.                   |
+-----------------------------------+------------------------------------------------------------------------------------------------------+
| :file:`zephyr.meta`               | A file with the Zephyr and nRF Connect SDK git hashes for the commits used to build the application. |
|                                   | If your working tree contains uncommitted changes, the build system adds the suffix ``-dirty``       |
|                                   | to the relevant version field.                                                                       |
+-----------------------------------+------------------------------------------------------------------------------------------------------+
| :file:`tfm_s.elf`                 | An ELF file for the TF-M image that is being built. Can be used for debugging purposes.              |
+-----------------------------------+------------------------------------------------------------------------------------------------------+
| :file:`manifest.json`             | Output artifact that uses information from the :file:`zephyr.meta` output file.                      |
+-----------------------------------+------------------------------------------------------------------------------------------------------+
| :file:`dfu_multi_image.bin`       | Multi-image package that contains a CBOR manifest and a set of user-selected update images,          |
|                                   | such as firmware images for different cores.                                                         |
|                                   | Used for DFU purposes by :ref:`ug_matter` and :ref:`ug_zigbee` protocols.                            |
+-----------------------------------+------------------------------------------------------------------------------------------------------+
| :file:`signed_by_b0_mcuboot.bin`  | Intermediate file only signed by NSIB.                                                               |
+-----------------------------------+------------------------------------------------------------------------------------------------------+
| :file:`signed_by_b0_s1_image.bin` | Intermediate file only signed by NSIB.                                                               |
+-----------------------------------+------------------------------------------------------------------------------------------------------+
