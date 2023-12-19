.. _matter_bridge_app_extending_protocol:

Adding support for a proprietary protocol
#########################################

The :ref:`Matter Bridge architecture <ug_matter_overview_bridge_ncs_implementation>` describes the two sides of bridging relationship, ``Matter Bridged Device`` and ``Bridged Device Data Provider``.
The ``Matter Bridged Device`` represents the side that natively supports Matter, and the ``Bridged Device Data Provider`` represents a non-Matter interface.
It is not specified what protocol should be used for a ``Bridged Device Data Provider`` role.

The Matter Bridge application supports bridging Matter to Bluetooth LE data providers that use the Bluetooth LE protocol to communicate with physical end devices, and simulated data providers that emulate real devices behavior in the application.
You can add support for any protocol as long as you are able to describe data model translation between Matter and that protocol.

To provide support for a new protocol, you have to implement the ``Bridged Device Data Provider`` role that is able to communicate with the physical end devices using that protocol.
The interactions to be implemented by the ``Bridged Device Data Provider`` vary depending on the bridged Matter device type.

Typically the ``Bridged Device Data Provider`` has to provide up-to-date information about the device's state by pushing notification about the state changes.
This allows updating the Matter Data Model state and also enables handling Matter subscriptions.
The subscription handling can be implemented either by utilizing the implemented protocol's native support for pushing notifications by the end device, or by performing periodic data polling, which is less efficient.

If the Matter device can perform write operations or be controlled using Matter invoke command operations, the ``Bridged Device Data Provider`` shall be able to send appropriate information to the physical end device.
