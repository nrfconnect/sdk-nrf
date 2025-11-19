.. _ncs_release_notes_310_preview1:

Changelog for |NCS| v3.1.0-preview1
###################################

.. contents::
   :local:
   :depth: 2

This changelog reflects the most relevant changes from the latest official release.

.. HOWTO

   When adding a new PR, decide whether it needs an entry in the changelog.
   If it does, update this page.
   Add the sections you need, as only a handful of sections are kept when the changelog is cleaned.
   The "Protocols" section serves as a highlight section for all protocol-related changes, including those made to samples, libraries, and so on.

Known issues
************

Known issues are only tracked for the latest official release.
See `known issues for nRF Connect SDK v3.0.0`_ for the list of issues valid for the latest release.

Changelog
*********

The following sections provide detailed lists of changes by component.

IDE, OS, and tool support
=========================

|no_changes_yet_note|

Board support
=============

|no_changes_yet_note|

Build and configuration system
==============================

|no_changes_yet_note|

Bootloaders and DFU
===================

|no_changes_yet_note|

Developing with nRF91 Series
============================

|no_changes_yet_note|

Developing with nRF70 Series
============================

|no_changes_yet_note|

Developing with nRF54L Series
=============================

|no_changes_yet_note|

Developing with nRF54H Series
=============================

|no_changes_yet_note|

Developing with nRF53 Series
============================

|no_changes_yet_note|

Developing with nRF52 Series
============================

|no_changes_yet_note|

Developing with Thingy:91 X
===========================

|no_changes_yet_note|

Developing with Thingy:91
=========================

|no_changes_yet_note|

Developing with Thingy:53
=========================

|no_changes_yet_note|

Developing with PMICs
=====================

|no_changes_yet_note|

Developing with Front-End Modules
=================================

|no_changes_yet_note|

Developing with custom boards
=============================

|no_changes_yet_note|

Security
========

|no_changes_yet_note|

Protocols
=========

|no_changes_yet_note|

Amazon Sidewalk
---------------

|no_changes_yet_note|

Bluetooth® LE
-------------

|no_changes_yet_note|

Bluetooth Mesh
--------------

|no_changes_yet_note|

DECT NR+
--------

|no_changes_yet_note|

Enhanced ShockBurst (ESB)
-------------------------

|no_changes_yet_note|

Gazell
------

|no_changes_yet_note|

Matter
------

* Updated:

  * The ``west zap-generate`` command to remove previously generated ZAP files before generating new files.
    To skip removing the files, use the ``--keep-previous`` argument.

Matter fork
+++++++++++

|no_changes_yet_note|

nRF IEEE 802.15.4 radio driver
------------------------------

|no_changes_yet_note|

Thread
------

|no_changes_yet_note|


Wi-Fi®
------

|no_changes_yet_note|

Applications
============

|no_changes_yet_note|

Connectivity bridge
-------------------

|no_changes_yet_note|

IPC radio firmware
------------------

|no_changes_yet_note|

Matter bridge
-------------

|no_changes_yet_note|

nRF5340 Audio
-------------
* Updated:

  * The application to use the ``NFC.TAGHEADER0`` value from FICR as the broadcast ID instead of using a random ID.

nRF Desktop
-----------

* Updated the application configurations for dongles on memory-limited SoCs (nRF52820) to reuse the system workqueue for GATT Discovery Manager (:kconfig:option:`CONFIG_BT_GATT_DM_WORKQ_SYS`).
  This helps to reduce RAM usage.

nRF Machine Learning (Edge Impulse)
-----------------------------------

|no_changes_yet_note|

Serial LTE modem
----------------

* Updated:

  * The ``AT#XPPP`` command to support the CID parameter to specify the PDN connection used for PPP.
  * The ``#XPPP`` notification to include the CID of the PDN connection used for PPP.
  * The initialization of the application to ignore a failure in nRF Cloud module initialization.
    This occurs sometimes especially during development.
  * The initialization of the application to send "INIT ERROR" over to UART and show clear error log to indicate that the application is not operational in case of failing initialization.

Thingy:53: Matter weather station
---------------------------------

|no_changes_yet_note|

Samples
=======

This section provides detailed lists of changes by :ref:`sample <samples>`.

Amazon Sidewalk samples
-----------------------

|no_changes_yet_note|

Bluetooth samples
-----------------

* :ref:`bluetooth_isochronous_time_synchronization` sample:

  * Fixed an issue where the sample would assert with the :kconfig:option:`CONFIG_ASSERT` Kconfig option enabled.
    This was due to calling the :c:func:`bt_iso_chan_send` function from a timer ISR handler and sending SDUs to the controller with invalid timestamps.

|no_changes_yet_note|

Bluetooth Mesh samples
----------------------

|no_changes_yet_note|

Bluetooth Fast Pair samples
---------------------------

* :ref:`fast_pair_locator_tag` sample:

  * Added possibility to build and run the sample without the motion detector support (with the :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_DULT_MOTION_DETECTOR` Kconfig option disabled).

Cellular samples
----------------

* Deprecated the LTE Sensor Gateway sample.
  It is no longer maintained.

* nRF Cloud multi-service sample:

  * Added support for native simulator platform and updated the documentation accordingly.

Cryptography samples
--------------------

|no_changes_yet_note|

Debug samples
-------------

|no_changes_yet_note|

DECT NR+ samples
----------------

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

|no_changes_yet_note|

Networking samples
------------------

|no_changes_yet_note|

NFC samples
-----------

* Added experimental ``llvm`` toolchain support for the ``nrf54l15dk/nrf54l15/cpuapp`` board target to the following samples:

  * :ref:`writable_ndef_msg`
  * :ref:`nfc_shell`

* :ref:`record_text` sample:

  * Added support for the ``nrf54l15dk/nrf54l15/cpuapp/ns`` board target.

nRF5340 samples
---------------

|no_changes_yet_note|

Peripheral samples
------------------

|no_changes_yet_note|

PMIC samples
------------

|no_changes_yet_note|

Protocol serialization samples
------------------------------

|no_changes_yet_note|

SDFW samples
------------

|no_changes_yet_note|

Sensor samples
--------------

|no_changes_yet_note|

SUIT samples
------------

|no_changes_yet_note|

Trusted Firmware-M (TF-M) samples
---------------------------------

|no_changes_yet_note|

Thread samples
--------------

|no_changes_yet_note|

Wi-Fi samples
-------------

* :ref:`wifi_radiotest_samples`:

  * Updated :ref:`wifi_radio_test` and :ref:`wifi_radio_test_sd` samples to clarify platform support for single-domain and multi-domain radio tests.

Other samples
-------------

|no_changes_yet_note|

Drivers
=======

This section provides detailed lists of changes by :ref:`driver <drivers>`.

* Added the :ref:`mspi_sqspi` that allows for communication with devices that use MSPI bus-based Zephyr drivers.

Wi-Fi drivers
-------------

|no_changes_yet_note|

Flash drivers
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

* :ref:`bt_fast_pair_readme` library:

  * Updated:

    * The :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_RING_REQ_TIMEOUT_DULT_MOTION_DETECTOR` Kconfig option dependency.
      The dependency has been updated from the :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_DULT` Kconfig option to :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_DULT_MOTION_DETECTOR`.

Common Application Framework
----------------------------

|no_changes_yet_note|

Debug libraries
---------------

* Added an experimental :ref:`Zephyr Core Dump <zephyr:coredump>` backend that writes a core dump to an internal flash or RRAM partition.
  To enable this backend, set the :kconfig:option:`CONFIG_DEBUG_COREDUMP_BACKEND_OTHER` and :kconfig:option:`CONFIG_DEBUG_COREDUMP_BACKEND_NRF_FLASH_PARTITION` Kconfig options.

* :ref:`cpu_load` library:

  * Added prefix ``NRF_`` to all Kconfig options (for example, :kconfig:option:`CONFIG_NRF_CPU_LOAD`) to avoid conflicts with Zephyr Kconfig options with the same names.

DFU libraries
-------------

|no_changes_yet_note|

Gazell libraries
----------------

|no_changes_yet_note|

Security libraries
------------------

|no_changes_yet_note|

Modem libraries
---------------

* :ref:`at_parser_readme` library:

  * Added support for parsing DECT NR+ modem firmware names.

* :ref:`lte_lc_readme` library:

  * Added the :kconfig:option:`CONFIG_LTE_LC_DNS_FALLBACK_MODULE` and :kconfig:option:`CONFIG_LTE_LC_DNS_FALLBACK_ADDRESS` Kconfig options to enable setting a fallback DNS address.
    The :kconfig:option:`CONFIG_LTE_LC_DNS_FALLBACK_MODULE` Kconfig option is enabled by default.
    If the application has configured a DNS server address in Zephyr's native networking stack, using the :kconfig:option:`CONFIG_DNS_SERVER1` Kconfig option, the same server is set as the fallback address for DNS queries offloaded to the nRF91 Series modem.
    Otherwise, the :kconfig:option:`CONFIG_LTE_LC_DNS_FALLBACK_ADDRESS` Kconfig option controls the fallback DNS server address that is set to Cloudflare's DNS server: 1.1.1.1 by default.
    The device might or might not receive a DNS address by the network during PDN connection.
    Even within the same network, the PDN connection establishment method (PCO vs ePCO) might change when the device operates in NB-IoT or LTE Cat-M1, resulting in missing DNS addresses when one method is used, but not the other.
    Having a fallback DNS address ensures that the device always has a DNS to fallback to.

* Modem SLM library:

  * Added:

      * The ``CONFIG_MODEM_SLM_UART_RX_BUF_COUNT`` Kconfig option for configuring RX buffer count.
      * The ``CONFIG_MODEM_SLM_UART_RX_BUF_SIZE`` Kconfig option for configuring RX buffer size.
      * The ``CONFIG_MODEM_SLM_UART_TX_BUF_SIZE`` Kconfig option for configuring TX buffer size.
      * The ``CONFIG_MODEM_SLM_AT_CMD_RESP_MAX_SIZE`` Kconfig option for buffering AT command responses.

  * Updated the UART implementation between the host device, using the Modem SLM library, and the device running the Serial LTE modem application.

  * Removed:

      * The ``CONFIG_MODEM_SLM_DMA_MAXLEN`` Kconfig option.
        Use ``CONFIG_MODEM_SLM_UART_RX_BUF_SIZE`` instead.
      * The ``modem_slm_reset_uart`` function as there is no longer a need to reset the UART.

Multiprotocol Service Layer libraries
-------------------------------------

|no_changes_yet_note|

Libraries for networking
------------------------

|no_changes_yet_note|

Libraries for NFC
-----------------

|no_changes_yet_note|

nRF RPC libraries
-----------------

|no_changes_yet_note|

Other libraries
---------------

|no_changes_yet_note|

Shell libraries
---------------

|no_changes_yet_note|

sdk-nrfxlib
-----------

See the changelog for each library in the :doc:`nrfxlib documentation <nrfxlib:README>` for additional information.

Scripts
=======

* Added the :file:`ncs_ironside_se_update.py` script in the :file:`scripts/west_commands` folder.
  The script adds the west command ``west ncs-ironside-se-update`` for installing an IronSide SE update.

* :ref:`nrf_desktop_config_channel_script` Python script:

  * Updated:

    * The udev rules for Debian, Ubuntu, and Linux Mint HID host computers (replaced the :file:`99-hid.rules` file with :file:`60-hid.rules`).
      This is done to ensure that the rules are properly applied for an nRF Desktop device connected directly over Bluetooth LE.
      The new udev rules are applied to any HID device that uses the Nordic Semiconductor Vendor ID (regardless of Product ID).
    * The HID device discovery to ensure that a discovery failure of a HID device would not affect other HID devices.
      Without this change, problems with discovery of a HID device could lead to skipping discovery and listing of other HID devices (even if the devices work properly).

Integrations
============

This section provides detailed lists of changes by :ref:`integration <integrations>`.

Google Fast Pair integration
----------------------------

|no_changes_yet_note|

Edge Impulse integration
------------------------

|no_changes_yet_note|

Memfault integration
--------------------

|no_changes_yet_note|

AVSystem integration
--------------------

|no_changes_yet_note|

nRF Cloud integration
---------------------

|no_changes_yet_note|

CoreMark integration
--------------------

|no_changes_yet_note|

DULT integration
----------------

|no_changes_yet_note|

MCUboot
=======

The MCUboot fork in |NCS| (``sdk-mcuboot``) contains all commits from the upstream MCUboot repository up to and including ``81315483fcbdf1f1524c2b34a1fd4de6c77cd0f4``, with some |NCS| specific additions.

The code for integrating MCUboot into |NCS| is located in the :file:`ncs/nrf/modules/mcuboot` folder.

The following list summarizes both the main changes inherited from upstream MCUboot and the main changes applied to the |NCS| specific additions:

|no_changes_yet_note|

Zephyr
======

.. NOTE TO MAINTAINERS: All the Zephyr commits in the below git commands must be handled specially after each upmerge and each nRF Connect SDK release.

The Zephyr fork in |NCS| (``sdk-zephyr``) contains all commits from the upstream Zephyr repository up to and including ``9a6f116a6aa9b70b517a420247cd8d33bbbbaaa3``, with some |NCS| specific additions.

For the list of upstream Zephyr commits (not including cherry-picked commits) incorporated into nRF Connect SDK since the most recent release, run the following command from the :file:`ncs/zephyr` repository (after running ``west update``):

.. code-block:: none

   git log --oneline 9a6f116a6a ^fdeb735017

For the list of |NCS| specific commits, including commits cherry-picked from upstream, run:

.. code-block:: none

   git log --oneline manifest-rev ^9a6f116a6a

The current |NCS| main branch is based on revision ``9a6f116a6a`` of Zephyr.

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

|no_changes_yet_note|

cJSON
=====

|no_changes_yet_note|

Documentation
=============

|no_changes_yet_note|
