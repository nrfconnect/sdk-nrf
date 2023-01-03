.. _ncs_release_notes_changelog:

Changelog for |NCS| v2.2.99
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

Known issues
************

Known issues are only tracked for the latest official release.
See `known issues for nRF Connect SDK v2.2.0`_ for the list of issues valid for the latest release.

Changelog
*********

The following sections provide detailed lists of changes by component.

MCUboot
=======

|no_changes_yet_note|

Application development
=======================

|no_changes_yet_note|

RF Front-End Modules
--------------------

|no_changes_yet_note|

Build system
------------

|no_changes_yet_note|

Protocols
=========

This section provides detailed lists of changes by :ref:`protocol <protocols>`.
See `Samples`_ for lists of changes for the protocol-related samples.

Bluetooth LE
------------

|no_changes_yet_note|

Bluetooth mesh
--------------

|no_changes_yet_note|

Matter
------

* Added:

  * Support for Wi-Fi Network Diagnostic Cluster (which counts the number of packets received and transmitted on the Wi-Fi interface).
  * Default support for nRF7002 revision B.

* Fixed:

  * Connection timing out when attaching to a Wi-Fi Access Point that requires Wi-Fi Protected Access 3 (WPA3).

* Updated:

  * Default heap implementation to use Zephyr's ``sys_heap`` (:kconfig:option:`CONFIG_CHIP_MALLOC_SYS_HEAP`) to better control the RAM usage of Matter applications.

See `Matter samples`_ for the list of changes for the Matter samples.

Matter fork
+++++++++++

The Matter fork in the |NCS| (``sdk-connectedhomeip``) contains all commits from the upstream Matter repository up to, and including, ``bc6b43882a56ddb3e94d3e64956bd5f3292b4058``.

The following list summarizes the most important changes inherited from the upstream Matter:

* |no_changes_yet_note|

Thread
------

|no_changes_yet_note|

See `Thread samples`_ for the list of changes for the Thread samples.

Zigbee
------

|no_changes_yet_note|

See `Zigbee samples`_ for the list of changes for the Zigbee samples.

ESB
---

  * Added support for front-end modules.
  * The ESB module requires linking the :ref:`MPSL library <nrfxlib:mpsl_lib>`.
  * The number of PPI/DPPI channels used is increased from 3 to 6.
  * Assigned events 6-7 from the EGU0 instance to the ESB module.
  * Changed the type parameter of the function :c:func:`esb_set_tx_power` to ``int8_t``.

nRF IEEE 802.15.4 radio driver
------------------------------

|no_changes_yet_note|

Wi-Fi
-----

|no_changes_yet_note|

Applications
============

This section provides detailed lists of changes by :ref:`application <applications>`.

nRF9160: Asset Tracker v2
-------------------------

* Added:

  * Wi-Fi support for nRF9160 DK + nRF7002 EK configuration.

nRF9160: Serial LTE modem
-------------------------

* Added an RFC1350 TFTP client, currently supporting only *READ REQUEST*.

nRF5340 Audio
-------------

* Added:

  * Support for Front End Module nRF21540.

* Updated:

  * Power module has been re-factored so that it uses upstream Zephyr INA23X sensor driver.
  * BIS headsets can now switch between two broadcast sources (two hardcoded broadcast names).

nRF Desktop
-----------

* Changed:

   * Implemented adjustments to avoid flooding logs:

      * Set the max compiled-in log level to ``warning`` for the Non-Volatile Storage (:kconfig:option:`CONFIG_NVS_LOG_LEVEL`).
      * Lowered a log level to ``debug`` for the ``Identity x created`` log in the :ref:`nrf_desktop_ble_bond`.

Samples
=======

Bluetooth samples
-----------------

* :ref:`peripheral_uart` sample:

  * Changed:

    * Fixed a possible memory leak in the :c:func:`uart_init` function.

* :ref:`peripheral_hids_keyboard` sample:

  * Changed:

    * Fixed a possible out-of-bounds memory access issue in the :c:func:`hid_kbd_state_key_set` and :c:func:`hid_kbd_state_key_clear` functions.

* :ref: `ble_nrf_dm` sample:

  * Added:

    * Support for high-precision distance estimate using more compute-intensive algorithms.

  * Changed:

    * Added energy consumption information to documentation.
    * Added a documentation section about distance offset calibration.

Bluetooth mesh samples
----------------------

|no_changes_yet_note|

nRF9160 samples
---------------

* :ref:`modem_shell_application` sample:

  * Updated:

    * Timeout command-line arguments for the ``location get`` command changed from integers in milliseconds to floating-point values in seconds.

* :ref:`nrf_cloud_rest_cell_pos_sample` sample:

  * Added:

    * Usage of GCI search option if running modem firmware 1.3.4.

  * Updated:

    * The sample now waits for RRC idle mode before requesting neighbor cell measurements.

* :ref:`lwm2m_client` sample:

  * Added:

    * Support for nRF7002 EK shield and Wi-Fi based location.
    * Location events and event handlers.

  * Updated:

    * The sensor module has been simplified.
      It does not use application events, filtering, or configurable periods anymore.


Peripheral samples
------------------

* :ref:`radio_test` sample:

  * Added support for the nRF7002 board.
  * Fixed sample building with support for the Skyworks front-end module.

Trusted Firmware-M (TF-M) samples
---------------------------------

|no_changes_yet_note|

Thread samples
--------------

* Changed:

  * Overlay structure changed:
    * ``overlay-rtt.conf`` removed from all samples.
    * ``overlay-log.conf`` now uses RTT backend by default.
    * Logs removed from default configuration (moved to ``overlay-logging.conf``)
    * Asserts removed from default configuration (moved to ``overlay-debug.conf``)

Matter samples
--------------

* :ref:`matter_lock_sample`:

  * Added `thread_wifi_switched` build type that enables switching between Thread and Wi-Fi network support in the field.

NFC samples
-----------

|no_changes_yet_note|

nRF5340 samples
---------------

|no_changes_yet_note|

Gazell samples
--------------

|no_changes_yet_note|

Zigbee samples
--------------

|no_changes_yet_note|

Wi-Fi samples
-------------

|no_changes_yet_note|

Other samples
-------------

* :ref:`esb_prx_ptx` sample:

  * Added:

    * Support for front-end modules and :ref:`zephyr:nrf21540dk_nrf52840`.

Drivers
=======

This section provides detailed lists of changes by :ref:`driver <drivers>`.

* Reduced log verbosity of :ref:`pmw3360`.
* Reduced log verbosity of :ref:`paw3212`.

Libraries
=========

This section provides detailed lists of changes by :ref:`library <libraries>`.

Binary libraries
----------------

|no_changes_yet_note|

Bluetooth libraries and services
--------------------------------

* :ref:`mds_readme`:

  * Fixed URI generation in the :c:func:`data_uri_read` function.

* :ref:`ble_rpc` library:

  * Fixed a possible memory leak in the :c:func:`bt_gatt_indicate_rpc_handler` function.

* :ref:`bt_le_adv_prov_readme` library:

  * Changed the :kconfig:option:`CONFIG_BT_ADV_PROV_FAST_PAIR_BATTERY_DATA_MODE` Kconfig option (default value) to not include Fast Pair battery data in the Fast Pair advertising payload by default.

Bootloader libraries
--------------------

|no_changes_yet_note|

Modem libraries
---------------

* :ref:`lib_location` library:

  * Added:

    * Support for the application to send the Wi-Fi access point list to the cloud.

  * Updated:

    * Use of :ref:`lib_multicell_location` library has been removed because the library is deprecated.
      Relevant functionality from the library is moved to this library.
      The following features were not copied:

      * Definition of HTTPS port for HERE service, that is :kconfig:option:`CONFIG_MULTICELL_LOCATION_HERE_HTTPS_PORT`.
      * HERE v1 API.
      * nRF Cloud CA certificate handling.

    * Renamed ``enum location_cellular_ext_result`` to ``enum location_ext_result``, because Wi-Fi will use the same enumeration.
    * Renamed :kconfig:option:`CONFIG_LOCATION_METHOD_WIFI_SERVICE_NRF_CLOUD` to :kconfig:option:`CONFIG_LOCATION_SERVICE_NRF_CLOUD`.
    * Renamed :kconfig:option:`CONFIG_LOCATION_METHOD_WIFI_SERVICE_HERE` to :kconfig:option:`CONFIG_LOCATION_SERVICE_HERE`.
    * Renamed :kconfig:option:`CONFIG_LOCATION_METHOD_WIFI_SERVICE_HERE_API_KEY` to :kconfig:option:`CONFIG_LOCATION_SERVICE_HERE_API_KEY`.
    * Renamed :kconfig:option:`CONFIG_LOCATION_METHOD_WIFI_SERVICE_HERE_HOSTNAME` to :kconfig:option:`CONFIG_LOCATION_SERVICE_HERE_HOSTNAME`.
    * Renamed :kconfig:option:`CONFIG_LOCATION_METHOD_WIFI_SERVICE_HERE_TLS_SEC_TAG` to :kconfig:option:`CONFIG_LOCATION_SERVICE_HERE_TLS_SEC_TAG`.
    * Improved GNSS assistance data need handling.

Libraries for networking
------------------------

* :ref:`lib_fota_download` library:

  * Fixed a bug where the :c:func:`download_client_callback` function was continuing to read the offset value even if :c:func:`dfu_target_offset_get` returned an error.
  * Fixed a bug where the cleanup of the downloading state was not happening when an error event was raised.

* :ref:`lib_nrf_cloud` library:

  * Updated:

    * The MQTT disconnect event is now handled by the FOTA module, allowing for updates to be completed while disconnected and reported properly when reconnected.
    * GCI search results are now encoded in location requests.
    * The neighbor cell's time difference value is now encoded in location requests.

* :ref:`lib_nrf_cloud` library:

  * Fixed a bug where the same buffer was incorrectly shared between caching a P-GPS prediction and loading a new one, when external flash was used.

* :ref:`lib_lwm2m_location_assistance` library:

  * Added:

    * Support for Wi-Fi based location through LwM2M.
    * API for scanning Wi-Fi access points.

  * Removed:

    * Location events and event handlers.

Libraries for NFC
-----------------

* :ref:`lib_nfc`

  * Updated:

    * Added the possibility of moving an NFC callback to a thread context.
    * Added support for zero-latency interrupts for NFC.
    * Aligned the :file:`ncs/nrf/subsys/nfc/lib/platform.c` file with new library implementation.

* :ref:`lib_nfc_ndef`:

  * Fixed a write to a constant field in the :c:func:`ac_rec_payload_parse` function.

* :ref:`nfc_t4t_isodep_readme`:

  * Fixed out of bounds access in the :c:func:`ats_parse` function.

Other libraries
---------------

* :ref:`lib_contin_array` library:

  * Separated the library from the :ref:`nrf53_audio_app` and moved it to :file:`lib/contin_array`.
    Updated code and documentation accordingly.

* :ref:`lib_location` library:

  * Updated:

    * GNSS filtered ephemerides are no longer used when the :kconfig:option:`CONFIG_NRF_CLOUD_AGPS_FILTERED_RUNTIME` Kconfig option is enabled.

  * Fixed:

    * An issue causing the A-GPS data download to be delayed until the RRC connection release.

* Secure Partition Manager (SPM):

  * Removed Secure Partition Manager (SPM) and the Kconfig option ``CONFIG_SPM``.
    It is replaced by the Trusted Firmware-M (TF-M) as the supported trusted execution solution.
    See :ref:`Trusted Firmware-M (TF-M) <ug_tfm>` for more information about the TF-M.

* PCM Mix:

  * PCM mix (Pulse Code Modulation) audio mixer has been moved out of the nRF5340 Audio
    application, and into lib/pcm_mix.

Common Application Framework (CAF)
----------------------------------

|no_changes_yet_note|

Shell libraries
---------------

|no_changes_yet_note|

Libraries for Zigbee
--------------------

|no_changes_yet_note|

sdk-nrfxlib
-----------

See the changelog for each library in the :doc:`nrfxlib documentation <nrfxlib:README>` for additional information.

DFU libraries
-------------

|no_changes_yet_note|

Scripts
=======

This section provides detailed lists of changes by :ref:`script <scripts>`.

* :ref:`west_sbom`:

  * Output now contains source repository and version information for each file.

MCUboot
=======

The MCUboot fork in |NCS| (``sdk-mcuboot``) contains all commits from the upstream MCUboot repository up to and including ``cfec947e0f8be686d02c73104a3b1ad0b5dcf1e6``, with some |NCS| specific additions.

The code for integrating MCUboot into |NCS| is located in the :file:`ncs/nrf/modules/mcuboot` folder.

The following list summarizes both the main changes inherited from upstream MCUboot and the main changes applied to the |NCS| specific additions:

* |no_changes_yet_note|

Zephyr
======

.. NOTE TO MAINTAINERS: All the Zephyr commits in the below git commands must be handled specially after each upmerge and each NCS release.

The Zephyr fork in |NCS| (``sdk-zephyr``) contains all commits from the upstream Zephyr repository up to and including ``cd16a8388f71a6cce0cea871f75f6d4ac8f56da9``, with some |NCS| specific additions.

For the list of upstream Zephyr commits (not including cherry-picked commits) incorporated into nRF Connect SDK since the most recent release, run the following command from the :file:`ncs/zephyr` repository (after running ``west update``):

.. code-block:: none

   git log --oneline cd16a8388f ^71ef669ea4

For the list of |NCS| specific commits, including commits cherry-picked from upstream, run:

.. code-block:: none

   git log --oneline manifest-rev ^cd16a8388f

The current |NCS| main branch is based on revision ``cd16a8388f`` of Zephyr.

Additions specific to |NCS|
---------------------------

|no_changes_yet_note|

zcbor
=====

|no_changes_yet_note|

Trusted Firmware-M
==================

|no_changes_yet_note|

cJSON
=====

|no_changes_yet_note|

Documentation
=============

* Added:

  * Documentation template for the :ref:`Ecosystem integration <Ecosystem_integration>` user guides.

.. |no_changes_yet_note| replace:: No changes since the latest |NCS| release.
