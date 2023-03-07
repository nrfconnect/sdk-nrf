.. _ug_bt_mesh:

Bluetooth mesh
##############

The |NCS| provides support for developing applications using the Bluetooth® mesh protocol.
The support is based on Zephyr's :ref:`bluetooth_mesh` implementation.

The `Bluetooth mesh profile specification`_ is developed and published by the Bluetooth® Special Interest Group (SIG).
It allows one-to-one, one-to-many, and many-to-many communication, using the Bluetooth LE protocol to exchange messages between the nodes on the network.
All nodes in a Bluetooth mesh network can communicate with each other, as long as there is a chain of nodes between them to relay the messages.
The messages are encrypted with two layers of 128-bit AES-CCM encryption, allowing secure communication between thousands of devices.

The end-user applications are implemented as a set of mesh models.
The Bluetooth SIG defines some generic and reusable models in the `Bluetooth mesh model specification`_, but vendors are free to define their own models.

See :ref:`samples` for the list of available Bluetooth mesh samples.

The Bluetooth mesh samples use the `nRF Mesh mobile app`_ to perform provisioning and configuration.

Read more about the Bluetooth mesh in the Bluetooth SIG's `Bluetooth mesh introduction blog`_, `Bluetooth mesh study guide`_ and `Bluetooth mesh FAQ`_.
Make also sure to check the official `Bluetooth mesh glossary`_.

.. toctree::
   :maxdepth: 1
   :caption: Subpages:

   supported_features
   concepts
   architecture
   configuring
   model_config_app
   fota
   node_removal
   vendor_model/index
   reserved_ids
