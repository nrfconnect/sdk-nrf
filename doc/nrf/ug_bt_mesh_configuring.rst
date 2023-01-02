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

Additional configuration options
================================

This section lists additional configuration options that can be used to configure behavior and performance of Bluetooth mesh.

* :kconfig:option:`CONFIG_SOC_FLASH_NRF_PARTIAL_ERASE` - Enables partial erase on supported hardware platforms.
  Partial erase allows the flash page erase operation to be split into several small chunks preventing longer CPU stalls.
  This improves responsiveness of a mesh node during the defragmentation of storage areas used by the settings subsystem.
