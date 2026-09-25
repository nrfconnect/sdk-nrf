.. _can_scan:

sCAN CAN shim driver
####################

.. contents::
   :local:
   :depth: 2

This driver integrates the :ref:`sCAN` soft peripheral with the Zephyr CAN API, enabling control over the peripheral.
This allows for using the :ref:`sCAN` soft peripheral with higher layer zephyr libraries like :ref:`can_isotp` and ``socket_can``, external modules like :ref:`external_module_cannectivity`, the CAN shell :ref:`can_shell`, along with any native zephyr applications/samples like :file:`zephyr/samples/drivers/can`.

Configuration
*************

The sCAN CAN shim driver is defined in the devicetree using the :dtcompatible:`nordic,nrf-scan`.
The following example is for the nRF54L15DK

.. code:: dts

   &pinctrl {
           scan_default: scan_default {
                   group1 {
                           psels = <NRF_PSEL(SDP_MSPI_DQ1, 2, 2)>; /* TX */
                           nordic,drive-mode = <NRF_DRIVE_S0S1>;
                           bias-pull-up;
                   };

                   group2 {
                           psels = <NRF_PSEL(SDP_MSPI_DQ3, 2, 4)>; /* RX */
                           nordic,drive-mode = <NRF_DRIVE_S0S1>;
                           bias-disable;
                   };
           };

           scan_sleep: scan_sleep {
                   group1 {
                           low-power-enable;
                           psels = <NRF_PSEL(SDP_MSPI_DQ0, 2, 2)>,
                                   <NRF_PSEL(SDP_MSPI_DQ1, 2, 4)>;
                           bias-pull-up;
                };
           };
   };

   &spi00 {
           status = "disabled";
   };

   &cpuflpr_vpr {
           pinctrl-0 = <&scan_default>;
           pinctrl-1 = <&scan_sleep>;
           pinctrl-names = "default", "sleep";
           interrupts = <76 NRF_DEFAULT_IRQ_PRIORITY>;
           status = "okay";
           /delete-property/ execution-memory;
   };

   / {
           soc {
                   softperiph_ram: memory@2003b400 {
                           reg = <0x2003b400 0x4400>;
                           ranges = <0 0x2003b400 0x4400>;
                           #address-cells = <1>;
                           #size-cells = <1>;

                           scan: scan@4200 {
                                   compatible = "nordic,nrf-scan";
                                   #address-cells = <1>;
                                   #size-cells = <0>;
                                   reg = <0x4200 0x200>;
                           };
                   };
           };
   };

More example devicetree overlays can be found in the board overlays for the tests in :file:`nrf/tests/zephyr/drivers/can/`.
Relevant porting information regarding RAM layout and pin configuration can be found here :ref:`scan_nrf54L_series_porting_guide`.

To configure the sCAN CAN shim driver with the devicetree, complete the following steps:

1. Specify the base address of the sCAN registers in RAM by creating the :dtcompatible:`nordic,nrf-scan` node within the RAM region reserved for the soft peripheral.
#. Specify an interrupt for communication between the application and FLPR cores, and determine the pins for sCAN operation.
   Provide this information in the properties of the node labeled as ``cpuflpr_vpr``.
#. If the ``execution-memory`` property exists in the ``cpuflpr_vpr`` node, delete it to ensure the FLPR core does not use the Nordic VPR coprocessor launcher (see the :kconfig:option:`CONFIG_NORDIC_VPR_LAUNCHER` Kconfig option).

Testing
*******

There are tests available in :file:`nrf/tests/zephyr/drivers/can/` which can be built and run.
The :ref:`can_shell` can optionally be enabled to allow for locally testing the sCAN CAN shim driver.
This is the simplest way to quickly send and receive some messages to validate its functionality.

There are also samples at :file:`zephyr/samples/drivers/can/` which can be built and run for any board defining the ``zephyr,canbus`` chosen node:

.. code:: dts

   chosen {
           zephyr,canbus = &scan;
   };

API documentation
*****************

| Header file: :file:`zephyr/include/zephyr/drivers/can.h`
| Source file: :file:`nrf/drivers/can/can_nrf_scan.c`

.. doxygengroup:: can_interface
