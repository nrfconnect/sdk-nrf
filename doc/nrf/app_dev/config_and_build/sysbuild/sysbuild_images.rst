.. _sysbuild_images:

Sysbuild images
###############

.. contents::
   :local:
   :depth: 2

:ref:`configuration_system_overview_sysbuild` allows you to add additional images to your builds.

Enabling images
***************

To add an additional image using sysbuild, you must modify the central sysbuild configuration.
This is typically done in a `sysbuild.conf` file within an application, which is a Kconfig fragment applied to the default sysbuild configuration when a project is configured.

.. note::
   On the nRF54H20 SoC, do not use any ``SECURE_BOOT`` or ``MCUBOOT`` option.
   The nRF54H20 SoC boot sequence is based on the Secure Domain, and it cannot be disabled.
   For more information, see :ref:`ug_nrf54h20_architecture_boot`.

The following sysbuild Kconfig options can be used to enable images in a build:

+-------------------------------------------------+-----------------------------------------------------------------------------------------+
| Sysbuild Kconfig option                         | Description                                                                             |
+=================================================+=========================================================================================+
| :kconfig:option:`SB_CONFIG_SECURE_BOOT_APPCORE` | Enable secure boot for application core (or main core if device only has a single core).|
+-------------------------------------------------+-----------------------------------------------------------------------------------------+
| :kconfig:option:`SB_CONFIG_BOOTLOADER_MCUBOOT`  | Build MCUboot image.                                                                    |
+-------------------------------------------------+-----------------------------------------------------------------------------------------+

The following sysbuild Kconfig options are also available for nRF53-based devices.
These options determine whether the secure boot image is included on the network core and specify the image for the network core:

+--------------------------------------------------+-----------------------------------------------------------------------------------------------------------+
| Sysbuild Kconfig option                          | Description                                                                                               |
+==================================================+===========================================================================================================+
| :kconfig:option:`SB_CONFIG_SECURE_BOOT_NETCORE`  | Enable secure boot for network core.                                                                      |
+--------------------------------------------------+-----------------------------------------------------------------------------------------------------------+
| :kconfig:option:`SB_CONFIG_NETCORE_EMPTY`        | |NCS| empty network core image :ref:`nrf5340_empty_net_core`.                                             |
+--------------------------------------------------+-----------------------------------------------------------------------------------------------------------+
| :kconfig:option:`SB_CONFIG_NETCORE_HCI_IPC`      | Zephyr hci_ipc Bluetooth image :zephyr:code-sample:`bluetooth_hci_ipc`.                                   |
+--------------------------------------------------+-----------------------------------------------------------------------------------------------------------+
| :kconfig:option:`SB_CONFIG_NETCORE_RPC_HOST`     | |NCS| rpc_host Bluetooth image :ref:`ble_rpc_host`.                                                       |
+--------------------------------------------------+-----------------------------------------------------------------------------------------------------------+
| :kconfig:option:`SB_CONFIG_NETCORE_802154_RPMSG` | Zephyr 802.15.4 image :zephyr:code-sample:`nrf_ieee802154_rpmsg`.                                         |
+--------------------------------------------------+-----------------------------------------------------------------------------------------------------------+
| :kconfig:option:`SB_CONFIG_NETCORE_IPC_RADIO`    | |NCS| ipc_radio image :ref:`ipc_radio`.                                                                   |
+--------------------------------------------------+-----------------------------------------------------------------------------------------------------------+
| :kconfig:option:`SB_CONFIG_NETCORE_NONE`         | No network core image.                                                                                    |
+--------------------------------------------------+-----------------------------------------------------------------------------------------------------------+

The following sysbuild Kconfig options are available when MCUboot is configured in :ref:`firmware loader mode <ug_bootloader_using_firmware_loader_mode>`:
These options specify the image for the firmware loader:

+-----------------------------------------------------------+-----------------------------------------------------------------+
| Sysbuild Kconfig option                                   | Description                                                     |
+===========================================================+=================================================================+
| :kconfig:option:`SB_CONFIG_FIRMWARE_LOADER_IMAGE_SMP_SVR` | Include :zephyr:code-sample:`smp-svr` as firmware loader image. |
+-----------------------------------------------------------+-----------------------------------------------------------------+

.. _sysbuild_images_adding_custom_images:

Adding custom images
********************

Custom images can be added directly to a project (or board) or to a Zephyr module, making them accessible to multiple projects.

.. _sysbuild_images_adding_to_single_project:

Adding to a single project
--------------------------

To add an image to a single project, you need a :file:`sysbuild.cmake` file in the root folder of your project to incorporate the image into the project.
If the image selection is optional, a :file:`Kconfig.sysbuild` file in the root folder of your project is also required to include Kconfig options for the sysbuild configuration.
If the image selection is mandatory, the :file:`Kconfig.sysbuild` file can be omitted.


