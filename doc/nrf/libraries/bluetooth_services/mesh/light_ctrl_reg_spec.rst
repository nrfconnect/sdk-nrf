.. _bt_mesh_light_ctrl_reg_spec_readme:

Specification-defined illuminance regulator
###########################################

.. contents::
   :local:
   :depth: 2

This module implements the illuminance regulator defined in the Bluetooth® Mesh model specification.

The regulator operates in a compile time configurable update interval between 10 and 100 ms.
The interval can be configured through the :kconfig:option:`CONFIG_BT_MESH_LIGHT_CTRL_REG_SPEC_INTERVAL` option.

For each step, the regulator:

1. Calculates the integral of the error since the last step.
#. Adds the integral to an internal sum.
#. Multiplies this sum by an integral coefficient.
#. Summarizes the sum with the raw difference multiplied by a proportional coefficient.

The error, the regulator coefficients, and the internal sum, are represented as 32-bit floating point values.
The resulting output level is represented as an unsigned 16-bit integer.

To reduce noise, the regulator has a configurable accuracy property which allows it to ignore errors smaller than the configured accuracy (represented as a percentage of the light level).

API documentation
*****************

| Header file: :file:`include/bluetooth/mesh/light_ctrl_reg_spec.h`
| Source file: :file:`subsys/bluetooth/mesh/light_ctrl_reg_spec.c`

.. doxygengroup:: bt_mesh_light_ctrl_reg_spec
   :project: nrf
   :members:
