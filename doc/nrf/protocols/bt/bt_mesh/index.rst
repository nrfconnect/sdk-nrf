.. _ug_bt_mesh:

Bluetooth Mesh
##############

Bluetooth® Mesh is a profile specification (see `Bluetooth Mesh profile specification`_) developed and published by the Bluetooth Special Interest Group (SIG).
It allows one-to-one, one-to-many, and many-to-many communication, using the Bluetooth LE protocol to exchange messages between the nodes on the network.

Bluetooth Mesh primarily targets simple control and monitoring applications, like light control or sensor data gathering.
The packet format is optimized for small control packets, issuing single commands or reports, and is not intended for data streaming or other high-bandwidth applications.

The |NCS| provides support for developing applications using the Bluetooth Mesh protocol.
The support is based on Zephyr's :ref:`bluetooth_mesh` implementation.
For more information about the |NCS| implementation of the Bluetooth Mesh, see :ref:`Bluetooth Mesh overview <ug_bt_mesh_overview_intro>`.

The |NCS| provides a number of :ref:`bt_mesh_samples` that demonstrate the Bluetooth Mesh features and behavior.

Find more information about the Bluetooth Mesh in the Bluetooth SIG's `Bluetooth Mesh introduction blog`_, `Bluetooth Mesh study guide`_ and `Bluetooth Mesh FAQ`_.


.. toctree::
   :maxdepth: 1
   :caption: Subpages:

   overview/index
   configuring
   model_config_app
   fota
   node_removal
   vendor_model/index
