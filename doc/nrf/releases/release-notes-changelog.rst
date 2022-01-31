.. _ncs_release_notes_changelog:

Changelog for |NCS| v1.8.99
###########################

.. contents::
   :local:
   :depth: 2

The most relevant changes that are present on the main branch of the |NCS|, as compared to the latest official release, are tracked in this file.

.. note::
   This file is a work in progress and might not cover all relevant changes.

.. HOWTO

   When adding a new PR, decide whether it needs an entry in the changelog.
   If it does, update this page.
   Add the sections you need, as only a handful of sections is kept when the changelog is cleaned.
   "Protocols" section serves as a highlight section for all protocol-related changes, including those made to samples, libraries, and so on.

Highlights
**********

* Removed support for Pelion DM library.

Known issues
************

Known issues are only tracked for the latest official release.
See `known issues for nRF Connect SDK v1.8.0`_ for the list of issues valid for the latest release.

Changelog
*********

The following sections provide detailed lists of changes by component.

Application development
=======================

Using Edge Impulse
------------------

* Added instruction on how to download a model from a public Edge Impulse project.

Protocols
=========

This section provides detailed lists of changes by :ref:`protocol <protocols>`.
See `Samples`_ for lists of changes for the protocol-related samples.

Matter
------

* Added ``EXPERIMENTAL`` select in Kconfig that informs that Matter support is experimental.

Thread
------

* Changed how Thread 1.2 is enabled, refer to :ref:`thread_ug_thread_specification_options` and :ref:`thread_ug_feature_sets` for more information.

Zigbee
------

* Updated ZBOSS Zigbee stack to version ``3.11.1.0+5.1.1``.
  See the :ref:`nrfxlib:zboss_changelog` in the nrfxlib documentation for detailed information.
* Added development ZBOSS stack library version based on the ZBOSS build v3.11.1.177+v5.1.1.
  This library version is dedicated for testing ZCL v8 features.
* Added ZBOSS libraries variant with ZBOSS Traces enabled.

Applications
============

This section provides detailed lists of changes by :ref:`application <applications>`.

nRF9160: Asset Tracker v2
-------------------------

* Updated the code and documentation to use the acronym GNSS instead of GPS when not referring explicitly to the GPS system.
* Added support for atmospheric pressure readings retrieved from the BME680 sensor on Thingy:91.
* Fixed an issue where PSM could be requested from the network even though it was disabled in Kconfig.
* Added new documentation for Asset Tracker v2 :ref:`asset_tracker_v2_modem_module`.
* Added support for A-GPS filtered ephemerides.
* Added new documentation for :ref:`asset_tracker_v2_util_module` and :ref:`api_modules_common`.
* Updated support for P-GPS preemptive updates and P-GPS coexistence with A-GPS.
* Added functionality that allows the application to wait for A-GPS data to be processed before starting GNSS positioning.

nRF9160: Asset Tracker
----------------------

* The application is removed.
  Hence, it is recommended to upgrade to the :ref:`asset_tracker_v2` application.

nRF Machine Learning (Edge Impulse)
-----------------------------------

* Added:

  * Added :kconfig:`CONFIG_ML_APP_SENSOR_EVENT_DESCR` option that globally defines sensor used by the application modules.
  * Bluetooth LE bonding functionality.
    The functionality relies on :ref:`caf_ble_bond`.

* Updated:

  * Renamed ``ml_state`` module to ``ml_app_mode`` module.

nRF Desktop
-----------

* Added:

  * Possibility to ask for bootloader variant using config channel.
  * Added Kconfig options that allow erasing dongle bond on the gaming mouse using buttons or config channel.
  * Added two states to enable erasing dongle peer: ``STATE_DONGLE_ERASE_PEER`` and ``STATE_DONGLE_ERASE_ADV``.
  * Added new application specific Kconfig option to enable :ref:`nrf_desktop_ble_bond`.
  * Added configuration for ``nrf52833dk_nrf52820`` to allow testing of nRF Desktop dongle on the nRF52820 SoC.

* Updated:

   * Documentation and diagrams for the :ref:`nrf_desktop_ble_bond`.
   * Moved Fn key related macros to an application specific header file (:file:`configuration/common/fn_key_id.h`).
   * Config channel no longer uses orphaned sections to store module Id information.
     Hence, the :kconfig:`CONFIG_LINKER_ORPHAN_SECTION_PLACE` option is no longer required in the config file.

