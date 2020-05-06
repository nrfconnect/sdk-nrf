.. _lib_pcd:

Peripheral CPU DFU (PCD)
########################

The Peripheral CPU DFU (PCD) provides functionality for sending/receiving a DFU image between cores on a multi core SoC.

Overview
********

On a multi core architecture, the different roles of performing DFU of one of the cores can be split between different cores.
As an example, the validation of the signature for the DFU image could be performed by CPU 1, while the transfer of the image to CPU 2 is performed by CPU 2.
This library provides functionality facilitating DFU procedures in such architectures.

The PCD library is used in the :ref:`netboot` sample.

API documentation
*****************

| Header file: :file:`include/dfu/pcd.h`
| Source files: :file:`subsys/dfu/pcd/`

.. doxygengroup:: pcd
   :project: nrf
   :members:
