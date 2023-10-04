.. _ug_bt_mesh_configuring:

Configuring Bluetooth mesh in |NCS|
###################################

.. contents::
   :local:
   :depth: 2

The Bluetooth® mesh support is controlled by :kconfig:option:`CONFIG_BT_MESH`, which depends on the following configuration options:

* :kconfig:option:`CONFIG_BT` - Enables the Bluetooth subsystem.
* :kconfig:option:`CONFIG_BT_OBSERVER` - Enables the Bluetooth Observer role.
* :kconfig:option:`CONFIG_BT_PERIPHERAL` - Enables the Bluetooth Peripheral role.

Optional features configuration
*******************************

Optional features in the Bluetooth mesh stack must be explicitly enabled:

* :kconfig:option:`CONFIG_BT_MESH_RELAY` - Enables message relaying.
* :kconfig:option:`CONFIG_BT_MESH_FRIEND` - Enables the Friend role.
* :kconfig:option:`CONFIG_BT_MESH_LOW_POWER` - Enables the Low Power role.
* :kconfig:option:`CONFIG_BT_MESH_PROVISIONER` - Enables the Provisioner role.
* :kconfig:option:`CONFIG_BT_MESH_GATT_PROXY` - Enables the GATT Proxy Server role.
* :kconfig:option:`CONFIG_BT_MESH_PB_GATT` - Enables the GATT provisioning bearer.
* :kconfig:option:`CONFIG_BT_MESH_CDB` - Enables the Configuration Database subsystem.

The persistent storage of the Bluetooth mesh provisioning and configuration data is enabled by :kconfig:option:`CONFIG_BT_SETTINGS`.
See the :ref:`zephyr:bluetooth-persistent-storage` section of :ref:`zephyr:bluetooth-arch` for details.

Mesh models
===========

The |NCS| Bluetooth mesh model implementations are optional features, and each model has individual Kconfig options that must be explicitly enabled.
See :ref:`bt_mesh_models` for details.

Mesh settings/performance
=========================

The following configuration options are used to configure the behavior and performance of a Bluetooth mesh network.
For more information about configuration options affecting the memory footprint of Bluetooth mesh, see :ref:`memory footprint optimization guide for Bluetooth mesh <app_memory_bt_mesh>`.

* :kconfig:option:`CONFIG_BT_MESH_PROXY_USE_DEVICE_NAME` - Includes the GAP device name in a scan response when the GATT Proxy feature is enabled.
* :kconfig:option:`CONFIG_BT_MESH_DK_PROV` - Enables the Bluetooth mesh provisioning handler for the nRF5x development kits.
* :kconfig:option:`CONFIG_BT_MESH_ADV_BUF_COUNT` - Defines the number of advertising buffers for local messages.
  Increase to improve the performance, at the cost of increased RAM usage.
* :kconfig:option:`CONFIG_BT_MESH_ADV_EXT_GATT_SEPARATE` - Enables the use of a separate extended advertising set for GATT Server Advertising.
* :kconfig:option:`CONFIG_BT_MESH_ADV_EXT_FRIEND_SEPARATE` - Enables the use of a separate extended advertising set for Friend advertising.
* :kconfig:option:`CONFIG_BT_MESH_RELAY_ADV_SETS` - Defines a maximum number of simultaneous relay messages.
* :kconfig:option:`CONFIG_BT_MESH_ADV_EXT_RELAY_USING_MAIN_ADV_SET` - Sets the main advertising set to relay messages.

Additional configuration options
================================

This section lists additional configuration options that can be used to configure behavior and performance of Bluetooth mesh.
The provided values are meant as suggestions only, and should be individually adjusted for each application.

* :kconfig:option:`CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE` - Sets the system workqueue stack size.
  Use the option to increase the system workqueue stack size if the default system workqueue stack size is low.
* :kconfig:option:`CONFIG_MAIN_STACK_SIZE` - Sets the initialization and main thread stack size.
  Use the option to increase the stack size of the default initialization and main thread if necessary.
* :kconfig:option:`CONFIG_HWINFO` - Enables the hardware information driver.
  The hardware information driver must be enabled to perform provisioning of the device.
  See the UUID section of :ref:`bt_mesh_dk_prov`.
* :kconfig:option:`CONFIG_PM_SINGLE_IMAGE` - Enables the use of :ref:`partition_manager` for single-image builds.
* :kconfig:option:`CONFIG_PM_PARTITION_SIZE_SETTINGS_STORAGE` - Sets the size of the partition used for settings storage.
  Use the option to increase the size if necessary.
* :kconfig:option:`CONFIG_SOC_FLASH_NRF_PARTIAL_ERASE` - Enables partial erase on supported hardware platforms.
  Partial erase allows the flash page erase operation to be split into several small chunks preventing longer CPU stalls.
  This improves responsiveness of a mesh node during the defragmentation of storage areas used by the settings subsystem.
* :kconfig:option:`CONFIG_DK_LIBRARY` - Enables the :ref:`dk_buttons_and_leds_readme` library for the nRF5x development kits.
  Use this option to enable the library if the nRF5x development kits are used.

Logging
-------

* :kconfig:option:`CONFIG_NCS_SAMPLES_DEFAULTS` - Enables the Zephyr system logger with minimal logging implementation.
  This is enabled by default for all samples in |NCS|.
  For more information, see :ref:`ug_logging_zephyr`.
* :kconfig:option:`CONFIG_LOG_MODE_DEFERRED` - Enables deferred logging.
  Setting this configuration option is recommended to avoid slowing down the processing of mesh messages.
  It improves the LPN power consumption when the friendship is established.
* :kconfig:option:`CONFIG_LOG_BUFFER_SIZE` - Sets the number of bytes dedicated for the logger internal buffer.
  Increase the number to avoid missing logs in case of a complex protocol or functionality issue.
* :kconfig:option:`CONFIG_LOG_PROCESS_THREAD_SLEEP_MS` - Sets the sleep period for the internal log processing thread.
  Decrease the value to flush logs more quickly.

GATT Proxy performance
----------------------

These options are only compatible with devices supporting Bluetooth Low Energy (LE) v4.2 or higher.


* The following configuration options allow fitting the full relayed mesh advertiser frame into a single Link Layer payload:

  * :kconfig:option:`CONFIG_BT_CTLR_DATA_LENGTH_MAX` set to 37.
  * :kconfig:option:`CONFIG_BT_BUF_ACL_TX_SIZE` set to 37.
  * :kconfig:option:`CONFIG_BT_BUF_ACL_RX_SIZE` set to 37.

* The following option allows sending up to several data frames during the single connection interval:

  * :kconfig:option:`CONFIG_BT_CTLR_SDC_TX_PACKET_COUNT` =10.

Bluetooth settings/performance
------------------------------

The following configuration options are used to configure the Bluetooth Low Energy behavior and performance:

* :kconfig:option:`CONFIG_BT_COMPANY_ID` - Sets the Bluetooth Company Identifier for this device.
* :kconfig:option:`CONFIG_BT_DEVICE_NAME` - Defines the Bluetooth device name.
* :kconfig:option:`CONFIG_BT_L2CAP_TX_MTU` - Sets the maximum L2CAP MTU for L2CAP TX buffers.
  When GATT is enabled, the recommended value is the value of :kconfig:option:`CONFIG_BT_BUF_ACL_TX_SIZE` minus 4.
* :kconfig:option:`CONFIG_BT_L2CAP_TX_BUF_COUNT` - Sets the number of buffers available for outgoing L2CAP packets.
* :kconfig:option:`CONFIG_BT_RX_STACK_SIZE` - Sets the size of the receiving thread stack.
* :kconfig:option:`CONFIG_BT_EXT_ADV_MAX_ADV_SET` - Sets the maximum number of simultaneous advertising sets.
* :kconfig:option:`CONFIG_BT_BUF_ACL_RX_SIZE` - Sets the data size needed for HCI ACL RX buffers.

Disabled and unused Bluetooth features
--------------------------------------

The following feature options are by default disabled in the samples, but it needs to be considered if any of them are required by the application and thus should be enabled:

* :kconfig:option:`CONFIG_BT_CTLR_DUP_FILTER_LEN` =0.
* :kconfig:option:`CONFIG_BT_CTLR_LE_ENC` =n.
* :kconfig:option:`CONFIG_BT_CTLR_CHAN_SEL_2` =n.
* :kconfig:option:`CONFIG_BT_CTLR_MIN_USED_CHAN` =n.
* :kconfig:option:`CONFIG_BT_CTLR_PRIVACY` =n.
* :kconfig:option:`CONFIG_BT_PHY_UPDATE` =n.

Emergency data storage (EMDS)
-----------------------------

The following configuration options should be considered in case of large networks with high demands on storing the replay protection list (RPL) data.
This will require additional hardware.
For more information, see :ref:`emds_readme`.

* :kconfig:option:`CONFIG_EMDS` - Enables the emergency data storage.
* :kconfig:option:`CONFIG_BT_MESH_RPL_STORAGE_MODE_EMDS` - Enables the persistent storage of RPL in EMDS.
* :kconfig:option:`CONFIG_PM_PARTITION_SIZE_EMDS_STORAGE` =0x4000 - Defines the partition size for the Partition Manager.
* :kconfig:option:`CONFIG_EMDS_SECTOR_COUNT` =4 - Defines the sector count of the emergency data storage area.

.. _ug_bt_mesh_configuring_lpn:

Low Power node (LPN)
--------------------

The Low Power node (LPN) is a :ref:`power optimization <app_power_opt>` feature specific to Bluetooth mesh.

The following configuration options are relevant when using the LPN feature:

* Serial communication consumes considerable power, and disabling it should be considered.

  * :kconfig:option:`CONFIG_SERIAL` =n.
  * :kconfig:option:`CONFIG_UART_CONSOLE` =n.

* While enabled, secure beacons will be advertised periodically.
  This consumes power, and is not required for a Low Power node.

  * :kconfig:option:`CONFIG_BT_MESH_BEACON_ENABLED` =n.

* Each LPN poll event consumes power.
  Extending the interval between the poll events will improve the power consumption.

  * :kconfig:option:`CONFIG_BT_MESH_LPN_POLL_TIMEOUT` =600.

* While the GATT Proxy feature is enabled, the Network ID is periodically advertised.
  Disabling it will conserve the energy.

  * :kconfig:option:`CONFIG_BT_MESH_GATT_PROXY_ENABLED` =n.

* Reducing the Node ID advertisement timeout decreases the period where the device consumes power for advertising.

  * :kconfig:option:`CONFIG_BT_MESH_NODE_ID_TIMEOUT` =30.