* :file:`kconfig.sysbuild` file:

    .. code-block:: kconfig

        config MY_APP_IMAGE_ABC
            bool "Include ABC image"
            depends on SOC_SERIES_NRF53
            default y if BOARD_NRF5340DK_NRF5340_CPUAPP
            help
              Will include the ABC image in the build, which will...

        source "$(ZEPHYR_BASE)/share/sysbuild/Kconfig"

* :file:`sysbuild.cmake` file

    .. code-block:: cmake

        if(SB_CONFIG_MY_APP_IMAGE_ABC)
          ExternalZephyrProject_Add(
            APPLICATION ABC
            SOURCE_DIR "<path_to_application>"
            BUILD_ONLY true   # This will build the application and not flash it, this **must** be used when building additional images to a core (not the primary image) when using Partition Manager, as the main application for each core will flash a merged hex file instead
          )
        endif()

This method can be used to add a new image to the existing board target.

.. _sysbuild_images_adding_custom_network_core_images:

Adding custom network core images
=================================

To add an image for a different board target (like for the network core of the nRF5340 SoC), you must use a different syntax.
This can be handled using the following approach:

* :file:`kconfig.sysbuild` file:

    .. code-block:: kconfig

        menu "Network core configuration"
            depends on SUPPORT_NETCORE

        config SUPPORT_NETCORE_ABC
            bool
            default y

        choice NETCORE
            prompt "Netcore image"
            depends on SUPPORT_NETCORE && !EXTERNAL_CONFIGURED_NETCORE

        config NETCORE_ABC
            bool "ABC"
            help
              Use ABC image as the network core image.

        endchoice

        if !NETCORE_NONE

        config NETCORE_IMAGE_NAME
            default "abc" if NETCORE_ABC

        config NETCORE_IMAGE_PATH
            default "$(ZEPHYR_MY_MODULE_MODULE_DIR)/<image_path>" if NETCORE_ABC

        endif # !NETCORE_NONE

        endmenu

        source "$(ZEPHYR_BASE)/share/sysbuild/Kconfig"

* :file:`sysbuild.cmake` file - This file is optional and should be used only if specific custom configurations are required for the application.

    .. code-block:: cmake

        if(SB_CONFIG_NETCORE_ABC)
          # Project can optionally be configured here if needed

          # This will add a Kconfig fragment file, named `my_extra.conf` from the application directory
          add_overlay_config(${SB_CONFIG_NETCORE_IMAGE_NAME} ${SB_CONFIG_NETCORE_IMAGE_PATH}/my_extra.conf)
          # This will add a devicetree overlay file, named `my_extra.dts` from the application directory
          add_overlay_dts(${SB_CONFIG_NETCORE_IMAGE_NAME} ${SB_CONFIG_NETCORE_IMAGE_PATH}/my_extra.dts)
          # This will set a bool Kconfig option in the image (note: sysbuild forces this setting, it cannot be overwritten by changing the application configuration)
          set_config_bool(${SB_CONFIG_NETCORE_IMAGE_NAME} CONFIG_MY_CUSTOM_CONFIG y)
          # This will set a string (or numeric) Kconfig option in the image (note: sysbuild forces this setting, it cannot be overwritten by changing the application configuration)
          set_property(TARGET ${SB_CONFIG_NETCORE_IMAGE_NAME} APPEND_STRING PROPERTY CONFIG "CONFIG_FOO=my_custom_value\n")
        endif()

.. _sysbuild_images_adding_custom_firmware_loader_images:

Adding custom firmware loader images
====================================

You can add custom firmware loader images similarly to how nRF5340 network core images are incorporated.
This can be handled using the following approach:

* :file:`kconfig.sysbuild` file:

    .. code-block:: kconfig

        menu "Firmware loader configuration"
            depends on MCUBOOT_MODE_FIRMWARE_UPDATER

        config SUPPORT_FIRMWARE_LOADER_ABC
            bool
            default y

        choice FIRMWARE_LOADER
            prompt "Firmware loader image"
            depends on MCUBOOT_MODE_FIRMWARE_UPDATER

        config FIRMWARE_LOADER_IMAGE_ABC
            bool "ABC"
            help
              Use ABC image as the firmware loader image.

        endchoice

        if !FIRMWARE_LOADER_IMAGE_NONE

        config FIRMWARE_LOADER_IMAGE_NAME
            default "abc" if FIRMWARE_LOADER_IMAGE_ABC

        config FIRMWARE_LOADER_IMAGE_PATH
            default "$(ZEPHYR_MY_MODULE_MODULE_DIR)/<image_path>" if FIRMWARE_LOADER_IMAGE_ABC

        endif # !FIRMWARE_LOADER_IMAGE_NONE

        endmenu

        source "$(ZEPHYR_BASE)/share/sysbuild/Kconfig"

