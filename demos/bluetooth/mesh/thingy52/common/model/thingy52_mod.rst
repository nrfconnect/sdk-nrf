.. _bt_mesh_thingy52_mod_readme:

Thingy:52 model
###############

The Thingy52 model is a vendor spesific model that are designed for applications
involving LED and speaker functionality. The Thingy:52 model represents a single Thingy:52 device.

This model is well suited for applications where the designer wants to create applications
that propagates light and sound effects between nodes inside a bluetooth mesh. One clear
advantage is that the propagation behaviour between nodes can be reconfigured at runtime due
to the pub/sub configuration capabilities of bluetooth mesh, making it easy to alter the
effects on the fly without having to flash new firmware on the devices.

It is implemented as a control model, meaning that it can both send and receive messages to and from
other models of the same type

The Model creates a single model instance in the mesh composition data.
The Thingy:52 Model can send messages to other Thingy:52 models as long as it has the right application keys.

States
******

The Thingy:52 Model contains the following states:

Time to Live: ``uint8_t``
	The Time to Live state determines how many hops a single message has left before it should be disregarded.

Duration: ``int32_t``
	The Duration state determines how long the effects of a message shall last within a mesh node.

Color: :c:type:`bt_mesh_thingy52_rgb`
	The Color state determines the RBG color assosiated with a message.

Speaker On: ``boolean``
	The Speaker On state determines if a message should support speaker output.

Each state is assosiated with a single message, and is accessed by your application through :cpp:type:`rgb_set_handler`.
It is expected that your application holds the state of each induvidual message, and handles them according
to your application's spesification.


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

