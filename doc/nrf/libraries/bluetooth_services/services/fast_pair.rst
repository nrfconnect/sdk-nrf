.. _bt_fast_pair_readme:

Google Fast Pair Service (GFPS)
###############################

.. contents::
   :local:
   :depth: 2

The Google Fast Pair Service (Fast Pair for short) implements a Bluetooth® Low Energy (LE) GATT Service required when :ref:`ug_bt_fast_pair`.

Service UUID
************

The Fast Pair service uses UUID of ``0xFE2C``.

Characteristics
***************

The Fast Pair GATT characteristics are described in detail in the `Fast Pair GATT Characteristics`_ documentation.
The implementation in the |NCS| follows these requirements.

Configuration
*************

Set the :kconfig:option:`CONFIG_BT_FAST_PAIR` Kconfig option to enable the module.

The following Kconfig options are also available for this module:

* :kconfig:option:`CONFIG_BT_FAST_PAIR_STORAGE_ACCOUNT_KEY_MAX` - The option configures maximum number of stored Account Keys.
* :kconfig:option:`CONFIG_BT_FAST_PAIR_CRYPTO_TINYCRYPT`, :kconfig:option:`CONFIG_BT_FAST_PAIR_CRYPTO_MBEDTLS`, and :kconfig:option:`CONFIG_BT_FAST_PAIR_CRYPTO_OBERON` - These options are used to select the cryptographic backend for Fast Pair.
  The Oberon backend is used by default.
  The Mbed TLS backend uses Mbed TLS crypto APIs, which are now considered legacy APIs.
* :kconfig:option:`CONFIG_BT_FAST_PAIR_PN` - The option enables the `Fast Pair Personalized Name extension`_.
* :kconfig:option:`CONFIG_BT_FAST_PAIR_STORAGE_PN_LEN_MAX` - The option specifies the maximum length of a stored Fast Pair Personalized Name.

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

Bluetooth Security Manager Protocol (SMP)
-----------------------------------------

The service selects the Kconfig options :kconfig:option:`CONFIG_BT_SMP`, :kconfig:option:`CONFIG_BT_SMP_APP_PAIRING_ACCEPT`, and :kconfig:option:`CONFIG_BT_SMP_ENFORCE_MITM`.
The Fast Pair specification requires support for Bluetooth® Low Energy pairing and enforcing :term:`Man-in-the-Middle (MITM)` protection during the Fast Pair procedure.

Firmware Revision characteristic
--------------------------------

The Fast Pair specification requires enabling GATT Device Information Service and the Firmware Revision characteristic.
For this reason, the default values of the Kconfig options :kconfig:option:`CONFIG_BT_DIS` and :kconfig:option:`CONFIG_BT_DIS_FW_REV`, respectively, are set to enabled.
The default value of :kconfig:option:`CONFIG_BT_DIS_FW_REV_STR` is set to :kconfig:option:`CONFIG_MCUBOOT_IMAGE_VERSION` if :kconfig:option:`CONFIG_BOOTLOADER_MCUBOOT` is enabled.
Otherwise, it is set to ``0.0.0+0``.

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
Because of this, the GFPS selects the :kconfig:option:`CONFIG_SETTINGS` Kconfig option.

Implementation details
**********************

The implementation uses :c:macro:`BT_GATT_SERVICE_DEFINE` to statically define and register the Fast Pair GATT service.
The Fast Pair service automatically handles all of the requests received from the Fast Pair Seeker.
No application input is required to handle the requests.

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

| Header file: :file:`include/bluetooth/services/fast_pair.h`
| Source files: :file:`subsys/bluetooth/services/fast_pair`

.. doxygengroup:: bt_fast_pair
   :project: nrf
   :members:
