.. _bt_fast_pair_readme:

Google Fast Pair Service (GFPS)
###############################

.. contents::
   :local:
   :depth: 2

The Google Fast Pair Service (Fast Pair for short) implements a Bluetooth® Low Energy (LE) GATT Service required for :ref:`ug_bt_fast_pair` with the |NCS|.

Service UUID
************

The Fast Pair service uses UUID of ``0xFE2C``.

Characteristics
***************

The Fast Pair GATT characteristics are described in detail in the `Fast Pair GATT Characteristics`_ documentation.
The implementation in the |NCS| follows these requirements.

The Fast Pair service also contains additional GATT characteristics under the following conditions:

* The Additional Data GATT characteristic is enabled when an extension requires it.
  Currently, only the Personalized Name extension (:kconfig:option:`CONFIG_BT_FAST_PAIR_PN`) requires this characteristic.
* The Beacon Actions GATT characteristic when the Find My Device Network (FMDN) extension is enabled (:kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN`).
  The characteristic is described in the `Fast Pair Find My Device Network extension`_ documentation.

Configuration
*************

The Fast Pair Service is enabled with :kconfig:option:`CONFIG_BT_FAST_PAIR` Kconfig option set in the main application image.

.. note::
   When building with sysbuild, value of the :kconfig:option:`CONFIG_BT_FAST_PAIR` Kconfig option is overwritten by ``SB_CONFIG_BT_FAST_PAIR``.
   For more details about enabling Fast Pair for your application, see the :ref:`ug_bt_fast_pair_prerequisite_ops_kconfig` section in the Fast Pair integration guide.

With the :kconfig:option:`CONFIG_BT_FAST_PAIR` Kconfig option enabled, the following Kconfig options are available for this service:

* :kconfig:option:`CONFIG_BT_FAST_PAIR_GATT_SERVICE_MODEL_ID` - The option adds the Model ID characteristic to the Fast Pair GATT service.
  This option is enabled by default unless the :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN` is enabled.
  This is done to align default configuration with `Fast Pair Device Feature Requirements for Locator Tags`_ documentation.
* :kconfig:option:`CONFIG_BT_FAST_PAIR_REQ_PAIRING` - The option enforces the requirement for Bluetooth pairing and bonding during the `Fast Pair Procedure`_.
  This option is enabled by default.
  See the :ref:`ug_bt_fast_pair_gatt_service_no_ble_pairing` for more details.
* :kconfig:option:`CONFIG_BT_FAST_PAIR_STORAGE_USER_RESET_ACTION` - The option enables user reset action that is executed together with the Fast Pair factory reset operation.
  See the :ref:`ug_bt_fast_pair_factory_reset_custom_user_reset_action` for more details.
* :kconfig:option:`CONFIG_BT_FAST_PAIR_STORAGE_ACCOUNT_KEY_MAX` - The option configures maximum number of stored Account Keys.
* :kconfig:option:`CONFIG_BT_FAST_PAIR_CRYPTO_TINYCRYPT`, :kconfig:option:`CONFIG_BT_FAST_PAIR_CRYPTO_MBEDTLS`, :kconfig:option:`CONFIG_BT_FAST_PAIR_CRYPTO_OBERON`, and :kconfig:option:`CONFIG_BT_FAST_PAIR_CRYPTO_PSA` - These options are used to select the cryptographic backend for Fast Pair.
  The Oberon backend is used by default.
  The Mbed TLS backend uses Mbed TLS crypto APIs that are now considered legacy APIs.
* :kconfig:option:`CONFIG_BT_FAST_PAIR_PN` - The option enables the `Fast Pair Personalized Name extension`_.
  This option is enabled by default unless the :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN` is enabled.
  This is done to align default configuration with `Fast Pair Device Feature Requirements for Locator Tags`_ documentation.

  * :kconfig:option:`CONFIG_BT_FAST_PAIR_STORAGE_PN_LEN_MAX` - The option specifies the maximum length of a stored Fast Pair Personalized Name.

* :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN` - The option enables the FMDN extension.

  * :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_DULT` - The option enables the Detecting Unwanted Location Trackers (DULT) support in the FMDN extension (see :ref:`ug_bt_fast_pair_prerequisite_ops_fmdn_dult_integration`):

    * :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_DULT_MANUFACTURER_NAME` - The option configures the manufacturer name parameter.
    * :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_DULT_MODEL_NAME` - The option configures the model name parameter.
    * :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_DULT_ACCESSORY_CATEGORY` - The option configures the accessory category parameter.
    * :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_DULT_FIRMWARE_VERSION_MAJOR`, :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_DULT_FIRMWARE_VERSION_MINOR` and :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_DULT_FIRMWARE_VERSION_REVISION` - These options configure the firmware version parameter.

  * There are following advertising configuration options for the FMDN extension (see :ref:`ug_bt_fast_pair_advertising_fmdn`):

    * :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_TX_POWER` - The option sets the TX power (dBm) in the Bluetooth LE controller for FMDN advertising and connections.
    * :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_TX_POWER_CORRECTION_VAL` - The value of this option is added to the TX power readout from the Bluetooth LE controller to calculate the calibrated TX power reported in the Read Beacon Parameters response.
    * :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_MAX_CONN` - The option configures a maximum number of FMDN connections.
      This option is bounded by the :kconfig:option:`CONFIG_BT_MAX_CONN` and cannot exceed its value.
    * :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_ECC_SECP160R1` and :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_ECC_SECP256R1` - These options are used to select the elliptic curve for calculating the FMDN advertising payload.
      The secp160r1 elliptic curve is enabled by default.

  * There are following battery configuration options for the FMDN extension (see :ref:`ug_bt_fast_pair_advertising_fmdn_battery` and :ref:`ug_bt_fast_pair_gatt_service_fmdn_battery_dult`):

    * :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_BATTERY_LEVEL_LOW_THR` - The option configures the threshold percentage value for entering the low battery state as defined in the FMDN extension.
    * :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_BATTERY_LEVEL_CRITICAL_THR` - The option configures the threshold percentage value for entering the critically low battery state as defined in the FMDN extension.
    * :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_BATTERY_DULT` - The option configures the FMDN module to pass the battery information to the DULT module and to support its mechanism for providing battery information to the connected peers.
      This option can only be used when the :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_DULT` Kconfig option is enabled.

  * There are following read mode configuration options for the FMDN extension (see :ref:`ug_bt_fast_pair_gatt_service_fmdn_read_mode_callbacks`):

    * :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_READ_MODE_FMDN_RECOVERY_TIMEOUT` - The option configures the Ephemeral Identity Key (EIK) recovery mode timeout in minutes.

  * There are following ringing configuration options for the FMDN extension (see :ref:`ug_bt_fast_pair_gatt_service_fmdn_ring_callbacks`):

    * :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_RING_COMP_NONE`, :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_RING_COMP_ONE`, :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_RING_COMP_TWO`, and :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_RING_COMP_THREE` - These options are used to select the set of ringing components.
      The option with no ringing component is enabled by default.
    * :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_RING_VOLUME` - The option enables ringing volume support.
    * :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_RING_REQ_TIMEOUT_DULT_BT_GATT` - The option configures the ringing timeout for connected peers that use DULT-based ringing mechanism.
      This option can only be used when the :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_DULT` is enabled.

  * There are following beacon clock service configuration options for the FMDN extension (see :ref:`ug_bt_fast_pair_prerequisite_ops_fmdn_clock_svc`):

    * :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_CLOCK_NVM_UPDATE_TIME` - The option configures the time interval (in minutes) of periodic beacon clock writes to the non-volatile memory.
    * :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_CLOCK_NVM_UPDATE_RETRY_TIME` - The option configures the retry time (in seconds) when the beacon clock write to the non-volatile memory fails.

See the Kconfig help for details.

Override of default Kconfig values
==================================

To simplify the configuration process, the GFPS modifies the default values of related Kconfig options to meet the Fast Pair requirements.
The service also enables some of the functionalities using Kconfig select statement.

Bluetooth privacy
-----------------

The service selects Bluetooth privacy (:kconfig:option:`CONFIG_BT_PRIVACY`).

During not discoverable advertising, the Resolvable Private Address (RPA) rotation must be done together with the Fast Pair payload update.
Because of this, the RPA cannot be rotated by Zephyr in the background.

During discoverable advertising session, the Resolvable Private Address (RPA) rotation must not happen.
Therefore, consider the following points:

* Make sure that your advertising session is shorter than the value in the :kconfig:option:`CONFIG_BT_RPA_TIMEOUT` option.
* Call the :c:func:`bt_le_oob_get_local` function to trigger RPA rotation and reset the RPA timeout right before advertising starts.

.. note::
   If you use the FMDN extension, and your Provider is provisioned as an FMDN beacon, do not use the :c:func:`bt_le_oob_get_local` function.
   For more details, see the :ref:`Setting up Bluetooth LE advertising <ug_bt_fast_pair_advertising>` section of the Fast Pair integration guide.

Bluetooth Security Manager Protocol (SMP)
-----------------------------------------

The service selects the Kconfig options :kconfig:option:`CONFIG_BT_SMP`, :kconfig:option:`CONFIG_BT_SMP_APP_PAIRING_ACCEPT`, and :kconfig:option:`CONFIG_BT_SMP_ENFORCE_MITM`.
The Fast Pair specification requires support for Bluetooth® Low Energy pairing and enforcing :term:`Man-in-the-Middle (MITM)` protection during the Fast Pair procedure.

Firmware Revision characteristic
--------------------------------

The Fast Pair specification requires enabling GATT Device Information Service and the Firmware Revision characteristic.
For this reason, the default values of the Kconfig options :kconfig:option:`CONFIG_BT_DIS` and :kconfig:option:`CONFIG_BT_DIS_FW_REV`, respectively, are set to enabled.
The default value of :kconfig:option:`CONFIG_BT_DIS_FW_REV_STR` is set to :kconfig:option:`CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION` if :kconfig:option:`CONFIG_BOOTLOADER_MCUBOOT` is enabled.
The option is enforced by sysbuild when ``SB_CONFIG_BOOTLOADER_MCUBOOT`` is enabled.

Otherwise, it is set to ``0.0.0+0``.

This requirement does not apply for the locator tag use case as specified in the `Fast Pair Device Feature Requirements for Locator Tags`_ documentation.
As a result, these Kconfig overrides are not applied when the :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN` Kconfig option is enabled.

MTU configuration
-----------------

The Fast Pair specification suggests using ATT maximum transmission unit (MTU) value of ``83`` if possible.
Because of this requirement, the default values of the following Kconfig options are modified by the GFPS Kconfig:

* :kconfig:option:`CONFIG_BT_L2CAP_TX_MTU`
* :kconfig:option:`CONFIG_BT_BUF_ACL_TX_SIZE`
* :kconfig:option:`CONFIG_BT_BUF_ACL_RX_SIZE`
* :kconfig:option:`CONFIG_BT_CTLR_DATA_LENGTH_MAX`

.. tip::
   When using :ref:`nRF53 Series <ug_nrf53>` devices, this part of the configuration cannot be automatically updated for the network core and you must manually align it.
   The listed options must be set on the network core to the default values specified by the GFPS Kconfig options.

Security re-establishment
-------------------------

By default, the Fast Pair service disables the automatic security re-establishment request as a peripheral (:kconfig:option:`CONFIG_BT_GATT_AUTO_SEC_REQ`).
This allows a Fast Pair Seeker to control the security re-establishment.

Partition Manager
-----------------

The Fast Pair provisioning data is preprogrammed to a dedicated flash memory partition.
The GFPS selects the :kconfig:option:`CONFIG_PM_SINGLE_IMAGE` Kconfig option to enable the :ref:`partition_manager`.

Settings
--------

The GFPS uses Zephyr's :ref:`zephyr:settings_api` to store Account Keys and the Personalized Name.
With the FMDN extension enabled, it additionally stores the Owner Account Key, the EIK and the Beacon Clock.
Because of this, the GFPS selects the :kconfig:option:`CONFIG_SETTINGS` Kconfig option.

Bluetooth LE extended advertising for the FMDN extension
--------------------------------------------------------

The FMDN extension (see :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN`) selects the :kconfig:option:`CONFIG_BT_EXT_ADV` Kconfig option.
The extension uses the Bluetooth LE Extended Advertising Zephyr API to support simultaneous broadcast of multiple advertising sets.
In the simplest scenario, you should have the following two advertising sets in your application:

* The application-specific advertising set with the Fast Pair payload.
* The FMDN advertising set for the FMDN extension.

For more details regarding the advertising policy of the FMDN extension, see the :ref:`Setting up Bluetooth LE advertising <ug_bt_fast_pair_advertising>` section of the Fast Pair integration guide.

DULT module for the FMDN extension
----------------------------------

The :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_DULT` of the FMDN extension selects the :kconfig:option:`CONFIG_DULT` Kconfig option to enable the DULT module.
The FMDN extension implementation also acts as middleware between the user application and the DULT module.
The DULT module integration is required for small and not easily discoverable accessories.
The :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_DULT` is enabled by default.

The :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_BATTERY_DULT` of the FMDN extension selects the :kconfig:option:`CONFIG_DULT_BATTERY` Kconfig option to enable the battery support in the DULT module.
With this option enabled, the FMDN extension passes the battery information also to the DULT module.

For more details on the DULT module, see the :ref:`dult_readme` module documentation.

Implementation details
**********************

The implementation uses :c:macro:`BT_GATT_SERVICE_DEFINE` to statically define and register the Fast Pair GATT service.
The Fast Pair service automatically handles all requests received from the Fast Pair Seeker except for operations on the Beacon Actions characteristic that is part of the FMDN extension.
For more details, see the :ref:`Setting up GATT service <ug_bt_fast_pair_gatt_service>` section of the Fast Pair integration guide.

Bluetooth authentication
========================

The Bluetooth pairing is handled using a set of Bluetooth authentication callbacks (:c:struct:`bt_conn_auth_cb`).
The pairing flow and the set of Bluetooth authentication callbacks in use depend on whether the connected peer follows the Fast Pair pairing flow:

* If the peer follows the Fast Pair pairing flow, the Fast Pair service calls the :c:func:`bt_conn_auth_cb_overlay` function to automatically overlay the Bluetooth authentication callbacks.
  The function is called while handling the Key-based Pairing request.
  Overlying callbacks allow the GFPS to take over Bluetooth authentication during the `Fast Pair Procedure`_ and perform all of the required operations without interacting with the application.
* If the peer does not follow the Fast Pair pairing flow, normal Bluetooth LE pairing and global Bluetooth authentication callbacks are used.

API documentation
*****************

| Header file: :file:`include/bluetooth/services/fast_pair/fast_pair.h`
| Source files: :file:`subsys/bluetooth/services/fast_pair`

.. doxygengroup:: bt_fast_pair
   :project: nrf
   :members:

FMDN extension
==============

| Header file: :file:`include/bluetooth/services/fast_pair/fmdn.h`
| Source files: :file:`subsys/bluetooth/services/fast_pair/fmdn`

.. doxygengroup:: bt_fast_pair_fmdn
   :project: nrf
   :members:

Fast Pair UUID API
==================

| Header file: :file:`include/bluetooth/services/fast_pair/uuid.h`
| Source files: :file:`subsys/bluetooth/services/fast_pair`

.. doxygengroup:: bt_fast_pair_uuid
   :project: nrf
   :members:
