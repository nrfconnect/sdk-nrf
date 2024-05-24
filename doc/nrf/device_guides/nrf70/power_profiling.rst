.. _ug_nrf70_developing_power_profiling:

Power profiling of nRF7002 DK
#############################

.. contents::
   :local:
   :depth: 2

The Power Profiler Kit II (PPK2) of Nordic Semiconductor can be used in conjunction with the nRF7002 DK to evaluate the power consumption of nRF70 Series devices in Low-power mode.
To measure the power consumption of the nRF7002 DK, complete the following steps:

1. Remove the jumper on **P23** (VBAT jumper).
#. Connect **GND** on the PPK2 kit to any **GND** on the nRF7002 DK.
   You can use the **P21** pin **1** labeled as **GND** (-).
#. Connect the **Vout** on the PPK2 kit to the **P23** pin **1** on the nRF7002 DK.

   .. figure:: images/power_profiler2_pc_nrf7002_dk.png
      :alt: Typical configuration for measuring power on the nRF7002 DK

      Typical configuration for measuring power on the nRF7002 DK

#. Configure PPK2 as a source meter with 3.6 volts.

   The following image shows the Power Profiler Kit II example output for DTIM wakeup:

   .. figure:: images/power_profiler_dtim_wakeup.png
      :alt: PPK2 output for DTIM wakeup

      PPK2 output for DTIM wakeup

   To reproduce the plots for DTIM period of 3, complete the following steps using the :ref:`wifi_shell_sample` sample:

     1. Configure an AP with DTIM value of 3.
     #. Connect to the AP using the following Wi-Fi shell commands:

        .. code-block:: console

           wifi scan
           wifi connect <SSID>

     #. Check the connection status using the following Wi-Fi shell command:

        .. code-block:: console

           wifi status

   The following image shows the PPK2 output for DTIM period of 3:

   .. figure:: images/power_profiler_dtim_output.png
      :alt: PPK2 output for DTIM period of 3

      PPK2 output for DTIM period of 3

   To reproduce the plots for TWT interval of one minute, complete the following steps using the :ref:`wifi_shell_sample` sample:

     1. Connect to a TWT supported Wi-Fi 6 AP using the following Wi-Fi shell commands:

        .. code-block:: console

           wifi scan
           wifi connect <SSID>

     #. Check the connection status using the following Wi-Fi shell command:

        .. code-block:: console

           wifi status

     #. Start a TWT session using the following Wi-Fi shell command:

        .. code-block:: console

           wifi twt setup 0 0 1 1 0 1 1 1 8 60000

   The following image shows the PPK2 output for TWT interval of one minute:

   .. figure:: images/power_profiler_twt.png
      :alt: PPK2 output for TWT

      PPK2 output for TWT
