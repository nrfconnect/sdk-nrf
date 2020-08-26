.. _bt_mesh_thingy52_orientation_handler:

Thingy:52 Orientation Handler
######################

The Orientation Handler is a simple API to extract the current orientation of a Thingy:52 device, using it's
on board low power accelerometer (lis2dh12).


The following code block examplifies the initialization and usage of the API.

.. code-block:: console

   static struct device *orientation_dev;


   void main(void)
   {
   	orientation_dev = device_get_binding(DT_PROP(DT_NODELABEL(lis2dh12), label));

   	uint8_t orr = orientation_get(orientation_dev);
   }

In addition, the user must ensure that the concatination buffer size is defined in the :file:`app.overlay` file of the project:

.. code-block:: console

   &i2c1 {
	concat-buf-size = <6>;
   };


API documentation
*****************

| Header file: :file:`demos/bluetooth/mesh/thingy52/common/orientation_handler.h`
| Source file: :file:`demos/bluetooth/mesh/thingy52/common/orientation_handler.c`

.. doxygengroup:: bt_mesh_thingy52_orientation_handler
   :project: nrf
   :members:
