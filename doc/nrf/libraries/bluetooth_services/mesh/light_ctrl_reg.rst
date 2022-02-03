.. _bt_mesh_light_ctrl_reg_readme:

Light Lightness Control regulator
#################################

This module is an abstract proportional integral (PI) regulator.
Regulator implementations should declare a :c:struct:`bt_mesh_light_ctrl_reg` struct and supply implementations for ``init``, ``start``, and ``stop``.

For an example of a regulator implementation, see :ref:`bt_mesh_light_ctrl_reg_spec_readme`.


API documentation
*****************

| Header file: :file:`include/bluetooth/mesh/light_ctrl_reg.h`
| Source file: :file:`subsys/bluetooth/mesh/light_ctrl_reg.c`

.. doxygengroup:: bt_mesh_light_ctrl_reg
   :project: nrf
   :members:
