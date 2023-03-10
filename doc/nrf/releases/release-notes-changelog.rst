.. _ncs_release_notes_changelog:

Changelog for |NCS| v2.3.99
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
See `known issues for nRF Connect SDK v2.3.0`_ for the list of issues valid for the latest release.

Changelog
*********

The following sections provide detailed lists of changes by component.

IDE and tool support
====================

|no_changes_yet_note|

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

Working with nRF52 Series
=========================

|no_changes_yet_note|

Working with nRF53 Series
=========================

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

|no_changes_yet_note|

See `Matter samples`_ for the list of changes for the Matter samples.

Matter fork
+++++++++++

The Matter fork in the |NCS| (``sdk-connectedhomeip``) contains all commits from the upstream Matter repository up to, and including, the ``1.0.0.2`` tag.

  * Added:

    * The Matter Nordic UART Service (NUS) feature to the :ref:`matter_lock_sample`.
      This feature allows using Nordic UART Service to control the device remotely through Bluetooth LE and adding custom text commands to a Matter sample.
      The Matter NUS implementation allows controlling the device regardless of whether the device is connected to a Matter network or not.
      The feature is dedicated for the nRF5340 and the nRF52840 DKs.

The following list summarizes the most important changes inherited from the upstream Matter:

|no_changes_yet_note|

Thread
------

|no_changes_yet_note|

See `Thread samples`_ for the list of changes for the Thread samples.

Zigbee
------

|no_changes_yet_note|

See `Zigbee samples`_ for the list of changes for the Zigbee samples.

Enhanced ShockBurst (ESB)
-------------------------

* Added support for bigger payload size.
  ESB supports a payload with a size of 64 bytes or more.

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

* Updated:

  * Use defines from the :ref:`lib_nrf_cloud` library for nRF Cloud related string values.
  * The application now integrates the :ref:`lib_lwm2m_client_utils` FOTA callback functionality.

nRF9160: Serial LTE modem
-------------------------

* Added:

  * AT command ``#XWIFIPOS`` to get Wi-Fi location from nRF Cloud.

* Updated:

  * Use defines from the :ref:`lib_nrf_cloud` library for nRF Cloud related string values.

* Fixed:

  * A bug in receiving large MQTT Publish message.

nRF5340 Audio
-------------

|no_changes_yet_note|

nRF Machine Learning (Edge Impulse)
-----------------------------------

|no_changes_yet_note|

nRF Desktop
-----------

* Updated:

  * The :ref:`nrf_desktop_dfu` automatically enables 8-bit write block size emulation (:kconfig:option:`CONFIG_SOC_FLASH_NRF_EMULATE_ONE_BYTE_WRITE_ACCESS`) to ensure that update images with sizes unaligned to word size can be successfully stored in the internal FLASH.
    The feature is not enabled if the MCUboot bootloader is used and the secondary slot is placed in an external FLASH (when :kconfig:option:`CONFIG_PM_EXTERNAL_FLASH_MCUBOOT_SECONDARY` is enabled).
  * In the Fast Pair configurations, the bond erase operation is enabled for the dongle peer, which will let you change the bonded Bluetooth Central.

Samples
=======

Bluetooth samples
-----------------

* Removed:

  * The Bluetooth 3-wire coex sample because of the removal of the 3-wire implementation.

Bluetooth mesh samples
----------------------

|no_changes_yet_note|

nRF9160 samples
---------------

* :ref:`modem_shell_application` sample:

  * Updated:

    * Use defines from the :ref:`lib_nrf_cloud` library for nRF Cloud related string values.
      Remove the inclusion of the file :file:`nrf_cloud_codec.h`.

* :ref:`slm_shell_sample` sample:

  * Added:

    * Support for the nRF7002 DK PCA10143.

* :ref:`lwm2m_client` sample:

  * Updated:

    * The sample now integrates the :ref:`lib_lwm2m_client_utils` FOTA callback functionality.

* :ref:`nrf_cloud_mqtt_multi_service` sample:

  * Updated:

    * Increased the MCUboot partition size to the minimum necessary to allow bootloader FOTA.

Peripheral samples
------------------

|no_changes_yet_note|

Trusted Firmware-M (TF-M) samples
---------------------------------

|no_changes_yet_note|

Thread samples
--------------

|no_changes_yet_note|

Matter samples
--------------

* :ref:`matter_lock_sample`:

    * Added the Matter Nordic UART Service (NUS) feature, which allows controlling the door lock device remotely through Bluetooth LE using two simple commands: ``Lock`` and ``Unlock``.
      This feature is dedicated for the nRF52840 and the nRF5340 DKs.

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

|no_changes_yet_note|

Drivers
=======

This section provides detailed lists of changes by :ref:`driver <drivers>`.

|no_changes_yet_note|

Libraries
=========

This section provides detailed lists of changes by :ref:`library <libraries>`.

Binary libraries
----------------

|no_changes_yet_note|

Bluetooth libraries and services
--------------------------------

* :ref:`bt_le_adv_prov_readme` library:

  * Added API to enable or disable the Swift Pair provider (:c:func:`bt_le_adv_prov_swift_pair_enable`).

* :ref:`bt_fast_pair_readme`:

  * Added the :c:func:`bt_fast_pair_info_cb_register` function and the :c:struct:`bt_fast_pair_info_cb` structure to register Fast Pair information callbacks.
    The :c:member:`bt_fast_pair_info_cb.account_key_written` callback can be used to notify the application about the Account Key writes.

Bootloader libraries
--------------------

|no_changes_yet_note|

Modem libraries
---------------

* :ref:`nrf_modem_lib_readme` library:

  * Added:

    * The function :c:func:`nrf_modem_lib_fault_strerror` to retrieve a statically allocated textual description of a given modem fault.
      The function can be enabled using the new Kconfig option :kconfig:option:`CONFIG_NRF_MODEM_LIB_FAULT_STRERROR`.

  * Updated:

    * The Kconfig option :kconfig:option:`CONFIG_NRF_MODEM_LIB_IPC_PRIO_OVERRIDE` is now deprecated.

  * Removed:

    * The deprecated function ``nrf_modem_lib_get_init_ret``.
    * The deprecated function ``nrf_modem_lib_shutdown_wait``.
    * The deprecated Kconfig option ``CONFIG_NRF_MODEM_LIB_TRACE_ENABLED``.

Libraries for networking
------------------------

* :ref:`lib_nrf_cloud` library:

  * Added:

    * A public header file :file:`nrf_cloud_defs.h` that contains common defines for interacting with nRF Cloud and the :ref:`lib_nrf_cloud` library.
    * A new event :c:enum:`NRF_CLOUD_EVT_TRANSPORT_CONNECT_ERROR` to indicate an error while the transport connection is being established when the :kconfig:option:`CONFIG_NRF_CLOUD_CONNECTION_POLL_THREAD` Kconfig option is enabled.
      Earlier this was indicated with a second :c:enum:`NRF_CLOUD_EVT_TRANSPORT_CONNECTING` event with an error status.

* :ref:`lib_lwm2m_client_utils` library:

  * Updated:

    * :file:`lwm2m_client_utils.h` includes new API for FOTA to register application callback to receive state changes and requests for the update process.

  * Removed:

    * The old API ``lwm2m_firmware_get_update_state_cb()``.

Libraries for NFC
-----------------

|no_changes_yet_note|

Other libraries
---------------

|no_changes_yet_note|

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

|no_changes_yet_note|

MCUboot
=======

The MCUboot fork in |NCS| (``sdk-mcuboot``) contains all commits from the upstream MCUboot repository up to and including ``cfec947e0f8be686d02c73104a3b1ad0b5dcf1e6``, with some |NCS| specific additions.

The code for integrating MCUboot into |NCS| is located in the :file:`ncs/nrf/modules/mcuboot` folder.

The following list summarizes both the main changes inherited from upstream MCUboot and the main changes applied to the |NCS| specific additions:

* Added support for the downgrade prevention feature using hardware security counters (:kconfig:option:`MCUBOOT_HARDWARE_DOWNGRADE_PREVENTION`).

Zephyr
======

.. NOTE TO MAINTAINERS: All the Zephyr commits in the below git commands must be handled specially after each upmerge and each nRF Connect SDK release.

The Zephyr fork in |NCS| (``sdk-zephyr``) contains all commits from the upstream Zephyr repository up to and including ``e1e06d05fa8d1b6ac1b0dffb1712e94e308861f8``, with some |NCS| specific additions.

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

|no_changes_yet_note|
