.. _ipc_radio:

IPC radio firmware
##################

.. contents::
   :local:
   :depth: 2

The IPC radio firmware allows to use the radio peripheral from another core in a multicore device.

.. _ipc_radio_overview:

Application overview
********************

You can use this firmware as a general serialized radio peripheral.

The firmware supports both Bluetooth® Low Energy and IEEE 802.15.4 simultaneously.
In addition, you can configure the Bluetooth selection as an HCI :ref:`bluetooth_controller` or an RPC serialization interface (:ref:`ble_rpc`).

HCI IPC serialization
=====================

The firmware exposes the :ref:`bluetooth_controller` support to another core using the IPC subsystem.

Host for nRF RPC Bluetooth Low Energy
=====================================

The firmware is running the full Bluetooth Low Energy stack.
It receives serialized function calls that it decodes and executes, then sends response data to the client.

The serialization includes:

* :ref:`bt_gap`
* :ref:`bluetooth_connection_mgmt`
* :ref:`bt_gatt`
* :ref:`Bluetooth Cryptography <bt_crypto>`

IEEE 802.15.4
=============

The firmware exposes radio driver support to another core using the IPC subsystem.

.. _ipc_radio_reqs:

Requirements
************

The firmware supports the following development kits:

.. table-from-sample-yaml::

To automatically attach the firmware image, you need to use :ref:`configuration_system_overview_sysbuild`.

.. _ipc_radio_config:

Configuration
*************

|config|

Application
===========

You can set the supported radio configurations using the following Kconfig options:

* :kconfig:option:`CONFIG_IPC_RADIO_BT` - For the Bluetooth Low Energy serialization.
* :kconfig:option:`CONFIG_IPC_RADIO_802154` - For the IEEE 802.15.4 serialization.


You can select the Bluetooth Low Energy serialization using the :kconfig:option:`CONFIG_IPC_RADIO_BT_SER` Kconfig option:

* :kconfig:option:`CONFIG_IPC_RADIO_BT_HCI_IPC` - For the Bluetooth HCI serialization.
* :kconfig:option:`CONFIG_IPC_RADIO_BT_RPC` - For the Bluetooth host API serialization.

The Bluetooth Low Energy and IEEE 802.15.4 functionalities can operate simultaneously and are only limited by available memory.

.. note::
   The IEEE 802.15.4 is currently not supported on the :ref:`zephyr:nrf54h20dk_nrf54h20` board.

Sysbuild Kconfig options
========================

To enable the firmware, use the sysbuild configuration ``SB_CONFIG_NETCORE_IPC_RADIO``.

You can set the supported radio configurations using the following sysbuild Kconfig options:

* ``SB_CONFIG_NETCORE_IPC_RADIO_BT_HCI_IPC``
* ``SB_CONFIG_NETCORE_IPC_RADIO_BT_RPC``
* ``SB_CONFIG_NETCORE_IPC_RADIO_IEEE802154``

Configuration files
===================

The application provides predefined configuration files for typical use cases.
You can find the configuration files in the application directory.

The following files are available:

* :file:`base-debug.conf` - Configuration base file enabling debug build.
* :file:`base-release.conf` - Configuration base file enabling release build. 
* :file:`config-802154.conf` - Configuration file enabling IEEE 802.15.4.
* :file:`config-bt_hci_ipc.conf` - Configuration file enabling Bluetooth Low Energy over HCI.
* :file:`config-bt_rpc.conf` - Configuration file enabling Bluetooth Low Energy over RPC.
* :file:`stack-bt_hci_802154.conf` - Configuration stack file for Bluetooth Low Energy over HCI and IEEE 802.15.4.
* :file:`stack-bt_hci.conf` - Configuration stack file for Bluetooth Low Energy over HCI only.
* :file:`stack-bt_rpc.conf` - Configuration stack file for Bluetooth Low Energy over RPC only.

The final configuration is a combination of:

 * ``base`` file - Either debug o release variant.
 * ``config`` files - One or more file enabling and configuring a functionality.
 * ``stack`` optional file - Sets appropriate stack sizes for a specific combination of functionalities.

.. note::
   When you use sysbuild to build an application which uses the IPC radio firmware as the network or radio core image, the preceding configuration files are added automatically to the IPC radio firmware.
   The selection of specific configuration files is determined by the sysbuild Kconfig.

   For instance, the ``SB_CONFIG_NETCORE_IPC_RADIO_IEEE802154`` Kconfig option enables the :file:`config-802154.conf` configuration file to be used with the IPC radio firmware.

.. _ipc_radio_build_run:

Building and running as a single image
**************************************

.. |application path| replace:: :file:`applications/ipc_radio`

.. include:: /includes/application_build_and_run.txt

For instructions on how to enable a specific configuration overlay file, see :ref:`building_advanced`.

.. note::
   You cannot use :ref:`ble_rpc` together with the HCI :ref:`bluetooth_controller`.

Dependencies
************

The dependencies may vary according to the configuration.

This firmware can use the following |NCS| libraries:

* :ref:`softdevice_controller`
* :ref:`nrf_802154_sl`
* :ref:`mpsl`
* :ref:`ble_rpc`

It can use the following `sdk-nrfxlib`_ library:

* :ref:`nrfxlib:nrf_rpc`
