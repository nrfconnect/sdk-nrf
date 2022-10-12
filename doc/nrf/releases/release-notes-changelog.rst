.. _ncs_release_notes_changelog:

Changelog for |NCS| v2.1.99
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
See `known issues for nRF Connect SDK v2.1.0`_ for the list of issues valid for the latest release.

Changelog
*********

The following sections provide detailed lists of changes by component.

Application development
=======================

|no_changes_yet_note|

Protocols
=========

This section provides detailed lists of changes by :ref:`protocol <protocols>`.
See `Samples`_ for lists of changes for the protocol-related samples.

Bluetooth LE
------------

|no_changes_yet_note|

For details, see :ref:`nrfxlib:softdevice_controller_changelog`.

Bluetooth mesh
--------------

|no_changes_yet_note|

See `Bluetooth mesh samples`_ for the list of changes for the Bluetooth mesh samples.

Matter
------

|no_changes_yet_note|

See `Matter samples`_ for the list of changes for the Matter samples.

Matter fork
+++++++++++

The Matter fork in the |NCS| (``sdk-connectedhomeip``) contains all commits from the upstream Matter repository up to, and including, ``708685f4821df2aa0304f02db2773c429ad25eb8``.

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

|no_changes_yet_note|

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

|no_changes_yet_note|

nRF9160: Serial LTE modem
-------------------------

* Added:

  * Added optional data modem flow control option :ref:`CONFIG_SLM_DATAMODE_URC <CONFIG_SLM_DATAMODE_URC>`.

* Updated:

  * Removed automatic quit of data mode in GNSS, FTP and HTTP services.

nRF5340 Audio
-------------

* Added:

  * Kconfig options for different sample rates and BAP presets.
  * Added Kconfig options for different sample rates and BAP presets.
  * Added bidirectional mode for the CIS mode.

* Updated:

  * LE Audio Controller Subsystem for nRF53 from version 3303 to version 3307.
  * Removed support for the nRF5340 Audio DK (PCA10121) board version 0.7.1 or older

* Fixed:

  * An issue with the figure for :ref:`nrf53_audio_app_overview_architecture_i2s` in the :ref:`nrf53_audio_app` documentation.
    The figure now correctly shows the interaction with the Bluetooth modules.
  * SMP is not advertising in CIS mode.
  * MCUMGR command can't receive in BIS mode.

nRF Machine Learning (Edge Impulse)
-----------------------------------

* Thingy:52 (``thingy52_nrf52832``) is no longer supported.

nRF Desktop
-----------

|no_changes_yet_note|

Thingy:53 Zigbee weather station
--------------------------------

|no_changes_yet_note|

Connectivity Bridge
-------------------

|no_changes_yet_note|

Samples
=======

This section provides detailed lists of changes by :ref:`sample <sample>`, including protocol-related samples.
For lists of protocol-specific changes, see `Protocols`_.

Bluetooth samples
-----------------

* :ref:`ble_throughput` sample:

  * Added terminal commands for selecting the role.
  * Updated the ASCII art used for showing progress to feature the current Nordic Semiconductor logo.

Bluetooth mesh samples
----------------------

* :ref:`bluetooth_mesh_sensor_server` sample:

  * Added:

    * Ability to limit the reported temperatures based on :c:var:`bt_mesh_sensor_dev_op_temp_range_spec` as a setting for the :c:var:`bt_mesh_sensor_present_dev_op_temp` sensor type.
    * Ability to persistently store the above-mentioned setting.
    * A sensor descriptor of the temperature sensor.

* :ref:`bluetooth_mesh_sensor_client` sample:

  * Added:

    * Ability to use buttons to send get and set messages for a sensor setting, as well as a get message for a sensor descriptor.

nRF9160 samples
---------------

* :ref:`lwm2m_client` sample:

  * Added:

    * Ability to use buttons to generate location assistance requests.
    * Documentation on :ref:`lwmwm_client_testing_shell`.

* :ref:`nrf_cloud_rest_cell_pos_sample` sample:

  * Updated:

   * Sample moved from :file:`samples/nrf9160/nrf_cloud_rest_cell_pos` to :file:`samples/nrf9160/nrf_cloud_rest_cell_location`.

Thread samples
--------------

* :ref:`ot_cli_sample` sample:

  * Removed:

    * Thread Certification support files in favor of regular sample overlays.

Matter samples
--------------

* :ref:`matter_light_bulb_sample`:

  * Introduced support for Matter over Wi-Fi on standalone ``nrf7002dk_nrf5340_cpuapp`` and on ``nrf5340dk_nrf5340_cpuapp`` with the ``nrf7002_ek`` shield attached.

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

* Added:

  * :ref:`wifi_radio_test` sample with the radio test support and :ref:`subcommands for FICR/OTP programming <wifi_ficr_prog>`.
  * :ref:`wifi_scan_sample` sample that demonstrates how to scan for the access points.
  * :ref:`wifi_station_sample` sample that demonstrates how to connect the Wi-Fi station to a specified access point.

Other samples
-------------

|no_changes_yet_note|

Drivers
=======

This section provides detailed lists of changes by :ref:`driver <drivers>`.

* |no_changes_yet_note|

Libraries
=========

This section provides detailed lists of changes by :ref:`library <libraries>`.

Binary libraries
----------------

|no_changes_yet_note|

Bluetooth libraries and services
--------------------------------

* :ref:`bt_le_adv_prov_readme`:

  * Added the :kconfig:option:`CONFIG_BT_ADV_PROV_TX_POWER_CORRECTION_VAL` option to TX power advertising data provider (:kconfig:option:`CONFIG_BT_ADV_PROV_TX_POWER`).
    The option adds a predefined value to the TX power, that is included in the advertising data.
  * Changed :c:member:`bt_le_adv_prov_adv_state.bond_cnt` to :c:member:`bt_le_adv_prov_adv_state.pairing_mode`.
    The information about whether the advertising device is looking for a new peer is more meaningful for the Bluetooth LE data providers.

* :ref:`bt_mesh` library:

  * :ref:`bt_mesh_dk_prov` module: Changed the UUID generation to prevent trailing zeros in the UUID.

    Migration note: To retain the legacy generation of UUID, enable the option :kconfig:option:`CONFIG_BT_MESH_DK_LEGACY_UUID_GEN`.

See `Bluetooth mesh samples`_ for the list of changes for the Bluetooth mesh samples.

* :ref:`nrf_bt_scan_readme`:

  * Added the ability to use the module when the Bluetooth Observer role is enabled.

* :ref:`bt_fast_pair_readme` service:

  * Disabled automatic security re-establishment request as a peripheral (:kconfig:option:`CONFIG_BT_GATT_AUTO_SEC_REQ`) to allow the Fast Pair Seeker to control the security re-establishment.

Bootloader libraries
--------------------

|no_changes_yet_note|

Modem libraries
---------------

* :ref:`modem_info_readme` library:

  * Removed:

    * :c:func:`modem_info_json_string_encode` and :c:func:`modem_info_json_object_encode` functions.
    * network_mode field from :c:struct:`network_param`.
    * ``MODEM_INFO_NETWORK_MODE_MAX_SIZE``.
    * ``CONFIG_MODEM_INFO_ADD_BOARD``.

* :ref:`nrf_modem_lib_readme` library:

  * Updated:

    * The :c:func:`getaddrinfo` function to return ``EAFNOSUPPORT`` instead of ``EPROTONOSUPPORT`` when socket family is not supported.
    * The :c:func:`bind` function to return ``EAFNOSUPPORT`` instead of ``ENOTSUP`` when socket family is not supported.
    * The :c:func:`sendto` function to return ``EAFNOSUPPORT`` instead of ``ENOTSUP`` when socket family is not supported.
    * The :c:func:`connect` function to not override the error codes set by the Modem library when called with raw parameters (non-IP).

  * Fixed:

    * An issue where the :c:func:`getsockopt` function causes segmentation fault when the ``optlen`` parameter is provided as ``NULL``.
    * An issue where the :c:func:`recvfrom` function causes segmentation fault when the ``from`` and ``fromlen`` parameters are provided as ``NULL``.

* :ref:`lib_location` library:

  * Added timeout for the entire location request.

Libraries for networking
------------------------

* :ref:`lib_multicell_location` library:

  * Removed the Kconfig option :kconfig:option:`CONFIG_MULTICELL_LOCATION_MAX_NEIGHBORS`.
    The maximum number of supported neighbor cell measurements for HERE location services depends on the :kconfig:option:`CONFIG_LTE_NEIGHBOR_CELLS_MAX` Kconfig option.

* :ref:`lib_download_client` library:

  * Updated the library so that it does not retry download on disconnect.
  * Fixed a race condition when starting the download.

* :ref:`lib_nrf_cloud` library:

  * Updated:

    * The stack size of the MQTT connection monitoring thread can now be adjusted by setting the :kconfig:option:`CONFIG_NRF_CLOUD_CONNECTION_POLL_THREAD_STACK_SIZE` Kconfig option.

  * Removed:

    * An unused parameter of the :c:func:`nrf_cloud_connect` function.

* :ref:`lib_lwm2m_client_utils` library:

  * Added support for using X509 certificates.


Libraries for NFC
-----------------

|no_changes_yet_note|

Other libraries
---------------

* Added:
  * :ref:`lib_sfloat` library.

Common Application Framework (CAF)
----------------------------------

* :ref:`caf_sensor_manager`:

  * Added:

    * Dynamic control for the :ref:`sensor sample period<sensor_sample_period>`.
    * Test for the sample.

Shell libraries
---------------

|no_changes_yet_note|

Libraries for Zigbee
--------------------

|no_changes_yet_note|

sdk-nrfxlib
-----------

See the changelog for each library in the :doc:`nrfxlib documentation <nrfxlib:README>` for additional information.

Scripts
=======

This section provides detailed lists of changes by :ref:`script <scripts>`.

* :ref:`west_sbom`:

  * SPDX License List database updated to version 3.18.

MCUboot
=======

The MCUboot fork in |NCS| (``sdk-mcuboot``) contains all commits from the upstream MCUboot repository up to and including ``1d4404116a9a6b54d54ea9aa3dd2575286e666cd``, plus some |NCS| specific additions.

The code for integrating MCUboot into |NCS| is located in the :file:`ncs/nrf/modules/mcuboot` folder.

The following list summarizes both the main changes inherited from upstream MCUboot and the main changes applied to the |NCS| specific additions:

* |no_changes_yet_note|

Zephyr
======

.. NOTE TO MAINTAINERS: All the Zephyr commits in the below git commands must be handled specially after each upmerge and each NCS release.

The Zephyr fork in |NCS| (``sdk-zephyr``) contains all commits from the upstream Zephyr repository up to and including ``71ef669ea4a73495b255f27024bcd5d542bf038c``, plus some |NCS| specific additions.

For the list of upstream Zephyr commits (not including cherry-picked commits) incorporated into nRF Connect SDK since the most recent release, run the following command from the :file:`ncs/zephyr` repository (after running ``west update``):

.. code-block:: none

   git log --oneline 71ef669ea4 ^45ef0d2

For the list of |NCS| specific commits, including commits cherry-picked from upstream, run:

.. code-block:: none

   git log --oneline manifest-rev ^71ef669ea4

The current |NCS| main branch is based on revision ``71ef669ea4`` of Zephyr.

Additions specific to |NCS|
---------------------------

* |no_changes_yet_note|

zcbor
=====

* |no_changes_yet_note|

Trusted Firmware-M
==================

* |no_changes_yet_note|

cJSON
=====

* |no_changes_yet_note|

Documentation
=============

* Added:

  * :ref:`app_memory`: Configuration options affecting memory footprint for Bluetooth mesh, that can be used to optimize the application.
  * Documentation for the :ref:`lib_bh1749`.

* Updated:

  * :ref:`gs_assistant` steps to reflect the fact that the |nRFVSC| is the default recommended IDE.
  * :ref:`ug_matter_gs_adding_cluster` documentation with new code snippets to align it with the source code of refactored Matter template sample.

.. |no_changes_yet_note| replace:: No changes since the latest |NCS| release.
