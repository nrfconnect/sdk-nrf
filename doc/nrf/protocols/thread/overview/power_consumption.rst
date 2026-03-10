.. _thread_power_consumption:

OpenThread power consumption
############################

.. contents::
   :local:
   :depth: 2

This page provides information about the amount of power used by Sleepy End Devices (SEDs) and Synchronized Sleepy End Devices (SSEDs) that use the OpenThread stack.

.. _thread_power_consumption_methodology:

Measurements methodology
************************

The measurement setup consists of the following:

* An nRF52840 DK board used as a Thread leader.
* A DUT board used as a Thread child.
* A `Power Profiler Kit II (PPK2)`_ attached to the DUT according to the instructions in its Quick start guide.

The leader board is flashed with the regular :ref:`Thread CLI sample <ot_cli_sample>` firmware.

.. code-block::

   cd ncs/nrf/samples/openthread/cli/
   west build -b nrf52840dk/nrf52840 -- -Dcli_SNIPPET="ot-ci;ot-logging"

The DUT board is flashed with the :ref:`Thread CLI sample low power mode <ot_cli_sample_low_power>` firmware.
In the build command below, replace *board_target* with the board target name of the DUT.

.. parsed-literal::
   :class: highlight

   cd ncs/nrf/samples/openthread/cli/
   west build -b *board_target* -- -Dcli_SNIPPET="ot-ci" -DEXTRA_CONF_FILE=extra_conf/low_power.conf

After the Thread network is enabled on the leader, the child is configured with the desired parameters.
Once attached to the network, the child device will switch to sleep mode (disabling all peripherals except RTC) and wake up according to its configuration:

* For the SED case, it will regularly send MAC Data Poll Commands.
* For the SSED case, it will regularly switch to radio receive mode.

The PPK2 is used to collect power consumption data during at least 60 seconds.
During this time no data frames are being sent from the leader to the child.
Afterwards, the CSV data is parsed with a script to produce the below results.

.. _thread_power_consumption_data:

Power consumption data
**********************

The following tables show the power consumption measured with the configuration used.

.. tabs::

   .. group-tab:: Thread 1.4 SED

      .. table:: Configuration

         +--------------------------+---------------------+--------------------------+----------------------------+-------------------------------+
         | Parameter                | nrf52840dk/nrf52840 | nrf5340dk/nrf5340/cpuapp | nrf54l15dk/nrf54l15/cpuapp | nrf54lm20dk/nrf54lm20a/cpuapp |
         +==========================+=====================+==========================+============================+===============================+
         | Board revision           |              v3.0.2 |                   v2.0.2 |                     v0.9.3 |                        v0.5.1 |
         +--------------------------+---------------------+--------------------------+----------------------------+-------------------------------+
         | Supply voltage [V]       |                 3.0 |                      3.0 |                       3.0  |                          3.0  |
         +--------------------------+---------------------+--------------------------+----------------------------+-------------------------------+
         | Transmission power [dBm] |                   0 |                        0 |                         0  |                            0  |
         +--------------------------+---------------------+--------------------------+----------------------------+-------------------------------+
         | Polling period [ms]      |                1000 |                     1000 |                      1000  |                         1000  |
         +--------------------------+---------------------+--------------------------+----------------------------+-------------------------------+

      .. table:: Power consumption

        +-------------------------------+-----------------------+----------------------------+----------------------------+-------------------------------+
        | Parameter                     |   nrf52840dk/nrf52840 |   nrf5340dk/nrf5340/cpuapp | nrf54l15dk/nrf54l15/cpuapp | nrf54lm20dk/nrf54lm20a/cpuapp |
        +===============================+=======================+============================+============================+===============================+
        | Total charge per minute [μC]  |                  1110 |                       1270 |                        980 |                          1030 |
        +-------------------------------+-----------------------+----------------------------+----------------------------+-------------------------------+
        | Average data poll charge [μC] |                  15.3 |                       17.8 |                       14.3 |                          15.0 |
        +-------------------------------+-----------------------+----------------------------+----------------------------+-------------------------------+
        | Average sleep current [μA]    |                  3.19 |                       3.31 |                       1.98 |                          2.16 |
        +-------------------------------+-----------------------+----------------------------+----------------------------+-------------------------------+


   .. group-tab:: Thread 1.4 SSED

      .. table:: Configuration

         +--------------------------------+---------------------+--------------------------+----------------------------+-------------------------------+
         | Parameter                      | nrf52840dk/nrf52840 | nrf5340dk/nrf5340/cpuapp | nrf54l15dk/nrf54l15/cpuapp | nrf54lm20dk/nrf54lm20a/cpuapp |
         +================================+=====================+==========================+============================+===============================+
         | Board revision                 |              v3.0.2 |                   v2.0.2 |                     v0.9.3 |                        v0.5.1 |
         +--------------------------------+---------------------+--------------------------+----------------------------+-------------------------------+
         | Supply voltage [V]             |                 3.0 |                      3.0 |                        3.0 |                           3.0 |
         +--------------------------------+---------------------+--------------------------+----------------------------+-------------------------------+
         | Transmission power [dBm]       |                   0 |                        0 |                          0 |                             0 |
         +--------------------------------+---------------------+--------------------------+----------------------------+-------------------------------+
         | CSL period [ms]                |                1000 |                     1000 |                       1000 |                          1000 |
         +--------------------------------+---------------------+--------------------------+----------------------------+-------------------------------+
         | CSL timeout [s]                |                  20 |                       20 |                         20 |                            20 |
         +--------------------------------+---------------------+--------------------------+----------------------------+-------------------------------+
         | Child's CSL accuracy [ppm]     |                 ±20 |                      ±20 |                        ±50 |                           ±50 |
         +--------------------------------+---------------------+--------------------------+----------------------------+-------------------------------+
         | Child's CSL uncertainty [μs]   |                ±120 |                     ±120 |                       ±300 |                          ±300 |
         +--------------------------------+---------------------+--------------------------+----------------------------+-------------------------------+

      .. table:: Power consumption

         +---------------------------------+-----------------------+----------------------------+----------------------------+-------------------------------+
         | Parameter                       |   nrf52840dk/nrf52840 |   nrf5340dk/nrf5340/cpuapp | nrf54l15dk/nrf54l15/cpuapp | nrf54lm20dk/nrf54lm20a/cpuapp |
         +=================================+=======================+============================+============================+===============================+
         | Total charge per minute [μC]    |                  1080 |                       1130 |                        857 |                           918 |
         +---------------------------------+-----------------------+----------------------------+----------------------------+-------------------------------+
         | Average CSL receive charge [μC] |                  14.2 |                       14.3 |                       11.6 |                          12.4 |
         +---------------------------------+-----------------------+----------------------------+----------------------------+-------------------------------+
         | Average data poll charge [μC]   |                  21.0 |                       23.2 |                       19.3 |                          20.8 |
         +---------------------------------+-----------------------+----------------------------+----------------------------+-------------------------------+
         | Average sleep current [μA]      |                  3.18 |                       3.31 |                       1.98 |                          2.15 |
         +---------------------------------+-----------------------+----------------------------+----------------------------+-------------------------------+
