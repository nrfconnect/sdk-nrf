.. _nrf54l_testing_dfu:

Testing the DFU solution
########################

You can evaluate the DFU functionality by running the :zephyr:code-sample:`smp-svr` sample for the ``nrf54l15pdk/nrf54l51/cpuapp`` board target, which is available for both Bluetooth® LE and serial channels.
This allows you to build and test the DFU solutions that are facilitated through integration with child images and the partition manager.

To compile the SMP server sample for testing secondary image slots on external SPI NOR flash, run the following command:

.. code-block:: console

   west build -b nrf54l15pdk/nrf54l15/cpuapp -d build/smp_svr_54l_3 zephyr/samples/subsys/mgmt/mcumgr/smp_svr -T sample.mcumgr.smp_svr.bt.nrf54l15pdk.ext_flash

.. note::

   Make sure to use the correct board target depending on your PDK version:

   * For the PDK revision v0.2.1, AB0-ES7, use the ``nrf54l15pdk@0.2.1/nrf54l15/cpuap`` board target.
   * For the PDK revisions v0.3.0 and v0.7.0, use the ``nrf54l15pdk/nrf54l15/cpuapp`` board target.

This configuration sets up the secondary image slot on the serial flash memory installed on the nRF54L15 PDK.
It also enables the relevant SPI and the SPI NOR flash drivers.
