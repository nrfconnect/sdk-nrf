.. _bt_mesh_ug_intro:

Bluetooth mesh overview
#######################

.. contents::
   :local:
   :depth: 2

The `Bluetooth mesh profile specification`_ is developed and published by the Bluetooth SIG.
It allows one-to-one, one-to-many, and many-to-many communication, using the Bluetooth LE protocol to exchange messages between the nodes on the network.
All nodes in a Bluetooth mesh network can communicate with each other, as long as there's a chain of nodes between them to relay the messages.
The messages are encrypted with two layers of 128-bit AES-CCM encryption, allowing secure communication between thousands of devices.

The end-user applications are implemented as a set of mesh models.
The Bluetooth SIG defines some generic and reusable models in the `Bluetooth mesh model specification`_, but vendors are free to define their own models.

Read more about the Bluetooth mesh in the Bluetooth SIG's `Bluetooth mesh study guide`_ and `Bluetooth mesh FAQ`_.
Make also sure to check the official `Bluetooth mesh glossary`_.

.. _mesh_ug_supported features:

Supported features
******************

The Bluetooth mesh in |NCS| supports all mandatory features of the `Bluetooth mesh profile specification`_ and the `Bluetooth mesh model specification`_, as well as most of the optional features.

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

In addition to these features, |NCS| implements several models defined in the `Bluetooth mesh model specification`_.
See :ref:`bt_mesh_models` for details.
