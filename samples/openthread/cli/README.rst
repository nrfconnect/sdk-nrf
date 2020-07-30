.. _ot_cli_sample:

Thread: CLI
###########

The Thread CLI sample demonstrates the usage of OpenThread Command Line Interface inside the Zephyr shell.

Overview
********

The sample demonstrates the usage of commands listed in `OpenThread CLI Reference`_.
OpenThread CLI is integrated into the system shell accessible over serial connection.
To indicate a Thread command, the ``ot`` keyword needs to precede the command.

The amount of commands you can test depends on the application configuration.
The CLI sample comes with the :ref:`full set of OpenThread functionalities <thread_ug_feature_sets>` enabled (:option:`CONFIG_OPENTHREAD_NORDIC_LIBRARY_MASTER`).

If used alone, the sample allows you to test the network status.
It is recommended to use at least two development kits running the same sample to be able to test communication.

An additional functionality of this particular sample is the possibility of updating OpenThread libraries
with the version compiled from the current source.

See :ref:`ug_thread_cert` for information on how to use this sample on Thread Certification Test Harness.

.. _ot_cli_sample_diag_module:

Diagnostic module
=================

By default, the CLI sample comes with the :option:`CONFIG_OPENTHREAD_NORDIC_LIBRARY_MASTER` :ref:`configuration set <thread_ug_feature_sets>` enabled, which allows you to use Zephyr's diagnostic module with its ``diag`` commands.
Use these commands for manually checking hardware-related functionalities without running a Thread network.
For example, when adding a new functionality or during the manufacturing process to ensure radio communication is working.
See `Testing diagnostic module`_ section for an example.

.. note::
    If you disable the :option:`CONFIG_OPENTHREAD_NORDIC_LIBRARY_MASTER` configuration set, you can enable the diagnostic module with the :option:`CONFIG_OPENTHREAD_DIAG` Kconfig option.


Requirements
************

The sample supports the following development kits for testing the network status:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf52840dk_nrf52840, nrf52833dk_nrf52833

Optionally, you can use one or more compatible development kits programmed with this sample or another :ref:`Thread sample <openthread_samples>` for :ref:`testing communication or diagnostics <ot_cli_sample_testing_multiple>` and :ref:`thread_ot_commissioning_configuring_on-mesh`.

User interface
**************

All the interactions with the application are handled using serial communication.
See `OpenThread CLI Reference`_ for the list of available serial commands.

Building and running
********************

.. |sample path| replace:: :file:`samples/openthread/cli`

|enable_thread_before_testing|

.. include:: /includes/build_and_run.txt

To update OpenThread libraries provided by the ``nrfxlib``, please invoke ``west build -b nrf52840dk_nrf52840 -t install_openthread_libraries``.

Testing
=======

After building the sample and programming it to your development kit, test it by performing the following steps:

#. Turn on the development kit.
#. Set up the serial connection with the development kit.
   For more details, see :ref:`putty`.

   .. note::
        |thread_hwfc_enabled|

#. Invoke some of the OpenThread commands:

   a. Test the state of the Thread network with the ``ot state`` command.
      For example:

      .. code-block:: console

         uart:~$ ot state
         router
         Done

   #. Get the Thread network name with the ``ot networkname`` command.
      For example:

      .. code-block:: console

         uart:~$ ot networkname
         ot_zephyr
         Done

   #. Get the IP addresses of the current thread network with the ``ot ipaddr`` command.
      For example:

      .. code-block:: console

         uart:~$ ot ipaddr
         fdde:ad00:beef:0:0:ff:fe00:800
         fdde:ad00:beef:0:3102:d00b:5cbe:a61
         fe80:0:0:0:8467:5746:a29f:1196
         Done

.. _ot_cli_sample_testing_multiple:

Testing with more boards
------------------------

If you are using more than one development kit for testing the CLI sample, you can also complete the following testing procedures:

.. contents::
    :local:
    :depth: 1

The following testing procedures assume you are using two development kits.

Testing communication between boards
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

To test communication between boards, complete the following steps:

#. Make sure both development kits are programmed with the CLI sample.
#. Turn on the developments kits.
#. Set up the serial connection with both development kits.
   For more details, see :ref:`putty`.

   .. note::
        |thread_hwfc_enabled|

#. Test communication between the boards with the ``ot ping <ip_address_of_the_first_board>`` command.
   For example:

   .. code-block:: console

      uart:~$ ot ping fdde:ad00:beef:0:3102:d00b:5cbe:a61
      16 bytes from fdde:ad00:beef:0:3102:d00b:5cbe:a61: icmp_seq=1 hlim=64 time=22ms

Testing diagnostic module
~~~~~~~~~~~~~~~~~~~~~~~~~

To test diagnostic commands, complete the following steps:

#. Make sure both development kits are programmed with the CLI sample.
#. Turn on the developments kits.
#. Set up the serial connection with both development kits.
   For more details, see :ref:`putty`.

   .. note::
        |thread_hwfc_enabled|.

#. Make sure that the diagnostic module is enabled and configured with proper radio channel and transmission power by running the following commands on both devices:

   .. code-block:: console

      uart:~$ ot diag start
      start diagnostics mode
      status 0x00
      Done
      uart:~$ ot diag channel 11
      set channel to 11
      status 0x00
      Done
      uart:~$ ot diag power 0
      set tx power to 0 dBm
      status 0x00
      Done

#. Transmit a fixed number of packets with the given length from one of the devices.
   For example, to transmit 20 packets that contain 100 B of random data, run the following command:

   .. code-block:: console

      uart:~$ ot diag send 20 100
      sending 0x14 packet(s), length 0x64
      status 0x00
      Done

#. Read the radio statistics on the other device by running the following command:

   .. code-block:: console

      uart:~$ ot diag stats
      received packets: 20
      sent packets: 0
      first received packet: rssi=-29, lqi=255
      last received packet: rssi=-30, lqi=255
      Done

Dependencies
************

This sample uses the following Zephyr libraries:

* :ref:`zephyr:kernel_api`:

  * ``include/kernel.h``

* :ref:`zephyr:thread_protocol_interface`
