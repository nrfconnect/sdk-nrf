.. _nrf_desktop_bluetooth_guide:

nRF Desktop: Bluetooth
######################

.. contents::
   :local:
   :depth: 2

The nRF Desktop devices use :ref:`Zephyr's Bluetooth API <zephyr:bluetooth>` to handle the Bluetooth LE connections.

This API is used only by the application modules that handle such connections.
The information about peer and connection state is propagated to other application modules using :ref:`app_event_manager` events.

The :ref:`CONFIG_DESKTOP_BT <config_desktop_app_options>` Kconfig option enables support for Bluetooth connectivity in the nRF Desktop.
Specific Bluetooth configurations and application modules are selected or implied according to the HID device role.
Apart from that, the defaults of Bluetooth-related Kconfigs are aligned with the nRF Desktop use case.

The nRF Desktop devices come in the following roles:

* HID peripheral (:ref:`CONFIG_DESKTOP_ROLE_HID_PERIPHERAL <config_desktop_app_options>`) that works as a Bluetooth Peripheral (:ref:`CONFIG_DESKTOP_BT_PERIPHERAL <config_desktop_app_options>`)

  * Support only the Bluetooth Peripheral role (:kconfig:option:`CONFIG_BT_PERIPHERAL`).
  * Handle only one Bluetooth LE connection at a time.
  * Use more than one Bluetooth local identity.

* HID dongle (:ref:`CONFIG_DESKTOP_ROLE_HID_DONGLE <config_desktop_app_options>`) that works as a Bluetooth Central (:ref:`CONFIG_DESKTOP_BT_CENTRAL <config_desktop_app_options>`)

  * Support only the Bluetooth Central role (:kconfig:option:`CONFIG_BT_CENTRAL`).
  * Handle multiple Bluetooth LE connections simultaneously.
  * Use only one Bluetooth local identity (the default one).

Both central and peripheral devices have dedicated configuration options and use dedicated modules.

The nRF Desktop peripheral configurations that enable `Fast Pair`_ support use a slightly different Bluetooth configuration.
This is needed to improve the user experience.
See :ref:`nrf_desktop_bluetooth_guide_fast_pair` for more details.

.. note::
   An nRF Desktop device cannot support both central and peripheral roles.

Common configuration and application modules
********************************************

Some Bluetooth-related :ref:`configuration options <nrf_desktop_bluetooth_guide_configuration>` (including :ref:`nrf_desktop_bluetooth_guide_configuration_ll` in a separate section) and :ref:`application modules <nrf_desktop_bluetooth_guide_modules>` are common for every nRF Desktop device.

.. _nrf_desktop_bluetooth_guide_configuration:

Configuration options
=====================

This section describes the most important Bluetooth Kconfig options common for all nRF Desktop devices.
For detailed information about every option, see the Kconfig help.

* :kconfig:option:`CONFIG_BT_MAX_PAIRED`

  * nRF Desktop central: The maximum number of paired devices is greater than or equal to the maximum number of simultaneously connected peers.
    The :kconfig:option:`CONFIG_BT_MAX_PAIRED` is by default set to :ref:`CONFIG_DESKTOP_HID_DONGLE_BOND_COUNT <config_desktop_app_options>`.
  * nRF Desktop peripheral: The maximum number of paired devices is equal to the number of peers plus one, where the one additional paired device slot is used for erase advertising.

* :kconfig:option:`CONFIG_BT_ID_MAX`

  * nRF Desktop central: The device uses only one Bluetooth local identity, that is the default one.
  * nRF Desktop peripheral: The number of Bluetooth local identities must be equal to the number of peers plus two.

    * One additional local identity is used for erase advertising.
    * The other additional local identity is the default local identity, which is unused, because it cannot be reset after removing the bond.
      Without the identity reset, the previously bonded central could still try to reconnect after being removed from Bluetooth bonds on the peripheral side.

* :kconfig:option:`CONFIG_BT_MAX_CONN`

  * nRF Desktop central: This option is set to the maximum number of simultaneously connected devices.
    The :kconfig:option:`CONFIG_BT_MAX_CONN` is by default set to :ref:`CONFIG_DESKTOP_HID_DONGLE_CONN_COUNT <config_desktop_app_options>`.
  * nRF Desktop peripheral: The default value (one) is used.

.. note::
   After changing the number of Bluetooth peers for the nRF Desktop peripheral device, update the LED effects used to represent the Bluetooth connection state.
   For details, see :ref:`nrf_desktop_led_state`.

.. _nrf_desktop_bluetooth_guide_configuration_ll:

Link Layer configuration options
================================

The nRF Desktop devices use one of the following Link Layers:

* :kconfig:option:`CONFIG_BT_LL_SW_SPLIT`
    This Link Layer does not support the Low Latency Packet Mode (LLPM) and has a lower memory usage, so it can be used by memory-limited devices.

* :kconfig:option:`CONFIG_BT_LL_SOFTDEVICE`
    This Link Layer does support the Low Latency Packet Mode (LLPM).
    If you opt for this Link Layer and enable the :kconfig:option:`CONFIG_BT_CTLR_SDC_LLPM`, the :kconfig:option:`CONFIG_CAF_BLE_USE_LLPM` is also enabled by default and can be configured further:

    * When :kconfig:option:`CONFIG_CAF_BLE_USE_LLPM` is enabled, set the value for :kconfig:option:`CONFIG_BT_CTLR_SDC_MAX_CONN_EVENT_LEN_DEFAULT` to ``3000``.

      This is required by the nRF Desktop central and helps avoid scheduling conflicts with the Bluetooth Link Layer.
      Such conflicts could lead to a drop in HID input report rate or a disconnection.
      Because of this, if the nRF Desktop central supports LLPM and more than one simultaneous Bluetooth connection, it also uses 10 ms connection interval instead of 7.5 ms.
      Setting the value of :kconfig:option:`CONFIG_BT_CTLR_SDC_MAX_CONN_EVENT_LEN_DEFAULT` to ``3000`` also enables the nRF Desktop central to exchange data with up to three standard Bluetooth LE peripherals during every connection interval (every 10 ms).

    * When :kconfig:option:`CONFIG_CAF_BLE_USE_LLPM` is disabled, the device will use only standard Bluetooth LE connection parameters with the lowest available connection interval of 7.5 ms.

      If the LLPM is disabled and more than two simultaneous Bluetooth connections are supported (:kconfig:option:`CONFIG_BT_MAX_CONN`), you can set the value for :kconfig:option:`CONFIG_BT_CTLR_SDC_MAX_CONN_EVENT_LEN_DEFAULT` to ``2500``.
      With this value, the nRF Desktop central can exchange the data with up to three Bluetooth LE peripherals during every 7.5-ms connection interval.
      Using the value of ``3000`` for more than two simultaneous Bluetooth LE connections will result in a lower HID input report rate.

.. _nrf_desktop_bluetooth_guide_modules:

Application modules
===================

Every nRF Desktop device that enables Bluetooth must handle connections and manage bonds.
These features are implemented by the following modules:

* :ref:`nrf_desktop_ble_state` - Enables Bluetooth and LLPM (if supported), and handles Zephyr connection callbacks.
* :ref:`nrf_desktop_ble_bond` - Manages Bluetooth bonds and local identities.

You need to enable all these modules to enable both features.
For information about how to enable the modules, see their respective documentation pages.

Optionally, you can also enable the following module:

* :ref:`nrf_desktop_ble_qos` - Helps achieve better connection quality and higher report rate.
  The module can be used only with the SoftDevice Link Layer.

.. note::
   The nRF Desktop devices enable the :kconfig:option:`CONFIG_BT_SETTINGS` Kconfig option.
   When this option is enabled, the application is responsible for calling the :c:func:`settings_load` function - this is handled by the :ref:`nrf_desktop_settings_loader`.

.. _nrf_desktop_bluetooth_guide_peripheral:

Bluetooth Peripheral
********************

The nRF Desktop peripheral devices must include additional configuration options and additional application modules to comply with the HID over GATT specification.

The HID over GATT profile specification requires Bluetooth Peripherals to define the following GATT Services:

* HID Service - Handled in the :ref:`nrf_desktop_hids`.
* Battery Service - Handled in the :ref:`nrf_desktop_bas`.
* Device Information Service - Implemented in Zephyr and enabled with the :kconfig:option:`CONFIG_BT_DIS` Kconfig option.
  The device identifiers are configured according to the common :ref:`nrf_desktop_hid_device_identifiers` by default.
  You can configure it using Kconfig options with the ``CONFIG_BT_DIS`` prefix.

The nRF Desktop peripherals must also define a dedicated GATT Service that is used to provide the following information:

* Information on whether the device can use the LLPM Bluetooth connection parameters.
* Hardware ID of the peripheral.

The GATT Service is implemented by the :ref:`nrf_desktop_dev_descr`.

Apart from the GATT Services, an nRF Desktop peripheral device must enable and configure the following application modules:

* :ref:`nrf_desktop_ble_adv` - Controls the Bluetooth advertising.
* :ref:`nrf_desktop_ble_latency` - Keeps the connection latency low when the :ref:`nrf_desktop_config_channel` is used or when either the :ref:`nrf_desktop_ble_smp` or :ref:`nrf_desktop_dfu_mcumgr` receives an update image.
  This is done to ensure quick data transfer.

Optionally, you can also enable the following module:

* :ref:`nrf_desktop_qos` - Forwards the Bluetooth LE channel map generated by :ref:`nrf_desktop_ble_qos`.
  The Bluetooth LE channel map is forwarded using GATT characteristic.
  The Bluetooth Central can apply the channel map to avoid congested RF channels.
  This results in better connection quality and a higher report rate.

.. _nrf_desktop_bluetooth_guide_fast_pair:

Fast Pair
=========

The nRF Desktop peripheral can be built with Google `Fast Pair`_ support.
The configurations that enable Fast Pair are specified in the files with filenames ending with the ``fast_pair`` and ``release_fast_pair`` suffixes.

.. note::
   The Fast Pair integration in the nRF Desktop is :ref:`experimental <software_maturity>`.
   The factory reset of the Fast Pair non-volatile data is not yet supported.

   The Fast Pair support in the |NCS| is :ref:`experimental <software_maturity>`.
   See :ref:`ug_bt_fast_pair` for details.

