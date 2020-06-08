.. _ot_cli_sample:

Thread: CLI sample
##################

The Thread CLI sample demonstrates the usage of OpenThread Command Line Interface inside the Zephyr shell.

Overview
********

The sample demonstrates the usage of commands listed in `OpenThread CLI Reference`_.
OpenThread CLI is integrated into the system shell accessible over serial connection.
To indicate a Thread command, the ``ot`` keyword needs to precede the command.

The amount of commands you can test depends on the application configuration.
This sample has Commissioner, Joiner, and Border Router roles enabled by default.

If used alone, the sample allows you to test the network status.
It is recommended to use at least two development kits running the same sample to be able to test communication.

Requirements
************

* One of the following development kits for testing the network status:

  * |nRF52840DK|
  * |nRF52833DK|

* Optionally, one or more compatible development kits programmed with this sample or another :ref:`Thread sample <openthread_samples>` for testing communication and :ref:`thread_ot_commissioning_configuring_on-mesh`.

User interface
**************

All the interactions with the application are handled using serial communication.
See `OpenThread CLI Reference`_ for the list of available serial commands.

Building and running
********************

.. |sample path| replace:: :file:`samples/openthread/cli`

|enable_thread_before_testing|

.. include:: /includes/build_and_run.txt

Testing
=======

After building the sample and programming it to your development kit, test it by performing the following steps:

#. Turn on the development kit.
#. Set up the serial connection with the development kit.
   For more details, see :ref:`putty`.
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

#. Optionally, if you are using more than one development kit, complete the following additional steps:

   a. Connect to another board using serial connection.
   #. Test communication between the boards with the ``ot ping <ip_address_of_the_first_board>`` command.
      For example:

      .. code-block:: console

         uart:~$ ot ping fdde:ad00:beef:0:3102:d00b:5cbe:a61
         16 bytes from fdde:ad00:beef:0:3102:d00b:5cbe:a61: icmp_seq=1 hlim=64 time=22ms


Dependencies
************

This sample uses the following Zephyr libraries:

* :ref:`zephyr:kernel_api`:

  * ``include/kernel.h``

* :ref:`zephyr:thread_protocol_interface`
