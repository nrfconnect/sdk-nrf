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

Drivers
=======

This section describes the changes related to drivers.

|no_changes_yet_note|

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