* :file:`sysbuild.cmake` file - This file is optional and should be used only if specific custom configurations are required for the application.

    .. code-block:: cmake

        if(SB_CONFIG_FIRMWARE_LOADER_IMAGE_ABC)
          # Project can optionally be configured here if needed

          # This will add a Kconfig fragment file, named `my_extra.conf` from the application directory
          add_overlay_config(${SB_CONFIG_FIRMWARE_LOADER_IMAGE_NAME} ${SB_CONFIG_FIRMWARE_LOADER_IMAGE_PATH}/my_extra.conf)
          # This will add a devicetree overlay file, named `my_extra.dts` from the application directory
          add_overlay_dts(${SB_CONFIG_FIRMWARE_LOADER_IMAGE_NAME} ${SB_CONFIG_FIRMWARE_LOADER_IMAGE_PATH}/my_extra.dts)
          # This will set a bool Kconfig option in the image (note: sysbuild forces this setting, it cannot be overwritten by changing the application configuration)
          set_config_bool(${SB_CONFIG_FIRMWARE_LOADER_IMAGE_NAME} CONFIG_MY_CUSTOM_CONFIG y)
          # This will set a string (or numeric) Kconfig option in the image (note: sysbuild forces this setting, it cannot be overwritten by changing the application configuration)
          set_property(TARGET ${SB_CONFIG_FIRMWARE_LOADER_IMAGE_NAME} APPEND_STRING PROPERTY CONFIG "CONFIG_FOO=my_custom_value\n")
        endif()

.. _sysbuild_images_adding_to_a_single_board:

Adding to a single board
========================

You can place the same code as in the :ref:`sysbuild_images_adding_to_single_project` section, without the Zephyr sourcing, in a board directory.
This enables the use of those images for any sysbuild-based project being built for that board:

Kconfig.sysbuild:

.. code-block:: kconfig

    config MY_APP_IMAGE_ABC
        bool "Include ABC image"
        depends on SOC_SERIES_NRF53
        default y if BOARD_NRF5340DK_NRF5340_CPUAPP
        help
          Will include the ABC image in the build, which will...

sysbuild.cmake:

.. code-block:: cmake

    if(SB_CONFIG_MY_APP_IMAGE_ABC)
      ExternalZephyrProject_Add(
        APPLICATION ABC
        SOURCE_DIR "<path_to_application>"
        BUILD_ONLY true   # This will build the application and not flash it, this **must** be used when building additional images to a core (not the primary image) when using Partition Manager, as the main application for each core will flash a merged hex file instead
      )
    endif()

.. _sysbuild_images_adding_via_a_zephyr_module:

Adding through a Zephyr module
==============================

To add images in a Zephyr module, create a folder within the module to hold the `Kconfig.sysbuild` and (optionally, if needed) `CMakeLists.txt` files.
Then, add this folder to the Zephyr module file:

.. code-block:: yaml

    build:
      sysbuild-cmake: sysbuild  # Only needed if a sysbuild CMakeLists.txt file is being added
      sysbuild-kconfig: sysbuild/Kconfig.sysbuild

The ``CMakeLists.txt`` file is the same as the ``sysbuild.cmake`` file from the previous examples.
The ``Kconfig.sysbuild`` file is the same as the file from the previous examples but without the Zephyr sourcing.
When images are configured, these additional images will be available from sysbuild and can be used in any project within the tree.

Kconfig.sysbuild:

.. code-block:: kconfig

    menu "Network core configuration"
        depends on SUPPORT_NETCORE

    config SUPPORT_NETCORE_ABC
        bool
        default y

    choice NETCORE
        prompt "Netcore image"
        depends on SUPPORT_NETCORE && !EXTERNAL_CONFIGURED_NETCORE

    config NETCORE_ABC
        bool "ABC"
        help
          Use ABC image as the network core image.

    endchoice

    if !NETCORE_NONE

    config NETCORE_IMAGE_NAME
        default "abc" if NETCORE_ABC

    config NETCORE_IMAGE_PATH
        default "$(ZEPHYR_MY_MODULE_MODULE_DIR)/<image_path>" if NETCORE_ABC

    endif # !NETCORE_NONE

    endmenu

.. _sysbuild_images_editing_in_nrfvsc:

Editing sysbuild images and domains in |nRFVSC|
***********************************************

|nRFVSC| provides a GUI for editing sysbuild images and domains.
See the `How to work with sysbuild domains`_ page in the extension documentation for more information.
