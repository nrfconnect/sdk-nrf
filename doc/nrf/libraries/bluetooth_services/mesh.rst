.. _bt_mesh:

Bluetooth mesh profile
######################

Bluetooth® mesh is supported for development in |NCS|, through the Zephyr :ref:`zephyr:bluetooth_mesh` implementation.
Nordic Semiconductor's implementation of the Bluetooth mesh allows applications to use the features provided by the Bluetooth mesh when running on supported Nordic devices.

The `Bluetooth mesh profile specification`_ is developed and published by the Bluetooth® Special Interest Group (SIG).
It is a solution that allows one-to-one, one-to-many, and many-to-many communication, using the Bluetooth Low Energy protocol to exchange messages between the nodes in the network.
The nodes can communicate with each other as long as they are in direct radio range of each other, or there are enough devices available that are capable of listening and forwarding these messages.
See the :ref:`Bluetooth mesh user guide <ug_bt_mesh>` for an overview of the technology, like supported features, concepts and architecture.

The end-user applications (such as Luminaire control) are defined with the help of client-server Bluetooth mesh models defined in the `Bluetooth mesh model specification`_.
Bluetooth mesh libraries contain modules, including the Bluetooth mesh models, provided by Nordic Semiconductor to aid in the development of Bluetooth mesh-based applications.
They implement default behavior of a Bluetooth mesh device, and are used in :ref:`bt_mesh_samples`.

For information on how to use the supplied libraries for Bluetooth mesh, see :ref:`ug_bt_mesh_configuring`.

Information about the changes and known issues for Bluetooth mesh in each release can be found in |NCS|'s :ref:`release_notes` and on the :ref:`known_issues` page.

.. toctree::
   :maxdepth: 1
   :caption: Subpages:

   mesh/model_types.rst
   mesh/models.rst
   mesh/properties.rst
   mesh/dk_prov.rst
   mesh/sensor.rst
   mesh/sensor_types.rst
