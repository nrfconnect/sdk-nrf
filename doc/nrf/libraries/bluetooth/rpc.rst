.. _ble_rpc:

Bluetooth Low Energy Remote Procedure Call
##########################################

.. contents::
   :local:
   :depth: 2

The Bluetooth® Low Energy Remote Procedure Call (RPC) solution is a set of libraries that allows using the Bluetooth Low Energy stack running entirely on a separate device or CPU.

Overview
********

The solution allows calling the Bluetooth Low Energy API on a different CPU or device.
This is accomplished by running a full Bluetooth Low Energy stack on one device and serializing the API from another device.
Use this solution when you do not want your firmware to include the Bluetooth stack, for example to offload the application CPU, save memory, or to be able to build your application in a different environment.

Implementation
==============

The Bluetooth Low Energy RPC is a solution that consists of the following components:

  * Bluetooth RPC Client and common software libraries that serialize the Bluetooth Host API and enable RPC communication.
    These libraries need to be part of the user application.
  * :ref:`ipc_radio` application that includes the full configuration of the Bluetooth Low Energy stack, Bluetooth RPC Host and common libraries that enable communication with the Bluetooth RPC Client.
    This software includes the :ref:`ug_ble_controller` and needs to run on a device or CPU that has the radio hardware peripheral, for example nRF5340 network core.
  * Build system files that automate building a Bluetooth LE application in the RPC variant with the additional image containing the Bluetooth LE stack.

You can add support for serializing Bluetooth-related custom APIs by implementing your own client and host procedures.
You can use the following files as examples:

  * :file:`subsys/bluetooth/rpc/client/bt_rpc_conn_client.c`
  * :file:`subsys/bluetooth/rpc/host/bt_rpc_conn_host.c`

Supported backends
==================

The library supports the following two Bluetooth Low Energy controllers:

  * :ref:`zephyr:bluetooth-ctlr-arch`
  * :ref:`softdevice_controller`

Requirements
************

Some configuration options related to Bluetooth Low Energy must be the same on the host and client.
Set the following options in the same way for the :ref:`ipc_radio` or :ref:`ble_rpc_host`, and application core:

  * :kconfig:option:`CONFIG_BT_CENTRAL`
  * :kconfig:option:`CONFIG_BT_PERIPHERAL`
  * :kconfig:option:`CONFIG_BT_FILTER_ACCEPT_LIST`
  * :kconfig:option:`CONFIG_BT_USER_PHY_UPDATE`
  * :kconfig:option:`CONFIG_BT_USER_DATA_LEN_UPDATE`
  * :kconfig:option:`CONFIG_BT_PRIVACY`
  * :kconfig:option:`CONFIG_BT_SCAN_WITH_IDENTITY`
  * :kconfig:option:`CONFIG_BT_REMOTE_VERSION`
  * :kconfig:option:`CONFIG_BT_SMP`
  * :kconfig:option:`CONFIG_BT_CONN` - hidden option that depends on :kconfig:option:`CONFIG_BT_CENTRAL` or :kconfig:option:`CONFIG_BT_PERIPHERAL`.
  * :kconfig:option:`CONFIG_BT_REMOTE_INFO`
  * :kconfig:option:`CONFIG_BT_FIXED_PASSKEY`
  * :kconfig:option:`CONFIG_BT_SMP_APP_PAIRING_ACCEPT`
  * :kconfig:option:`CONFIG_BT_EXT_ADV`
  * :kconfig:option:`CONFIG_BT_OBSERVER`
  * :kconfig:option:`CONFIG_BT_ECC`
  * :kconfig:option:`CONFIG_BT_DEVICE_NAME_DYNAMIC`
  * :kconfig:option:`CONFIG_BT_SMP_SC_PAIR_ONLY`
  * :kconfig:option:`CONFIG_BT_PER_ADV`
  * :kconfig:option:`CONFIG_BT_PER_ADV_SYNC`
  * :kconfig:option:`CONFIG_BT_MAX_PAIRED`
  * :kconfig:option:`CONFIG_BT_SETTINGS_CCC_LAZY_LOADING`
  * :kconfig:option:`CONFIG_BT_BROADCASTER`
  * :kconfig:option:`CONFIG_BT_SETTINGS`
  * :kconfig:option:`CONFIG_BT_GATT_CLIENT`
  * :kconfig:option:`CONFIG_BT_RPC_INTERNAL_FUNCTIONS`
  * :kconfig:option:`CONFIG_BT_DEVICE_APPEARANCE_DYNAMIC`
  * :kconfig:option:`CONFIG_BT_MAX_CONN`
  * :kconfig:option:`CONFIG_BT_ID_MAX`
  * :kconfig:option:`CONFIG_BT_EXT_ADV_MAX_ADV_SET`
  * :kconfig:option:`CONFIG_BT_DEVICE_NAME_MAX`
  * :kconfig:option:`CONFIG_BT_PER_ADV_SYNC_MAX`
  * :kconfig:option:`CONFIG_BT_DEVICE_APPEARANCE`
  * :kconfig:option:`CONFIG_BT_DEVICE_NAME`
  * :kconfig:option:`CONFIG_NRF_RPC_CBKPROXY_OUT_SLOTS` on one core must be equal to :kconfig:option:`CONFIG_NRF_RPC_CBKPROXY_IN_SLOTS` on the other.

To keep all the above configuration options in sync, you can create a snippet with an overlay file that is shared between the application and network core.
Then, you can invoke build command like this:

.. parsed-literal::
   :class: highlight

   west build -b *board_target* -S my_snippet

For more details, see the :ref:`zephyr:snippets` documentation.

Configuration
*************

To enable the Bluetooth Low Energy RPC library, use the sysbuild configuration ``SB_CONFIG_NETCORE_IPC_RADIO`` along with the ``SB_CONFIG_NETCORE_IPC_RADIO_BT_RPC`` option.
Set the :makevar:`SNIPPET` to ``nordic-bt-rpc`` to apply the necessary configuration for the Bluetooth Low Energy RPC library on both cores.
If you use a custom board, you need to create a custom snippet with a similar configuration to the ``nordic-bt-rpc`` snippet.

Build the application using the following command:

.. code-block:: console

   west build -b *board_name* -S nordic-bt-rpc -- -DSB_CONFIG_NETCORE_IPC_RADIO=y -DSB_CONFIG_NETCORE_IPC_RADIO_BT_RPC=y

.. note::
   The samples that support the Bluetooth Low Energy RPC use the :makevar:`FILE_SUFFIX` variable along with :makevar:`SNIPPET` to adjust the selection and configuration of the network and radio core firmware.

Samples using the library
*************************

The following |NCS| sample and application use this library:

* :ref:`ipc_radio`
* :ref:`ble_rpc_host`

The following |NCS| samples can optionally use this library:

* :ref:`central_uart`
* :ref:`peripheral_hids_mouse`
* :ref:`peripheral_uart`

The :ref:`ipc_radio` sample, is a configurable application, that exposes the Bluetooth LE stack functionality running on an MCU with radio (for example, the nRF5340 network core) to another CPU through the :ref:`nrfxlib:nrf_rpc`.
When using the :ref:`ipc_radio` as Bluetooth Low Energy over RPC, reconfigure it by setting the ``SB_CONFIG_NETCORE_IPC_RADIO_BT_RPC`` sysbuild Kconfig option to ``y`` to run the whole Bluetooth LE stack on the network core.
For more details, see the :ref:`Configuration <ipc_radio_config>` section of the IPC radio firmware application guide.

The :ref:`ble_rpc_host` application is an alternative to the :ref:`ipc_radio` sample.

Limitations
***********

The library currently supports serialization of the following:

  * :ref:`zephyr:bt_gap`
  * :ref:`zephyr:bluetooth_connection_mgmt`
  * :ref:`zephyr:bt_gatt`
  * :ref:`Bluetooth Cryptography <zephyr:bt_crypto>`

The behavior of the Bluetooth implementation is almost the same as Zephyr's with the following exceptions:

  * The latency is longer because of the overhead for exchanging messages between cores.
    The Bluetooth LE API is not strictly real-time by design, so the additional latency introduced by the IPC communication should be acceptable in most applications.
    To reduce the latency, consider using a different transport backend for nRF RPC.
    See :ref:`nrf_rpc_architecture` for details.
  * Using advanced Bluetooth LE configurations, such as multiple simultaneous connections or advanced security features can be a limitation, because the child image (:ref:`ble_rpc_host` or :ref:`ipc_radio`) might require significantly more memory than the MCU it runs on has available.
    Typically, network or radio cores are more memory-constrained than the application MCU.
  * The :c:func:`bt_gatt_cancel` function is not implemented.
  * The ``flags`` field of  the :c:struct:`bt_gatt_subscribe_params` structure is atomic, so it cannot be correctly handled by the nRF RPC.
    The library implements the following workaround for it:

    * All ``flags`` are sent to the network core when either the :c:func:`bt_gatt_subscribe` or :c:func:`bt_gatt_resubscribe` function is called.
      This covers most of the cases, because the ``flags`` are normally set once before those functions calls.
    * If you want to read or write the ``flags`` after the subscription, you have to call :c:func:`bt_rpc_gatt_subscribe_flag_set`, :c:func:`bt_rpc_gatt_subscribe_flag_clear`, or :c:func:`bt_rpc_gatt_subscribe_flag_get`.

Dependencies
************

The library has the following dependencies:

  * :ref:`nrf_rpc`
  * :ref:`bluetooth`

.. _ble_rpc_api:

API documentation
*****************

This library does not define a new Bluetooth API except for ``flags`` modification.
Instead, it uses Zephyr's :ref:`zephyr:bluetooth_api`.

| Header file: :file:`include/bluetooth/bt_rpc.h`
| Source files: :file:`subsys/bluetooth/rpc/`

.. doxygengroup:: bt_rpc
