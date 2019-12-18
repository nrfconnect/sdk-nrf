.. _bluetooth_guide:

Bluetooth Configuration
#######################

The nRF52 Desktop devices use the Zephyr Bluetooth API (:ref:`zephyr:bluetooth`) to handle BLE connections.
Mentioned API is used only by the modules, that are dedicated to handle the BLE connections.
Responsibilities are split between these modules.
Information about peer and connection state is propagated to other application modules using :ref:`event_manager` events.

There are two types of Bluetooth devices in nRF52 Desktop:

* **Peripheral (mouse or keyboard)**

  Handles one connection at a time.
  Uses more than one Bluetooth local identity.

* **Central (dongle)**

  Handles multiple connections simultaneously.
  Uses one Bluetooth local identity (the default one).

Some configuration options and application modules are common for every nRF52 Desktop device, but both Central and Peripheral devices also have dedicated configuration options and use dedicated modules.

There are no nRF52 Desktop devces which are Bluetooth Central and Peripheral simultaneously.

Common
******

In order to use Bluetooth, it has to be enabled and configured.
The Zephyr Bluetooth is configured using the Kconfig options.

Configuration
=============

Bluetooth Kconfig options common for nRF52 Desktop devices are described below.
Detailed information is available as Kconfig help.

* :option:`CONFIG_BT`
* :option:`CONFIG_BT_SETTINGS`
* :option:`CONFIG_BT_SMP`
* :option:`CONFIG_BT_SIGNING`
* :option:`CONFIG_BT_MAX_PAIRED`

  For Bluetooth Central, maximum number of paired devices is equal to maximum number of simultaneously connected peers.
  For Bluetooth Peripheral, maximum number of paired devices is equal to number of peers increased by 1 (one additional paired device slot is used for erase advertising).

* :option:`CONFIG_BT_ID_MAX`

  For Bluetooth Peripheral, number of Bluetooth local identities must be equal to number of peers increased by 2.
  One additional local identity is used for erase advertising and the default local identity is unused, because it cannot be reset.
  Bluetooth Central devies use only one Bluetooth local identity - the default one.

* :option:`CONFIG_BT_MAX_CONN`

  For Bluetooth Central, this option must be set to maximum number of simultaneously connected devices.
  For Bluetooth Peripheral, the default value (one) is used.

* :option:`CONFIG_BT_DEVICE_NAME`
* :option:`CONFIG_BT_DEVICE_APPEARANCE`
* :option:`CONFIG_BT_CTLR`
* :option:`CONFIG_BT_AUTO_PHY_UPDATE` (disabled)

  Workaround to make sure peripheral works correctly with Android devces.

* :option:`CONFIG_BT_CTLR_TX_PWR_PLUS_8`

  Select Bluetooth Radio transmit power level.

Some unused functionalities were also disabled to reduce the memory usage:

* :option:`CONFIG_BT_CTLR_CONN_PARAM_REQ`
* :option:`CONFIG_BT_DATA_LEN_UPDATE`

Link Layer
----------

Zephyr gives possibility to choose the Bluetooth Link Layer.
nRF52 Desktop uses one of the following link layers:

* :option:`CONFIG_BT_LL_SW_LEGACY`

  - used for desktop mice (lower memory consumption)
  - no LLPM support

* :option:`CONFIG_BT_LL_NRFXLIB`

  - used for gaming mouse, keyboard and dongle
  - supports LLPM
  - memory consumption will be lowered in the future releases (work in progress)

Application modules
===================

Every nRF52 device, which uses Bluetooth has to handle connections and manage bonds.
These features are implemented by the following modules:

* :ref:`ble_state` - enables Bluetooth and LLPM (if supported), handles connection callbacks.
* :ref:`ble_bond` - manages Bluetooth bonds and local identities.

Bluetooth Peripheral
********************

nRF52 Desktop Peripheral devices must include additional configuration options and additional application modules to comply with the HID over GATT specification.

Configuration
=============

Defning Bluetooth Peripheral device requires the following configuration options:

* :option:`CONFIG_BT_PERIPHERAL`
* :option:`CONFIG_BT_PERIPHERAL_PREF_MIN_INT`, :option:`CONFIG_BT_PERIPHERAL_PREF_MAX_INT`, :option:`CONFIG_BT_PERIPHERAL_PREF_SLAVE_LATENCY`, :option:`CONFIG_BT_PERIPHERAL_PREF_TIMEOUT`

  When left undefined, default values will be used.

* :option:`CONFIG_BT_CONN_PARAM_UPDATE_TIMEOUT`
* :option:`CONFIG_BT_WHITELIST`

  Select this option to enable whitelisting incoming connection requests in :ref:`ble_adv` module.

GATT Server configuration
-------------------------

HID over GATT profile specification requires from Bluetooth Peripherals defining the following GATT services:

* HID service

  Implemented in :ref:`hids` application module.

* Battery Service

  Implemented in :ref:`bas` appplication module.

* Device Information Service

  Implemented in Zephyr, enabled using :option:`CONFIG_BT_GATT_DIS`.
  Configured using Kconfig options dedicated for the component.
  There is no application module dedicated to handle the DIS.

nRF52 Desktop Periperals must also define dedicated GATT Service used to inform if the device supports LLPM (implemented in :ref:`dev_descr` application module).

Use the following options to configure GATT for the device:

* :option:`CONFIG_BT_GATT_UUID16_POOL_SIZE`
* :option:`CONFIG_BT_GATT_CHRC_POOL_SIZE`
* :option:`CONFIG_BT_SETTINGS_CCC_STORE_ON_WRITE`
* :option:`CONFIG_BT_SETTINGS_CCC_LAZY_LOADING`

Application modules
===================

Apart from mentioned :ref:`hids`, :ref:`bas` and :ref:`dev_descr` modules, Peripheral device must define the following modules:

* :ref:`ble_adv` - the module is used to control Bluetooth advertising.
* :ref:`ble_latency` - the module is used to control the Bluetooth connection latency (keep the latency low when :ref:`config_channel` is in use).

Bluetooth Central
*****************

Defining Bluetooth Central device, in addition to common configuration options, requires the following configuration:

* :option:`CONFIG_BT_CENTRAL`
* :option:`CONFIG_BT_CTLR_TX_BUFFER_SIZE`, :option:`CONFIG_BT_CTLR_DATA_LENGTH_MAX`

  Bigger buffer size and data length are required by :ref:`config_channel`.

* :option:`CONFIG_BLECTRL_MAX_CONN_EVENT_LEN_DEFAULT`

  Max connection event length should be set to 2500us to ensure that multiple BLE Peripherals will be able to exchange the data during single connection interval.
  By default this option is set to 7500us, which would result in only one data exchange per connection interval - this lowers Peripherals' report rates.

For Bluetooth Central, enable and configure the following modules:

* :ref:`ble_scan` - manages scanning for the Bluetooth Peripheral devices.
* :ref:`ble_discovery` - discovers and reads GATT characteristics defined by Peripheral.
* :ref:`hid_forward` - subscribes for HID reports from the Bluetooth Peripherals (HID over GATT) and forwards the data using application events.
* :ref:`ble_qos` (optional) - used to avoid "crowded channels" to achieve better connection quality (higher report rate).
