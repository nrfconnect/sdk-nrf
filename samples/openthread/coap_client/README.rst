.. _coap_client_sample:

Thread: CoAP Client
###################

.. contents::
   :local:
   :depth: 2

The :ref:`Thread <ug_thread>` CoAP Client sample demonstrates controlling light resources of other nodes within an OpenThread network.
To show this interaction, the sample requires a server sample that is compatible with the OpenThread network and has a light resource available.
The recommended server sample referenced on this page is :ref:`coap_server_sample`.

This sample supports optional :ref:`coap_client_sample_multi_ext` and :ref:`Minimal Thread Device variant <thread_ug_device_type>`, which can be turned on or off.
See :ref:`coap_client_sample_activating_variants` for details.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

You can use one or more of the development kits listed above as the Thread CoAP Client.
You also need one or more compatible development kits programmed with the :ref:`coap_server_sample` sample.

.. include:: /includes/tfm.txt

Multiprotocol extension requirements
====================================

If you enable the :ref:`coap_client_sample_multi_ext`, make sure you have a phone with the `nRF Toolbox`_ application installed or an additional development kit programmed with :ref:`central_uart` sample.

.. note::
  The :ref:`testing instructions <coap_client_sample_testing_ble>` refer to nRF Toolbox, but similar applications can be used as well, for example `nRF Connect for Mobile`_.

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

.. _coap_client_sample_multi_ext:

Multiprotocol Bluetooth LE extension
====================================

This optional extension demonstrates the OpenThread stack and :ref:`nrfxlib:softdevice_controller` working concurrently.
It uses the :ref:`nus_service_readme` library to control the LED states over Bluetooth® LE in a Thread network.
For more information about the multiprotocol feature, see :ref:`ug_multiprotocol_support`.

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
    The mode the device is working in:

    * On when in the Minimal End Device (MED) mode.
    * Off when in the Sleepy End Device (SED) mode.

Button 3:
    Toggle the power consumption between SED and MED.

For more information, see :ref:`thread_ug_device_type` in the Thread user guide.

Multiprotocol Bluetooth LE extension assignments
================================================

LED 2:
   On when Bluetooth LE connection is established.

UART command assignments:
   The following command assignments are configured and used in nRF Toolbox when :ref:`coap_client_sample_testing_ble`:

   * ``u`` - Send a unicast CoAP message over Thread (the same operation as **Button 1**).
   * ``m`` - Send a multicast CoAP message over Thread (the same operation as **Button 2**).
   * ``p`` - Send a pairing request as CoAP message over Thread (the same operation as **Button 4**).

Configuration
*************

|config|

.. _coap_client_sample_activating_variants:

Snippets
========

.. include:: /includes/sample_snippets.txt

The following snippets are available:

* ``debug`` - Enables debugging the Thread sample by enabling :c:func:`__ASSERT()` statements globally.
* ``mtd`` - Enables the Minimal Thread Device variant.
* ``logging`` - Enables logging using RTT.
  For additional options, refer to :ref:`RTT logging <ug_logging_backends_rtt>`.
* ``multiprotocol_ble`` - Enables the Multiprotocol Bluetooth LE extension.

FEM support
===========

.. include:: /includes/sample_fem_support.txt


Building and running
********************

.. |sample path| replace:: :file:`samples/openthread/coap_client`

|enable_thread_before_testing|

.. include:: /includes/build_and_run_ns.txt

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

After building the MTD variant of this sample and programming it, the device starts in the SED mode with **LED 3** off.
This means that the radio is disabled when idle and the serial console is not working to decrease the power consumption.
You can switch to the MED mode at any moment during the standard testing procedure.

To toggle MED, press **Button 3** on the client node.
**LED 3** turns on to indicate the switch to the MED mode.
At this point, the radio is enabled when it is idle and the serial console is operating.

Pressing **Button 3** again will switch the mode back to SED.
Switching between SED and MED modes does not affect the standard testing procedure, but terminal logs are not available in the SED mode.

.. _coap_client_sample_testing_ble:

Testing multiprotocol Bluetooth LE extension
--------------------------------------------

To test the multiprotocol Bluetooth LE extension, you can use nRF Toolbox or the :ref:`central_uart` sample.
The steps below assume nRF Toolbox as the Bluetooth tester.

First, you need to set up nRF Toolbox as follows:

1. Tap :guilabel:`UART` to open the UART application in nRF Toolbox.

   .. figure:: /images/nrftoolbox_uart_default.png
      :alt: UART application in nRF Toolbox

      UART application in nRF Toolbox

#. Tap the :guilabel:`EDIT` button in the top right corner of the application to configure the UART commands.
   The button configuration window appears.

#. Create the active application buttons by completing the following steps:

   a. Bind the top left button to the ``u`` command, with EOL set to LF and an icon of your choice.
      For this testing procedure, the :guilabel:`>` icon is used.
   #. Bind the top middle button to the ``m`` command, with EOL set to LF and an icon of your choice.
      For this testing procedure, the play button icon is used.
   #. Bind the top right button to the ``p`` command, with EOL set to LF and an icon of your choice.
      For this testing procedure, the settings gear icon is used.

      .. figure:: /images/nrftoolbox_uart_settings.png
         :alt: Configuring buttons in nRF Toolbox - UART application

         Configuring buttons in the UART application of nRF Toolbox

#. Tap the :guilabel:`DONE` button in the top right corner of the application.

#. Tap :guilabel:`CONNECT` and select the ``NUS_CoAP_client`` device from the list to connect to the device.

   .. figure:: /images/nrftoolbox_uart_connected.png
      :alt: nRF Toolbox - UART application view after establishing connection

      The UART application of nRF Toolbox after establishing the connection

   .. note::
      Observe that **LED 2** on your CoAP Multiprotocol Client node lights up, which indicates that the Bluetooth connection is established.

When you have set up nRF Toolbox, complete the following steps after the standard `Testing`_ procedure:

#. In nRF Toolbox, tap the middle button to control **LED 4** on all CoAP server nodes.
#. To pair a client with a server, complete the following steps:

   a. Press **Button 4** on a server node to enable pairing.
   #. In nRF Toolbox, tap the right button to pair the two nodes.

#. In nRF Toolbox, tap the left button to control **LED 4** on the paired server node.

Sample output
=============

You can observe the sample logging output with a terminal emulator |ANSI| (for example, nRF Connect Serial Terminal).
See :ref:`test_and_optimize` for the required settings and steps.

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

The following dependencies are added by the optional multiprotocol Bluetooth LE extension:

* :ref:`nrfxlib:softdevice_controller`
* :ref:`nus_service_readme`
* Zephyr's :ref:`zephyr:bluetooth_api`:

  * ``include/bluetooth/bluetooth.h``
  * ``include/bluetooth/gatt.h``
  * ``include/bluetooth/hci.h``
  * ``include/bluetooth/uuid.h``

In addition, it uses the following secure firmware component:

* :ref:`Trusted Firmware-M <ug_tfm>`
