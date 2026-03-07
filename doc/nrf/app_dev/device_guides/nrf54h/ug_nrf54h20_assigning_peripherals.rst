.. _ug_nrf54h20_assigning_peripherals:

Assigning peripherals to cores on the nRF54H20 SoC
##################################################

.. contents::
   :local:
   :depth: 2

Peripherals in the global domain of the nRF54H20 SoC can have global ownership.
However, you must assign each peripheral IRQ to a specific core by specifying the peripheral's interrupt controller and enabling the peripheral in the devicetree.
You must also set the peripheral ownership using the ``status`` property.
By default, the global peripherals are assigned to the application core.
Their IRQs are also routed to the application core.
To assign a global peripheral to a different core, do the following:

1. Update its devicetree node in the application core as follows:

   * If the IRQ is forwarded to a VPR that is owned by the application core, set ``status = "reserved"``.
   * If the peripheral is assigned to the radio core, set ``status = "disabled"`` in the application core devicetree.

See the following example of the application core devicetree forwarding the IRQ to the PPR core (owned by the application core itself):
.. code-block:: devicetree

  /* Will be used by PPR core */
  &uart135 {
          status = "reserved";
  };

#. For global peripherals whose interrupts are routed to the core to which they are assigned, you must also update the interrupt parent in the application core devicetree accordingly:

.. code-block:: devicetree

  /* Will be used by FLPR core */
  &can120 {
          status = "reserved";
          interrupt-parent = <&cpuflpr_clic>; /* FLPR core's interrupt controller */
  };

#. The peripheral can now be enabled and configured as usual in the devicetree of the core to which the peripheral is assigned.

* PPR core devicetree:

.. code-block:: devicetree

  &uart120 {
          status = "okay";
          ...
  };

* FLPR core devicetree:

.. code-block:: devicetree

  &can120 {
          status = "okay";
          ...
  };
