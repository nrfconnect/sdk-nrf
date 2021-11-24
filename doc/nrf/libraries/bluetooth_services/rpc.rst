.. _ble_rpc:

Bluetooth Low Energy Remote Procedure Call
##########################################

.. contents::
   :local:
   :depth: 2

The |NCS| supports Bluetooth® Low Energy (LE) stack serialization.
The full Bluetooth LE stack can run on another device or CPU, such as the nRF5340 DK network core using :ref:`nrfxlib:nrf_rpc`.

Network core
************

The :ref:`ble_rpc_host` sample is designed specifically to enable the Bluetooth LE stack functionality on a remote MCU (for example, the nRF5340 network core) using the :ref:`nrfxlib:nrf_rpc`.
You can program this sample to the network core to run standard Bluetooth Low Energy samples on nRF5340.
You can use either the SoftDevice Controller or the Zephyr Bluetooth LE Controller for this sample.

Application core
****************

To use the Bluetooth LE stack through nRF RPC, an additional configuration is needed.
When building samples for the application core, enable the :kconfig:`CONFIG_BT_RPC_STACK` to run the Bluetooth LE stack on the network core.
This option builds :ref:`ble_rpc_host` automatically as a child image.
For more details, see: :ref:`ug_nrf5340_building`.

Open a command prompt in the build folder of the application sample and enter the following command to build the application for the application core, with :ref:`ble_rpc_host` as child image:

.. code-block:: console

   west build -b nrf5340dk_nrf5340_cpuapp -- -DCONFIG_BT_RPC_STACK=y

Requirements
************

Some configuration options related to Bluetooth LE must be the same on the host (network core) and client (application core).
Set the following options in the same way for the :ref:`ble_rpc_host` and application core sample:

   * :kconfig:`CONFIG_BT_CENTRAL`
   * :kconfig:`CONFIG_BT_PERIPHERAL`
   * :kconfig:`CONFIG_BT_FILTER_ACCEPT_LIST`
   * :kconfig:`CONFIG_BT_USER_PHY_UPDATE`
   * :kconfig:`CONFIG_BT_USER_DATA_LEN_UPDATE`
   * :kconfig:`CONFIG_BT_PRIVACY`
   * :kconfig:`CONFIG_BT_SCAN_WITH_IDENTITY`
   * :kconfig:`CONFIG_BT_REMOTE_VERSION`
   * :kconfig:`CONFIG_BT_SMP`
   * :kconfig:`CONFIG_BT_CONN`
   * :kconfig:`CONFIG_BT_REMOTE_INFO`
   * :kconfig:`CONFIG_BT_FIXED_PASSKEY`
   * :kconfig:`CONFIG_BT_SMP_APP_PAIRING_ACCEPT`
   * :kconfig:`CONFIG_BT_EXT_ADV`
   * :kconfig:`CONFIG_BT_OBSERVER`
   * :kconfig:`CONFIG_BT_ECC`
   * :kconfig:`CONFIG_BT_DEVICE_NAME_DYNAMIC`
   * :kconfig:`CONFIG_BT_SMP_SC_PAIR_ONLY`
   * :kconfig:`CONFIG_BT_PER_ADV`
   * :kconfig:`CONFIG_BT_PER_ADV_SYNC`
   * :kconfig:`CONFIG_BT_MAX_PAIRED`
   * :kconfig:`CONFIG_BT_SETTINGS_CCC_LAZY_LOADING`
   * :kconfig:`CONFIG_BT_BROADCASTER`
   * :kconfig:`CONFIG_BT_SETTINGS`
   * :kconfig:`CONFIG_BT_MAX_CONN`
   * :kconfig:`CONFIG_BT_ID_MAX`
   * :kconfig:`CONFIG_BT_EXT_ADV_MAX_ADV_SET`
   * :kconfig:`CONFIG_BT_DEVICE_NAME_MAX`
   * :kconfig:`CONFIG_BT_DEVICE_NAME_MAX`
   * :kconfig:`CONFIG_BT_PER_ADV_SYNC_MAX`
   * :kconfig:`CONFIG_BT_DEVICE_NAME`
   * :kconfig:`CONFIG_CBKPROXY_OUT_SLOTS` on one core must be equal to :kconfig:`CONFIG_CBKPROXY_IN_SLOTS` on the other.

To keep all the above configuration options in sync, create an overlay file that is shared between the application and network core.
Then, you can invoke build command like this:

.. parsed-literal::
   :class: highlight

   west build -b *board* -- -DCONFIG_OVERLAY=my_overlay_file.conf

API documentation
*****************

This library does not define a new API.
Instead, it uses Zephyr's Bluetooth API.
The |NCS| currently supports serialization of the following:

* :ref:`zephyr:bt_gap`
* :ref:`zephyr:bluetooth_connection_mgmt`
* :ref:`zephyr:bt_gatt`
* :ref:`Bluetooth Cryptography <zephyr:bt_crypto>`

The behavior of the implementation is almost the same as Zephyr's with the following exceptions:

* The latency is longer because of the overhead for exchanging messages between cores.
* The :c:func:`bt_gatt_cancel` function is not implemented.
* The ``flags`` field of  the :c:struct:`bt_gatt_subscribe_params` structure is atomic, so it cannot be correctly handled by the nRF RPC.
  The library implements the following workaround for it:

  * All ``flags`` are sent to the network core when either the :c:func:`bt_gatt_subscribe` or :c:func:`bt_gatt_resubscribe` function is called.
    This covers most of the cases, because the ``flags`` are normally set once before those functions calls.
  * If you want to read or write the ``flags`` after the subscription, you have to call :c:func:`bt_rpc_gatt_subscribe_flag_set`, :c:func:`bt_rpc_gatt_subscribe_flag_clear` or :c:func:`bt_rpc_gatt_subscribe_flag_get`.
