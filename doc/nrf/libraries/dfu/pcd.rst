.. _subsys_pcd:

Peripheral CPU DFU (PCD)
########################

.. contents::
   :local:
   :depth: 2

The peripheral CPU DFU (PCD) library provides functionality for transferring DFU images from the application core to the network core on the nRF5340 System on Chip (SoC).

Overview
********

In the nRF5340 SoC, the application core cannot access the flash of the network core.
However, firmware upgrades for the network core are stored in the application flash.
This is because of the limited size of the network core flash.
Hence, a mechanism for downloading firmware updates from the application to the network core flash is needed.
The PCD library provides functionality for both cores in this situation.
A region of SRAM, accessible by both cores, is used for inter-core communication.

The application core uses the PCD library to communicate where the network core can find the firmware update and its expected SHA.
This is done after verifying the signature of a firmware update targeting the network core.
When the update instructions are written, the application core enables the network core, and waits for a ``pcd_status`` indicating that the update has successfully completed.
The SRAM region used to communicate update instructions to the network core is locked before the application is started on the application core.
This is done to avoid compromised code from being able to perform updates of the network core firmware.

The network core uses the PCD library to look for instructions on where to find the updates.
Once an update instruction is found, this library is used to transfer the firmware update image.

On the application core, the PCD library is used by the :doc:`mcuboot:index-ncs` sample.
On the network core, the PCD library is used by the :ref:`nc_bootloader` sample.


API documentation
*****************

| Header file: :file:`include/dfu/pcd.h`
| Source files: :file:`subsys/dfu/pcd/`

.. doxygengroup:: pcd
   :project: nrf
   :members:
