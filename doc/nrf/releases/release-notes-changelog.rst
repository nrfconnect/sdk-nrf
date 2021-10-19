.. _ncs_release_notes_changelog:

Changelog for |NCS| v1.7.99
###########################

.. contents::
   :local:
   :depth: 2

The most relevant changes that are present on the main branch of the |NCS|, as compared to the latest official release, are tracked in this file.

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

Application development
=======================

* Build system:

  * Fixed the NCSIDB-581 bug where application signing and file conversion for Device Firmware Update (DFU) could fail in SEGGER Embedded Studio during a build.

Protocols
=========

This section provides detailed lists of changes by :ref:`protocol <protocols>`.
See `Samples`_ for lists of changes for the protocol-related samples.

* No entries yet.

Applications
============

This section provides detailed lists of changes by :ref:`application <applications>`.

nRF9160: Asset Tracker
----------------------

* Updated the application to start sending batch messages to the new bulk endpoint topic supported in nRF Cloud.
* Updated the application to use nRF Cloud A-GPS directly without the A-GPS library. SUPL is no longer supported.

nRF Machine Learning (Edge Impulse)
-----------------------------------

* Added non-secure configuration for building :ref:`nrf_machine_learning_app` with :ref:`zephyr:thingy53_nrf5340`.
* Added secure configuration for building :ref:`nrf_machine_learning_app` with :ref:`zephyr:nrf5340dk_nrf5340`.
* Added power manager to the :ref:`nrf_machine_learning_app` application.
* Updated information about custom build types.

nRF Desktop
-----------

* Added:

  * Added documentation for :ref:`nrf_desktop_usb_state_pm`.
  * Added :ref:`nrf_desktop_ble_state_pm`.

* Removed:

  * Removed configuration files used for building the application with :kconfig:`CONFIG_BT_LL_SW_SPLIT` for various boards.
    The configuration files for boards that do not have room for the SoftDevice LL in flash or SRAM remain untouched.

* Updated:

  * Updated information about custom build types.
  * Updated documentation for :ref:`nrf_desktop_usb_state`.
  * Updated documentation with information about forwarding boot reports.
    See the documenation page of nRF Desktop's :ref:`nrf_desktop_hid_forward` for details.
  * Fixed an issue that was causing the HID keyboard LEDs to remain turned on after host disconnection while no other hosts were connected.

nRF9160: Serial LTE modem
-------------------------

* Added new AT commands related to the General Purpose Input/Output (GPIO).
* Added the ``#XUUID`` command to read out the device UUID from the modem.
* Added to the ``XNRFCLOUD`` command the following features:

  * The possibility to send to and receive from nRF Cloud JSON messages in data mode.
  * The ability to read out the ``sec_tag`` and the UUID of the device.

Samples
=======

This section provides detailed lists of changes by :ref:`sample <sample>`, including protocol-related samples.
For lists of protocol-specific changes, see `Protocols`_.

Bluetooth samples
-----------------

* Updated some samples with support for :ref:`zephyr:thingy53_nrf5340` in non-secure configuration.
* :ref:`ble_llpm` sample - Added role selection.
  The user now selects the role for each board by typing "m" or "s" in the terminal emulator.

Bluetooth mesh samples
----------------------

* Added:

  * :ref:`bluetooth_ble_peripheral_lbs_coex` sample, demonstrating how to combine Bluetooth mesh and Bluetooth Low Energy features in a single application.

* Updated:

  * Updated some samples with support for :ref:`zephyr:thingy53_nrf5340` in non-secure configuration.

nRF9160 samples
---------------

* :ref:`https_client` sample:

  * Added a possibility to use TF-M and Zephyr Mbed TLS instead of using the offloaded TLS stack in modem.

* :ref:`lwm2m_client` sample:

  * Added support for Thingy:91.
  * Added more LwM2M objects.
  * LwM2M sensor objects now uses the actual sensors available to the Thingy:91. If the nRF9160 DK is used, it uses simulated sensors instead.
  * Added support for polling sensors and notifying the server if the measured changes are large enough.
  * Added support for full modem firmware update.
  * Increased the NB-IoT time (in seconds) before the registration timeout when the LwM2M Registration Update message is sent by the engine.

Matter samples
--------------

* Added:

  * Multi-image Device Firmware Upgrade over Bluetooth LE support for nRF5340 DK in lock and light bulb samples.
  * Low-power build support in :ref:`Matter door lock <matter_lock_sample>`.

Zigbee samples
--------------

* Added:

   * :ref:`Zigbee shell <zigbee_shell_sample>` sample.

* Updated:

   * Fixed issue with cluster declaration in :ref:`Zigbee shell <zigbee_shell_sample>` sample and :ref:`Zigbee template <zigbee_template_sample>` sample.

Other samples
-------------

* :ref:`bootloader` sample:

  * Improved how hardware unique keys are handled.

    * Introduced :kconfig:`CONFIG_HW_UNIQUE_KEY_LOAD` with fewer dependencies than :kconfig:`CONFIG_HW_UNIQUE_KEY` solely for loading the key.
    * The bootloader now allows a single boot with no key present, to allow the app to write a key.
      After the first boot, the key must be present or the bootloader won't boot the app.

* Added the :ref:`hw_unique_key_usage` sample.

Drivers
=======

This section provides detailed lists of changes by :ref:`driver <drivers>`.

* Added API documentation and :ref:`conceptual documentation page <sensor_sim>` for the simulated sensor driver.
* Added API documentation and :ref:`conceptual documentation page <paw3212>` for the PAW3212 motion sensor driver.

Libraries
=========

This section provides detailed lists of changes by :ref:`library <libraries>`.

Common Application Framework (CAF)
----------------------------------

Added:

* :ref:`caf_preview_sample` sample.
* :ref:`caf_ble_state_pm` CAF module.
* :ref:`caf_buttons_pm_keep_alive`.

Updated:

* :ref:`caf_power_manager` documentation page with the state transition diagram.
* The power management support in modules is now enabled by default when the :kconfig:`CONFIG_CAF_PM_EVENTS` Kconfig option is enabled.
* The :ref:`caf_power_manager` now has a dependency on :kconfig:`CONFIG_PM_POLICY_APP`, which is required by the application that is using the :ref:`caf_power_manager` to link.

Modem libraries
---------------

* :ref:`lte_lc_readme` library:

  * Changed the value of an invalid E-UTRAN cell ID from zero to UINT32_MAX for the LTE_LC_EVT_NEIGHBOR_CELL_MEAS event.
  * Added support for multiple LTE event handlers. Thus, deregistration is not possible by using lte_lc_register_handler(NULL) anymore and it is done by the :c:func:`lte_lc_deregister_handler` function in the API.
  * Added neighbor cell measurement search type parameter in :c:func:`lte_lc_neighbor_cell_measurement`.
  * Added timing advance measurement time to current cell data in :c:enum:`LTE_LC_EVT_NEIGHBOR_CELL_MEAS` event.

* :ref:`nrf_modem_lib_readme` library:

  * Added a possibility to create native sockets when nRF91 socket offloading is enabled.

* :ref:`pdn_readme` library:

  * Added an optional ``family`` parameter to :c:func:`pdn_activate`, which is used to report when the IP family of a PDN changes after activation.

Libraries for networking
------------------------

* :ref:`lib_lwm2m_client_utils` library:

  * Added support for Firmware Update object to use :ref:`lib_fota_download` library for downloading firmware images.
  * Added support for full modem firmware update.

* :ref:`multicell_location` library:

  * Updated to only request neighbor cell measurements when connected and to only copy neighbor cell measurements if they exist.
  * Added support for Polte location service.
  * Removed device ID from the :c:func:`multicell_location_get` parameter list. nRF Cloud and HERE did not use it. Skyhook will now set modem UUID as its device ID.

* :ref:`lib_nrf_cloud` library:

  * Removed GNSS socket API support from A-GPS and P-GPS.
  * Added support for sending data to a new bulk endpoint topic that is supported in nRF Cloud.
    A message published to the bulk topic is typically a combination of multiple messages.
  * Changed REST API for A-GPS to use GNSS interface structure instead of GPS driver structure.
    Also changed from GPS driver ``GPS_AGPS_`` request types to ``NRF_CLOUD_AGPS_`` request types.
  * Added function :c:func:`nrf_cloud_jwt_generate` that generates a JWT using the :ref:`lib_nrf_cloud` library's configured values.

* :ref:`lib_nrf_cloud_agps` library:

  * Removed GNSS socket API support.

* :ref:`lib_nrf_cloud_pgps` library:

  * Fixed an issue with :kconfig:`CONFIG_NRF_CLOUD_PGPS_TRANSPORT_NONE` to ensure predictions are properly stored.
  * Fixed error handling associated with :kconfig:`CONFIG_NRF_CLOUD_PGPS_TRANSPORT_NONE`.
  * Added :c:func:`nrf_cloud_pgps_request_reset` so P-GPS application request handler can indicate failure to process the request.
    This ensures the P-GPS library tries the request again.
  * Added :kconfig:`CONFIG_NRF_CLOUD_PGPS_SOCKET_RETRIES`.
  * Changed :c:func:`nrf_cloud_pgps_init` to limit allowable :kconfig:`CONFIG_NRF_CLOUD_PGPS_NUM_PREDICTIONS` to an even number,
    and limited :kconfig:`CONFIG_NRF_CLOUD_PGPS_REPLACEMENT_THRESHOLD` to this value minus 2.

