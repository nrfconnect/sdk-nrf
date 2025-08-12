.. _ug_using_wifi_shell_sample:

Using the Wi-Fi Shell sample
############################

.. contents::
   :local:
   :depth: 2

The Wi-Fi® Shell sample runs on a host MCU that drives an nRF70 Series companion device and allows the control of the Wi-Fi functionality through a shell interface.
The Shell sample allows you to test Nordic Semiconductor’s Wi-Fi chipsets.

For more information on the samples in |NCS|, see the following:

* Overall description: :ref:`wifi_shell_sample` sample

Build instructions
******************

For information on the build instructions, see :ref:`Wi-Fi Shell sample building and running <wifi_shell_sample_building_and_running>`.

Set the :kconfig:option:`CONFIG_NRF70_UTIL` Kconfig option to ``y`` in the :file:`<ncs_repo>/nrf/samples/wifi/shell/prj.conf` file to enable the ``nrf70 util`` commands.

Scan, connect, and ping to a network using the Wi-Fi Shell sample
*****************************************************************

The Shell sample allows the development kit to associate with, and ping to any Wi-Fi capable access point in Station (STA) mode.

The following commands lets you scan, connect, and ping to a desired network or access point using the Wi-Fi Shell sample.

1. Scan all the access points in the vicinity:

   .. code-block:: shell

      uart:~$ wifi scan

#. Connect to the desired access point using SSID from the scan command:

   .. code-block:: shell

      uart:~$ wifi connect -s <SSID> -k <key_management> -p <passphrase>

#. Query the status of the connection:

   .. code-block:: shell

      uart:~$ wifi status

#. Once the connection is established and you want to fix the data rate:

   .. code-block:: shell

      uart:~$ nrf70 util tx_rate <frame_format> <rate_val>

#. When the connection is established, you can run network tools like ping:

   .. code-block:: shell

      uart:~$ net ping 10 192.168.1.100

#. To disconnect:

   .. code-block:: shell

      uart:~$ wifi disconnect

Setting the MCS rate
====================

``tx_rate`` sets TX data rate to a fixed value or ``AUTO``.

Parameters:

.. code-block::

   <frame_format>: The TX data rate type to be set,

   where:

   0 - LEGACY
   1 - HT
   2 - VHT
   3 - HE_SU
   4 - HE_ER_SU
   5 - AUTO

.. code-block::

   <rate_val>: The TX data rate value to be set, valid values are:

   Legacy : <1, 2, 55, 11, 6, 9, 12, 18, 24, 36, 48, 54>
   Non-legacy: <MCS index value between 0 - 7>
   AUTO: <No value needed>