These configurations support multiple bonds for each Bluetooth local identity (:kconfig:option:`CONFIG_CAF_BLE_STATE_MAX_LOCAL_ID_BONDS` is set to ``3``) and erase advertising (:ref:`CONFIG_DESKTOP_BLE_PEER_ERASE <config_desktop_app_options>`), but Bluetooth peer selection (:ref:`CONFIG_DESKTOP_BLE_PEER_SELECT <config_desktop_app_options>`) is disabled.
You can now pair with your other hosts without switching the peripheral back in pairing mode (without triggering the erase advertising).
The nRF Desktop peripheral that integrates Fast Pair behaves as follows:

  * The dongle peer does not use the Fast Pair advertising payload.
  * The bond erase operation is enabled for the dongle peer.
    This lets you change the bonded Bluetooth Central.
  * If the dongle peer (:ref:`CONFIG_DESKTOP_BLE_DONGLE_PEER_ENABLE <config_desktop_app_options>`) is enabled, the `Swift Pair`_ payload is, by default, included only for the mentioned peer.
    In the Fast Pair configurations, the dongle peer is intended to be used for all of the peers that are not Fast Pair Seekers.
  * If the used Bluetooth local identity has no bonds, the device advertises in pairing mode, and the Fast Pair discoverable advertising is used.
    This allows to pair with the nRF Desktop device using both Fast Pair and normal Bluetooth pairing flows.
    This advertising payload is also used during the erase advertising.
  * If the used Bluetooth local identity already has a bond, the device is no longer in the pairing mode and the Fast Pair not discoverable advertising is used.
    This allows to pair only with the Fast Pair Seekers linked to Google Accounts that are already associated with the nRF Desktop device.
    In this mode, the device rejects normal Bluetooth pairing by default (:ref:`CONFIG_DESKTOP_FAST_PAIR_LIMIT_NORMAL_PAIRING <config_desktop_app_options>` option is enabled).
    The Fast Pair UI indication is hidden after the Provider reaches :kconfig:option:`CONFIG_CAF_BLE_STATE_MAX_LOCAL_ID_BONDS` bonded peers on the used local identity.
  * The :ref:`nrf_desktop_factory_reset` is enabled by default if the :ref:`nrf_desktop_config_channel` is supported by the device.
    The factory reset operation removes both Fast Pair and Bluetooth non-volatile data.
    The factory reset operation is triggered using the configuration channel.

After a successful erase advertising procedure, the peripheral removes all of the bonds of a given Bluetooth local identity.

Apart from that, the following changes are applied in configurations that support Fast Pair:

* The ``SB_CONFIG_BT_FAST_PAIR`` Kconfig option is enabled in the sysbuild configuration.
  For more details about enabling Fast Pair for your application, see the :ref:`ug_bt_fast_pair_prerequisite_ops_kconfig` section in the Fast Pair integration guide.
* The static :ref:`partition_manager` configuration is modified to introduce a dedicated non-volatile memory partition used to store the Fast Pair provisioning data.
* Bluetooth privacy feature (:kconfig:option:`CONFIG_BT_PRIVACY`) is enabled.
* The fast and slow advertising intervals defined in the :ref:`nrf_desktop_ble_adv` are aligned with Fast Pair expectations.
* The Bluetooth advertising filter accept list (:kconfig:option:`CONFIG_CAF_BLE_ADV_FILTER_ACCEPT_LIST`) is disabled to allow Fast Pair Seekers other than the bonded one to connect outside of the pairing mode.
* The security failure timeout (:ref:`CONFIG_DESKTOP_BLE_SECURITY_FAIL_TIMEOUT_S <config_desktop_app_options>`) is longer to prevent disconnections during the Fast Pair procedure.
* Passkey authentication (:ref:`CONFIG_DESKTOP_BLE_ENABLE_PASSKEY <config_desktop_app_options>`) is disabled on the keyboard.
  Currently, Fast Pair does not support devices that use a screen or keyboard for Bluetooth authentication.
* TX power correction value (:kconfig:option:`CONFIG_BT_ADV_PROV_TX_POWER_CORRECTION_VAL`) is configured to align the TX power included in the advertising data with the Fast Pair expectations.

See :ref:`ug_bt_fast_pair` for detailed information about Fast Pair support in the |NCS|.

Bluetooth Central
*****************

The nRF Desktop central must implement Bluetooth scanning and handle the GATT operations.
The central must also control the Bluetooth connection parameters.
These features are implemented by the following application modules:

* :ref:`nrf_desktop_ble_scan` - Controls the Bluetooth scanning.
* :ref:`nrf_desktop_ble_conn_params` - Controls the Bluetooth connection parameters and reacts on latency update requests received from the connected peripherals.
* :ref:`nrf_desktop_ble_discovery` - Handles discovering and reading the GATT Characteristics from the connected peripheral.
* :ref:`nrf_desktop_hid_forward` - Subscribes for HID reports from the Bluetooth Peripherals (HID over GATT) and forwards data using application events.
