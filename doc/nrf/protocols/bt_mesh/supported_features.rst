.. _mesh_ug_supported_features:

Supported Bluetooth mesh features
#################################

The Bluetooth® mesh in |NCS| supports all mandatory features of the `Bluetooth mesh profile specification`_ and the `Bluetooth mesh model specification`_, as well as most of the optional features.

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
