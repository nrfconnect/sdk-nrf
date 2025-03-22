.. _ug_wifi_build_instructions:

Build instructions
##################

.. contents::
   :local:
   :depth: 2

This section outlines example instructions to build the Wi-Fi® radio test sample binaries for the nRF7002 :term:`Development Kit (DK)` and :term:`Evaluation Kit (EK)` kits.

Stand-alone Wi-Fi Radio test
****************************

For information on building the stand-alone Wi-Fi Radio test, see :ref:`Wi-Fi Radio test sample building and running <wifi_radio_sample_building_and_running>`.

Combined build for Radio test and Wi-Fi Radio test
**************************************************

The combined build configuration builds the binary files for the Wi-Fi Radio test that resides in the application core and the Radio test (short-range) that resides in the network core.
In the :file:`<ncs_repo>/nrf/samples/wifi/radio_test/prj.conf` file, set :kconfig:option:`CONFIG_BOARD_ENABLE_CPUNET` = ``y``.

.. code-block:: shell

   $ west build -p -b nrf7002dk_nrf5340_cpuapp (DK build)
   $ west build -p -b nrf5340dk_nrf5340_cpuapp -- -DSHIELD= nrf7002ek (EK build)

The following HEX files are generated:

* Combined HEX file: :file:`build/zephyr/merged_domains.hex`
* APP core HEX file: :file:`build/zephyr/merged.hex`
* NET core HEX file: :file:`build/peripheral_radio_test/zephyr/merged_CPUNET.hex`
