.. _migration_3.5:

Migration notes for |NCS| v3.5.0 (Working draft)
################################################

.. contents::
   :local:
   :depth: 3

This document describes the changes required or recommended when migrating your application from |NCS| v3.4.0 to |NCS| v3.5.0.

.. HOWTO
   Add changes in the following format:
   Component (for example, application, sample or libraries)
   *********************************************************
   .. toggle::
      * Change1 and description
      * Change2 and description

.. _migration_3.5_required:

Required changes
****************

The following changes are mandatory to make your application work in the same way as in previous releases.

Build and configuration system
==============================

This section describes the changes related to the build and configuration system.

.. toggle::

   * Device Firmware Update (DFU) support for the nRF70 Series firmware patch has been removed, together with the following Kconfig options:

     * ``SB_CONFIG_DFU_MULTI_IMAGE_PACKAGE_WIFI_FW_PATCH``
     * ``SB_CONFIG_DFU_ZIP_WIFI_FW_PATCH``
     * ``CONFIG_NRF_WIFI_FW_PATCH_DFU``

     If your application enabled any of these options, remove them.
     The nRF70 Series firmware patch is no longer allocated a separate MCUboot update slot.
     If you set the :kconfig:option:`SB_CONFIG_MCUBOOT_UPDATEABLE_IMAGES` Kconfig option or a static partition layout explicitly for a firmware-patch build, reduce the number of updatable images by one and remove the now-unused update-slot partitions.
     Storing the nRF70 Series firmware patch in external flash using the :kconfig:option:`SB_CONFIG_WIFI_PATCHES_EXT_FLASH_XIP` or :kconfig:option:`SB_CONFIG_WIFI_PATCHES_EXT_FLASH_STORE` Kconfig option is not affected.

Samples and applications
========================

This section describes the changes related to samples and applications.

.. _bt_fast_pair_migration_3.5:

Bluetooth Fast Pair samples
---------------------------

.. toggle::

   * The :ref:`fast_pair_locator_tag` and :ref:`fast_pair_input_device` samples no longer support the nRF52 and nRF53 Series devices.
     The following board targets have been removed from both samples:

     * ``nrf52dk/nrf52832``
     * ``nrf52840dk/nrf52840``
     * ``nrf5340dk/nrf5340/cpuapp``
     * ``nrf5340dk/nrf5340/cpuapp/ns``

     Additionally, the following board targets have been removed from the :ref:`fast_pair_locator_tag` sample:

     * ``nrf52833dk/nrf52833``
     * ``thingy53/nrf5340/cpuapp``
     * ``thingy53/nrf5340/cpuapp/ns``

     If your application is based on one of these samples and targets an nRF52 or nRF53 Series device, continue using the |NCS| v3.4.0 release or migrate your design to a supported nRF54L Series device.

.. _matter_migration_3.5:

Matter
------

.. toggle::

   * All Matter samples, shared sample code, devicetree partition files, and Matter-specific snippets have been moved from ``sdk-nrf`` to the separate `Matter add-on <ncs-matter add-on repository_>`_ repository (``ncs-matter``).
     The Matter bridge and Thingy:53 weather station reference applications are also relocated into the add-on under :file:`ncs-matter/samples/`.

     If your project is based on a Matter sample or application from ``sdk-nrf`` v3.4.0 or earlier, you must migrate to the add-on structure to continue receiving sample updates.
     See :ref:`migration_sdk_nrf_to_ncs_matter` for the full migration guide, including path mapping tables, Kconfig symbol renames, devicetree include updates, snippet name changes, and workspace setup instructions.

Libraries
=========

This section describes the changes related to libraries.

.. toggle::

   * :ref:`lib_location` library:

     * The library now always uses the chosen ``zephyr,wifi`` node to find the used Wi-Fi device.
       If your application uses the deprecated ``ncs,location-wifi`` node, you need to change it to use the ``zephyr,wifi`` node instead:

       .. code-block:: dts

          chosen {
                  zephyr,wifi = &mywifi;
          };

   * :ref:`trusted_storage_readme` library:

     * The library has been deprecated and will be removed in a future release.
       Use the :ref:`Secure Storage subsystem <secure_storage>` instead (:kconfig:option:`CONFIG_SECURE_STORAGE`).
       If you have an existing installation that uses the Trusted Storage library with entries stored in non-volatile memory, you can switch to using Secure Storage without losing any data by enabling the :kconfig:option:`CONFIG_SECURE_STORAGE_TRUSTED_STORAGE_COMPATIBILITY` Kconfig option.

Drivers
=======

This section describes the changes related to drivers.

.. toggle::

   * Wi-Fi drivers for the nRF70 and nRF71 Series:

     * Updated the default values of the following Kconfig options to reduce the default RAM footprint of the Wi-Fi drivers:

       * :kconfig:option:`CONFIG_NRF70_RX_NUM_BUFS` (or :kconfig:option:`CONFIG_NRF71_RX_NUM_BUFS`) from ``48`` to ``16``.
       * :kconfig:option:`CONFIG_NRF70_MAX_TX_AGGREGATION` (or :kconfig:option:`CONFIG_NRF71_MAX_TX_AGGREGATION`) from ``12`` to ``4``.
       * :kconfig:option:`CONFIG_NRF_WIFI_DATA_HEAP_SIZE` from ``130000`` to ``65536``.

     * If your application relies on the previous default values, set these Kconfig options to their earlier values.

