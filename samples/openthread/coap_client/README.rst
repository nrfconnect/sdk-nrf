.. _coap_client_sample:

Thread: CoAP Client
###################

The Thread CoAP Client sample demonstrates controlling light resources of other nodes within an OpenThread network.
To show this interaction, the sample requires a server sample that is compatible with the OpenThread network and has a light resource available.
The recommended server sample referenced on this page is :ref:`coap_server_sample`.

.. note::
    This sample supports :ref:`coap_client_sample_multi_ext` and Minimal Thread Device variant.
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

.. _coap_client_sample_multi_ext:

Multiprotocol |BLE| extension
=============================

This extension demonstrates the OpenThread stack and :ref:`nRF Bluetooth LE Controller <nrfxlib:ble_controller>` working concurrently.
It uses the :ref:`nus_service_readme` library to control the LED states over |BLE| in a Thread network.

Requirements
************

* One or more of the following development kits for Thread CoAP Client:

  * |nRF52840DK|
  * |nRF52833DK|

* One or more compatible development kits programmed with the :ref:`coap_server_sample` sample.

Multiprotocol extension requirements
====================================

* A phone or tablet with the `nRF Toolbox`_ application installed.

  .. note::
    The :ref:`testing instructions <coap_client_sample_testing_ble>` refer to nRF Toolbox, but similar applications can be used as well, for example `nRF Connect for Mobile`_.

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

Multiprotocol |BLE| extension assignments
=========================================

LED 2:
   On when |BLE| connection is established.

UART command assignments:
   The following command assignments are configured and used in nRF Toolbox:

   * ``u`` - Send a unicast CoAP message over Thread (the same operation as **Button 1**).
   * ``m`` - Send a multicast CoAP message over Thread (the same operation as **Button 2**).
   * ``p`` - Send a pairing request as CoAP message over Thread (the same operation as **Button 4**).

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
* For the Multiprotocol BLE extension, apply :file:`overlay-multiprotocol_ble.conf`.

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

.. _coap_client_sample_testing_ble:

Testing multiprotocol |BLE| extension
-------------------------------------

To test the multiprotocol |BLE| extension, complete the following steps after the standard `Testing`_ procedure:

#. Set up nRF Toolbox by completing the following steps:

   .. tabs::

      .. tab:: 1. Start UART

         Tap :guilabel:`UART` to open the UART application in nRF Toolbox.

         .. figure:: /images/nrftoolbox_uart_default.png
            :alt: UART application in nRF Toolbox

            UART application in nRF Toolbox

      .. tab:: 2. Configure commands

         Configure the UART commands by completing the following steps:

         1. Tap the :guilabel:`EDIT` button in the top right corner of the application.
            The button configuration window appears.
         #. Create the active application buttons by completing the following steps:

            a. Bind the top left button to the ``u`` command, with EOL set to LF and an icon of your choice.
               For this testing procedure, the :guilabel:`>` icon is used.
            #. Bind the top middle button to the ``m`` command, with EOL set to LF and an icon of your choice.
               For this testing procedure, the play button icon is used.
            #. Bind the top left button to the ``p`` command, with EOL set to LF and an icon of your choice.
               For this testing procedure, the settings gear icon is used.

            .. figure:: /images/nrftoolbox_uart_settings.png
               :alt: Configuring buttons in nRF Toolbox - UART application

               Configuring buttons in the UART application of nRF Toolbox

         #. Tap the :guilabel:`DONE` button in the top right corner of the application.

      .. tab:: 3. Connect to device

         Tap :guilabel:`CONNECT` and select the ``NUS_CoAP_client`` device from the list of devices.

         .. figure:: /images/nrftoolbox_uart_connected.png
            :alt: nRF Toolbox - UART application view after establishing connection

            The UART application of nRF Toolbox after establishing the connection

         .. note::
            Observe that **LED 2** on your CoAP Multiprotocol Client node is solid, which indicates that the Bluetooth connection is established.
   ..

#. In nRF Toolbox, press the middle button to control **LED 4** on all CoAP server nodes.
#. Pair a client with a server by completing the following steps:

   a. Press **Button 4** on a server node to enable pairing.
   #. In nRF Toolbox, press the right button to pair the two nodes.

#. In nRF Toolbox, press the left button to control **LED 4** on the paired server node.

Sample output
=============

The sample logging output can be observed through a serial port.
For more details, see :ref:`putty`.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`nRF Bluetooth LE Controller <nrfxlib:ble_controller>`
* :ref:`nus_service_readme`
* :ref:`dk_buttons_and_leds_readme`
* :ref:`coap_utils_readme`

In addition, it uses the following Zephyr libraries:

* :ref:`zephyr:coap_sock_interface`

  * ``include/net/coap.h``

* :ref:`zephyr:logging_api`:

  * ``include/logging/log.h``

* :ref:`zephyr:kernel_api`:

  * ``include/kernel.h``

* :ref:`zephyr:bluetooth_api`:

  * ``include/bluetooth/bluetooth.h``
  * ``include/bluetooth/gatt.h``
  * ``include/bluetooth/hci.h``
  * ``include/bluetooth/uuid.h``
