.. _ncs_release_notes_changelog:

Changelog for |NCS| v1.7.99
###########################

.. contents::
   :local:
   :depth: 2

The most relevant changes that are present on the master branch of the |NCS|, as compared to the latest official release, are tracked in this file.

.. note::
   This file is a work in progress and might not cover all relevant changes.

Highlights
**********

There are no entries for this section yet.

Known issues
************

Known issues are only tracked for the latest official release.
See `known issues for nRF Connect SDK v1.7.0`_ for the list of issues valid for the latest release.

Changelog
*********

The following sections provide detailed lists of changes by component.

nRF9160
=======

* Updated:

  * :ref:`lwm2m_client` sample:

    * Added support for Thingy:91.
    * Added more LwM2M objects.
    * LwM2M sensor objects now uses the actual sensors available to the Thingy:91. If the nRF9160 DK is used, it uses simulated sensors instead.
    * Added possibility to poll sensors and notify the server if the measured changes are large enough.

  * A-GPS library:

    * Removed GNSS socket API support.

  * :ref:`lib_nrf_cloud` library:

    * Removed GNSS socket API support from A-GPS and P-GPS.
    * Added support for sending data to a new bulk endpoint topic that is supported in nRF Cloud.
      A message published to the bulk topic is typically a combination of multiple messages.
    * Changed REST API for A-GPS to use GNSS interface structure instead of GPS driver structure.
      Also changed from GPS driver ``GPS_AGPS_`` request types to ``NRF_CLOUD_AGPS_`` request types.

  * :ref:`asset_tracker_v2` application:

    * Updated the application to start sending batch messages to the new bulk endpoint topic supported in nRF Cloud.

  * :ref:`multicell_location` sample:

    * Updated to only request neighbor cell measurements when connected and to only copy neighbor cell measurements if they exist.

  * :ref:`lte_lc_readme` library:

    * Changed the value of an invalid E-UTRAN cell ID from zero to UINT32_MAX for the LTE_LC_EVT_NEIGHBOR_CELL_MEAS event.

  * :ref:`https_client` sample:

    * Added a possibility to use TF-M and Zephyr Mbed TLS instead of using the offloaded TLS stack in modem.

  * :ref:`serial_lte_modem` application:

    * Added the ``#XUUID`` command to read out the device UUID from the modem.
    * Added to the ``XNRFCLOUD`` command the following features:
      * The possibility to send to and receive from nRF Cloud JSON messages in data mode.
      * The ability to read out the ``sec_tag`` and the UUID of the device.

  * :ref:`lib_lwm2m_client_utils` library:

    * Added support for Firmware Update object to use :ref:`lib_fota_download` library for downloading firmware images.

nRF5
====

nRF Desktop
-----------

* Added:

  * Added documentation for :ref:`nrf_desktop_usb_state_pm`.

* Updated:

  * Updated information about custom build types.
  * Updated documentation for :ref:`nrf_desktop_usb_state`.
  * Updated documentation with information about forwarding boot reports.
    See the documenation page of nRF Desktop's :ref:`nrf_desktop_hid_forward` for details.
  * Fixed an issue that was causing the HID keyboard LEDs to remain turned on after host disconnection while no other hosts were connected.

Bluetooth® LE
-------------

* Updated:

  * Updated some samples with support for :ref:`zephyr:thingy53_nrf5340` in non-secure configuration.

Bluetooth mesh
--------------

* Updated:

  * Updated some samples with support for :ref:`zephyr:thingy53_nrf5340` in non-secure configuration.

Matter
------

* Added:

  * Multi-image Device Firmware Upgrade over Bluetooth LE support for nRF5340 DK in lock and light bulb samples.

Zigbee
------

* Added:

   * :ref:`Zigbee shell <zigbee_shell_sample>`.

Common
======

Edge Impulse
------------

* Added non-secure configuration for building with :ref:`zephyr:thingy53_nrf5340`.
* Updated information about custom build types.

Common Application Framework (CAF)
----------------------------------

Added:

* :ref:`caf_preview_sample` sample.

Updated:

* The power management support in modules is now enabled by default when the :kconfig:`CONFIG_CAF_PM_EVENTS` Kconfig option is enabled.

Hardware unique key
-------------------

* Make the checking for hw_unique_key_write_random() more strict; panic if any key is unwritten after writing random keys.
* Refactored the HUK_HAS_* macros to be defined/undefined instead of 1/0.
* Added a new sample :ref:`hw_unique_key_usage` showing how to use a hardware unique key to derive an encryption key.
  The sample can be run with or without TF-M.

Bugfixes:

* Fixed ``hw_unique_key_is_written()`` which would previously trigger a fault under certain circumstances.

Trusted Firmware-M
------------------

* Added:

  * Support for non-secure storage.
    This enables non-secure applications to use the Zephyr Settings API to save and load persistent data.

fprotect
--------

* Added a new function ``fprotect_is_protected()`` for devices with the ACL peripheral.

Immutable bootloader
--------------------

* Improved how hardware unique keys are handled.

  * Introduced :kconfig:`CONFIG_HW_UNIQUE_KEY_LOAD` with fewer dependencies than :kconfig:`CONFIG_HW_UNIQUE_KEY` solely for loading the key.
  * The bootloader now allows a single boot with no key present, to allow the app to write a key.
    After the first boot, the key must be present or the bootloader won't boot the app.

Immutable bootloader
--------------------

* Improved how hardware unique keys are handled.

  * Introduced :kconfig:`CONFIG_HW_UNIQUE_KEY_LOAD` with fewer dependencies than :kconfig:`CONFIG_HW_UNIQUE_KEY` solely for loading the key.
  * The bootloader now allows a single boot with no key present, to allow the app to write a key.
    After the first boot, the key must be present or the bootloader won't boot the app.

Secure partition manager (SPM)
------------------------------

Bug fixes:

* NCSDK-5156: Fixed the size calculation for the non-secure callable region, which prevented users from adding a large number of custom secure services.

MCUboot
=======

The MCUboot fork in |NCS| (``sdk-mcuboot``) contains all commits from the upstream MCUboot repository up to and including ``680ed07``, plus some |NCS| specific additions.

The code for integrating MCUboot into |NCS| is located in :file:`ncs/nrf/modules/mcuboot`.

The following list summarizes the most important changes inherited from upstream MCUboot:

* No changes yet

Mcumgr
======

The mcumgr library contains all commits from the upstream mcumgr repository up to and including snapshot ``657deb65``.

The following list summarizes the most important changes inherited from upstream mcumgr:

* No changes yet

Zephyr
======

.. NOTE TO MAINTAINERS: All the Zephyr commits in the below git commands must be handled specially after each upmerge and each NCS release.

The Zephyr fork in |NCS| (``sdk-zephyr``) contains all commits from the upstream Zephyr repository up to and including ``14f09a3b00``, plus some |NCS| specific additions.

For a complete list of upstream Zephyr commits incorporated into |NCS| since the most recent release, run the following command from the :file:`ncs/zephyr` repository (after running ``west update``):

.. code-block:: none

   git log --oneline 14f09a3b00 ^v2.6.0-rc1-ncs1

For a complete list of |NCS| specific commits, run:

.. code-block:: none

   git log --oneline manifest-rev ^14f09a3b00

The current |NCS| master branch is based on the Zephyr v2.7 development branch.

Matter (Project CHIP)
=====================

The Matter fork in the |NCS| (``sdk-connectedhomeip``) contains all commits from the upstream Matter repository up to, and including, ``9012f08de9b7340e7d59d51a7ec8a6cdcfda9d15``.

The following list summarizes the most important changes inherited from the upstream Matter:

* Added:

  * Support for Administrator Commissioning Cluster, which allows enabling or disabling the commissioning window on a Matter device.
    This is required by the Matter multi-admin functionality.

Partition Manager
=================

* Added the ``share_size`` functionality to let a partition share size with a partition in another region.

Secure Partition Manager
========================

* All EGU peripherals, instead of just EGU1 and EGU2, are now configurable to be non-secure and are configured as non-secure by default.

Documentation
=============

There are no entries for this section yet.
