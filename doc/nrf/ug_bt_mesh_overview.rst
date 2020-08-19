.. _bt_mesh_ug_intro:

Bluetooth Mesh overview
#######################

The `Bluetooth Mesh Profile Specification`_ is developed and published by the Bluetooth SIG.
It allows one-to-one, one-to-many, and many-to-many communication, using the |BLE| protocol to exchange messages between the nodes on the network.
All nodes in a Bluetooth Mesh network can communicate with each other, as long as there's a chain of nodes between them to relay the messages.
The messages are encrypted with two layers of 128-bit AES-CCM encryption, allowing secure communication between thousands of devices.

The end-user applications are implemented as a set of Mesh Models.
The Bluetooth SIG defines some generic and reusable models in the `Mesh Model Specification`_, but vendors are free to define their own models.

Read more about the Bluetooth Mesh in the Bluetooth SIG's `Bluetooth Mesh Study Guide`_ and `Bluetooth Mesh FAQ`_.
Make also sure to check the official `Bluetooth Mesh Glossary`_.

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
