.. _ug_bt_mesh:

Working with Bluetooth Mesh
###########################

The |NCS| provides support for developing applications using the Bluetooth Mesh protocol.
The support is based on Zephyr's :ref:`bluetooth_mesh` implementation.

.. _bt_mesh_ug_intro:

Introduction
************

The `Bluetooth Mesh Profile Specification`_ is developed and published by the Bluetooth SIG.
It allows one-to-one, one-to-many, and many-to-many communication, using the |BLE| protocol to exchange messages between the nodes on the network.
All nodes in a Bluetooth Mesh network can communicate with each other, as long as there's a chain of nodes between them to relay the messages.
The messages are encrypted with two layers of 128-bit AES-CCM encryption, allowing secure communication between thousands of devices.

The end-user applications are implemented as a set of Mesh Models.
The Bluetooth SIG defines some generic and reusable models in the `Mesh Model Specification`_, but vendors are free to define their own models.

Read more about the Bluetooth Mesh specification on the Bluetooth SIG website:

* `Bluetooth Mesh Glossary`_
* `Bluetooth Mesh FAQ`_
* `Bluetooth Mesh Study Guide`_

For information about basic Mesh concepts and architecture in NCS, read the following pages:

.. toctree::
   :maxdepth: 1

   ug_bt_mesh_concepts.rst
   ug_bt_mesh_architecture.rst

.. _mesh_ug_supported features:

Supported features
******************

The Bluetooth Mesh in |NCS| supports all mandatory features of the `Bluetooth Mesh Profile Specification`_ and the `Mesh Model Specification`_, as well as most of the optional features.

The following features are supported by Zephyr's :ref:`zephyr:bluetooth_mesh`:

* :ref:`zephyr:bluetooth_mesh_core`:

  * Node role
  * Relay feature
  * Low power feature
  * Friend feature

* :ref:`zephyr:bluetooth_mesh_provisioning`:

  * Provisionee role (GATT and Advertising Provisioning bearer)
  * Provisioner role (only Advertising Provisioning bearer)

* :ref:`GATT Proxy Server role <zephyr:bt_mesh_proxy>`
* :ref:`zephyr:bluetooth_mesh_models`

In addition to these features, |NCS| implements several models defined in the  `Mesh Model Specification`_.
See :ref:`bt_mesh_models` for details.

Configuration
*************

The Bluetooth Mesh support is controlled by :option:`CONFIG_BT_MESH`, which depends on the following configuration options:

* :option:`CONFIG_BT` - Enables the Bluetooth subsystem.
* :option:`CONFIG_BT_OBSERVER` - Enables the Bluetooth Observer role.
* :option:`CONFIG_BT_PERIPHERAL` - Enables the Bluetooth Peripheral role.

The Bluetooth Mesh subsystem currently performs best with :ref:`Zephyr's Bluetooth LE Controller <zephyr:bluetooth-arch>`.
To force |NCS| to build with this controller, enable :option:`CONFIG_BT_LL_SW_SPLIT`.


Optional features configuration
===============================

Optional features in the Bluetooth Mesh stack must be explicitly enabled:

* :option:`CONFIG_BT_MESH_RELAY` - Enables message relaying.
* :option:`CONFIG_BT_MESH_FRIEND` - Enables the Friend role.
* :option:`CONFIG_BT_MESH_LOW_POWER` - Enabels the Low Power role.
* :option:`CONFIG_BT_MESH_PROVISIONER` - Enables the Provisioner role.
* :option:`CONFIG_BT_MESH_GATT_PROXY` - Enables the GATT Proxy Server role.
* :option:`CONFIG_BT_MESH_PB_GATT` - Enables the GATT provisioning bearer.
* :option:`CONFIG_BT_MESH_CDB` - Enables the Configuration Database subsystem.

The persistent storage of the Bluetooth Mesh provisioning and configuration data is enabled by :option:`CONFIG_BT_SETTINGS`.
See the Persistent Storage section of :ref:`zephyr:bluetooth-arch` for details.

Mesh models
-----------

The |NCS| Bluetooth Mesh Model implementations are optional features, and each model has individual Kconfig options that must be explicitly enabled.
See :ref:`bt_mesh_models` for details.

Available drivers, libraries, and samples
*****************************************

See :ref:`samples` for the list of available Bluetooth Mesh samples.

The Bluetooth Mesh samples use the `nRF Mesh mobile app`_ to perform provisioning and configuration.
