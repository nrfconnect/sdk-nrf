.. _ncs_release_notes_changelog:

Changelog for |NCS| v2.6.99
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
See `known issues for nRF Connect SDK v2.6.0`_ for the list of issues valid for the latest release.

Changelog
*********

The following sections provide detailed lists of changes by component.

IDE and tool support
====================

|no_changes_yet_note|

Working with nRF91 Series
=========================

|no_changes_yet_note|

Working with nRF70 Series
=========================

|no_changes_yet_note|

Working with nRF52 Series
=========================

|no_changes_yet_note|

Working with nRF53 Series
=========================

|no_changes_yet_note|

Working with RF front-end modules
=================================

|no_changes_yet_note|

Protocols
=========

This section provides detailed lists of changes by :ref:`protocol <protocols>`.
See `Samples`_ for lists of changes for the protocol-related samples.

Bluetooth® LE
-------------

|no_changes_yet_note|

Bluetooth Mesh
--------------

|no_changes_yet_note|

Matter
------

|no_changes_yet_note|

Matter fork
+++++++++++

The Matter fork in the |NCS| (``sdk-connectedhomeip``) contains all commits from the upstream Matter repository up to, and including, the ``v1.2.0.1`` tag.

The following list summarizes the most important changes inherited from the upstream Matter:

|no_changes_yet_note|

Thread
------

|no_changes_yet_note|

Zigbee
------

|no_changes_yet_note|

Gazell
------

|no_changes_yet_note|

Enhanced ShockBurst (ESB)
-------------------------

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

Asset Tracker v2
----------------

* Updated:

  * The MQTT topic name for A-GNSS requests is changed to ``agnss`` for AWS and Azure backends.

Serial LTE modem
----------------

* Removed mention of Termite and Teraterm terminal emulators from the documentation.
  The recommended approach is to use one of the emulators listed on the :ref:`test_and_optimize` page.

nRF5340 Audio
-------------

|no_changes_yet_note|

nRF Machine Learning (Edge Impulse)
-----------------------------------

* Updated the ``ml_runner`` application module to allow running a machine learning model without anomaly support.

nRF Desktop
-----------

* Added:

  * Support for the nRF54L15 PDK with the ``nrf54l15pdk_nrf54l15_cpuapp`` board target.
    The PDK can act as a sample mouse or keyboard.
    It supports the Bluetooth LE HID data transport and uses SoftDevice Link Layer with Low Latency Packet Mode (LLPM) enabled.
  * The ``nrfdesktop-wheel-qdec`` DT alias support to :ref:`nrf_desktop_wheel`.
    You must use the alias to specify the QDEC instance used for scroll wheel, if your board supports multiple QDEC instances (for example ``nrf54l15pdk_nrf54l15_cpuapp``).
    You do not need to define the alias if your board supports only one QDEC instance, because in that case, the wheel module can rely on the ``qdec`` DT label provided by the board.

* Updated the :kconfig:option:`CONFIG_BT_ADV_PROV_TX_POWER_CORRECTION_VAL` Kconfig option value in the nRF52840 Gaming Mouse configurations with the Fast Pair support.
  The value is now aligned with the Fast Pair requirements.

Thingy:53: Matter weather station
---------------------------------

|no_changes_yet_note|

Matter Bridge
-------------

* Added:

   The :kconfig:option:`CONFIG_BRIDGE_BT_MAX_SCANNED_DEVICES` kconfig option to set the maximum number of scanned Bluetooth LE devices.
   The :kconfig:option:`CONFIG_BRIDGE_BT_SCAN_TIMEOUT_MS` kconfig option to set the scan timeout.

IPC radio firmware
------------------

|no_changes_yet_note|

Samples
=======

This section provides detailed lists of changes by :ref:`sample <samples>`.

Bluetooth samples
-----------------

* Added the :ref:`bluetooth_isochronous_time_synchronization` sample showcasing time-synchronized processing of isochronous data.

Bluetooth Mesh samples
----------------------

* :ref:`bluetooth_mesh_sensor_client` sample:

   * Added support for the :ref:`zephyr:nrf54l15pdk_nrf54l15` board.

* :ref:`bluetooth_mesh_sensor_server` sample:

   * Added support for the :ref:`zephyr:nrf54l15pdk_nrf54l15` board.

* :ref:`bt_mesh_chat` sample:

   * Added support for the :ref:`zephyr:nrf54l15pdk_nrf54l15` board.

* :ref:`bluetooth_mesh_light_switch` sample:

  * Added support for the :ref:`zephyr:nrf54l15pdk_nrf54l15` board.

Cellular samples
----------------

* :ref:`ciphersuites` sample:

  * Updated the :file:`.pem` certificate for example.com.

Cryptography samples
--------------------

* Added :ref:`crypto_spake2p` sample.

Debug samples
-------------

|no_changes_yet_note|

Edge Impulse samples
--------------------

|no_changes_yet_note|

Enhanced ShockBurst samples
---------------------------

|no_changes_yet_note|

Gazell samples
--------------

|no_changes_yet_note|

Keys samples
------------

|no_changes_yet_note|

Matter samples
--------------

* Removed:

  * The :file:`configuration` directory which contained the Partition Manager configuration file.
    It has been replaced replace with :file:`pm_static_<BOARD>` Partition Manager configuration files for all required target boards in the samples' directories.
  * The :file:`prj_no_dfu.conf` file.
  * Support for ``no_dfu`` build type for nRF5350 DK, nRF52840 DK and nRF7002 DK.

* Added test event triggers to all Matter samples.
  By utilizing the test event triggers, you can simulate various operational conditions and responses in your Matter device without the need for external setup.

  To get started with using test event triggers in your Matter samples and to understand the capabilities of this feature, refer to the :ref:`ug_matter_test_event_triggers` page.

Multicore samples
-----------------

|no_changes_yet_note|

Networking samples
------------------

* Updated:

  *  The networking samples to support import of certificates in valid PEM formats.

NFC samples
-----------

|no_changes_yet_note|

nRF5340 samples
---------------

|no_changes_yet_note|

Peripheral samples
------------------

|no_changes_yet_note|

PMIC samples
------------

|no_changes_yet_note|

Sensor samples
--------------

|no_changes_yet_note|

Trusted Firmware-M (TF-M) samples
---------------------------------

|no_changes_yet_note|

Thread samples
--------------

|no_changes_yet_note|

Sensor samples
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

|no_changes_yet_note|

Drivers
=======

This section provides detailed lists of changes by :ref:`driver <drivers>`.

Wi-Fi drivers
-------------

|no_changes_yet_note|

Libraries
=========

This section provides detailed lists of changes by :ref:`library <libraries>`.

Binary libraries
----------------

|no_changes_yet_note|

Bluetooth libraries and services
--------------------------------

|no_changes_yet_note|

Bootloader libraries
--------------------

|no_changes_yet_note|

Debug libraries
---------------

|no_changes_yet_note|

DFU libraries
-------------

|no_changes_yet_note|

Modem libraries
---------------

* :ref:`nrf_modem_lib_readme`:

  * Fixed an issue with the CFUN hooks when the Modem library is initialized during ``SYS_INIT`` at kernel level and makes calls to the :ref:`nrf_modem_at` interface before the application level initialization is done.

Libraries for networking
------------------------

* :ref:`lib_nrf_cloud` library:

  * Added:

    * Support for Wi-Fi anchor names in the :c:struct:`nrf_cloud_location_result` structure.
    * The :kconfig:option:`CONFIG_NRF_CLOUD_LOCATION_ANCHOR_LIST` Kconfig option to enable including Wi-Fi anchor names in the location callback.
    * The :kconfig:option:`CONFIG_NRF_CLOUD_LOCATION_ANCHOR_LIST_BUFFER_SIZE` Kconfig option to control the buffer size used for the anchor names.

* :ref:`lib_mqtt_helper` library:

  * Changed the library to read certificates as standard PEM format. Previously the certificates had to be manually converted to string format before compiling the application.
  * Replaced the ``CONFIG_MQTT_HELPER_CERTIFICATES_FILE`` Kconfig option with :kconfig:option:`CONFIG_MQTT_HELPER_CERTIFICATES_FOLDER`. The new option specifies the folder where the certificates are stored.

* :ref:`lib_nrf_provisioning` library:

  * Added the :c:func:`nrf_provisioning_set_interval` function to set the interval between provisioning attempts.

* :ref:`lib_nrf_cloud_coap` library:

  * Updated to request proprietary PSM mode for ``SOC_NRF9151_LACA`` and ``SOC_NRF9131_LACA`` in addition to ``SOC_NRF9161_LACA``.

  * Added the :c:func:`nrf_cloud_coap_shadow_desired_update` function to allow devices to reject invalid shadow deltas.

Libraries for NFC
-----------------

|no_changes_yet_note|

Security libraries
------------------

|no_changes_yet_note|

Other libraries
---------------

* Added the :ref:`lib_uart_async_adapter` library.

* :ref:`app_event_manager`:

  * Added the :kconfig:option:`CONFIG_APP_EVENT_MANAGER_REBOOT_ON_EVENT_ALLOC_FAIL` Kconfig option.
    The option allows to select between system reboot or kernel panic on event allocation failure for default event allocator.

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

Scripts
=======

This section provides detailed lists of changes by :ref:`script <scripts>`.

|no_changes_yet_note|

MCUboot
=======

The MCUboot fork in |NCS| (``sdk-mcuboot``) contains all commits from the upstream MCUboot repository up to and including ``a4eda30f5b0cfd0cf15512be9dcd559239dbfc91``, with some |NCS| specific additions.

The code for integrating MCUboot into |NCS| is located in the :file:`ncs/nrf/modules/mcuboot` folder.

The following list summarizes both the main changes inherited from upstream MCUboot and the main changes applied to the |NCS| specific additions:

|no_changes_yet_note|

Zephyr
======

.. NOTE TO MAINTAINERS: All the Zephyr commits in the below git commands must be handled specially after each upmerge and each nRF Connect SDK release.

The Zephyr fork in |NCS| (``sdk-zephyr``) contains all commits from the upstream Zephyr repository up to and including ``0051731a41fa2c9057f360dc9b819e47b2484be5``, with some |NCS| specific additions.

For the list of upstream Zephyr commits (not including cherry-picked commits) incorporated into nRF Connect SDK since the most recent release, run the following command from the :file:`ncs/zephyr` repository (after running ``west update``):

.. code-block:: none

   git log --oneline 0051731a41 ^23cf38934c

For the list of |NCS| specific commits, including commits cherry-picked from upstream, run:

.. code-block:: none

   git log --oneline manifest-rev ^0051731a41

The current |NCS| main branch is based on revision ``0051731a41`` of Zephyr.

.. note::
   For possible breaking changes and changes between the latest Zephyr release and the current Zephyr version, refer to the :ref:`Zephyr release notes <zephyr_release_notes>`.

Additions specific to |NCS|
---------------------------

|no_changes_yet_note|

zcbor
=====

|no_changes_yet_note|

Trusted Firmware-M
==================

* Support PSA PAKE APIs from the PSA Crypto API specification 1.2.

cJSON
=====

|no_changes_yet_note|

Documentation
=============

* Recommend the use of a :file:`VERSION` file for :ref:`ug_fw_update_image_versions_mcuboot` in the :ref:`ug_fw_update_image_versions` user guide.
