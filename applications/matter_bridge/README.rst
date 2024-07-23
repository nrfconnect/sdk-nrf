.. _matter_bridge_app:

Matter bridge
#############

The Matter bridge application can be used to build a :ref:`bridge <ug_matter_overview_bridge>` device using the :ref:`Matter <ug_matter>` application layer.
The bridge device allows the use of non-Matter devices in a :ref:`Matter fabric <ug_matter_network_topologies_structure>` by exposing them as Matter endpoints.
The devices on the non-Matter side of the Matter bridge are called *bridged devices*.

The Matter bridge device functions as a Matter accessory, allowing it to be paired with and controlled through a Matter network.
This network can be constructed using either a low-power 802.11ax (Wi-Fi 6) or 802.15.4 (Thread) protocol.

Currently the Matter bridge application supports the following types of *bridged devices*:

* Bluetooth® LE peripheral devices - Devices that operate on real data (such as sensor measurements or lighting state).
* Simulated devices - Devices that interact with the bridge by using data fabricated programmatically by the software.

Note that, in addition to already implemented *bridged devices*, the Matter bridge architecture allows you to add support for other devices that run over diverse connectivity technologies, such as Zigbee or Bluetooth® Mesh.
To learn more details about supported *bridged devices*, refer to the :ref:`Bridged device support <matter_bridge_app_bridged_support>` section of Matter bridge application guide.

See the subpages for how to use the application and how to extend it.

.. toctree::
   :maxdepth: 2
   :caption: Subpages:

   doc/matter_bridge_description
   doc/extending_bridge
