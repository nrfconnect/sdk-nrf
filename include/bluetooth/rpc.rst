.. _ble_rpc:

Bluetooth Low Energy Remote Procedure Call
##########################################

.. contents::
   :local:
   :depth: 2

The nRF Connect SDK supports Bluetooth Low Energy (LE) stack serialization.
The full Bluetooth LE stack can run on another device or CPU, such as the nRF5340 DK network core using :ref:`nrfxlib:nrf_rpc`.

.. note::
   The |NCS| currently supports serialization of the :ref:`zephyr:bt_gap` and the :ref:`zephyr:bluetooth_connection_mgmt` only.
   Due to the limited support, a special :option:`CONFIG_SUPPORT_BT_RPC` option was added and it allows enabling the :option:`CONFIG_BT_RPC` option.
   Samples using other Bluetooth LE features, such as GATT, are currently not supported and they have the :option:`CONFIG_SUPPORT_BT_RPC` option disabled.
   If you want to use the :option:`CONFIG_BT_RPC` option in your application, you must create a Kconfig file with following content:

   .. code-block:: none

      config SUPPORT_BT_RPC
        bool
        default y

      source "Kconfig.zephyr"

Network core
************

The :ref:`ble_rpc_host` sample is designed specifically to enable the Bluetooth LE stack functionality on a remote MCU (for example, the nRF5340 network core) using the :ref:`nrfxlib:nrf_rpc`.
You can program this sample to the network core to run standard Bluetooth Low Energy samples on nRF5340.
You can use either the SoftDevice Controller or the Zephyr Bluetooth LE Controller for this sample.

Application core
****************

To use the Bluetooth LE stack through nRF RPC, an additional configuration is needed.
When building samples for the application core, enable the :option:`CONFIG_BT_RPC` to run the Bluetooth LE stack on the network core.
This option builds :ref:`ble_rpc_host` automatically as a child image.
For more details, see: :ref:`ug_nrf5340_building`.

Open a command prompt in the build folder of the application sample and enter the following command to build the application for the application core, with :ref:`ble_rpc_host` as child image:

.. code-block:: console

   west build -b nrf5340dk_nrf5340_cpuapp -- -DCONFIG_BT_RPC=y

Requirements
************

Some configuration options related to Bluetooth LE must be the same on the host (network core) and client (application core).
Set the following options in the same way for the :ref:`ble_rpc_host` and application core sample:

   * :option:`CONFIG_BT_CENTRAL`
   * :option:`CONFIG_BT_PERIPHERAL`
   * :option:`CONFIG_BT_WHITELIST`
   * :option:`CONFIG_BT_USER_PHY_UPDATE`
   * :option:`CONFIG_BT_USER_DATA_LEN_UPDATE`
   * :option:`CONFIG_BT_PRIVACY`
   * :option:`CONFIG_BT_SCAN_WITH_IDENTITY`
   * :option:`CONFIG_BT_REMOTE_VERSION`
   * :option:`CONFIG_BT_SMP`
   * :option:`CONFIG_BT_CONN`
   * :option:`CONFIG_BT_REMOTE_INFO`
   * :option:`CONFIG_BT_FIXED_PASSKEY`
   * :option:`CONFIG_BT_SMP_APP_PAIRING_ACCEPT`
   * :option:`CONFIG_BT_EXT_ADV`
   * :option:`CONFIG_BT_OBSERVER`
   * :option:`CONFIG_BT_ECC`
   * :option:`CONFIG_BT_DEVICE_NAME_DYNAMIC`
   * :option:`CONFIG_BT_SMP_SC_PAIR_ONLY`
   * :option:`CONFIG_BT_PER_ADV`
   * :option:`CONFIG_BT_PER_ADV_SYNC`
   * :option:`CONFIG_BT_MAX_CONN`
   * :option:`CONFIG_BT_ID_MAX`
   * :option:`CONFIG_BT_EXT_ADV_MAX_ADV_SET`
   * :option:`CONFIG_BT_DEVICE_NAME_MAX`
   * :option:`CONFIG_BT_DEVICE_NAME_MAX`
   * :option:`CONFIG_BT_PER_ADV_SYNC_MAX`
   * :option:`CONFIG_BT_DEVICE_NAME`
   * :option:`CONFIG_CBKPROXY_OUT_SLOTS` on one core must be equal to :option:`CONFIG_CBKPROXY_IN_SLOTS` on the other.

To keep all the above configuration options in sync, create an overlay file that is shared between the application and network core.
Then, you can invoke build command like this:

.. parsed-literal::
   :class: highlight

   west build -b *board* -- -DCONFIG_OVERLAY=my_overlay_file.conf
