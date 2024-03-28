.. _ble_rpc:

Bluetooth Low Energy Remote Procedure Call
##########################################

.. contents::
   :local:
   :depth: 2

The Bluetooth® Low Energy Remote Procedure Call library allows the full Bluetooth Low Energy stack to run on another device or CPU such as the nRF5340 DK network core.

Overview
********

The library allows to use the Bluetooth Low Energy API from a different CPU or device.
This is accomplished by running a full Bluetooth Low Energy stack on one device and serializing the API from another device.

Implementation
==============

The library encloses the configured Bluetooth Low Energy stack and the serialization on the host (network core) side.
The client (application core) side contains the serialization used to access the host side.

Supported backends
==================

The library supports the following two Bluetooth Low Energy controllers:

  * :ref:`zephyr:bluetooth-ctlr-arch`
  * :ref:`softdevice_controller`

Requirements
************

Some configuration options related to Bluetooth Low Energy must be the same on the host and client.
Set the following options in the same way for the :ref:`ble_rpc_host` and application core:

   * :kconfig:option:`CONFIG_BT_CENTRAL`
   * :kconfig:option:`CONFIG_BT_PERIPHERAL`
   * :kconfig:option:`CONFIG_BT_FILTER_ACCEPT_LIST`
   * :kconfig:option:`CONFIG_BT_USER_PHY_UPDATE`
   * :kconfig:option:`CONFIG_BT_USER_DATA_LEN_UPDATE`
   * :kconfig:option:`CONFIG_BT_PRIVACY`
   * :kconfig:option:`CONFIG_BT_SCAN_WITH_IDENTITY`
   * :kconfig:option:`CONFIG_BT_REMOTE_VERSION`
   * :kconfig:option:`CONFIG_BT_SMP`
   * :kconfig:option:`CONFIG_BT_CONN`
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
   * :kconfig:option:`CONFIG_CBKPROXY_OUT_SLOTS` on one core must be equal to :kconfig:option:`CONFIG_CBKPROXY_IN_SLOTS` on the other.

To keep all the above configuration options in sync, create an overlay file that is shared between the application and network core.
Then, you can invoke build command like this:

.. parsed-literal::
   :class: highlight

   west build -b *board* -- -DOVERLAY_CONFIG=my_overlay_file.conf

Configuration
*************

Set the :kconfig:option:`CONFIG_BT_RPC_STACK` Kconfig option to enable the Bluetooth RPC library.

Additionally, you can use the following options:

  * :kconfig:option:`CONFIG_BT_RPC`
  * :kconfig:option:`CONFIG_BT_RPC_CLIENT`
  * :kconfig:option:`CONFIG_BT_RPC_HOST`
  * :kconfig:option:`CONFIG_BT_RPC_STACK`
  * :kconfig:option:`CONFIG_BT_RPC_INITIALIZE_NRF_RPC`
  * :kconfig:option:`CONFIG_BT_RPC_GATT_SRV_MAX`
  * :kconfig:option:`CONFIG_BT_RPC_GATT_BUFFER_SIZE`
  * :kconfig:option:`CONFIG_BT_RPC_INTERNAL_FUNCTIONS`
  * :kconfig:option:`CONFIG_CBKPROXY_OUT_SLOTS`
  * :kconfig:option:`CONFIG_CBKPROXY_IN_SLOTS`

For more details, see the Kconfig option description.

Samples using the library
*************************

The following |NCS| sample and application use this library:

* :ref:`ble_rpc_host`
* :ref:`ipc_radio`

The :ref:`ble_rpc_host` sample enables the Bluetooth LE stack functionality on a remote MCU (for example, the nRF5340 network core) using the :ref:`nrfxlib:nrf_rpc`.
When building samples for the application core, enable the :kconfig:option:`CONFIG_BT_RPC_STACK` Kconfig option to run the Bluetooth LE stack on the network core.
This option builds :ref:`ble_rpc_host` automatically as a child image.
For more details, see :ref:`ug_nrf5340_building`.

The :ref:`ipc_radio` application is an alternative to the :ref:`ble_rpc_host` sample.

Limitations
***********

The library currently supports serialization of the following:

* :ref:`zephyr:bt_gap`
* :ref:`zephyr:bluetooth_connection_mgmt`
* :ref:`zephyr:bt_gatt`
* :ref:`Bluetooth Cryptography <zephyr:bt_crypto>`

The behavior of the Bluetooth implementation is almost the same as Zephyr's with the following exceptions:

* The latency is longer because of the overhead for exchanging messages between cores.
* The :c:func:`bt_gatt_cancel` function is not implemented.
* The ``flags`` field of  the :c:struct:`bt_gatt_subscribe_params` structure is atomic, so it cannot be correctly handled by the nRF RPC.
  The library implements the following workaround for it:

  * All ``flags`` are sent to the network core when either the :c:func:`bt_gatt_subscribe` or :c:func:`bt_gatt_resubscribe` function is called.
    This covers most of the cases, because the ``flags`` are normally set once before those functions calls.
  * If you want to read or write the ``flags`` after the subscription, you have to call :c:func:`bt_rpc_gatt_subscribe_flag_set`, :c:func:`bt_rpc_gatt_subscribe_flag_clear` or :c:func:`bt_rpc_gatt_subscribe_flag_get`.

Dependencies
************

The library has the following dependencies:

  * :ref:`nrf_rpc`
  * :ref:`bluetooth`

.. _ble_rpc_api:

API documentation
*****************

This library does not define a new API.
Instead, it uses Zephyr's :ref:`zephyr:bluetooth_api`.