* :ref:`lib_rest_client` library:

  * Added REST client library for sending REST requests and receiving their responses.

Trusted Firmware-M libraries
----------------------------

* Added:

  * Support for non-secure storage.
    This enables non-secure applications to use the Zephyr Settings API to save and load persistent data.

Other libraries
---------------

* Added API documentation and :ref:`conceptual documentation page <wave_gen>` for the wave generator library.

* :ref:`event_manager` library:

  * Increased number of supported Event Manager events.

* :ref:`fprotect_readme` library:

  * Added a new function ``fprotect_is_protected()`` for devices with the ACL peripheral.

* :ref:`lib_hw_unique_key` library:

  * Make the checking for ``hw_unique_key_write_random()`` more strict; panic if any key is unwritten after writing random keys.
  * Refactored the ``HUK_HAS_*`` macros to be defined/undefined instead of 1/0.
  * Added a new sample :ref:`hw_unique_key_usage` showing how to use a hardware unique key to derive an encryption key.
    The sample can be run with or without TF-M.
  * Fixed ``hw_unique_key_is_written()`` which would previously trigger a fault under certain circumstances.

* :ref:`profiler` library:

  * Updated Python scripts to use multiple processes that communicate over sockets.
  * Increase the number of supported profiler events.

* :ref:`lib_spm`:

  * : Fixed the NCSDK-5156 issue with the size calculation for the non-secure callable region, which prevented users from adding a large number of custom secure services.
  * All EGU peripherals, instead of just EGU1 and EGU2, are now configurable to be non-secure and are configured as non-secure by default.

Libraries for Zigbee
--------------------

* Added ZCL commands to the :ref:`Zigbee shell <lib_zigbee_shell>` library.
* Fixes and improvements in :ref:`Zigbee Shell  <lib_zigbee_shell>` library.
* Added :ref:`BDB command for printing install codes <bdb_ic_list>` to the :ref:`Zigbee shell <lib_zigbee_shell>` library.
* Improve logging in :ref:`ZBOSS OSIF <lib_zigbee_osif>` library and :ref:`Zigbee Shell <lib_zigbee_shell>` library.

Scripts
=======

This section provides detailed lists of changes by :ref:`script <scripts>`.

Partition Manager
-----------------

* Partition manager information is no longer appended to the ``rom_report`` target.
  To inspect the current partition manager configuration please use the ``partition_manager_report`` target.
* Added the ``share_size`` functionality to let a partition share size with a partition in another region.

MCUboot
=======

The MCUboot fork in |NCS| (``sdk-mcuboot``) contains all commits from the upstream MCUboot repository up to and including ``680ed07``, plus some |NCS| specific additions.

The code for integrating MCUboot into |NCS| is located in :file:`ncs/nrf/modules/mcuboot`.

The following list summarizes the most important changes inherited from upstream MCUboot:

* The value of the :kconfig:`CONFIG_PM_PARTITION_SIZE_MCUBOOT_SECONDARY` Kconfig option does not have to be specified manually as it automatically shares the value with the primary partition.

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

The current |NCS| main branch is based on the Zephyr v2.7 development branch.

Matter (Project CHIP)
=====================

The Matter fork in the |NCS| (``sdk-connectedhomeip``) contains all commits from the upstream Matter repository up to, and including, ``9012f08de9b7340e7d59d51a7ec8a6cdcfda9d15``.

The following list summarizes the most important changes inherited from the upstream Matter:

* Added:

  * Support for Administrator Commissioning Cluster, which allows enabling or disabling the commissioning window on a Matter device.
    This is required by the Matter multi-admin functionality.

Documentation
=============

In addition to documentation related to the changes listed above, the following documentation has been updated:

* General changes:

  * Modified section names on this page.
    Now the section names better match the |NCS| code and documentation structure.
  * :ref:`ncs_introduction`:

    * Added a section describing how licenses work in |NCS|.
    * Added a section describing the Git tool.
    * Expanded the existing section about the West tool.

  * :ref:`glossary` - Added new terms related to :ref:`ug_matter` and :ref:`ug_zigbee`.
  * :ref:`library_template` - added a template for documenting libraries.

* Libraries:

  * Added the documentation page for :ref:`lib_fatal_error`.
