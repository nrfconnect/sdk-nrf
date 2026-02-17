.. _ug_nrf54h20_assigning_peripherals:

nRF54H20 Assigning peripherals to cores
#######################################

.. contents::
   :local:
   :depth: 2

Peripherals in the global domain of the nRF54H20 SoC must be assigned to a specific core.
Assigning a global peripheral to a core is done by specifying the peripheral's interrupt controller and status in the devicetree.
By default, the global peripherals are assigned to the application core.
To assign a global peripheral to a different core, set its status to "reserved" application core's devicetree:

Application core's devicetree:

.. code-block:: devicetree

   /* Will be used by PPR core */
   &uart135 {
           status = "reserved";
   };

For global peripherals which have their interrupts routed to the core to which they will be assigned, the interrupt parent must be updated as well:

Application core's devicetree:

.. code-block:: devicetree

   /* Will be used by FLPR core */
   &can120 {
           status = "reserved";
           interrupt-parent = <&cpuflpr_clic>; /* FLPR core's interrupt controller */
   };

The peripheral can now be enabled and configured as usual in the devicetree of the core to which the peripheral is assigned:

PPR core devicetree:

.. code-block:: devicetree

   &uart120 {
           status = "okay";
           ...
   };

FLPR core devicetree:

.. code-block:: devicetree

   &can120 {
           status = "okay";
           ...
   };
