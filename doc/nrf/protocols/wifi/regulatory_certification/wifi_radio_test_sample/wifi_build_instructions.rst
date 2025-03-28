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
In the :file:`<ncs_repo>/nrf/samples/wifi/radio_test/multi_domain/prj.conf` file, set :kconfig:option:`CONFIG_SUPPORT_NETCORE_PERIPHERAL_RADIO_TEST` = ``y``.

.. code-block:: shell

   $ west build -p -b nrf7002dk/nrf5340/cpuapp (DK build)

The following HEX files are generated:

* APP core HEX file: :file:`/build/merged.hex`
* NET core HEX file: :file:`/build/merged_CPUNET.hex`

The nRF5340 DK + nRF7002 EK cannot be used in Wi-Fi and short-range combined mode. Use it for either Wi-Fi or short-range testing only.
The **IOVDD** and **BUCKEN GPIO** (**P1.00** and **P1.01**) on the nRF5340 DK were originally meant for the UART1 interface and routed through the IMCU debugger chip.
These pins are reused for the nRF7002 EK.
Consequently, UART1, used for the short-range communication console, will not function and must be disabled.

.. note::

   It is recommended to open solder bridges **SB29** and **SB30** on the nRF5340 DK when using it with the nRF7002 EK shield.
   If solder bridges **SB29** and **SB30** are open, UART1 functionality will be disabled, preventing short-range (network core) console output.
   Therefore, the modified nRF5340 DK along with the nRF7002 EK must be used for Wi-Fi (application core) testing.
