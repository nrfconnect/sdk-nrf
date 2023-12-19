.. _matter_bridge_app:

Matter bridge
#############

The Matter bridge application can be used to build a :ref:`bridge <ug_matter_overview_bridge>` device using the :ref:`Matter <ug_matter>` application layer.
The bridge device allows the use of non-Matter devices in a :ref:`Matter fabric <ug_matter_network_topologies_structure>` by exposing them as Matter endpoints.
The devices on the non-Matter side of the Matter bridge are called *bridged devices*.

The Matter bridge device works as a Matter accessory device, meaning it can be paired and controlled remotely over a Matter network built on top of a low-power 802.11ax (Wi-Fi 6) network.

See the subpages for how to use the application and how to extend it.

.. toctree::
   :maxdepth: 2
   :caption: Subpages:

   doc/matter_bridge_description
   doc/extending_bridge