Clock control nrf deprecation
-----------------------------

.. toggle::

   The :ref:`clock_control_api` driver has been updated for the following clocks on nRF52, nRF53, nRF91, and nRF54L Series devices:

   * HFCLK
   * LFCLK
   * XO
   * XO24M
   * HFCLK192M
   * HFCLKAUDIO

   To restore the legacy driver implementation, set :kconfig:option:`CONFIG_CLOCK_CONTROL_NRF` to ``y``.

   To migrate your code from |NCS| v3.4.0 to |NCS| v3.5.0, complete the following steps:

   1. Enable each application-controlled clock in the application-specific or board-specific devicetree overlay file.

      This enables the corresponding clock driver.
      For example:

      .. code-block:: dts

          /* if nRF54L XO is to be controlled */
          &xo {
              status = "okay";
          };

          /* if nRF52, nRF53 HFCLK is to be controlled */
          &hfclk {
              status = "okay";
          };

          /* if nRF52, nRF53, nRF91 or nRF54L LFCLK is to be controlled */
          &lfclk {
              status = "okay";
          }

          /* if HFCLK192M is to be controlled */
          &hfclk192m {
              status = "okay";
          }

          /* if XO24M is to be controlled */
          &xo24m {
              status = "okay";
          }

          /* if HFCLKAUDIO is to be controlled */
          &hfclkaudio {
              status = "okay";
          }

   #. Rename the following Kconfig options:

      * Replace :kconfig:option:`CONFIG_NRFX_CLOCK_USE_LFRC_CALIBRATION` with :kconfig:option:`CONFIG_NRFX_CLOCK_LFCLK_USE_LFRC_CALIBRATION`.
      * Replace :kconfig:option:`CONFIG_NRFX_CLOCK_LF_CAL_ENABLED` with :kconfig:option:`CONFIG_CLOCK_CONTROL_NRF_K32SRC_RC_CALIBRATION`.

   #. Move the following Kconfig options to the ``nordic,nrf-clock-lfclk`` devicetree node:

      * Replace :kconfig:option:`CONFIG_CLOCK_CONTROL_NRF_K32SRC_FREQUENCY` with the ``k32src-frequency`` property.
      * Replace :kconfig:option:`CONFIG_CLOCK_CONTROL_NRF_SOURCE` with the ``k32src`` property.
      * Replace :kconfig:option:`CONFIG_CLOCK_CONTROL_NRF_ACCURACY_PPM` with the ``k32src-accuracy-ppm`` property.
      * Replace :kconfig:option:`CONFIG_CLOCK_CONTROL_NRF_ACCURACY` with the ``k32src-accuracy-ppm`` property.
      * Replace :kconfig:option:`CONFIG_NRFX_CLOCK_LFXO_TWO_STAGE_ENABLED` with the ``k32src`` property.

   #. Update your application to use the new clock control API.

      Use the following mapping when you update the API calls:

      * ``mgr`` is the on-off manager created for ``nordic,nrf-clock`` and obtained using ``z_nrf_clock_control_get_onoff``.
      * ``dev`` is the device compatible with ``nordic,nrf-clock``.
      * ``sys`` is the subsystem for ``nordic,nrf-clock``.
         The new clocks implementation does not use it.
      * ``new_dev`` is the device that corresponds to the previously used ``sys`` value.
        It must be compatible with one of the following nodes:

        * ``nordic,nrf-clock-lfclk``
        * ``nordic,nrf-clock-hfclk``
        * ``nordic,nrf-clock-xo``
        * ``nordic,nrf-clock-hfclk192m``
        * ``nordic,nrf-clock-xo24m``
        * ``nordic,nrf-clock-hfclkaudio``

      The following example shows the deprecated API usage and the corresponding new API usage:

      .. code-block:: c

         // Old API usage (deprecated)
         z_nrf_clock_calibration_init(&mgrs);    //1
         onoff_release(mgr)                      //2
         onoff_request(mgr, &cli);               //3
         onoff_cancel_or_release(mgr, &cli);     //4
         clock_control_on(dev,sys)               //5
         clock_control_off(dev,sys)              //6
         clock_control_async_on(dev,sys)         //7
         clock_control_get_status(dev,sys)       //8
         z_nrf_clock_control_get_onoff(sys)      //9

         // New API usage
         z_nrf_clock_calibration_init();                             //1
         nrf_clock_control_release(new_dev, NULL);                   //2
         nrf_clock_control_request(new_dev, NULL, &cli);             //3
         nrf_clock_control_cancel_or_release(new_dev, NULL, &cli);   //4
         clock_control_on(new_dev, NULL)                             //5
         clock_control_off(new_dev, NULL)                            //6
         clock_control_async_on(new_dev, NULL)                       //7
         clock_control_get_status(new_dev, NULL)                     //8
         // Remove all uses of z_nrf_clock_control_get_onoff         //9

.. _migration_3.5_recommended:

Recommended changes
*******************

The following changes are recommended for your application to work optimally after the migration.

Build and configuration system
==============================

|no_changes_yet_note|

Samples and applications
========================

This section describes the changes related to samples and applications.

|no_changes_yet_note|

Libraries
=========

This section describes the changes related to libraries.

|no_changes_yet_note|
