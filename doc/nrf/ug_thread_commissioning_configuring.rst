.. _thread_ot_commissioning_configuring_on-mesh:

Configuring on-mesh Thread commissioning with CLI sample
########################################################

You can configure on-mesh Thread commissioning using the :ref:`Thread CLI sample <ot_cli_sample>` with two devices to form a Thread network.
One device will act as a Commissioner and the other will be a Joiner.

.. note::
    Before you start the configuration process, make sure you are familiar with :ref:`Thread commissioning concepts <thread_ot_commissioning>`, especially :ref:`thread_ot_commissioning_types_on-mesh`.

.. _thread_ot_commissioning_configuring_on-mesh_cli_requirements:

Requirements
============

* Two of the following development kits:

  * |nRF52840DK|
  * |nRF52833DK|

.. _thread_ot_commissioning_configuring_on-mesh_cli_flashing:

Programming the DKs
===================

Flash both development kits with the :ref:`Thread CLI sample <ot_cli_sample>`.
See the sample's page for details.

After programming the DKs and turning them on, both devices will be pre-commissioned and will form a Thread network.
This network needs to be manually disabled.

Disabling the Thread network
============================

The |NCS|'s Thread CLI sample comes with the autostart feature, which means that the devices will form the network automatically without user intervention.
To properly observe the commissioning process, it is recommended to form a new Thread network manually.

To disconnect from the network before before starting the commissioning process, run the following command:

.. code-block:: console

   uart:~$ ot thread stop

.. _thread_ot_commissioning_configuring_on-mesh_cli_forming:

Forming a new network
=====================

To form a new Thread network, create a new dataset on one of the devices and set it as active by entering the following commands:

.. code-block:: console

   uart:~$ ot dataset init new
   Done
   uart:~$ ot dataset commit active
   Done

To view the newly generated network settings, enter the ``dataset`` command, for example:

.. code-block:: console

   uart:~$ ot dataset
   Active Timestamp: 1
   Channel: 23
   Channel Mask: 07fff800
   Ext PAN ID: 36dd32babd209538
   Mesh Local Prefix: fd51:51f2:fb58:c849/64
   Master Key: 0278f75cb81f04834f09b5fc095852d6
   Network Name: OpenThread-8299
   PAN ID: 0x8299
   PSKc: 658f3f958bade7db07a36c3fbf2fa2c9
   Security Policy: 0, onrcb
   Done

Both devices have now different network settings, which means that it is impossible for either device to communicate with the other.
This allows for adding a device to the network using on-mesh commissioning.

.. _thread_ot_commissioning_configuring_on-mesh_cli_commissioning:

Performing on-mesh commissioning
================================

Complete the following steps:

1. Start the newly configured Thread network on the first device, which will become the Commissioner:

   a. Run the following command:

      .. code-block:: console

         uart:~$ ot ifconfig up
         Done
         uart:~$ ot thread start
         Done

   #. After a couple of seconds, check the state:

      .. code-block:: console

         uart:~$ ot state
         Leader
         Done

   On-mesh commissioning can be now used to add the second CLI device to the newly formed Thread network.
   The device that formed the network takes the role of the Commissioner.
   The second device will become the Joiner.
#. Perform the on-mesh commissioning:

   a. Retrieve the ``EUI64`` identifier from the Joiner:

      .. code-block:: console

         uart:~$ ot eui64
         f4ce3687a6e4f6e8
         Done

   #. Start the Commissioner:

      .. code-block:: console

         uart:~$ ot commissioner start
         Done

   #. Give the Commissioner the ``EUI64`` identifier of the Joiner and set up a pre-shared key (see :ref:`thread_ot_commissioning_roles_authentication` for encoding limitations):

      .. code-block:: console

         uart:~$ ot commissioner joiner add <eui64> <pre-shared base32-thread key>

      For example:

      .. code-block:: console

         uart:~$ ot commissioner joiner add f4ce3687a6e4f6e8 N0RD1C
         Done

      The Commissioner starts listening for the specified Joiner.
   #. Start the Joiner:

      .. code-block:: console

         uart:~$ ot ifconfig up
         Done
         uart:~$ ot joiner start <pre-shared base32-thread key>
         Done

      For example:

      .. code-block:: console

         uart:~$ ot ifconfig up
         Done
         uart:~$ ot joiner start N0RD1C
         Done

      After a couple of seconds, the following message appears:

      .. code-block:: console

         Join success

      The Joiner starts broadcasting Discovery Requests on all available channels.
      When the Commissioner receives a Discovery Request, it responds to the sender.
      After the response, a DTLS session is established to securely authenticate the Joiner and exchange the network credentials.
   #. After a successful joining process, attach the newly added device to the Thread network with the following command:

      .. code-block:: console

         uart:~$ ot thread start
         Done

Both devices are now able to ping each other.

----

Copyright disclaimer
    |Google_CCLicense|
