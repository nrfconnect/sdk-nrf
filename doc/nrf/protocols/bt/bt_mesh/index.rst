.. _ug_bt_mesh:

Bluetooth Mesh
##############

The |NCS| provides support for developing applications using the Bluetooth® Mesh protocol.
The support is based on Zephyr's :ref:`bluetooth_mesh` implementation.

The `Bluetooth Mesh profile specification`_ is developed and published by the Bluetooth Special Interest Group (SIG).
It allows one-to-one, one-to-many, and many-to-many communication, using the Bluetooth LE protocol to exchange messages between the nodes on the network.
All nodes in a Bluetooth Mesh network can communicate with each other, as long as there is a chain of nodes between them to relay the messages.
The messages are encrypted with two layers of 128-bit AES-CCM encryption, allowing secure communication between thousands of devices.

The end-user applications are implemented as a set of mesh models.
The Bluetooth SIG defines some generic and reusable models in the `Bluetooth Mesh model specification`_, but vendors are free to define their own models.

The |NCS| provides a number of :ref:`bt_mesh_samples` that demonstrate the Bluetooth Mesh features and behavior.

The Bluetooth Mesh samples use the `nRF Mesh mobile app`_ to perform provisioning and configuration.

Read more about the Bluetooth Mesh in the Bluetooth SIG's `Bluetooth Mesh introduction blog`_, `Bluetooth Mesh study guide`_ and `Bluetooth Mesh FAQ`_.
Make also sure to check the official `Bluetooth Mesh glossary`_.

.. toctree::
   :maxdepth: 1
   :caption: Subpages:

   supported_features
   concepts
   architecture
   configuring
   model_config_app
   nlc
   fota
   node_removal
   vendor_model/index
   reserved_ids
