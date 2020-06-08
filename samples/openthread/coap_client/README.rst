.. _coap_client_sample:

Thread: CoAP Client
###################

The Thread CoAP Client sample demonstrates controlling light resources of other nodes within an OpenThread network.
To show this interaction, the sample requires a server sample that is compatible with the OpenThread network and has a light resource available.
The recommended server sample referenced on this page is :ref:`coap_server_sample`.

.. note::
    This sample supports the Minimal Thread Device variant.
    See :ref:`coap_client_sample_activating_variants` for details.

Overview
********

This sample demonstrates how to access resources available on a Thread server node.
After the Thread network is established, the client node can control the state of **LED 4** on server nodes.
It can turn on the LED either on every server node in the network with a multicast message, or on a single specific server node that is paired with the client node.

The following CoAP resources are accessed on the server side:

* ``/light`` - Used to control **LED 4**.
* ``/provisioning`` - Used to perform provisioning.

This sample uses :ref:`Zephyr CoAP API<zephyr:coap_sock_interface>` for communication, which is the preferred API to use for new CoAP applications.
For example usage of the native Thread CoAP API, see the :ref:`coap_server_sample` sample.

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
   Send a unicast ``LIGHT_TOGGLE`` message to the ``/light`` resource on a paired device.
   If no device is paired with the specific client node, pressing the button has no effect.

Button 2:
   Send a multicast ``LIGHT_ON`` or ``LIGHT_OFF`` message (alternatively) to the ``/light`` resource.
   Sending this multicast message instead of ``LIGHT_TOGGLE`` allows to synchronize the state of the LEDs on several server nodes.

Button 4:
   Send a multicast pairing request to the ``/provisioning`` resource.

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

.. _coap_client_sample_activating_variants:

Activating sample extensions
============================

To activate the extensions supported by this sample, modify :makevar:`CONF_FILE` in the following manner:

* For the Minimal Thread Device variant, apply :file:`overlay-mtd.conf`.

For more information, see :ref:`important-build-vars` in the Zephyr documentation.

Testing
=======

After building the sample and programming it to your development kit, test it by performing the following steps:

#. Program at least one development kit with :ref:`coap_server_sample` and reset it.
#. Turn on the CoAP Client and at least one CoAP Server.
   They will create the Thread network.

   .. note::
     It may take up to 15 seconds for nodes to establish the network.
     When the sample application is ready, the **LED 1** starts blinking.

#. Press **Button 2** on a client node to control **LED 4** on all server nodes.
#. Pair a client with a server by completing the following steps:

   a. Press **Button 4** on a server node to enable pairing.
   #. Press **Button 4** on a client node to pair it with the server node in the pairing mode.

#. Press **Button 1** on the client node to control the **LED 4** on the paired server node.

.. _coap_client_sample_testing_mtd:

Testing Minimal Thread Device
-----------------------------

After building the MTD variant of this sample and programming it, the device starts in the SED mode with LED 3 disabled.
You can switch to the MED mode at any moment during the standard testing procedure.

To toggle MED, press **Button 3** on the client node.
**LED 3** turns on to indicate the switch to the MED mode.
At this point, the radio is enabled and power consumption increases.

Pressing **Button 3** again will switch the mode back to SED.
This does not affect the standard testing procedure.

Sample output
=============

The sample logging output can be observed through a serial port.
For more details, see :ref:`putty`.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`dk_buttons_and_leds_readme`
* :ref:`coap_utils_readme`

In addition, it uses the following Zephyr libraries:

* :ref:`zephyr:coap_sock_interface`

  * ``include/net/coap.h``

* :ref:`zephyr:logging_api`:

  * ``include/logging/log.h``

* :ref:`zephyr:kernel_api`:

  * ``include/kernel.h``
