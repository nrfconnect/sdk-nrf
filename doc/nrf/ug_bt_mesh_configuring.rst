.. _ug_bt_mesh_configuring:

Configuring Bluetooth mesh in |NCS|
###################################

.. contents::
   :local:
   :depth: 2

The Bluetooth mesh support is controlled by :option:`CONFIG_BT_MESH`, which depends on the following configuration options:

* :option:`CONFIG_BT` - Enables the Bluetooth subsystem.
* :option:`CONFIG_BT_OBSERVER` - Enables the Bluetooth Observer role.
* :option:`CONFIG_BT_PERIPHERAL` - Enables the Bluetooth Peripheral role.

The Bluetooth mesh subsystem currently performs best with :ref:`Zephyr's Bluetooth LE Controller <zephyr:bluetooth-arch>`.
To force |NCS| to build with this controller, enable :option:`CONFIG_BT_LL_SW_SPLIT`.

Optional features configuration
*******************************

Optional features in the Bluetooth mesh stack must be explicitly enabled:

* :option:`CONFIG_BT_MESH_RELAY` - Enables message relaying.
* :option:`CONFIG_BT_MESH_FRIEND` - Enables the Friend role.
* :option:`CONFIG_BT_MESH_LOW_POWER` - Enabels the Low Power role.
* :option:`CONFIG_BT_MESH_PROVISIONER` - Enables the Provisioner role.
* :option:`CONFIG_BT_MESH_GATT_PROXY` - Enables the GATT Proxy Server role.
* :option:`CONFIG_BT_MESH_PB_GATT` - Enables the GATT provisioning bearer.
* :option:`CONFIG_BT_MESH_CDB` - Enables the Configuration Database subsystem.

The persistent storage of the Bluetooth mesh provisioning and configuration data is enabled by :option:`CONFIG_BT_SETTINGS`.
See the :ref:`zephyr:bluetooth-persistent-storage` section of :ref:`zephyr:bluetooth-arch` for details.

Mesh models
===========

The |NCS| Bluetooth mesh model implementations are optional features, and each model has individual Kconfig options that must be explicitly enabled.
See :ref:`bt_mesh_models` for details.
