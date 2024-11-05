.. _nrf54l_testing_dfu:

Testing the DFU solution
########################

You can evaluate the DFU functionality by running the :zephyr:code-sample:`smp-svr` sample for the ``nrf54l15dk/nrf54l15/cpuapp`` board target, which is available for both Bluetooth® LE and serial channels.
This allows you to build and test the DFU solutions that are facilitated through integration with child images and the partition manager.

To compile the SMP server sample for testing secondary image slots on external SPI NOR flash, make sure you are in the :file:`ncs` directory and run the command based on the selected partitioning method.

.. tabs::

   .. group-tab:: Partition Manager

      To build with the Partition Manager, run the following command:

      .. code-block:: console

         west build -b nrf54l15dk/nrf54l15/cpuapp -d build/smp_svr_54l zephyr/samples/subsys/mgmt/mcumgr/smp_svr -T sample.mcumgr.smp_svr.bt.nrf54l15dk.ext_flash

   .. group-tab:: DTS partitioning

      To build with the DTS partitioning, run the following command:

      .. code-block:: console

         west build -b nrf54l15dk/nrf54l15/cpuapp -d build/smp_svr_54l_d zephyr/samples/subsys/mgmt/mcumgr/smp_svr -T sample.mcumgr.smp_svr.bt.nrf54l15dk.ext_flash.pure_dts

This configuration sets up the secondary image slot on the serial flash memory installed on the nRF54L15 DK.
It also enables the relevant SPI and the SPI NOR flash drivers.