Samples
=======

This section provides detailed lists of changes by :ref:`sample <sample>`, including protocol-related samples.
For lists of protocol-specific changes, see `Protocols`_.

Bluetooth samples
-----------------

* Added:

  * :ref:`multiple_adv_sets` sample.
  * :ref:`ble_nrf_dm` sample.

* Updated:

  * :ref:`peripheral_rscs` - Corrected the number of bytes for setting the Total Distance Value and specified the data units.
  * :ref:`direct_test_mode` - Added support for front-end module devices that support 2-pin PA/LNA interface with additional support for the Skyworks SKY66114-11 and the Skyworks SKY66403-11.
  * :ref:`peripheral_hids_mouse` - Added a notice about encryption requirement.
  * :ref:`peripheral_hids_keyboard` - Added a notice about encryption requirement.
  * :ref:`central_and_peripheral_hrs` - Clarified the "Requirements" section and fixed a link in the "Testing" section of the document.


nRF9160 samples
---------------

* :ref:`modem_shell_application` sample:

  * Added a new shell command ``cloud`` for establishing an MQTT connection to nRF Cloud.
  * Removed support for the GPS driver.
  * The LED 1 on the development kit indicates the LTE registration status.
  * Added a new shell command ``filtephem`` to enable or disable nRF Cloud A-GPS filtered ephemerides mode (REST only).
  * Various PPP updates. For example, started using Zephyr async PPP driver configuration to provide better throughput for dial-up.

* :ref:`http_application_update_sample` sample:

  * Added support for application downgrade.
    The sample now alternates updates between two different application images.

* :ref:`gnss_sample` sample:

  * Added support for minimal assistance using factory almanac, time and location.
  * Added support for TTFF test mode.
  * Added support for nRF Cloud A-GPS filtered mode.

* nRF9160: HTTP update samples:

  * HTTP update samples now set the modem in the power off mode after the firmware image download completes. This avoids ungraceful disconnects from the network upon pressing the reset button on the kit.

Thread samples
--------------

* :ref:`ot_cli_sample` sample:

  * Removed `overlay-thread_1_2.conf` as Thread 1.2 is now supported as described in :ref:`thread_ug_thread_specification_options`.

Other samples
-------------

:ref:`radio_test` - Added support for front-end module devices that support 2-pin PA/LNA interface with additional support for the Skyworks SKY66114-11 and the Skyworks SKY66403-11.

* :ref:`secure_partition_manager` sample:

  * Bug fixes:

    * NCSDK-12230: Fixed an issue where low baud rates would trigger a fault by selecting as system clock driver ``SYSTICK`` instead of ``RTC1``.


Drivers
=======

This section provides detailed lists of changes by :ref:`driver <drivers>`.

* |no_changes_yet_note|

Libraries
=========

This section provides detailed lists of changes by :ref:`library <libraries>`.

Bluetooth libraries and services
--------------------------------

* :ref:`rscs_readme` library:

  * Added units for :c:struct:`bt_rscs_measurement` members.

* :ref:`ble_rpc` library:

  * Fixed the issue related to missing buffer size variables for the user PHY update and the user data length update procedures.

Common Application Framework (CAF)
----------------------------------

* Added a simple implementation of the :ref:`caf_ble_bond`.
  The implementation allows to erase bonds for default Bluetooth local identity.
* Migrated :ref:`nRF Desktop settings loader <nrf_desktop_settings_loader>` to :ref:`lib_caf` as :ref:`CAF: Settings loader module <caf_settings_loader>`.
* :ref:`caf_leds`:

  * Added new LED effect macro: :c:macro:`LED_EFFECT_LED_BLINK2`.
  * Added a macro to pass color arguments between macro calls: :c:macro:`LED_COLOR_ARG_PASS`.
* Updated:

  * Unify module ID reference location.
    The array holding module reference objects is explicitly defined in linker script to avoid creating an orphan section.
    ``MODULE_ID`` macro and :c:func:`module_id_get` function now returns module reference from dedicated section instead of module name.
    The module name can not be obtained from reference object directly, a helper function (:c:func:`module_name_get`) should be used instead.
  * Fixed the NCSDK-13058 known issue related to directed advertising in CAF.
  * Fixed NULL dereferencing in :ref:`caf_sensor_manager` during :c:struct:`wake_up_event` handling for sensors that do not support trigger.

Bootloader libraries
--------------------

* Added a separate section for :ref:`lib_bootloader`.

Libraries for networking
------------------------
* :ref:`lib_fota_download` library:
  * Skipping host name check when connecting to TLS service using just IP address.

Modem libraries
---------------

* :ref:`nrf_modem_lib_readme` library:

  * Fixed a bug in the socket offloading component, where the :c:func:`recvfrom` wrapper could do an out-of-bounds copy of the sender's address, when the application is compiled without IPv6 support. In some cases, the out of bounds copy could indefinitely block the :c:func:`send` and other socket API calls.

* :ref:`at_monitor_readme` library:

  * Introduced AT_MONITOR_ISR macro to monitor AT notifications in an interrupt service routine.
  * Removed :c:func:`at_monitor_init` function and :kconfig:`CONFIG_AT_MONITOR_SYS_INIT` option. The library now initializes automatically when enabled.

* :ref:`at_cmd_parser_readme` library:

  * Can now parse AT command responses containing the response result, for example, ``OK`` or ``ERROR``.

* :ref:`nrf_modem_lib_readme`:

  * The modem trace handling is moved from :file:`nrf_modem_os.c` to a new file :file:`nrf_modem_lib_trace.c`, which also provides the API for starting a trace session for a given time interval or until a given size of trace data is received.

Event manager
-------------

* Added:

  * ``EVENT_SUBSCRIBE_FIRST`` subscriber priority.

* Updated:

  * Modified the sections used by the event manager. Stopped using orphaned sections. Removed forced alignment for x86. Reworked priorities.
  * Event manager no longer uses orphaned sections to store information about event types, listeners and subscribers.
    Hence, the :kconfig:`CONFIG_LINKER_ORPHAN_SECTION_PLACE` option is no longer required in the config file.

Event manager Profiler Tracer
-----------------------------

* Updated:

  * Event manager Profiler Tracer no longer use orphaned sections to store profiler information.
    Hence, the :kconfig:`CONFIG_LINKER_ORPHAN_SECTION_PLACE` option is no longer required in the config file.

Libraries for networking
------------------------

* :ref:`lib_fota_download` library:

  * Fixed an issue where the application would not be notified of errors originating from inside :c:func:`download_with_offset`. In the http_update samples, this would result in the dfu start button interrupt being disabled after a connect error in :c:func:`download_with_offset` after a disconnect during firmware download.
  * Fixed an issue that :ref:`lib_fota_download`, after a link-connection timeout, restarted the firmware image download from an incorrect offset.
    This resulted in an invalid image and required a restart of the firmware update process.
* :ref:`lib_dfu_target` library:

  * Updated the implementation of modem delta upgrades in the DFU target library to use the new socketless interface provided by the :ref:`nrf_modem`.

* :ref:`lib_location` library:

  * Added A-GPS filtered ephemerides support for both MQTT and REST transports.

* :ref:`lib_nrf_cloud` library:

  * Added A-GPS filtered ephemerides support, with ability to set matching threshold mask angle, for both MQTT and REST transports.
  * When filtered ephemerides is enabled, A-GPS assistance requests to cloud are limited to no more than once every two hours.
  * Updated MQTT connection error handling.
    Now, unacknowledged pings and other errors result in a transition to the disconnected state.
    This ensures that reconnection can take place.

* :ref:`lib_nrf_cloud_rest` library:

  * Updated to use the :ref:`lib_rest_client` library for REST API calls.

Other libraries
---------------

* Moved :ref:`lib_bootloader` to a section of their own.
  * Added write protection by default for the image partition.

* :ref:`lib_date_time` library:

  * Removed the :kconfig:`CONFIG_DATE_TIME_IPV6` Kconfig option.
    The library now automatically uses IPv6 for NTP when available.

* :ref:`lib_location` library:

  * Added support for GNSS high accuracy.

* :ref:`lib_ram_pwrdn` library:
  * Added functions for powering up and down RAM sections for a given address range.
  * Added experimental functionality to automatically power up and down RAM sections based on the libc heap usage.

Event Manager
+++++++++++++

* Modified sections used by the Event Manager and stopped using orphaned sections.
* Removed forced alignment for x86.
* Reworked priorities.

sdk-nrfxlib
-----------

See the changelog for each library in the :doc:`nfxlib documentation <nrfxlib:README>` for additional information.

Scripts
=======

This section provides detailed lists of changes by :ref:`script <scripts>`.

Unity
-----

* Fixed bug that resulted in the mocks for some functions not having the __wrap_ prefix. This happened for functions declared with whitespaces between identifier and parameter list.

HID Configurator
----------------

* Added:

  * HID Configurator now recognizes the bootloader variant as a DFU module variant for the configuration channel communication.
    The new implementation is backward compatible: the new version of the script checks for module name and acts accordingly.

MCUboot
=======

The MCUboot fork in |NCS| (``sdk-mcuboot``) contains all commits from the upstream MCUboot repository up to and including ``680ed07``, plus some |NCS| specific additions.

The code for integrating MCUboot into |NCS| is located in the :file:`ncs/nrf/modules/mcuboot` folder.

The following list summarizes both the main changes inherited from upstream MCUboot and the main changes applied to the |NCS| specific additions:

* |no_changes_yet_note|

Mcumgr
======

The mcumgr library contains all commits from the upstream mcumgr repository up to and including snapshot ``657deb65``.

The following list summarizes the most important changes inherited from upstream mcumgr:

* |no_changes_yet_note|

Zephyr
======

.. NOTE TO MAINTAINERS: All the Zephyr commits in the below git commands must be handled specially after each upmerge and each NCS release.

The Zephyr fork in |NCS| (``sdk-zephyr``) contains all commits from the upstream Zephyr repository up to and including ``3f82656``, plus some |NCS| specific additions.

For a complete list of upstream Zephyr commits incorporated into |NCS| since the most recent release, run the following command from the :file:`ncs/zephyr` repository (after running ``west update``):

.. code-block:: none

   git log --oneline 3f82656 ^v2.6.0-rc1-ncs1

For a complete list of |NCS| specific commits, run:

.. code-block:: none

   git log --oneline manifest-rev ^3f82656

The current |NCS| main branch is based on the Zephyr v2.7 development branch.

Matter (Project CHIP)
=====================

The Matter fork in the |NCS| (``sdk-connectedhomeip``) contains all commits from the upstream Matter repository up to, and including, ``77ab003e5fcd409cd225b68daa3cdaf506ed1107``.

The following list summarizes the most important changes inherited from the upstream Matter:

* Added:

  * Support for General Diagnostics, Software Diagnostics and Thread Network Diagnostics clusters.
  * Initial support for the Matter OTA (Over-the-air) Requestor role, used for updating the device firmware.

cddl-gen
========

The `cddl-gen`_ module has been updated from version 0.1.0 to 0.3.0.
Release notes for 0.3.0 can be found in :file:`ncs/nrf/modules/lib/cddl-gen/RELEASE_NOTES.md`.

The change prompted some changes in the CMake for the module, notably:

* The CMake function ``target_cddl_source()`` was removed.
* The non-generated source files (:file:`cbor_encode.c` and :file:`cbor_decode.c`) and their accompanying header files are now added to the build when :kconfig:`CONFIG_CDDL_GEN` is enabled.

Also, it prompted the following:

* The code of some libraries using cddl-gen (like :ref:`lib_fmfu_fdev`) has been regenerated.
* The sdk-nrf integration test has been expanded into three new tests.

Documentation
=============

In addition to documentation related to the changes listed above, the following documentation has been updated:

* Reorganized the contents of the :ref:`ug_app_dev` section:

  * Added new subpage :ref:`app_optimize` and moved the optimization sections under it.
  * Added new subpage :ref:`ext_components` and moved the sections for using external components or modules under it.

* Reorganized the contents of the :ref:`protocols` section:

  * Reduced the ToC levels of the subpages.

* Reorganized the contents of the :ref:`ug_radio_fem` section:

  * Added new section :ref: `ug_radio_fem_nrf21540_spi_gpio`.
  * Added new section :ref: `ug_radio_fem_direct_support`.
  * Added more information about supported protocols and hardware.

* Added the :ref:`ug_thingy91_gsg` guide and reworked the :ref:`ug_thingy91` user guide.

.. |no_changes_yet_note| replace:: No changes since the latest |NCS| release.
