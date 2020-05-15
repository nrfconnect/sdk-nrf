.. _coap_client_sample:

Thread: CoAP Client
###################

The Thread CoAP Client sample demonstrates controlling light resources of other nodes within an OpenThread network.
To show this interaction, the sample requires a server sample that is compatible with the OpenThread network and has a light resource available.
The recommended server sample referenced on this page is :ref:`coap_server_sample`.

Overview
********

This sample demonstrates how to access resources available on a Thread server node.
After the Thread network is established, the client node can control the state of LED 4 on server nodes.
It can turn on the LED either on every server node in the network with a multicast message, or on a single specific server node that is paired with the client node.

The following CoAP resources are accessed on the server side:

* ``/light`` - Used to control LED 4.
* ``/provisioning`` - Used to perform provisioning.

This sample uses :ref:`Zephyr CoAP API<zephyr:coap_sock_interface>` for communication, which is the preferred API to use for new CoAP applications.
For example usage of the native Thread CoAp API, see the :ref:`coap_server_sample` sample.

Requirements
************

* One or more of the following development kits for Thread CoAP Client:

  * |nRF52840DK|
  * |nRF52833DK|

* One or more compatible development kits programmed with the :ref:`coap_server_sample` sample.

User interface
**************

The following LED and buttons of the client development kit are used by this sample:

LED 1:
  On when the OpenThread connection is established.

Button 1:
  Pressing results in sending a unicast LIGHT_TOGGLE message to the ``/light`` resource on a paired server node.
  If no server node is paired with the specific client node, pressing the button has no effect.

Button 2:
  Pressing results in sending a multicast LIGHT_ON or a multicast LIGHT_OFF message (alternatively) to the ``/light`` resource on server nodes.
  Sending this kind of message instead of a LIGHT_TOGGLE one allows to synchronize the state of the LEDs on several server nodes at once.

Button 4:
  Pressing results in sending a multicast pairing request to the ``/provisioning`` resource on other nodes within the network.

Minimal Thread Device assignments
=================================

When the device is working as Minimal Thread Device (MTD), the following LED and Button assignments are also available:

LED 3:
    Indicates the mode the device is working in:

    * On when in the Minimal End Device (MED) mode.
    * Off when in the Sleepy End Device (SED) mode.

Button 3:
    Pressing results in toggling the power consumption between SED and MED.

For more information, see :ref:`thread_ug_device_type` in the Thread user guide.


Building and running
********************
.. |sample path| replace:: :file:`samples/openthread/coap_client`

|enable_thread_before_testing|

.. include:: /includes/build_and_run.txt

.. note::
   To activate the Minimal Thread Device variant of this sample, modify :makevar:`CONF_FILE` by applying ``overlay-mtd.conf``.
   For more information, see :ref:`important-build-vars` in the Zephyr documentation.

Testing
=======

After building the sample and programming it to your development kit, test it by performing the following steps:

#. Program at least one development kit with :ref:`coap_server_sample` and reset it.
#. Turn on the Simple CoAP Client node.
   This node becomes the Thread network Leader.
#. Turn on all the other nodes, including the Simple CoAP Server nodes.
   They enter the network as Children, and will gradually become Routers.

   .. note::
      It can take up to 15 seconds for Thread to establish network.

#. Press Button 2 on a client node to control LED 4 on all server nodes.
#. Pair a client with a server by completing the following steps:

   a. Press Button 4 on a server node to enable pairing.
   #. Press Button 4 on a client node to pair it with the server node in the pairing mode.

#. Press Button 1 on the client node to control the LED 4 on the paired server node.

Testing Minimal Thread Device
-----------------------------

After building the MTD variant of this sample and programming it, the device starts in the SED mode with LED 3 disabled.
You can switch to the MED mode at any moment during the standard testing procedure.

To toggle MED, press Button 3 on the client node.
LED 3 turns on to indicate the switch to the MED mode.
At this point, the radio is enabled and power consumption increases.

Pressing Button 3 again will switch the mode back to SED.
This does not affect the standard testing procedure.

Running OpenThread CLI commands
-------------------------------

You can connect to any of the Simple CoAP Server or Simple CoAP Client nodes through a serial port and run OpenThread CLI commands.
For more details, see :ref:`putty`.

.. note::
    In Zephyr shell, every OpenThread command needs to be prepended with the `ot` keyword.
    For example, ``ot channel 20``.

For complete CLI documentation, refer to `OpenThread CLI Reference`_.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`dk_buttons_and_leds_readme`

In addition, it uses the following Zephyr libraries:

* :ref:`CoAP <zephyr:coap_sock_interface>`

* :ref:`zephyr:kernel_api`:

  * ``include/kernel.h``
