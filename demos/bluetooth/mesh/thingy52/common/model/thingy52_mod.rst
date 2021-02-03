.. _bt_mesh_thingy52_mod_readme:

Thingy:52 model
###############

The Thingy:52 model is a vendor-specific model designed for applications involving LED and speaker functionality.
The Thingy:52 model represents a single Thingy:52 device.

The Thingy:52 model is implemented as a control model, meaning that it can both send messages to and receive them from other models of the same type.
The model is well suited for applications propagating light and sound effects between nodes in a mesh network.
One clear advantage is that the propagation behavior between nodes can be reconfigured at runtime due to the publication/subscription configuration capabilities of Bluetooth mesh.
This makes it easy to alter the effects on-the-fly, without having to program new firmware onto the devices.

The Thingy:52 model creates a single model instance in the mesh composition data.
It can send messages to other Thingy:52 models as long as it has the right application keys.

States
******

The Thingy:52 model contains the following states:

Time to Live: ``uint8_t``
	The Time to Live state determines how many hops a single message has left before it should be disregarded.

Duration: ``int32_t``
	The Duration state determines how long the effects of a message shall last within a mesh node.

Color: :c:type:`bt_mesh_thingy52_rgb`
	The Color state determines the RGB (red, green, blue) color assosiated with a message.

Speaker On: ``boolean``
	The Speaker On state determines if a message should support speaker output.

Each state is assosiated with a single message and is accessed by your application through :c:type:`rgb_set_handler`.
It is expected that your application holds the state of each individual message, and handles them according to your application's specification.


Extended models
***************

None.

Persistent storage
******************

None.

API documentation
*****************

| Header file: :file:`demos/bluetooth/mesh/thingy52/common/thingy52_mod.h`
| Source file: :file:`demos/bluetooth/mesh/thingy52/common/thingy52_mod.c`

.. doxygengroup:: bt_mesh_thingy52_mod
   :project: nrf
   :members:
