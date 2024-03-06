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

The measurement setup consists of:

* An nrf52840dk_nrf52840 board used as a Thread leader.
* A DUT board used as a Thread child.
* A `Power Profiler Kit II (PPK2)`_ attached to the DUT according to the instructions in its Quick start guide.

The leader board is flashed with the regular :ref:`Thread CLI sample <ot_cli_sample>` firmware.

   .. code-block::

      cd ncs/nrf/samples/openthread/cli/
      west build -b nrf52840dk_nrf52840 -S ci -S logging

The DUT board is flashed with the :ref:`Thread CLI sample low power mode <ot_cli_sample_low_power>` firmware.
In the build command below, replace ``build-target`` with the build target name of the DUT.

   .. code-block::

      cd ncs/nrf/samples/openthread/cli/
      west build -b build-target -S ci -S low_power


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

   .. group-tab:: Thread 1.3 SED

      .. table:: Configuration

         +--------------------------+---------------------+--------------------------+
         | Parameter                | nrf52840dk_nrf52840 | nrf5340dk_nrf5340_cpuapp |
         +==========================+=====================+==========================+
         | Board revision           |              v3.0.1 |                   v2.0.1 |
         +--------------------------+---------------------+--------------------------+
         | Supply voltage [V]       |                 3.0 |                      3.0 |
         +--------------------------+---------------------+--------------------------+
         | Transmission power [dBm] |                   0 |                        0 |
         +--------------------------+---------------------+--------------------------+
         | Polling period [ms]      |                1000 |                     1000 |
         +--------------------------+---------------------+--------------------------+

      .. table:: Power consumption

         +-------------------------------+-----------------------+----------------------------+
         | Parameter                     |   nrf52840dk_nrf52840 |   nrf5340dk_nrf5340_cpuapp |
         +===============================+=======================+============================+
         | Total charge per minute [μC]  |               1136.00 |                    1203.00 |
         +-------------------------------+-----------------------+----------------------------+
         | Average data poll charge [μC] |                 16.57 |                      17.15 |
         +-------------------------------+-----------------------+----------------------------+
         | Average sleep current [μA]    |                  2.58 |                       3.15 |
         +-------------------------------+-----------------------+----------------------------+


   .. group-tab:: Thread 1.3 SSED

      .. table:: Configuration

         +--------------------------------+---------------------+--------------------------+
         | Parameter                      | nrf52840dk_nrf52840 | nrf5340dk_nrf5340_cpuapp |
         +================================+=====================+==========================+
         | Board revision                 |              v3.0.1 |                   v2.0.1 |
         +--------------------------------+---------------------+--------------------------+
         | Supply voltage [V]             |                 3.0 |                      3.0 |
         +--------------------------------+---------------------+--------------------------+
         | Transmission power [dBm]       |                   0 |                        0 |
         +--------------------------------+---------------------+--------------------------+
         | CSL period [ms]                |                1000 |                     1000 |
         +--------------------------------+---------------------+--------------------------+
         | CSL timeout [s]                |                  20 |                       20 |
         +--------------------------------+---------------------+--------------------------+
         | Parent's CSL accuracy [ppm]    |                 ±20 |                      ±20 |
         +--------------------------------+---------------------+--------------------------+
         | Parent's CSL uncertainty [μs]  |                ±120 |                     ±120 |
         +--------------------------------+---------------------+--------------------------+

      .. table:: Power consumption

         +---------------------------------+-----------------------+----------------------------+
         | Parameter                       |   nrf52840dk_nrf52840 |   nrf5340dk_nrf5340_cpuapp |
         +=================================+=======================+============================+
         | Total charge per minute [μC]    |               1042.20 |                    1148.60 |
         +---------------------------------+-----------------------+----------------------------+
         | Average CSL receive charge [μC] |                 13.69 |                      14.23 |
         +---------------------------------+-----------------------+----------------------------+
         | Average data poll charge [μC]   |                 22.32 |                      23.23 |
         +---------------------------------+-----------------------+----------------------------+
         | Average sleep current [μA]      |                  2.71 |                       3.14 |
         +---------------------------------+-----------------------+----------------------------+
